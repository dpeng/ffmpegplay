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
	double playGetTotalTime();
	double playGetCurrentTime();
	void playSetSeekPosition(unsigned int pos);
	void playOnlyAudio(bool isOnlyAudio);
private:
	HANDLE m_playFFMpegProcessHandler;
	static DWORD WINAPI playFFMpegPro(LPVOID pParam);
};
