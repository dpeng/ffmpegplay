#include "gdirender.h"

#include <stdio.h>

#include "colorspace.h"

/***********************************************************
 * RGBPlane: 24bits/32bits
 ***********************************************************/
GDIRender::RGBPlane::RGBPlane()
: data(0), w(0), h(0)
{
}

GDIRender::RGBPlane::~RGBPlane()
{
    if (data) {
        free(data);
    }
}

#define BYTES 3

void GDIRender::RGBPlane::resize(int width, int height)
{
	int w1=width;
	int h1=height;
	int size=w1*h1*BYTES;

	if (w1<=0||h1<=0) return ;

    if (data) {
        if (w==w1&&h1==h) {
 			return;
		}

		free(data);
        data = (unsigned char *) malloc(size);
    } else {
        data = (unsigned char *) malloc(size);	
    }
    
    memset(data, 0, size);
    
    w = w1;
    h = h1;
}

unsigned char *GDIRender::RGBPlane::getLine(int y)
{
    return &data[y*w*BYTES];
};

/***********************************************************
 *
 ***********************************************************/
GDIRender::GDIRender()
{
	m_hWnd	= 0;
	m_cb	= 0;
	m_userData = 0;

	m_flip				= true;
	m_rgbReverseOrder	= true;

	m_pOSDString[0] = NULL;
	m_OSDStringX = 0;
	m_OSDStringY = 0;  
	m_OSDColor = RGB(255, 255, 255);
	m_BackColor = RGB(0, 0, 0);
	m_bTransparent = true;
	m_OSDFont = 0;

	m_partRect.left = -1;
	m_partRect.top = -1;
	m_partRect.bottom = -1;
	m_partRect.right = -1;
	
	for (int i = 0;i<MAX_MASK_COUNT;++i)
	{
		m_maskRect[i].left = -1;
		m_maskRect[i].right = -1;
		m_maskRect[i].bottom = -1;
		m_maskRect[i].top = -1;
	}
	
	m_showMask = 0;
}

GDIRender::~GDIRender()
{
}

int GDIRender::init(int index, HWND hWnd, int width, int height, draw_callback cb, long udata)
{
	m_index	= index;
	m_hWnd	= hWnd;
	m_cb	= cb;
	m_userData = udata;

	return 0;
}

/************************************************************************
 * 1. YUV --> RGB24.
 * 2. RGB24 Select to Memory DC.
 * 3. StretchBlt.
 ***********************************************************************/
