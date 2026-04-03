#pragma once
#include "../../engine/scene/scene.h"
#include "../data/ui_config.h"
#include "../data/session_data.h"
#include "../data/game_stats.h"
#include "../data/level_config.h"
#include "../factory/blueprint_manager.h"
#include "../system/fwd.h"

namespace game::scene
{
class LevelClearScene : public engine::scene::Scene
{
    friend class game::system::DebugUISystem;

    // 场景中共享的数据实例
    std::shared_ptr<game::factory::BlueprintManager> blueprint_manager_;
    std::shared_ptr<game::data::UIConfig> ui_config_;
    std::shared_ptr<game::data::LevelConfig> level_config_;
    std::shared_ptr<game::data::SessionData> session_data_;

    game::data::GameStats& game_stats_; ///< @brief 构造函数传入关卡内游戏统计数据，需要在此场景中显示

    // 目前只需要DebugUISystem
    std::unique_ptr<game::system::DebugUISystem> debug_ui_system_;

    bool show_save_panel_{false}; ///< @brief 是否显示保存面板

public:
    LevelClearScene(engine::core::Context& context, 
        std::shared_ptr<game::factory::BlueprintManager> blueprint_manager,
        std::shared_ptr<game::data::UIConfig> ui_config,
        std::shared_ptr<game::data::LevelConfig> level_config,
        std::shared_ptr<game::data::SessionData> session_data,
        game::data::GameStats& game_stats);
    ~LevelClearScene();

    void init() override;
    void render() override;

private:
    // 按钮回调函数
    void onNextLevelClick();
    void onBackToTitleClick();
    void onSaveClick();
};
}   // namespace game::scene