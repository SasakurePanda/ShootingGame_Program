#pragma once
#include <SimpleMath.h>
#include <vector>
#include <memory>
#include "commontypes.h" 
#include "Model.h"       
#include "Component.h"

class Component;

class GameObject
{
public:
    GameObject() = default;
    virtual ~GameObject() = default;

    virtual void Initialize();
    virtual void Update();
    virtual void Draw();

    void AddComponent(std::shared_ptr<Component> comp);

    // Transform関連のGetter / Setter
    void SetPosition(const Vector3& pos) { m_transform.pos = pos; }
    void SetRotation(const Vector3& rot) { m_transform.rot = rot; }
    void SetScale(const Vector3& scl) { m_transform.scale = scl; }

    const Vector3& GetPosition() { return m_transform.pos; }
    const Vector3& GetRotation() { return m_transform.rot; }
    const Vector3& GetScale()    { return m_transform.scale; }

    const SRT& GetTransform() const { return m_transform; }

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
    SRT m_transform;
};