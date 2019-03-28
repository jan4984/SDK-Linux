此项目是使用iFLYOS SDK在树莓派上运行的参考和指导. SDK本身不开源, 此项目内代码是使用SDK的参考

## 开发环境

* 拉编译环境: 

```bash
$docker pull ccr.ccs.tencentyun.com/dev-runtime/iflyos_raspbian_compiler:1.6
```

* 编译此项目
```bash
#进入此项目源码目录
$mkdir ~/ivs-built
$docker run -it --rm -v $PWD:/ivs/sdk2-agents -v $HOME/ivs-built:/ivs-build ccr.ccs.tencentyun.com/dev-runtime/iflyos_raspbian_compiler:1.6 bash
#进入容器内运行
$cd /ivs/sdk2-agents
$bash build.sh
```

如果没有错误,编译结果在 `~/ivs-built`下面.

## 树莓派系统准备

* 购买,给树莓派安装raspbian:jessie
* 配置好网络
* 配置alsa音频系统,设置默认输入输出设备. 如从usb外置声卡输入输出,参考:

```bash
$cat /proc/asound/cards
0 [ALSA           ]: bcm2835 -
1 [ArrayUAC10     ]: USB-Audio -
$echo 'defaults.pcm.card 1' | sudo tee /etc/asound.conf
$echo 'defaults.ctl.card 1' | sudo tee -a /etc/asound.conf
$sudo /etc/init.d/alsa-utils restart
$arecord -c1 -fS16_LE -r16000 test.wav #录音,ctrl+c结束
$aplay test.wav #应该可以听到刚才的录音
$mkdir /home/pi/ivs
```

* 安装所需运行库

```bash
$sudo apt install -y libopus0 libsqlite3-0 gstreamer-1.0 gstreamer1.0-plugins-good gstreamer1.0-plugins-bad libatlas3-base
```

## 开放平台申请

https://device.iflyos.cn/device

## 拷贝iFLYOS到树莓派

* 拷贝程序和资源到树莓派

```bash
#从release下载编译好的程序
$cd /home/pi/ivs
$wget -Oprebult.tar.gz https://xxxxxx
$tar xvf prebuilt.tar.gz
#下载snowboy唤醒资源
$wget -O snowboy.umdl 'https://github.com/Kitt-AI/snowboy/blob/master/resources/models/snowboy.umdl?raw=true'
$wget -O common.res 'https://github.com/Kitt-AI/snowboy/blob/master/resources/common.res?raw=true'
#如需替换MicAgent或IFLYOSUmbrella,则用自己编译的程序覆盖
```

* 修改` ivsConfig.json` 文件中的 `deviceSerialNumber` 和 `clientId`。`clientId` 见 IFLYOS控制台 - 设备信息，`deviceSerialNumber`可自行制定，请保证在同一个`clientId`下`deviceSerialNumber`唯一。

## 运行

下载app端[小飞在线](https://cdn.iflyos.cn/docs/iflyhome.apk),注册用户.

到[iFLYOS设备接入控制台](https://device.iflyos.cn/products)选择你的`clientId`对应的设备,把你注册的手机号添加到白名单.

按顺序启动:

控制台1
```bash
$cd /home/pi/ivs
$LD_LIBRARY_PATH=libs bash ./start.sh 
```

控制台2
```bash
$cd /home/pi/ivs
$LD_LIBRARY_PATH=libs ./GstreamerMediaPlayerAgent
```

控制台3
```bash
$cd /home/pi/ivs
$LD_LIBRARY_PATH=libs ./MicAgent
```
* 授权

没有意外的话,你可以听到"你的授权码为"xxxxxx"6个数字. 到小飞在线app中添加设备,选择"沃家智能音箱A1",直接勾选"已听到设备提示音""下一步",随便输入一个wifi名称下一步,勾选"已听到开始联网""下一步".填入树莓派上播放的6个数字,"提交".

授权成功以后,会听到"我已联网,你可以对我说xxxxx"

* 交互

用"snowboy"唤醒,然后可以开始对话

## 定制开发

如果要在iFLYOS SDK之外开发定制功能，请先查阅[iFLYOS设备接入文档](https://doc.iflyos.cn/device/)，确保了解iFLYOS的运行原理后，再开始。


## FAQ

1. 如果有些小伙伴不想使用jessie版本该怎么办?

   因为系统版本不同,如果运行时找不到对应版本的库,可以尝试从编译容器中拷贝出系统中没有的动态库到树莓派运行环境,放到/home/rpi/ivs/libs下.