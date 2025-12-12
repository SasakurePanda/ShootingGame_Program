#pragma once
#include <vector>
#include <memory>
#include <SimpleMath.h>
#include <string>
#include <random>   
#include "ModelResource.h"


//建物の初期設定
struct BuildingConfig
{
    std::string modelPath;                  //ファイルパス
    int count = 10;                         //建物の数
    float areaWidth = 10.0f;                //建物幅
    float areaDepth = 10.0f;                //建物奥行き
    float spacing = 10.0f;                  //建物の配置間隔

    bool randomizeRotation = true;
    float minScale = 75.0f, maxScale = 75.0f; //大きさの最小/最大

    float footprintSizeX = 6.0f;         //建物の足元サイズX
    float footprintSizeZ = 6.0f;         //建物の足元サイズZ


    DirectX::SimpleMath::Vector3 baseColliderSize = { 3.0f, 17.0f, 3.0f };  //基本のコライダーサイズ

    int maxAttemptsPerBuilding = 50;    //1つの建物を置くために試行する回数上限

    std::vector<DirectX::SimpleMath::Vector3> fixedPositions;  //固定生成時の建物の位置;
};

class GameScene;

class BuildingSpawner
{
public:
    BuildingSpawner(GameScene* scene);


    //建物の生成関数
    int RandomSpawn(const BuildingConfig& cfg);

    int Spawn(const BuildingConfig& cfg);

private:
    GameScene* m_scene;

    struct PlacedRect
    {
        float cx, cz;           //中心座標 X,Z
        float halfW, halfD;     //半幅, 半奥行
    };

    std::vector<PlacedRect> m_placed;   //既に配置した建物の footprint
    std::mt19937_64 m_rng;
};