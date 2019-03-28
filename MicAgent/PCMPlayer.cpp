//
// Created by jan on 19-1-28.
//

#include <iostream>
#include <cstring>
#include <fstream>
#include "PCMPlayer.h"

namespace iflyos {
namespace agent {

int PCMPlayer::paStreamCallback(const void* input, void* output, unsigned long frameCount,
                     const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
    auto thiz = (PCMPlayer*)userData;
    auto dataSrcPtr = (short*)thiz->m_currentPlaying->data();
    dataSrcPtr += thiz->m_currentPlayingSampleOffset;
    auto dataDestPtr = (short*)output;
    auto remaining = (thiz->m_currentPlaying->size() / 2) - thiz->m_currentPlayingSampleOffset;
    if(remaining > frameCount){
        if(thiz->m_vol == 100) {
            memcpy(output, (const void *) dataSrcPtr, frameCount * 2);
        }else {
            for (auto i = 0u; i < frameCount; i++) {
                dataDestPtr[i] = (short) ((float) dataSrcPtr[i] * thiz->m_vol / 100);
            }
        }
        thiz->m_currentPlayingSampleOffset += frameCount;
        return paContinue;
    }

    if(thiz->m_vol == 100) {
        memcpy(output, (const void *) dataSrcPtr, remaining * 2);
        memset((void*)&dataDestPtr[remaining], 0, (frameCount - remaining)*2);
    }else {
        for (auto i = 0u; i < remaining; i++) {
            dataDestPtr[i] = (short) ((float) dataSrcPtr[i] * thiz->m_vol / 100);
        }
        for (auto i = remaining; i < frameCount; i++) {
            dataDestPtr[i] = 0;
        }
    }
    return paComplete;
}

void PCMPlayer::PaStreamFinishedCallback(void *userData) {
    auto thiz = (PCMPlayer*)userData;
    thiz->m_executor.enqueue([thiz]{
        if(thiz->m_paStream) {
            Pa_StopStream(thiz->m_paStream);
        }
    });
}

PCMPlayer::PCMPlayer(std::vector<std::string> filesToLoad)
        : m_paStream{nullptr}, m_executor{1}, m_currentPlaying{nullptr}, m_currentPlayingSampleOffset{0}, m_vol{100} {
    for(auto& file : filesToLoad){
        std::ifstream ifs(file, std::ios_base::in | std::ios_base::binary);
        if(!ifs.is_open()){
            printf("ifstream(%s) open failed:%d\n", file.c_str(), (int)errno);
            return;
        }
        ifs.seekg(0,std::ios_base::end);
        int size = ifs.tellg();
        size = (size + 1) / 2 * 2;
        auto cache = std::vector<char>(size / 2);
        ifs.seekg(0, std::ios_base::beg);
        size_t offset = 0;
        auto dataPtr = cache.data();
        while(offset < cache.size()){
            dataPtr += offset;
            ifs.read(dataPtr, cache.size() - offset);
            auto rd = ifs.gcount();
            if(rd <= 0)
                break;
            offset += rd;
        }
        printf("cached %d bytes of file:%s\n", (int)cache.size(), file.c_str());
        m_mapBuffer[std::string(file)] = cache;
    }

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        printf("Pa_Initialize() failed:%d\n", err);
        return;
    }

    err = Pa_OpenDefaultStream(&m_paStream, 0, 1,
                               paInt16, 48000, paFramesPerBufferUnspecified, paStreamCallback, this);
    if (err != paNoError || !m_paStream)
    {
        printf("Pa_OpenDefaultStream() failed:%d\n", err);
        Pa_Terminate();
        return;
    }
    Pa_SetStreamFinishedCallback(m_paStream, PaStreamFinishedCallback);

}

void PCMPlayer::play(const std::string &file, const int vol) {
    auto found = m_mapBuffer.find(file);
    if(found == m_mapBuffer.end()){
        std::cout << "can not find cached file:" << file << std::endl;
        return;
    }
    //stop();
    m_executor.enqueue([this,file, vol]{
        m_currentPlaying = &m_mapBuffer[file];
        m_currentPlayingSampleOffset = 0;
        m_vol = vol < 0 ? 0 : (vol > 100 ? 100: vol);
        PaError err = Pa_StartStream(m_paStream);
        if (err != paNoError) {
            printf("Pa_StartStream() failed:%d\n", err);
            return;
        }
    });
}

void PCMPlayer::stop() {
    m_executor.enqueue([this]{
        if (m_paStream && Pa_IsStreamStopped(m_paStream) != 1) {
            Pa_StopStream(m_paStream);//after return all audio write callback should finish, so no rare condition
        }
    });
}

PCMPlayer::~PCMPlayer() {
    if(m_paStream){
        Pa_AbortStream(m_paStream);
        Pa_CloseStream(m_paStream);
        Pa_Terminate();
    }
    m_executor.safeQuit();
    m_executor.join();
}


}
}