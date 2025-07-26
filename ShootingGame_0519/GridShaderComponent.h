#pragma once
#include "Component.h"
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class GridShaderComponent : public Component
{
public:
    GridShaderComponent();
    ~GridShaderComponent();

    void Initialize() override;
    void Draw() override;

private:
    ComPtr<ID3D11Buffer> m_VertexBuffer;
    UINT m_VertexCount = 0;

    void CreateGridVertices();
};
