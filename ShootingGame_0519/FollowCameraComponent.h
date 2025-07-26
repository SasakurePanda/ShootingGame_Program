#pragma once
#include "Component.h"
#include "commontypes.h"
#include "GameObject.h"
#include "SpringVector3.h"

class FollowCameraComponent : public Component
{
public:
    FollowCameraComponent();
    void Update() override;

    //Playerなどの追尾する対称のセット関数
    void SetTarget(GameObject* target);
    //追尾対象との前後の距離のセット関数
    void SetDistance(float dist);
    //カメラの高さのセット関数
    void SetHeight(float h);

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
};

