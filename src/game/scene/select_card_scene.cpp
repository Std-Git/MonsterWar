#include "select_card_scene.h"
#include "game_scene.h"
#include "../data/ui_config.h"
#include "../data/session_data.h"
#include "../../engine/ui/ui_manager.h"
#include "../../engine/core/context.h"
#include "../../engine/core/time.h"
#include "../../engine/core/game_state.h"
#include "../../engine/audio/audio_player.h"
#include "../../engine/utils/events.h"
#include "../../engine/render/camera.h"
#include "../../engine/system/render_system.h"
#include "../../engine/system/ysort_system.h"
#include "../../engine/system/animation_system.h"
#include "../../engine/system/movement_system.h"
#include "../../engine/loader/level_loader.h"
#include "../../engine/loader/basic_entity_builder.h"
#include "../system/debug_ui_system.h"
#include "../ui/select_card_ui.h"
#include <spdlog/spdlog.h>
#include <entt/entity/registry.hpp>

using namespace entt::literals;

namespace game::scene
{
SelectCardScene::SelectCardScene(engine::core::Context &context, 
    std::shared_ptr<game::factory::BlueprintManager> blueprint_manager, 
    std::shared_ptr<game::data::SessionData> session_data, 
    std::shared_ptr<game::data::UIConfig> ui_config, 
    std::shared_ptr<game::data::LevelConfig> level_config)
    : engine::scene::Scene("SelectCardScene", context),
    blueprint_manager_(std::move(blueprint_manager)),
    session_data_(std::move(session_data)),
    ui_config_(std::move(ui_config)),
    level_config_(std::move(level_config)) {}

SelectCardScene::~SelectCardScene() = default;

bool SelectCardScene::init()
{
    if (!initSessionData())             { spdlog::error("初始化session_data_失败"); return false; }
    if (!initLevelConfig())             { spdlog::error("初始化关卡配置失败"); return false; }
    if (!initBlueprintManager())        { spdlog::error("初始化蓝图管理器失败"); return false; }
    if (!initUIConfig())                { spdlog::error("初始化UI配置失败"); return false; }
    if (!loadLevel())                   { spdlog::error("加载标题关卡失败"); return false; }
    if (!initSystems())                 { spdlog::error("初始化系统失败"); return false; }
    if (!initRegistryContext())         { spdlog::error("初始化注册表上下文失败"); return false; }
    if (!initUI())                      { spdlog::error("初始化UI失败"); return false; }

    context_.getGameState().setState(engine::core::State::Title);
    context_.getTime().setTimeScale(1.0f);  // 重置游戏速度

    context_.getAudioPlayer().playMusic("select_card_bgm"_hs); // 设置标题场景背景音乐
    context_.getCamera().setPosition(glm::vec2(-315.0f, -57.0f));

    return engine::scene::Scene::init();
}

void SelectCardScene::update(float delta_time)
{
    engine::scene::Scene::update(delta_time);
    animation_system_->update(delta_time);
    movement_system_->update(registry_, delta_time);
    ysort_system_->update(registry_);
}

void SelectCardScene::render()
{
    auto &renderer = context_.getRenderer();
    auto &camera = context_.getCamera();

    render_system_->update(registry_, renderer, camera);

    engine::scene::Scene::render();
    //ebug_ui_system_->updateTitle(*this);
}

bool SelectCardScene::initSessionData()
{
    if (!session_data_)
    {
        session_data_ = std::make_shared<game::data::SessionData>();
        if (!session_data_->loadDefaultData())
        {
            spdlog::error("初始化session_data_失败");
            return false;
        }
    }
    return true;
}

bool SelectCardScene::initLevelConfig()
{
    if (!level_config_)
    {
        level_config_ = std::make_shared<game::data::LevelConfig>();
        if (!level_config_->loadFromFile("assets/data/level_config.json"))
        {
            spdlog::error("加载关卡配置失败");
            return false;
        }
    }
    return true;
}

bool SelectCardScene::initBlueprintManager()
{
    if (!blueprint_manager_)
    {
        blueprint_manager_ = std::make_shared<game::factory::BlueprintManager>(context_.getResourceManager());
        if (!blueprint_manager_->loadEnemyClassBlueprints("assets/data/zombie_data.json") ||
            !blueprint_manager_->loadPlayerClassBlueprints("assets/data/plant_data.json") ||
            !blueprint_manager_->loadProjectileBlueprints("assets/data/projectile_data.json") ||
            !blueprint_manager_->loadEffectBlueprints("assets/data/effect_data.json") ||
            !blueprint_manager_->loadSkillBlueprints("assets/data/skill_data.json"))
        {
            spdlog::error("加载蓝图失败");
            return false;
        }
    }
    return true;
}

bool SelectCardScene::initUIConfig()
{
    if (!ui_config_)
    {
        ui_config_ = std::make_shared<game::data::UIConfig>();
        if (!ui_config_->loadFromFile("assets/data/ui_config.json"))
        {
            spdlog::error("加载UI配置失败");
            return false;
        }
    }
    return true;
}

bool SelectCardScene::loadLevel()
{
    engine::loader::LevelLoader level_loader;
    auto map_path = level_config_->getMapPath(session_data_->getLevelNumber());
    if (!level_loader.loadLevel(map_path, this))
    {
        spdlog::error("加载关卡失败");
        return false;
    }
    return true;
}

bool SelectCardScene::initSystems()
{
    // 初始化系统
    auto& dispatcher = context_.getDispatcher();
    debug_ui_system_ = std::make_unique<game::system::DebugUISystem>(registry_, context_);
    render_system_ = std::make_unique<engine::system::RenderSystem>();
    ysort_system_ = std::make_unique<engine::system::YSortSystem>();
    animation_system_ = std::make_unique<engine::system::AnimationSystem>(registry_, dispatcher);
    movement_system_ = std::make_unique<engine::system::MovementSystem>();
    return true;
}

bool SelectCardScene::initRegistryContext()
{
    // 让注册表存储一些数据类型实例作为上下文，方便使用
    registry_.ctx().emplace<std::shared_ptr<game::data::SessionData>>(session_data_);
    registry_.ctx().emplace<std::shared_ptr<game::factory::BlueprintManager>>(blueprint_manager_);
    registry_.ctx().emplace<std::shared_ptr<game::data::UIConfig>>(ui_config_);
    return true;
}

bool SelectCardScene::initUI()
{
    auto window_size = context_.getGameState().getLogicalSize();
    if (!ui_manager_->init(window_size))
        return false;

    /* 先用ImGui实现UI，未来再使用游戏内UI */
    return true;
}

bool SelectCardScene::initSelectCardUI()
{
    try
    {
        select_card_ui = std::make_unique<game::ui::SelectCardUI>(registry_, *ui_manager_, context_);
    }
    catch (const std::exception &e)
    {
        spdlog::error("初始化单位肖像UI失败: {}", e.what());
        return false;
    }
    return true;
}

void SelectCardScene::onStartGameClick()
{
    // 如果数据是存档载入的，有可能已经通关，此时需要进入下一关
    if (session_data_->isLevelClear())
    {
        session_data_->setLevelClear(false);
        session_data_->addOneLevel();
    }
    requestReplaceScene(std::make_unique<game::scene::GameScene>(
        context_,
        blueprint_manager_,
        session_data_,
        ui_config_,
        level_config_)
    );
}

void SelectCardScene::onConfirmRoleClick()
{
    show_unit_info_ = !show_unit_info_;
    /* 用 ImGui 快速实现逻辑，未来再完善游戏内UI*/
}

void SelectCardScene::onLoadGameClick()
{
    show_load_panel_ = !show_load_panel_;
    /* 用 ImGui 快速实现逻辑，未来再完善游戏内UI*/
}

void SelectCardScene::onQuitClick()
{
    quit();
}

} // namespace game::scene