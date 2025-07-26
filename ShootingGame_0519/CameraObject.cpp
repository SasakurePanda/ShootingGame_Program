#include "CameraObject.h"

//CameraObject::CameraObject()
//{
// 
//}

void CameraObject::Initialize()
{
    // FollowCameraComponent‚ğ¶¬•’Ç‰Á
    m_FollowCameraComponent = std::make_shared<FollowCameraComponent>();
    AddComponent(m_FollowCameraComponent);
}