#pragma once
#include "ui_state.h"

namespace engine::ui::state
{

/**
 * @brief 按下状态
 * 
 * 当鼠标按下 UI 元素时，会切换到该状态
 */
class UIPressedState final : public UIState
{
    friend class engine::ui::UIInteractive;
public:
    UIPressedState(engine::ui::UIInteractive *owner);
    ~UIPressedState();

private:
    void enter() override;
    void update(float delta_time, engine::core::Context &context) override;
    bool onMouseReleased();
};

}   // namespace engine::ui::state  