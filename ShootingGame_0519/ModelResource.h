//#pragma once
//#include <vector>
//#include <string>
//#include <wrl/client.h>
//#include <memory>
//#include <d3d11.h>
//#include "commontypes.h" // MATERIAL, VERTEX_3D etc.
//
//using Microsoft::WRL::ComPtr;
//
//struct ModelMeshInfo
//{
//    ComPtr<ID3D11Buffer> vertexBuffer;
//    ComPtr<ID3D11Buffer> indexBuffer;
//    UINT indexCount = 0;
//    MATERIAL material;
//    ComPtr<ID3D11ShaderResourceView> srvDiffuse;
//    // ... other SRVs
//};
//
//struct ModelResource
//{
//    std::string path; // normalized key
//    std::vector<ModelMeshInfo> meshes;
//    // optional bounding box, center, scale
//};
//
//using ModelResourcePtr = std::shared_ptr<ModelResource>;