int GDIRender::render(unsigned char *py, unsigned char *pu, unsigned char *pv, int width, int height)
{
	int err = 0;
	HDC hDC = 0;
	HDC hMemDC = 0 ;
	HBITMAP hBitMap = 0;
	
	// 1. YUV --> RGB24

	m_rgb.resize(width, height);

	if (m_showMask) {
		// 处理YUV数据
		int ysize = width*height;
		int uvsize = ysize/4;
		
		int xpos,xwidth;
		
		for (int i = 0;i<MAX_MASK_COUNT;++i) {
			if (m_maskRect[i].left>0&&m_maskRect[i].top>0&&m_maskRect[i].bottom>0&&m_maskRect[i].right>0) 
			{
				for (int j=m_maskRect[i].top;j<=m_maskRect[i].bottom;j++)
				{
					xpos = j*width+m_maskRect[i].left;
					xwidth = m_maskRect[i].right-m_maskRect[i].left;
					memset(py+xpos, 0, xwidth);
					
					if (j%2)
					{
						xpos = j/2*width/2+m_maskRect[i].left/2;
						xwidth = m_maskRect[i].right/2-m_maskRect[i].left/2;
						memset(pu+xpos, 128, xwidth);
						
						xpos = j/2*width/2+m_maskRect[i].left/2;
						memset(pv+xpos, 128, xwidth);
					}
				}
			}
		}

		YUV_TO_RGB24(py,pu, pv,width,m_rgb.getLine(0), width, -height, width*BYTES);
		
	} else {
		YUV_TO_RGB24(py,  pu, pv,width,m_rgb.getLine(0), width, -height, width*BYTES);

	}

	//获得目的绘图窗口大小
	//如果窗口最小化了，就不做处理，节约CPU资源
	RECT rcDest;
	GetClientRect(m_hWnd, &rcDest);
	if (rcDest.left<0&&rcDest.top<0&&rcDest.right<0&&rcDest.bottom<0) {
		return 0; 
	}
	
	//获得窗口设备,窗口DC为目标DC
	hDC = GetWindowDC(m_hWnd);
	if( hDC == NULL ) {
		err = -1;
		goto ret;
	}

	//建立一个和窗口设备兼容的内存设备
	hMemDC = CreateCompatibleDC(hDC);
	if( hMemDC == NULL ) {
		ReleaseDC(m_hWnd, hDC);
		err = -1;
		goto ret;
	}

	// 创建一个跟源一样大小的位图对象, 将位图对象选进内存DC
	// 准备COPY
	
	BITMAPINFO  bitmapinfo;
    BITMAPFILEHEADER bmpHeader;
	
    bmpHeader.bfType = 'MB';
    bmpHeader.bfSize = width*height*BYTES + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpHeader.bfReserved1 = 0;
    bmpHeader.bfReserved2 = 0;
    bmpHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bitmapinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapinfo.bmiHeader.biWidth = width;
    bitmapinfo.bmiHeader.biHeight = height;
    bitmapinfo.bmiHeader.biPlanes = 1;
    bitmapinfo.bmiHeader.biBitCount = BYTES*8;
    bitmapinfo.bmiHeader.biCompression = BI_RGB;
    bitmapinfo.bmiHeader.biSizeImage = width*height*BYTES;
    bitmapinfo.bmiHeader.biXPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biClrUsed = 0;
    bitmapinfo.bmiHeader.biClrImportant = 0;
	
	//创建一个和窗口设备兼容的位图对象
	hBitMap = CreateCompatibleBitmap(hDC, rcDest.right-rcDest.left, rcDest.bottom - rcDest.top);
	if( hBitMap == NULL ) {
		DeleteDC(hMemDC);
		ReleaseDC(m_hWnd, hDC);
		err = -1;
		goto ret;
	}
	
	//把位图对象选进内存设备,至于这样做的原因,可以查看MSDN关于
	//创建内存设备的帮助
	SelectObject(hMemDC, hBitMap);

	//设置内存设备的StretchBIT模式
    SetStretchBltMode(hMemDC,COLORONCOLOR);

	//把源图像StretchBlt到内存设备中(有缩放)
    StretchDIBits(hMemDC, 0, 0, rcDest.right - rcDest.left ,rcDest.bottom - rcDest.top ,
        0, 0,m_rgb.w,m_rgb.h, m_rgb.getLine(0), &bitmapinfo, DIB_RGB_COLORS, SRCCOPY); 

	// 局部放大

	//OSD叠加,在内存设备上写字
	// ShowString(hMemDC); 
	if (m_cb) {
		m_cb(m_index, hMemDC, m_userData);
	}

	int srcX, srcY, srcW, srcH;
	int dstW, dstH;

	dstW = rcDest.right-rcDest.left;
	dstH = rcDest.bottom-rcDest.top;
	
	if (m_partRect.left>=0&&m_partRect.right>0&&m_partRect.top>=0&&m_partRect.bottom>0) 
	{
		// 变换为目标矩形的坐标
		float scalex = float(m_partRect.right-m_partRect.left)/dstW;
		float scaley = float(m_partRect.bottom-m_partRect.top)/dstH;

		srcX = (int)(m_partRect.left * scalex);
		srcY = (int)(m_partRect.top * scaley);
		srcW = (int)((m_partRect.right - m_partRect.left) * scalex);
		srcH = (int)((m_partRect.bottom - m_partRect.top) * scaley);
	} else {
		srcX = 0;
		srcY = 0;
		srcW = dstW;
		srcH = dstH;
	}

	StretchBlt(hDC,0,0,dstW,dstH,hMemDC,srcX,srcY,srcW,srcH,SRCCOPY);

	if (m_cb)
	{
		m_cb(m_index, hDC, 0);
	}
	
	DeleteObject(hBitMap);
	DeleteDC(hMemDC);
    ReleaseDC(m_hWnd, hDC);

	err = 0;

ret:	
	return err;
}

int GDIRender::clean()
{
	return 0;
}

void GDIRender::resize()
{
}

#define LIMIT(x) \
(unsigned char)(((x>0xffffff)?0xff0000:((x<=0xffff)?0:x&0xff0000))>>16)

