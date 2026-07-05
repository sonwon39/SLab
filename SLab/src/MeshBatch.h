#pragma once

#include "d3d12.h"
#include <string>
#include "MeshType.h"

class StaticMesh;
class Material;
class PrimitiveComponent;
class RootSignature;

// 한 번의 mesh draw를 위한 가벼운 참조 묶음.
// 자원은 모두 외부 소유 (mesh = StaticMesh 자산, material = World 캐시, owner = 컴포넌트).
class MeshBatch
{
  public:
    void Render(ID3D12GraphicsCommandList* commandList);
    void SyncCB(float deltaTime);

    // pso 변경 and 성공 여부 확인 return
    bool InitGraphicsCommand(ID3D12GraphicsCommandList* commandList);
    const RootSignature* GetCurrentRootSignature()
    {
        return currentRS;
    }

  public:
    StaticMesh* mesh = nullptr;
    const Material* material = nullptr;
    PrimitiveComponent* owner = nullptr; // per-primitive CB sync용
    std::string psoName;
    const RootSignature* currentRS;

	MeshType meshType;
};
