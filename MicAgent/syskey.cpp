#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <map>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include <linux/input.h>
#include <fcntl.h>
#include <poll.h>
#include <thread>

#include "syskey.h"

#define AML_KEY_VOL_DOWN		114
#define AML_KEY_VOL_UP			113

#define AML_KEY_PLAY_PAUSE		143
#define AML_KEY_MIC_MUTE		115

#define USER_DEFINED_KEY_NETCONFIG      10001
#define USER_DEFINED_KEY_RESTART_IFLYOS 10011

typedef enum{
    KEY_EVENT_DOWN = 0,
    KEY_EVENT_UP,
    KEY_EVENT_LONG_PRESS,
}SYSKEY_Event;

typedef struct _KeyInfo{
    SYSKEY_Event 	event;
    int 			keyCode;
    unsigned long 	time;
}KeyInfo;

#define KERNEL_KEYUP  0
#define KERNEL_KEYDOWN 1

#define LONGPRESS_TIME_MS   (3000)

static int key_fd_wr = -1;
static int key_fd_rd = -1;
static int extra_fd = -1;

typedef enum
{
    EM_SYSTIME_FROM_BOOT, /* 从开机启动开始计算的时间，不受用户设置时间的影响 */
    EM_SYSTIME_UTC, /* UTC时间，如果系统时间更新，则会受到影响 */
}EM_SYS_TIME;
/*
* 获取系统当前时间，单位毫秒
*/
unsigned long getCurrentSysTime_ms(EM_SYS_TIME type)
{
    if( type == EM_SYSTIME_FROM_BOOT )
    {
        struct timespec t;
        t.tv_sec = t.tv_nsec = 0;
        clock_gettime(CLOCK_MONOTONIC, &t);
        return ((t.tv_sec*1000)+(t.tv_nsec/(1000*1000)));
    }
    else //if( type == EM_SYSTIME_UTC)
    {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return ((tv.tv_sec*1000)+(tv.tv_usec/1000));
    }
}

static SYSKEY_Value SYSKey_getKeyVauleCheckLongPress(int nKeyCode, bool long_press)
{
    // printf("key_event.code: %d\n", nKeyCode);
    switch (nKeyCode) {
        case AML_KEY_VOL_DOWN:
            return long_press ? KEY_VOL_DOWN_LONG_PRESS : KEY_VOL_DOWN;
        case AML_KEY_VOL_UP:
            return long_press ? KEY_VOL_UP_LONG_PRESS : KEY_VOL_UP;
        case AML_KEY_PLAY_PAUSE:
            return long_press ? KEY_PLAY_PAUSE_LONG_PRESS : KEY_PLAY_PAUSE;
        case AML_KEY_MIC_MUTE:
            return long_press ? KEY_NET_CONFIG : KEY_MIC_MUTE;
        default:
            // printf("%s: %d error\n", __func__, __LINE__);
            return KEY_ERROR;
    }
}

int SYSKey_getKeyEvent(struct input_event *key_event)
{
    struct pollfd fds[2];

    fds[0].revents = 0;
    fds[0].fd = extra_fd;
    fds[0].events = POLLIN;
    fds[1].revents = 0;
    fds[1].fd = key_fd_rd;
    fds[1].events = POLLIN;

    int ret = 0;

    ret = poll(&fds[0], sizeof(fds)/sizeof(fds[0]), 1<<30);
    if (fds[0].revents & POLLIN){
        char b[1];
        read(extra_fd, b, 1);
        if(b[0]){
            key_event->code = USER_DEFINED_KEY_NETCONFIG;
        }else{
            key_event->code = USER_DEFINED_KEY_RESTART_IFLYOS;
        }

        return 0;
    }else if(fds[1].revents & POLLIN){
        char b[128];
        read(key_fd_rd, b, sizeof(b));
        key_event->code = b[0];
    }

    if (ret >= -1) {
        return 0;
    } else {
        return ret;
    }
}

SYSKEY_Value SYSKey_getKeyVaule()
{
    static const std::map<char, SYSKEY_Value> C2KMap = {
            {'u', KEY_VOL_UP},
            {'U', KEY_VOL_UP_LONG_PRESS},
            {'d', KEY_VOL_DOWN},
            {'D', KEY_VOL_DOWN_LONG_PRESS},
            {'m', KEY_MIC_MUTE},
            {'p', KEY_PLAY_PAUSE},
            {'P', KEY_PLAY_PAUSE_LONG_PRESS},
            {'n', KEY_NET_CONFIG},
            {'O', KEY_SET_DND_ON},
            {'o', KEY_SET_DND_OFF},
    };
    // struct pollfd pollfd_key;
    struct input_event key_event;
    //int ret = 0;
    int code = 0;
    //check long press
    unsigned long lastdown_time = 0;
    bool long_press = false;
    key_event.code = 0;

    while (!SYSKey_getKeyEvent(&key_event)) {
        if(key_event.code == USER_DEFINED_KEY_NETCONFIG)
            return KEY_NET_CONFIG;
        if(key_event.code == USER_DEFINED_KEY_RESTART_IFLYOS)
            return KEY_RESTART_IFLYOS;
        auto i = C2KMap.find((char)key_event.code);
        if(i == C2KMap.end()){
            printf("not process keypress %c\n", (char)key_event.code);
            return KEY_NONE;
        }
        return i->second;
    }
    return KEY_NONE;
}

int SysKey_Init(const char *devicePath, const int extraFd)
{
    int fd[2];
    pipe(fd);
    key_fd_rd = fd[0];
    key_fd_wr = fd[1];
    extra_fd = extraFd;

    if(key_fd_rd < 0){
        printf("[%s %d] open %s filed!\n", __func__, __LINE__, devicePath);
        return -1;
    }

    return key_fd_wr;
}

int SysKey_Uninit(void)
{
    close(key_fd_rd);
    close(key_fd_wr);
    return 0;
}
