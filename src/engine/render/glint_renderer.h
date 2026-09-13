#pragma once
#include "../component/glint_component.h"
#include "../component/sprite_component.h"
#include "../utils/math.h"
#include <SDL3/SDL_render.h>
#include <entt/core/hashed_string.hpp>
#include <glm/vec2.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace engine::render
{
/**
 * @brief 通用表面流光渲染器（引擎渲染层）
 *
 * 在无 shader 的 SDL3 Render API 下实现"精灵表面滚动的彩色斜纹"，
 * 供附魔、选中高亮、技能充能等效果复用：
 * 1. 提取源精灵区域的 alpha 通道并缓存（离屏目标 + ReadPixels 一次性读回）；
 * 2. 维护一张与精灵区域等大的 STREAMING 遮罩纹理，每帧重建：
 *    形状严格固定（每像素 alpha = 源 alpha，光效贴合精灵、透明零污染），
 *    条纹相位随累计时间平移（斜纹亮度 = 0.5+0.5·cos(2π·(x+y+scroll)/period)），
 *    滚动通过相位偏移编码，形状不会随滚动漂移；
 * 3. 条纹亮度经预计算 512 项查表（LUT）求值，替代逐像素 std::cos，
 *    单像素开销由 ~70 cycles 降至 ~5 cycles（约 10 倍）；
 * 4. 颜色经 SDL_SetTextureColorModFloat 即时设置（不同效果颜色无需重建纹理），
 *    强度经 alpha mod 调整，绘制用 ADD 混合产生叠加发光质感。
 *
 * 资源管理遵循 RAII：所有 SDL_Texture 由 unique_ptr + 删除器持有，
 * 对象析构（或 clearCache）时自动释放。
 *
 * 无异常保证：所有失败路径记录日志并安全返回，不抛出异常。
 */
class GlintRenderer final
{
public:
    /**
     * @brief 构造函数
     * @param renderer 非拥有指针，可为空（此时 isValid() 返回 false，draw 为空操作）
     */
    explicit GlintRenderer(SDL_Renderer* renderer) noexcept;

    ~GlintRenderer();

    /** @brief 渲染器是否有效 */
    [[nodiscard]] bool isValid() const noexcept { return renderer_ != nullptr; }

    /**
     * @brief 在精灵之上叠加绘制表面流光（屏幕坐标已由调用方完成相机变换）
     * @param src_texture   源精灵纹理（非拥有指针）
     * @param sprite        精灵（src_rect 决定遮罩区域）
     * @param position_screen 屏幕坐标左上角
     * @param size          目标大小（已含缩放）
     * @param rotation      旋转角度（度），与精灵绘制保持一致
     * @param glint         流光参数（颜色 / 速度 / 强度 / 周期 / 累计时间）
     */
    void draw(SDL_Texture* src_texture, const component::Sprite& sprite,
              const glm::vec2& position_screen, const glm::vec2& size, float rotation,
              const component::GlintComponent& glint) noexcept;

    /** @brief 清空全部缓存（RAII 自动释放纹理） */
    void clearCache() noexcept;

    // 禁用拷贝与移动语义（持有独占的渲染资源）
    GlintRenderer(const GlintRenderer&) = delete;
    GlintRenderer& operator=(const GlintRenderer&) = delete;
    GlintRenderer(GlintRenderer&&) = delete;
    GlintRenderer& operator=(GlintRenderer&&) = delete;

private:
    // RAII：SDL_Texture 删除器（与 TextureManager 惯例一致）
    struct SDLTextureDeleter
    {
        void operator()(SDL_Texture* texture) const noexcept
        {
            if (texture)
            {
                SDL_DestroyTexture(texture);
            }
        }
    };
    using TexturePtr = std::unique_ptr<SDL_Texture, SDLTextureDeleter>;

    /// @brief 源 alpha 缓存键（源纹理 ID + 源矩形）
    struct AlphaKey
    {
        entt::id_type texture_id{};
        float src_x{};
        float src_y{};
        float src_w{};
        float src_h{};

        bool operator==(const AlphaKey&) const = default;
    };
    /// @brief AlphaKey 的哈希器
    struct AlphaKeyHash
    {
        size_t operator()(const AlphaKey& key) const noexcept;
    };

    /// @brief 遮罩纹理缓存键（按尺寸复用 STREAMING 纹理）
    struct SizeKey
    {
        int width{};
        int height{};

        bool operator==(const SizeKey&) const = default;
    };
    /// @brief SizeKey 的哈希器
    struct SizeKeyHash
    {
        size_t operator()(const SizeKey& key) const noexcept;
    };

    /** @brief 获取（缓存命中或提取）源精灵区域的 alpha 数组 */
    [[nodiscard]] const std::vector<uint8_t>& getSourceAlpha(SDL_Texture* src_texture,
                                                             const component::Sprite& sprite) noexcept;

    /** @brief 提取源精灵 src_rect 区域的 alpha 数组（离屏目标 + ReadPixels 一次性读回） */
    [[nodiscard]] std::vector<uint8_t> readSourceAlpha(SDL_Texture* src_texture,
                                                       const component::Sprite& sprite) noexcept;

    /** @brief 获取（复用或创建）指定尺寸的 STREAMING 遮罩纹理 */
    [[nodiscard]] SDL_Texture* getMaskedTexture(int width, int height) noexcept;

    /** @brief 获取（复用或重建）指定尺寸的离屏渲染目标 */
    [[nodiscard]] SDL_Texture* getOffscreenTarget(int width, int height) noexcept;

    SDL_Renderer* renderer_{nullptr};                                   ///< @brief 非拥有指针
    std::unordered_map<AlphaKey, std::vector<uint8_t>, AlphaKeyHash> alpha_cache_;   ///< @brief 源 alpha 缓存
    std::unordered_map<SizeKey, TexturePtr, SizeKeyHash> mask_cache_;    ///< @brief 遮罩纹理缓存
    TexturePtr offscreen_target_;                                       ///< @brief 复用的离屏渲染目标（alpha 提取用）
    int offscreen_width_{0};                                            ///< @brief 离屏目标宽度
    int offscreen_height_{0};                                           ///< @brief 离屏目标高度
};

} // namespace engine::render
