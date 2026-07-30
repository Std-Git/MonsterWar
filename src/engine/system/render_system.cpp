#include "render_system.h"
#include "../render/renderer.h"
#include "../render/camera.h"
#include "../component/transform_component.h"
#include "../component/sprite_component.h"
#include "../component/render_component.h"
#include "../component/animation_component.h"
#include <spdlog/spdlog.h>

namespace engine::system
{
void RenderSystem::update(entt::registry &registry, render::Renderer &renderer, const render::Camera &camera)
{
    //spdlog::trace("RenderSystem::update");

    // 对 RenderComponent storage 排序，比较规则由 RenderComponent::operator< 定义。
    registry.sort<component::RenderComponent>([](const auto& lhs, const auto& rhs) {
        return lhs < rhs;
    });

    // EnTT 的 view 默认会选择元素最少的 storage 驱动迭代。
    // 显式指定使用 RenderComponent，才能保证遍历顺序与上面的排序一致。
    auto view = registry.view<component::RenderComponent, component::TransformComponent, component::SpriteComponent>();
    view.use<component::RenderComponent>();
    for (auto entity : view)
    {
        const auto& render = view.get<component::RenderComponent>(entity);
        const auto &transform = view.get<component::TransformComponent>(entity);
        const auto &sprite = view.get<component::SpriteComponent>(entity);

        glm::vec2 size = sprite.size_;
        glm::vec2 offset = sprite.offset_;

        // 如果存在 AnimationComponent，尝试获取当前动画的自定义值
        if (auto animation = registry.try_get<component::AnimationComponent>(entity); animation)
        {
            auto it = animation->animations_.find(animation->current_animation_id_);
            if (it != animation->animations_.end())
            {
                const auto &anim = it->second;
                if (anim.size_.has_value())
                    size = *anim.size_;
                if (anim.offset_.has_value())
                    offset = *anim.offset_;
            }
        }
        // 对方向的额外偏移
        if (sprite.sprite_.is_flipped_)
        {
            offset.x = -size.x * transform.scale_.x - offset.x; // size.x / 2.0f - offset.x + size.x / 2.0f;
        }

        auto position = transform.position_ + offset;   // 位置 = 变换组件的位置 + 精灵的偏移
        auto final_size = size * transform.scale_;              // 大小 = 精灵的大小 * 变换组件的缩放
        // 绘制时应用Render组件中的颜色调整参数
        renderer.drawSprite(camera, sprite.sprite_, position, final_size, transform.rotation_, render.color_);
    }
}

}   // namespace engine::system