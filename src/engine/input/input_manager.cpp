#include "input_manager.h"
#include "../core/config.h"
#include "../utils/events.h"
#include <stdexcept>
#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include <glm/vec2.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>

namespace engine::input
{

    InputManager::InputManager(SDL_Renderer *sdl_renderer, const engine::core::Config *config, entt::dispatcher *dispatcher)
        : sdl_renderer_(sdl_renderer), dispatcher_(dispatcher)
    {
        if (!sdl_renderer_)
        {
            spdlog::error("输入管理器: SDL_Renderer 为空指针");
            throw std::runtime_error("输入管理器: SDL_Renderer 为空指针");
        }
        initializeMappings(config);
        // 获取初始鼠标位置
        float x, y;
        SDL_GetMouseState(&x, &y);
        mouse_position_ = {x, y};
        spdlog::trace("初始鼠标位置: ({}, {})", mouse_position_.x, mouse_position_.y);
    }

    entt::sink<entt::sigh<bool()>> InputManager::onAction(entt::id_type action_name_id, ActionState action_state)
    {
        // 如果 action_name 不存在，自动创建一个 std::array<...>
        // .at()会进行边缘检查，更安全
        return actions_to_func_[action_name_id].at(static_cast<size_t>(action_state));
    }

    //  --- 更新和事件处理 ---

