
//
// Created by jan on 18-11-29.
//

#include "linux/input.h"

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include <sys/stat.h>
#include "IFLYOSInterface.h"
#include "IFLYOSThreadPool.h"
#include "NetworkMonitor.h"
#include "Recorder.h"
#include "NetworkSetter.h"
#include "syskey.h"
#include "sonic.h"
#include "PCMPlayer.h"

#define FIFO_PATH "/tmp/iflyos_inputaudio"
#define TEST_RECORD_PATH "/tmp/iflyos_audio_record.pcm"

using namespace iflyosInterface;
static volatile bool forwarding = false;
static volatile int fdAudio = -1;
static ThreadPool *executor = nullptr;

static volatile bool quit = false;
static volatile bool hasNetwork = false;
static volatile auto micOn = true;
static const char NET_CONFIG[1] = {'\1'};
static const char NO_NET_CONFIG[1] = {'\0'};
static int restartChannel = -1;
static auto recorder = std::make_shared<iflyos::agent::Recorder>();
static volatile bool inThinking = false;

enum IFLYOSReadyState{
    NOT_START,
    STARTED
};
static volatile IFLYOSReadyState firstDialogState = IFLYOSReadyState::NOT_START;
static iflyos::agent::PCMPlayer* pcmPlayer;

static void setNetwork(const int timeoutSeconds){
    printf("need re-auth, remove DBs\n");
    std::system("rm -rf /home/pi/ivs/*.db");
}

//设备开始说话
static void onSpeaking(const char*, const char*){
    inThinking = false;
}

//设备识别完成,开始等待语义处理结果
static void onThinking(const char*, const char*){
    inThinking = true;
}

//进入IDLE状态
static void stopCapture(const char * /*evt*/, const char * /*msg*/) {
    inThinking = false;
    printf("stopCapture:%d\n", (int)firstDialogState);
    //如果是第一次报告IDLE状态,则播报欢迎语
    if(firstDialogState == IFLYOSReadyState::NOT_START){
        std::system("/home/pi/ivs/SampleAppCtrl.sh play /home/pi/ivs/sound_effect/connected.mp3 0.5");
        firstDialogState = IFLYOSReadyState::STARTED;
        return;
    }

    std::system("echo 3 > /home/pi/ivs/ledfile");
    executor->enqueue([] {
        if (!forwarding) return;
        printf("to close mic fifo\n");
        forwarding = false;
        close(fdAudio);
        fdAudio = -1;
    });
}

//IFLYOS退出
static void iflyosAppExited(const char * /*evt*/, const char * /*msg*/) {
    executor->enqueue([] {
        if (!forwarding) return;
        printf("to close mic fifo\n");
        forwarding = false;
        close(fdAudio);
        fdAudio = -1;
    });
}

//音量已改变,播放音量声效
static void volChanged(const char* /*evt*/, const char* msg){
    std::vector<std::string> lines;
    iflyosInterface::IFLYOS_InterfaceGetLines(msg, lines);
    if(lines.size() != 2){
        printf("volChanged need 2 line but %s\n", msg);
        return;
    }
    auto vol = std::atoi(lines[0].c_str());
    pcmPlayer->play("/home/pi/ivs/sound_effect/plastic-soda-pop.wav", vol);
}

//需要重新授权
static void revokeAuth(const char* /*evt*/, const char* /*msg*/){
    printf("revokeAuth\n");
    //模拟用户按键进入配网
    write(restartChannel, NO_NET_CONFIG, 1);
}

//token过期,需要重新授权
static void tokenExpired(const char* /*evt*/, const char* /*msg*/){
    std::system("/home/pi/ivs/SampleAppCtrl.sh play /home/pi/ivs/sound_effect/expired.mp3 0.5");
    printf("token expired, to exit iflyosapp\n");
    firstDialogState = IFLYOSReadyState::STARTED;
    sleep(1);
    //模拟用户按键进入配网
    write(restartChannel, NET_CONFIG, 1);
}

#define LEADING_PCM "/home/pi/ivs/leading.pcm"
#define NUMBERS_PCM "/home/pi/ivs/numbers.pcm"
#define TOGETHER_PCM "/tmp/authTip.wav"

