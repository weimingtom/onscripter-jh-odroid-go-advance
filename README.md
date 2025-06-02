# onscripter-jh-odroid-go-advance
[WIP] My ONScripter-jh ‌ODROID-GO Advance and RK2020 with Ubuntu and RGB10 with Ubuntu port,

## References  
* https://forum.odroid.com/viewtopic.php?t=40167  
SDL2 | No available video device - ODROID  
```
https://github.com/OtherCrashOverride/sdl-go2
```
* https://forum.odroid.com/viewtopic.php?f=221&t=45184  
[experimental] SDL2 with Tear-Free Zero-copy Rotation - ODROID  
```
https://github.com/JohnnyonFlame/SDL-ge2d/tree/2.0.22  
https://github.com/OtherCrashOverride/libgou.git  
https://github.com/OtherCrashOverride/retrorun-go2/tree/gou  
https://github.com/OtherCrashOverride/SDL-ge2d/tree/gou-dev   
https://github.com/JohnnyonFlame/SDL-ge2d/commits/2.0.22  
ppssppp
```
* https://forum.odroid.com/viewtopic.php?t=37975  
SDL does not work？ - ODROID  
```
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install libx11-dev libsm-dev libxext-dev git cmake mercurial libudev-dev libdrm-dev zlib1g-dev pkg-config libasound2-dev alsa-utils htop bc ifupdown2 net-tools libssl1.0-dev mlocate bluez libfreetype6-dev libx11-xcb1 libxcb-dri2-0 
# Not sure if all of these are needed ^ 
sudo apt install libdrm-dev libgbm-dev
# Definitely needed these ^^ 
cd ~
hg clone http://hg.libsdl.org/SDL SDL2
cd SDL2
./configure --disable-video-opengl --enable-video-kmsdrm
make 
sudo make install 
cd test
./configure
make 
```
* https://forum.odroid.com/viewtopic.php?f=98&t=32173#p233475  
GBM Video Driver - Retro Gaming - Tinkering Image Howto - ODROID  
```
wget https://www.areascout.at/kodi/mali-x11-gbm-fbdev_19.0.6-1_armhf.deb
dpkg -i mali-x11-gbm-fbdev_19.0.6-1_armhf.deb

Checkout SDL2, build and install it:

cd ~
hg clone http://hg.libsdl.org/SDL SDL2
cd SDL2
./configure --disable-video-opengl --enable-video-kmsdrm

https://github.com/hrydgard/ppsspp.git  

```
* https://discourse.libsdl.org/t/sdl2-not-working-without-x11-mode-in-odroid/24669/9  
Sdl2 not working without x11 mode in odroid - SDL Development - Simple Directmedia Layer  

