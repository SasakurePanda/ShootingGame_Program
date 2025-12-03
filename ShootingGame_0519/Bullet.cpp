#include "Bullet.h"
#include "Enemy.h"
#include "OBBColliderComponent.h"
#include "CollisionManager.h"
#include "SphereComponent.h"
#include "Renderer.h" // optional: for debug draw
#include <iostream>

void Bullet::Initialize()
{
    //BulletComponentを追加して運動を担当させる
    auto m_bulletComp = std::make_shared<BulletComponent>();
    if (m_bulletComp)
    {
        m_bulletComp->SetLifetime(5.0f);
    }
    m_bulletComp->Initialize();
    AddComponent(m_bulletComp);

    //OBB コライダーを追加して衝突判定に参加させる（サイズは直径ベース）
    m_collider = AddComponent<OBBColliderComponent>();

    float radius = m_radius; // もし m_radius を外部から変えたいならここを書き換

    if (m_collider)
    {
        m_collider->SetSize(Vector3(m_radius * 2.0f, m_radius * 2.0f, m_radius * 2.0f));
    }

    ID3D11Device* dev = Renderer::GetDevice();
    m_primitive.CreateSphere(dev, m_radius, 16, 8); 
}

void Bullet::Update(float dt)
{   
    GameObject::Update(dt); 
}

void Bullet::Draw(float alpha)
{
    //std::cout << "Bullet::Draw called" << std::endl;

    //ワールド行列セット
    Matrix4x4 world = GetTransform().GetMatrix();
    Renderer::SetWorldMatrix(&world);

    Renderer::SetDepthEnable(true);
    Renderer::DisableCulling(false);

    //保存: 現在 PS にバインドされている CB(slot3) を取得しておく
    ID3D11Buffer* prevPSCB = nullptr;
    Renderer::GetDeviceContext()->PSGetConstantBuffers(3, 1, &prevPSCB);

    // --- 2) マテリアルをセット（slot 3 を使う設計に合わせる） ---
    MATERIAL mat{};
    mat.Diffuse = Color(1.0f, 0.0f, 0.0f, 1.0f); // 赤
    Renderer::SetMaterial(mat);

    // --- 3) (オプションだが安全) 1x1 の赤テクスチャを作成して t0 にバインド ---
    // static にして一度だけ作る
    static ComPtr<ID3D11ShaderResourceView> s_redSRV;

    static ComPtr<ID3D11ShaderResourceView> s_blueSRV;

    auto bc = GetComponent<BulletComponent>();
    BulletComponent::BulletType owner = BulletComponent::BulletType::UNKNOW;

    if (bc)
    {
        owner = bc->GetBulletType();
    }

    // helper lambda: create 1x1 SRV with RGBA
    auto createColorSRV = [&](ComPtr<ID3D11ShaderResourceView>& outSRV, ID3D11Device* dev, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (outSRV) { return; }
        D3D11_TEXTURE2D_DESC td{};
        td.Width = 1; td.Height = 1; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        uint8_t texRGBA[4] = { r, g, b, a };
        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = texRGBA;
        sd.SysMemPitch = 4;
        ComPtr<ID3D11Texture2D> tex;
        if (SUCCEEDED(dev->CreateTexture2D(&td, &sd, tex.GetAddressOf())))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
            srvd.Format = td.Format;
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvd.Texture2D.MipLevels = 1;
            dev->CreateShaderResourceView(tex.Get(), &srvd, outSRV.GetAddressOf());
        }
        };

    ID3D11Device* dev = Renderer::GetDevice();
    createColorSRV(s_redSRV, dev, 255, 0, 0, 255);   // 赤（Player弾）
    createColorSRV(s_blueSRV, dev, 0, 120, 255, 255); // 青（Enemy弾）

    ID3D11ShaderResourceView* colorSRV = nullptr;
    if (owner == BulletComponent::BulletType::ENEMY) colorSRV = s_blueSRV.Get();
    else colorSRV = s_redSRV.Get();

    // 保存してある PS SRV を取り出して復元できるようにしておく
    ID3D11ShaderResourceView* prevSRV = nullptr;
    Renderer::GetDeviceContext()->PSGetShaderResources(0, 1, &prevSRV);

    if (colorSRV) Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &colorSRV);

    //描画
    m_primitive.Draw(Renderer::GetDeviceContext());

    //復元
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &prevSRV);
    if (prevSRV) prevSRV->Release();
}

void Bullet::OnCollision(GameObject* other)
{
    if (!other) { return; }

    //Enemyに衝突したら両方消す(他の弾やプレイヤーとは別扱いに)
    //if (dynamic_cast<Enemy*>(other))
    //{
    //    IScene* scene = GetScene();
    //    if (scene)
    //    {
    //        std::cout << "Enemyに衝突したためBulletを削除します " << std::endl;
    //        scene->RemoveObject(this);
    //        scene->RemoveObject(other);
    //    }

    //    //SceneManager::SetCurrentScene("ResultScene");
    //}
}

