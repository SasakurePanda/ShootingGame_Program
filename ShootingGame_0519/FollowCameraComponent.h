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
    FollowCameraComponent();
    void Update(float dt) override;

    //カメラが追う対象をセットする関数
    void SetTarget(GameObject* target);

    void SetDistance(float dist) { m_DefaultDistance = dist; }
    void SetHeight(float h) { m_DefaultHeight = h; }

    void SetSensitivity(float s) { m_Sensitivity = s; }
    float GetSensitivity() const { return m_Sensitivity; }

    Matrix GetView() const { return m_ViewMatrix; }
    Matrix GetProj() const { return m_ProjectionMatrix; }

    Vector3 GetForward() const override;
    Vector3 GetRight() const override;

    //レティクルのスクリーン位置を外部（GameScene）から毎フレーム渡すための関数
    void SetReticleScreenPos(const Vector2& screenPos) { m_ReticleScreen = screenPos; }

    // ICameraViewProvider で追加したメソッドの実装を宣言
    Vector3 GetAimPoint() const override { return m_AimPoint; }

    Vector3 GetPosition() const { return m_Spring.GetPosition();}

private:
    void UpdateCameraPosition();

    GameObject* m_Target = nullptr;   //カメラが追従する対象(プレイヤーなど)のポインタ

    float m_DefaultDistance = 30.0f;  //追従対象の後方にどのぐらいにカメラがいるのか
    float m_DefaultHeight = 2.5f;     //追従対象からどのぐらい高い所にカメラがいるのか

    float m_AimDistance = 20.0f;      //エイムしたときの後方どのあたりにカメラがいるのか
    float m_AimHeight = 1.8f;         //エイムしたとき追従対象からどのぐらい高い所にカメラがいるのか

    bool m_IsAiming = false;          //今エイムしているかどうかのbool型

    float m_Yaw = 0.0f;               //マウス操作で積み上げていく回転角(ヨー)
    float m_Pitch = 0.0f;             //マウス操作で積み上げていく回転角(ピッチ)
    float m_Sensitivity = 0.01f;      //m_Sensitivity は小さいほど「ゆっくり回る」

    float m_PitchLimitMin = XMConvertToRadians(-15.0f); //ピッチ(上下)の制限値(最小)
    float m_PitchLimitMax = XMConvertToRadians(45.0f);  //ピッチ(上下)の制限値(最大)
    float m_YawLimit = XMConvertToRadians(120.0f);      //ヨー(左右)の制限値

    Matrix m_ViewMatrix;        //ビュー行列
    Matrix m_ProjectionMatrix;  //プロジェクト行列

    SpringVector3 m_Spring;     //カメラ位置をスプリングで滑らかに追従させるためのラッパー
    
    Vector2 m_ReticleScreen = Vector2(0.0f, 0.0f); // ピクセル座標。Init 中に中央でセット済みでもOK
    Vector3 m_AimPoint = Vector3::Zero;            //レティクルが指すワールド座標
    float m_AimPlaneDistance = 50.0f;              //レイと交差させる「カメラ前方の平面までの距離」

    float m_LookAheadDistance = 8.0f;    // どれだけ前方（レティクル方向）を注視するか（画面内でプレイヤーをずらす量）
    float m_LookAheadLerp = 10.0f;       // lookTarget のスムーズ度合い
    bool  m_UsePlayerOrientedCamera = true; // true: カメラ位置はプレイヤーの向きに合わせて behind に置く（自然）

    Vector3 m_LookTarget = Vector3::Zero;
};
