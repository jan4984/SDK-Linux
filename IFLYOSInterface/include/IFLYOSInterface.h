//
// Created by jan on 18-11-26.
//

#ifndef IFLYOSCLIENTSDK_INTERFACE_H
#define IFLYOSCLIENTSDK_INTERFACE_H

#define IFLYOS_INTERFACE_CURRENT_VERSION 1

#define IFLYOS_DO_REGISTER_EVT_RECEIVER          "DO_REGISTER_EVT_RECEIVER"

//启动IFLYOS
//param: 每行一个配置项，key1:value1\nkey2:value2\n
#define IFLYOS_DO_START                          "DO_START"

//可执行程序完整路径
#define IFLYOS_ARG_NAME_EXE_PATH                 "exe:"
//配置文件完整路径
#define IFLYOS_ARG_NAME_CFG_PATH                 "cfg:"
//log等级
#define IFLYOS_ARG_NAME_LOG_LEVEL                "log:"
//iflyos interface 版本
#define IFLYOS_ARG_NAME_INTF_VER                 "intfVer:"

//退出IFLYOS
#define IFLYOS_DO_STOP                           "DO_STOP"

//求被杀，然后恢复出产设置
#define IFLYOS_EVT_FACTORY_RESET                 "EVT_FACTORY_RESET"

//请求重启
#define IFLYOS_EVT_REBOOT                        "EVT_BOOT"

//请求升级固件
#define IFLYOS_EVT_SW_UPDATE                     "EVT_SW_UPDATE"

//升级固件成功
//prams:第1行版本名称，第2行版本描述
#define IFLYOS_DO_REPORT_SW_UPDATE_SUCCESS       "DO_REPORT_SW_UPDATE_SUCCESS"

//升级固件失败
//params:第一行错误类型，第二行错误消息
#define IFLYOS_DO_REPORT_SW_UPDATE_FAIL          "DO_REPORT_SW_UPDATE_FAIL"

//开始升级固件
//prams:第1行版本名称，第2行版本描述
#define IFLYOS_DO_REPORT_SW_UPDATE_START         "DO_REPORT_SW_UPDATE_START"

//请求检查固件升级
#define IFLYOS_EVT_SW_UPDATE_CHECK               "EVT_SW_UPDATE_CHECK"

//检查升级成功
//params:第一行是否需要升级true/false，第二行版本名称，第三行版本描述
#define IFLYOS_DO_REPORT_SW_UPDATE_CHECK_SUCCESS "DO_REPORT_SW_UPDATE_CHECK_SUCCESS"

//检查升级失败
#define IFLYOS_DO_REPORT_SW_UPDATE_CHECK_FAIL    "DO_REPORT_SW_UPDATE_CHECK_FAIL"

//授权被清除
#define IFLYOS_EVT_REVOKE_AUTHORIZATION          "EVT_REVOKE_AUTHORIZATION"

//请求更换唤醒词
//params:第一行url,第二行key
#define IFLYOS_EVT_SET_WAKEWORD                  "EVT_SET_WAKEWORD"

//更换唤醒词成功
//params:key
#define IFLYOS_DO_REPORT_SET_WW_SUCCESS          "DO_REPORT_SET_WW_SUCCESS"

//更换唤醒词失败
//params:第一行key,第二行错误消息
#define IFLYOS_DO_REPORT_SET_WW_FAIL             "DO_REPORT_SET_WW_FAIL"

//清除本地数据然后退出
#define IFLYOS_DO_CLEAR_DATA_THEN_QUIT           "DO_CLEAR_DATA_THEN_QUIT"

//已停止
#define IFLYOS_EVT_STOPPED                       "EVT_STOPPED"

//免打扰模式
//params:true/false
#define IFLYOS_DO_SET_DND                        "DO_SET_DND"

//请求用户授权，进入等待用户授权
//param: 需要展示/播放给用户的UserCode
#define IFLYOS_EVT_AUTH_REQUIRED                 "EVT_AUTH_REQUIRED"
//正在向服务器请求授权
#define IFLOS_EVT_AUTH_PENDING                   "EVT_AUTH_PENDING"
//授权令牌过期，IFLYOS退出，需要重新启动(IFLYOS_DO_START)
#define IFLYOS_EVT_AUTH_TOKEN_EXPIRED            "EVT_AUTH_TOKEN_EXPIRED"
//授权成功
#define IFLYOS_EVT_AUTH_OK                       "EVT_AUTH_OK"

