#pragma once
#include "ui_element.h"
#include <optional>
#include "../utils/math.h"

namespace engine::ui
{

/**
 * @brief 用于分组其他 UI 元素的容器 UI 元素
 * 
 * Panel 通常用于布局和组织
 * 可以选择是否绘制背景色
 */
class UIPanel final : public UIElement
{
    std::optional<engine::utils::FColor> background_color_;     ///< @brief 可选背景颜色
public:
    /**
     * @brief 构造一个 Panel
     * 
     * @param position Panel 的局部位置
     * @param size Panel 的大小
     * @param background_color Panel 的背景颜色，如果不需要背景色，传入 std::nullopt
     */
    explicit UIPanel(glm::vec2 position = {0.0f, 0.0f},
                     glm::vec2 size = {0.0f, 0.0f},
                     std::optional<engine::utils::FColor> background_color = std::nullopt);

    void setBackgroundColor(std::optional<engine::utils::FColor> background_color) { background_color_ = std::move(background_color); }
    const std::optional<engine::utils::FColor>& getBackgroundColor() const { return background_color_; }
    
    void render(engine::core::Context& context) override;
};

}   // namespace engine::ui