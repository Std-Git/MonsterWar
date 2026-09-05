#include "plants_card_ui.h"
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
PlantsCardUI::PlantsCardUI(entt::registry &registry, 
    engine::ui::UIManager &ui_manager, 
    engine::core::Context &context)
    : registry_(registry), ui_manager_(ui_manager), context_(context)
{
    // 构造函数直接初始化(创建单位肖像UI),可省去init() 函数
    createUnitsCardUI();
    // -- 注册事件函数
    context_.getDispatcher().sink<game::defs::UnitPlacedEvent>().connect<&PlantsCardUI::onUnitPlacedEvent>(this);
    //context_.getDispatcher().sink<game::defs::RemoveUICardEvent>().connect<&UnitsCardUI::onRemoveUICardEvent>(this);
    spdlog::trace("PlantsCardUI 构造完成");
}

PlantsCardUI::~PlantsCardUI()
{
    //context_.getDispatcher().sink<game::defs::RemoveUICardEvent>().disconnect<&UnitsCardUI::onRemoveUICardEvent>(this);
}

void PlantsCardUI::update(float delta_time)
{
    // 1. 先更新冷却计时器(使 isOnCooldown 状态保持最新)
    updateCooldown(delta_time);
    // 2. 更新遮盖面板(依赖冷却状态)
    updateCardCover();
    updateCardLabelsColor();
    // 检测是否按下移动肖像面板的按键
    auto& input_manager = context_.getInputManager();
    if (input_manager.isActionDown("move_left"_hs))
    {
        moveCardPanelLeft(delta_time);
    } else if (input_manager.isActionDown("move_right"_hs))
    {
        moveCardPanelRight(delta_time);
    }
}

void PlantsCardUI::updateCardCover()
{
    // 获取 game_stats
    auto &game_stats = registry_.ctx().get<game::data::GameStats &>();
    // 获取anchor_panel 中的所有子元素 (frame_panel)
    auto &frame_panels = anchor_panel_->getChildren();
    for (auto& frame_panel : frame_panels)
    {
        // 获取 frame_panel 中的 cover_panel
        auto cover_panel = frame_panel->getChildById("cover_panel"_hs);
        // 设置cover_panel 的可见性 (frame_panel 的 order_index_已经设置为出击cost耗费值)
        if (cover_panel)
        {
            cover_panel->setVisible(game_stats.cost_ < frame_panel->getOrderIndex());
        }
    }
}

void PlantsCardUI::updateCardLabelsColor()
{
    auto &game_stats = registry_.ctx().get<game::data::GameStats &>();
    auto session_data = registry_.ctx().get<std::shared_ptr<game::data::SessionData>>();
    auto blueprint_manager = registry_.ctx().get<std::shared_ptr<game::factory::BlueprintManager>>();
    auto &unit_map = session_data->getUnitMap();

    for (const auto &[name_id, unit_data] : unit_map)
    {
        auto it = cost_label_map_.find(name_id);
        if (it == cost_label_map_.end())
            continue;
        int base_cost = blueprint_manager->getPlayerClassBlueprint(unit_data.class_id_).player_.cost_;
        int cost = static_cast<int>(std::round(engine::utils::statModify(static_cast<float>(base_cost), 1, unit_data.rarity_)));
        engine::utils::FColor color = game_stats.cost_ < cost ? engine::utils::FColor::red() : engine::utils::FColor::orange();
        it->second->setTextFColor(color);
    }
}

