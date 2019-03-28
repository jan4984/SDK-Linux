//
// Created by jan on 19-1-28.
//

#ifndef IFLYOSCLIENTSDK_NETWORKMONITOR_H
#define IFLYOSCLIENTSDK_NETWORKMONITOR_H

#include <string>
#include <chrono>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <thread>

namespace iflyos {
namespace agent{

class NetworkMonitor {
public:
    enum NetworkStatus {
        NETWORK_FAIL,
        HTTPS_OK
    };

    class NetworkStatusObserver {
    public:
        virtual ~NetworkStatusObserver() = default;
        virtual void onNetworkStatusChanged(NetworkStatus status) = 0;
    };

    NetworkMonitor(std::chrono::seconds checkInterval);
    void addObserver(std::shared_ptr<NetworkStatusObserver> observer);

    ~NetworkMonitor();

    //reset state to unknown, will trigger status report
    void reset();

    static bool trySetTimeOneTime(std::chrono::seconds timeout);

private:

    std::unordered_set<std::shared_ptr<NetworkStatusObserver>> m_observers;
    std::mutex m_mutex;
    NetworkStatus m_status;
    std::chrono::seconds m_checkInterval;
    bool m_timeSet;
    std::thread m_worker;
    volatile bool m_quit;
};

}
}

#endif //IFLYOSCLIENTSDK_NETWORKMONITOR_H
