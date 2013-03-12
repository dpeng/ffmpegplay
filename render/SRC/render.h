/*************************************************************************
创建日期: 2005年12月2日
作者： 冯江(feng_jiang@dhmail.com)

2005年12月2日： 创建
*************************************************************************/

#ifndef DHRENDERLIB_H
#define DHRENDERLIB_H

#include <windows.h>

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the RENDER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// RENDER_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef RENDER_EXPORTS
#define RENDER_API __declspec(dllexport)
#else
#define RENDER_API __declspec(dllimport)
#endif

// 错误
#define RENDER_ERR_NOERROR	0

// 致命错误,或者说一般性错误均返回-1
// 致命错误包括内存错误
#define RENDER_ERR_FATAL	(-1)
#define RENDER_ERR_MEMORY	(-1)

// 被使用,警告性错误
#define RENDER_ERR_WARNING	(-2)
#define RENDER_ERR_USED		(-2)

// 视频显示相关错误,包括DirectDraw错误等
#define RENDER_ERR_VIDEO	(-3)
#define RENDER_ERR_DDRAW	(-3)

// 音频相关错误
#define RENDER_ERR_AUDIO	(-4)

//通道索引错误
#define RENDER_ERR_CHANNEL_INDEX	(-5)

enum VideoRenderMethod
{
	ByDDOffscreen = 0,	// 默认, DirectDraw Offscreen方式,支持OSD
	ByDDOverlay = 1,	// Overlay方式,不支持OSD
	ByGDI = 2			// 总是备选,支持OSD
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
/*!
 * 抓图功能
 * 参数描述：
 * handle: 句柄
 * path: 路径
 * basename: 基本文件名，不包括扩展名，扩展名根据格式给定
 * format: 格式, 0是bmp，1是jpg
 * count：连续抓图数
**/
RENDER_API int render_grab_picture(int index, const char *path = "", const char *basename = "DHGRAB", int format = 0, int count = 1);

/*!
 * 获取版本号.
 */
RENDER_API int render_version(int *major, int *minor, int *patcher);

RENDER_API int render_set_volume(int index, DWORD lVolume );// 范围: 0 -- 0xFFFF, 0 音量最小

//设置音频输出方式，有2种如下，(默认是PCM_OUT)
//DIRECTSOUND_WITH_LOOPBUFFER 
//PCM_OUT 
RENDER_API int render_audio_set_style(int style);

//字符叠加
RENDER_API int render_show_string(int index, //通道号
								  char *pString, //字符串
								  int x, //起始x坐标
								  int y, //起始y坐标
								  COLORREF OSDColor, //字体颜色
								  bool bTransparent, //是否透明
								  COLORREF BackColor, //背景颜色
								  HFONT OSDFont); //字体样色

// 局部放大,给定的参数为原始YUV图像的一个区域
RENDER_API int render_show_rect(int index, int left, int top, int right, int bottom);

// 局部放大,给定的参数为原始YUV图像的一个区域
RENDER_API int render_show_mask(int index, int enabled, int maskid, int left, int top, int right, int bottom);

#endif /* DHRENDERLIB_H */