    void InputManager::update()
    {
        // 1.根据上一帧的值更新默认的动作状态
        for (auto &[action_name_id, state] : action_states_)
        {
            if (state == ActionState::PRESSED)
            {
                state = ActionState::HELD; // 当某一个键按下不动时，并不会生成 SDL_Event
            }
            else if (state == ActionState::RELEASED)
            {
                state = ActionState::INACTIVE;
            }
        }

        // 2.处理所有待处理的 SDL 事件 (浙江设定 action_states_ 的值)
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event); // ImGui 步骤2 处理 ImGui 事件
            processEvent(event);
        }

        // 3.触发回调
        for (auto &[action_name_id, state] : action_states_)
        {
            if (state != ActionState::INACTIVE) // 如果动作状态不是 INACTIVE
            {
                // 且有绑定回调函数
                if (auto it = actions_to_func_.find(action_name_id); it != actions_to_func_.end())
                {
                    // it->second.at(static_cast<size_t>(state)).publish(); // 触发回调函数 (之前的做法)
                    //  collect 方法可以获取回调函数返回值，放入 lambda 函数的参数中
                    //  而 lambda 函数的返回值为真时停止分发信号
                    //  分发信号的顺序为 “后绑定先调用”
                    it->second.at(static_cast<size_t>(state)).collect([](bool result)
                                                                      { return result; });
                }
            }
        }
    }

    void InputManager::quit()
    {
        dispatcher_->trigger<engine::utils::QuitEvent>();
    }

    void InputManager::processEvent(const SDL_Event &event)
    {
        // 如果 ImGui 捕获了鼠标，则不处理该事件(避免穿透到游戏中)
        if (ImGui::GetIO().WantCaptureMouse)
        {
            return;
        }

        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            SDL_Scancode scancode = event.key.scancode; // 获取按键的 scancode
            bool is_down = event.key.down;
            bool is_repeat = event.key.repeat;

            auto it = input_to_actions_.find(scancode);
            if (it != input_to_actions_.end()) // 如果有按键对应的 action
            {
                const std::vector<entt::id_type> &associated_actions = it->second;
                for (const auto &action_name : associated_actions)
                {
                    updateActionState(action_name, is_down, is_repeat); // 更新 action 状态
                }
            }
            // <bug> bug7：不应更新鼠标位置
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            Uint32 button = event.button.button; // 获取鼠标按钮
            bool is_down = event.button.down;
            auto it = input_to_actions_.find(button);
            if (it != input_to_actions_.end()) // 如果有鼠标按钮对应的 action
            {
                const std::vector<entt::id_type> &associated_actions = it->second;
                for (const auto &action_name : associated_actions)
                {
                    // 鼠标事件不考虑 repeat,所以第三个参数传false
                    updateActionState(action_name, is_down, false); // 更新 action 状态
                }
            }
            // 在点击时更新鼠标位置，同时更新逻辑位置
            mouse_position_ = {event.button.x, event.button.y};
            SDL_RenderCoordinatesFromWindow(sdl_renderer_, mouse_position_.x, mouse_position_.y, &logical_mouse_position_.x, &logical_mouse_position_.y);
            break;
        }
        case SDL_EVENT_MOUSE_MOTION: // 处理鼠标运动
            mouse_position_ = {event.button.x, event.button.y};
            SDL_RenderCoordinatesFromWindow(sdl_renderer_, mouse_position_.x, mouse_position_.y, &logical_mouse_position_.x, &logical_mouse_position_.y);
            break;
        case SDL_EVENT_WINDOW_MOVED:
        case SDL_EVENT_WINDOW_RESIZED:
            // 拖拽窗口
            dispatcher_->enqueue<engine::utils::WindowMovedEvent>();
            break;
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_RESTORED:
            // 窗口显示
            dispatcher_->enqueue<engine::utils::WindowExposedEvent>();
            break;
        case SDL_EVENT_QUIT:
            quit();
            break;
        default:
            break;
        }
    }

    // ---- 状态查询方法 ----

    bool InputManager::isActionDown(entt::id_type action_name_id) const
    {
        // C++17 引入的“带有初始化语句的 if 语句”
        if (auto it = action_states_.find(action_name_id); it != action_states_.end())
        {
            return it->second == ActionState::PRESSED || it->second == ActionState::HELD;
        }
        return false;
    }

    bool InputManager::isActionPressed(entt::id_type action_name_id) const
    {
        if (auto it = action_states_.find(action_name_id); it != action_states_.end())
        {
            return it->second == ActionState::PRESSED;
        }
        return false;
    }

    bool InputManager::isActionReleased(entt::id_type action_name_id) const
    {
        if (auto it = action_states_.find(action_name_id); it != action_states_.end())
        {
            return it->second == ActionState::RELEASED;
        }
        return false;
    }

    glm::vec2 InputManager::getMousePosition() const
    {
        return mouse_position_;
    }

    glm::vec2 InputManager::getLogicalMousePosition() const
    {
        // 每帧最多计算一次，避免每次调用时在计算
        return logical_mouse_position_;
    }

    ImGuiKeyChord InputManager::getShortcutForAction(entt::id_type action_name_id) const
    {
        auto it = action_shortcuts_.find(action_name_id);
        if (it != action_shortcuts_.end())
        {
            return it->second;
        }
        return ImGuiKey_None;
    }

    std::string InputManager::getActionKeyName(entt::id_type action_name_id) const
    {
        auto it = action_key_names_.find(action_name_id);
        if (it != action_key_names_.end())
        {
            return it->second;
        }
        return "None";
    }

    // --- 初始化输入映射 ---

    void InputManager::initializeMappings(const engine::core::Config *config)
    {
        spdlog::trace("初始化输入映射...");
        if (!config)
        {
            spdlog::error("输入管理器: Config 为空指针");
            throw std::runtime_error("输入管理器: Config 为空指针");
        }
        auto actions_to_keyname = config->input_mappings_; // 获取配置中的输入映射 (动作 -> 按键名称)
        input_to_actions_.clear();
        action_states_.clear();

        // 如果配置中没有定义鼠标按钮动作(通常不需要配置)，则添加默认映射，用于UI
        if (actions_to_keyname.find("mouse_left") == actions_to_keyname.end())
        {
            spdlog::debug("配置中没有定义 'mouse_left' 动作，添加默认映射到 ' MouseLeft'");
            actions_to_keyname["mouse_left"] = {"MouseLeft"}; // 如果缺失则添加默认映射
        }
        if (actions_to_keyname.find("mouse_right") == actions_to_keyname.end())
        {
            spdlog::debug("配置中没有定义 'mouse_right' 动作，添加默认映射到 ' MouseRight'");
            actions_to_keyname["mouse_right"] = {"MouseRight"};
        }
        // 遍历 动作 -> 按键名称 的映射
        for (const auto &[action_name, key_names] : actions_to_keyname)
        {
            // 每个动作对应一个动作状态，初始化为 INACTIVE
            auto action_name_id = entt::hashed_string(action_name.c_str()); // <bug> bug4: entt::hashed_string 打作 entt::id_type
            action_states_[action_name_id] = ActionState::INACTIVE;
            spdlog::trace("映射动作: {}", action_name);
            // 设置"按键 -> 动作" 的映射
            for (const auto &key_name : key_names)
            {
                SDL_Scancode scancode = scancodeFromString(key_name);  // 尝试根据按键名称获取 scancode
                Uint32 mouse_button = mouseButtonFromString(key_name); // 尝试根据按键名称获取鼠标按钮
                // 未来可添加其他输入类型

                if (scancode != SDL_SCANCODE_UNKNOWN) // 如果 scancode 有效，则将action添加到 scancode_to_actions_map_
                {
                    input_to_actions_[scancode].push_back(action_name_id);
                    spdlog::trace("映射按键：{} (Scancode: {}) 到动作：{}", key_name, static_cast<int>(scancode), action_name);
                }
                else if (mouse_button != 0) // 如果 mouse_button 有效，则将action添加到 mouse_button_to_actions_map_
                {
                    input_to_actions_[mouse_button].push_back(action_name_id);
                    spdlog::trace("映射鼠标按钮：{} (Mouse Button: {}) 到动作：{}", key_name, static_cast<int>(mouse_button), action_name);
                }
                else
                {
                    spdlog::warn("输入映射警告：未知键或按钮名称 '{}' 用于动作 '{}'", key_name, action_name);
                }
            }
        } 
        
        auto actions_to_keychord = config->imgui_input_mappings_; // 获取配置中的输入映射 (动作 -> 按键组合)
        for (const auto &[action_name, key_chords] : actions_to_keychord)
        {
            auto imgui_action_name_id = entt::hashed_string(action_name.c_str());
            ImGuiKeyChord key = ImGuiKey_None;
            std::string key_name_str = "";
            // 设置"按键 -> 动作" 的映射
            for (const auto &key_chord : key_chords)
            {
                key = parseKeyChordFromString(key_chord);
                if (key != ImGuiKey_None)
                {
                    key_name_str = key_chord;
                    break;
                }
            }
            if (key != ImGuiKey_None)
            {
                action_shortcuts_[imgui_action_name_id] = ImGuiKeyChord(key);
                action_key_names_[imgui_action_name_id] = key_name_str;
            }
        }
        spdlog::trace("输入映射初始化完成");
    }

    // --- 工具函数 ---
    // 将字符串名称转换为 SDL_Scancode
    SDL_Scancode InputManager::scancodeFromString(std::string_view key_name)
    {
        return SDL_GetScancodeFromName(key_name.data());
    }

    ImGuiKeyChord InputManager::parseKeyChordFromString(std::string_view key_name)
    {
        // 常用键名映射
        static const std::unordered_map<std::string_view, ImGuiKey> key_map = {
            {"Space", ImGuiKey_Space},
            {"Enter", ImGuiKey_Enter},
            {"Tab", ImGuiKey_Tab},
            {"Backspace", ImGuiKey_Backspace},
            {"Escape", ImGuiKey_Escape},
            {"LeftArrow", ImGuiKey_LeftArrow},
            {"RightArrow", ImGuiKey_RightArrow},
            {"UpArrow", ImGuiKey_UpArrow},
            {"DownArrow", ImGuiKey_DownArrow},
            {"Delete", ImGuiKey_Delete},
            {"Home", ImGuiKey_Home},
            {"End", ImGuiKey_End},
            {"PageUp", ImGuiKey_PageUp},
            {"PageDown", ImGuiKey_PageDown},
            {"Insert", ImGuiKey_Insert},
            {"A", ImGuiKey_A}, {"B", ImGuiKey_B}, {"C", ImGuiKey_C}, 
            {"D", ImGuiKey_D}, {"E", ImGuiKey_E}, {"F", ImGuiKey_F}, 
            {"G", ImGuiKey_G}, {"H", ImGuiKey_H}, {"I", ImGuiKey_I},
            {"J", ImGuiKey_J}, {"K", ImGuiKey_K}, {"L", ImGuiKey_L},
            {"M", ImGuiKey_M}, {"N", ImGuiKey_N}, {"O", ImGuiKey_O},
            {"P", ImGuiKey_P}, {"Q", ImGuiKey_Q}, {"R", ImGuiKey_R},
            {"S", ImGuiKey_S}, {"T", ImGuiKey_T}, {"U", ImGuiKey_U},
            {"V", ImGuiKey_V}, {"W", ImGuiKey_W}, {"X", ImGuiKey_X},
            {"Y", ImGuiKey_Y}, {"Z", ImGuiKey_Z},
            {"0", ImGuiKey_0}, {"1", ImGuiKey_1}, {"2", ImGuiKey_2}, {"3", ImGuiKey_3}, {"4", ImGuiKey_4}, 
            {"5", ImGuiKey_5}, {"6", ImGuiKey_6}, {"7", ImGuiKey_7}, {"8", ImGuiKey_8}, {"9", ImGuiKey_9},
        };

        auto it = key_map.find(key_name);
        if (it != key_map.end())
        {
            return it->second;
        }
        return ImGuiKey_None;
    }

    // 将鼠标按钮名称字符串转换为 SDL 按钮 Uint8 值
    Uint32 InputManager::mouseButtonFromString(std::string_view button_name)
    {
        if (button_name == "MouseLeft")
            return SDL_BUTTON_LEFT;
        if (button_name == "MouseRight")
            return SDL_BUTTON_RIGHT;
        if (button_name == "MouseMiddle")
            return SDL_BUTTON_MIDDLE;
        // SDL 还定义了 SDL_BUTTON_X1 和 SDL_BUTTON_X2
        if (button_name == "MouseX1")
            return SDL_BUTTON_X1;
        if (button_name == "MouseX2")
            return SDL_BUTTON_X2;
        return 0; // 0 不是有效的按钮值，表示无效
    }

    void InputManager::updateActionState(entt::id_type action_name_id, bool is_input_active, bool is_repeat_event)
    {
        auto it = action_states_.find(action_name_id);
        if (it == action_states_.end())
        {
            spdlog::warn("尝试更新未注册的动作状态: {}", action_name_id);
            return;
        }

        if (is_input_active) // 输入被激活(按下)
        {
            if (is_repeat_event)
            {
                it->second = ActionState::HELD;
            }
            else // 非重复地按下事件
            {
                it->second = ActionState::PRESSED;
            }
        }
        else // 输入被释放 (松开)
        {
            it->second = ActionState::RELEASED;
        }
    }

} // namespace engine::input