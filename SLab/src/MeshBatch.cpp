#include "MeshBatch.h"
#include <iostream>

#include "StaticMesh.h"
#include "Material.h"
#include "GameFramework/PrimitiveComponent.h"
#include "Renderer.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include "Engine\RenderEngine.h"

using namespace Renderer;

void MeshBatch::Render(ID3D12GraphicsCommandList* commandList)
{
    if (!mesh || !material || !owner || !currentRS)
        return;

    material->Bind(commandList, 0); // SRV table 바인딩

    int lcb = currentRS->GetSlot(BindKey::LocalCB);
    if (lcb >= 0)
        commandList->SetGraphicsRootConstantBufferView(lcb, owner->GetCBGPUAddress());

    int mcb = currentRS->GetSlot(BindKey::MaterialCB);
    if (mcb >= 0)
        commandList->SetGraphicsRootConstantBufferView(mcb, owner->GetMCBGPUAddress());

	int smcb = currentRS->GetSlot(BindKey::SkinnedMeshCB);
    if (smcb >= 0)
        commandList->SetGraphicsRootConstantBufferView(smcb, owner->GetSMCBGPUAddress());

	if (meshType == MeshType::Mesh)
        mesh->Render(commandList); // VB/IB + DrawIndexed (StaticMesh가 이미 갖고 있음)
    else if (meshType == MeshType::Point)
        mesh->RenderPoints(commandList);
}

void MeshBatch::SyncCB(float deltaTime)
{
    if (!owner)
        return;
    if (owner->GetUpdateConstant())
    {
        owner->SyncCB(deltaTime);
    }
}

bool MeshBatch::InitGraphicsCommand(ID3D12GraphicsCommandList* commandList)
{
    GraphicsPSO pso;
    if (!GetGraphicsPSO(psoName, pso))
    {
        std::cout << "Failed to find pso\n";
        return false;
    }
    currentRS = pso.GetRootSignature();

    if (m_renderEngine->GetCurrPSOName() == psoName)
        return false;

    m_renderEngine->SetCurrPSOName(psoName);
    commandList->SetPipelineState(pso.GetPSO());
    commandList->SetGraphicsRootSignature(currentRS->GetSignature());

    return true;
}
