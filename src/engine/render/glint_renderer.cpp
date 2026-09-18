#include "glint_renderer.h"
#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>

namespace
{
    /// @brief 预计算 512 项条纹亮度表：亮度(t) = 0.5+0.5·cos(2π·t)，t∈[0,1)
    /// 相位归一化后查表替代逐像素 std::cos（~70 cycles → ~5 cycles，约 10 倍），
    /// 512 档量化下的亮度误差 < 0.01，经 uint8 截断后视觉不可见。
    const std::array<float, 512> &luminanceTable() noexcept
    {
        static const std::array<float, 512> table = []
        {
            std::array<float, 512> t{};
            constexpr double kTwoPi = 6.283185307179586;
            for (size_t i = 0; i < t.size(); ++i)
            {
                t[i] = static_cast<float>(0.5 + 0.5 * std::cos(kTwoPi * static_cast<double>(i) / 512.0));
            }
            return t;
        }();
        return table;
    }
} // namespace

namespace engine::render
{

    GlintRenderer::GlintRenderer(SDL_Renderer *renderer) noexcept
        : renderer_(renderer)
    {
        if (!renderer_)
        {
            spdlog::warn("GlintRenderer 构造：渲染器指针为空，流光将不可用");
            return;
        }
        spdlog::trace("GlintRenderer 构造完成");
    }

    GlintRenderer::~GlintRenderer()
    {
        clearCache();
        spdlog::trace("GlintRenderer 析构，已释放全部纹理");
    }

    size_t GlintRenderer::AlphaKeyHash::operator()(const AlphaKey &key) const noexcept
    {
        size_t hash = std::hash<entt::id_type>{}(key.texture_id);
        const auto mix = [&hash](float value)
        {
            const uint32_t bits = std::bit_cast<uint32_t>(value);
            hash ^= std::hash<uint32_t>{}(bits) + 0x9e3779b9u + (hash << 6u) + (hash >> 2u);
        };
        mix(key.src_x);
        mix(key.src_y);
        mix(key.src_w);
        mix(key.src_h);
        return hash;
    }

    size_t GlintRenderer::SizeKeyHash::operator()(const SizeKey &key) const noexcept
    {
        size_t hash = std::hash<int>{}(key.width);
        hash ^= std::hash<int>{}(key.height) + 0x9e3779b9u + (hash << 6u) + (hash >> 2u);
        return hash;
    }

