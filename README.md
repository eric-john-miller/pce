PCE - PC Emulator
==============================================================================

PCE is a collection of microcomputer emulators.'
This is a fork from Hampa Hug <hampa@hampa.ch> https://github.com/retrohun/pce

My purpose for forking this was to make modifications to support creating a working 1/3 scale Macintosh SE using a Raspberry Pi Zero in a hand crafted case.

Install
==============================================================================
The usual...

./configure

make

sudo make install

Based on this blog post by ToughDev, http://www.toughdev.com/content/2016/11/pcemacplus-the-ultimate-68k-classic-macintosh-emulator/, I used the following configuration options:

--with-x 

--with-sdl 

--enable-tun 

--enable-char-ppp 

--enable-char-tcp 

--enable-sound-oss 

--enable-char-slip 

--enable-char-pty 

--enable-char-posix 

--enable-char-termios 

CFLAGS=-I/usr/local/include/SDL2

Run
==============================================================================
pce-macplus -v -c mac-se.cfg -l pce.log -r

Where  

-c mac-se.cfg loads the config file mac-se.cfg

-r runs the emulator

-l pce.log writes to the log file pce.log

-v logs verbose comments
