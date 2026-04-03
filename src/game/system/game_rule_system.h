#pragma once
#include "../defs/events.h"
#include <entt/entity/fwd.hpp>
#include <entt/signal/fwd.hpp>

namespace game::system
{
class GameRuleSystem
{
    entt::registry &registry_;
    entt::dispatcher &dispatcher_;

    bool is_level_clear_{false};    ///< @brief 是否关卡已通关
    float level_clear_timer_{0.0f}; ///< @brief 关卡通关计时器(实现延迟切换场景)

public:
    GameRuleSystem(entt::registry &registry, entt::dispatcher &dispatcher);
    ~GameRuleSystem();

    void update(float delta_time);

private:
    // 事件回调函数
    void onEnemyArriveHome(const game::defs::EnemyArriveHomeEvent &event);
    void onUpgradeUnitEvent(const game::defs::UpgradeUnitEvent &event);
    void onRetreatEvent(const game::defs::RetreatEvent &event);
    void onLevelClearDelayedEvent(const game::defs::LevelClearDelayedEvent &event);
};

}   // namespace game::system