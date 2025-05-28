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
	//Gameを構成している全Sceneを保存している
	static std::unordered_map
		<std::string, std::unique_ptr<IScene>> m_scenes;
	
	//今稼働しているScene名を文字列で入れていく変数
	static std::string m_currentSceneName;

public:
	//ゲームの中で使うシーンを登録する関数
	static void RegisterScene(const std::string& name, std::unique_ptr<IScene> scene);

	//現在のシーンを設定する関数(stringに設定したいScene名を入れる)
	static void SetCurrentScene(std::string);

	static std::string GetCurrentSceneName();

	//現在シーンの更新処理をする関数
	static void Update(uint64_t deltatime);

	//現在シーンの描画処理をする関数
	static void Draw(uint64_t deltatime);

	//シーンマネージャーの初期化をする関数
	static void Init();

	//全シーンを解放して終了処理をする関数
	static void Uninit();
};