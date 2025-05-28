#include "SceneManager.h"
#include "TitleScene.h"
#include "TestScene.h"

std::unordered_map<std::string, std::unique_ptr<IScene>> SceneManager::m_scenes;

std::string SceneManager::m_currentSceneName;

void SceneManager::RegisterScene(const std::string& name, std::unique_ptr<IScene> scene)
{
    m_scenes[name] = std::move(scene); //m_sceneに引数のScene名とスーマートポインタを使って登録
}

void SceneManager::SetCurrentScene(std::string)
{


}

std::string SceneManager::GetCurrentSceneName()
{
    return m_currentSceneName;
}

void SceneManager::Init()
{
    //-------------------------------------------------------------
    //例）TitleSceneを登録
    //RegisterScene("TitleScene", std::make_unique<TitleScene>());
    //m_scenes["TitleScene"]->init();
    //-------------------------------------------------------------
    RegisterScene("TitleScene", std::make_unique<TitleScene>());
    m_scenes["TitleScene"]->Init();
    RegisterScene("TestScene", std::make_unique<TestScene>());
    m_scenes["TitleScene"]->Init();
    
    //初期シーンにTitleSceneを設定
    m_currentSceneName = "TitleScene";
}

void SceneManager::Update(uint64_t deltatime)
{
    //現在選択中のSceneクラスのUpdateをかける
    m_scenes[m_currentSceneName]->Update(deltatime);
}

void SceneManager::Draw(uint64_t deltatime)
{
    //現在選択中のSceneクラスのDrawをかける
    m_scenes[m_currentSceneName]->Draw(deltatime);
}

void SceneManager::Uninit()
{
    // 登録されているすべてシーンの終了処理
    for (auto& s : m_scenes)
    {
        s.second->Uninit();
    }

    m_scenes.clear();
    m_currentSceneName.clear();
}