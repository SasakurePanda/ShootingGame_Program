#pragma once
#include "Component.h"
#include "commontypes.h"
#include "GameObject.h"
#include "SpringVector3.h"
#include "ICameraViewProvider.h"
#include <DirectXMath.h>
#include <SimpleMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

class FollowCameraComponent : public Component, public ICameraViewProvider
{
public:

    void SetReticleObject(class Reticle* r) { m_pReticle = r; }

    FollowCameraComponent();
    void Initialize() override {};
    void Update(float dt) override;

    //カメラが追う対象をセットする関数
    void SetTarget(GameObject* target);

    void SetDistance(float dist) { m_DefaultDistance = dist; }
    void SetHeight(float h) { m_DefaultHeight = h; }

    void SetSensitivity(float s) { m_Sensitivity = s; }
    float GetSensitivity() const { return m_Sensitivity; }

    // ブースト状態通知（MoveComponent から呼ばれる）
    void SetBoostState(bool isBoosting) override;

    Matrix GetView() const { return m_ViewMatrix; }
    Matrix GetProj() const { return m_ProjectionMatrix; }

    Vector3 GetForward() const override;
    Vector3 GetRight() const override;

    //レティクルのスクリーン位置を外部（GameScene）から毎フレーム渡すための関数
    void SetReticleScreenPos(const Vector2& screenPos) { m_ReticleScreen = screenPos; }

    //ICameraViewProvider で追加したメソッドの実装を宣言
    Vector3 GetAimPoint() const override { return m_AimPoint; }

    Vector3 GetPosition() const { return m_Spring.GetPosition(); }

    DirectX::SimpleMath::Vector3 GetAimDirectionFromReticle() const;
private:
    class Reticle* m_pReticle = nullptr;

    void UpdateCameraPosition(float dt);

    GameObject* m_Target = nullptr;   //カメラが追従する対象(プレイヤーなど)のポインタ

    float m_DefaultDistance = 30.0f;  //追従対象の後方にどのぐらいにカメラがいるのか
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
    bool  m_UsePlayerOrientedCamera = true; // true: カメラ位置はプレイヤーの向きに合わせて behind に置く（自然）

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
   // --- ブースト制御用メンバ ---
    bool m_boostRequested = false;   // 現在ボタンでブースト要求中か（MoveComponent から SetBoostState）
    float m_boostBlend = 0.0f;       // 0..1 の補間値（0 = 通常追随, 1 = ブースト時の遅追従）
    float m_boostBlendSpeed = 6.0f;  // ブレンドの速さ

    // ブースト時の目標値（チューニング）
    float m_boostedStiffness = 6.0f;
    float m_boostedDamping = 3.0f;
    float m_boostAimDistanceAdd = 8.0f; // ブースト時、注視距離を伸ばす（＝カメラが後ろに残る印象）

    // 通常のバネパラメータ（FollowCameraComponent コンストラクタで設定）
    float m_normalStiffness = 12.0f;
    float m_normalDamping = 6.0f;
};
