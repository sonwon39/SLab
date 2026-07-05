#include "Material.h"

void Material::Initialize(D3D12_GPU_DESCRIPTOR_HANDLE srvTable)
{
    m_srvTable = srvTable;
}

void Material::Bind(ID3D12GraphicsCommandList* commandList, UINT parameterIndex) const
{
    commandList->SetGraphicsRootDescriptorTable(parameterIndex, m_srvTable);
}
