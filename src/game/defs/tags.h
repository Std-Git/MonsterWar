#pragma once

/* 在 entt 中，空的结构体可以作为“标签组件”使用 */
/* 他不占用内存空间，当需要给实体添加标签时，这是最推介的做法 */
namespace game::defs
{

struct DeadTag {};          ///< @brief 死亡标签，用于标记死亡并延迟删除

struct FaceLeftTag {};      ///< @brief 角色图片默认朝右，如果朝左就添加一个标签，用于翻转判断

struct MeleeUnitTag {};     ///< @brief 近战单位标签
struct RangedUnitTag {};    ///< @brief 远程单位标签

}   // namespace game::defs