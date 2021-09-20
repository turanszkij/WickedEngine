# Build win32 executable to verify unit tests pass when run
# against XAudio2 on Windows.

WINEROOT = /usr/include/wine

CROSSCC = x86_64-w64-mingw32-gcc

CFLAGS += -g -Wall

all: faudio_tests.exe

faudio_tests.exe: xaudio2.c
	$(CROSSCC) $(CFLAGS) -Wall -I$(WINEROOT)/windows/ -o $@ $< -lole32
