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

public:
    GameRuleSystem(entt::registry &registry, entt::dispatcher &dispatcher);
    ~GameRuleSystem();

    void update(float delta_time);

private:
    // 事件回调函数
    void onEnemyArriveHome(const game::defs::EnemyArriveHomeEvent &event);
};

}   // namespace game::system