#pragma once
#include "../defs/events.h"
#include <vector>
#include <algorithm>
#include <utility>
#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>

namespace engine::core
{
    class Context;
}

namespace engine::ui
{
    class UIPanel;
    class UIManager;
    class UIButton;
    class UIImage;
}

namespace game::ui
{
    
/**
 * @brief 单位肖像UI
 *
 * 负责管理单位肖像UI的创建，更新和排列
 */
class SelectCardUI
{
    // -- 构造函数传入的外部组件引用 --
    entt::registry &registry_;
    engine::ui::UIManager &ui_manager_;
    engine::core::Context &context_;

    // -- 空卡槽相关成员 --
    engine::ui::UIPanel* empty_slots_panel_;            ///< @brief 空卡槽面板
    float slots_anim_elapsed_{0.0f};                    ///< @brief 空卡槽动画计时
    bool slots_anim_triggered_{false};                  ///< @brief 空卡槽动画是否已触发
    bool slots_anim_completed_{false};                  ///< @brief 空卡槽动画是否已完成
    static constexpr float SLOTS_ANIM_DURATION{0.5f};   ///< @brief 空卡槽动画持续时间(秒)
    static constexpr int EMPTY_SLOT_COUNT{8};  ///< @brief 空卡槽数量

    engine::ui::UIPanel *choose_card_panel_; ///< @brief 选择植物面板
    // -- 卡牌点选相关成员 --
    /** @brief 选择面板中的卡牌按钮信息 */
    struct CardButtonInfo
    {
        engine::ui::UIButton *button = nullptr;      ///< @brief 卡牌按钮指针
        engine::ui::UIPanel *card_panel = nullptr;   ///< @brief 卡牌面板指针(遮罩添加到此面板下)
        entt::id_type card_id = entt::id_type{};     ///< @brief 卡牌ID
        bool selected = false;                       ///< @brief 是否已选中
        engine::ui::UIPanel *dark_overlay = nullptr; ///< @brief 选中时的深色遮罩
        int card_cost = 0; ///< @brief 卡牌cost值
    };

    /** @brief 飞行中的卡牌信息 */
    struct FlyingCardInfo
    {
        engine::ui::UIPanel *panel = nullptr; ///< @brief 飞行中的卡牌面板
        glm::vec2 start_pos{};                ///< @brief 起始位置
        glm::vec2 end_pos{};                  ///< @brief 目标位置
        float elapsed{0.0f};                  ///< @brief 动画计时
        int slot_index{-1};                   ///< @brief 目标槽位索引
        int card_index{-1};                   ///< @brief 源卡牌在 choose_card_buttons_ 中的索引
    };
    /** @brief 槽位移动画信息 */
    struct SlotMoveAnimation
    {
        engine::ui::UIButton *button = nullptr; ///< @brief 移动的卡牌按钮
        glm::vec2 start_pos{};                  ///< @brief 起始位置
        glm::vec2 end_pos{};                    ///< @brief 目标位置
        float elapsed{0.0f};                    ///< @brief 动画计时
    };

    static constexpr float CARD_ANIM_DURATION{0.3f};      ///< @brief 卡牌飞行动画持续时间(秒)
    static constexpr float SLOT_MOVE_ANIM_DURATION{0.2f}; ///< @brief 槽位卡牌上移动画持续时间(秒)
    static constexpr int CARDS_PER_ROW{4};           ///< @brief 每行卡牌数量

    std::vector<CardButtonInfo> choose_card_buttons_; ///< @brief 选择面板中的卡牌按钮
    std::vector<FlyingCardInfo> flying_cards_;        ///< @brief 飞行中的卡牌
    std::vector<bool> slot_occupied_;                 ///< @brief 槽位是否已被占用
    std::vector<int> slot_to_card_index_;             ///< @brief 槽位索引 -> 选卡面板卡牌索引 的映射
    std::vector<engine::ui::UIButton *> placed_card_images_; ///< @brief 已放置在槽位中的卡牌图片指针
    std::vector<SlotMoveAnimation> slot_move_animations_;    ///< @brief 槽位卡牌上移动画列表

public:
    /**
     * @brief 构造函数
     * @param registry 注册表
     * @param ui_manager UI管理器
     * @param context 上下文
     */
    SelectCardUI(entt::registry &registry,
                 engine::ui::UIManager &ui_manager,
                 engine::core::Context &context);

    ~SelectCardUI();

    void update(float delta_time);

private:
    void createEmptySlots();         ///< @brief 创建左侧灰色空卡槽(从屏幕左侧外侧滑入)
    void updateChooseSceneAnimation(float delta_time); ///< @brief 更新空卡槽滑入动画
    void createChooseCardPanel(); ///< @brief 创建"Choose Your Plants"背景面板

    // 卡牌点选相关
    void onCardClicked(entt::id_type card_id, int card_index); ///< @brief 卡牌点击回调
    void onSlotClicked(int slot_index);                        ///< @brief 槽位点击回调
    void updateFlyingCards(float delta_time);                  ///< @brief 更新飞行中的卡牌动画
    void updateSlotMoveAnimations(float delta_time);           ///< @brief 更新槽位卡牌上移动画
};

} // namespace game::ui