//服务异常导致授权失败,IFLYOS退出，需要重新启动(IFLYOS_DO_START)
#define IFLYOS_EVT_AUTH_SERVER_ERROR              "EVT_AUTH_SERVER_ERROR"

//发起交互
//param: FIFO路径，IFLYOS将从里面读取音频上传到IVS
#define IFLYOS_DO_DIALOG_START                   "DO_DIALOG_START"
//放弃本次交互
#define IFLYOS_DO_DIALOG_CANCEL                  "DO_DIALOG_CANCEL"

//录音结束，等待服务端回应
#define IFLYOS_EVT_DIALOG_THINKING               "EVT_DIALOG_THINKING"
//开始播放服务器语音合成响应
#define IFLYOS_EVT_DIALOG_SPEAKING               "EVT_DIALOG_SPEAKING"
//对话结束
#define IFLYOS_EVT_DIALOG_END                    "EVT_DIALOG_END"
//追问
#define IFLYOS_EVT_DIALOG_EXPECT                 "EVT_DIALOG_EXPECT"
//实时返回本次交互识别到的文本
#define IFLYOS_EVT_INTERMEDIA_TEXT               "EVT_INTERMEDIA_TEXT"

//执行暂停/播放
#define IFLYOS_DO_KEYPRESS_PLAY_PAUSE            "DO_KEYPRESS_PLAY_PAUSE"

//执行暂停
#define IFLYOS_DO_KEYPRESS_PAUSE                 "DO_KEYPRESS_PAUSE"
//执行播放
#define IFLYOS_DO_KEYPRESS_PLAY                  "DO_KEYPRESS_PLAY"
//执行停止闹钟响铃按键
#define IFLYOS_DO_KEYPRESS_STOP_ALARM            "DO_KEYPRESS_STOP_ALARM"
//执行减小音量按键
#define IFLYOS_DO_KEYPRESS_VOL_DOWN              "DO_KEYPRESS_VOL_DOWN"
//执行增大音量按键
#define IFLYOS_DO_KEYPRESS_VOL_UP                "DO_KEYPRESS_VOL_UP"

//音量改变
//param:第一行，当前数值，0-100
//param:第二行，是否静音，true/false
#define IFLYOS_EVT_VOL_CHANGED                   "EVT_VOL_CHANGED"

//客户自定义指令
//param: payload json字符串
#define IFLYOS_EVT_CUSTOM_DIRECTIVE              "EVT_CUSTOM_DIRECTIVE"
//更新客户自定义状态
//param: payload json字符串
#define IFLYOS_DO_CUSTOM_CONTEXT                 "DO_CUSTOM_CONTEXT"
//自定义事件
#define IFLYOS_DO_CUSTOM_EVENT                   "DO_CUSTOM_EVENT"

#define IFLYOS_EVENT_RECV_ADDR                   "IFLYOS_EVENT_RECV_ADDR"
#define IFLYOS_UMBRELLA_RECV_ADDR                "IFLYOS_UMBRELLA_RECV_ADDR"


//#############################################################################################################
//####################################MediaPlayer IPC Messages#################################################
//#############################################################################################################

