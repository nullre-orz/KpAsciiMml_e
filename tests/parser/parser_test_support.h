#pragma once

#include "../test_support.h"
#include "../../src/KbAsciiMml/musiccom/command.h"
#include <array>

inline void CheckCommand(const MusicCom::Command& command, MusicCom::CommandType type,
                         std::array<int, 3> arguments = {})
{
    Check(command.GetType() == type, "incorrect command type");
    Check(command.GetArg(0) == arguments[0] && command.GetArg(1) == arguments[1] && command.GetArg(2) == arguments[2],
          "incorrect command arguments");
}

inline void CheckChannelCommand(const MusicCom::MusicData& music, int channel, MusicCom::CommandType type,
                                std::array<int, 3> arguments = {})
{
    Check(music.IsChannelPresent(channel), "channel data was not stored");
    CheckCommand(*music.GetChannelHead(channel), type, arguments);
}
