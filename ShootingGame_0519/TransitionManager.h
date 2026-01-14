#pragma once
#include <string>
#include <functional>
#include <d3d11.h>

/// <summary>
/// 画面遷移演出のタイプ列挙型
/// </summary>
enum class TransitionType
{
    FADE,
    IRIS
};

class TransitionManager
{
public:
    static void Init(); 
    static void Update(float deltaTime);
    static void Draw(float deltaTime);
    static void Uninit();
    
    //--------Set関数-------
    static void Start(float duration, std::function<void()> onComplete = nullptr);
    static void SetFadeSpeed(float speed) { m_fadeSpeed = speed; }
	static void SetType(TransitionType type) { m_type = type; }

	//--------Get関数-------
    static bool IsTransitioning() { return m_isTransitioning; }

private:
    ///static void FinishTransitionPhase();

    static ID3D11ShaderResourceView* m_TextureSRV;
    static bool m_isTransitioning; 
    static float m_duration; 
    static float m_elapsed;
    static float m_fadeSpeed;
    static float m_alpha;
    static int m_phase;
    static TransitionType m_type;
    static float m_timer;
    static std::string m_nextScene;
    static std::function<void()> m_preload;

};
