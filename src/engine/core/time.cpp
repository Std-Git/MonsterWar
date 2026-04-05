#include "time.h"
#include <spdlog/spdlog.h>
#include <SDL3/SDL_timer.h> // 用于 SDL_GetTicksNS() 和 SDL_DelayNS()

namespace engine::core
{

    Time::Time()
    {
        // 初始化 last_time_ 和 frame_start_time_ 为当前时间, 避免第一帧 DeltaTime 过大
        last_time_ = SDL_GetTicksNS();
        frame_start_time_ = last_time_;
        spdlog::trace("Time 初始化, Last time: {}", last_time_);
    }

    void Time::update()
    {
        frame_start_time_ = SDL_GetTicksNS(); // 计入进入 update 的时间

        // 如果游戏暂停，不更新delta_time
        if (is_paused_)
        {
            last_time_ = frame_start_time_;
            delta_time_ = 0.0f;
            return;
        }

        auto current_delta_time = static_cast<double>(frame_start_time_ - last_time_) / 1000000000.0;
        if (target_frame_time_ > 0.0) // 如果设置了目标帧率，则限制帧率，否则 delta_time_ = current_delta_time
        {
            limitFrameRate(static_cast<float>(current_delta_time));
        }
        else
        {
            delta_time_ = current_delta_time;
        }

        last_time_ = SDL_GetTicksNS(); // 记录离开 update 的时间
    }

    void Time::limitFrameRate(float current_delta_time)
    {
        // 如果当前帧耗费时间小于目标时间间隔，则等待剩余时间
        if (current_delta_time < target_frame_time_)
        {
            double time_to_wait_ = target_frame_time_ - current_delta_time;
            Uint64 ns_to_wait = static_cast<Uint64>(time_to_wait_ * 1000000000.0);
            SDL_DelayNS(ns_to_wait);
            delta_time_ = static_cast<double>(SDL_GetTicksNS() - last_time_) / 1000000000.0;
        }
        else
        { // 否则，直接使用当前帧耗费的时间
            delta_time_ = static_cast<double>(current_delta_time);
        }
    }

    float Time::getDeltaTime() const
    {
        return static_cast<float>(delta_time_ * time_scale_);
    }

    float Time::getUnscaledDeltaTime() const
    {
        return static_cast<float>(delta_time_);
    }

    void Time::setTimeScale(float scale)
    {
        if (scale < 0.0f)
        {
            spdlog::warn("Time scale cannot be less than 0.0f. Clamping to 0.0f.");
            scale = 0.0f; // 防止负时间缩放
        }
        time_scale_ = scale;
    }

    float Time::getTimeScale() const
    {
        return static_cast<float>(time_scale_);
    }

    void Time::setTargetFps(int fps)
    {
        if (fps < 0)
        {
            spdlog::warn("Target FPS 不能为负。Setting to 0 (unlimited).");
            target_fps_ = 0;
        }
        else
        {
            target_fps_ = fps;
        }

        if (target_fps_ > 0)
        {
            target_frame_time_ = 1.0 / static_cast<double>(target_fps_);
            spdlog::info("Target FPS 设置为: {} (Frame time: {:.6f}s)", target_fps_, target_frame_time_);
        }
        else
        {
            target_frame_time_ = 0.0;
            spdlog::info("Target FPS 设置为: Unlimited");
        }
    }

    int Time::getTargetFps() const
    {
        return target_fps_;
    }

    void Time::pause()
    {
        is_paused_ = true;
        spdlog::info("Time: 正在拖动窗口, 不更新current_delta_time_");
    }

    void Time::resume()
    {
        is_paused_ = false;
        last_time_ = SDL_GetTicksNS();
        spdlog::info("Time: 窗口没有被拖动, 恢复更新current_delta_time_");
    }

    bool Time::getIsPaused() const
    {
        return is_paused_;
    }

} // namespace engine::core