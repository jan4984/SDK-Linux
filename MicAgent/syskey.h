#ifndef _SYS_KEY_H_
#define _SYS_KEY_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KEY_VOL_DOWN,
    KEY_VOL_UP,
    KEY_PLAY_PAUSE,
    KEY_MIC_MUTE,
    KEY_VOL_DOWN_LONG_PRESS,
    KEY_VOL_UP_LONG_PRESS,
    KEY_PLAY_PAUSE_LONG_PRESS,
    KEY_NET_CONFIG,
    KEY_RESTART_IFLYOS,
    KEY_SET_DND_ON,
    KEY_SET_DND_OFF,
    KEY_NONE,
    KEY_ERROR,
}SYSKEY_Value;

int SysKey_Init(const char *devicePath, const int extraFd);

int SysKey_Uninit(void);

SYSKEY_Value SYSKey_getKeyVaule();

#ifdef __cplusplus
}
#endif

#endif
