#ifndef GDIRENDER_H
#define GDIRENDER_H

#include "videorender.h"

#include <windows.h>

class GDIRender : public VideoRender
{
public:
	GDIRender();
	virtual ~GDIRender();

	virtual int init(int index, HWND hWnd, int width, int height, draw_callback cb, long udata);
	virtual int clean();
	virtual int render(unsigned char *py, unsigned char *pu, unsigned char *pv, int width, int height);
	virtual void resize();
	virtual BOOL SetString(char * pcStr,
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
	inline void colorConvert(
		unsigned char *py, 
		unsigned char *pu, 
		unsigned char *pv, 
		int width, int height);

	void ShowString(HDC hDC);

private:
	int m_index;
	HWND m_hWnd;
	draw_callback m_cb;
	long m_userData;

	bool m_flip;
	bool m_rgbReverseOrder;

	struct RGBPlane 
	{
        RGBPlane();
        ~RGBPlane();
        
        unsigned char * data;
        int w, h;
        
        void resize(int width,  int height);
        unsigned char * getLine(int y);
    }  m_rgb;

	//OSD属性
	char m_pOSDString[256];//要叠加的字符串
	int m_OSDStringX;//起始x坐标
	int m_OSDStringY;//起始y坐标
	HFONT m_OSDFont;//字体
	COLORREF m_OSDColor;//颜色
	COLORREF m_BackColor;//背景色
	bool m_bTransparent;//是否透明
	HFONT m_FontPre;//保留前面的字体样式
	
	// 局部显示属性
	RECT m_partRect;
	
	int m_showMask;
	RECT m_maskRect[MAX_MASK_COUNT];
};

#endif /* GDIRENDER_H */
