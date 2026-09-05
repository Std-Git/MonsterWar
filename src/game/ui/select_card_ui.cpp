#include "select_card_ui.h"
#include "../data/ui_config.h"
#include "../data/session_data.h"
#include "../data/game_stats.h"
#include "../factory/blueprint_manager.h"
#include "../../engine/core/context.h"
#include "../../engine/core/game_state.h"
#include "../../engine/ui/ui_element.h"
#include "../../engine/ui/ui_panel.h"
#include "../../engine/ui/ui_image.h"
#include "../../engine/ui/ui_button.h"
#include "../../engine/ui/ui_label.h"
#include "../../engine/ui/ui_manager.h"
#include "../../engine/input/input_manager.h"
#include <entt/core/hashed_string.hpp>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::ui
{
    SelectCardUI::SelectCardUI(entt::registry &registry,
                               engine::ui::UIManager &ui_manager,
                               engine::core::Context &context)
        : registry_(registry), ui_manager_(ui_manager), context_(context),
          slot_occupied_(EMPTY_SLOT_COUNT, false),
          slot_to_card_index_(EMPTY_SLOT_COUNT, -1),
          placed_card_images_(EMPTY_SLOT_COUNT, nullptr)
    {
        // 构造函数直接初始化(创建单位肖像UI),可省去init() 函数
        // createUnitsCardUI();
        createEmptySlots();
        createChooseCardPanel();
        // -- 注册事件函数
        // context_.getDispatcher().sink<game::defs::RemoveUICardEvent>().connect<&UnitsCardUI::onRemoveUICardEvent>(this);
        spdlog::trace("UnitsCardUI 构造完成");
    }

    SelectCardUI::~SelectCardUI()
    {
        // context_.getDispatcher().sink<game::defs::RemoveUICardEvent>().disconnect<&UnitsCardUI::onRemoveUICardEvent>(this);
    }

    void SelectCardUI::update(float delta_time)
    {
        updateChooseSceneAnimation(delta_time);
        updateFlyingCards(delta_time);
        updateSlotMoveAnimations(delta_time);
        // 检测是否按下移动肖像面板的按键
        /*auto &input_manager = context_.getInputManager();
        if (input_manager.isActionDown("move_left"_hs))
        {
            moveCardPanelLeft(delta_time);
        }
        else if (input_manager.isActionDown("move_right"_hs))
        {
            moveCardPanelRight(delta_time);
        }*/
    }

    void SelectCardUI::createEmptySlots()
    {

        auto session_data = registry_.ctx().get<std::shared_ptr<game::data::SessionData>>();
        auto ui_config = registry_.ctx().get<std::shared_ptr<game::data::UIConfig>>();
        auto window_size = context_.getGameState().getLogicalSize();

        // 获取 "Empty Slot" 卡牌纹理信息
        auto empty_card = ui_config->getCard(entt::hashed_string("Empty Slot"_hs));
        auto frame_size = ui_config->getUnitPanelFrameSize();

        // 创建左侧已选择卡牌的面板
        auto empty_slots_panel = std::make_unique<engine::ui::UIPanel>(glm::vec2(-frame_size.x, 18.0f), glm::vec2(frame_size.x, frame_size.y * (EMPTY_SLOT_COUNT + 1)));
        empty_slots_panel->setId("empty_slots_panel"_hs);

        // 创建8个空卡槽，初始位置在屏幕左侧外侧(动画起点)
        for (int i = 0; i < EMPTY_SLOT_COUNT; ++i)
        {
            // 起始X：屏幕左侧外侧
            float start_x = 0.0f;
            // Y：从屏幕顶部向下排列
            float y = 18.0f + static_cast<float>(i) * (frame_size.y);
            glm::vec2 slot_pos(start_x, y);
            // 在slot_panel中添加Empty Slot卡牌图片
            auto empty_slot = std::make_unique<engine::ui::UIImage>(empty_card,
                                                                    slot_pos,
                                                                    frame_size);
            spdlog::warn("slot_size: {}, {}", empty_slot->getSourceRect().value().size.x, empty_slot->getSourceRect().value().size.y);
            empty_slots_panel->addChild(std::move(empty_slot));
        }
        auto reset_button_image = ui_config->getCard(entt::hashed_string("Reset Seeds"_hs));
        auto reset_button = std::make_unique<engine::ui::UIButton>(context_, reset_button_image, reset_button_image, reset_button_image, glm::vec2(0.0f, 18.0f + frame_size.y * EMPTY_SLOT_COUNT), frame_size,
                                                                   // 点击回调：重置所有槽位
                                                                   [this]()
                                                                   {
                                                                    spdlog::warn("clean");
                                                                       // 1. 移除所有飞行中的卡牌
                                                                       for (auto &flying : flying_cards_)
                                                                       {
                                                                           ui_manager_.getRootElement()->removeChild(flying.panel);
                                                                       }
                                                                       flying_cards_.clear();

                                                                       // 2. 移除所有深色遮罩并重置选中状态
                                                                       for (auto &card_info : choose_card_buttons_)
                                                                       {
                                                                           if (card_info.dark_overlay)
                                                                           {
                                                                               card_info.card_panel->removeChild(card_info.dark_overlay);
                                                                               card_info.dark_overlay = nullptr;
                                                                           }
                                                                           card_info.selected = false;
                                                                       }

                                                                       // 3. 移除所有已放置的卡牌图片
                                                                       for (auto &slot_img : placed_card_images_)
                                                                       {
                                                                           if (slot_img)
                                                                           {
                                                                               slot_img->getParent()->setNeedRemove(true);
                                                                           }
                                                                       }
                                                                       placed_card_images_.assign(EMPTY_SLOT_COUNT, nullptr);

                                                                       // 清除槽位上移动画
                                                                       slot_move_animations_.clear();
                                                                       // 4. 重置所有槽位占用状态
                                                                       for (int i = 0; i < EMPTY_SLOT_COUNT; ++i)
                                                                       {
                                                                           slot_occupied_[i] = false;
                                                                       } 
                                                                       // 5. 重置槽位->卡牌索引映射
                                                                       for (int i = 0; i < EMPTY_SLOT_COUNT; ++i)
                                                                       {
                                                                           slot_to_card_index_[i] = -1;
                                                                       }

                                                                       spdlog::warn("重置所有槽位和卡牌状态"); },
                                                                   nullptr,  // 悬停进入
                                                                   nullptr); // 悬停离开);
        empty_slots_panel->addChild(std::move(reset_button));

        // 将父面板添加到UI管理器
        ui_manager_.addElement(std::move(empty_slots_panel));

        // 触发动画(下一帧开始滑入)
        slots_anim_triggered_ = true;
        slots_anim_elapsed_ = 0.0f;

        empty_slots_panel_ = static_cast<engine::ui::UIPanel *>(ui_manager_.getRootElement()->getChildById("empty_slots_panel"_hs));

        spdlog::warn("空卡槽创建完成, 数量={}, 尺寸={:.1f}x{:.1f}",
                     EMPTY_SLOT_COUNT, frame_size.x, frame_size.y);
    }

    void SelectCardUI::updateChooseSceneAnimation(float delta_time)
    {
        // 动画未完成且已触发时才更新
        if (slots_anim_completed_ || !slots_anim_triggered_)
            return;

        slots_anim_elapsed_ += delta_time;

        auto window_size = context_.getGameState().getLogicalSize();
        auto ui_config = registry_.ctx().get<std::shared_ptr<game::data::UIConfig>>();
        auto session_data = registry_.ctx().get<std::shared_ptr<game::data::SessionData>>();
        // auto &unit_map = session_data->getUnitMap();
        auto empty_card = ui_config->getCard(entt::hashed_string("Empty Slot"_hs));
        auto card_rect = empty_card.getSourceRect();
        auto choose_image_size = ui_config->getCardDisplaySize(entt::hashed_string("Choose Your Plants"_hs));
        auto frame_size = ui_config->getUnitPanelFrameSize();

        // float padding_y = 10.0f;
        // float spacing = 0.0f;

        // 起始X：屏幕左侧外侧
        float slots_default_y = empty_slots_panel_->getPosition().y;
        glm::vec2 slots_start_pos = glm::vec2(-frame_size.x, slots_default_y);
        glm::vec2 slots_end_pos = glm::vec2(0.0f, slots_default_y);

        empty_slots_panel_->setPosition(engine::utils::easeOutCubic(slots_start_pos, slots_end_pos, slots_anim_elapsed_, SLOTS_ANIM_DURATION));

        float choose_default_x = choose_card_panel_->getPosition().x;
        glm::vec2 choose_start_pos = glm::vec2(choose_default_x, window_size.y);
        glm::vec2 choose_end_pos = glm::vec2(choose_default_x, window_size.y - choose_image_size.y);
        choose_card_panel_->setPosition(engine::utils::easeOutCubic(choose_start_pos, choose_end_pos, slots_anim_elapsed_, SLOTS_ANIM_DURATION));

        // 动画完成
        if (std::clamp(slots_anim_elapsed_ / SLOTS_ANIM_DURATION, 0.0f, 1.0f) >= 1.0f)
        {
            slots_anim_completed_ = true;
            spdlog::warn("空卡槽滑入动画完成");
        }
    }

    void SelectCardUI::createChooseCardPanel()
    {
        auto ui_config = registry_.ctx().get<std::shared_ptr<game::data::UIConfig>>();
        auto window_size = context_.getGameState().getLogicalSize();

        // 获取 "Choose Your Plants" 卡牌纹理和显示尺寸（统一使用 ui_config 的 getCardDisplaySize）
        auto choose_card_id = entt::hashed_string("Choose Your Plants"_hs);
        auto choose_card = ui_config->getCard(choose_card_id);
        auto panel_size = ui_config->getCardDisplaySize(choose_card_id);
        auto frame_size = ui_config->getUnitPanelFrameSize();
        auto bg = ui_config->getCard(entt::hashed_string("underlay"_hs));

        // 水平居中放置在屏幕顶部
        auto panel = std::make_unique<engine::ui::UIPanel>(
            glm::vec2(frame_size.x + 15.0f, window_size.y), panel_size);
        panel->setId("choose_card_panel"_hs);
        panel->addChild(std::make_unique<engine::ui::UIImage>(bg, glm::vec2(15.0f, 153.0f), ui_config->getCardDisplaySize(entt::hashed_string("underlay"_hs))));
        panel->addChild(std::make_unique<engine::ui::UIImage>(choose_card, glm::vec2(0.0f, 0.0f), panel_size));

        // -- 创建可点击的卡牌按钮 --
        auto &card_list = ui_config->getCardList();
        spdlog::warn("卡牌列表数量: {}", card_list.size());

        // 卡牌布局参数
        float card_padding_x = 24.0f;
        float card_padding_y = 156.0f;
        float card_spacing_x = 8.0f;
        float card_spacing_y = 4.0f;

        for (int i = 0; i < static_cast<int>(card_list.size()); ++i)
        {
            int row = i / CARDS_PER_ROW;
            int col = i % CARDS_PER_ROW;

            float card_x = card_padding_x + static_cast<float>(col) * (frame_size.x + card_spacing_x);
            float card_y = card_padding_y + static_cast<float>(row) * (frame_size.y + card_spacing_y);
            glm::vec2 card_pos(card_x, card_y);

            auto card_id = card_list[i];
            auto card_img = ui_config->getCard(card_id);

            // 获取cost
            auto session_data = registry_.ctx().get<std::shared_ptr<game::data::SessionData>>();
            auto blueprint_manager = registry_.ctx().get<std::shared_ptr<game::factory::BlueprintManager>>();
            auto &unit_map = session_data->getUnitMap();
            int card_cost = 0;
            auto unit_it = unit_map.find(card_id);
            if (unit_it != unit_map.end())
            {
                auto &unit_data = unit_it->second;
                card_cost = blueprint_manager->getPlayerClassBlueprint(unit_data.class_id_).player_.cost_;
                card_cost = static_cast<int>(std::round(engine::utils::statModify(static_cast<float>(card_cost), 1, unit_data.rarity_)));
            }
            // 创建卡牌面板，button和cost标签都作为其子元素
            auto card_panel = std::make_unique<engine::ui::UIPanel>(card_pos, frame_size);
            auto button = std::make_unique<engine::ui::UIButton>(
                context_,
                card_img,
                card_img,
                card_img,
                glm::vec2(0.0f, 0.0f),
                frame_size,
                [this, card_id, i]()
                {
                    onCardClicked(card_id, i);
                    spdlog::warn("Clicked");
                },
                nullptr,
                nullptr);
            engine::ui::UIButton *button_ptr = button.get();
            spdlog::warn("Button Created");
            card_panel->addChild(std::move(button));
            // 创建cost标签并添加到卡牌面板上（右上角）
            std::string cost_str = std::to_string(card_cost);
            glm::vec2 text_size = context_.getTextRenderer().getTextSize(cost_str,
                                                                         entt::hashed_string(ui_config->getUnitPanelFontPath().c_str()),
                                                                         ui_config->getUnitPanelFontSize(),
                                                                         ui_config->getUnitPanelFontPath(),
                                                                         true);
            float x_offset = frame_size.x - text_size.x;
            float y_offset = ui_config->getUnitPanelFontOffset().y;
            auto cost_label = std::make_unique<engine::ui::UILabel>(context_.getTextRenderer(),
                                                                    cost_str,
                                                                    ui_config->getUnitPanelFontPath(),
                                                                    ui_config->getUnitPanelFontSize(),
                                                                    engine::utils::FColor::orange(),
                                                                    glm::vec2(x_offset, y_offset));
            // 记录按钮指针
            CardButtonInfo info;
            info.button = button_ptr;
            info.card_panel = card_panel.get(); // 保存卡牌面板指针，用于遮罩定位
            info.card_id = card_id;
            info.selected = false;
            info.card_cost = card_cost; // 保存cost值，供飞行/槽位卡片使用
            choose_card_buttons_.push_back(info);
            card_panel->addChild(std::move(cost_label));
            panel->addChild(std::move(card_panel));
            spdlog::warn("卡牌按钮创建完成: {}, {}", card_id, i);
        }

        ui_manager_.addElement(std::move(panel));
        choose_card_panel_ = static_cast<engine::ui::UIPanel *>(ui_manager_.getRootElement()->getChildById("choose_card_panel"_hs));

        auto rock_button = std::make_unique<engine::ui::UIButton>(
            context_,
            ui_config->getButtonImage(entt::hashed_string("Let's Rock"), game::data::ButtonState::Normal),
            ui_config->getButtonImage(entt::hashed_string("Let's Rock"), game::data::ButtonState::Hover),
            ui_config->getButtonImage(entt::hashed_string("Let's Rock"), game::data::ButtonState::Pressed),
            glm::vec2(window_size - ui_config->getCardDisplaySize(entt::hashed_string("Let's Rock"))),
            ui_config->getCardDisplaySize(entt::hashed_string("Let's Rock")),
            [](){
                spdlog::warn("Let's Rock");
            });
        ui_manager_.addElement(std::move(rock_button));
        spdlog::warn("选择植物面板创建完成, 尺寸={:.1f}x{:.1f}", panel_size.x, panel_size.y);
    }

    void SelectCardUI::onCardClicked(entt::id_type card_id, int card_index)
    {
        auto ui_config = registry_.ctx().get<std::shared_ptr<game::data::UIConfig>>();
        auto frame_size = ui_config->getUnitPanelFrameSize();
        spdlog::trace("卡牌点击: {}, {}", card_id, card_index);

        // 查找对应的按钮信息
        if (card_index < 0 || card_index >= static_cast<int>(choose_card_buttons_.size()))
        {
            spdlog::error("卡牌索引越界: {}", card_index);
            return;
        }

        auto &card_info = choose_card_buttons_[card_index];

        // 如果已经选中，不做任何操作
        if (card_info.selected)
        {
            spdlog::trace("卡牌已选中，忽略重复点击: {}", card_id);
            return;
        }

        // 2. 查找第一个可用槽位
        int target_slot = -1;
        for (int i = 0; i < EMPTY_SLOT_COUNT; ++i)
        {
            // 检查该槽位是否已被飞行中的卡牌占用
            bool is_flying_to = false;
            for (const auto &flying : flying_cards_)
            {
                if (flying.slot_index == i)
                {
                    is_flying_to = true;
                    break;
                }
            }
            if (!is_flying_to && !slot_occupied_[i])
            {
                target_slot = i;
                break;
            }
        }
        // 若无可用槽位，直接返回，不添加遮罩
        if (target_slot == -1)
        {
            spdlog::warn("没有可用的空槽位，卡牌无法放置");
            return;
        }

        // 2.  确认有可用槽位后,在卡牌原位添加深色遮罩
        if (card_info.dark_overlay == nullptr && card_info.button != nullptr)
        {
            auto overlay = std::make_unique<engine::ui::UIPanel>(
                glm::vec2(0.0f, 0.0f), frame_size);
            overlay->setBackgroundColor(engine::utils::FColor(0.0f, 0.0f, 0.0f, 0.45f));
            // 将遮罩添加到choose_card_panel中，位置与按钮一致
            card_info.dark_overlay = overlay.get();
            card_info.card_panel->addChild(std::move(overlay));
        }

        // 3. 创建飞行动画卡牌
        auto flying_panel = std::make_unique<engine::ui::UIPanel>(glm::vec2(0.0f, 0.0f), frame_size);
        auto card_img = ui_config->getCard(card_id);
        flying_panel->addChild(std::make_unique<engine::ui::UIImage>(card_img, glm::vec2(0.0f, 0.0f), frame_size));

        // 计算起始位置（卡牌在choose_card_panel中的世界坐标）
        glm::vec2 button_pos = card_info.button->getPosition();
        glm::vec2 start_pos = choose_card_panel_->getPosition() + card_info.card_panel->getPosition() + card_info.button->getPosition();

        // 计算目标位置（槽位的世界坐标）
        float slot_y = 18.0f + static_cast<float>(target_slot) * frame_size.y;
        glm::vec2 end_pos = empty_slots_panel_->getPosition() + glm::vec2(0.0f, slot_y);

        flying_panel->setPosition(start_pos); // 给飞行卡牌添加cost标签（右上角）
        std::string cost_str = std::to_string(card_info.card_cost);
        glm::vec2 text_size = context_.getTextRenderer().getTextSize(cost_str,
                                                                     entt::hashed_string(ui_config->getUnitPanelFontPath().c_str()),
                                                                     ui_config->getUnitPanelFontSize(),
                                                                     ui_config->getUnitPanelFontPath(),
                                                                     true);
        float x_offset = frame_size.x - text_size.x;
        float y_offset = ui_config->getUnitPanelFontOffset().y;
        auto cost_label = std::make_unique<engine::ui::UILabel>(
            context_.getTextRenderer(),
            cost_str,
            ui_config->getUnitPanelFontPath(),
            ui_config->getUnitPanelFontSize(),
            engine::utils::FColor::orange(),
            glm::vec2(x_offset, y_offset));
        flying_panel->addChild(std::move(cost_label));
        // 将飞行卡牌添加到根元素（与choose_card_panel同级）
        ui_manager_.getRootElement()->addChild(std::move(flying_panel));
        // move 之后通过根节点获取安全的裸指针，避免悬垂
        engine::ui::UIPanel *flying_panel_ptr = static_cast<engine::ui::UIPanel *>(ui_manager_.getRootElement()->getChildren().back().get());
        // 记录飞行卡牌信息
        FlyingCardInfo flying_info;
        flying_info.panel = flying_panel_ptr;
        flying_info.start_pos = start_pos;
        flying_info.end_pos = end_pos;
        flying_info.elapsed = 0.0f;
        flying_info.slot_index = target_slot;
        flying_info.card_index = card_index;
        flying_cards_.push_back(flying_info);

        // 确认有可用槽位后才设置选中状态
        card_info.selected = true;

        spdlog::trace("卡牌 {} 点击选中, 飞向槽位 {}", card_id, target_slot);
    }

    void SelectCardUI::onSlotClicked(int slot_index)
    {
        if (slot_index < 0 || slot_index >= EMPTY_SLOT_COUNT || !slot_occupied_[slot_index])
        {
            return;
        }
        // 1. 通过 slot_to_card_index_ 映射查找对应的选卡面板卡牌
        int card_idx = slot_to_card_index_[slot_index];
        if (card_idx >= 0 && card_idx < static_cast<int>(choose_card_buttons_.size()))
        {
            auto &card_info = choose_card_buttons_[card_idx];
            // 移除原位深色遮罩
            if (card_info.dark_overlay)
            {
                card_info.card_panel->removeChild(card_info.dark_overlay);
                card_info.dark_overlay = nullptr;
            }
            // 重置选中状态，使卡牌可再次被选择
            card_info.selected = false;
        }
        // 2. 获取UI配置
        auto ui_config = registry_.ctx().get<std::shared_ptr<game::data::UIConfig>>();
        auto frame_size = ui_config->getUnitPanelFrameSize();
        // 3. 隐藏被删除的卡牌按钮（不删除，避免悬垂指针）
        if (slot_index < static_cast<int>(placed_card_images_.size()))
        {
            auto *button = placed_card_images_[slot_index];
            if (button)
            {
                button->getParent()->setNeedRemove(true);
                //button->setVisible(false);
                //button->setPosition(glm::vec2(-9999.0f, -9999.0f));
            }
        }
        // 4. 移除被移除槽位及下方卡牌的已有动画
        slot_move_animations_.erase(
            std::remove_if(slot_move_animations_.begin(), slot_move_animations_.end(),
                           [slot_index, this](const SlotMoveAnimation &anim)
                           {
                               for (int i = slot_index; i < EMPTY_SLOT_COUNT; ++i)
                               {
                                   if (placed_card_images_[i] == anim.button)
                                   {
                                       return true;
                                   }
                               }
                               return false;
                           }),
            slot_move_animations_.end());
        // 5. 收集需要上移的卡牌 (从 slot_index + 1 到最后)
        std::vector<std::pair<int, int>> cards_to_move; // (source_slot, card_index)
        for (int i = slot_index + 1; i < EMPTY_SLOT_COUNT; ++i)
        {
            if (slot_occupied_[i])
            {
                cards_to_move.push_back({i, slot_to_card_index_[i]});
            }
        }
        // 6. 移动卡牌并更新数据
        int current_slot = slot_index;
        for (auto &[src_slot, card_idx] : cards_to_move)
        {
            // 更新映射
            slot_to_card_index_[current_slot] = card_idx;
            slot_occupied_[current_slot] = true;
            // 获取卡牌按钮
            auto *button = placed_card_images_[src_slot];
            if (button)
            {
                auto panel = button->getParent();
                // 记录起始位置（当前按钮位置）
                glm::vec2 start_pos = panel->getPosition();
                // 目标位置（上一个槽位）
                glm::vec2 end_pos = glm::vec2(0.0f, 18.0f + static_cast<float>(current_slot) * frame_size.y);
                // 设置按钮到起始位置并显示
                panel->setPosition(start_pos);
                // 启动上移动画
                SlotMoveAnimation anim;
                anim.button = button;
                anim.start_pos = start_pos;
                anim.end_pos = end_pos;
                anim.elapsed = 0.0f;
                slot_move_animations_.push_back(anim);
            }
            // 更新 placed_card_images_
            placed_card_images_[current_slot] = button;
            placed_card_images_[src_slot] = nullptr;
            current_slot++;
        }
        // 7. 清除多余的槽位
        for (int i = current_slot; i < EMPTY_SLOT_COUNT; ++i)
        {
            slot_occupied_[i] = false;
            slot_to_card_index_[i] = -1;
            if (i < static_cast<int>(placed_card_images_.size()))
            {
                placed_card_images_[i] = nullptr;
            }
        }
        spdlog::trace("槽位 {} 卡牌已取消选择，下方卡牌上移", slot_index);
    }

    void SelectCardUI::updateFlyingCards(float delta_time)
    {
        auto ui_config = registry_.ctx().get<std::shared_ptr<game::data::UIConfig>>();
        auto frame_size = ui_config->getUnitPanelFrameSize();
        // 从后往前遍历，方便安全删除
        for (auto it = flying_cards_.begin(); it != flying_cards_.end();)
        {
            auto &flying = *it;
            flying.elapsed += delta_time;
            float t = std::clamp(flying.elapsed / CARD_ANIM_DURATION, 0.0f, 1.0f);
            glm::vec2 current_pos = engine::utils::easeOutCubic(
                flying.start_pos, flying.end_pos, flying.elapsed, CARD_ANIM_DURATION);
            flying.panel->setPosition(current_pos);
            if (t >= 1.0f)
            {
                // === 动画完成，执行放置逻辑 ===
                // 1. 移除飞行卡牌面板
                ui_manager_.getRootElement()->removeChild(flying.panel);
                // 2. 在槽位中放置卡牌图片
                const auto &children = empty_slots_panel_->getChildren();
                if (flying.slot_index < static_cast<int>(children.size()))
                {
                    auto &slot_element = *children[flying.slot_index];
                    glm::vec2 slot_pos = slot_element.getPosition();
                    // 通过 card_index 查找卡牌ID
                    if (flying.card_index >= 0 && flying.card_index < static_cast<int>(choose_card_buttons_.size()))
                    {
                        auto card_id = choose_card_buttons_[flying.card_index].card_id;
                        auto card_img = ui_config->getCard(card_id);
                        auto card_panel = std::make_unique<engine::ui::UIPanel>(slot_pos, frame_size);
                        // 1. 先创建 UIButton unique_ptr
                        auto card_slot = std::make_unique<engine::ui::UIButton>(
                            context_, card_img, card_img, card_img, glm::vec2(0.0f, 0.0f), frame_size,
                            nullptr, nullptr, nullptr);

                        // 3. 获取裸指针，此时 card_slot 已声明
                        engine::ui::UIButton *captured_button_ptr = card_slot.get();

                        // 4. 重新绑定点击回调，安全捕获裸指针
                        card_slot->setClickCallback(
                            [this, captured_button_ptr]()
                            {
                                // 通过按钮指针在 placed_card_images_ 中查找当前槽位索引
                                for (int i = 0; i < EMPTY_SLOT_COUNT; ++i)
                                {
                                    if (i < static_cast<int>(placed_card_images_.size()) && placed_card_images_[i] == captured_button_ptr)
                                    {
                                        onSlotClicked(i); // ← 动态查找当前实际所在的槽位
                                        return;
                                    }
                                }
                            });

                        // 必须在 move 转移所有权前获取裸指针，否则会导致悬垂指针
                        placed_card_images_[flying.slot_index] = captured_button_ptr;
                        card_panel->addChild(std::move(card_slot));
                        // 给槽位中的卡牌添加cost标签（右上角）
                        auto &card_info = choose_card_buttons_[flying.card_index];
                        std::string cost_str = std::to_string(card_info.card_cost);
                        glm::vec2 text_size = context_.getTextRenderer().getTextSize(cost_str,
                                                                                     entt::hashed_string(ui_config->getUnitPanelFontPath().c_str()),
                                                                                     ui_config->getUnitPanelFontSize(),
                                                                                     ui_config->getUnitPanelFontPath(),
                                                                                     true);
                        float x_offset = frame_size.x - text_size.x;
                        float y_offset = ui_config->getUnitPanelFontOffset().y;
                        // cost标签添加到empty_slots_panel_，位置 = slot_pos + 偏移
                        auto cost_label = std::make_unique<engine::ui::UILabel>(
                            context_.getTextRenderer(),
                            cost_str,
                            ui_config->getUnitPanelFontPath(),
                            ui_config->getUnitPanelFontSize(),
                            engine::utils::FColor::orange(),
                            glm::vec2(x_offset, y_offset));
                        card_panel->addChild(std::move(cost_label));
                        empty_slots_panel_->addChild(std::move(card_panel));
                    }
                }
                // 3. 保留 selected 状态和 dark_overlay（不重置、不删除），使卡牌不可再次被选择
                // 记录 slot_index -> card_index 的映射，供 onSlotClicked 使用
                if (flying.card_index >= 0 && flying.card_index < static_cast<int>(choose_card_buttons_.size()))
                {
                    slot_to_card_index_[flying.slot_index] = flying.card_index;
                }
                // 4. 标记槽位为已占用
                slot_occupied_[flying.slot_index] = true;
                // 5. 从飞行卡牌列表中移除
                it = flying_cards_.erase(it);
                spdlog::trace("卡牌已放置到槽位 {}, 槽位已占用, 遮罩保留", flying.slot_index);
            }
            else
            {
                ++it;
            }
        }
    }

    void SelectCardUI::updateSlotMoveAnimations(float delta_time)
    {
        for (auto it = slot_move_animations_.begin(); it != slot_move_animations_.end();)
        {
            auto &anim = *it;
            anim.elapsed += delta_time;
            float t = std::clamp(anim.elapsed / SLOT_MOVE_ANIM_DURATION, 0.0f, 1.0f);
            anim.button->getParent()->setPosition(engine::utils::easeOutCubic(
                anim.start_pos, anim.end_pos, anim.elapsed, SLOT_MOVE_ANIM_DURATION));
            if (t >= 1.0f)
            {
                it = slot_move_animations_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
} // namespace game::ui