void PlantsCardUI::createUnitsCardUI()
{

    if (!ui_manager_.init(context_.getGameState().getLogicalSize())) return;     

    auto ui_config = registry_.ctx().get<std::shared_ptr<game::data::UIConfig>>();
    auto session_data = registry_.ctx().get<std::shared_ptr<game::data::SessionData>>();
    auto blueprint_manager = registry_.ctx().get<std::shared_ptr<game::factory::BlueprintManager>>();

    // 获取单位面板的间隔，角色map，角色数量
    auto padding = ui_config->getUnitPanelPadding();
    auto& unit_map = session_data->getUnitMap();
    auto unit_num = unit_map.size();

    //  -- 在屏幕下方创建一个panel UI 条，用于显示角色肖像 --
    // 获取窗口大小和角色肖像框大小
    auto window_size = context_.getGameState().getLogicalSize();
    auto frame_size = ui_config->getUnitPanelFrameSize();

    /*auto shovel_pos = glm::vec2(-1.2f, window_size.y - 33.76);
    auto shovel_button = std::make_unique<engine::ui::UIButton>(context_,
                                                                ui_config->getCard(entt::id_type("Shovel1")),
                                                                ui_config->getCard(entt::id_type("Shovel1")),
                                                                ui_config->getCard(entt::id_type("Shovel1")),
                                                                shovel_pos,
                                                                ui_config->getCard(entt::id_type("Shovel1")).getSourceRect());*/

    // 根据角色数量，角色肖像框大小，间隔计算panel的位置和大小
    auto pos = glm::vec2(0.0f, window_size.y - frame_size.y - 2 * padding);
    auto size = glm::vec2(unit_num * frame_size.x + (unit_num - 1) * padding, frame_size.y + 2 * padding);
    auto anchor_panel = std::make_unique<engine::ui::UIPanel>(pos, size);
    // 设置背景色
    anchor_panel->setBackgroundColor(engine::utils::FColor(0.1f, 0.1f, 0.1f, 0.1f));
    // 设置ID, 以后即可根据ID找到该panel
    anchor_panel->setId("anchor_panel"_hs);

    // 依次添加角色肖像，每个肖像由四部分依次叠加：Card, frame, icon, cost, 可以通过一个frame_panel 定位(位于上层anchor_panel之中)
    int index = 0;
    for (auto &[name_id, unit_data] : unit_map)
    {
        auto card = ui_config->getCard(name_id);
        //auto frame = ui_config->getCardFrame(unit_data.rarity_);
        //auto icon = ui_config->getIcon(unit_data.class_id_);
        auto cost = blueprint_manager->getPlayerClassBlueprint(unit_data.class_id_).player_.cost_;
        cost = static_cast<int>(std::round(engine::utils::statModify(static_cast<float>(cost), 1, unit_data.rarity_))); // 只有稀有度对 cost 有影响

        // 创建每个肖像的frame_panel
        auto frame_pos = glm::vec2(padding + index * (frame_size.x + padding), padding);
        auto frame_panel = std::make_unique<engine::ui::UIPanel>(frame_pos, frame_size);
        frame_panel->setId(name_id);

        // 依次添加四个元素，为了能够交互，将frame 设置为按钮，并绑定点击事件
        //frame_panel->addChild(std::make_unique<engine::ui::UIImage>(card, glm::vec2(0.0f, 0.0f), frame_size));
        frame_panel->addChild(std::make_unique<engine::ui::UIButton>(context_,
                                                                     card,
                                                                     card,
                                                                     card,
                                                                     glm::vec2(0.0f, 0.0f),
                                                                     frame_size,
                                                                     [this, name_id, &unit_data, cost]() {  // 按钮点击回调：发送单位准备事件
                                                                        if (!isOnCooldown(name_id))
                                                                            context_.getDispatcher().enqueue(game::defs::PrepUnitEvent{name_id, unit_data.class_id_, cost});
                                                                     },
                                                                     [this, name_id]() {    // 按钮悬停回调：发送单位肖像悬停进入事件
                                                                        context_.getDispatcher().enqueue(game::defs::UICardHoverEnterEvent{name_id});
                                                                     },
                                                                     [this]() {     // 按钮悬停离开回调：发送单位肖像悬停离开事件
                                                                        context_.getDispatcher().enqueue(game::defs::UICardHoverLeaveEvent{});
                                                                     }
                                                                     ));
        // ======================================================= //
        // 冷却条背景面板 (深色，覆盖整张卡牌，表示未冷却部分)
        auto cooldown_bg = std::make_unique<engine::ui::UIPanel>(glm::vec2(0.0f, 0.0f), frame_size);
        // 使用比阳光不足遮盖更深的灰色 (alpha 更大 = 更不透明 = 更深)
        cooldown_bg->setBackgroundColor(engine::utils::FColor(0.0f, 0.0f, 0.0f, 0.35f));
        cooldown_bg->setVisible(false);  // 默认隐藏，冷却时才显示
        auto cooldown_bg_ptr = cooldown_bg.get();
        frame_panel->addChild(std::move(cooldown_bg));
        cooldown_bg_panels_[name_id] = cooldown_bg_ptr;
        
        // 冷却条填充面板 (浅色，从底部向上增长，表示已冷却部分)
        auto cooldown_fill = std::make_unique<engine::ui::UIPanel>(glm::vec2(0.0f, 0.0f), glm::vec2(frame_size.x, 0.0f));
        // 使用与阳光不足遮盖相同的灰度 (0.0, 0.0, 0.0, 0.2)
        cooldown_fill->setBackgroundColor(engine::utils::FColor(0.0f, 0.0f, 0.0f, 0.2f));
        cooldown_fill->setVisible(false);  // 默认隐藏，冷却时才显示
        auto cooldown_fill_ptr = cooldown_fill.get();
        frame_panel->addChild(std::move(cooldown_fill));
        cooldown_fill_panels_[name_id] = cooldown_fill_ptr;
        
        std::string cost_str = std::to_string(cost);

        // 测量文字尺寸（调用 getTextSize）
        glm::vec2 text_size = context_.getTextRenderer().getTextSize(cost_str,
                                                                     entt::hashed_string(ui_config->getUnitPanelFontPath().c_str()),
                                                                     ui_config->getUnitPanelFontSize(),
                                                                     ui_config->getUnitPanelFontPath(),
                                                                     true); // is_dirty 为 true，确保使用最新文本

        // 计算偏移：右对齐，垂直方向使用原来的 y 偏移（或自行调整）
        float x_offset = frame_size.x - text_size.x;
        float y_offset = ui_config->getUnitPanelFontOffset().y;  // 保持原来的垂直偏移
        //frame_panel->addChild(std::make_unique<engine::ui::UIImage>(icon, glm::vec2(0.0f, 0.0f), frame_size / 2.0f));
        auto label = std::make_unique<engine::ui::UILabel>(context_.getTextRenderer(),
                                                                 std::to_string(cost),
                                                                 ui_config->getUnitPanelFontPath(),
                                                                 ui_config->getUnitPanelFontSize(),
                                                                 engine::utils::FColor::orange(),
                                                                 glm::vec2(x_offset, y_offset));
        auto label_ptr = label.get();
        frame_panel->addChild(std::move(label));
        cost_label_map_[name_id] = label_ptr;

        // 最后添加一个灰色的遮盖panel,cost 不足以支撑该角色出击时显示
        auto cover_panel = std::make_unique<engine::ui::UIPanel>(glm::vec2(0.0f, 0.0f), frame_size);
        cover_panel->setBackgroundColor(engine::utils::FColor(0.0f, 0.0f, 0.0f, 0.2f));
        cover_panel->setId("cover_panel"_hs);
        frame_panel->addChild(std::move(cover_panel));

        // 将frame_panel 添加到anchor_panel中，并使用cost 作为排序键
        anchor_panel->addChild(std::move(frame_panel), cost);
        index++;
    }
    // 将 anchor_panel 添加到ui_manager中
    ui_manager_.addElement(std::move(anchor_panel));

    // 移动赋值后需要找到anchor_panel,并将指针赋值给成员变量anchor_panel_
    anchor_panel_ = static_cast<engine::ui::UIPanel*>(ui_manager_.getRootElement()->getChildById("anchor_panel"_hs));

    anchor_panel_->sortChildrenByOrderIndex();      // 对anchor_panel 中的子元素(frame_panel)进行排序
    arrangeUnitsCardUI();                       // 按顺序排列anchor_panel中的子元素(frame_panel)的位置
}

