#include "grid_system.h"
#include "../component/grid_component.h"
#include "../component/stats_component.h"
#include "../component/blocked_by_component.h"
#include "../defs/tags.h"
#include "../defs/constants.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/component/velocity_component.h"
#include "../../engine/utils/events.h"
#include "../../engine/utils/math.h"
#include <entt/entity/view.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::system
{

void GridSystem::update(entt::registry &registry, entt::dispatcher &dispatcher)
{
    //spdlog::trace("BlockSystem::update");
    //  -- 检查阻挡者是否有效 --
    auto view = registry.view<engine::component::TransformComponent,
                              game::component::StatsComponent,
                              game::component::GridComponent>();
    for (auto entity : view)
    {
        auto& grid_component = view.get<component::GridComponent>(entity);
        auto& transform_component = view.get<engine::component::TransformComponent>(entity);
        grid_component.grid_x_ = (transform_component.position_.x - 1) / 9;
        grid_component.grid_y_ = (transform_component.position_.y - 1) / 5;
    }

}

}   // namespace game::system

