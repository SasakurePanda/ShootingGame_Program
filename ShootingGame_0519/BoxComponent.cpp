#include "BoxComponent.h"
#include "Renderer.h"
#include "GameObject.h"
#include <cassert>

using namespace DirectX::SimpleMath;

std::shared_ptr<Primitive> BoxComponent::s_sharedPrimitive = nullptr;

void BoxComponent::Initialize()
{
    // 共有プリミティブが未作成なら作成する（デバイスは Renderer から取得）
    if (!s_sharedPrimitive)
    {
        ID3D11Device* device = Renderer::GetDevice();
        assert(device && "Renderer::GetDevice() is null in BoxComponent::Initialize");

        // allocate and create unit box (1x1x1)
        s_sharedPrimitive = std::make_shared<Primitive>();
        s_sharedPrimitive->CreateBox(device, 1.0f, 1.0f, 1.0f);
    }
}

void BoxComponent::Draw(float /*alpha*/)
{
    if (!GetOwner()){ return; }

    Renderer::SetDepthEnable(false);

    // ワールド行列（GameObject のスケール/回転/平行移動を含む）
    Matrix4x4 world = GetOwner()->GetTransform().GetMatrix();
    Renderer::SetWorldMatrix(&world);

    ID3D11DeviceContext* ctx = Renderer::GetDeviceContext();
    if (!ctx){ return; }

    if (s_sharedPrimitive)
    {
        s_sharedPrimitive->Draw(ctx);
    }
}
