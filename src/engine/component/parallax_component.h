#pragma once
#include <glm/vec2.hpp>
#include <utility>

namespace engine::component
{
/**
 * @brief 视差组件，包含速度滚动因子，是否重复和是否可见 (须和 Sprite 配合使用)
 */
struct ParallaxComponent final : public Component
{
    glm::vec2 scroll_factor_{};         ///< @brief 滚动速度因子(0=静止，1=随相机移动，<1=比相机慢)
    glm::bvec2 repeat_{true};           ///< @brief 是否重复
    bool is_visible_{true};             ///< @brief 是否可见

    /**
     * @brief 构造函数
     * @param scroll_factor 滚动速度因子
     * @param repeat 是否重复, 默认为true
     * @param is_visible 是否可见, 默认为true
     */
    ParallaxComponent(glm::vec2 scroll_factor,
                      glm::bvec2 repeat = glm::bvec2(true, true),
                      bool is_visible = true) : 
                      scroll_factor_(std::move(scroll_factor)), 
                      repeat_(std::move(repeat)), 
                      is_visible_(is_visible) {}

};

}   // namespace engine::component