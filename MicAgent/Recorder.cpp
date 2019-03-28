//
// Created by jan on 19-1-28.
//

#include <iostream>
#include <fstream>
#include <memory>
#include "Recorder.h"

namespace iflyos {
namespace agent {

static std::ofstream* orgRecordFile = nullptr;
void Recorder::start() {
    printf("to start record with format:%d\n", (int)m_format);
    m_pcmParams.channels = 1;
    if(!orgRecordFile){
        if(std::getenv("ORG_RECORDER")){
            orgRecordFile = new std::ofstream("/data/org.pcm", std::ios_base::binary | std::ios_base::out);
        }
    }
    m_pcmParams.channels = 1;
    m_pcmParams.format = SND_PCM_FORMAT_S16;
    m_pcmParams.sample_rate = 16000;
    auto ret = alsa_tdmin_init(&m_pcmParams);
    if(ret){
        std::cout << "init alsa failed:" << ret << std::endl;
        return;
    }


    const std::string resource_filename = "common.res";
    const std::string model_filename = "snowboy.umdl";
    std::string sensitivity_str = "0.5";
    float audio_gain = 1;
    bool apply_frontend = false;

    // Initializes Snowboy detector.
    auto detector = std::make_shared<snowboy::SnowboyDetect>(resource_filename, model_filename);
    detector->SetSensitivity(sensitivity_str);
    detector->SetAudioGain(audio_gain);
    detector->ApplyFrontend(apply_frontend);
    std::cout << "snowboy sample rate:" << detector->SampleRate() << ",channels:" << detector->NumChannels() << std::endl;

    m_quit = false;
    m_executor = std::make_shared<iflyosInterface::ThreadPool>(1);
    m_executor->enqueue([this, detector]{
        readAudio(detector);
    });
}

void Recorder::stop() {
    printf("to stop record\n");
    m_quit = true;
    if(m_executor){
        m_executor->safeQuit();
        m_executor->join();
        m_executor.reset();
        m_executor = nullptr;
    }
    if(m_pcmParams.handle) {
        alsa_tdmin_uninit(&m_pcmParams);
        m_pcmParams.handle = nullptr;
    }
}

void Recorder::addObserver(std::shared_ptr<Recorder::AudioObserver> observer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.insert(observer);
}

void Recorder::readAudio(std::shared_ptr<snowboy::SnowboyDetect> detector) {
    if(m_quit)
        return;
    auto ret = alsa_read_tdmin_pcm(&m_pcmParams);
    if (ret) {
        printf("MicAgent read pcm failed:%d\n", ret);
        m_executor->enqueue([this,detector] { readAudio(detector); });
        return;
    }
    if (m_format != NONE_OUTPUT) {
        if (m_pcmParams.chunk_bytes != sizeof(m_caeBuf) / 2) {
            printf("ERROR:we fixed %d ms package\n", FRAME_TIME);
            m_executor->enqueue([this,detector] { readAudio(detector); });
            return;
        }
        auto detectResult = detector->RunDetection((const int16_t*)m_pcmParams.data_buf,  (m_pcmParams.chunk_bytes)/sizeof(int16_t));
        if (detectResult > 0) {
            std::cout << "detect result:" << detectResult << std::endl;
            for(auto observer : m_observers){
                observer->onWakeWord(0, 0, 0);
            }
        }

        if(m_format == CAE_OUTPUT) {
            const auto inBuf = (short *) m_pcmParams.data_buf;
            for (unsigned int i = 0; i < FRAME_TIME * SAMPLES_PER_MS; i++) {
                int baseIdx = i * CH_NUM;
                for (int n = 0; n < CH_NUM; ++n) {
                    char *out2 = (char *) (&m_caeBuf[baseIdx + n]);
                    char *s16 = (char *) (&inBuf[baseIdx + n]);
                    out2[2] = s16[0];
                    out2[3] = s16[1];
                }
            }

            auto bufPtr = (const char *) m_caeBuf;
            auto wr = CAEAudioWrite(m_cae, bufPtr, sizeof(m_caeBuf) / 2);
            if (wr) {
                printf("write audio to cae failed:%d\n", wr);
            }
            bufPtr += sizeof(m_caeBuf) / 2;
            wr = CAEAudioWrite(m_cae, bufPtr, sizeof(m_caeBuf) / 2);
            if (wr) {
                printf("write audio to cae failed:%d\n", wr);
            }
        }else if(m_format == ORIGIN_OUTPUT){
            {
                //std::lock_guard<std::mutex> lock(m_mutex);
                for(auto observer : m_observers){
                    observer->onAudio((const void*)m_pcmParams.data_buf, nullptr, m_pcmParams.chunk_bytes, ORIGIN_OUTPUT);
                }
            }
        }else if(m_format == SONIC_DECODE) {
            for(auto i = 0u; i < sizeof(m_sonicBuf) / sizeof(m_sonicBuf[0]); i++){
                m_sonicBuf[i] = (float)((short*)m_pcmParams.data_buf)[i * CH_NUM]/(1<<15);
            }
            for(auto i = 0u; i < sizeof(m_sonicBuf) / sizeof(m_sonicBuf[0]); i++){
                m_sonicBuf2[i] = (float)((short*)m_pcmParams.data_buf)[i * CH_NUM + 2]/(1<<15);
            }
            {
                //std::lock_guard<std::mutex> lock(m_mutex);
                for(auto observer : m_observers){
                    observer->onAudio((const void*)m_sonicBuf, (const void*)m_sonicBuf2, sizeof(m_sonicBuf), SONIC_DECODE);
                }
            }
        }
    }
    m_executor->enqueue([this,detector]{readAudio(detector);});
}

void Recorder::cb_ivw(short angle, short channel, float power, short CMScore, short beam, void *userData) {
    auto thisObj = (Recorder*)userData;
    CAEResetEng(thisObj->m_cae);
    CAESetRealBeam(thisObj->m_cae, beam);
    {
        //std::lock_guard<std::mutex> lock(thisObj->m_mutex);
        for(auto observer : thisObj->m_observers){
            observer->onWakeWord(angle, CMScore, beam);
        }

    }
    if(thisObj->m_format != CAE_OUTPUT){
        return;
    }
}

void Recorder::cb_audio(const void *audioData, unsigned int audioLen, int param1, const void *param2, void *userData) {
    auto thisObj = (Recorder*)userData;
    if(thisObj->m_format != CAE_OUTPUT){
        return;
    }
    if(orgRecordFile){
        orgRecordFile->write((const char*)audioData, 256);
        orgRecordFile->flush();
    }
    {
        //std::lock_guard<std::mutex> lock(thisObj->m_mutex);
        for(auto observer : thisObj->m_observers){
            observer->onAudio(audioData, nullptr, 256, CAE_OUTPUT);
        }
    }
}

static std::thread* t_stdin_reader = nullptr;
Recorder::Recorder() : m_executor{nullptr}, m_format{Format::NONE_OUTPUT}{
    m_pcmParams.handle = nullptr;
    for(unsigned int i = 0; i < FRAME_TIME * SAMPLES_PER_MS; i++) {
        int baseIdx = i * CH_NUM;
        m_caeBuf[baseIdx + 0] = 0x00000100;
        m_caeBuf[baseIdx + 1] = 0x00000200;
        m_caeBuf[baseIdx + 2] = 0x00000300;
        m_caeBuf[baseIdx + 3] = 0x00000400;
    }
}

void Recorder::setFormat(Recorder::Format format) {
    if(m_format != format) {
        if(format == CAE_OUTPUT) {
            auto ret = CAENew(&m_cae, "/home/pi/ivs/a113.jet", cb_ivw, cb_audio, nullptr, (void *) this);
            if (ret) {
                std::cout << "cae init fail:" << ret << std::endl;
            }
            CAEResetEng(m_cae);
        } else if(m_cae){
            CAEDestroy(m_cae);
            m_cae = nullptr;
        }
        m_format = format;
    }
}

Recorder::~Recorder() {
    stop();
    if(m_cae) {
        CAEDestroy(m_cae);
    }
}

void Recorder::fakeWakeUp(std::shared_ptr<Recorder> obj, int fakeWakeupFd) {
    //NOT THREAD SAFE
    if(!t_stdin_reader && fakeWakeupFd >= 0){
        t_stdin_reader = new std::thread([obj, fakeWakeupFd]{
            char b[512];
            while(true){
                auto rd = read(0, b, sizeof(b));
                if(rd <= 0)
                    break;
                if(b[0] == ' '){
                    for(auto observer : obj->m_observers){
                        observer->onWakeWord(0, 10, 0);
                    }
                }else {
                    write(fakeWakeupFd, b, 1);
                }
            }
        });
    }
}

}
}
