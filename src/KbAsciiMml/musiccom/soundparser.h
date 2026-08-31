#pragma once

#include <memory>
#include <string>

namespace MusicCom
{
    class SoundData;
    std::unique_ptr<SoundData> ParseSound(const std::string& mml_filename);

} // namespace MusicCom
