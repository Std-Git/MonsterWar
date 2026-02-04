#pragma once
#include "ui_element.h"
#include "../render/image.h"
#include <string_view>
#include <optional>
#include <SDL3/SDL_rect.h>

namespace engine::ui
{

/**
 * @brief  一个用于显示纹理或部分纹理的UI元素
 * 
 * 继承自 UIElement 并添加了渲染图像的功能
 */
class UIImage final : public UIElement
{
protected:
    engine::render::Image image_;

public:
    /**
     * @brief 构造一个 UIImage 对象 (通过纹理路径构造)
     * 
     * @param texture_path 要显示的纹理路径
     * @param position 图像的局部位置
     * @param size 图像元素的大小，如果为{0，0}，则使用纹理的整个区域
     * @param source_rect 可选：要绘制的文理部分(如果为空，则使用纹理的整个区域)
     * @param is_flipped 可选：精灵是否应该水平翻转
     */
    UIImage(std::string_view texure_path, 
            glm::vec2 position = {0.0f, 0.0f},
            glm::vec2 size = {0.0f, 0.0f},
            std::optional<engine::utils::Rect> source_rect = std::nullopt,
            bool is_flipped = false);
    
    /**
     * @brief 构造一个 UIImage 对象 (通过纹理ID构造)
     * 
     * @param texture_id 要显示的纹理ID
     * @param position 图像的局部位置
     * @param size 图像元素的大小，如果为{0，0}，则使用纹理的整个区域
     * @param source_rect 可选：要绘制的文理部分(如果为空，则使用纹理的整个区域)
     * @param is_flipped 可选：精灵是否应该水平翻转
     * @note 用此方法，需确保对应的ID纹理已经加载到 ResourceManager 中，因此不需要提供纹理路径
     */
    UIImage(entt::id_type texture_id,
            glm::vec2 position = {0.0f, 0.0f},
            glm::vec2 size = {0.0f, 0.0f},
            std::optional<engine::utils::Rect> source_rect = std::nullopt,
            bool is_flipped = false);

    /**
     * @brief 构造一个 UIImage 对象 (通过 Image对象构造)
     * 
     * @param image 要显示的 Image 对象
     * @param position 图像的局部位置
     * @param size 图像元素的大小，如果为{0，0}，则使用纹理的整个区域
     */
    UIImage(engine::render::Image& image,
            glm::vec2 position = {0.0f, 0.0f},
            glm::vec2 size = {0.0f, 0.0f});

    // 核心方法
    void render(engine::core::Context& context) override;

    // -- Getters and setters
    const engine::render::Image& getImage() const { return image_; }
    void setImage(engine::render::Image image) { image_ = std::move(image); }

    std::string_view getTexturePath() const { return image_.getTexturePath(); }
    entt::id_type getTextureId() const { return image_.getTextureId(); }
    void setTexture(std::string_view texture_path) { image_.setTexture(texture_path); }

    const std::optional<engine::utils::Rect>& getSourceRect() const { return image_.getSourceRect(); }
    void setSourceRect(std::optional<engine::utils::Rect> source_rect) { image_.setSourceRect(std::move(source_rect)); }

    bool isFlipped() const { return image_.isFlipped(); }
    void setFlipped(bool flipped) { image_.setFlipped(flipped); }
};

}   // namespace engine::ui