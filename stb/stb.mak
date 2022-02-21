CC  = cl
LD  = link

VLIVDIR = ..\..\vliv\src
STBDIR = ..\..\stb

DEBUG=/Ox 

CFLAGS = /nologo /W3 $(DEBUG) /MD /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I$(VLIVDIR) /I$(STBDIR)


SYSLIBS = wininet.lib user32.lib gdi32.lib kernel32.lib comctl32.lib comdlg32.lib shlwapi.lib \
	  shell32.lib advapi32.lib version.lib strsafe.lib

all: stb.dll

stbhandler.obj : stbhandler.c stbhandler.h
	$(CC) $(CFLAGS) /c stbhandler.c

stb.dll: stbhandler.obj
	$(LD) /dll /map /out:stb.dll stbhandler.obj


clean:
	del stbhandler.obj stb.dll