## Three good SDL2 Modifications for ‌ODROID-GO Advance (see below How to build my own libSDL2), all are good      
* (I always use this) https://github.com/AreaScout/SDL    
* (Can be used but I don't use) https://github.com/OtherCrashOverride/sdl-go2/tree/master  
* (Can be used but I don't use) https://github.com/OtherCrashOverride/sdl-go2/tree/tearing  

## How to build my own libSDL2-2.0.so.0.12.0 and launch /opt/system/testsprite2.sh with CONFIGURATION menu 
* DO NOT USE SYSTEM's libSDL2-2.0.so.0.8.0, it does not work          
* Use this prebuilt so file directly, https://github.com/weimingtom/onscripter-jh-odroid-go-advance/blob/master/out_bin/libSDL2-2.0.so.0  
* Build it manually, see https://github.com/weimingtom/onscripter-jh-odroid-go-advance/blob/master/vendor/testsprite2_v1.txt
* And see https://github.com/weimingtom/onscripter-jh-odroid-go-advance/blob/master/vendor/SDL-master_v1_v1.txt  
* Don't use ./configure directly, https://github.com/weimingtom/onscripter-jh-odroid-go-advance/blob/master/vendor/AreaScout_SDL-master.zip
* You can use AreaScout_SDL-master.zip or sdl-go2-master.zip or sdl-go2-tearing.zip all available
* USE this to configure: LDFLAGS=-lrga ./configure --disable-video-opengl --enable-video-kmsdrm  
```
ubuntu-18.04.3-4.4-es-odroid-goA-20200923.img
odroid/odroid

sudo apt install libgbm-dev libdrm-dev pkg-config
sudo apt install libx11-dev libxext-dev

unzip AreaScout_SDL-master.zip  
cd SDL-master/  
(undefined reference to `c_RkRgaInit')
LDFLAGS=-lrga ./configure --disable-video-opengl --enable-video-kmsdrm
(don't use ./configure as possible)

cd test
gcc -o testsprite2 testsprite2.c -g -O2 -D_REENTRANT -I/usr/include/SDL2 -DHAVE_OPENGLES2 -DHAVE_OPENGL -g -lSDL2_test -lSDL2  

(actually libSDL2-2.0.so.0.12.0)
(don't do this) sudo cp /home/odroid/libSDL2-2.0.so.0.8.0 /usr/lib/aarch64-linux-gnu/.
(don't do this) sudo cp /home/odroid/libSDL2-2.0.so.0.8.0_old /usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0.8.0
(need libgbm-dev libdrm-dev pkg-config  libx11-dev libxext-dev ???)
Replace /usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0.8.0 with my built libSDL2-2.0.so.0.12.0
(better to copy libSDL2-2.0.so.0.12.0 to /home/odroid/SDL-master/test/libSDL2-2.0.so.0)
```
* Write to /opt/system/testsprite2.sh (with WinSCP or SSH/PuTTY), for adding LD_LIBRARY_PATH to search new built libSDL2-2.0.so.0  
```
#!/bin/sh

cd /home/odroid/SDL-master/test/
export LD_LIBRARY_PATH=/home/odroid/SDL-master/test/
./testsprite2 > a.txt 2>&1
```
* need cd /opt/system/ && chmod +x ./testsprite2.sh
* and then launch testsprite2 from CONFIGURATION menu

## How to build onscripter-jh and launch /opt/system/ons.sh with CONFIGURATION menu      
* (See upper, how to build my own libSDL2-2.0.so.0.12.0) You must first copy your own libSDL2-2.0.so.0 to /home/odroid, see LD_LIBRARY_PATH=/home/odroid
```
ubuntu-18.04.3-4.4-es-odroid-goA-20200923.img
odroid/odroid

sudo apt update
sudo apt install unzip make gcc g++ nano gdb
sudo apt install libsdl2-dev liblua5.1-0-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libbz2-dev libfontconfig1-dev libogg-dev libvorbis-dev  

unzip jh10001-onscripter-jh-a11f51d5728f.zip
cp Makefile.Linux Makefile
nano Makefile
(remove lua and SIMD depends)
make

cp onscripter ../onscripter_cn_test/  

winscp copy libSDL2-2.0.so.0.12.0 to /home/odroid/libSD2-2.0.so.0

unzip onscripter_cn_test.zip
cd onscripter_cn_test
LD_LIBRARY_PATH=/home/odroid ../jh10001-onscripter-jh-a11f51d5728f/onscripter
LD_LIBRARY_PATH=/home/odroid gdb ./onscripter

sudo apt install libgbm-dev libdrm-dev pkg-config
sudo apt install libx11-dev libxext-dev
```
* Write to ons.sh with SSH/PuTTY (or with WinSCP), and launch it from CONFIGURATION menu with game handheld system GUI    
```
#!/bin/sh

cd /home/odroid/onscripter_cn_test
export LD_LIBRARY_PATH=/home/odroid
./onscripter > a.txt 2>&1
```
* cd /opt/system
* chmod +x ./ons.sh
* reboot
* And then launch /opt/system/ons.sh with CONFIGURATION menu  
