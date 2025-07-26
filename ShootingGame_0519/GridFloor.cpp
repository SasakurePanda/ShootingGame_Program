#include "GridFloor.h"

void GridFloor::Initialize()
{
    auto modelComp = std::make_shared<ModelComponent>();
    modelComp->LoadModel("Asset/Model/Robot/12211_Robot_l2.obj"); // îCà”ÇÃè∞ÉÇÉfÉã
    AddComponent(modelComp);
}