#pragma once

#include "wrl.h"
#include "d3d12.h"
#include "GraphicsCommon.h"

#include <vector>
#include <map>
#include <string>

class GraphicsPSO;
class ComputePSO;
class RootSignature;

namespace Renderer
{
extern std::map<std::string, GraphicsPSO> m_PSOs;
extern std::map<std::string, ComputePSO> m_CPSOs;

extern std::vector<std::string> psoNames;
extern std::vector<std::string> cpsoNames;

extern DXGI_FORMAT backBufferFormat;
extern DXGI_FORMAT hdrFormat;
extern DXGI_FORMAT dsBufferFormat;
extern DXGI_FORMAT dsOnlyFormat;
extern DXGI_FORMAT dsOnlyDsvFormat;
extern DXGI_FORMAT dsOnlySrvFormat;
extern GraphicsPSO skinnedMeshDsOnlyPbrPSO;

// PSO 초기화
void Initialize(const Microsoft::WRL::ComPtr<ID3D12Device5>& device);
void Shutdown(void);

ID3D12PipelineState* GetPSO(std::string psoName);

bool GetGraphicsPSO(const std::string& psoName, GraphicsPSO& pso);

ComputePSO GetComputePSO(const std::string& psoName);

void BindCPSO(const std::string& psoName, ID3D12GraphicsCommandList* c);
void BindPSO(const std::string& psoName, ID3D12GraphicsCommandList* c);

} // namespace Renderer
