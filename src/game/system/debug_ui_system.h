#pragma once
#include "../defs/events.h"
#include <entt/entity/fwd.hpp>

namespace engine::core
{
    class Context;
}

namespace game::scene
{
    class TitleScene;
    class LevelClearScene;
    class EndScene;
}

namespace game::system
{
/**
 * @brief 调试UI系统，负责显示调试UI
 * 
 * @note 调试UI的主要目的是方便debug,并快速开发UI原型
 * @note 游戏正式发布时往往会删除，因此不需要过度设计
 */
class DebugUISystem
{
    entt::registry &registry_;
    engine::core::Context &context_;

    entt::id_type hovered_portrait_{entt::null};    ///< @brief 悬浮肖像的角色名称id
    bool show_debug_ui_{false};                     ///< @brief 是否显示调试UI

public:
    DebugUISystem(entt::registry &registry, engine::core::Context &context);
    ~DebugUISystem();

    // ImGui 步骤3: 一轮循环内，ImGui 需要做的操作（逻辑+渲染）
    void update();                                              ///< @brief 针对GameScene的更新
    void updateTitle(game::scene::TitleScene &title_scene);     ///< @brief 针对TitleScene的更新 (直接传入场景引用)
    void updateLevelClear(game::scene::LevelClearScene& level_clear_scene); ///< @brief 针对LevelClearScene的更新
    void updateEnd(game::scene::EndScene& end_scene);                       ///< @brief 针对EndScene的更新

private:
    // 封装开始，结束帧的方法
    void beginFrame();
    void endFrame();

    // 封装每一个UI显示模块
    //void renderDemoUI();
    // --- GameScene ---
    void renderHoveredPortrait();
    void renderHoveredUnit();
    void renderSelectedUnit();
    void renderInfoUI();
    void renderSettingUI();
    void renderDebugUI();

    // --- TitleScene ---
    void renderTitleLogo();
    void renderTitleButtons(game::scene::TitleScene &title_scene);

    // --- LevelClearScene ---
    void renderLevelClearText();
    void renderLevelClearTable(game::scene::LevelClearScene& level_clear_scene);
    void renderLevelClearButtons(game::scene::LevelClearScene& level_clear_scene);

    // --- EndScene ---
    void renderEndText(game::scene::EndScene& end_scene);
    void renderEndButtons(game::scene::EndScene &end_scene);

    // --- Shared ---
    void renderUnitInfoUI(bool& show_unit_info);
    void renderSavePanelUI(bool& show_save_panel);
    void renderLoadPanelUI(bool& show_load_panel);
    void renderUnitTable();

    // 事件回调函数
    void onUIPortraitHoverEnterEvent(const game::defs::UIPortraitHoverEnterEvent &event);
    void onUIPortraitHoverLeaveEvent();
};

}   // namespace game::system