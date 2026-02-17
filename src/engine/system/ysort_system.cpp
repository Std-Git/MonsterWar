#include "ysort_system.h"
#include "../component/render_component.h"
#include "../component/transform_component.h"
#include <entt/entity/registry.hpp>

namespace engine::system
{

void YSortSystem::update(entt::registry& registry)
{
    // 让 RenderComponent 的深度 depth_ 等于 TransformComponent 的 y 坐标
    auto view = registry.view<component::RenderComponent, const component::TransformComponent>();
    for (auto entity : view)
    {
        auto& render = view.get<component::RenderComponent>(entity);
        const auto& transform = view.get<const component::TransformComponent>(entity);
        render.depth_ = transform.position_.y;
    }
}

}