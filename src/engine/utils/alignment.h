#ifndef ENGINE_UITLS_ALIGNMENT_H
#define ENGINE_UITLS_ALIGNMENT_H

namespace engine::utils 
{
/**
 * @enum class Alignment
 * @brief 定义对象或组件相对于其位置参考点的对齐方式
 */

enum class Alignment
{
    NONE,               // 不指定对齐方式，偏移量通常为 (0, 0) 或手动设置
    TOP_LEFT,           // 左上角
    TOP_CENTER,         // 顶部中心
    TOP_RIGHT,          // 右上角
    CENTER_LEFT,        // 左侧中心
    CENTER,             // 中心
    CENTER_RIGHT,       // 右侧中心
    BOTTOM_LEFT,        // 左下角
    BOTTOM_CENTER,      // 底部中心
    BOTTOM_RIGHT        // 右下角
};

}   // namespace engine::utils

#endif  // ENGINE_UITLS_ALIGNMENT_H