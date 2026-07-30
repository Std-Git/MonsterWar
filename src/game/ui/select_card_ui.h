#pragma once
#include "../defs/events.h"
#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>

namespace engine::core
{
    class Context;
}

namespace engine::ui
{
    class UIPanel;
    class UIManager;
}

namespace game::ui
{
    
/**
 * @brief 单位肖像UI
 *
 * 负责管理单位肖像UI的创建，更新和排列
 */
class SelectCardUI
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
    SelectCardUI(entt::registry &registry,
                 engine::ui::UIManager &ui_manager,
                 engine::core::Context &context);

    ~SelectCardUI();

    void update(float delta_time);

    engine::ui::UIPanel *getAnchorPanel() const { return anchor_panel_; }

private:
    void updateCardCover();    ///< @breif 更新肖像遮盖
    void createUnitsCardUI();  ///< @brief 创建单位肖像UI
    void arrangeUnitsCardUI(); ///< @brief 排列画面下方的单位肖像UI (肖像增/减时调用)

    void moveCardPanelRight(float delta_time); ///< @brief 向右移动单位肖像UI
    void moveCardPanelLeft(float delta_time);  ///< @brief 向左移动单位肖像UI

    // 事件回调函数
    void onRemoveUICardEvent(const game::defs::RemoveUICardEvent &event);
};

} // namespace game::ui