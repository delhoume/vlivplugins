# clang -O2 -D_CRT_SECURE_NO_DEPRECATE -DWIN32 -DWINDOWS -I. -I..\.. -c dzhandler.c
# link /dll /SUBSYSTEM:windows /out:dz.dll dzhandler.o kernel32.lib ucrt.lib msvcrt.lib
# gives very small dll !


CC  = cl
LD  = link

VLIVDIR = ..\..\vliv\src

DEBUG=/Ox 
SYSLIBS = user32.lib winhttp.lib 

CFLAGS = /nologo /W3 $(DEBUG) /MD /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I$(VLIVDIR)

all: dz.dll

dzhandler.obj : dzhandler.c dzhandler.h
	$(CC) $(CFLAGS) /c dzhandler.c

dz.dll: dzhandler.obj
	$(LD) /dll /map /out:dz.dll dzhandler.obj $(SYSLIBS)


clean:
	del dzhandler.obj dzhandler.dll
