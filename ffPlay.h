#pragma once
#include <windows.h>
#include <render.h>
#include <assert.h>
#include <tchar.h>
#include <stdio.h>
#include <atlstr.h>
extern "C" 
{
#include "avcodec.h"
#include "avformat.h"
#include "cmdutils.h "
}
typedef struct VideoState {
	AVStream *audio_st;
	AVStream *video_st;
	AVFormatContext *ic;
	int video_stream;
	int audio_stream;
	SwsContext *img_convert_ctx;
} VideoState;

class CffPlay
{
public:
	CffPlay(void);
	~CffPlay(void);
	void playMpegFile(char* fileName, HWND hWnd);
	void playPause();
	void playClose();
	double playGetTotalTime();
	double playGetCurrentTime();
	void playSetSeekPosition(unsigned int pos);
	void playOnlyAudio(BOOL isOnlyAudio);
private:
	AVPacket m_flushPkt;
	HANDLE m_playFFMpegProcessHandler;
	VideoState m_currentStream;
	static DWORD WINAPI playFFMpegPro(LPVOID pParam);
	int stream_component_open(VideoState *is, int stream_index);
};
