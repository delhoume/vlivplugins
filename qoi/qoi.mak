# clang -O2 -D_CRT_SECURE_NO_DEPRECATE -DWIN32 -DWINDOWS -I. -I..\.. -c qoihandler.c
# link /dll /SUBSYSTEM:windows /out:qoi.dll qoihandler.o kernel32.lib ucrt.lib msvcrt.lib
# gives very small dll !


CC  = cl
LD  = link

VLIVDIR = ..\..\vliv\src

DEBUG=/Ox 

CFLAGS = /nologo /W3 $(DEBUG) /MD /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I$(VLIVDIR)

all: qoi.dll

qoihandler.obj : qoihandler.c qoihandler.h qoi.h
	$(CC) $(CFLAGS) /c qoihandler.c

qoi.dll: qoihandler.obj
	$(LD) /dll /map /out:qoi.dll qoihandler.obj


clean:
	del qoihandler.obj qoihandler.dll
