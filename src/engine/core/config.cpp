#include "config.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace engine::core 
{
Config::Config(std::string_view file_path)
{
    loadFromFile(file_path);
}   

bool Config::loadFromFile(std::string_view file_path)
{
    auto path = std::filesystem::path(file_path);  
    std::ifstream file(path);
    if (!file.is_open())
    {
        spdlog::warn("配置文件 '{}' 未找到。将使用默认设置并创建默认配置文件", file_path);
        if (!saveToFile(file_path))
        {
            spdlog::error("无法创建默认配置文件 '{}'", file_path);
            return false;
        }
        return false;   // 文件不存在，使用默认值
    }

    try
    {
        nlohmann::json j;
        file >> j;
        fromJson(j);
        spdlog::info("成功从 '{}' 加载配置", file_path);
        return true;
    }
    catch (const std::exception& e)
    {
        spdlog::error("读取配置文件 '{}' 时发生错误: {}。使用默认设置", file_path, e.what());
    }
    return false;
}

void Config::fromJson(const nlohmann::json& j)
{
    if (j.contains("window"))
    {
        const auto& window_config = j["window"];
        window_title_ = window_config.value("title", window_title_);
        window_width_ = window_config.value("width", window_width_);
        window_height_ = window_config.value("height", window_height_);
        window_resizable_ = window_config.value("resizable", window_resizable_);
        window_scale_ = window_config.value("window_scale", window_scale_);
        window_logical_scale_ = window_config.value("logical_scale", window_logical_scale_);
        spdlog::info("窗口标题已被设置为: {}", window_title_);
        spdlog::info("窗口大小已被设置为: {}x{}", window_width_, window_height_);
        spdlog::info("窗口缩放已被设置为: {}", window_scale_);
        spdlog::info("窗口逻辑缩放已被设置为: {}", window_logical_scale_);
        spdlog::info("窗口是否可调整大小已被设置为: {}", window_resizable_);
    }
    if (j.contains("graphics"))
    {
        const auto& graphics_config = j["graphics"];
        vsync_enabled_ = graphics_config.value("vsync", vsync_enabled_);
        spdlog::info("垂直同步已被设置为: {}", vsync_enabled_);
        imgui_main_scale_ = graphics_config.value("imgui_main_scale", imgui_main_scale_);
        if (imgui_main_scale_ <= 0)
        {
            spdlog::warn("ImGui 主窗口缩放比例必须大于零, 设置为默认值 1.0");
            imgui_main_scale_ = 1.0f;
        }
        spdlog::info("ImGui 主窗口缩放比例已被设置为: {}", imgui_main_scale_);
        imgui_style_ = graphics_config.value("imgui_style", imgui_style_);
        spdlog::info("ImGui 风格已被设置为: {}", imgui_style_);
        imgui_font_path_ = graphics_config.value("imgui_font_path", imgui_font_path_);
        spdlog::info("ImGui 字体路径已被设置为: {}", imgui_font_path_);
    }
    if (j.contains("performance"))
    {
        const auto& perf_config = j["performance"];
        target_fps_ = perf_config.value("target_fps", target_fps_);
        if (target_fps_ < 0)
        {
            spdlog::warn("目标 FPS 不能为负数, 设置为零(无限制)");
            target_fps_ = 0;
        }
        spdlog::info("目标 FPS 已被设置为: {}", target_fps_);
    }
    if (j.contains("audio"))
    {
        const auto& audio_config = j["audio"];
        music_volume_ = audio_config.value("music_volume", music_volume_);
        sound_volume_ = audio_config.value("sound_volume", sound_volume_);
        spdlog::info("音乐音量已被设置为: {}", music_volume_);
        spdlog::info("音效音量已被设置为: {}", sound_volume_);
    }

    // 从 JSON 加载 input_mappings
    if (j.contains("input_mappings") && j["input_mappings"].is_object())
    {
        const auto& mappings_json = j["input_mappings"];
        try
        {
            // 直接尝试从 JSON 对象转换为 map<string, vector<string>>
            auto input_mappings = mappings_json.get<std::unordered_map<std::string, std::vector<std::string>>>();
            // 如果成功, 则将 input_mappings 移动到 input_mappings_ 中
            input_mappings_ = std::move(input_mappings);
            spdlog::trace("成功从配置中加载输入映射");
        }
        catch(const std::exception& e)
        {
            spdlog::warn("配置加载警告：解析 'input_mappings' 时发生异常，使用默认映射。错误：{}", e.what());
        }
    }
    else
    {
        spdlog::trace("配置跟踪：未找到 'input_mappings' 部分或不是对象，使用头文件中定义的默认映射");
    }
    // 从 JSON 加载 imgui_input_mappings
    if (j.contains("imgui_input_mappings") && j["imgui_input_mappings"].is_object())
    {
        const auto &imgui_mappings_json = j["imgui_input_mappings"];
        try
        {
            // 直接尝试从 JSON 对象转换为 map<string, vector<string>>
            auto imgui_input_mappings = imgui_mappings_json.get<std::unordered_map<std::string, std::vector<std::string>>>();
            // 如果成功, 则将 input_mappings 移动到 input_mappings_ 中
            imgui_input_mappings_ = std::move(imgui_input_mappings);
            spdlog::trace("成功从配置中加载输入映射");
        }
        catch (const std::exception &e)
        {
            spdlog::warn("配置加载警告：解析 'imgui_input_mappings' 时发生异常，使用默认映射。错误：{}", e.what());
        }
    }
    else
    {
        spdlog::trace("配置跟踪：未找到 'imgui_input_mappings' 部分或不是对象，使用头文件中定义的默认映射");
    }
}

bool Config::saveToFile(std::string_view file_path)
{    
    auto path = std::filesystem::path(file_path); 
    std::ofstream file(path);
    if (!file.is_open())
    {
        spdlog::error("无法打开配置文件 '{}' 以进行写入", file_path);
        return false;
    }

    try
    {
        nlohmann::ordered_json j = toJson();
        file << j.dump(4);  // 使用缩进为 4 的格式化输出
        spdlog::info("成功将配置保存到到 '{}'", file_path);
        return true;
    }
    catch(const std::exception& e)
    {
        spdlog::error("写入配置文件 '{}' 时出错: {}", file_path, e.what());
    }
    return false;
}

nlohmann::ordered_json Config::toJson() const
{
    return nlohmann::ordered_json
    {
        {"window", {
            {"title", window_title_},
            {"width", window_width_},
            {"height", window_height_},
            {"window_scale", window_scale_},
            {"logical_scale", window_logical_scale_},
            {"resizable", window_resizable_}
        }},
        {"graphics", {
            {"vsync", vsync_enabled_},
            {"imgui_main_scale", imgui_main_scale_},
            {"imgui_style", imgui_style_},
            {"imgui_font_path", imgui_font_path_}
        }},
        {"performance", {
            {"target_fps", target_fps_}
        }},
        {"audio", {
            {"music_volume", music_volume_},
            {"sound_volume", sound_volume_}
        }},
        {"input_mappings", input_mappings_},
        {"imgui_input_mappings", imgui_input_mappings_}
    };
}

}