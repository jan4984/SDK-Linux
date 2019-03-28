//
// Created by jan on 19-1-28.
//

#ifndef IFLYOSCLIENTSDK_NETWORKSETTER_H
#define IFLYOSCLIENTSDK_NETWORKSETTER_H

#include <memory>

namespace iflyos {
namespace agent {

class NetworkSetter {
public:
    static void start(bool& needAuth, bool& success, const int timeout);
};

}
}


#endif //IFLYOSCLIENTSDK_NETWORKSETTER_H
