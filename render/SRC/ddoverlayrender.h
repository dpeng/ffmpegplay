#ifndef DDOVERLAYRENDER_H
#define DDOVERLAYRENDER_H

#include "render.h"
#include "videorender.h"
#include "colorspace.h"

#include <windows.h>
#include <ddraw.h>
#include <stdio.h>

#define LPMYDIRECTDRAW			LPDIRECTDRAW7
#define IMyDirectDrawSurface	IDirectDrawSurface7
#define LPMYDIRECTDRAWSURFACE	LPDIRECTDRAWSURFACE7
#define MYDDSURFACEDESC			DDSURFACEDESC2
#define MYDDSCAPS				DDSCAPS2

class DDOverlayRender : public VideoRender
{
public:
	// 构造函数
    DDOverlayRender(DWORD color);

	// 析构函数
	virtual ~DDOverlayRender();

	// 接口函数

	virtual int init(int index, HWND hWnd, int width, int height, draw_callback cb, long udata);
	virtual int clean();
    	virtual int render(unsigned char *py, unsigned char *pu, unsigned char *pv, int width, int height);
	virtual void resize();
	virtual int SetString(char * pcStr,
						  int x,
						  int y, 
						  COLORREF OSDColor, 
						  bool bTransparent,
						  COLORREF BackColor,
						  HFONT OSDFont);
	virtual int SetShowRect(int left, int top, int right, int bottom);
	virtual int SetMaskRect(int maskid, int left, int top, int right, int bottom);
	virtual int CleanMaskRect();
	virtual HWND GetWind();
private:
	// 初始化DirectDraw
    int initDirectDraw();

	// 创建绘图表面
    HRESULT createDrawSurface();

	// 创建文字表面, 目前不用了.
	// HRESULT createTextSurface(HWND hWnd, const char *text);

	// 内部清理
    HRESULT destroyDDObjects();

	// 检查是否支持Overlay
    BOOL hasOverLaySupport();

	// 检查是否支持FourCC (四字符代码)
    BOOL hasFourCCSupport(LPMYDIRECTDRAWSURFACE lpdds);
    
	// 颜色操作函数
    DWORD colorMatch(IMyDirectDrawSurface * pdds, COLORREF rgb);
    DWORD convertGDIColor(COLORREF dwGDIColor);
	
	HRESULT createClipper(HWND hwnd);

	void adjustRect(RECT &rcSrc, RECT &rcDest);
	void ShowString();
private:
	// 给定的参数
	int				m_index;

	DWORD			m_colorKey;

	// 窗口句柄
    HWND			m_hWnd;
	
	int				m_width;	// 要创建的绘图表面的宽度
	int				m_height;	// 要创建的绘图表面的高度
	
	draw_callback m_callback;
	long m_userData;

private: // DirectDraw 相关
	// DirectDraw对象
    LPMYDIRECTDRAW        m_pDD;						// DirectDraw对象
    LPMYDIRECTDRAWSURFACE m_pDDSPrimary;				// 主表面

    LPMYDIRECTDRAWSURFACE m_pDDSVideoSurface;		// 绘图表面
    LPMYDIRECTDRAWSURFACE m_pDDSVideoSurfaceBack;	// 后台绘图表面

	// Overlay方式需要使用的参数
    DDOVERLAYFX m_OverlayFX;
    DWORD m_dwOverlayFlags;
	DDBLTFX	m_ddbltfx;
	bool m_updataOverlay;
 
	// 绘图能力信息
    DDCAPS  m_ddCaps;
    
	// Overlay支持标识
    BOOL m_hasOverlaySupport;

	// FourCC支持标识
    BOOL m_hasFourCCSupport;

	color_convert_func m_colorConvert;

	//目的表面在主表面上的位置
	RECT m_destRect;
	RECT rcSrc;

	//屏幕宽和高
	int	m_screenWidth;
	int	m_screenHeight;

	// 局部显示属性
	int m_showPart; // 局部放大
	RECT m_partRect;
	
	int m_showMask;
	RECT m_maskRect[MAX_MASK_COUNT];
	
	unsigned char m_tempYuvData[704*576*3/2];
};

#endif /* DDOVERLAYRENDER_H */
