#pragma once
#include "GameObject.h"
#include "TextureComponent.h"
#include <SimpleMath.h>
#include <memory>

using namespace DirectX::SimpleMath;

class HPBar : public GameObject
{
public:
    HPBar(const std::wstring& framePath, const std::wstring& gaugePath, float width = 200.0f, float height = 24.0f);
    ~HPBar() override = default;

    void Initialize() override;
    void Update(float dt) override;
    void Draw(float alpha) override;

    // 画面上の左上位置をセット（px）
    void SetScreenPos(float x, float y) { m_pos = { x, y }; UpdateLayout(); }

    // HP を更新（外部から呼ぶ）
    void SetHP(float current, float max);

    // 即時で比率を設定する（デバッグ用）
    void SetRatioImmediate(float r) { m_ratio = std::clamp(r, 0.0f, 1.0f); UpdateLayout(); }

private:
    void UpdateLayout(); // テクスチャの位置/サイズを反映する

    std::wstring m_framePath;
    std::wstring m_gaugePath;
    float m_width;
    float m_height;

    Vector2 m_pos = Vector2(100.0f, 100.0f); // 左上座標

    std::shared_ptr<TextureComponent> m_frameTex;
    std::shared_ptr<TextureComponent> m_gaugeTex;

    // HP 管理
    float m_currentHP = 100.0f;
    float m_maxHP = 100.0f;
    float m_ratio = 1.0f;          //表示中の比率(0..1)
    float m_targetRatio = 1.0f;    //目標比率（スムージング先）
    float m_lerpSpeed = 8.0f;      //速さ（高いほど即時）
};
