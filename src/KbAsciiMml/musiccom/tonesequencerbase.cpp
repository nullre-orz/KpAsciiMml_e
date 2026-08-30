#include "tonesequencerbase.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace MusicCom
{
    const int TONE_KEY_OFF = -1;

    void ToneSequencerBase::ProcessCommandFrame(int current_frame)
    {
        PartSequencerBase::ProcessCommandFrame(current_frame);
        ProcessEffect(current_frame);
    }

    CommandIterator ToneSequencerBase::ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data)
    {
        auto update_keyoff_frames = [this](PartData& part_data, CommandIterator ptr, int current_frame, int length)
        {
            part_data.Commands.LinkedItem = FindLinkedItem(CommandIterator(ptr));
            if (ptr->GetType() == CommandType::TYPE_TIE)
            {
                part_data.Note.KeyOffFrame = part_data.Note.EndFrame;
            }
            else
            {
                if (part_data.Note.GateTime >= 8)
                {
                    // Q8以上では途中キーオフを無効化する
                    part_data.Note.KeyOffFrame = std::numeric_limits<int>::max();
                }
                else
                {
                    // 64分音符単位でゲート時間を計算する
                    part_data.Note.KeyOffFrame = current_frame + (length - 1) * part_data.Note.GateTime / 8 + 1;
                }
            }
        };

        const Command& command = *ptr++;
        switch (command.GetType())
        {
        case CommandType::TYPE_NOTE:
        {
            if (!part_data.Commands.LinkedItem)
            {
                if (part_data.Playback.HasPreviousNote)
                {
                    KeyOff();
                }
                part_data.Note.KeyOnFrame = current_frame;
            }

            // オクターブの変更を反映
            part_data.Note.Octave = part_data.Note.ReservedOctave;

            UpdateTone(command.GetArg(0), part_data);
            if (part_data.Commands.LinkedItem != CommandType::TYPE_TIE)
            {
                KeyOn();
            }
            part_data.Playback.HasPreviousNote = true;

            if (part_data.Note.LastTone == TONE_KEY_OFF)
            {
                part_data.Note.LastOctave = part_data.Note.Octave;
                part_data.Note.LastTone = part_data.Note.Tone;
            }

            int length = ResolveNoteLength(part_data, command.GetArg(1));
            UpdateNoteFrames(part_data, current_frame, length);
            update_keyoff_frames(part_data, ptr, current_frame, length);

            break;
        }
        case CommandType::TYPE_REST:
        {
            part_data.Playback.HasPreviousNote = true;
            int length = ResolveNoteLength(part_data, command.GetArg(0));
            UpdateNoteFrames(part_data, current_frame, length);

            if (part_data.Commands.LinkedItem != CommandType::TYPE_TIE)
            {
                // &R でなければキーオフ
                KeyOff();

                // 休符後のトーンエフェクトを無効にするためキーオフ状態を設定
                part_data.Note.Tone = TONE_KEY_OFF;
            }
            else
            {
                update_keyoff_frames(part_data, ptr, current_frame, length);
            }

            break;
        }
        case CommandType::TYPE_WAIT:
        {
            auto linked_item = part_data.Commands.LinkedItem;
            int length = ResolveNoteLength(part_data, command.GetArg(0));
            UpdateNoteFrames(part_data, current_frame, length);
            update_keyoff_frames(part_data, ptr, current_frame, length);

            // W開始前のタイはWを越えて維持する
            if (linked_item == CommandType::TYPE_TIE)
            {
                part_data.Commands.LinkedItem = linked_item;
            }

            break;
        }
        case CommandType::TYPE_LENGTH:
            part_data.Note.DefaultLength = command.GetArg(0);
            break;
        case CommandType::TYPE_OCTAVE:
            part_data.Note.ReservedOctave = std::clamp(command.GetArg(0), 0, 8);
            break;
        case CommandType::TYPE_OCTAVE_DOWN:
            part_data.Note.ReservedOctave = std::max(part_data.Note.ReservedOctave - 1, 0);
            break;
        case CommandType::TYPE_OCTAVE_UP:
            part_data.Note.ReservedOctave = std::min(part_data.Note.ReservedOctave + 1, 8);
            break;
        case CommandType::TYPE_VOLUME:
            part_data.Note.Volume = command.GetArg(0);
            SetVolume(std::clamp(part_data.Note.Volume, 0, 15));
            break;
        //case CommandType::TYPE_TONE:
        case CommandType::TYPE_GATE_TIME:
            part_data.Note.GateTime = command.GetArg(0);
            break;
        case CommandType::TYPE_DETUNE:
            part_data.Note.Detune = command.GetArg(0);
            break;
        case CommandType::TYPE_PORTAMENTO:
            part_data.Effect.PortamentoLength = std::max(command.GetArg(0), 0);
            // Iのみ排他
            part_data.Effect.VibratoDepth = 0;
            break;
        case CommandType::TYPE_TREMOLO:
            part_data.Effect.TremoloDepth = command.GetArg(0);
            part_data.Effect.TremoloLength = std::max(command.GetArg(1), 1);
            part_data.Effect.TremoloDelay = std::max(command.GetArg(2), 0);
            break;
        case CommandType::TYPE_VIBRATO:
            part_data.Effect.VibratoDepth = command.GetArg(0);
            part_data.Effect.VibratoLength = std::max(command.GetArg(1), 1);
            part_data.Effect.VibratoDelay = std::max(command.GetArg(2), 0);
            // Pのみ排他
            part_data.Effect.PortamentoLength = 0;
            break;
        //case CommandType::TYPE_ENV_FORM:
        //case CommandType::TYPE_ENV_PERIOD:
        case CommandType::TYPE_DIRECT:
            register_writer_(command.GetArg(0), command.GetArg(1));
            break;
        }

        return ptr;
    }

    void ToneSequencerBase::ProcessEffect(int current_frame)
    {
        // エフェクト等の処理
        // ディレイはKeyOnFrameを基準とする

        // ビブラート用サブ関数
        auto set_vibrato = [this](int keyon_length, int base_tone)
        {
            auto part_data = part_data_;
            int depth = (((keyon_length - part_data.Effect.VibratoDelay) / part_data.Effect.VibratoLength) & 1) ? -part_data.Effect.VibratoDepth : part_data.Effect.VibratoDepth;
            ApplyVibratoEffect(part_data.Note.Octave, base_tone, depth);
        };

        // 一時停止中の場合は何もしない
        if (!part_data_.Playback.Playing)
        {
            return;
        }

        // ゲートタイム(Q)でのキーオフ
        if (part_data_.Note.KeyOffFrame <= current_frame)
        {
            KeyOff();
        }

        ProcessVolumeEffect(current_frame);

        int keyon_length = current_frame - part_data_.Note.KeyOnFrame;

        // Tone
        if (part_data_.Effect.PortamentoLength != 0 && part_data_.Note.Tone != TONE_KEY_OFF)
        {
            // ポルタメントは音程が変わった時点で適用する。
            // P+1を分母として整数差分を求め、P+1 tick目に目標音程へ切り替える。
            int tick = current_frame - part_data_.Note.BeginFrame;
            if (tick <= part_data_.Effect.PortamentoLength + 1)
            {
                ApplyPortamentoEffect(part_data_.Note.Octave, part_data_.Note.Tone, part_data_.Note.LastOctave, part_data_.Note.LastTone, tick, part_data_.Effect.PortamentoLength);
            }
        }
        else if (part_data_.Effect.VibratoDepth != 0 && keyon_length >= part_data_.Effect.VibratoDelay)
        {
            if (part_data_.Note.Tone != TONE_KEY_OFF)
            {
                set_vibrato(keyon_length, part_data_.Note.Tone);
            }
            else if (part_data_.Note.LastTone != TONE_KEY_OFF)
            {
                set_vibrato(keyon_length, part_data_.Note.LastTone);
                part_data_.Note.LastTone = TONE_KEY_OFF;
            }
        }
    }

    void ToneSequencerBase::ProcessVolumeEffect(int current_frame)
    {
        if (!NeedsVolumeEffect(part_data_))
        {
            return;
        }

        int keyon_length = current_frame - part_data_.Note.KeyOnFrame;
        int final_volume = part_data_.Note.Volume;

        if (part_data_.Effect.TremoloDepth != 0 && keyon_length >= part_data_.Effect.TremoloDelay)
        {
            if (((keyon_length - part_data_.Effect.TremoloDelay) / part_data_.Effect.TremoloLength) & 1)
                final_volume -= part_data_.Effect.TremoloDepth;
        }

        final_volume = AdjustVolumeEffect(final_volume, keyon_length, part_data_);
        SetVolume(std::clamp(final_volume, 0, 15));
    }

    int ToneSequencerBase::AdjustVolumeEffect(int volume, int, const PartData&)
    {
        return volume;
    }

    void ToneSequencerBase::ApplyVibratoEffect(int octave, int tone, int depth)
    {
        int adjusted_tone = static_cast<int>(tone * pow(2.0, depth / (255.0 * 12.0)) + 0.5);
        SetTone(octave, adjusted_tone);
    }

} // namespace MusicCom
