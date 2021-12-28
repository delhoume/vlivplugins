CC  = cl
LD  = link

DEBUG=/Ox

CFLAGS = /nologo /MD /W3 $(DEBUG) /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I../../vliv\src /Ilibwebp-0.2.1-windows-x86/dev/Include

SYSLIBS = wininet.lib user32.lib gdi32.lib kernel32.lib comctl32.lib comdlg32.lib shlwapi.lib \
	  shell32.lib advapi32.lib version.lib strsafe.lib libwebp-0.2.1-windows-x86/dev/Lib/libwebp.lib

all: webp.dll

webp.obj : webp.c webp.h
	$(CC) $(CFLAGS) /c webp.c

webp.dll: webp.obj
	$(LD)  /dll /out:webp.dll webp.obj $(SYSLIBS)

clean:
	del webp.obj webp.dll
