//
// Created by jan on 19-1-28.
//

#include <unistd.h>

#include "NetworkMonitor.h"
#include "jnh.h"

namespace iflyos {
namespace agent {
//https://cdn.iflyos.cn/device-wifi/wifistub.html
//http://easybox.iflyos.cn/time_now

static std::chrono::seconds LOSE_CHECK(10);

static bool fetchAndSetTime(const int timeout){
    int code;
    char body[128] = {0};
    int bodyLen = sizeof(body) - 1;
    int ret = jnh_get("easybox.iflyos.cn", 80, "/time_now", timeout, timeout, &code, body, &bodyLen);
    printf("jnh ret:%d, status:%d, body: %s\n", ret, code, body);
    if(ret != JNHE_OK) return false;
    if(code != 200) return false;
    std::string cmd = std::string("date +%s -s @") + body;
    auto cmdRet = std::system(cmd.c_str());
    if(WEXITSTATUS(cmdRet) == 0){
        return true;
    }
    printf("set time failed, cmd returns:%d\n", WEXITSTATUS(cmdRet));
    return false;
}

static bool testConnection(const int timeout){
    int code;
    int bodyLen = 0;
    int ret = jnh_get("easybox.iflyos.cn", 80, "/wifistub", timeout, timeout, &code, nullptr, &bodyLen);
    printf("jnh ret:%d, status:%d\n", ret, code);
    if(ret != JNHE_OK) return false;
    if(code != 200) return false;
    return true;
}


NetworkMonitor::NetworkMonitor(std::chrono::seconds checkInterval) :
        m_status{NetworkStatus::NETWORK_FAIL},
        m_checkInterval{checkInterval},
        m_timeSet{false},
        m_quit{false}{
    m_worker = std::thread([this]{
        auto first = true;
        while(!m_quit && !m_timeSet){
            if(!first) {
                sleep(LOSE_CHECK.count());
            }
            printf("to perform url http://easybox.iflyos.cn/time_now\n");
            if(fetchAndSetTime(10000)){
                continue;
            }
            m_timeSet = true;
        }

        int failTimes = 0;
        while(!m_quit){
            sleep(2);
            printf("to perform url http://easybox.iflyos.cn/wifistub\n");
            auto ret = testConnection(m_checkInterval.count() * 1000);
            printf("curl returns:%d, failTimes:%d\n",(int)ret, failTimes);
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (!ret && failTimes >= 2 && m_status != NETWORK_FAIL) {
                    m_status = NETWORK_FAIL;
                    for (auto observer : m_observers) {
                        observer->onNetworkStatusChanged(m_status);
                    }
                }

                if (ret && m_status == NETWORK_FAIL) {
                    m_status = HTTPS_OK;
                    for (auto observer : m_observers) {
                        observer->onNetworkStatusChanged(m_status);
                    }
                }
            }

            if(ret) {
                failTimes = 0;
                sleep(m_checkInterval.count());
            }else{
                failTimes++;
                sleep(LOSE_CHECK.count());
            }
        }
    });
}

NetworkMonitor::~NetworkMonitor(){
    m_quit = true;
    m_worker.join();
}

void NetworkMonitor::addObserver(
        std::shared_ptr<NetworkMonitor::NetworkStatusObserver> observer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.insert(observer);
}


bool NetworkMonitor::trySetTimeOneTime(std::chrono::seconds timeout) {
    printf("to perform url http://easybox.iflyos.cn/time_now\n");
    auto ret = fetchAndSetTime(timeout.count() * 1000);
    printf("curl returns:%d\n",(int)ret);
    return true;
}

void NetworkMonitor::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_status = NETWORK_FAIL;

}

}
}