#include "engine/core/game_app.h"
#include "engine/core/context.h"
#include "game/scene/game_scene.h"
#include "engine/utils/events.h"
#include <spdlog/spdlog.h>
#include <entt/signal/dispatcher.hpp>

void setupInitialScene(engine::core::Context& context)
{
    // GameApp 在调用 run 方法之前，先创建并设置初始场景
    auto game_scene = std::make_unique<game::scene::GameScene>(context);
    context.getDispatcher().trigger<engine::utils::PushSceneEvent>(engine::utils::PushSceneEvent{std::move(game_scene)});
}

int main(int /* argc */, char * /* argv */[])
{
    spdlog::set_level(spdlog::level::trace);

    engine::core::GameApp app;
    app.registerSceneSetup(setupInitialScene);
    app.run();
    return 0;
}