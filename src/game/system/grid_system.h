#pragma once

#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>

namespace game::system
{
/**
 * @brief 网格系统
 * 用于更新实体所在网格坐标
 */
class GridSystem
{
public:
    void update(entt::registry& registry, entt::dispatcher& dispatcher);
};

}   // namespace game::system