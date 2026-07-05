# SLab

유체 시뮬레이션 학습·실습용 DirectX 12 랩(lab). 기존 `SEngine` 렌더러를 기반으로,
여러 유체 시뮬레이션 튜토리얼을 학습한 뒤 이곳에 모듈로 추가·실험하는 것이 목적이다.

## 빌드

- **Visual Studio 2022**: `SLab.sln` 열고 `x64` 구성(Debug/Release) 빌드.
- **명령줄**:
  ```
  build.bat Debug x64      :: 빌드
  rebuild.bat Release x64   :: 리빌드
  clean.bat                 :: 클린
  ```
- 산출물: `x64/<Config>/SLab.exe`

### 의존성
- Windows SDK, MSVC v143 (VS2022), C++20
- **vcpkg** (전역 통합, `$(VcpkgRoot)` 사용): `directxtk12`, `imgui`, `physx`, `stb`, `assimp`, `fp16`, `directx-headers`
- **NuGet**: `WinPixEventRuntime` (`packages/`, `packages.config`로 복원)

## 디렉터리 구조

```
SLab/                     # 레포 루트 (SLab.sln)
├─ SLab/                  # 프로젝트 (SLab.vcxproj)
│  ├─ src/                #   C++ 소스 (유일한 include 루트)
│  │  ├─ Core/            #     D3D12 저수준 래퍼 (Buffer/Texture/Timer)
│  │  ├─ GameFramework/   #     Actor/Component 시스템
│  │  ├─ Engine/          #     World/Scene/Level/RenderEngine
│  │  ├─ Actors/          #     구체 Actor
│  │  ├─ ActorComponents/ #
│  │  ├─ AssetManager/    #     모델/텍스처/라이트 로딩
│  │  ├─ InputHelper/     #     입력/마우스
│  │  ├─ SPH/ StableFluids/ Noise/   #  ← 시뮬레이션 모듈 (여기에 추가)
│  │  ├─ PBR/             #     PBRHLSLCompat.h (C++/HLSL 공용 구조체)
│  │  └─ *.cpp/*.h        #     App/Renderer/공용 헤더 (root)
│  └─ shaders/            #   HLSL (빌드 시 컴파일)
│     ├─ Default/ CubeMap/ PBR/
│     └─ SPH/ StableFluids/ Noise/
└─ packages/              # NuGet (gitignore)
```

## Include 규칙

- **단일 include 루트 = `src/`** (`AdditionalIncludeDirectories`에 `$(ProjectDir)src`).
- 같은 폴더 헤더는 그대로(`#include "Actor.h"`), **다른 폴더 헤더는 루트 기준 경로로 정규화**
  (예: `#include "Core/ConstantBuffer.h"`, `#include "GameFramework/Actor.h"`).

## 셰이더

- **빌드 시** `FxCompile`(SM 6.0)로 컴파일되어 `x64/<Config>/CompiledShaders/<Name>.h` 헤더로 생성됨.
  C++에서는 `#include "CompiledShaders/<Name>.h"` 후 `g_p<Name>` 바이트코드 사용.
- 셰이더 컴파일러의 include 경로도 `src/`로 설정되어 있어, `shaders/`의 HLSL이 `src/`의
  C++/HLSL 공용 구조체 헤더(`GlobalConstant.h`, `PBR/PBRHLSLCompat.h`, `SPH/Particle.h` 등)를 참조한다.
- 셰이더 파일명은 프로젝트 전역에서 **유일**해야 한다(헤더 출력이 이름만으로 평탄화됨).

## 새 시뮬레이션 모듈 추가하기

1. `src/<Sim>/`에 C++ (`<Sim>.h/.cpp`, 공용 구조체 헤더)를, `shaders/<Sim>/`에 HLSL을 둔다.
2. HLSL이 공용 구조체를 쓰면 `#include "GlobalConstant.h"`처럼 `src/` 루트 기준으로 include.
3. `SLab.vcxproj`에 파일 추가(VS에서 추가하면 자동). `.hlsl`은 `FxCompile` + `ShaderType`
   (`*VS`=Vertex, `*PS`=Pixel, `*GS`=Geometry, `*CS`=Compute) 지정.
4. `RenderEngine`/`Scene`에서 모듈을 연결.

> 참고: 튜토리얼(FlipWater)은 `d3dcompiler`로 **런타임 셰이더 컴파일**을 쓴다(수정→재실행이 빠름).
> 향후 반복 개발 편의를 위해 런타임 컴파일로 전환하는 것도 선택지다(렌더러 셰이더 로딩 재작성 필요).

---
*Derived from `SEngine`. 원본과 소스 내용은 동일하며, 폴더 구조 정리 + include 정규화 + VS2022 프로젝트 재생성만 수행되었다.*
