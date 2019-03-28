//
// Created by jan on 19-1-28.
//

#ifndef IFLYOSCLIENTSDK_PCMPLAYER_H
#define IFLYOSCLIENTSDK_PCMPLAYER_H

#include <vector>
#include <string>
#include <map>

#include "portaudio.h"
#include "IFLYOSThreadPool.h"

namespace iflyos {
namespace agent {

class PCMPlayer {
public:
    PCMPlayer(std::vector<std::string> filesToLoad);
    void play(const std::string& file, const int vol = 100);
    void stop();

    ~PCMPlayer();

private:
    static int paStreamCallback(const void* input, void* output, unsigned long frameCount,
                                const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);
    static void PaStreamFinishedCallback(void *userData);

    PaStream* m_paStream;
    iflyosInterface::ThreadPool m_executor;
    std::map<std::string, std::vector<char>> m_mapBuffer;
    std::vector<char>* m_currentPlaying;
    size_t m_currentPlayingSampleOffset;
    int m_vol;
};

}
}

#endif //IFLYOSCLIENTSDK_PCMPLAYER_H
