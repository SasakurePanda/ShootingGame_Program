#pragma once
#include "Component.h"
#include "commontypes.h"
#include "GameObject.h"
#include "SpringVector3.h"
#include "ICameraViewProvider.h"

//カメラのベクトルを使ってPlayerの動く方向が決まるのでそれを渡せるように
//インターフェイスを継承
class FollowCameraComponent : public Component,public ICameraViewProvider
{
public:
    FollowCameraComponent();
    void Update() override;

    //Playerなどの追尾する対称のセット関数
    void SetTarget(GameObject* target)
    {
        m_Target = target;
        if (target)
        {
            Vector3 initial = target->GetPosition() + Vector3(0, m_Height, -m_Distance);
            m_Spring.Reset(initial);
        }
    }

    //追尾対象との前後の距離のセット関数
    void SetDistance(float dist) { m_Distance = dist; }
    //カメラの高さのセット関数
    void SetHeight(float h) { m_Height = h; }

    //--------------------インターフェイス分の関数宣言--------------------
    Vector3 GetForward() const override
    {
        //ビュー行列の逆から前方向
        return m_ViewMatrix.Invert().Forward(); 
    }

    Vector3 GetRight() const override
    {
        // ビュー行列の逆から右方向
        return m_ViewMatrix.Invert().Right(); 
    }
    //---------------------------------------------------------------------


private:
    //追尾対象保存変数
    GameObject* m_Target = nullptr;

    //追尾対象との距離を示す変数
    float m_Distance = 20.0f;

    //カメラの高さを示す変数
    float m_Height = 10.0f;

    Matrix4x4 m_ViewMatrix;
    Matrix4x4 m_ProjectionMatrix;

    SpringVector3 m_Spring; //バネ追尾にしよい

    float m_Yaw = 0.0f;   //左右方向の回転角(ラジアン)
    float m_Pitch = 0.0f; //上下方向の回転角(ラジアン)
};

