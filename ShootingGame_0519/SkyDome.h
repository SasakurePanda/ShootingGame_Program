#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "GameObject.h"
#include "Primitive.h"
#include "ICameraViewProvider.h"

class SkyDome : public GameObject
{
public:
    SkyDome(const std::string& texPath = "");
    ~SkyDome() override = default;

    void Initialize() override;
    void Update(float dt) override;
    void Draw(float alpha) override;

    void SetCamera(ICameraViewProvider* provider)
    {
        m_cameraProvider = provider;
    }

private:
    Primitive m_primitive;      //スカイドーム用の球体描画
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;  //
    float m_radius = 100.0f;                                     // カメラがいる座標系の単位に合わせて
    ICameraViewProvider* m_cameraProvider = nullptr;             //位置を合わせるためのカメラ取得
};