void PlantsCardUI::arrangeUnitsCardUI()
{
    // 获取ui_config
    auto ui_config = registry_.ctx().get<std::shared_ptr<game::data::UIConfig>>();
    // 获取单位面板的间隔、大小
    auto padding = ui_config->getUnitPanelPadding();
    auto frame_size = ui_config->getUnitPanelFrameSize();

    // 遍历panel 中的子元素(定位panel)，并依次设定位置
    for (size_t i = 0; i < anchor_panel_->getChildren().size(); i++)
    {
        auto &child = anchor_panel_->getChildren()[i];
        child->setPosition(glm::vec2(padding + i * (frame_size.x + padding), padding));
    }
    // 更新panel 的size
    anchor_panel_->setSize(glm::vec2(padding + anchor_panel_->getChildren().size() * (frame_size.x + padding),
                                    frame_size.y + 2 * padding));
}

void PlantsCardUI::moveCardPanelRight(float delta_time)
{
    // 获取panel的位置
    auto panel_position = anchor_panel_->getPosition();
    // 如果位置为负就向右移，最多到达0
    panel_position.x = glm::min(0.0f, panel_position.x + delta_time * 400.0f);
    anchor_panel_->setPosition(panel_position);
}

void PlantsCardUI::moveCardPanelLeft(float delta_time)
{
    // 获取窗口大小
    const auto &window_size = context_.getGameState().getLogicalSize();
    // 获取panel的位置
    auto panel_position = anchor_panel_->getPosition();
    const auto& panel_size = anchor_panel_->getSize();
    // 如果面板宽度小于窗口宽度，则不移动
    if (panel_size.x < window_size.x) return;

    // 如果右端超出屏幕就向左移动，右端最多到达窗口宽度
    panel_position.x = glm::max(window_size.x - panel_size.x, panel_position.x - delta_time * 400.0f);
    anchor_panel_->setPosition(panel_position);
}

