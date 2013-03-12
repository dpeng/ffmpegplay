#ifndef DHRENDERMANAGER_H
#define DHRENDERMANAGER_H

#include "render.h"
#include "bmpwriter.h"

#include <windows.h>

class CAudioRender;
class DSRender;
struct VideoRender;
struct AudioRender;
class DSRenderNoLoopBuffer;
	
#define RENDERITEMCOUNT 256
#define DIRECTSOUND_WITHOUT_LOOPBUFFER 256

class DhRenderManager
{
public:
	DhRenderManager();
	virtual ~DhRenderManager();

	int init(HWND hWnd, char *path);
	
	int open(int index, HWND hWnd, int width, int height, 
		draw_callback cb, long udata, VideoRenderMethod vrm, DWORD ck);
	
	int close();
	int close(int index);

	// 代理接口
	int renderVideo(int index, 
		unsigned char *py, 
		unsigned char *pu, 
		unsigned char *pv, 
		int width, int height);
	
	bool setvolume(int index,DWORD lVolume);

	void setaudiorenderstyle( int style);
	
	int renderAudio(int index, unsigned char *pcm, int len, 
		int bitsPerSample, int samplesPerSecond);

	int grabPicture(int index, 
		const char *path, 
		const char *basename, 
		int format, int count);
	
	DWORD getvolume(int index );

	int showstring(int index, 
				   char *pString,
				   int x, int y,  
				   COLORREF OSDColor, 
				   bool bTransparent,
				   COLORREF BackColor,
				   HFONT OSDFont);

	int showrect(int index, int left, int top, int right, int bottom);
	int showmask(int index, int enabled, int maskid, int left, int top, int right, int bottom);

private:
	struct RenderItem 
	{
		VideoRender	*m_videoRender;
		CAudioRender *m_audioRender;
		HANDLE VideoRenderLock;
		HANDLE AudioRenderLock;
	};
	
	RenderItem m_items[RENDERITEMCOUNT];
	BmpWriter m_bmpWriter;

	int m_grabIndex;// 当前抓图通道
	int m_grabCount; // 当前抓图数
	int m_grabFinished; // 完成的抓图数
	int m_grabFormat;
	char m_baseName[256]; // 

	int m_AudioRenderStyle;

	int m_indexUseOverlay;

	char m_jpegPath[256];

	bool m_inited;
};

#endif /* DHRENDERMANAGER_H */





















