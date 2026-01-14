#include "TransitionManager.h"
#include "TextureManager.h"
#include "SceneManager.h"
#include "Renderer.h"         // DrawFullScreenQuad 等（下で使う）
#include "Application.h"
#include <cassert>
#include <iostream>
#include <algorithm>

ID3D11ShaderResourceView* TransitionManager::m_TextureSRV;
bool  TransitionManager::m_isTransitioning;
float TransitionManager::m_fadeSpeed;
TransitionType TransitionManager::m_type = TransitionType::FADE;
float TransitionManager::m_timer;
float TransitionManager::m_duration;    //
float TransitionManager::m_elapsed;
float TransitionManager::m_alpha;
int   TransitionManager::m_phase;
std::string TransitionManager::m_nextScene;
std::function<void()> TransitionManager::m_preload;

//--------------------------------------------------------
//                      初期化関数
//--------------------------------------------------------
void TransitionManager::Init()
{
    m_TextureSRV = nullptr;
    m_isTransitioning = false;
    m_timer = 0.0f;
    m_duration = 1.0f;
    m_elapsed = 0.0f;
    m_alpha = 0.0f;
    m_phase = 0;
    m_type = TransitionType::FADE;
    m_nextScene.clear();
	m_preload = nullptr;

    m_TextureSRV = TextureManager::Load("Asset/Texture/Transition_Fade01.png");
}

//--------------------------------------------------------
//                      更新関数
//--------------------------------------------------------
void TransitionManager::Update(float deltaTime)
{
    if (!m_isTransitioning){ return; }

    //安全確保
    if (deltaTime < 0.0f)
    {
        deltaTime = 0.0f;
    }

    m_elapsed += deltaTime;

    float half = m_duration * 0.5f;

    if (m_phase == 0)
    {
        if (half <= 0.0f)
        {
            m_alpha = 1.0f;
            return;
        }

        float t = std::clamp(m_elapsed / half, 0.0f, 1.0f);
        m_alpha = t;

        if (m_elapsed >= half)
        {
            // フェーズ切り替え（ここでシーン切替等のコールバックを呼ぶ）
            if (m_isTransitioning)
            {
                // ユーザが渡したコールバック（シーン読み込みやリソース差し替え等）
                m_preload();
            }

            // 次フェーズのため経過時間を半分を超えた分だけ引く
            m_elapsed -= half;
            m_phase = 1;
        }
    }
    else if (m_phase == 1)
    {
        // フェードイン（1 -> 0） : 経過 0..half
        if (half <= 0.0f)
        {
            m_alpha = 0.0f;
            // 終了
            m_isTransitioning = false;
            return;
        }

        float t = std::clamp(m_elapsed / half, 0.0f, 1.0f);
        m_alpha = 1.0f - t;

        if (m_elapsed >= half)
        {
            // トランジション終了
            m_isTransitioning = false;
            m_phase = 0;
            m_elapsed = 0.0f;
            m_alpha = 0.0f;
            m_preload = nullptr;
        }
    }
}
//--------------------------------------------------------
//                     描画関数
//--------------------------------------------------------
void TransitionManager::Draw(float deltaTime)
{
    if (!m_isTransitioning){ return; }

    if (!m_TextureSRV){ return; }

    Vector2 topLeft;
    topLeft.x = 0.0f;
    topLeft.y = 0.0f;

    Vector2 size;
    size.x = static_cast<float>(Application::GetWidth());
    size.y = static_cast<float>(Application::GetHeight());

    //ブレンド／深度設定
    Renderer::SetDepthEnable(false);
    Renderer::SetBlendState(BS_ALPHABLEND);

    //アルファ定数バッファに現在の値をセットしてバインド（Renderer に実装済みの SetTextureAlpha を使う）
    Renderer::SetTextureAlpha(m_alpha);

    //画像を描画
    Renderer::DrawTexture(m_TextureSRV, topLeft, size);
}

//--------------------------------------------------------
//                     描画関数
//--------------------------------------------------------
void TransitionManager::Uninit()
{
    m_TextureSRV = nullptr;
    m_isTransitioning = false;
    m_preload = nullptr;
}

/// <summary>
/// 画面遷移の開始時の設定関数
/// </summary>
/// <param name="duration"></param>
/// <param name="onComplete"></param>
void TransitionManager::Start(float duration, std::function<void()> onComplete)
{
    if (duration <= 0.0f)
    {
        // 即時完了扱い
        if (onComplete)
        {
            onComplete();
        }
        return;
    }

    m_duration = duration;
    m_elapsed = 0.0f;
    m_phase = 0;           // 0: fade-out, 1: fade-in
    m_isTransitioning = true;
    m_preload = onComplete;
    m_alpha = 0.0f;        // 開始時は透明 -> フェードアウトで不透明へ
}