    SDL_Texture *GlintRenderer::getOffscreenTarget(int width, int height) noexcept
    {
        if (width <= 0 || height <= 0)
        {
            return nullptr;
        }
        // 尺寸变化或尚未创建时重建离屏目标
        if (!offscreen_target_ || offscreen_width_ != width || offscreen_height_ != height)
        {
            offscreen_target_.reset(); // 先释放旧纹理（RAII）
            offscreen_target_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ABGR8888,
                                                      SDL_TEXTUREACCESS_TARGET, width, height));
            if (!offscreen_target_)
            {
                spdlog::error("创建离屏渲染目标失败 ({}x{}): {}", width, height, SDL_GetError());
                return nullptr;
            }
            offscreen_width_ = width;
            offscreen_height_ = height;
            spdlog::trace("离屏渲染目标重建: {}x{}", width, height);
        }
        return offscreen_target_.get();
    }

    std::vector<uint8_t> GlintRenderer::readSourceAlpha(SDL_Texture *src_texture,
                                                        const component::Sprite &sprite) noexcept
    {
        const int src_w = static_cast<int>(sprite.src_rect_.size.x);
        const int src_h = static_cast<int>(sprite.src_rect_.size.y);
        if (src_w <= 0 || src_h <= 0)
        {
            spdlog::warn("readSourceAlpha: 无效的源矩形尺寸 {}x{}", src_w, src_h);
            return {};
        }

        SDL_Texture *target = getOffscreenTarget(src_w, src_h);
        if (!target)
        {
            return {};
        }

        // 将源精灵的 src_rect 区域绘制到离屏目标（目标尺寸与区域一致，dst 传 nullptr 按原尺寸绘制）
        if (!SDL_SetRenderTarget(renderer_, target))
        {
            spdlog::error("切换渲染目标失败: {}", SDL_GetError());
            return {};
        }
        SDL_SetRenderDrawColorFloat(renderer_, 0.0f, 0.0f, 0.0f, 0.0f);
        SDL_RenderClear(renderer_);
        const SDL_FRect src_rect = {sprite.src_rect_.position.x, sprite.src_rect_.position.y,
                                    static_cast<float>(src_w), static_cast<float>(src_h)};
        if (!SDL_RenderTexture(renderer_, src_texture, &src_rect, nullptr))
        {
            spdlog::error("绘制源纹理到离屏目标失败: {}", SDL_GetError());
            SDL_SetRenderTarget(renderer_, nullptr);
            return {};
        }

        // 一次性读回当前渲染目标（离屏）为 surface
        SDL_Surface *surface = SDL_RenderReadPixels(renderer_, nullptr);
        if (!surface)
        {
            spdlog::error("读回离屏像素失败: {}", SDL_GetError());
            SDL_SetRenderTarget(renderer_, nullptr);
            return {};
        }
        SDL_SetRenderTarget(renderer_, nullptr);

        // 提取 alpha：离屏目标创建为 ABGR8888（小端内存字节序 R,G,B,A），alpha 位于每像素第 4 字节
        std::vector<uint8_t> alpha(static_cast<size_t>(src_w) * static_cast<size_t>(src_h));
        if (surface->format == SDL_PIXELFORMAT_ABGR8888)
        {
            const auto *pixels = static_cast<const uint8_t *>(surface->pixels);
            for (int y = 0; y < src_h; ++y)
            {
                for (int x = 0; x < src_w; ++x)
                {
                    alpha[static_cast<size_t>(y) * static_cast<size_t>(src_w) + static_cast<size_t>(x)] =
                        pixels[static_cast<size_t>(y) * static_cast<size_t>(surface->pitch) + static_cast<size_t>(x) * 4u + 3u];
                }
            }
        }
        else
        {
            // 兜底：任意格式逐像素读取 alpha（仅纹理首次提取时的一次性开销）
            for (int y = 0; y < src_h; ++y)
            {
                for (int x = 0; x < src_w; ++x)
                {
                    uint8_t r = 0, g = 0, b = 0, a = 0;
                    SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a);
                    alpha[static_cast<size_t>(y) * static_cast<size_t>(src_w) + static_cast<size_t>(x)] = a;
                }
            }
        }
        SDL_DestroySurface(surface);
        return alpha;
    }

    const std::vector<uint8_t> &GlintRenderer::getSourceAlpha(SDL_Texture *src_texture,
                                                              const component::Sprite &sprite) noexcept
    {
        const AlphaKey key{sprite.texture_id_, sprite.src_rect_.position.x, sprite.src_rect_.position.y,
                           sprite.src_rect_.size.x, sprite.src_rect_.size.y};
        if (const auto it = alpha_cache_.find(key); it != alpha_cache_.end())
        {
            return it->second;
        }
        // 首次访问：提取并缓存
        auto [it, inserted] = alpha_cache_.emplace(key, readSourceAlpha(src_texture, sprite));
        if (!inserted)
        {
            return it->second;
        }
        // emplace 成功但内容可能为空（提取失败）：空结果也缓存，避免反复失败重试
        return it->second;
    }

    SDL_Texture *GlintRenderer::getMaskedTexture(int width, int height) noexcept
    {
        const SizeKey key{width, height};
        if (const auto it = mask_cache_.find(key); it != mask_cache_.end())
        {
            return it->second.get();
        }

        SDL_Texture *raw = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ABGR8888,
                                             SDL_TEXTUREACCESS_STREAMING, width, height);
        if (!raw)
        {
            spdlog::error("创建遮罩流光纹理失败 ({}x{}): {}", width, height, SDL_GetError());
            return nullptr;
        }
        if (!SDL_SetTextureBlendMode(raw, SDL_BLENDMODE_ADD))
        {
            spdlog::warn("设置遮罩流光纹理混合模式失败: {}", SDL_GetError());
        }
        mask_cache_.emplace(key, TexturePtr(raw));
        return raw;
    }

    void GlintRenderer::draw(SDL_Texture *src_texture, const component::Sprite &sprite,
                             const glm::vec2 &position_screen, const glm::vec2 &size, float rotation,
                             const component::GlintComponent &glint) noexcept
    {
        if (!renderer_ || !src_texture || !glint.enabled_)
        {
            return;
        }
        if (glint.intensity_ <= 0.0f || glint.period_ <= 0.0f)
        {
            return;
        }

        const int src_w = static_cast<int>(sprite.src_rect_.size.x);
        const int src_h = static_cast<int>(sprite.src_rect_.size.y);
        const std::vector<uint8_t> &alpha = getSourceAlpha(src_texture, sprite);
        if (alpha.empty() || src_w <= 0 || src_h <= 0)
        {
            return;
        }

        SDL_Texture *masked = getMaskedTexture(src_w, src_h);
        if (!masked)
        {
            return;
        }

        // 每帧重建遮罩纹理：形状固定（alpha = 源 alpha），条纹相位随滚动时间平移
        // 滚动通过相位偏移编码而非 src 偏移，保证形状严格贴合、绝不漂移
        void *pixels = nullptr;
        int pitch = 0;
        if (!SDL_LockTexture(masked, nullptr, &pixels, &pitch))
        {
            spdlog::error("锁定遮罩流光纹理失败: {}", SDL_GetError());
            return;
        }
        const float scroll = std::fmod(glint.elapsed_ * glint.speed_, glint.period_);
        const auto &luminance_lut = luminanceTable();
        constexpr uint32_t kTableSize = 512u; // 2 的幂，用位与求索引
        constexpr uint32_t kTableMask = kTableSize - 1u;
        auto *dst = static_cast<uint8_t *>(pixels);
        for (int y = 0; y < src_h; ++y)
        {
            for (int x = 0; x < src_w; ++x)
            {
                // 45° 斜向条纹：相位 = x+y+scroll；归一化到 [0,1) 后查表求亮度
                const float t = std::fmod(static_cast<float>(x + y) + scroll, glint.period_) / glint.period_;
                const float luminance =
                    luminance_lut[static_cast<uint32_t>(t * static_cast<float>(kTableSize)) & kTableMask];
                const uint8_t src_alpha =
                    alpha[static_cast<size_t>(y) * static_cast<size_t>(src_w) + static_cast<size_t>(x)];
                const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(pitch) + static_cast<size_t>(x) * 4u;
                dst[idx + 0u] = 255u; // RGB 固定为白色，绘制时经 color mod 染成效果颜色
                dst[idx + 1u] = 255u;
                dst[idx + 2u] = 255u;
                dst[idx + 3u] = static_cast<uint8_t>(luminance * static_cast<float>(src_alpha)); // A = 亮度 × 源alpha
            }
        }
        SDL_UnlockTexture(masked);

        // 颜色经 color mod 即时生效，强度经 alpha mod 调整
        SDL_SetTextureColorModFloat(masked, glint.color_.r, glint.color_.g, glint.color_.b);
        SDL_SetTextureAlphaModFloat(masked, std::clamp(glint.intensity_, 0.0f, 1.0f));

        const SDL_FRect src_rect{0.0f, 0.0f, static_cast<float>(src_w), static_cast<float>(src_h)};
        const SDL_FRect dst_rect{position_screen.x, position_screen.y, size.x, size.y};
        if (!SDL_RenderTextureRotated(renderer_, masked, &src_rect, &dst_rect, rotation, nullptr,
                                      sprite.is_flipped_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE))
        {
            spdlog::error("绘制表面流光失败: {}", SDL_GetError());
        }
    }

    void GlintRenderer::clearCache() noexcept
    {
        alpha_cache_.clear();      // 释放源 alpha 缓存
        mask_cache_.clear();       // unique_ptr 自动释放全部遮罩纹理（RAII）
        offscreen_target_.reset(); // 释放离屏目标
        offscreen_width_ = 0;
        offscreen_height_ = 0;
    }

} // namespace engine::render