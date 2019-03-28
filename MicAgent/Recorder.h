//
// Created by jan on 19-1-28.
//

#ifndef IFLYOSCLIENTSDK_RECORDER_H
#define IFLYOSCLIENTSDK_RECORDER_H

#include <string>
#include <chrono>
#include <unordered_set>
#include <memory>
#include <mutex>

#include <IFLYOSThreadPool.h>
#include "snowboy-detect.h"
#include "tdmin_interface.h"

using CAE_HANDLE=void*;
static inline int CAENew(
        CAE_HANDLE*, const char*,
        void(*)(short angle, short channel, float power, short CMScore, short beam, void *userData),
        void(*)(const void *audioData, unsigned int audioLen, int param1, const void *param2, void *userData),
        void*, void*){
    return 0;
}

static inline void CAEDestroy(CAE_HANDLE){}
static int CAEAudioWrite(CAE_HANDLE, const void*, int){
    return 0;
}
static void CAEResetEng(CAE_HANDLE){}
static void CAESetRealBeam(CAE_HANDLE, short){}

namespace iflyos {
namespace agent {

class Recorder {
public:
    enum Format{
        NONE_OUTPUT,
        CAE_OUTPUT,
        ORIGIN_OUTPUT,
        SONIC_DECODE,
    };
    class AudioObserver {
    public:
        virtual void onAudio(const void *data, const void *data2, size_t bytesCount, Format format) = 0;
        virtual void onWakeWord(short angle, short CMScore, short beam) = 0;

        virtual ~AudioObserver() = default;
    };

    Recorder();
    void start();

    void stop();

    void setFormat(Format format);

    void addObserver(std::shared_ptr<AudioObserver> observer);

    virtual ~Recorder();
    static void fakeWakeUp(std::shared_ptr<Recorder> obj, int fakeWakeupFd);
private:

    static void cb_audio(const void *audioData, unsigned int audioLen, int param1, const void *param2, void *userData);
    static void cb_ivw(short angle, short channel, float power, short CMScore, short beam, void *userData);
    void readAudio(std::shared_ptr<snowboy::SnowboyDetect>);
    std::unordered_set<std::shared_ptr<AudioObserver>> m_observers;
    std::shared_ptr<iflyosInterface::ThreadPool> m_executor;
    std::mutex m_mutex;
    volatile bool m_quit;
    CAE_HANDLE m_cae;
    volatile Format m_format;

    PCMContainer m_pcmParams;
#define CH_NUM 1
#define FRAME_TIME 20//ms
#define SAMPLES_PER_MS 16
#define SAMPLE_BYTES 2

    uint32_t m_caeBuf[SAMPLES_PER_MS * FRAME_TIME * CH_NUM];
    float m_sonicBuf[SAMPLES_PER_MS * FRAME_TIME];
    float m_sonicBuf2[SAMPLES_PER_MS * FRAME_TIME];
};


}
}
#endif //IFLYOSCLIENTSDK_RECORDER_H
