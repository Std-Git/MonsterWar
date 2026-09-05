#include "ui_config.h"
#include "../../engine/render/image.h"
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <entt/core/hashed_string.hpp>

namespace game::data
{

UIConfig::~UIConfig() = default;

bool UIConfig::loadFromFile(std::string_view path)
{
    std::filesystem::path file_path(path);
    std::ifstream file(file_path);
    nlohmann::json json;
    file >> json;

    try {
        //loadIcon(json["icon"]);
        loadCard(json["card"]);
        loadDisplay(json["display"]);
        //loadCardFrame(json["card_frame"]);
        loadLayout(json["layout"]);
        // 加载可选择的卡牌列表
        loadCardList(json["card_list"]);
        // 加载按钮图片
        loadButton(json["button"]);
    }
    catch (const std::exception &e)
    {
        spdlog::error("载入 UI config 失败：{}", e.what());
        return false;
    }

    return true;
}

void UIConfig::loadIcon(nlohmann::json& json)
{
    for (auto& [key, value] : json.items()) {
        entt::id_type id = entt::hashed_string(key.c_str());
        auto texture_path = value["sprite_sheet"].get<std::string>();
        engine::utils::Rect src_rect = {static_cast<float>(value["x"]),
            static_cast<float>(value["y"]),
            static_cast<float>(value["width"]),
            static_cast<float>(value["height"])};
        icon_map_[id] = engine::render::Image(texture_path, src_rect, false);
    }
}

void UIConfig::loadCard(nlohmann::json& json)
{
    for (auto& [key, value] : json.items()) {
        entt::id_type id = entt::hashed_string(key.c_str());
        auto texture_path = value["sprite_sheet"].get<std::string>();
        float card_width = static_cast<float>(value["width"]);
        float card_height = static_cast<float>(value["height"]);
        engine::utils::Rect src_rect = {static_cast<float>(value["x"]),
            static_cast<float>(value["y"]),
            card_width,
            card_height};
        card_map_[id] = engine::render::Image(texture_path, src_rect, false);

        // 加载卡牌冷却时间(如果配置中存在)
        if (value.contains("cooldown")) {
            card_cooldown_map_[id] = value["cooldown"].get<float>();
        }

        // Card 类型：显示尺寸由 frame_size 决定，不需要额外设置 display 参数
        CardDisplayInfo info;
        info.type = CardType::Card;
        card_display_info_[id] = info;
    }
}

void UIConfig::loadDisplay(nlohmann::json &json)
{
    for (auto &[key, value] : json.items())
    {
        entt::id_type id = entt::hashed_string(key.c_str());
        auto texture_path = value["sprite_sheet"].get<std::string>();
        float w = static_cast<float>(value["width"]);
        float h = static_cast<float>(value["height"]);
        engine::utils::Rect src_rect = {static_cast<float>(value["x"]),
                                        static_cast<float>(value["y"]),
                                        w,
                                        h};
        card_map_[id] = engine::render::Image(texture_path, src_rect, false);

        // Display 类型：默认显示为原始大小
        CardDisplayInfo info;
        info.type = CardType::Display;
        info.display_width = w;
        info.display_height = h;
        info.scale = 1.0f;
        card_display_info_[id] = info;
    }
}

void UIConfig::loadCardList(nlohmann::json &json)
{
    for (const auto &card_id_str : json)
    {
        std::string id_str = card_id_str.get<std::string>();
        entt::id_type id = entt::hashed_string(id_str.c_str());
        card_list_.push_back(id);
        spdlog::trace("卡牌列表: {} (id={})", id_str, static_cast<unsigned int>(id));
    }
    spdlog::warn("卡牌列表加载完成, 数量={}", card_list_.size());
}

void UIConfig::loadCardFrame(nlohmann::json& json)
{
    for (auto& [key, value] : json.items()) {
        auto texture_path = value["sprite_sheet"].get<std::string>();
        int level = value["level"].get<int>();
        engine::utils::Rect src_rect = {static_cast<float>(value["x"]),
            static_cast<float>(value["y"]),
            static_cast<float>(value["width"]),
            static_cast<float>(value["height"])};
        card_frame_map_[level] = engine::render::Image(texture_path, src_rect, false);
    }
}

void UIConfig::loadLayout(nlohmann::json& json)
{
    unit_panel_padding_ = json["unit_panel"]["padding"].get<float>();
    unit_panel_frame_size_ = {json["unit_panel"]["frame_size"]["width"].get<float>(),
                              json["unit_panel"]["frame_size"]["height"].get<float>()};
    spdlog::warn("{}, {}", unit_panel_frame_size_.x, unit_panel_frame_size_.y);
    unit_panel_font_size_ = json["unit_panel"]["font_size"].get<int>();
    unit_panel_font_path_ = json["unit_panel"]["font_path"].get<std::string>();
    spdlog::warn("{} {}", unit_panel_font_size_, unit_panel_font_path_);
    unit_panel_font_offset_ = {json["unit_panel"]["font_offset"]["x"].get<float>(),
                               json["unit_panel"]["font_offset"]["y"].get<float>()};
}

void UIConfig::loadButton(nlohmann::json &json)
{
    for (auto &[key, value] : json.items())
    {
        entt::id_type id = entt::hashed_string(key.c_str());
        ButtonImages images;

        // 加载 normal 状态图片（必需）
        auto &normal = value["normal"];
        images.normal = engine::render::Image(
            normal["sprite_sheet"].get<std::string>(),
            engine::utils::Rect{static_cast<float>(normal["x"]),
             static_cast<float>(normal["y"]),
             static_cast<float>(normal["width"]),
             static_cast<float>(normal["height"])},
            false);

        // 加载 hover 状态图片（必需）
        auto &hover = value["hover"];
        images.hover = engine::render::Image(
            hover["sprite_sheet"].get<std::string>(),
            engine::utils::Rect{static_cast<float>(hover["x"]),
                                static_cast<float>(hover["y"]),
                                static_cast<float>(hover["width"]),
                                static_cast<float>(hover["height"])},
            false);

        // 加载 pressed 状态图片（可选，不存在时回退到 normal）
        if (value.contains("pressed"))
        {
            auto &pressed = value["pressed"];
            images.pressed = engine::render::Image(
                pressed["sprite_sheet"].get<std::string>(),
                engine::utils::Rect{static_cast<float>(pressed["x"]),
                                    static_cast<float>(pressed["y"]),
                                    static_cast<float>(pressed["width"]),
                                    static_cast<float>(pressed["height"])},
                false);
            images.has_pressed = true;
        }
        // 记录按钮显示尺寸（使用 normal 状态图片的宽高）
        CardDisplayInfo info;
        info.type = CardType::Button;
        info.display_width = static_cast<float>(normal["width"]);
        info.display_height = static_cast<float>(normal["height"]);
        info.scale = 1.0f;
        card_display_info_[id] = info;

        button_map_[id] = std::move(images);
        spdlog::trace("按钮图片加载: {} (id={})", key, static_cast<unsigned int>(id));
    }
    spdlog::warn("按钮图片加载完成, 数量={}", button_map_.size());
}

engine::render::Image& UIConfig::getIcon(entt::id_type id)
{
    if (auto it = icon_map_.find(id); it != icon_map_.end()) {
        return it->second;
    } else {
        spdlog::error("Icon 未找到：{}", id);
        return icon_map_.begin()->second;
    }
}

engine::render::Image& UIConfig::getCard(entt::id_type id)
{
    if (auto it = card_map_.find(id); it != card_map_.end()) {
        return it->second;
    } else {
        spdlog::error("Card 未找到：{}", id);
        return card_map_.begin()->second;
    }
}

engine::render::Image& UIConfig::getCardFrame(int level)
{
    if (auto it = card_frame_map_.find(level); it != card_frame_map_.end()) {
        return it->second;
    } else {
        spdlog::error("Card frame 未找到：{}", level);
        return card_frame_map_.begin()->second;
    }
}

float UIConfig::getCardCooldown(entt::id_type id) const
{
    auto it = card_cooldown_map_.find(id);
    if (it != card_cooldown_map_.end()) {
        return it->second;
    }
    return 0.0f;  // 默认冷却时间0秒
}

CardDisplayInfo UIConfig::getCardDisplayInfo(entt::id_type id) const
{
    auto it = card_display_info_.find(id);
    if (it != card_display_info_.end())
    {
        return it->second;
    }
    return CardDisplayInfo{}; // 返回默认值(Card类型display=0, scale=1.0)
}

glm::vec2 UIConfig::getCardDisplaySize(entt::id_type id) const
{
    auto info = getCardDisplayInfo(id);

    if (info.type == CardType::Display)
    {
        // Display 类型：使用 display_width/height + scale
        float w = info.display_width;
        float h = info.display_height;
        float scale = info.scale > 0.0f ? info.scale : 1.0f;
        return glm::vec2(w * scale, h * scale);
    }
    else if (info.type == CardType::Button)
    {
        // Button 类型：使用按钮 normal 状态图片的原始宽高
        return glm::vec2(info.display_width, info.display_height);
    }
    else
    {
        // Card 类型：直接使用默认 frame_size
        return unit_panel_frame_size_;
    }
}

engine::render::Image &UIConfig::getButtonImage(entt::id_type id, ButtonState state)
{
    auto it = button_map_.find(id);
    if (it == button_map_.end())
    {
        spdlog::error("Button 未找到：{}", id);
        return button_map_.begin()->second.normal;
    }

    auto &images = it->second;
    switch (state)
    {
    case ButtonState::Normal:
        return images.normal;
    case ButtonState::Hover:
        return images.hover;
    case ButtonState::Pressed:
        // 如果没有配置 pressed，回退到 normal
        if (images.has_pressed)
            return images.pressed;
        return images.normal;
    }
    return images.normal;
}

}   // namespace game::data
