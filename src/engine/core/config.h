#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp> // nlohmann_json 提供的前向声明

namespace engine::core
{
    /**
     * @brief 管理应用程序的配置设置。
     *
     * 提供配置项的默认值，并支持从 json 文件加载/保存配置
     * 如果失败或文件不存在，将使用默认值
     */

    class Config final
    {
    public:
        //  --- 默认配置值 --- (为了方便拓展，全部设置为公有)
        // 窗口设置
        std::string window_title_ = "PlantsVsZombies_EnChanted";
        int window_width_ = 480;
        int window_height_ = 360;
        bool window_resizable_ = true;
        float window_scale_ = 1.0f;
        float window_logical_scale_ = 1.0f;

        // 图形设置
        bool vsync_enabled_ = true; ///< @brief 是否启用垂直同步
        float imgui_main_scale_ = 1.0f; ///< @brief ImGui 主窗口缩放比例
        std::string imgui_style_ = "Dark"; ///< @brief ImGui 风格

        // 性能设置
        int target_fps_ = 60; ///< @brief 目标 fps 设置， 0 表示不限制

        // 音频设置
        float music_volume_ = 0.2f; ///< @brief 音乐音量
        float sound_volume_ = 0.5f; ///< @brief 音效音量

        // 存储动作名称到 SDL_Scancode 名称列表的映射
        std::unordered_map<std::string, std::vector<std::string>> input_mappings_ =
            {
                // 提供一些合理的默认值，以防配置文件加载失败或缺少此部分
                {"move_left", {"A", "Left"}},
                {"move_right", {"D", "Right"}},
                {"move_up", {"W", "Up"}},
                {"move_down", {"S", "Down"}},
                {"jump", {"J", "Space"}},
                {"attack", {"K", "MouseLeft"}},
                {"pause", {"P", "Escape"}}
                // 可以继续添加更多默认动作
        };

        // 存储动作名称到 ImGuiKeyChord 名称列表的映射
        std::unordered_map<std::string, std::vector<std::string>> imgui_input_mappings_ = 
        {
            // 提供一些合理的默认值，以防配置文件加载失败或缺少此部分
            {"upgrade", {"U"}},
            {"retreat", {"R"}},
            {"skill", {"S"}},
            {"pause", {"Escape"}}
        };

        explicit Config(std::string_view file_path); ///< @brief 构造函数，指定配置文件路径

        // 删除拷贝和移动语义
        Config(const Config &) = delete;
        Config &operator=(const Config &) = delete;
        Config(Config &&) = delete;
        Config &operator=(Config &&) = delete;

        bool loadFromFile(std::string_view file_path);             ///< @brief 从指定的 JSON 文件加载配置，成功返回 true, 否则返回 false
        [[nodiscard]] bool saveToFile(std::string_view file_path); ///< @brief 将当前配置保存到指定的 JSON 文件，成功返回 true, 否则返回 false

    private:
        void fromJson(const nlohmann::json &j); ///< @brief 从 JSON 对象反序列化配置
        nlohmann::ordered_json toJson() const;  ///< @brief 将当前配置序列化为 JSON 对象 (按顺序)
    };

} // namespace engine::core