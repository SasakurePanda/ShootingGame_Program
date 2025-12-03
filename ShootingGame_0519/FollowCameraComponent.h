#pragma once
#include "Component.h"
#include "commontypes.h"
#include "GameObject.h"
#include "SpringVector3.h"
#include "ICameraViewProvider.h"
#include <DirectXMath.h>
#include <SimpleMath.h>
#include <random>

using namespace DirectX;
using namespace DirectX::SimpleMath;

class PlayAreaComponent;

class FollowCameraComponent : public Component, public ICameraViewProvider
{
public:

    FollowCameraComponent();
    void Initialize() override {};
    void Update(float dt) override;

    //カメラが追う対象をセットする関数
    void SetTarget(GameObject* target);
    void SetVerticalAimScale(float s) { m_VerticalAimScale = std::clamp(s, 0.0f, 1.0f); }
    void SetDistance(float dist) { m_DefaultDistance = dist; }
    void SetHeight(float h) { m_DefaultHeight = h; }
    void SetSensitivity(float s) { m_Sensitivity = s; }
    void SetBoostState(bool isBoosting) override;
    void SetPlayArea(PlayAreaComponent* p) { m_playArea = p; }

    //レティクルのスクリーン位置を外部（GameScene）から毎フレーム渡すための関数
    void SetReticleScreenPos(const Vector2& screenPos) { m_ReticleScreen = screenPos; }

    float GetVerticalAimScale() const { return m_VerticalAimScale; }
    float GetSensitivity() const { return m_Sensitivity; }

    Matrix GetView() const { return m_ViewMatrix; }
    Matrix GetProj() const { return m_ProjectionMatrix; }

    Vector3 GetForward() const override;
    Vector3 GetRight() const override;

   
    //ICameraViewProvider で追加したメソッドの実装を宣言
    Vector3 GetAimPoint() const override { return m_AimPoint; }

    Vector3 GetPosition() const { return m_Spring.GetPosition(); }

    DirectX::SimpleMath::Vector3 GetAimDirectionFromReticle() const;

    //----------------------------カメラ演出関数まとめ----------------------------
    enum class ShakeMode
    {
        Horizontal,   //スクリーンのX軸
        Vertical,     //スクリーンのY軸
        //Random4,      //上下左右のいずれかをランダムに選ぶ
        ALL           //X/Y両方にランダムな成分を与える
    };
    
    void Shake(float magnitude, float duration, ShakeMode mode = ShakeMode::Horizontal);    //カメラ振動関数：振動幅、振動時間


private:

    void UpdateCameraPosition(float dt);

    GameObject* m_Target = nullptr;   //カメラが追従する対象(プレイヤーなど)のポインタ

    float m_DefaultDistance = 15.0f;  //追従対象の後方にどのぐらいにカメラがいるのか
    float m_DefaultHeight = 3.5f;     //追従対象からどのぐらい高い所にカメラがいるのか

    float m_AimDistance = 20.0f;      //エイムしたときの後方どのあたりにカメラがいるのか
    float m_AimHeight = 2.8f;         //エイムしたとき追従対象からどのぐらい高い所にカメラがいるのか

    bool m_IsAiming = false;          //今エイムしているかどうかのbool型

    float m_Yaw = 0.0f;               //マウス操作で積み上げていく回転角(ヨー)
    float m_Pitch = 0.0f;             //マウス操作で積み上げていく回転角(ピッチ)
    float m_Sensitivity = 0.001f;     //m_Sensitivity は小さいほど「ゆっくり回る」

    float m_PitchLimitMin = XMConvertToRadians(-15.0f); //ピッチ(上下)の制限値(最小)
    float m_PitchLimitMax = XMConvertToRadians(45.0f);  //ピッチ(上下)の制限値(最大)
    float m_YawLimit = XMConvertToRadians(120.0f);      //ヨー(左右)の制限値

    Matrix m_ViewMatrix;        //ビュー行列
    Matrix m_ProjectionMatrix;  //プロジェクト行列

    SpringVector3 m_Spring;     //カメラ位置をスプリングで滑らかに追従させるラッパー

    Vector2 m_ReticleScreen = Vector2(0.0f, 0.0f); // クライアント座標（px）
    Vector3 m_AimPoint = Vector3::Zero;            //レティクルが指すワールド座標
    float m_AimPlaneDistance = 50.0f;              //レイと交差させる「カメラ前方の平面までの距離」

    float m_LookAheadDistance = 8.0f;    // どれだけ前方（レティクル方向）を注視するか（画面内でプレイヤーをずらす量）
    float m_LookAheadLerp = 10.0f;       // lookTarget のスムーズ度合い

    Vector3 m_LookTarget = Vector3::Zero;

    // --- 追加メンバ: レティクルに応じたカメラの横シフト量（チューニング用） ---
    float m_ScreenOffsetScale = 16.0f; // 画面幅 1.0 正規化あたりのワールド単位換算（調整可）
    float m_MaxScreenOffset = 20.0f;  // 最大シフト（ワールド単位）

    float m_PrevPlayerYaw = 0.0f;
    float m_TurnOffsetScale = 8.0f;   // yawSpeed -> ワールド横オフセット換算（調整用）
    float m_TurnOffsetMax = 12.0f;     // オフセット最大値（ワールド単位）
    float m_CurrentTurnOffset = 0.0f; // 現在の横オフセット（滑らかに更新）
    float m_TurnOffsetLerp = 6.0f;    // オフセットが変化するときの滑らかさ（大きいと即時）


    // -----------------ブースト関連の変数------------------------
    bool m_boostRequested = false;   // 現在ボタンでブースト要求中か（MoveComponent から SetBoostState）
    float m_boostBlend = 0.0f;       // 0..1 の補間値（0 = 通常追随, 1 = ブースト時の遅追従）
    float m_boostBlendSpeed = 6.0f;  // ブレンドの速さ

    // ブースト時の目標値（チューニング）
    float m_boostAimDistanceAdd = 8.0f; // ブースト時、注視距離を伸ばす（＝カメラが後ろに残る印象）

    // 通常のバネパラメータ（FollowCameraComponent コンストラクタで設定）
    float m_normalStiffness = 12.0f;
    float m_normalDamping = 6.0f;

    float m_Fov = XMConvertToRadians(45.0f); // コンストラクタで作った FOV と合わせる
    PlayAreaComponent* m_playArea = nullptr;

    float m_VerticalAimScale = 0.85f;

    //-----------------シェイク用メンバ-----------------
    float m_shakeMagnitude = 0.0f;        //現在の振幅（ワールド単位）
    float m_shakeTimeRemaining = 0.0f;    //残り時間（秒）
    float m_shakeTotalDuration = 0.0f;    //最初に指定した振動時間（秒）
    float m_shakePhase = 0.0f;            //波形フェーズ
    float m_shakeFrequency = 25.0f;       //振動の基準周波数(Hz) — チューニング可

    DirectX::SimpleMath::Vector3 m_shakeOffset = DirectX::SimpleMath::Vector3::Zero;
    ShakeMode m_shakeMode = ShakeMode::Horizontal;  //現在の振動方向を決めるモード
};
