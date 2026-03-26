#include "game_rule_system.h"
#include "../defs/tags.h"
#include "../data/game_stats.h"
#include "../factory/blueprint_manager.h"
#include "../component/cost_regen_component.h"
#include "../component/stats_component.h"
#include "../component/class_name_component.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/utils/math.h"
#include "../../engine/utils/events.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::system
{

GameRuleSystem::GameRuleSystem(entt::registry &registry, entt::dispatcher &dispatcher)
    :registry_(registry), dispatcher_(dispatcher)
{
    dispatcher_.sink<game::defs::EnemyArriveHomeEvent>().connect<&GameRuleSystem::onEnemyArriveHome>(this);
    dispatcher_.sink<game::defs::UpgradeUnitEvent>().connect<&GameRuleSystem::onUpgradeUnitEvent>(this);
    dispatcher_.sink<game::defs::RetreatEvent>().connect<&GameRuleSystem::onRetreatEvent>(this);
}

GameRuleSystem::~GameRuleSystem()
{
    dispatcher_.disconnect(this);
}

void GameRuleSystem::update(float delta_time)
{
    //更新cost
    auto& game_stats = registry_.ctx().get<game::data::GameStats&>();
    game_stats.cost_ += game_stats.cost_gen_per_second_ * delta_time;
    // 更新 COST 恢复
    auto view_cost_regen = registry_.view<component::CostRegenComponent>();
    for (auto entity : view_cost_regen)
    {
        auto& cost_regen = view_cost_regen.get<component::CostRegenComponent>(entity);
        game_stats.cost_ += cost_regen.rate_ * delta_time;
    }
}

void GameRuleSystem::onEnemyArriveHome(const game::defs::EnemyArriveHomeEvent &event)
{
    spdlog::info("敌人到达基地");
    auto& game_stats = registry_.ctx().get<game::data::GameStats&>();
    game_stats.enemy_arrived_count_++;      // 敌人到达数量 +1
    game_stats.home_hp_ -= 1;               // 基地血量 -1
    if (game_stats.home_hp_ <= 0)
    {
        spdlog::warn("基地被摧毁");
        // TODO: 切换场景逻辑
    }
}

void GameRuleSystem::onUpgradeUnitEvent(const game::defs::UpgradeUnitEvent &event)
{
    if (event.entity_ == entt::null || !registry_.valid(event.entity_)) return;
    // 扣除COST
    auto& game_stats = registry_.ctx().get<game::data::GameStats&>();
    game_stats.cost_ -= event.cost_;
    // 获取Stats组件并让其等级 + 1
    auto& stats = registry_.get<game::component::StatsComponent>(event.entity_);
    stats.level_++;
    // 更新属性 (需要从蓝图中获取基础数据，然后根据等级和稀有度修改Stats组件)
    auto& blueprint_manager = registry_.ctx().get<std::shared_ptr<game::factory::BlueprintManager>>();
    const auto& class_name = registry_.get<game::component::ClassNameComponent>(event.entity_);
    const auto& stats_blueprint = blueprint_manager->getPlayerClassBlueprint(class_name.class_id_).stats_;
    stats.hp_ = engine::utils::statModify(stats_blueprint.hp_, stats.level_, stats.rarity_);
    stats.max_hp_ = engine::utils::statModify(stats_blueprint.hp_, stats.level_, stats.rarity_);
    stats.atk_ = engine::utils::statModify(stats_blueprint.atk_, stats.level_, stats.rarity_);
    stats.def_ = engine::utils::statModify(stats_blueprint.def_, stats.level_, stats.rarity_);
    // 去除受伤标记
    registry_.remove<game::defs::InjuredTag>(event.entity_);
    // 创建特效
    const auto& transform = registry_.get<engine::component::TransformComponent>(event.entity_);
    dispatcher_.enqueue(game::defs::EffectEvent{"level_up"_hs, transform.position_, false});
    // 播放音效
    dispatcher_.enqueue(engine::utils::PlaySoundEvent{event.entity_, "unit_upgrade"_hs});
}
void GameRuleSystem::onRetreatEvent(const game::defs::RetreatEvent &event)
{
    if (event.entity_ == entt::null || !registry_.valid(event.entity_)) return;
    // 返还cost
    auto& game_stats = registry_.ctx().get<game::data::GameStats&>();
    game_stats.cost_ += event.cost_;
    // 发送移除单位事件
    dispatcher_.enqueue(game::defs::RemovePlayerUnitEvent{event.entity_});
}

}   // namespace game::system