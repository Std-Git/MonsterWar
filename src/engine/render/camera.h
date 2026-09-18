#pragma once
#include "../utils/math.h"
#include <optional>
#include <functional>

namespace engine::render
{

    /**
     * @brief 相机类负责管理相机位置合适口大小，并提供坐标转换功能
     * 他还包含限制相机移动范围的边界，以及平滑移动逻辑
     */

    class Camera final
    {
    private:
        glm::vec2 viewport_size_;                         ///< @brief 视口大小 (屏幕大小)
        glm::vec2 position_;                              ///< @brief 相机左上角的世界坐标
        std::optional<engine::utils::Rect> limit_bounds_; ///< @brief 相机移动范围限制,空值表示不限制

        // 平滑移动相关成员
        std::optional<glm::vec2> target_position_;                                                 ///< @brief 平滑移动目标位置
        std::function<glm::vec2(const glm::vec2 &, const glm::vec2 &, float, float)> easing_func_; ///< @brief 缓动函数, 接受(起点, 终点, 已过时间, 总时间)返回插值位置
        float smooth_move_duration_ = 0.0f;                                                        ///< @brief 平滑移动持续时间(秒)
        float smooth_move_elapsed_ = 0.0f;                                                         ///< @brief 平滑移动已过时间(秒)
        std::optional<glm::vec2> smooth_move_start_;                                               ///< @brief 平滑移动起始位置

    public:
        /**
         * @brief 构造相机对象
         * @param viewport_size 视口大小
         * @param position 相机位置
         * @param limit_bounds 相机移动范围限制
         */
        Camera(glm::vec2 viewport_size, glm::vec2 position = glm::vec2(0.0f, 0.0f), std::optional<engine::utils::Rect> limit_bounds = std::nullopt);

        void move(const glm::vec2 &offset); ///< @brief 移动相机位置

        glm::vec2 worldToScreen(const glm::vec2 &world_pos) const;                                             ///< @brief 将世界坐标转换为屏幕坐标
        glm::vec2 worldToScreenWithParallax(const glm::vec2 &world_pos, const glm::vec2 &scroll_factor) const; ///< @brief 将世界坐标转换为屏幕坐标，考虑视差效果
        glm::vec2 screenToWorld(const glm::vec2 &screen_pos) const;                                            ///< @brief 将屏幕坐标转换为世界坐标

        void setPosition(glm::vec2 position);                                 ///< @brief 设置相机位置
        void setLimitBounds(std::optional<engine::utils::Rect> limit_bounds); ///< @brief 设置相机移动范围限制

        const glm::vec2 &getPosition() const;                      ///< @brief 获取相机位置
        std::optional<engine::utils::Rect> getLimitBounds() const; ///< @brief 获取相机移动范围限制
        glm::vec2 getViewportSize() const;                         ///< @brief 获取视口大小

        /**
         * @brief 开始平滑移动到目标位置
         * @param target 目标位置
         * @param duration 移动持续时间(秒)
         * @param easing 缓动函数, 为空时使用 easeInOutCubic
         * @note 调用后每帧需调用 update() 驱动进度, 可通过 cancelSmoothMove() 取消
         */
        void smoothMoveTo(glm::vec2 target, float duration,
                          std::function<glm::vec2(const glm::vec2 &, const glm::vec2 &, float, float)> easing = {});

        /**
         * @brief 每帧更新, 处理平滑移动逻辑
         * @param delta_time 帧间隔时间(秒)
         * @note 各 Scene 根据自身需求决定是否调用, 不需要相机移动逻辑的 Scene 不调用即可
         */
        void update(float delta_time);

        /**
         * @brief 取消当前平滑移动
         */
        void cancelSmoothMove();

        /**
         * @brief 检查是否正在平滑移动
         * @return 是否正在平滑移动
         */
        bool isSmoothMoving() const;

        // 禁用拷贝和移动语义
        Camera(const Camera &) = delete;
        Camera &operator=(const Camera &) = delete;
        Camera(Camera &&) = delete;
        Camera &operator=(Camera &&) = delete;

    private:
        void clampPosition(); ///< @brief 将相机位置限制在移动范围之内
    };


}   // namespace engine::render