void PlantsCardUI::onRemoveUICardEvent(const game::defs::RemoveUICardEvent &event)
{
    anchor_panel_->removeChildById(event.name_id_);
    arrangeUnitsCardUI();
    cost_label_map_.erase(event.name_id_); // 移除对应条目
    // 同时清理冷却相关数据
    card_cooldown_remaining_.erase(event.name_id_);
    card_cooldown_max_.erase(event.name_id_);
    cooldown_fill_panels_.erase(event.name_id_);
    cooldown_bg_panels_.erase(event.name_id_);
}

void PlantsCardUI::onUnitPlacedEvent(const game::defs::UnitPlacedEvent &event)
{
    startCooldown(event.name_id_);
}

// ==================== 冷却系统实现 ====================

void PlantsCardUI::startCooldown(entt::id_type card_id)
{
    // 如果卡牌已在冷却中，不重复启动
    if (isOnCooldown(card_id)) {
        return;
    }
    
    // 从 UIConfig 获取配置的冷却时间
    auto ui_config = registry_.ctx().get<std::shared_ptr<game::data::UIConfig>>();
    float max_cooldown = ui_config->getCardCooldown(card_id);
    
    card_cooldown_remaining_[card_id] = max_cooldown;
    card_cooldown_max_[card_id] = max_cooldown;
    
    // 显示冷却条面板
    auto fill_it = cooldown_fill_panels_.find(card_id);
    if (fill_it != cooldown_fill_panels_.end()) {
        fill_it->second->setVisible(true);
    }
    auto bg_it = cooldown_bg_panels_.find(card_id);
    if (bg_it != cooldown_bg_panels_.end()) {
        bg_it->second->setVisible(true);
    }
    
    spdlog::debug("卡牌 [{}] 开始冷却，冷却时间: {:.1f} 秒", card_id, max_cooldown);
}

bool PlantsCardUI::isOnCooldown(entt::id_type card_id) const
{
    auto it = card_cooldown_remaining_.find(card_id);
    if (it != card_cooldown_remaining_.end()) {
        return it->second > 0.0f;
    }
    return false;
}

void PlantsCardUI::updateCooldown(float delta_time)
{
    auto ui_config = registry_.ctx().get<std::shared_ptr<game::data::UIConfig>>();
    auto frame_size = ui_config->getUnitPanelFrameSize();
    
    std::vector<entt::id_type> completed_cooldowns;
    
    for (auto& [id, remaining] : card_cooldown_remaining_) {
        if (remaining > 0.0f) {
            // 减少冷却时间
            remaining -= delta_time;
            if (remaining < 0.0f) {
                remaining = 0.0f;
            }
            
            // 更新冷却条填充面板的高度 (从底部向上增长)
            auto fill_it = cooldown_fill_panels_.find(id);
            if (fill_it != cooldown_fill_panels_.end()) {
                float ratio = (card_cooldown_max_[id] > 0.0f) ? (remaining / card_cooldown_max_[id]) : 0.0f;
                // 面板高度 = 比例 * 卡牌高度，宽度保持卡牌宽度
                fill_it->second->setSize(glm::vec2(frame_size.x, ratio * frame_size.y));
            }
            
            // 如果冷却完成，标记移除
            if (remaining == 0.0f) {
                completed_cooldowns.push_back(id);
                spdlog::debug("卡牌 [{}] 冷却完成", id);
            }
        }
    }
    
    // 移除冷却完成的卡牌
    for (auto id : completed_cooldowns) {
        card_cooldown_remaining_.erase(id);
        card_cooldown_max_.erase(id);
        
        // 隐藏冷却条面板
        auto fill_it = cooldown_fill_panels_.find(id);
        if (fill_it != cooldown_fill_panels_.end()) {
            fill_it->second->setVisible(false);
            // 重置高度为0
            auto ui_config2 = registry_.ctx().get<std::shared_ptr<game::data::UIConfig>>();
            auto frame_size2 = ui_config2->getUnitPanelFrameSize();
            fill_it->second->setSize(glm::vec2(frame_size2.x, 0.0f));
        }
        auto bg_it = cooldown_bg_panels_.find(id);
        if (bg_it != cooldown_bg_panels_.end()) {
            bg_it->second->setVisible(false);
        }
    }
}

}   // namespace game::ui
