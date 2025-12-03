#pragma once
#include "Component.h"
#include "Primitive.h"
#include <SimpleMath.h>
#include <memory>

using namespace DirectX::SimpleMath;

class BoxComponent : public Component
{
public:
    BoxComponent() = default;
    ~BoxComponent() override = default;

    void Initialize() override;
    void Update(float dt) override {}
    void Draw(float alpha) override;

private:
    static std::shared_ptr<Primitive> s_sharedPrimitive;
};

