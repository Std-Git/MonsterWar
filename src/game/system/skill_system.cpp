#include "skill_system.h"
#include "../defs/events.h"
#include "../defs/tags.h"
#include "../component/stats_component.h"
#include "../component/skill_component.h"
#include "../component/cost_regen_component.h"
#include "../factory/entity_factory.h"
#include "../factory/blueprint_manager.h"
#include "../../engine/component/transform_component.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>

using namespace entt::literals;

namespace game::system
{
    SkillSystem::SkillSystem(entt::registry &registry, entt::dispatcher &dispatcher, game::factory::EntityFactory &entity_factory)
        : registry_(registry), dispatcher_(dispatcher), entity_factory_(entity_factory)
    {
        dispatcher_.sink<game::defs::SkillReadyEvent>().connect<&SkillSystem::onSkillReadyEvent>(this);
        dispatcher_.sink<game::defs::SkillActiveEvent>().connect<&SkillSystem::onSkillActiveEvent>(this);
        dispatcher_.sink<game::defs::SkillDurationEndEvent>().connect<&SkillSystem::onSkillDurationEndEvent>(this);
        dispatcher_.sink<game::defs::RemovePlayerUnitEvent>().connect<&SkillSystem::onRemoveUnitEvent>(this);
    }

    SkillSystem::~SkillSystem()
    {
        dispatcher_.disconnect(this);
    }

    void SkillSystem::onSkillReadyEvent(const game::defs::SkillReadyEvent &event)
    {
        if (event.entity_ == entt::null || !registry_.valid(event.entity_))
            return;
        // 获取技能和位置组件
        auto &skill = registry_.get<game::component::SkillComponent>(event.entity_);
        const auto &transform = registry_.get<engine::component::TransformComponent>(event.entity_);
        // 先剔除可能存在的显示实体
        if (skill.display_entity_ != entt::null && registry_.valid(skill.display_entity_))
        {
            registry_.emplace_or_replace<game::defs::DeadTag>(skill.display_entity_);
        }
        // 创建新的显示实体 (技能准备就绪)
        skill.display_entity_ = entity_factory_.createSkillDisplay("skill_ready"_hs, transform.position_ + game::defs::SKILL_DISPLAY_OFFSET);
    }

    void SkillSystem::onSkillActiveEvent(const game::defs::SkillActiveEvent &event)
    {
        if (event.entity_ == entt::null || !registry_.valid(event.entity_))
            return;
        // 如果技能未就绪，则返回
        if (!registry_.any_of<game::defs::SkillReadyTag>(event.entity_))
            return;

        // 获取技能和位置组件
        auto &skill = registry_.get<game::component::SkillComponent>(event.entity_);
        const auto &transform = registry_.get<engine::component::TransformComponent>(event.entity_);
        // 先剔除可能存在的显示实体
        if (skill.display_entity_ != entt::null && registry_.valid(skill.display_entity_))
        {
            registry_.emplace_or_replace<game::defs::DeadTag>(skill.display_entity_);
        }
        // 创建新的显示实体 (技能激活)
        skill.display_entity_ = entity_factory_.createSkillDisplay("skill_active"_hs, transform.position_ + game::defs::SKILL_DISPLAY_OFFSET);

        // 移除技能准备就绪标签, 添加技能激活标签
        registry_.remove<game::defs::SkillReadyTag>(event.entity_);
        registry_.emplace<game::defs::SkillActiveTag>(event.entity_);

        // 添加 Buff
        addBuff(event.entity_, skill.skill_id_);
    }

    void SkillSystem::onSkillDurationEndEvent(const game::defs::SkillDurationEndEvent &event)
    {
        if (event.entity_ == entt::null || !registry_.valid(event.entity_))
            return;
        // 获取技能组件
        auto &skill = registry_.get<game::component::SkillComponent>(event.entity_);
        // 剔除技能显示实体
        if (skill.display_entity_ != entt::null && registry_.valid(skill.display_entity_))
        {
            registry_.emplace_or_replace<game::defs::DeadTag>(skill.display_entity_);
        }

        // 移除技能激活标签
        registry_.remove<game::defs::SkillActiveTag>(event.entity_);

        // 移除 Buff
        removeBuff(event.entity_, skill.skill_id_);
    }

    void SkillSystem::onRemoveUnitEvent(const game::defs::RemovePlayerUnitEvent &event)
    {
        if (event.entity_ == entt::null || !registry_.valid(event.entity_))
            return;
        // 移除技能显示实体
        if (auto skill = registry_.try_get<game::component::SkillComponent>(event.entity_); skill)
        {
            if (skill->display_entity_ != entt::null && registry_.valid(skill->display_entity_))
            {
                registry_.emplace_or_replace<game::defs::DeadTag>(skill->display_entity_);
            }
        }
    }

    void SkillSystem::addBuff(entt::entity entity, entt::id_type skill_id)
    {
        if (entt::null == entity || !registry_.valid(entity))
            return;

        // 获取 Buff 信息
        auto blueprint_manager = registry_.ctx().get<std::shared_ptr<game::factory::BlueprintManager>>();
        const auto &skill_blueprint = blueprint_manager->getSkillBlueprint(skill_id);
        const auto &buff_blueprint = skill_blueprint.buff_;

        // 将 Buff 应用到角色的 Stats 中
        auto &stats = registry_.get<game::component::StatsComponent>(entity);
        stats.hp_ *= buff_blueprint.hp_multiplier_;
        stats.atk_ *= buff_blueprint.atk_multiplier_;
        stats.def_ *= buff_blueprint.def_multiplier_;
        stats.range_ *= buff_blueprint.range_multiplier_;
        stats.atk_interval_ *= buff_blueprint.atk_interval_multiplier_;

        // 若存在 Cost 相关Buff, 则添加 COST 恢复组件
        if (buff_blueprint.cost_regen_ > 0.0f)
        {
            registry_.emplace_or_replace<game::component::CostRegenComponent>(entity, buff_blueprint.cost_regen_);
        }
    }

    void SkillSystem::removeBuff(entt::entity entity, entt::id_type skill_id)
    {
        if (entt::null == entity || !registry_.valid(entity))
            return;

        // 获取 Buff 信息
        auto blueprint_manager = registry_.ctx().get<std::shared_ptr<game::factory::BlueprintManager>>();
        const auto &skill_blueprint = blueprint_manager->getSkillBlueprint(skill_id);
        const auto &buff_blueprint = skill_blueprint.buff_;

        // 将 Buff 从角色的 Stats 中移除
        auto &stats = registry_.get<game::component::StatsComponent>(entity);
        stats.hp_ /= buff_blueprint.hp_multiplier_;
        stats.atk_ /= buff_blueprint.atk_multiplier_;
        stats.def_ /= buff_blueprint.def_multiplier_;
        stats.range_ /= buff_blueprint.range_multiplier_;
        stats.atk_interval_ /= buff_blueprint.atk_interval_multiplier_;

        // 若存在 Cost 相关Buff, 则移除 COST 恢复组件
        if (buff_blueprint.cost_regen_ > 0.0f)
        {
            registry_.remove<game::component::CostRegenComponent>(entity);
        }
    }
} // namespace game::system