//请求创建播放器
//params:播放器名字
#define IFLYOS_EVT_MEDIAPLAYER_CREATE               "EVT_MEDIAPLAYER_CREATE"
//设置音频源，设置完以后需要IFLYOS_DO_MEDIAPLAYER_REPORT_SOURCE_ID返回id
//params:第一行，播放器名字
//params:第二行，source类型, URL|MP3|PCM
//params:第三行，url值或者FIFO路径
//params:第四行，true|false|数值，true/false表示是否重复播放，数值表示偏移
#define IFLYOS_EVT_MEDIAPLAYER_SET_SOURCE           "EVT_MEDIAPLAYER_SET_SOURCE"
//params:第一行，播放器名字
//params:第二行，+N -N是上下调整，N是绝对音量值
#define IFLYOS_EVT_MEDIAPLAYER_SET_VOLUME           "EVT_MEDIAPLAYER_SET_VOLUME"
//params:第一行，播放器名字
//params:第二行，true/false
#define IFLYOS_EVT_MEDIAPLAYER_SET_MUTE             "EVT_MEDIAPLAYER_SET_MUTE"
//params:第一行，播放器名字
//params:第二行，音频源id
#define IFLYOS_EVT_MEDIAPLAYER_PLAY                 "EVT_MEDIAPLAYER_PLAY"
//params:第一行，播放器名字
//params:第二行，音频源id
#define IFLYOS_EVT_MEDIAPLAYER_STOP                 "EVT_MEDIAPLAYER_STOP"
//params:第一行，播放器名字
//params:第二行，音频源id
#define IFLYOS_EVT_MEDIAPLAYER_PAUSE                "EVT_MEDIAPLAYER_PAUSE"
//params:第一行，播放器名字
//params:第二行，音频源id
#define IFLYOS_EVT_MEDIAPLAYER_RESUME               "EVT_MEDIAPLAYER_RESUME"
//params:第一行，播放器名字
//params:第二行，音频源id
//params:第三行，play/stop/pause/resume
//params:第四行，true/false
#define IFLYOS_DO_MEDIAPLAYER_REPORT_CONTROL_RESULT "DO_MEDIAPLAYER_REPORT_CONTROL_RESULT"
//params:第一行，播放器名字
//params:第二行，音频源id
#define IFLYOS_EVT_MEDIAPLAYER_REPORT_OFFSET        "EVT_MEDIAPLAYER_REPORT_OFFSET"
//params:第一行，播放器名字
#define IFLYOS_EVT_MEDIAPLAYER_REPORT_VOLUME        "EVT_MEDIAPLAYER_REPORT_VOLUME"
//返回音频源id
//params:第一行，播放器名字
//params:第二行，音频源id
#define IFLYOS_DO_MEDIAPLAYER_REPORT_SOURCE_ID      "DO_MEDIAPLAYER_REPORT_SOURCE_ID"
//返回offset
//params:第一行，播放器名字
//params:第二行，音频源id
//params:第三行，offset
#define IFLYOS_DO_MEDIAPLAYER_REPORT_OFFSET         "DO_MEDIAPLAYER_REPORT_OFFSET"
//params:第一行，播放器名字
//params:第二行，音量
//params:第三行，是否静音true/false
#define IFLYOS_DO_MEDIAPLAYER_REPORT_VOLUME         "DO_MEDIAPLAYER_REPORT_VOLUME"
//返回offset
//params:第一行，播放器名字
//params:第二行，音频源id
//params:第三行，播放状态：play/stop/pause/finish/error
#define IFLYOS_DO_MEDIAPLAYER_REPORT_PLAY_STATUS    "DO_MEDIAPLAYER_REPORT_PLAY_STATUS"

#include <vector>
#include <functional>

namespace iflyosInterface {

int getCurrentVersion();

typedef std::function<void(const char* evt, const char* msg)> IFLYOS_InterfaceEvtHandler;

int IFLYOS_InterfaceUmberllaInit();

//为当前进程初始化事件接受器并指定地址
//一个进程只能有一个接收器
//分发事件通过IFLYOS_InterfaceRegisterHandler注册回掉实现
int IFLYOS_InterfaceInit(
        const char *address,
        const std::vector<std::pair<std::string, IFLYOS_InterfaceEvtHandler>> &eventsRegistration);

//销毁事件接收器
void IFLYOS_InterfaceDeinit();
void IFLYOS_InterfaceUmberllaClearForChild();

//在本地注册事件处理函数，接收到IFLYOS发送过来的evt事件以后会调用handler
//如果handler是nullptr则视为删除注册
int IFLYOS_InterfaceRegisterHandler(const IFLYOS_InterfaceEvtHandler handler, const char *evt);

//发送命令给IFLYOS
int IFLYOS_InterfaceSendEvent(const char *evt, const char *msg = "");

int IFLYOS_InterfaceUmbrellaSendAction(const char *evt, const char *msg = "");
int IFLYOS_InterfaceUmbrellaSendEvent(const char *evt, const char *msg = "");

void IFLYOS_InterfaceGetLines(const std::string &strVal, std::vector<std::string> &vecStr);
}

#endif //IFLYOSCLIENTSDK_INTERFACE_H
