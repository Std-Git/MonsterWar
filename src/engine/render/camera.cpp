#include "camera.h"
#include <spdlog/spdlog.h>

namespace engine::render
{

Camera::Camera(glm::vec2 viewport_size,  glm::vec2 position, std::optional<engine::utils::Rect> limit_bounds)
    : viewport_size_(std::move(viewport_size)), position_(std::move(position)), limit_bounds_(std::move(limit_bounds))
{
    spdlog::trace("Camera 初始化成功, 位置: {}, {}", position_.x, position_.y);
}

void Camera::setPosition(glm::vec2 position)
{
    position_ = std::move(position);
    clampPosition();
}

void Camera::move(const glm::vec2& offset)
{
    position_ += offset;
    clampPosition();
}

void Camera::setLimitBounds(std::optional<engine::utils::Rect> limit_bounds)
{
    limit_bounds_ = std::move(limit_bounds);
    clampPosition();    // 设置边界后，立即启用限制
}

const glm::vec2& Camera::getPosition() const
{
    return position_;
}

void Camera::clampPosition()
{
    // 边界检查需要确保相机视图 (position 到 position + viewport_size) 在 limit_bounds 内
    if (limit_bounds_.has_value() && limit_bounds_->size.x > 0 && limit_bounds_->size.y > 0)
    {
        // 计算允许的相机范围
        glm::vec2 min_cam_pos = limit_bounds_->position;
        glm::vec2 max_cam_pos = limit_bounds_->position + limit_bounds_->size - viewport_size_;

        // 确保 max_cam_pos 不小于 min_cam_pos (视窗可能比世界大)
        max_cam_pos.x = std::max(min_cam_pos.x, max_cam_pos.x);
        max_cam_pos.y = std::max(min_cam_pos.y, max_cam_pos.y);

        position_ = glm::clamp(position_, min_cam_pos, max_cam_pos);
    }
    // 如果 limit_bounds_ 无效则不需要进行限制
}

glm::vec2 Camera::worldToScreen(const glm::vec2& world_pos) const
{
    // 将世界坐标减去相机左上角位置
    return world_pos - position_;
}

glm::vec2 Camera::worldToScreenWithParallax(const glm::vec2& world_pos, const glm::vec2& scroll_factor) const
{
    // 相机位置滚动因子
    return world_pos - position_ * scroll_factor;
}

glm::vec2 Camera::screenToWorld(const glm::vec2& screen_pos) const
{
    // 将屏幕坐标加上相机左上角位置
    return screen_pos + position_;
}

glm::vec2 Camera::getViewportSize() const
{
    return viewport_size_;
}

std::optional<engine::utils::Rect> Camera::getLimitBounds() const
{
    return limit_bounds_;
}

void Camera::smoothMoveTo(glm::vec2 target, float duration,
                          std::function<glm::vec2(const glm::vec2 &, const glm::vec2 &, float, float)> easing)
{
    target_position_ = target;
    smooth_move_duration_ = duration;
    smooth_move_elapsed_ = 0.0f;
    smooth_move_start_ = position_;
    easing_func_ = std::move(easing);
}

void Camera::update(float delta_time)
{
    if (!target_position_.has_value())
        return;

    smooth_move_elapsed_ += delta_time;

    if (smooth_move_elapsed_ >= smooth_move_duration_)
    {
        // 移动完成, 设置到目标位置并钳制
        position_ = target_position_.value();
        glm::vec2 pos_before_clamp = position_;
        clampPosition();

        // 检查是否被阻挡
        if (position_ != pos_before_clamp)
        {
            spdlog::warn("Camera 平滑移动被阻挡: 目标位置 ({}, {}) 被限制在 ({}, {})",
                         pos_before_clamp.x, pos_before_clamp.y,
                         position_.x, position_.y);
        }

        target_position_.reset();
    }
    else
    {
        // 插值移动
        if (easing_func_)
        {
            position_ = easing_func_(smooth_move_start_.value(), target_position_.value(),
                                     smooth_move_elapsed_, smooth_move_duration_);
        }
        else
        {
            // 没有提供缓动函数, 使用线性插值
            position_ = glm::mix(smooth_move_start_.value(), target_position_.value(),
                                 smooth_move_elapsed_ / smooth_move_duration_);
        }
        clampPosition();
    }
}

void Camera::cancelSmoothMove()
{
    target_position_.reset();
}

bool Camera::isSmoothMoving() const
{
    return target_position_.has_value();
}

}   // namespace engine::render