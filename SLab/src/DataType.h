#pragma once

/**
 * 디스크립터 종류를 식별하기 위한 열거형.
 * 디스크립터 힙에 뷰를 생성할 때 어떤 종류의 뷰를 만들지 결정한다.
 */
enum DescriptorType
{
    RTV, // Render Target View
    UAV, // Unordered Access View
    SRV, // Shader Resource View
    DSV  // Depth Stencil View
};

enum class ViewDimensionType
{
    TEXTURE2D,
    TEXTURECUBE
};
