#pragma once
#include "Component.h"
#include "Model.h"
#include <memory>

class ModelComponent : public Component
{
public:
    ModelComponent() = default;
    virtual ~ModelComponent() override = default;

    // モデルを読み込む
    bool LoadModel(const std::string& filepath);

    // 描画（まだTransform連携していないので仮引数）
    void Draw() override;

private:
    std::unique_ptr<Model> m_model;
};