#define DIGIT_LEN 16000
#define BUFFER_LEN (DIGIT_LEN * 11)
//播放授权码
size_t sonic_speak(const char *digits) {
    FILE *fd;
    size_t count = 0;

    uint8_t *leading_buf = (uint8_t *) malloc(BUFFER_LEN);
    size_t leading_len;

    uint8_t *numbers_buf = (uint8_t *) malloc(BUFFER_LEN);
    //size_t numbers_len;

    fd = fopen(LEADING_PCM, "rb");
    leading_len = fread(leading_buf, sizeof(uint8_t), BUFFER_LEN, fd);
    fclose(fd);

    fd = fopen(NUMBERS_PCM, "rb");
    fread(numbers_buf, sizeof(uint8_t), BUFFER_LEN, fd);
    fclose(fd);

    //memcpy(out, leading_buf, leading_len);
    std::ofstream out(TOGETHER_PCM, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    out.write((const char*)leading_buf, leading_len);

    count += leading_len;

    for (size_t i = 0; i < strlen(digits); i++) {
        if (digits[i] < '0' || digits[i] > '9') {
            continue;
        }

        //memcpy(out + count, numbers_buf + (digits[i] - '0') * DIGIT_LEN, DIGIT_LEN);
        out.write((const char*)(numbers_buf + (digits[i] - '0') * DIGIT_LEN), DIGIT_LEN);
        //printf("write digit %c\n", digits[i]);
        count += DIGIT_LEN;
    }

    free(leading_buf);
    free(numbers_buf);

    return count;
}

static std::thread authCodePlayer;
static volatile auto stopAuthCodePlayer = false;
//播放授权码一直等授权结果
static void playAuthCode(const char* /*evt*/, const char * msg){
    sonic_speak(msg);
    stopAuthCodePlayer = false;
    authCodePlayer = std::thread([]{
        do{
            std::system("/home/pi/ivs/SampleAppCtrl.sh play " TOGETHER_PCM "&");
            sleep(10);
        }while(!stopAuthCodePlayer);
    });
    authCodePlayer.detach();
}

//授权结果OK
static void authOK(const char* /*evt*/, const char * msg){
    stopAuthCodePlayer = true;
}

//授权结果失败
static void authError(const char* /*evt*/, const char * msg){
    printf("auto error\n");
    stopAuthCodePlayer = true;
    iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_STOP);
    std::system("/home/pi/ivs/SampleAppCtrl.sh play /home/pi/ivs/sound_effect/config_wifi_fail.mp3 0.5");
    write(restartChannel, NET_CONFIG, 1);
}


//如果断网就退出IFLYOS;如果联网就启动IFLYOS
class NetworkStatusMonitor : public iflyos::agent::NetworkMonitor::NetworkStatusObserver{
public:
    void onNetworkStatusChanged(iflyos::agent::NetworkMonitor::NetworkStatus status) override{
        hasNetwork = status == iflyos::agent::NetworkMonitor::NetworkStatus::HTTPS_OK;
        if(status == iflyos::agent::NetworkMonitor::NetworkStatus::NETWORK_FAIL){
            printf("no network, to stop IVS\n");
            iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_STOP);
        }else{
            printf("network ok, to start IVS\n");
            sleep(2);
            iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_START);
        }
    }
    ~NetworkStatusMonitor() = default;
};

class OrgAudioObserver : public iflyos::agent::Recorder::AudioObserver {
private:
    long debugACC = 0;

public:
    //录音数据
    void onAudio(const void *data, const void *data2, size_t bytesCount, iflyos::agent::Recorder::Format format) {
        debugACC += bytesCount;
        if(debugACC % (2048 * 4)== 0) {
            printf("debug: got %d cae output\n", 2048 * 4);
        }
        if(forwarding && format == iflyos::agent::Recorder::Format::ORIGIN_OUTPUT){
            //写入管道
            auto wr = write(fdAudio, (const char *) data, bytesCount);
            if (wr != (ssize_t )bytesCount) {
                printf("short write audio to fifo :%zd, %d\n", wr, (int) errno);
            }
        }
    }

    //唤醒
    void onWakeWord(short angle, short CMScore, short beam) {
        if(firstDialogState == IFLYOSReadyState::NOT_START){
            printf("ignore wakeup while iflyos not started\n");
            return;
        }
        if(!micOn){
            printf("ignore wakeup while mic off\n");
            return;
        }
        //如果没有网络,唤醒后播报断网提示音
        if(!hasNetwork){
            std::system("/home/pi/ivs/SampleAppCtrl.sh play /home/pi/ivs/sound_effect/disconnected.mp3 0.5 &");
            return;
        }

        if(forwarding){
            //如果在Thinking状态,忽略唤醒请求
            if(inThinking) {
                printf("ignore wakeup while in thinking");
                return;
            }
            //如果在非Thinking的交互状态,因为录音已经在传输,仅仅播放唤醒提示音并告诉IFLYOS开始新的对话
            executor->enqueue([] {
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_DIALOG_START, FIFO_PATH);
                std::system("echo 0 > /home/pi/ivs/ledfile");
                //disable it because our demo for customer not include AEC
                //pcmPlayer->play("/home/pi/ivs/sound_effect/here.wav");
            });
        }else {
            //开始录音传输
            forwarding = true;
            executor->enqueue([] {
                std::cout << "start forward audio to FIFO" << std::endl;
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_DIALOG_START, FIFO_PATH);
                std::system("echo 0 > /home/pi/ivs/ledfile");
                pcmPlayer->play("/home/pi/ivs/sound_effect/here.wav");
                fdAudio = open(FIFO_PATH, O_WRONLY);
                if (fdAudio < 0) {
                    forwarding = false;
                    std::cout << "open fifo audio file failed" << strerror(errno) << std::endl;
                    return;
                }
            });
        }
    }
};

