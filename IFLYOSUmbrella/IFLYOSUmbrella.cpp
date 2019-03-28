//
// Created by jan on 18-11-30.
//

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <functional>
#include <sstream>
#include <cstring>
#include <signal.h>
#include <wait.h>

#include "IFLYOSInterface.h"
#include "IFLYOSThreadPool.h"

#define CODE_PCM_PATH  "/tmp/usercode"
#define CODE_NUMBERS_PATH "./numbers.pcm"

//check network configuration
//check network connectivity
//receive network config
//config network
//start/stop IFLYOS
//play user code external auth

using namespace iflyosInterface;

static ThreadPool exector(1);
static std::string exePath;
static std::string logLevel;
static std::string configPath;
static std::string micFIFOPath;
static int ipcVersion;
static volatile int iflyosPid = 0;
static std::thread *iflyosPorcessMonitor;

static void onIFLYOS_DoStart(const char * /*evt*/, const char *msg) {
    exector.enqueue([] {
        if (iflyosPid) return;

        if (ipcVersion != IFLYOS_INTERFACE_CURRENT_VERSION){
            printf("error: IPC interface version not match. %d <> %d\n", ipcVersion, IFLYOS_INTERFACE_CURRENT_VERSION);
            return;
        }

        std::cout << "to execute:" << exePath << " " << configPath << " " << logLevel << std::endl;
        auto pid = fork();
        if (pid == 0) {
            IFLYOS_InterfaceUmberllaClearForChild();
            sleep(2);//wait iflyosPorcessMonitor started
            std::ifstream f(exePath.c_str());
            std::cout << "executable exists?" << (f.good() == true) << std::endl;
            execl(exePath.c_str(), exePath.c_str(), configPath.c_str(), micFIFOPath.c_str(), logLevel.c_str(), (char *) NULL);
            printf("start iflyos app failed:%d\n", (int)errno);
            exit(-errno);
        }

        iflyosPid = pid;
        iflyosPorcessMonitor = new std::thread([] {
            wait(nullptr);
            printf("iflyosapp exited\n");
            exector.enqueue([] {
                iflyosPid = 0;
                IFLYOS_InterfaceUmbrellaSendEvent(IFLYOS_EVT_STOPPED);
            });
        });
        iflyosPorcessMonitor->detach();
    });
}

static void onCustomDirective(const char* /*evt*/, const char* msg){
    printf("onCustomDirective:%s\n", msg);
}

static void onIFLYOS_DoStop(const char * /*evt*/, const char *msg) {
    exector.enqueue([] {
        if (iflyosPid) {
            kill(iflyosPid, SIGTERM);
        }
    });
}

int main(int /*argc*/, char** /**argv[]*/) {
    auto intfVerStr = std::getenv("IFLYOS_ENV_IPC_VERSION");
    if(!intfVerStr){
        printf("IFLYOS_ENV_IPC_VERSION env not found\n");
        return 0;
    }
    ipcVersion = std::atoi(intfVerStr);

    logLevel = "DEBUG5";
    auto logLevelStr = std::getenv("IFLYOS_ENV_LOG_LEVEL");
    if(logLevelStr){
        logLevel = logLevelStr;
    }

    auto iflyosExeStr = std::getenv("IFLYOS_ENV_EXE_PATH");
    if(!iflyosExeStr){
        printf("IFLYOS_ENV_IPC_VERSION env not found\n");
        return 0;
    }
    exePath = iflyosExeStr;

    auto cfgPathStr = std::getenv("IFLYOS_ENV_CONFIG_PATH");
    if(!cfgPathStr){
        printf("IFLYOS_ENV_CONFIG_PATH env not found\n");
        return 0;
    }
    configPath = cfgPathStr;

    auto micFIFOPathStr = std::getenv("IFLYOS_ENV_MIC_FIFO_PATH");
    if(!micFIFOPathStr){
        printf("IFLYOS_ENV_MIC_FIFO_PATH env not found\n");
        return 0;
    }
    micFIFOPath = micFIFOPathStr;

    if(ipcVersion != IFLYOS_INTERFACE_CURRENT_VERSION){
        printf("error: iflyos interface version not match: %d <> %d\n", ipcVersion, IFLYOS_INTERFACE_CURRENT_VERSION);
        return 0;
    }


    int ret = IFLYOS_InterfaceUmberllaInit();
    if (ret) {
        printf("IFLYOS_InterfaceUmberllaInit failed:%d\n", ret);
        return ret;
    }

    IFLYOS_InterfaceRegisterHandler(onIFLYOS_DoStart, IFLYOS_DO_START);
    IFLYOS_InterfaceRegisterHandler(onIFLYOS_DoStop, IFLYOS_DO_STOP);
    IFLYOS_InterfaceRegisterHandler(onCustomDirective, IFLYOS_EVT_CUSTOM_DIRECTIVE);

    char c;
    while(true){
        std::cin >> c;
        if(c == '\n') continue;
        if(c == '\r') continue;
        if(c == 'q'){
            break;
        }
        if(c == 's'){
            onIFLYOS_DoStart(IFLYOS_DO_START, "");
            continue;
        }
        if(c == 't'){
            onIFLYOS_DoStop(IFLYOS_DO_STOP, "");
            continue;
        }
        if(c == 'r'){
            IFLYOS_InterfaceUmbrellaSendAction(IFLYOS_DO_CLEAR_DATA_THEN_QUIT);
            continue;
        }
        if(c== 'c'){
            IFLYOS_InterfaceUmbrellaSendAction(IFLYOS_DO_CUSTOM_CONTEXT, "{\"name\": \"测试自定义上下文\"}");
            continue;
        }
        if(c== 'e'){
            IFLYOS_InterfaceUmbrellaSendAction(IFLYOS_DO_CUSTOM_EVENT, "{\"name\": \"测试自定义事件\"}");
            continue;
        }
    }

    exector.safeQuit();
}
