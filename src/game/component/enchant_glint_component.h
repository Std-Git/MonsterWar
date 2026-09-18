#pragma once
#include "../../engine/component/glint_component.h"
#include <entt/entity/registry.hpp>
#include <optional>

namespace game::component
{
/**
 * @brief 附魔种类枚举
 *
 * 游戏层语义：定义有哪些"附魔"及其默认外观。
 * 每种附魔对应一组流光参数（颜色 / 速度 / 强度 / 条纹周期），
 * 由 toGlint() 转换为引擎通用 GlintComponent 后交给渲染层。
 */
enum class EnchantKind
{
    Flame,  ///< @brief 火焰附魔（橙）
    Frost,  ///< @brief 冰霜附魔（蓝）
    Shadow, ///< @brief 暗影附魔（紫）
    Arcane, ///< @brief 秘法附魔（青）
    Holy,   ///< @brief 神圣附魔（金）
};

/**
 * @brief 附魔光效组件（游戏语义层，挂载于游戏实体）
 *
 * 本组件承载"附魔"这一游戏概念：附魔种类 + 可选参数覆盖。
 * 引擎层只消费通用表面流光组件 engine::component::GlintComponent，
 * 不感知"附魔"语义；游戏层通过本组件表达"这个单位带哪种附魔"，
 * 并借 applyEnchantGlint() 将渲染参数落地到引擎组件。
 *
 * 使用示例：
 * @code
 * game::component::EnchantGlintComponent ench;
 * ench.kind_ = game::component::EnchantKind::Flame;
 * ench.intensity_ = 0.7f; // 覆盖默认强度
 * game::component::applyEnchantGlint(registry, entity, ench);
 * @endcode
 */
struct EnchantGlintComponent
{
    EnchantKind kind_{EnchantKind::Shadow}; ///< @brief 附魔种类（决定默认颜色 / 速度 / 强度 / 周期）
    std::optional<float> speed_;            ///< @brief 可选：覆盖条纹滚动速度（像素/秒）
    std::optional<float> intensity_;        ///< @brief 可选：覆盖光效强度（0~1）
    std::optional<float> period_;           ///< @brief 可选：覆盖条纹周期（像素）

    /**
     * @brief 转换为引擎通用流光参数（未覆盖的项取该附魔种类的默认预设）
     * @return 可直接挂载到引擎渲染链路的 GlintComponent
     */
    [[nodiscard]] engine::component::GlintComponent toGlint() const noexcept
    {
        using G = engine::component::GlintComponent;
        G glint;
        switch (kind_)
        {
        case EnchantKind::Flame:
            glint.color_ = {1.0f, 0.45f, 0.15f, 1.0f}; // 橙
            break;
        case EnchantKind::Frost:
            glint.color_ = {0.40f, 0.80f, 1.0f, 1.0f}; // 蓝
            break;
        case EnchantKind::Arcane:
            glint.color_ = {0.50f, 1.0f, 0.90f, 1.0f}; // 青
            break;
        case EnchantKind::Holy:
            glint.color_ = {1.0f, 0.92f, 0.55f, 1.0f}; // 金
            break;
        case EnchantKind::Shadow:
        default:
            glint.color_ = {0.85f, 0.55f, 1.0f, 1.0f}; // 紫
            break;
        }
        if (speed_)
        {
            glint.speed_ = *speed_;
        }
        if (intensity_)
        {
            glint.intensity_ = *intensity_;
        }
        if (period_)
        {
            glint.period_ = *period_;
        }
        return glint;
    }
};

/**
 * @brief 为实体挂载附魔光效（游戏语义 → 引擎参数）
 *
 * 同时在实体上写入本组件（语义标记，供游戏逻辑读取）与引擎
 * GlintComponent（渲染数据，由 RenderSystem 消费）。重复调用安全。
 *
 * @param registry 实体注册表
 * @param entity   目标实体
 * @param enchant  附魔配置
 */
inline void applyEnchantGlint(entt::registry& registry, entt::entity entity,
                              const EnchantGlintComponent& enchant)
{
    registry.emplace_or_replace<engine::component::GlintComponent>(entity, enchant.toGlint());
    registry.emplace_or_replace<EnchantGlintComponent>(entity, enchant);
}

} // namespace game::component