#pragma once
#include "../../engine/scene/scene.h"
#include "../../engine/system/fwd.h"
#include "../data/ui_config.h"
#include "../data/session_data.h"
#include "../data/level_config.h"
#include "../system/fwd.h"

namespace engine::ui
{
    class UIElement;
}

namespace game::factory
{
    class EntityFactory;
    class BlueprintManager;
}

namespace game::ui
{
    class SelectCardUI;
}

namespace game::scene
{
class SelectCardScene final : public engine::scene::Scene
{
    friend class game::system::DebugUISystem;   // 允许DebugUISystem访问私有成员变量及方法

    // 数据相关实例
    std::shared_ptr<game::factory::BlueprintManager> blueprint_manager_;
    std::shared_ptr<game::data::SessionData> session_data_;
    std::shared_ptr<game::data::UIConfig> ui_config_;
    std::shared_ptr<game::data::LevelConfig> level_config_;

    // 系统相关实例
    std::unique_ptr<engine::system::RenderSystem> render_system_;
    std::unique_ptr<engine::system::YSortSystem> ysort_system_;
    std::unique_ptr<engine::system::AnimationSystem> animation_system_;
    std::unique_ptr<engine::system::MovementSystem> movement_system_;
    std::unique_ptr<game::system::DebugUISystem> debug_ui_system_;

    std::unique_ptr<game::ui::SelectCardUI> select_card_ui; // 封装的单位肖像UI，负责管理单位肖像UI的创建、更新和排列

    float elapsed_time_{0.0f};
    bool message_logged_{false};

public:
    SelectCardScene(engine::core::Context& context,
               std::shared_ptr<game::factory::BlueprintManager> blueprint_manager = nullptr,
               std::shared_ptr<game::data::SessionData> session_data = nullptr,
               std::shared_ptr<game::data::UIConfig> ui_config = nullptr,
               std::shared_ptr<game::data::LevelConfig> level_config = nullptr);
    ~SelectCardScene();

    bool init() override;
    void update(float delta_time) override;
    void render() override;

private:
    // 初始化init函数(init函数中调用)
    [[nodiscard]] bool initSessionData();
    [[nodiscard]] bool initLevelConfig();
    [[nodiscard]] bool initBlueprintManager();
    [[nodiscard]] bool initUIConfig();
    //[[nodiscard]] bool initEntityFactory();
    [[nodiscard]] bool loadLevel();
    [[nodiscard]] bool initSystems();
    [[nodiscard]] bool initRegistryContext();
    [[nodiscard]] bool initUI();
    [[nodiscard]] bool initSelectCardUI();
};

}   // namespace game::scene