#if true
static void sigHandler(int sig) {
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    recorder->stop();
    exit(1);

}
#endif


int main(int argc, char *argv[]) {
    signal(SIGSEGV, &sigHandler);
    signal(SIGABRT, &sigHandler);
    //一些平台程序退出时不能释放alsa资源导致不能重新初始化alsa
    signal(SIGINT, &sigHandler);
    //curl_global_init(CURL_GLOBAL_DEFAULT);
    executor = new ThreadPool(1);

    using namespace iflyosInterface;
    //注册事件处理函数
    std::vector<std::pair<std::string, IFLYOS_InterfaceEvtHandler>> evtReg;
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_DIALOG_END, &stopCapture));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_DIALOG_THINKING, &onThinking));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_DIALOG_SPEAKING, &onSpeaking));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_STOPPED, &iflyosAppExited));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_AUTH_REQUIRED, &playAuthCode));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_AUTH_OK, &authOK));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_AUTH_SERVER_ERROR, &authError));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_VOL_CHANGED, &volChanged));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_AUTH_TOKEN_EXPIRED, &tokenExpired));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_REVOKE_AUTHORIZATION, &revokeAuth));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_SW_UPDATE,
            [](const char*, const char* msg){
                static bool trueFalse = false;
                if(trueFalse){
                    iflyosInterface::IFLYOS_InterfaceSendEvent(
                            IFLYOS_DO_REPORT_SW_UPDATE_START,
                            (std::string("测试版本号\n") + "测试版本描述内容").c_str());
                    iflyosInterface::IFLYOS_InterfaceSendEvent(
                            IFLYOS_DO_REPORT_SW_UPDATE_SUCCESS,
                            (std::string("测试版本号\n") + "测试版本描述内容").c_str());
                }else{
                    iflyosInterface::IFLYOS_InterfaceSendEvent(
                            IFLYOS_DO_REPORT_SW_UPDATE_START,
                            (std::string("测试版本号\n") + "测试版本描述内容").c_str());
                    iflyosInterface::IFLYOS_InterfaceSendEvent(
                            IFLYOS_DO_REPORT_SW_UPDATE_FAIL,
                            (std::string("测试错误\n") + "测试错误描述").c_str());
                }
                trueFalse = !trueFalse;
            }));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_SW_UPDATE_CHECK,
            [](const char*, const char*){
                static bool trueFalse = false;
                if(trueFalse){
                    iflyosInterface::IFLYOS_InterfaceSendEvent(
                            IFLYOS_DO_REPORT_SW_UPDATE_CHECK_SUCCESS,
                            (std::string("true\n") + "测试版本号\n" + "测试版本描述内容").c_str());
                }else{
                    iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_REPORT_SW_UPDATE_CHECK_FAIL);
                }
                trueFalse = !trueFalse;
            }));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_SET_WAKEWORD,
            [](const char*, const char* msg){
                static bool trueFalse = false;
                std::vector<std::string> lines;
                iflyosInterface::IFLYOS_InterfaceGetLines(msg, lines);
                if(trueFalse){
                    iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_REPORT_SET_WW_SUCCESS, lines[1].c_str());
                }else{
                    iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_REPORT_SET_WW_FAIL, (lines[1] + "测试错误描述").c_str());
                }
                trueFalse = !trueFalse;
            }));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_FACTORY_RESET,
            [](const char*, const char* msg){
                printf("FACTORY_RESET!!\n");
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_CLEAR_DATA_THEN_QUIT);
                sleep(5);//wait 5s for cleaning
                write(restartChannel, NET_CONFIG, 1);//then re-set network
            }));
    evtReg.push_back(std::pair<std::string, IFLYOS_InterfaceEvtHandler>(IFLYOS_EVT_REBOOT,
            [](const char*, const char* msg){
                printf("REBOOT\n");
                //note: do real reboot
            }));
    //初始化IPC通讯模块
    int ret = iflyosInterface::IFLYOS_InterfaceInit("IFLYOS_INTERFACE_MIC", evtReg);
    if (ret) return ret;
    //重启IFLYOS,这里杀掉,检测到网络OK会再启动
    iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_STOP);

    //启动一个30s一次的网络检查监视器
    printf("to create NetworkMonitor, 30s interval\n");
    iflyos::agent::NetworkMonitor monitor(std::chrono::seconds(30));
    monitor.addObserver(std::make_shared<NetworkStatusMonitor>());

    //如果60s内没有网络,进入配网
    auto count = 0;
    printf("to wait network ready\n");
    while(!hasNetwork){
        sleep(5);
        count += 5;
        if(count > 60){
            //配网超时3分钟
            setNetwork(60 * 3);
            break;
        }
    }

    //初始化录音和语音唤醒模块
    recorder->setFormat(iflyos::agent::Recorder::Format::ORIGIN_OUTPUT);
    recorder->addObserver(std::make_shared<OrgAudioObserver>());

    int fd[2];
    pipe(fd);
    restartChannel = fd[1];

    //初始化本地PCM播放器
    pcmPlayer = new iflyos::agent::PCMPlayer(std::vector<std::string>{
            "/home/pi/ivs/sound_effect/here.wav",
            "/home/pi/ivs/sound_effect/plastic-soda-pop.wav"
    });

    //开始录音
    recorder->start();

    //这里是等待IFLYOS启动的绝对标签,因为使用是过程可以会杀掉IFLYOS(比如重新配网\恢复出产设置等)
