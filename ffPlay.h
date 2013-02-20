#pragma once
#include <windows.h>

class CffPlay
{
public:
	CffPlay(void);
	~CffPlay(void);
	void playMpegFile(char* fileName, RECT* rc, char* variable);
	void playPause();
	void playClose();
	static DWORD WINAPI playFFMpegPro(LPVOID pParam);
};
