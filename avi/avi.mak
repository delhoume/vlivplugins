CC  = cl
LD  = link

VLIVDIR = ..\..\vliv\src

DEBUG=/Ox 

CFLAGS = /nologo /W3 $(DEBUG) /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I$(VLIVDIR)

SYSLIBS = wininet.lib user32.lib gdi32.lib kernel32.lib comctl32.lib comdlg32.lib shlwapi.lib \
	  shell32.lib advapi32.lib version.lib strsafe.lib vfw32.lib 

all: avi.dll

avi.obj : avi.c avi.h
	$(CC) $(CFLAGS) /c avi.c

avi.dll: avi.obj
	$(LD) /dll /out:avi.dll avi.obj $(SYSLIBS)

clean:
	del avi.obj avi.dll