WAIT_START:
    inThinking = false;
    printf("to wait iflyos app ready\n");
    while (firstDialogState == IFLYOSReadyState::NOT_START) {
        sleep(1);
    }

    printf("to read keypress\n");
    //进入按键处理循环
    auto fakeWakeUpWrFd = SysKey_Init("/dev/input/event2", fd[0]);
    //对于没有唤醒模块的Demo,fakeWakeUp用按键模拟唤醒
    //iflyos::agent::Recorder::fakeWakeUp(recorder, fakeWakeUpWrFd);
    while (!quit) {
        auto value = SYSKey_getKeyVaule();
        if (value == KEY_NONE || value == KEY_ERROR) {
            continue;
        }

        switch(value){
            case KEY_SET_DND_ON:
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_SET_DND, "true");
                break;
            case KEY_SET_DND_OFF:
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_SET_DND, "false");
                break;
            case KEY_RESTART_IFLYOS:{
                firstDialogState = IFLYOSReadyState::NOT_START;
                recorder->stop();
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_STOP);
                sleep(5);
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_START);
                goto WAIT_START;
            }

            case KEY_NET_CONFIG:{
                firstDialogState = IFLYOSReadyState::NOT_START;
                recorder->stop();
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_STOP);
                std::cout << "Press config button" << std::endl;
                setNetwork(60 * 3);
                printf("network set, start ivs\n");
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_START);
                goto WAIT_START;
            }
            case KEY_VOL_DOWN:
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_KEYPRESS_VOL_DOWN);
                break;
            case KEY_VOL_UP:
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_KEYPRESS_VOL_UP);
                break;
            case KEY_PLAY_PAUSE:
                iflyosInterface::IFLYOS_InterfaceSendEvent(IFLYOS_DO_KEYPRESS_PLAY_PAUSE);
                break;
            case KEY_MIC_MUTE:
                if(micOn) {
                    std::system("/home/pi/ivs/SampleAppCtrl.sh play /home/pi/ivs/sound_effect/disable-microphone.mp3 0.5");
                    //recorder.stop();
                    std::system("echo 2 > /home/pi/ivs/ledfile");
                    micOn = false;
                }else{
                    std::system("/home/pi/ivs/SampleAppCtrl.sh play /home/pi/ivs/sound_effect/enable-microphone.mp3 0.5");
                    //recorder.start();
                    std::system("echo 3 > /home/pi/ivs/ledfile");
                    micOn = true;
                }
                break;
            case KEY_VOL_DOWN_LONG_PRESS:
            case KEY_VOL_UP_LONG_PRESS:
            case KEY_PLAY_PAUSE_LONG_PRESS:
            case KEY_NONE:
            case KEY_ERROR:
                break;

        }
    }

    recorder->stop();
    executor->join();

    iflyosInterface::IFLYOS_InterfaceDeinit();
    if (forwarding) {
        close(fdAudio);
        fdAudio = -1;
    }
    delete pcmPlayer;
    return 0;
}

