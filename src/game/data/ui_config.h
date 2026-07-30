#pragma once
#include "../../engine/render/image.h"
#include <string_view>
#include <unordered_map>
#include <entt/entity/fwd.hpp>
#include <nlohmann/json_fwd.hpp>
#include <glm/vec2.hpp>

namespace game::data
{
/**
 * @brief 管理 UI 配置数据
 * 
 * 包含 icon,card, card_frame, unit_panel的配置数据
 */
class UIConfig
{
    /// @brief 存储职业类型icon的map
    std::unordered_map<entt::id_type, engine::render::Image> icon_map_;
    /// @brief 存储角色肖像的map
    std::unordered_map<entt::id_type, engine::render::Image> card_map_;
    /// @brief 存储角色肖像框的map (稀有度作为key)
    std::unordered_map<int, engine::render::Image> card_frame_map_;

    // -- 单位面板的配置数据 (从json配置文件读取) -- 
    float unit_panel_padding_{10.0f};                   ///< @brief 单位面板间隔
    glm::vec2 unit_panel_frame_size_{128.0f, 128.0f};   ///< @brief 单位面板框大小
    int unit_panel_font_size_{40};                      ///< @brief 单位面板字体大小
    std::string unit_panel_font_path_;                  ///< @brief 单位面板字体路径
    glm::vec2 unit_panel_font_offset_{16.0f, 72.0f};    ///< @brief 单位面板字体偏移

public:
    UIConfig() = default;
    ~UIConfig();

    [[nodiscard]] bool loadFromFile(std::string_view path = "assets/data/ui_config.json");  ///< @brief 从json配置文件加载数据

    // -- Getters -- 
    [[nodiscard]] engine::render::Image& getIcon(entt::id_type id);
    [[nodiscard]] engine::render::Image& getCard(entt::id_type id);
    [[nodiscard]] engine::render::Image& getCardFrame(int rarity);
    [[nodiscard]] float getUnitPanelPadding() const { return unit_panel_padding_; }
    [[nodiscard]] glm::vec2 getUnitPanelFrameSize() const { return unit_panel_frame_size_; }
    [[nodiscard]] int getUnitPanelFontSize() const { return unit_panel_font_size_; }
    [[nodiscard]] std::string getUnitPanelFontPath() const { return unit_panel_font_path_; }
    [[nodiscard]] glm::vec2 getUnitPanelFontOffset() const { return unit_panel_font_offset_; }
private:
    // -- 分步骤的数据加载函数 -- 
    void loadIcon(nlohmann::json& json);
    void loadCard(nlohmann::json& json);
    void loadCardFrame(nlohmann::json& json);
    void loadLayout(nlohmann::json& json);
};

}   // namespace game::data