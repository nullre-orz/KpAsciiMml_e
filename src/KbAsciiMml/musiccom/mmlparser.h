#pragma once

#include <memory>

namespace MusicCom
{
    class MusicData;
    std::unique_ptr<MusicData> ParseMML(const char* filename);

} // namespace MusicCom
