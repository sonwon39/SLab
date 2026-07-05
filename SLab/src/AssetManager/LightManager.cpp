#include "LightManager.h"
#include "RootSignature.h"

LightManager::LightManager()
{
    m_lightBuffer = std::make_shared<StructuredBuffer>();
}

LightManager::~LightManager()
{
}

void LightManager::Initialize(D3D12_RESOURCE_FLAGS flags, ID3D12GraphicsCommandList* commandList, const std::wstring & name)
{
    dataSize = m_lights.size() * sizeof(Light);
    m_lightBuffer->Initialize(m_lights.data(), dataSize, flags, commandList, name);
    m_lightBuffer->Map();

	//std::vector<D3D12_RESOURCE_BARRIER> barriers;
 //   D3D12_RESOURCE_BARRIER barrier;

 //   if (m_lightBuffer->Transition(D3D12_RESOURCE_STATE_COPY_DEST, barrier))
 //       barriers.push_back(barrier);
 //   if (!barriers.empty())
 //       commandList->ResourceBarrier((UINT)barriers.size(), barriers.data());

}
int LightManager::Allocate()
{
	if (m_lightNum < MAX_LIGHT)
	{
        return m_lightNum++;
	}
    return -1;
}
void LightManager::Update()
{
    m_lightBuffer->CopyToGpu(m_lights.data(), dataSize);
}

void LightManager::Bind(const RootSignature* rs, ID3D12GraphicsCommandList* cl) const
 {
    if (!rs || !cl)
        return;

    int ibl = rs->GetSlot(BindKey::LightCB);
    if (ibl >= 0)
        cl->SetGraphicsRootShaderResourceView(ibl, GetLightView());
 }
