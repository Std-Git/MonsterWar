#include "entity_factory.h"
#include "blueprint_manager.h"
#include "../data/entity_blueprint.h"
#include "../../engine/utils/math.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/component/sprite_component.h"
#include "../../engine/component/animation_component.h"
#include "../../engine/component/velocity_component.h"
#include "../../engine/component/render_component.h"
#include "../defs/tags.h"
#include "../../engine/component/audio_component.h"
#include "../component/projectile_component.h"
#include "../component/stats_component.h"
#include "../component/player_component.h"
#include "../component/blocker_component.h"
#include "../component/enemy_component.h"
#include "../component/class_name_component.h"
#include "../component/unit_prep_component.h"
#include "../component/skill_component.h"
#include <entt/entity/registry.hpp>
#include <entt/core/hashed_string.hpp>
#include <spdlog/spdlog.h>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

using namespace entt::literals;

namespace game::factory
{
EntityFactory::EntityFactory(entt::registry& registry, 
    BlueprintManager& blueprint_manager)
    : registry_(registry), blueprint_manager_(blueprint_manager) {}

entt::entity EntityFactory::createPlayerUnit(entt::id_type class_id, const glm::vec2 &position, int level, int rarity)
{
    auto entity = registry_.create();
    const auto& blueprint = blueprint_manager_.getPlayerClassBlueprint(class_id);
    // --- 添加组件 ---
    // 添加 Transform 组件
    addTransformComponent(entity, position, blueprint.sprite_.scale_);

    // 添加 Sprite 组件
    addSpriteComponent(entity, blueprint.sprite_);

    // 添加 Animation 组件
    addAnimationComponent(entity, blueprint.animations_, blueprint.sprite_, "idle"_hs);

    // 添加 Audio 组件
    addAudioComponent(entity, blueprint.sounds_);

    // 添加 Stats 组件
    addStatsComponent(entity, blueprint.stats_, level, rarity);

    // 添加 Player 组件
    addPlayerComponent(entity, blueprint.player_, rarity);

    // 添加 ProjectileID 组件
    addProjectileIDComponent(entity, blueprint.projectile_id_);

    // 添加 Skill 组件
    //addSkillComponent(entity, blueprint.player_.skill_id_);

    // 补充其他必要组件
    registry_.emplace<game::component::ClassNameComponent>(entity, class_id, blueprint.display_info_.name_);
    registry_.emplace<engine::component::RenderComponent>(entity); // 使用默认主图层
    registry_.emplace<game::defs::HasHealthBarTag>(entity);
    // 未来可添加其他组件

    return entity;
}

entt::entity EntityFactory::createEnemyUnit(entt::id_type class_id, const glm::vec2 &position, int target_waypoint_id, int level, int rarity)
{
    auto entity = registry_.create();
    const auto& blueprint = blueprint_manager_.getEnemyClassBlueprint(class_id);
    // --- 添加组件 ---
    // 添加 Transform 组件
    addTransformComponent(entity, position, blueprint.sprite_.scale_);

    // 添加 Sprite 组件
    addSpriteComponent(entity, blueprint.sprite_);

    // 添加 Animation 组件
    addAnimationComponent(entity, blueprint.animations_, blueprint.sprite_, "walk"_hs);

    // 添加 Audio 组件
    addAudioComponent(entity, blueprint.sounds_);

    // 添加 Stats 组件
    addStatsComponent(entity, blueprint.stats_, level, rarity);

    // 添加 Enemy 组件
    addEnemyComponent(entity, blueprint.enemy_, target_waypoint_id);

    // 添加 ProjectileID 组件
    addProjectileIDComponent(entity, blueprint.projectile_id_);

    // 补充其他必要组件
    registry_.emplace<game::component::ClassNameComponent>(entity, class_id, blueprint.display_info_.name_);
    registry_.emplace<engine::component::RenderComponent>(entity);      // 使用默认主图层
    registry_.emplace<game::defs::HasHealthBarTag>(entity);

    // 未来可添加其他组件
    // spdlog::warn("create enemy unit: {}", blueprint.display_info_.name_);

    return entity;
}

entt::entity EntityFactory::createProjectile(entt::id_type id, const glm::vec2 &start_position, const glm::vec2 &target_position, entt::entity target, float damage)
{
    // 创建投射物实体
    auto entity = registry_.create();
    const auto& blueprint = blueprint_manager_.getProjectileBlueprint(id);
    // 依次添加必要组件
    // 添加 ProjectileComponent
    registry_.emplace<game::component::ProjectileComponent>(entity, 
        target,
        damage,
        start_position, 
        target_position, 
        start_position, 
        blueprint.arc_height_,
        blueprint.total_flight_time_,
        0.0f);
    // 添加 SpriteComponent
    addSpriteComponent(entity, blueprint.sprite_);
    // 添加 TransformComponent
    addTransformComponent(entity, start_position, blueprint.sprite_.scale_);
    // 添加 AudioComponent
    addAudioComponent(entity, blueprint.sounds_);
    // 添加 RenderComponent(让投射物位于主图层+1, 即可以遮住角色)
    registry_.emplace<engine::component::RenderComponent>(entity, engine::component::RenderComponent::MAIN_LAYER + 1);
    return entity;
}

entt::entity EntityFactory::createUnitPrep(entt::id_type name_id, entt::id_type class_id, int cost, const glm::vec2 &position)
{
    auto entity = registry_.create();
    const auto& blueprint = blueprint_manager_.getPlayerClassBlueprint(class_id);
    addTransformComponent(entity, position, blueprint.sprite_.scale_);
    addSpriteComponent(entity, blueprint.sprite_);
    // 直接添加UnitPrepComponent
    registry_.emplace<game::component::UnitPrepComponent>(entity, 
        name_id, 
        blueprint.player_.type_,
        blueprint.stats_.range_,
        cost);

    // 补充渲染组件与显示攻击范围标志
    registry_.emplace<engine::component::RenderComponent>(entity, 100); // 显示优先度很高
    if (blueprint.player_.type_ == game::defs::PlayerType::RANGED)
    {
        registry_.emplace<game::defs::ShowRangeTag>(entity);
    }
    return entity;
}

// -- 组件创建函数 --

entt::entity EntityFactory::createEnemyDeadEffect(entt::id_type class_id, const glm::vec2 &position, const bool is_flipped)
{
    auto entity = registry_.create();
    const auto& blueprint = blueprint_manager_.getEnemyClassBlueprint(class_id);
    // 添加 Transform 组件
    addTransformComponent(entity, position);

    // 添加 Sprite 组件
    addSpriteComponent(entity, blueprint.sprite_, is_flipped);

    // 添加 Animation 组件 (死亡动画名称为"damage")
    addOneAnimationComponent(entity, blueprint.animations_.at("damage"_hs), blueprint.sprite_, "damage"_hs);

    // 补充其他必要组件
    registry_.emplace<engine::component::RenderComponent>(entity);
    registry_.emplace<game::defs::OneShotRemoveTag>(entity);
    return entity;
}

entt::entity EntityFactory::createEffect(entt::id_type effect_id, const glm::vec2 &position, const bool is_flipped)
{
    auto entity = registry_.create();
    const auto& blueprint = blueprint_manager_.getEffectBlueprint(effect_id);
    // 添加 Transform 组件
    addTransformComponent(entity, position);

    // 添加 Sprite 组件
    addSpriteComponent(entity, blueprint.sprite_, is_flipped);

    // 添加 Animation 组件, 只有一个动画，名称为特效id
    addOneAnimationComponent(entity, blueprint.animation_, blueprint.sprite_, effect_id);

    // 补充其他必要组件
    registry_.emplace<engine::component::RenderComponent>(entity, engine::component::RenderComponent::MAIN_LAYER + 10);
    registry_.emplace<game::defs::OneShotRemoveTag>(entity);
    return entity;
}

// -- 组件创建函数 --

entt::entity EntityFactory::createSkillDisplay(entt::id_type effect_id, const glm::vec2 &position)
{
    auto entity = registry_.create();
    const auto& effect_blueprint = blueprint_manager_.getEffectBlueprint(effect_id);
    // 添加 Transform 组件
    addTransformComponent(entity, position);

    // 添加 Sprite 组件
    addSpriteComponent(entity, effect_blueprint.sprite_);

    // 添加 Animation 组件 (角色上方的技能标识，循环播放)
    addOneAnimationComponent(entity, effect_blueprint.animation_, effect_blueprint.sprite_, effect_id, true);

    // 补充其他必要组件
    registry_.emplace<engine::component::RenderComponent>(entity, engine::component::RenderComponent::MAIN_LAYER + 20);
    return entity;
}

void EntityFactory::addTransformComponent(entt::entity entity, const glm::vec2 &position, const glm::vec2 &scale, float rotation)
{
    registry_.emplace<engine::component::TransformComponent>(entity, position, scale, rotation);
}

void EntityFactory::addSpriteComponent(entt::entity entity, const data::SpriteBlueprint &sprite, const bool is_flipped)
{
    registry_.emplace<engine::component::SpriteComponent>(entity,
        engine::component::Sprite(sprite.path_,
                                  sprite.src_rect_,
                                  is_flipped),
        sprite.size_,
        sprite.offset_);
    // 如果图片朝左就添加 FaceLeftTag
    if (!sprite.face_right_)
    {
        registry_.emplace<game::defs::FaceLeftTag>(entity);
    }
}

void EntityFactory::addAnimationComponent(entt::entity entity, 
    const std::unordered_map<entt::id_type, data::AnimationBlueprint> &animation_blueprints, 
    const data::SpriteBlueprint &sprite_blueprint, 
    entt::id_type default_animation_id)
{
    // 先创建 map 容器
    std::unordered_map<entt::id_type, engine::component::Animation> animations;
    // 针对每一个动画
    for (const auto& [anim_id, anim_blueprint] : animation_blueprints)
    {
        // 创建动画帧容器
        std::vector<engine::component::AnimationFrame> frames;
        // ---- 获取当前动画的帧宽高 ----
        float frame_width = sprite_blueprint.src_rect_.size.x;
        float frame_height = sprite_blueprint.src_rect_.size.y;
        if (anim_blueprint.width_.has_value())
            frame_width = *anim_blueprint.width_;
        if (anim_blueprint.height_.has_value())
            frame_height = *anim_blueprint.height_;
        // 依次读取蓝图中的每一帧
        for (const auto& frame_index : anim_blueprint.frames_)
        {
            engine::utils::Rect source_rect = sprite_blueprint.src_rect_;
            // 通过索引计算每一帧的源矩形区域
            source_rect.position.x += frame_index * frame_width;
            // 行偏移：使用全局帧高（不是当前动画的帧高）
            source_rect.position.y += anim_blueprint.row_ * sprite_blueprint.src_rect_.size.y;
            // 源矩形尺寸设置为当前动画的宽高
            source_rect.size.x = frame_width;
            source_rect.size.y = frame_height;
            // 创建动画帧并插入动画帧容器
            frames.emplace_back(source_rect, anim_blueprint.ms_per_frame_);
        }
        // ---------- 新增：提取动画级别的尺寸和偏移 ----------
        std::optional<glm::vec2> anim_size;
        std::optional<glm::vec2> anim_offset;
        // 仅当 width 和 height 同时存在时才设置 size
        if (anim_blueprint.width_.has_value() && anim_blueprint.height_.has_value())
        {
            anim_size = glm::vec2(*anim_blueprint.width_, *anim_blueprint.height_);
        }
        // 仅当 offset_x 和 offset_y 同时存在时才设置 offset
        if (anim_blueprint.offset_x_.has_value() && anim_blueprint.offset_y_.has_value())
        {
            anim_offset = glm::vec2(*anim_blueprint.offset_x_, *anim_blueprint.offset_y_);
        }
        // --------------------------------------------------

        // 将创建好的动画帧容器插入动画 map 容器 (可以直接使用蓝图的事件信息)
        animations.emplace(anim_id, engine::component::Animation(std::move(frames), anim_blueprint.events_, true, anim_size, anim_offset));
    }
    // 通过动画 map 容器创建动画组件
    registry_.emplace<engine::component::AnimationComponent>(entity, std::move(animations), default_animation_id);
}

void EntityFactory::addOneAnimationComponent(entt::entity entity, 
    const data::AnimationBlueprint &animation_blueprint, 
    const data::SpriteBlueprint &sprite_blueprint, 
    entt::id_type animation_id, bool loop)
{
    // 创建动画帧容器
    std::vector<engine::component::AnimationFrame> frames;
    // ---- 获取当前动画的帧宽高 ----
    float frame_width = sprite_blueprint.src_rect_.size.x;
    float frame_height = sprite_blueprint.src_rect_.size.y;
    if (animation_blueprint.width_.has_value())
        frame_width = *animation_blueprint.width_;
    if (animation_blueprint.height_.has_value())
        frame_height = *animation_blueprint.height_;

    // 依次读取蓝图中的每一个动画帧，并插入容器
    for (const auto& frame_index : animation_blueprint.frames_)
    {
        engine::utils::Rect source_rect = sprite_blueprint.src_rect_;
        source_rect.position.x += frame_index * frame_width;
        source_rect.position.y += animation_blueprint.row_ * frame_height;
        // 创建动画帧并插入动画帧容器
        frames.emplace_back(source_rect, animation_blueprint.ms_per_frame_);
    }
    // 创建动画 map 容器
    std::unordered_map<entt::id_type, engine::component::Animation> animations;
    // 将创建好的动画帧容器插入动画 map 容器 (只有一个动画)
    animations.emplace(animation_id, engine::component::Animation(std::move(frames), animation_blueprint.events_, loop));
    // 通过动画 map 容器创建动画组件
    registry_.emplace<engine::component::AnimationComponent>(entity, std::move(animations), animation_id);
}

void EntityFactory::addStatsComponent(entt::entity entity, const data::StatsBlueprint &stats, int level, int rarity)
{
    // 计算等级和稀有度对属性的影响 (未来可以改成数据驱动方便调整)
    auto hp = engine::utils::statModify(stats.hp_, level, rarity);
    auto atk = engine::utils::statModify(stats.atk_, level, rarity);
    auto def = engine::utils::statModify(stats.def_, level, rarity);

    registry_.emplace_or_replace<game::component::StatsComponent>(entity, 
        hp, 
        hp,
        atk, 
        def,
        stats.range_,
        stats.atk_interval_,
        0.0f,
        level,
        rarity);
}

void EntityFactory::addPlayerComponent(entt::entity entity, const data::PlayerBlueprint &player, int rarity)
{
    auto cost = static_cast<int>(std::round(player.cost_ * (0.9f + 0.1f * rarity)));
    registry_.emplace<game::component::PlayerComponent>(entity, cost);
    // 添加类型标签(近战，远程，治疗)
    if (player.type_ == game::defs::PlayerType::MELEE)
    {
        registry_.emplace<game::defs::MeleeUnitTag>(entity);    // 近战单位标签
        // 近战类型添加阻挡者组件
        registry_.emplace<game::component::BlockerComponent>(entity, player.block_);
    }
    else if (player.type_ == game::defs::PlayerType::RANGED)    // 远程单位标签
    {
        registry_.emplace<game::defs::RangedUnitTag>(entity);
        if (player.healer_)
        {
            registry_.emplace<game::defs::HealerTag>(entity);   // 治疗单位标签
        }
    }
    // TODO:未来添加技能组件
}

void EntityFactory::addEnemyComponent(entt::entity entity, const data::EnemyBlueprint &enemy, int target_waypoint_id)
{
    registry_.emplace<game::component::EnemyComponent>(entity, target_waypoint_id, enemy.speed_);
    registry_.emplace<engine::component::VelocityComponent>(entity, glm::vec2(0.0f, 0.0f));
    if (enemy.ranged_)  // 添加远程或近战标签备用
    {
        registry_.emplace<game::defs::RangedUnitTag>(entity);
    }
    else
    {
        registry_.emplace<game::defs::MeleeUnitTag>(entity);
    }
}

void EntityFactory::addAudioComponent(entt::entity entity, const data::SoundBlueprint &sounds)
{
    if (sounds.sounds_.empty()) return;
    // 将 sounds_ 中的键值对转换为 audio_map 中的键值对
    std::unordered_map<entt::id_type, entt::id_type> audio_map;
    for (const auto& [sound_key, sound_id] : sounds.sounds_)
    {
        audio_map.emplace(sound_key, sound_id);
    }
    registry_.emplace<engine::component::AudioComponent>(entity, std::move(audio_map));
}

void EntityFactory::addProjectileIDComponent(entt::entity entity, entt::id_type id)
{
    if (id == entt::null) return;
    registry_.emplace<game::component::ProjectileIDComponent>(entity, id);
}

void EntityFactory::addSkillComponent(entt::entity entity, entt::id_type skill_id)
{
    const auto& skill = blueprint_manager_.getSkillBlueprint(skill_id);
    registry_.emplace<game::component::SkillComponent>(entity, 
        skill_id, 
        entt::null,
        skill.name_,
        skill.description_,
        skill.cooldown_,
        skill.duration_,
        skill.cooldown_ / 2.0f, // 初始技能冷却时间为技能冷却时间的一半
        0.0f);
    // 如果是被动技能，则添加 PassiveSkillTag 与 SkillReadyTag 标签
    if (skill.passive_)
    {
        registry_.emplace<game::defs::PassiveSkillTag>(entity);
        registry_.emplace<game::defs::SkillReadyTag>(entity);
    }
}

}   // namespace game::factory