#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "NonCopyable.h"
#include "IScene.h"

// 前方宣言
class IScene;
//-------------------------------------------------------------
//Scene遷移などを管理するクラス
// (コピー禁止で生成)
//-------------------------------------------------------------
class SceneManager : NonCopyable
{
private:
	//Gameを構成している全Sceneを保存している
	static std::unordered_map
		<std::string, std::unique_ptr<IScene>> m_scenes;
	
	//今稼働しているScene名を文字列で入れていく変数
	static std::string m_currentSceneName;

	static bool IsSceneChange;

	static bool m_sceneChangedThisFrame;

public:
	//ゲームの中で使うシーンを登録する関数
	static void RegisterScene(const std::string& name, std::unique_ptr<IScene> scene);

	//現在のシーンを設定する関数(stringに設定したいScene名を入れる)
	static void SetCurrentScene(const std::string& name);

	//今のシーンを取得する
	static std::string GetCurrentSceneName();

	static void SetChangeScene(const std::string& name);

	//現在シーンの更新処理をする関数
	static void Update(float deltatime);

	//現在シーンの描画処理をする関数
	static void Draw(float deltatime);

	//UIとワールドに置いてあるオブジェクトとを別で描画
	static void DrawWorld(float deltatime);
	static void DrawUI(float deltatime);

	//シーンマネージャーの初期化をする関数
	static void Init();

	//全シーンを解放して終了処理をする関数
	static void Uninit();
};