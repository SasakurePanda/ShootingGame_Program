#include "GameObject.h"

void GameObject::Initialize()
{
    for (auto& comp : m_components)
    {
        comp->Initialize();
    }
}

void GameObject::Update(float dt)
{
    m_prevTransform = m_transform;

    for (auto& comp : m_components)
    {
        comp->Update(dt);
    }
}

void GameObject::Draw(float alpha)
{
    for (auto& comp : m_components)
    {
        comp->Draw(alpha);
    }
}

void GameObject::Uninit()
{
    char buf[256];
    sprintf_s(buf, "DEBUG: GameObject::Uninit called for obj=%p components=%zu\n", (void*)this, m_components.size());
    OutputDebugStringA(buf);

    if (m_uninitialized) { return; }// 二重解放防止
    m_uninitialized = true;

    for (auto& comp : m_components)
    {
        if (comp)
        {
            comp->Uninit();
        }
    }

    m_components.clear();
}

void GameObject::AddComponent(std::shared_ptr<Component> comp) 
{
    if (!comp) { return; }

    comp->SetOwner(this);
    m_components.push_back(comp);
}

DirectX::SimpleMath::Matrix GameObject::GetWorldMatrix() const
{
    using namespace DirectX::SimpleMath;
    // 自身のローカル行列
    Matrix world = m_transform.GetMatrix();

    // 親がいるなら親のワールド行列を掛ける（親→子 の順）
    const GameObject* p = m_parent;
    while (p)
    {
        // 親の行列を取得して左から掛ける（親行列 * 子行列）
        world = p->m_transform.GetMatrix() * world;
        p = p->m_parent;
    }
    return world;
}

// ワールド前方（Z負方向/正方向の定義はエンジンごとに異なる）
// SimpleMath::Matrix::Forward() を使うと行列の Forward ベクトルが得られます。
// 万一 Forward() が使えない環境なら下の Transform を使うフォールバックを用いる。
DirectX::SimpleMath::Vector3 GameObject::GetForward() const
{
    using namespace DirectX::SimpleMath;
    Matrix world = GetWorldMatrix();

#if defined(__has_member) // (just a harmless guard, not necessary)
#endif

    // try using Matrix::Forward() if available
    // (FollowCameraComponent でも同様に .Invert().Forward() を使っていたため互換性あり)
    Vector3 f;
#ifdef DIRECTX_SIMPLEMATH_HAS_FORWARDFUNC
    f = world.Forward();
#else
    // フォールバック：ローカルの正面 (0,0,1) または (0,0,-1) のどちらがエンジンで使われているかによる
    // ここでは DirectX の右手系を想定して -Z が前方の場合は (0,0,-1) を使うことが多いです。
    // FollowCameraComponent で使っている向きに合わせるため下は -Z を基準にしています。
    f = Vector3::Transform(Vector3(0.0f, 0.0f, -1.0f), world);
#endif

    if (f.LengthSquared() > 1e-6f) f.Normalize();
    else f = Vector3::Forward; // 安全策（SimpleMath::Vector3::Forward は (0,0,-1) か実装依存）
    return f;
}

DirectX::SimpleMath::Vector3 GameObject::GetRight() const
{
    using namespace DirectX::SimpleMath;
    Matrix world = GetWorldMatrix();
    Vector3 r;
#ifdef DIRECTX_SIMPLEMATH_HAS_RIGHTFUNC
    r = world.Right();
#else
    r = Vector3::Transform(Vector3(1.0f, 0.0f, 0.0f), world);
#endif
    if (r.LengthSquared() > 1e-6f) r.Normalize();
    else r = Vector3::Right;
    return r;
}

DirectX::SimpleMath::Vector3 GameObject::GetUp() const
{
    using namespace DirectX::SimpleMath;
    Matrix world = GetWorldMatrix();
    Vector3 u;
#ifdef DIRECTX_SIMPLEMATH_HAS_UPFUNC
    u = world.Up();
#else
    u = Vector3::Transform(Vector3(0.0f, 1.0f, 0.0f), world);
#endif
    if (u.LengthSquared() > 1e-6f) u.Normalize();
    else u = Vector3::Up;
    return u;
}
