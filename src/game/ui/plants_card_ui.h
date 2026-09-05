#pragma once
#include "../defs/events.h"
#include <unordered_map>
#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>

namespace engine::core
{
    class Context;
}

namespace engine::ui
{
    class UIPanel;
    class UILabel;
    class UIManager;
}

namespace game::ui
{
/**
 * @brief 单位肖像UI
 * 
 * 负责管理单位肖像UI的创建，更新和排列
 */
class PlantsCardUI
{
    // -- 构造函数传入的外部组件引用 --
    entt::registry &registry_;
    engine::ui::UIManager &ui_manager_;
    engine::core::Context &context_;

    engine::ui::UIPanel *anchor_panel_; ///< @brief 保存单位肖像UI的根面板(非拥有指针)，方便使用

public:
    /**
     * @brief 构造函数
     * @param registry 注册表
     * @param ui_manager UI管理器
     * @param context 上下文
     */
    PlantsCardUI(entt::registry &registry, 
        engine::ui::UIManager &ui_manager, 
        engine::core::Context &context);
    ~PlantsCardUI();

    void update(float delta_time);

    engine::ui::UIPanel* getAnchorPanel() const { return anchor_panel_; }
    
    /**
     * @brief 启动指定卡牌的冷却
     * @param card_id 卡牌ID
     * @note 如果卡牌已在冷却中则不重复启动
     */
    void startCooldown(entt::id_type card_id);

    /**
     * @brief 检查指定卡牌是否正在冷却中
     * @param card_id 卡牌ID
     * @return true 如果卡牌正在冷却中
     */
    [[nodiscard]] bool isOnCooldown(entt::id_type card_id) const;


private:
    std::unordered_map<entt::id_type, engine::ui::UILabel *> cost_label_map_; // 卡牌名 -> UILabel 指针
        
    // -- 冷却系统相关 --
    std::unordered_map<entt::id_type, float> card_cooldown_remaining_;  ///< @brief 卡牌剩余冷却时间 (卡牌ID -> 剩余秒数)
    std::unordered_map<entt::id_type, float> card_cooldown_max_;         ///< @brief 卡牌最大冷却时间 (卡牌ID -> 最大秒数)
    std::unordered_map<entt::id_type, engine::ui::UIPanel *> cooldown_fill_panels_;   ///< @brief 冷却条填充面板 (卡牌ID -> 面板指针)
    std::unordered_map<entt::id_type, engine::ui::UIPanel *> cooldown_bg_panels_;     ///< @brief 冷却条背景面板 (卡牌ID -> 面板指针)

    void updateCardCover();                                                   ///< @breif 更新肖像遮盖
    void updateCardLabelsColor();   ///< @breif 更新文字颜色
    void createUnitsCardUI();   ///< @brief 创建单位肖像UI
    void arrangeUnitsCardUI();  ///< @brief 排列画面下方的单位肖像UI (肖像增/减时调用)

    void moveCardPanelRight(float delta_time);  ///< @brief 向右移动单位肖像UI
    void moveCardPanelLeft(float delta_time);   ///< @brief 向左移动单位肖像UI

    // 事件回调函数
    void onRemoveUICardEvent(const game::defs::RemoveUICardEvent &event);
    void onUnitPlacedEvent(const game::defs::UnitPlacedEvent &event);
    
    // -- 冷却系统方法 --
    void updateCooldown(float delta_time);      ///< @brief 更新所有卡牌的冷却计时器和UI

};

}   // namespace game::ui