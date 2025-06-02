# onscripter-jh-odroid-go-advance
[WIP] My ONScripter-jh ‌ODROID-GO Advance and RK2020 with Ubuntu and RGB10 with Ubuntu port,

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
* USE this: LDFLAGS=-lrga ./configure --disable-video-opengl --enable-video-kmsdrm  
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
* Write to /opt/system/testsprite2.sh, for adding LD_LIBRARY_PATH to search new built libSDL2-2.0.so.0  
```
#!/bin/sh

cd /home/odroid/SDL-master/test/
export LD_LIBRARY_PATH=/home/odroid/SDL-master/test/
./testsprite2 > a.txt 2>&1
```
* need cd /opt/system/ && chmod +x ./testsprite2.sh
* and then launch testsprite2 from CONFIGURATION menu

## How to build onscripter-jh and launch /opt/system/ons.sh with CONFIGURATION menu      
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

winscp copy libSDL2-2.0.so.0.12.0 to /home/odroid/libSD2-2.0.so.0

unzip onscripter_cn_test.zip
cd onscripter_cn_test
LD_LIBRARY_PATH=/home/odroid ../jh10001-onscripter-jh-a11f51d5728f/onscripter
LD_LIBRARY_PATH=/home/odroid gdb ./onscripter

sudo apt install libgbm-dev libdrm-dev pkg-config
sudo apt install libx11-dev libxext-dev
```
* Write to ons.sh with ssh, and launch it from CONFIGURATION menu with game handheld system GUI    
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
