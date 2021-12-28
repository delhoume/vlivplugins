CC  = cl
LD  = link

VLIVDIR = ..\..

WINDOWSCODECSDIR="c:\program files\microsoft sdks\windows\v6.0"

DEBUG=/Ox 

CFLAGS = /nologo /W3 $(DEBUG) /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I$(VLIVDIR) /I$(WINDOWSCODECSDIR)\include

SYSLIBS = ole32.lib $(WINDOWSCODECSDIR)\lib\windowscodecs.lib user32.lib

all: wichandler.dll

wichandler.obj: wichandler.cpp wichandler.h
	$(CC) $(CFLAGS) /c wichandler.cpp

wichandler.dll: wichandler.obj
	$(LD) $(LDDEBUG) /dll /out:wichandler.dll wichandler.obj $(SYSLIBS)

clean:
	del wichandler.obj wichandler.dll wichandler.lib
