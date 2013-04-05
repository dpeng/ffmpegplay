
#ifndef RENDERLIB_H
#define RENDERLIB_H

#include <windows.h>

#ifdef RENDER_EXPORTS
#define RENDER_API __declspec(dllexport)
#else
#define RENDER_API __declspec(dllimport)
#endif

#define RENDER_ERR_NOERROR	0
#define RENDER_ERR_FATAL	(-1)
#define RENDER_ERR_MEMORY	(-1)
#define RENDER_ERR_WARNING	(-2)
#define RENDER_ERR_USED		(-2)
#define RENDER_ERR_VIDEO	(-3)
#define RENDER_ERR_DDRAW	(-3)
#define RENDER_ERR_AUDIO	(-4)
#define RENDER_ERR_CHANNEL_INDEX	(-5)

enum VideoRenderMethod
{
	ByDDOffscreen = 0,
	ByDDOverlay = 1,
	ByGDI = 2
};

enum AudioRenderMethod
{
	DIRECTSOUND_WITH_LOOPBUFFER = 0,
	PCM_OUT = 1
};

typedef void (__stdcall *draw_callback)(int index, HDC hDc, long dwUser);

RENDER_API int render_init(long hWnd=0);
RENDER_API int render_open(int index, HWND hWnd, int width, int height, draw_callback dcb, long udata, VideoRenderMethod vrm, DWORD ck);
RENDER_API int render_close_all();
RENDER_API int render_close(int index);
RENDER_API int render_video(int index, unsigned char *py, unsigned char *pu, unsigned char *pv, int width, int height);
RENDER_API int render_audio(int index, unsigned char *pcm, int len, int bits, int sampling);

RENDER_API int render_grab_picture(int index, const char *path = "", const char *basename = "DHGRAB", int format = 0, int count = 1);
RENDER_API int render_version(int *major, int *minor, int *patcher);

RENDER_API int render_set_volume(int index, DWORD lVolume );
RENDER_API int render_audio_set_style(int style);
RENDER_API int render_show_string(int index,
								  char *pString,
								  int x,
								  int y,
								  COLORREF OSDColor,
								  bool bTransparent,
								  COLORREF BackColor,
								  HFONT OSDFont);

RENDER_API int render_show_rect(int index, int left, int top, int right, int bottom);
RENDER_API int render_show_mask(int index, int enabled, int maskid, int left, int top, int right, int bottom);

#endif
