#include "engine/core/game_app.h"
#include "engine/core/context.h"
#include "game/scene/game_scene.h"
#include "engine/utils/events.h"
#include <spdlog/spdlog.h>
#include <entt/signal/dispatcher.hpp>

// 只在 Windows 平台上包含 Windows.h
#ifdef _WIN32
#include <Windows.h>
#endif

// 在程序开始时设置控制台编码
void initialize_environment()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

void setupInitialScene(engine::core::Context& context)
{
    // GameApp 在调用 run 方法之前，先创建并设置初始场景
    auto game_scene = std::make_unique<game::scene::GameScene>(context);
    context.getDispatcher().trigger<engine::utils::PushSceneEvent>(engine::utils::PushSceneEvent{std::move(game_scene)});
}

int main(int /* argc */, char * /* argv */[])
{
    initialize_environment();
    spdlog::set_level(spdlog::level::debug);

    engine::core::GameApp app;
    app.registerSceneSetup(setupInitialScene);
    app.run();
    return 0;
}