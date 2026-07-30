#include "ui_label.h"
#include "../core/context.h"
#include "../render/text_renderer.h"
#include <spdlog/spdlog.h>

namespace engine::ui
{
UILabel::UILabel(engine::render::TextRenderer &text_renderer, 
                 std::string_view text, 
                 std::string_view font_path,
                 int font_size, 
                 engine::utils::FColor text_color, 
                 glm::vec2 position)
    :  UIElement(std::move(position)),
       text_renderer_(text_renderer),
       text_(text),
       font_path_(font_path),
       font_id_(entt::hashed_string(font_path.data())),
       font_size_(font_size),
       text_fcolor_(std::move(text_color))
{
    // 获取文本渲染尺寸(函数内部会确保字体资源被加载)
    size_ = text_renderer_.getTextSize(text_, font_id_, font_size_, font_path_);   
    spdlog::trace("UILabel 构造完成");
}

void UILabel::render(engine::core::Context &context)
{
    if (!visible_ || text_.empty()) return;
    
    text_renderer_.drawUIText(text_, font_id_, font_size_, getScreenPosition(), text_fcolor_, is_dirty_);
    is_dirty_ = false; // 渲染完成后脏标识设置为 false, 下次渲染不需要重新设置TTF_Text

    // 渲染子元素 (调用基类方法)
    UIElement::render(context);
}

void UILabel::setText(std::string_view text)
{
    text_ = text;
    is_dirty_ = true;   // 不再立刻更新尺寸。而是设置脏标识为真
}

void UILabel::setFontPath(std::string_view font_path)
{
    font_path_ = font_path;
    font_id_ = entt::hashed_string(font_path.data());
    is_dirty_ = true;
}

void UILabel::setFontSize(int font_size)
{
    font_size_ = font_size;
    is_dirty_ = true;
}

void UILabel::setTextFColor(engine::utils::FColor text_fcolor)
{
    text_fcolor_ = std::move(text_fcolor);
    /* 颜色变化不影响尺寸*/
}

const glm::vec2 &UILabel::getSize()
{
    // 调用此函数时才尝试获取尺寸
    size_ = text_renderer_.getTextSize(text_, font_id_, font_size_, font_path_);
    is_dirty_ = false;
    return size_;
}

} // namespace engine::ui
