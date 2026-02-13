#标题
cmake_minimum_required(VERSION 3.10.0)
project(MonsterWar VERSION 0.1.0 LANGUAGES C CXX)

#设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#设置编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

#设置编译输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})

set(TARGET ${PROJECT_NAME}-${CMAKE_SYSTEM_NAME})

#查找并载入Cmake预设
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_mixer REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(glm REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(EnTT REQUIRED)

#设置通用源文件
set(SOURCES
    src/main.cpp
    src/engine/audio/audio_player.cpp
    src/engine/core/config.cpp
    src/engine/core/context.cpp
    src/engine/core/game_app.cpp
    src/engine/core/game_state.cpp
    src/engine/core/time.cpp
    src/engine/input/input_manager.cpp
    src/engine/loader/basic_entity_builder.cpp
    src/engine/loader/level_loader.cpp
    src/engine/render/camera.cpp
    src/engine/render/renderer.cpp
    src/engine/render/text_renderer.cpp
    src/engine/resource/resource_manager.cpp
    src/engine/resource/texture_manager.cpp
    src/engine/resource/audio_manager.cpp
    src/engine/resource/font_manager.cpp
    src/engine/system/animation_system.cpp
    src/engine/system/movement_system.cpp
    src/engine/system/render_system.cpp
    src/engine/ui/state/ui_hover_state.cpp
    src/engine/ui/state/ui_normal_state.cpp
    src/engine/ui/state/ui_pressed_state.cpp
    src/engine/ui/ui_button.cpp
    src/engine/ui/ui_element.cpp
    src/engine/ui/ui_image.cpp
    src/engine/ui/ui_interactive.cpp
    src/engine/ui/ui_label.cpp
    src/engine/ui/ui_manager.cpp
    src/engine/ui/ui_panel.cpp
    src/engine/scene/scene.cpp
    src/engine/scene/scene_manager.cpp
    src/game/scene/game_scene.cpp)

# --- 添加 ImGui 开始 --- 

# 定义 ImGui 源文件变量，方便管理
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    #Backend files
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

# 将 ImGui 的目录和其后端目录添加到头文件搜索路径
include_directories(
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# --- 添加 ImGui 结束 ---

#根据平台向“通用源文件”追加平台专属文件
if(WIN32)
    list(APPEND SOURCES resources.rc)
endif()

#添加可执行文件，使用配置好的“SOURCES”变量
add_executable(${TARGET} ${SOURCES} ${IMGUI_SOURCES})

#链接库
target_link_libraries(${TARGET}
                        ${SDL3_LIBRARIES}
                        SDL3_image::SDL3_image
                        SDL3_mixer::SDL3_mixer
                        SDL3_ttf::SDL3_ttf
                        glm::glm
                        nlohmann_json::nlohmann_json
                        spdlog::spdlog
                        EnTT::EnTT
                        )
#不弹出控制台窗口
#if (MSVC)
#    target_link_options(${TARGET} PRIVATE "/SUBSYSTEM:WINDOWS")
#endif()