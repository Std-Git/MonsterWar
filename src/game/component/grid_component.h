#pragma once

namespace game::component
{
/**
 * @brief 格子组件，记录实体现在在哪个格子中
 */
struct GridComponent
{
    int grid_x_{0};
    int grid_y_{0};
};

}   // namespace game::component