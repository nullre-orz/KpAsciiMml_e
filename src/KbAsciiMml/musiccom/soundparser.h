#pragma once

#include <string>

namespace MusicCom
{
    class SoundData;
    SoundData* ParseSound(const std::string& mml_filename);

} // namespace MusicCom
