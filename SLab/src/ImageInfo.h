#pragma once

struct ImageInfo
{
    UINT width;
    UINT height;
    UINT64 rowSize;
    UINT numRows;
    UINT64 rowPitch;
    std::string resultPath;
};
