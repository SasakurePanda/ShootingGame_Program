#pragma once
#include <SimpleMath.h>
#include <vector>
#include <memory>
#include "commontypes.h" 
#include "Model.h"       
#include "Component.h"
#include "IScene.h"

class Component;

class GameObject
{
public:
    GameObject() = default;
    virtual ~GameObject() = default;

    virtual void Initialize();
    virtual void Update(float dt);   
    virtual void Draw(float alpha); 
    virtual void Uninit();

    template<typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args)
    {
        auto comp = std::make_shared<T>(std::forward<Args>(args)...);
        comp->SetOwner(this);
        m_components.push_back(comp);
        return comp;
    }

    void AddComponent(std::shared_ptr<Component> comp);

    //位置・回転・大きさのセッター
    void SetPosition(const Vector3& pos) { m_transform.pos = pos;}
    void SetRotation(const Vector3& rot) { m_transform.rot = rot; }
    void SetScale(const Vector3& scl) { m_transform.scale = scl; }

    //位置・回転・大きさのセッター
    const Vector3& GetPosition() { return m_transform.pos; }
    const Vector3& GetRotation() { return m_transform.rot; }
    const Vector3& GetScale()    { return m_transform.scale;}

    //現在シーンのゲッター・セッター
    void SetScene(IScene* s) { m_scene = s; }
    IScene* GetScene() const { return m_scene; }

    //まとめてトランスフォームのゲッターセッター
    const SRT& GetTransform() const { return m_transform; }

    //
    DirectX::SimpleMath::Matrix  GetWorldMatrix() const;  //ワールド変換行列を返す
    DirectX::SimpleMath::Vector3 GetForward() const;      //ワールド前方(正規化済み)
    DirectX::SimpleMath::Vector3 GetRight() const;        //ワールド右方向(正規化済み)
    DirectX::SimpleMath::Vector3 GetUp() const;           //ワールド上方向(正規化済み)

    //衝突通知
    virtual void OnCollision(GameObject* other) {}

    template<typename T>
    std::shared_ptr<T> GetComponent() const
    {
        for (auto& comp : m_components)
        {
            // T型にダウンキャストできるか判定
            auto casted = std::dynamic_pointer_cast<T>(comp);
            if (casted)
            {
                return casted;
            }
        }
        return nullptr; // 見つからなければ nullptr
    }

private:
    std::vector<std::shared_ptr<Component>> m_components;
    bool m_uninitialized = false;
    SRT m_transform;
    Vector3 m_localPosition; // 現在ある位置
    GameObject* m_parent = nullptr; // 親オブジェクト（親がいない場合は nullptr）]
    SRT m_prevTransform; // ← 補間用に追加

    IScene* m_scene = nullptr;
};