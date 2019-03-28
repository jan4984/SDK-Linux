#!/bin/bash

function clear_wifi()
{
	cp /home/pi/ivs/wpa_supplicant.conf /etc/wpa_supplicant.conf
	sync
	wpa_cli reconfigure
	touch /home/pi/ivs/nowififlag
}

function clear_file()
{
	rm /data/ivs.log.*
	rm /data/update.zip
	rm /data/mme*
	rm /home/pi/ivs/mme*
	rm /home/pi/ivs/mdm*
	rm /home/pi/ivs/mtc*
}

function start_SampleApp()
{
	if [ -f "/home/pi/ivs/factoryflag" ]; then
		clear_wifi
		rm /home/pi/ivs/factory_mode/miscDatabase-dev.db
		rm /home/pi/ivs/factory_mode/alerts-dev.db
		rm /home/pi/ivs/factory_mode/settings-dev.db
		rm /home/pi/ivs/factory_mode/certifiedsender-dev.db
		rm /home/pi/ivs/factory_mode/notifications-dev.db
		rm /home/pi/ivs/factoryflag
		rm /home/pi/ivs/testflag
		rm /etc/wakeup
		rm /etc/connected
	fi

	clear_file
	cd /etc/ivs
	nohup SampleApp ivsConfig.json >/dev/nul 2>&1 &
}

function stop_SampleApp()
{
	killall SampleApp
	echo 3 > /home/pi/ivs/ledfile
}

function factory_init()
{
	echo iottest > /data/select.txt
	echo 12345678 >> /data/select.txt
	wifi_setup_bt.sh

	touch /home/pi/ivs/factoryflag
	rm /home/pi/ivs/nowififlag

	stop_SampleApp
}

function wait_wlan0()
{
	while ((true))
	do
		if [ -d "/var/run/wpa_supplicant" ]; then
			break;
		else
			sleep 1
		fi
	done
}

case "$1" in
	cuei)
		echo $2 > /home/pi/ivs/cuei
		sed -i "s/device_id/$2/g" /home/pi/ivs/ivsConfig.json
		sed -i "s/device_id/$2/g" /home/pi/ivs/factory_mode/ivsConfig.json
		sed -i "s/device_id/$2/g" ver.txt
		echo $2 > /home/pi/ivs/migu_id
		touch /home/pi/ivs/nowififlag
		sync
		;;
	configwifi)
		echo $2 > /data/select.txt
		echo $3 >> /data/select.txt
		sync
		wifi_setup_bt.sh &
		;;
	start)
		if [ -f /home/pi/ivs/factoryflag ]; then
			Updater /etc/ivs
		fi
		
		if [ ! -f /home/pi/ivs/ledfile ]; then
  			touch /home/pi/ivs/ledfile
		fi

		proc=`ps -ef | grep LEDCtrl | grep -v grep`
		if [ "$proc" == ""  ]; then
			LEDCtrl /home/pi/ivs/ledfile &
		fi

		#proc=`ps -ef | grep SampleApp | grep -v grep | grep -v SampleAppCtrl | grep -v S99SampleApp`
		#if [ "$proc" == ""  ]; then
		#	start_SampleApp
		#fi
		;;
	stop)
		stop_SampleApp
		killall -9 LEDCtrl
		rm /data/ivs.*
		rm /data/mme*
		rm /home/pi/ivs/mme*
		rm /home/pi/ivs/mdm*
		rm /home/pi/ivs/mtc*
		;;
	restart)
		stop_SampleApp
		start_SampleApp
		;;
	reset)
		stop_SampleApp
		rm /home/pi/ivs/*.db
		clear_wifi
		sync
		start_SampleApp
		;;
	factory)
		factory_init
		sync
		cd /etc/ivs
		nohup SampleApp factory_mode/ivsConfig.json >/dev/nul 2>&1 &
		;;
	test)
		wait_wlan0
		factory_init
		touch /home/pi/ivs/testflag
		sync
		cd /etc/ivs
		SampleApp factory_mode/ivsConfig.json
		;;
	wifibackup)
		rm -rf /data/etc
		rm -rf /data/lib
		mkdir -p /data/lib/modules/4.9.68/kernel/realtek/wifi
		mkdir -p /data/etc/wifi
		cp /lib/modules/4.9.68/kernel/realtek/wifi/8723ds.ko /data/lib/modules/4.9.68/kernel/realtek/wifi
		cp /etc/wifi/* /data/etc/wifi -rf
		cp /etc/wpa_supplicant.conf /data/etc
		urlmisc write $2
		;;
	update)
		cd /etc/ivs
		nohup Updater /etc/ivs >/dev/nul 2>&1 &
		;;
	wakeup)
		if [ -f "/home/pi/ivs/factoryflag" ]; then
			touch /etc/wakeup
			sleep 1
			rm /etc/wakeup
		fi
		sync
		;;
	connect)
		rm /home/pi/ivs/nowififlag
		if [ -f "/home/pi/ivs/factoryflag" ]; then
			touch /etc/connected
		fi
		sync
		;;
	play)
		killall gst-play-1.0
		killall aplay
		audioFile=$2
		if [ ${audioFile: -4} == ".wav" ]; then
			aplay -r16000 -c1 -fS16_LE $audioFile
		else
			#if [ ! -n "$3" ] ;then
			#gst-play-1.0 $2 `cat /home/pi/ivs/vol`
			#else
			#	gplay-mp3 $2 $3
		    #fi
		    gst-play-1.0 $2
		fi
		;;
	*)
		;;
esac

exit $?
