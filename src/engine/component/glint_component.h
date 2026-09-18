#pragma once
#include "../utils/math.h"
namespace engine::component
{
/**
 * @brief 通用表面流光组件（挂载于游戏实体）
 *
 * 实体带有该组件后，RenderSystem 会在绘制其精灵之后，叠加一层
 * 滚动的彩色斜纹流光（可配置颜色 / 速度 / 强度 / 条纹周期）。
 *
 * 本组件为引擎层通用能力，不含任何游戏语义（附魔、稀有度、种类预设等
 * 均不在此定义）；游戏层可在此基础上定义自己的"附魔""选中高亮""技能
 * 充能"等效果，通过填充颜色 / 速度 / 强度字段复用同一套渲染机制。
 *
 * 附魔颜色预设等游戏语义请见 game 模块（如 game/config/enchant_glint_config.h）。
 */
struct GlintComponent
{
    bool enabled_{true};                                    ///< @brief 是否启用光效
    engine::utils::FColor color_{0.85f, 0.55f, 1.0f, 1.0f}; ///< @brief 流光颜色
    float speed_{48.0f};                                    ///< @brief 条纹滚动速度（像素/秒）
    float intensity_{0.55f};                                ///< @brief 光效强度（叠加 alpha 上限，0~1）
    float period_{32.0f};                                   ///< @brief 条纹周期（像素，越大条纹越宽越疏）
    float elapsed_{0.0f};                                   ///< @brief 累计滚动时间（秒，由 RenderSystem 推进）
};
} // namespace engine::component