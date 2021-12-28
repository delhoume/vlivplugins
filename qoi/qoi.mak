# clang -O2 -D_CRT_SECURE_NO_DEPRECATE -DWIN32 -DWINDOWS -I. -I..\.. -c qoihandler.c
# link /dll /SUBSYSTEM:windows /out:qoi.dll qoihandler.o kernel32.lib ucrt.lib msvcrt.lib
# gives very small dll !


CC  = cl
LD  = link

VLIVDIR = ..\..

DEBUG=/Ox 

CFLAGS = /nologo /W3 $(DEBUG) /MD /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I$(VLIVDIR)

SYSLIBS = wininet.lib user32.lib gdi32.lib kernel32.lib comctl32.lib comdlg32.lib shlwapi.lib \
	  shell32.lib advapi32.lib version.lib strsafe.lib

all: qoi.dll

qoihandler.obj : qoihandler.c qoihandler.h
	$(CC) $(CFLAGS) /c qoihandler.c

qoi.dll: qoihandler.obj
	$(LD) /dll /map /out:qoi.dll qoihandler.obj


clean:
	del qoihandler.obj qoihandler.dll
