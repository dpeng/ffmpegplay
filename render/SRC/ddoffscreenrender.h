#ifndef DDRENDER_H
#define DDRENDER_H

#include "render.h"
#include "videorender.h"
#include "colorspace.h"

#include <windows.h>
#include <ddraw.h>
#include <stdio.h>

//#define PRINT_LOG
#define LPMYDIRECTDRAW			LPDIRECTDRAW7
#define LPMYDIRECTDRAWSURFACE	LPDIRECTDRAWSURFACE7
#define MYDDSURFACEDESC			DDSURFACEDESC2
#define IID_MyDirectDraw		IID_IDirectDraw7

class MyVideoRender : public VideoRender
{
public:
	MyVideoRender(int type=0, bool automode = true);// 构造函数
	virtual ~MyVideoRender();// 析构函数

	// 接口函数

	virtual int init(
		int index, HWND hWnd, 
		int width, int height, 
		draw_callback cb, long udata);

	virtual int clean();

	virtual int render(
		unsigned char *py, unsigned char *pu, unsigned char *pv, 
		int width, int height);

	virtual void resize();
	virtual int SetString(char * pcStr,
					  int x,
					  int y, 
					  COLORREF OSDColor, 
					  bool bTransparent,
					  COLORREF BackColor,
					  HFONT OSDFont );

	virtual int SetShowRect(int left, int top, int right, int bottom);
	virtual int SetMaskRect(int maskid, int left, int top, int right, int bottom);
	virtual int CleanMaskRect();
	virtual HWND GetWind();

private:
	/***************************************************
	 * 关于DIRECTDRAW
	 ***************************************************/
	
	int initDirectDraw(); // 初始化DirectDraw
	HRESULT createDrawSurface(bool overlay=false); // 创建绘图表面
	HRESULT destroyDDObjects(); // 内部清理
    BOOL hasOverLaySupport(); // 检查是否支持OVERLAY
	BOOL hasFourCCSupport(LPMYDIRECTDRAWSURFACE lpdds); // 检查支持的YUV格式
	HRESULT createClipper(HWND hwnd); // 创建剪切区(窗口型才需要)
	void adjustRect(RECT &rcSrc, RECT &rcDest);
	DWORD convertGDIColor( COLORREF dwGDIColor);
	HRESULT RestoreAll();

	/****************************************************
	 * 绘图
	 ****************************************************/
	void drawMaskRect(
		unsigned char *py,unsigned char *pu,unsigned char *pv,
		int width, int height);

	void drawMotion(
		unsigned char *py,unsigned char *pu,unsigned char *pv,
		int width, int height);

	/********************************************************
	 *
	 ********************************************************/
	inline int DisplayBYGDI(
		unsigned char *py, unsigned char *pu, unsigned char *pv, 
		int width, int height);

	virtual int DisplayByOffscreen(
		unsigned char *py, unsigned char *pu, unsigned char *pv, 
		int width, int height);

	virtual int DisplayByOverlay(
		unsigned char *py, unsigned char *pu, unsigned char *pv, 
		int width, int height);
private:
	
	int		m_index;	// 给定编号
	int		m_type;		// 输出类型，主要是0-Offscreen和1-Overlay，GDI为后备总是可用.
	bool	m_automode;	// 自动选择，m_type标示的是真实的类型
	HWND	m_hWnd;		// 窗口句柄

	int		m_width;	// 要创建的绘图表面的宽度
	int		m_height;	// 要创建的绘图表面的高度
	int		m_screenWidth;	// 屏幕实际宽
	int		m_screenHeight;	// 屏幕实际高

	draw_callback	m_callback;	// 绘图回调函数
	long			m_userData;	// 用户自定义数据

	LPMYDIRECTDRAW        m_pDD;				// DIRECTDRAW环境句柄
	LPMYDIRECTDRAWSURFACE m_pDDSPrimary;		// 主表面
	LPMYDIRECTDRAWSURFACE m_pDDVideoSurface;	//绘图表面
	LPMYDIRECTDRAWSURFACE m_pDDVideoSurfaceBack;//绘图表面(后台,双缓冲模型,不必要)
	
	/*************************************************
	 * Overlay方式需要使用的参数
	/*************************************************/ 
    DDOVERLAYFX m_OverlayFX;
    DWORD m_dwOverlayFlags;
	DDBLTFX	m_ddbltfx;
	bool m_updataOverlay;
	DWORD m_colorKey;
	
	// Overlay支持标识
    BOOL m_hasOverlaySupport;
	
	DDCAPS  m_ddCaps; // 绘图能力信息

	BOOL m_hasFourCCSupport; // FourCC支持标识

	// 颜色空间转换函数
	color_convert_func m_colorConvert;

	RECT m_destRect;	// 窗口大小
	
	//OSD属性
	class OSD {
	public:
		OSD();
		~OSD(){}

		void setText(const char *string);
		void setPosition(int xpos, int ypos);
		void setFont(HFONT f);
		void setColor(COLORREF f, COLORREF b, bool tp);
		void drawText(HDC hdc);

		char text[256];	//要叠加的字符串,支持单行
		int x,y;
		HFONT font;
		COLORREF fg,bg;		// 前景色,背景色
		bool isTransparent;	//是否透明
	};
	
	OSD m_osd;

	// 局部显示属性
	RECT m_partRect;
	
	// 遮挡属性
	int m_showMask;
	RECT m_maskRect[MAX_MASK_COUNT];

	/***************************************************
	 * GDI 显示方式相关
	 ***************************************************/
	struct RGBPlane {
        RGBPlane();
        ~RGBPlane();
        
        unsigned char * data;
        int w, h;
        
        void resize(int width,  int height);
        unsigned char * getLine(int y);

		void copy(
			unsigned char *py,unsigned char *pu,unsigned char *pv,
			int width, int height);

    }  m_rgb;
};

#endif /* DDRENDER_H */

