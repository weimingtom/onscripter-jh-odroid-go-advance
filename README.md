# onscripter-jh-odroid-go-advance
[WIP] My ONScripter-jh ‌ODROID-GO Advance and RK2020 with Ubuntu and RGB10 with Ubuntu port,

## How to build my own libSDL2-2.0.so.0.12.0 and launch /opt/system/testsprite2.sh with CONFIGURATION menu 
* DO NOT USE SYSTEM's libSDL2-2.0.so.0.8.0, it does not work          
* Use this prebuilt so file directly, https://github.com/weimingtom/onscripter-jh-odroid-go-advance/blob/master/out_bin/libSDL2-2.0.so.0  
* Build it manually, see https://github.com/weimingtom/onscripter-jh-odroid-go-advance/blob/master/vendor/testsprite2_v1.txt    
* Don't use ./configure directly, https://github.com/weimingtom/onscripter-jh-odroid-go-advance/blob/master/vendor/SDL2-2.32.6.tar.gz    
* USE this: LDFLAGS=-lrga ./configure --disable-video-opengl --enable-video-kmsdrm  
```
ubuntu-18.04.3-4.4-es-odroid-goA-20200923.img
odroid/odroid

sudo apt install libgbm-dev libdrm-dev pkg-config
sudo apt install libx11-dev libxext-dev

tar xzf SDL2-2.32.6.tar.gz  
cd SDL2-2.32.6/  
(undefined reference to `c_RkRgaInit')
LDFLAGS=-lrga ./configure --disable-video-opengl --enable-video-kmsdrm
(don't use ./configure as possible)

cd test
gcc -o testsprite2 testsprite2.c -g -O2 -D_REENTRANT -I/usr/include/SDL2 -DHAVE_OPENGLES2 -DHAVE_OPENGL -g -lSDL2_test -lSDL2  

(actally libSDL2-2.0.so.0.12.0)
(don't do this) sudo cp /home/odroid/libSDL2-2.0.so.0.8.0 /usr/lib/aarch64-linux-gnu/.
(don't do this) sudo cp /home/odroid/libSDL2-2.0.so.0.8.0_old /usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0.8.0
(need libgbm-dev libdrm-dev pkg-config  libx11-dev libxext-dev ???)
Replace /usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0.8.0 with my built libSDL2-2.0.so.0.12.0
(better to copy libSDL2-2.0.so.0.12.0 to /home/odroid/SDL-master/test/libSDL2-2.0.so.0)
```
* Write to /opt/system/testsprite2.sh
```
#!/bin/sh

cd /home/odroid/SDL-master/test/
export LD_LIBRARY_PATH=/home/odroid/SDL-master/test/
./testsprite2 > a.txt 2>&1
```
* need cd /opt/system/ && chmod +x ./testsprite2.sh
* and then launch testsprite2 from CONFIGURATION menu
* libSDL2-2.0.so.0

## How to launch onscripter-jh   
```
ubuntu-18.04.3-4.4-es-odroid-goA-20200923.img
odroid/odroid
```
