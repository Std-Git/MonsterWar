#pragma once

namespace game::component
{
/**
 * @brief 僵尸组件，包含目标节点 ID 和自身速度，所在草坪行数
 */
struct EnemyComponent
{
    int target_waypoint_id_;
    float speed_;
};

}   // namespace game::component