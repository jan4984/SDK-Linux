//
// Created by jan on 19-1-28.
//

#include "Recorder.h"
#include "NetworkSetter.h"
//#include "sonic.h"
#include "NetworkMonitor.h"
#include <stdint.h>
#include <string.h>
#include <sstream>
#include <iostream>

extern "C"{
static const char *quiet_profiles = "/home/pi/ivs/quiet-profiles.json";
static const char *quiet_profile_name = "audible";

int sonic_receiver_create(struct sonic_receiver_t *rec) {
//    quiet_decoder_options *opt = quiet_decoder_profile_filename(
//            quiet_profiles, quiet_profile_name);
//
//    rec->dec = quiet_decoder_create(opt, 16000);

    return 0;
}

char *sonic_receiver_consume(struct sonic_receiver_t *rec, float *buf, size_t len) {
//    uint8_t data[1024];
//    ssize_t received;
//
//    quiet_decoder_consume(rec->dec, buf, len);
//
//    received = quiet_decoder_recv(rec->dec, data, len);
//    if (received < 0) {
//        received = 0;
//    }
//
//    data[received] = '\0';
//
//    return strdup((char *) data);
    return nullptr;
}

void sonic_receiver_destroy(struct sonic_receiver_t *rec) {
//    quiet_decoder_destroy(rec->dec);
}

}

namespace iflyos {
namespace agent {


class SonicAudioObserver : public iflyos::agent::Recorder::AudioObserver {
public:
    SonicAudioObserver(std::function<void(const std::string& ssid, const std::string& pass)> h) : m_h{h}{
        //m_receiver_mic = (struct sonic_receiver_t *)malloc(sizeof(struct sonic_receiver_t));;
        //m_receiver_ref = (struct sonic_receiver_t *)malloc(sizeof(struct sonic_receiver_t));;
        //sonic_receiver_create(m_receiver_mic);
        //sonic_receiver_create(m_receiver_ref);
    }

    ~SonicAudioObserver(){
        //sonic_receiver_destroy(m_receiver_mic);
        //sonic_receiver_destroy(m_receiver_ref);
        //free(m_receiver_mic);
        //free(m_receiver_ref);
    }
    void onAudio(const void *data, const void* data2, size_t bytesCount, iflyos::agent::Recorder::Format format){
        if(format != iflyos::agent::Recorder::Format::SONIC_DECODE) return;
        if(done) return;
        const char *result;

        std::stringstream sstream;
        std::string flag;
        std::string ssid;
        std::string pass;

        printf("to try %d bytes to sonic decoder\n", (int)bytesCount);
        if (strlen(result = sonic_receiver_consume(m_receiver_mic, (float*)data, bytesCount / sizeof(float))) != 0
        || strlen(result = sonic_receiver_consume(m_receiver_ref, (float*)data2, bytesCount / sizeof(float))) != 0) {
            std::cout << "got network configuration:" << result << std::endl;
            sstream.str(result);
            sstream >> flag >> ssid >> pass;
            m_h(ssid, pass);
            needAuth = flag == "1";
            done = true;
        }
    }
    void onWakeWord(short angle, short CMScore, short beam) {

    }

    volatile bool done = false;
    volatile bool needAuth = true;

private:
    struct sonic_receiver_t * m_receiver_mic;
    struct sonic_receiver_t * m_receiver_ref;
    std::function<void(const std::string& ssid, const std::string& pass)> m_h;
};



void NetworkSetter::start(bool& needAuth, bool& success, const int timeout) {
    //auto clock = std::chrono::steady_clock();
    //auto startAt = clock.now();
    auto oneTry = []{
        auto observer = std::make_shared<SonicAudioObserver>([](const std::string& ssid, const std::string& pass){
            std::system("/home/pi/ivs/SampleAppCtrl.sh play /home/pi/ivs/sound_effect/start_config_wifi.mp3 0.5");
            std::system(("/home/pi/ivs/SampleAppCtrl.sh configwifi " + ssid + " " + pass + " &").c_str());
        });

        Recorder recorder;
        recorder.setFormat(Recorder::SONIC_DECODE);
        recorder.addObserver(observer);
        recorder.start();

        while(!observer->done){
            sleep(2);
        }

        return observer->needAuth;
    };

    //while(clock.now() - startAt < std::chrono::seconds(timeout)) {
        needAuth = oneTry();
        //startAt = clock.now();//the timeout is for network check, not for sonic decode
        printf("to sleep 20s\n");
        sleep(20);
        if(NetworkMonitor::trySetTimeOneTime( std::chrono::seconds(10))){
            success = true;
            return;
        }
        printf("to sleep 10s\n");
        sleep(10);
        if(NetworkMonitor::trySetTimeOneTime(std::chrono::seconds(10))){
            success = true;
            return;
        }
    //}
    success = false;
}

}
}
