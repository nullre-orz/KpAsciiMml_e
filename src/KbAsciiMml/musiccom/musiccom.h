#pragma once

#include <fmgen/opna.h>
#include <memory>

namespace MusicCom
{
    class Sequencer;
    class MusicData;
    class SoundData;

    class MusicCom
    {
    public:
        MusicCom();
        ~MusicCom();
        bool Load(const char* filename);
        bool PrepareMix(uint rate);
        void Mix(FM_SAMPLETYPE* dest, int nsamples);
        void SetFMVolume(int vol);
        void SetPSGVolume(int vol);
        void SetNoiseAdjustment(bool on);

    protected:
        // non-copyable
        MusicCom(const MusicCom&) = delete;
        MusicCom& operator=(const MusicCom&) = delete;

    private:
        FM::OPN opn;
        std::unique_ptr<Sequencer> pseq;
        std::unique_ptr<MusicData> pmusicdata;
        std::unique_ptr<SoundData> psounddata;
        int fmVolume;
        int psgVolume;
        bool adjustment;
    };

} // namespace MusicCom
