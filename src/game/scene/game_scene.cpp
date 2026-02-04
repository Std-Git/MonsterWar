#include "game_scene.h"
#include "../../engine/core/context.h"
#include "../../engine/input/input_manager.h"
#include "../../engine/audio/audio_player.h"
#include "../../engine/resource/resource_manager.h"
#include "../../engine/render/text_renderer.h"
#include "../../engine/ui/ui_manager.h"
#include "../../engine/ui/ui_image.h"
#include "../../engine/ui/ui_label.h"
#include "../../engine/utils/events.h"
#include <entt/core/hashed_string.hpp>
#include <entt/signal/sigh.hpp>
#include <entt/signal/dispatcher.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::scene
{
GameScene::GameScene(engine::core::Context &context)
    : engine::scene::Scene("GameScene", context)
{
}

GameScene::~GameScene()
{
}

void GameScene::init()
{
    // 测试场景编号，每创建一个场景，编号加1
    static int count = 0;
    scene_num_ = count++;
    spdlog::info("场景编号：{}", scene_num_);

    //注册输入回调事件
    //auto &input_manager = context_.getInputManager();
    //input_manager.onAction("jump").connect<&GameScene::onReplace>(this);    // J
    //input_manager.onAction("mouse_left").connect<&GameScene::onPush>(this); // 鼠标左键
    //input_manager.onAction("mouse_right").connect<&GameScene::onPop>(this); // 鼠标右键
    //input_manager.onAction("pause").connect<&GameScene::onQuit>(this);      // P   // <bug> bug1:onQuit 打成 quit

    // 测试资源管理器
    testResourceManager();

    Scene::init();
}

void GameScene::clean()
{
    // 断开输入回调事件(谁连接，谁负责断开)
    //auto &input_manager = context_.getInputManager();
    //input_manager.onAction("jump").disconnect<&GameScene::onReplace>(this);
    //input_manager.onAction("mouse_left").disconnect<&GameScene::onPush>(this);
    //input_manager.onAction("mouse_right").disconnect<&GameScene::onPop>(this);
    //input_manager.onAction("pause").disconnect<&GameScene::onQuit>(this);

    Scene::clean();
}

bool GameScene::onReplace()
{
    spdlog::info("onReplace, 切换场景");
    requestReplaceScene(std::make_unique<game::scene::GameScene>(context_));
    return true;
}

bool GameScene::onPush()
{
    spdlog::info("onPush, 压入场景");
    requestPushScene(std::make_unique<game::scene::GameScene>(context_));
    return true;
}

bool GameScene::onPop()
{
    spdlog::info("onPop, 弹出编号为 {} 的场景", scene_num_);
    requestPopScene();
    return true;
}

bool GameScene::onQuit()
{
    spdlog::info("quit, 退出游戏");
    quit();
    return true;
}

void GameScene::testResourceManager()
{
    // 载入资源
    context_.getResourceManager().loadTexture("assets/textures/Buildings/Castle.png"_hs);
    // 播放音乐
    context_.getAudioPlayer().playMusic("battle_bgm"_hs);

    // 测试 UI 元素 (使用载入的资源)
    ui_manager_->addElement(std::make_unique<engine::ui::UIImage>("assets/textures/Buildings/Castle.png"_hs));
    ui_manager_->addElement(std::make_unique<engine::ui::UILabel>(
        context_.getTextRenderer(),
        "Hello, World!",
        "assets/fonts/VonwaonBitmap-16px.ttf"));
}
}   // namespace game::scene