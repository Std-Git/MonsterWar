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
 * @brief 卡牌类型标识
 */
enum class CardType
{
    Card,   ///< 使用默认 frame_size
    Display,///< 使用独立的 display_width/height/scale
    Button  ///< 按钮类型（存储在独立的 button_map_ 中）
};

/**
 * @brief 按钮状态枚举
 */
enum class ButtonState
{
    Normal, ///< 正常状态
    Hover,  ///< 悬停状态
    Pressed ///< 按下状态
};

/**
 * @brief 按钮多状态图片集合
 */
struct ButtonImages
{
    engine::render::Image normal;  ///< 正常状态图片
    engine::render::Image hover;   ///< 悬停状态图片
    engine::render::Image pressed; ///< 按下状态图片
    bool has_pressed{false};       ///< 是否配置了 pressed 图片
};

/**
 * @brief 卡牌显示信息（独立于纹理源尺寸）
 */
struct CardDisplayInfo
{
    CardType type{CardType::Card}; // 卡牌类型
    float display_width{0.0f};    // 0 = 使用源纹理宽度
    float display_height{0.0f};   // 0 = 使用源纹理高度
    float scale{1.0f};            // 缩放倍数，默认1.0
};

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
    /// @brief 存储卡牌冷却时间 (卡牌ID -> 冷却秒数)
    std::unordered_map<entt::id_type, float> card_cooldown_map_;
    /// @brief 存储卡牌显示信息 (卡牌ID -> 显示尺寸+缩放)
    std::unordered_map<entt::id_type, CardDisplayInfo> card_display_info_;
    /// @brief 存储可选择的卡牌ID列表
    std::vector<entt::id_type> card_list_;
    /// @brief 存储按钮多状态图片 (按钮ID -> 三态图片)
    std::unordered_map<entt::id_type, ButtonImages> button_map_;

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
    [[nodiscard]] float getCardCooldown(entt::id_type id) const;  ///< @brief 获取卡牌冷却时间(秒),未配置时返回默认值5.0
    [[nodiscard]] CardDisplayInfo getCardDisplayInfo(entt::id_type id) const; ///< @brief 获取卡牌显示信息(尺寸+缩放),未配置时返回默认值
    [[nodiscard]] glm::vec2 getCardDisplaySize(entt::id_type id) const;       ///< @brief 获取卡牌最终显示尺寸(glm::vec2)，未配置时根据类型返回frame_size或源纹理尺寸
    [[nodiscard]] const std::vector<entt::id_type> &getCardList() const { return card_list_; } ///< @brief 获取可选择的卡牌ID列表
    [[nodiscard]] engine::render::Image &getButtonImage(entt::id_type id, ButtonState state);  ///< @brief 获取按钮指定状态的图片，未找到时回退到Normal
    [[nodiscard]] float getUnitPanelPadding() const { return unit_panel_padding_; }
    [[nodiscard]] glm::vec2 getUnitPanelFrameSize() const { return unit_panel_frame_size_; }
    [[nodiscard]] int getUnitPanelFontSize() const { return unit_panel_font_size_; }
    [[nodiscard]] std::string getUnitPanelFontPath() const { return unit_panel_font_path_; }
    [[nodiscard]] glm::vec2 getUnitPanelFontOffset() const { return unit_panel_font_offset_; }
private:
    // -- 分步骤的数据加载函数 -- 
    void loadIcon(nlohmann::json& json);
    void loadCard(nlohmann::json &json);
    void loadDisplay(nlohmann::json &json); ///< @brief 加载 Display 类型图片（显示尺寸为原始大小）
    void loadCardList(nlohmann::json &json);
    void loadCardFrame(nlohmann::json& json);
    void loadLayout(nlohmann::json &json);
    void loadButton(nlohmann::json &json); ///< @brief 加载按钮多状态图片
};

}   // namespace game::data