void GDIRender::colorConvert(unsigned char *py, unsigned char *pu, unsigned char *pv, int width, int height)
{ 
	const unsigned char * yplane  = (unsigned char *)py;
	const unsigned char * uplane  = (unsigned char *)pu;
	const unsigned char * vplane  = (unsigned char *)pv;

	int hh = height;
	int ww = width;

	int x, y;
	for (y = 0; y < hh; y+=2) {
		const unsigned char * yline  = yplane + (y * ww);
		const unsigned char * yline2 = yline  + ww;
		const unsigned char * uline  = uplane + ((y >> 1) * (ww >> 1));
		const unsigned char * vline  = vplane + ((y >> 1) * (ww >> 1));
        
		int yy  = y;
		if (m_flip)
			yy = (height-1) - yy;
		unsigned char * rgb  = m_rgb.getLine(yy);
		yy = y + 1;
		if (m_flip)
			yy = (hh-1) - yy;
		
		unsigned char * rgb2 = m_rgb.getLine(yy);
		
		for (x = 0; x < ww; x += 2) {
			long Cr = *uline++ - 128;     // calculate once for 4 pixels
			long Cb = *vline++ - 128;
			long lrc = 104635 * Cb;
			long lgc = -25690 * Cr + -53294 * Cb;
			long lbc = 132278 * Cr;
			
			if (m_rgbReverseOrder) { 
				long tmp;     // exchange red component and blue component
				tmp=lrc;
				lrc=lbc;
				lbc=tmp;
			}
			
			long Y  = *yline++ - 16;      // calculate for every pixel
			if (Y < 0)
				Y = 0;

			long l  = 76310 * Y;
			long lr = l + lrc;
			long lg = l + lgc;
			long lb = l + lbc;
			
			*rgb++ = LIMIT(lr);
			*rgb++ = LIMIT(lg);
			*rgb++ = LIMIT(lb);         

			Y  = *yline++ - 16;       // calculate for every pixel
			if (Y < 0)
				Y = 0;

			l  = 76310 * Y;
			lr = l + lrc;
			lg = l + lgc;
			lb = l + lbc;
			
			*rgb++ = LIMIT(lr);
			*rgb++ = LIMIT(lg);
			*rgb++ = LIMIT(lb);         

			Y  = *yline2++ - 16;     // calculate for every pixel
			if (Y < 0)
				Y = 0;

			l  = 76310 * Y;
			lr = l + lrc;
			lg = l + lgc;
			lb = l + lbc;
			
			*rgb2++ = LIMIT(lr);
			*rgb2++ = LIMIT(lg);
			*rgb2++ = LIMIT(lb);        

			Y  = *yline2++ - 16;      // calculate for every pixel
			if (Y < 0)
				Y = 0;

			l  = 76310 * Y;
			lr = l + lrc;
			lg = l + lgc;
			lb = l + lbc;
			
			*rgb2++ = LIMIT(lr);
			*rgb2++ = LIMIT(lg);
			*rgb2++ = LIMIT(lb);        
		}
	}
}

int GDIRender::SetString(char * pcStr,
						  int x,
						  int y, 
						  COLORREF OSDColor, 
						  bool bTransparent,
						  COLORREF BackColor,
						  HFONT OSDFont)
{
	strcpy(m_pOSDString, pcStr);
	m_OSDStringX = x;
	m_OSDStringY = y;
	m_OSDFont =  OSDFont;
	m_OSDColor = OSDColor;
	m_BackColor = BackColor;
	m_bTransparent = bTransparent;
	return 1;
}

void GDIRender::ShowString(HDC hDC)
{	
	if (hDC == NULL) {
		return ;
	}

	if(m_pOSDString == NULL) {	
		return;
	}
	
	if(m_bTransparent) {
		SetBkMode(hDC, TRANSPARENT);
	} else {
		SetBkColor(hDC,m_BackColor);
		SetBkColor(hDC, m_BackColor);
	}
	
	if(m_OSDFont != 0) {
		m_FontPre = (HFONT)SelectObject(hDC, m_OSDFont);//设置字体样式
	}
	
	SetTextColor(hDC,m_OSDColor);//设置背景色
	
	TextOut(hDC, m_OSDStringX, m_OSDStringY, m_pOSDString, strlen(m_pOSDString));//字体输出

	if(m_OSDFont != 0)
	{
		SelectObject(hDC, m_FontPre);
	}
}

int GDIRender::SetShowRect(int left, int top, int right, int bottom)
{
	if (right<left||bottom<top) {
		return -1;
	}
	
	if (left<0||right<0||top<0||bottom<0) {
		m_partRect.left		= -1;
		m_partRect.bottom	= -1;
		m_partRect.top		= -1;
		m_partRect.right	= -1;
	} else {
		m_partRect.left		= left;
		m_partRect.top		= top;
		m_partRect.right	= (right-left)%2==0?right:(right+1);
		m_partRect.bottom	= (bottom-top)%2==0?bottom:(bottom+1);
	}
	
	return 0;
}

int GDIRender::SetMaskRect(int maskid, int left, int top, int right, int bottom)
{
	if (maskid<0||maskid>=MAX_MASK_COUNT) {
		return -1;
	}
	
	if (right<left||bottom<top) {
		return -1;
	}
	
	m_maskRect[maskid].left		= left;
	m_maskRect[maskid].right	= right;
	m_maskRect[maskid].top		= top;
	m_maskRect[maskid].bottom	= bottom;
	
	m_showMask = 1;
	
	return 0;
}

int GDIRender::CleanMaskRect()
{
	for (int i = 0; i<MAX_MASK_COUNT;++i)
	{
		m_maskRect[i].left		= -1;
		m_maskRect[i].right		= -1;
		m_maskRect[i].top		= -1;
		m_maskRect[i].bottom	= -1;
	}
	
	m_showMask = 0;
	
	return 0;
}

HWND GDIRender::GetWind()
{
	return m_hWnd;
}

