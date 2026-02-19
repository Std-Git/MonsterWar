#pragma once

namespace engine::component 
{
/**
 * @brief 渲染组件，包含图层 ID 和深度  
 */    
struct RenderComponent
{
    static constexpr int MAIN_LAYER{10};    ///< @brief 主图层ID,默认为10

    int layer_{};    ///< @brief 图层 ID, 数字越小越先绘制
    float depth_{};  ///< @brief 在同一图层内的深度，数字越小越先绘制    
                    /* 可用于实现 y-sort 排序，也可设定其他渲染逻辑顺序 */
    //TODO: 未来可添加其他信息，比如透明度等
     
    RenderComponent(int layer = MAIN_LAYER, float depth = 0.0f) : layer_(layer), depth_(depth) {}

    // 重载比较运算符，用于排序
    bool operator<(const RenderComponent& other) const
    {
        if (layer_ == other.layer_)   // 如果图层相同，则比较深度
        {
            return depth_ < other.depth_;
        }
        return layer_ < other.layer_;    // 如果图层不同，则比较图层 ID
    }
};

}   // namespace engine::component