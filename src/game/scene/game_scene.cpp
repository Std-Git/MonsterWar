#include "game_scene.h"
#include "../component/enemy_component.h"
#include "../loader/entity_builder_mw.h"
#include "../system/followpath_system.h"
#include "../system/remove_dead_system.h"
#include "../factory/blueprint_manager.h"
#include "../factory/entity_factory.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/component/velocity_component.h"
#include "../../engine/component/sprite_component.h"
#include "../../engine/component/render_component.h"
#include "../../engine/core/context.h"
#include "../../engine/system/render_system.h"
#include "../../engine/system/movement_system.h"
#include "../../engine/system/animation_system.h"
#include "../../engine/system/ysort_system.h"
#include "../../engine/loader/level_loader.h"
#include <unordered_map>
#include <entt/core/hashed_string.hpp>
#include <entt/signal/sigh.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::scene
{
GameScene::GameScene(engine::core::Context &context)
    : engine::scene::Scene("GameScene", context)
{
    // 初始化系统
    render_system_ = std::make_unique<engine::system::RenderSystem>();
    movement_system_ = std::make_unique<engine::system::MovementSystem>();
    animation_system_ = std::make_unique<engine::system::AnimationSystem>();
    ysort_system_ = std::make_unique<engine::system::YSortSystem>();

    follow_path_system_ = std::make_unique<game::system::FollowPathSystem>();
    remove_dead_system_ = std::make_unique<game::system::RemoveDeadSystem>();

    spdlog::info("GameScene 构造完成");
}

GameScene::~GameScene()
{
}

void GameScene::init()
{
    if (!loadLevel())
    {
        spdlog::error("加载关卡失败");
        return;
    }
    if (!initEventConnections())
    {
        spdlog::error("初始化事件连接失败");
        return;
    }
    if (!initEntityFactory())
    {
        spdlog::error("初始化实体工厂失败");
        return;
    }
    createTestEnemy();
    Scene::init();
}

void GameScene::update(float delta_time)
{
    auto& dispatcher = context_.getDispatcher();

    // 每一帧最先清理死亡实体(要在 dispatcher 处理完事件后再清理，因此放在下一帧开头)
    remove_dead_system_->update(registry_);

    // 注意系统更新的顺序
    follow_path_system_->update(registry_, dispatcher, waypoint_nodes_);
    movement_system_->update(registry_, delta_time);
    animation_system_->update(registry_, delta_time);
    ysort_system_->update(registry_);   // 调用顺序要在 MovementSystem 之后

    Scene::update(delta_time);
}

void GameScene::render()
{
    render_system_->update(registry_, context_.getRenderer(), context_.getCamera());

    Scene::render();
}

void GameScene::clean()
{
    auto& dispatcher = context_.getDispatcher();
    dispatcher.disconnect(this);
    Scene::clean();
}

bool GameScene::loadLevel()
{
    engine::loader::LevelLoader level_loader;
    // 设置拓展的构建器 EntityBuilderMW
    level_loader.setEntityBuilder(std::make_unique<game::loader::EntityBuilderMW>(
        level_loader,
        context_,
        registry_,
        waypoint_nodes_,
        start_points_)
    );
    if (!level_loader.loadLevel("assets/maps/level1.tmj", this))
    {
        spdlog::error("加载关卡失败");
        return false;
    }
    return true;
}

bool GameScene::initEventConnections()
{
    auto& dispatcher = context_.getDispatcher();
    dispatcher.sink<game::defs::EnemyArriveHomeEvent>().connect<&GameScene::onEnemyArriveHome>(this);
    return true;
}

bool GameScene::initEntityFactory()
{
    // 如果蓝图管理器为空，可以先创建一个 (将来可能有构造函数传入)
    if (!blueprint_manager_)
    {
        blueprint_manager_ = std::make_shared<game::factory::BlueprintManager>(context_.getResourceManager());
        if (!blueprint_manager_->loadEnemyClassBlueprints("assets/data/enemy_data.json"))
        {
            spdlog::error("加载敌人蓝图失败");
            return false;
        }
    }
    entity_factory_ = std::make_unique<game::factory::EntityFactory>(registry_, *blueprint_manager_);
    spdlog::info("实体工厂初始化完成");
    return true;
}

void GameScene::onEnemyArriveHome(const game::defs::EnemyArriveHomeEvent &)
{
    spdlog::info("敌人到达基地");
    // TODO: 处理敌人到达基地的逻辑
}

void GameScene::createTestEnemy()
{
    // 每个起点创建一个敌人
    for (auto start_index : start_points_)
    {
        auto position = waypoint_nodes_[start_index].position_;

        entity_factory_->createEnemyUnits("wolf"_hs, position, start_index);
        entity_factory_->createEnemyUnits("slime"_hs, position, start_index);
        entity_factory_->createEnemyUnits("goblin"_hs, position, start_index);
        entity_factory_->createEnemyUnits("dark_witch"_hs, position, start_index);
    }
}

}   // namespace game::scene