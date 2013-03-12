#include "ddoffscreenrender.h"
#include "debugout.h"

#include <tchar.h>
#include <time.h>
#include <stdio.h>

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

DDPIXELFORMAT ddpfPixelFormats[] = 
{
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('U','Y','V','Y'),0,0,0,0,0},		// UYVY
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('Y','U','Y','2'),0,0,0,0,0},		// YUY2
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('Y','V','1','2'),0,0,0,0,0},		// YV12
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('Y','V','U','9'),0,0,0,0,0},		// YVU9
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('I','F','0','9'),0,0,0,0,0},		// IF09
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0x00FF0000,0x0000FF00,0x000000FF, 0}	//RGB32
};

/************************************************************
 * OSD
 ************************************************************/
MyVideoRender::OSD::OSD()
{
	text[0] = 0;
	x = y = 0;
	fg = RGB(255, 255, 255);
	bg = RGB(0,0,0);
	isTransparent = true;
	font = 0;
}

void MyVideoRender::OSD::setColor(COLORREF f, COLORREF b, bool tp)
{
	fg = f; bg = b; isTransparent = tp;
}

void MyVideoRender::OSD::setText(const char *string)
{
	if (string&&strlen(string)<255) {
		strcpy(text, string);
	}
}

void MyVideoRender::OSD::setPosition(int xpos, int ypos)
{
	x = xpos;y = ypos;
}

void MyVideoRender::OSD::setFont(HFONT f)
{
	font = f;
}

void MyVideoRender::OSD::drawText(HDC hDC)
{
	if (!text) {
		return;
	}
	
	if (isTransparent) {
		SetBkMode(hDC,TRANSPARENT);
	} else {
		SetBkColor(hDC,bg);
		SetTextColor(hDC,fg);
	}
		
	HFONT hOldFont = (HFONT) SelectObject(hDC, font);	
	TextOut(hDC,x,y,text,_tcslen(text));
	SelectObject(hDC, hOldFont);
}
/***********************************************************
 * RGBPlane: 32bits
 ***********************************************************/
MyVideoRender::RGBPlane::RGBPlane()
: data(0), w(0), h(0)
{
}

MyVideoRender::RGBPlane::~RGBPlane()
{
    if (data) {
        delete []data;
    }
}

void MyVideoRender::RGBPlane::resize(int width, int height)
{
	int w1=width;
	int h1=height;
	int size=w1*h1*4;

	if (w1<=0||h1<=0) return ;

    if (data) {
        if (w==w1&&h1==h) {
 			return;
		}

		delete []data;
        data = new unsigned char[size];
    } else {
        data = new unsigned char[size];
    }
    
    memset(data, 0, size);
    
    w = w1;
    h = h1;
}

unsigned char *MyVideoRender::RGBPlane::getLine(int y)
{
    return &data[y*w*4];
};

void MyVideoRender::RGBPlane::copy(
	unsigned char *py,unsigned char *pu,unsigned char *pv,int width, int height)
{
	resize(width,height);
	YUV_TO_RGB32(py,pu,pv,width,getLine(0),width,-height,width*4);
}

/*********************************************************
 * Offscreen Render
 *********************************************************/
MyVideoRender::MyVideoRender(int type, bool automode)
: m_pDD(0), m_pDDSPrimary(0),m_pDDVideoSurface(0),m_pDDVideoSurfaceBack(0)
{
	m_index		= -1;
	
	m_type		= type;
	m_automode	= automode;

	m_hWnd		= 0;

	m_width		= 352;
	m_height	= 288;

	m_callback	= 0;
	m_userData	= 0;

	m_updataOverlay = false;
	
	m_hasOverlaySupport	= FALSE;
	m_hasFourCCSupport	= FALSE;

	m_colorConvert = NULL;

	SetRectEmpty(&m_destRect);
	SetRectEmpty(&m_partRect);
	
	m_showMask = 0;

	for (int i = 0;i<MAX_MASK_COUNT;++i) {
		SetRectEmpty(&m_maskRect[i]);
	}
}

MyVideoRender::~MyVideoRender()
{
	clean();
}

int MyVideoRender::init(
	int index, HWND hWnd, 
	int width, int height, 
	draw_callback cb, long udata)
{
	// 参数赋值
	m_index		= index;
	m_hWnd		= hWnd;
	m_width		= width;
	m_height	= height;
	m_callback	= cb;
	m_userData	= udata;

	//给窗口增加属性WS_CLIPSIBLINGS
	//使得在窗口重叠时不闪烁
	//LONG Style = GetWindowLong(hWnd, GWL_STYLE);
	//SetWindowLong(hWnd,GWL_STYLE,Style|WS_CLIPSIBLINGS);
	
	if (m_type>1) return 0;

	resize();

	return initDirectDraw();
}

void MyVideoRender::resize()
{
	// 获取窗口大小
	RECT rect;
	GetWindowRect(m_hWnd, &rect);

	// dbg_print("%d,%d,%d,%d\n",rect.left,rect.top,rect.right,rect.bottom);

	if (!EqualRect(&m_destRect, &rect)) 
	{
		CopyRect(&m_destRect, &rect);
		if (m_destRect.left<0&&m_destRect.top<0
			&&m_destRect.right<0&&m_destRect.bottom<0) 
		{
			// 窗口最小化
			dbg_print("WINDOW RESIZE(MINIMIZE)\n");
		} else {
			m_updataOverlay = false;
		}
	}
}

int MyVideoRender::clean()
{
	return destroyDDObjects();
}

/************************************************************************
 * 初始化DirectDraw的一般步骤
 * 1. 创建DirectDraw对象，COM接口为IID_IDirectDraw7。
 * 2. 设置协作级别，协作级别如果为DDSCL_FULLSCREEN则还要调用SetDisplayMode()
 * 3. 创建主表面
 * 4. 创建后台主绘图表面（OFFSCREEN或者OVERLAY表面）
 * 5. 获取后台主绘图表面的附加翻转表面（可以多个）
 * 6. 如果是窗口模式，那么这里要设置裁剪区域
************************************************************************/
int MyVideoRender::initDirectDraw()
{
	destroyDDObjects();

	int err = 0;
	HRESULT hr = DD_OK;

	/*********************************************************
	 * 创建DIRECTDRAW环境
	 *********************************************************/
	hr = DirectDrawCreateEx(NULL,(VOID**)&m_pDD,IID_IDirectDraw7,NULL);
	if (FAILED(hr)) {
		err = -1;
		goto err_return;
	}

	// 协作级别，如果是全屏那么协作级别参数为
	// DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN
	if (FAILED(m_pDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL))) {
		err = -2;
		goto err_return;
	}

	// 如果实现全屏显示，那么此处要设置显示模式，如：
	// hr = m_pDD->SetDisplayMode(640,480,8,0,0);

	/*********************************************************
	 * 创建主表面
	 *********************************************************/
	MYDDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));

	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	if (FAILED(m_pDD->CreateSurface(&ddsd, &m_pDDSPrimary, NULL))) {
		err = -3;
		goto err_return;
	}

	/*********************************************************
	 * 检查能力
	 *********************************************************/
	m_hasOverlaySupport = hasOverLaySupport();

	DDCAPS ddCaps;
	ZeroMemory(&ddCaps, sizeof(DDCAPS));
	ddCaps.dwSize = sizeof(DDCAPS);

	if (FAILED(m_pDD->GetCaps(&ddCaps,NULL))) {
		err = -6;
		goto err_return;
	}

	if (ddCaps.dwCaps&DDCAPS_BLT
		&&ddCaps.dwCaps&DDCAPS_BLTFOURCC
		&&ddCaps.dwFXCaps&DDFXCAPS_BLTSHRINKX
		&&ddCaps.dwFXCaps&DDFXCAPS_BLTSHRINKY
		&&ddCaps.dwFXCaps&DDFXCAPS_BLTSTRETCHX
		&&ddCaps.dwFXCaps&DDFXCAPS_BLTSTRETCHY)
	{
		dbg_print("SUPPORT BLT-STRETCH/SHRINK and BLT-FOURCC\n");
	} else {
		err = -5;
		goto err_return;
	}

	/**********************************************************
	 * 得到屏幕大小
	 **********************************************************/
	m_pDD->GetDisplayMode(&ddsd);

	m_screenWidth	= ddsd.dwWidth;
	m_screenHeight	= ddsd.dwHeight;
	//dbg_print("SCREEN SIZE(%d,%d)\n",m_screenWidth,m_screenHeight);
	
	/*********************************************************
	 * 创建绘图表面
	 *********************************************************/
	if (FAILED(createDrawSurface(m_type))) {
		return -1;
	}

	/*******************************************************
	 * Overlay处理
	 *******************************************************/
	if (m_type==1/*Overlay*/) {
		// 如果是OVERLAY表面则设置OVERLAY显示属性
		ZeroMemory(&m_OverlayFX, sizeof(m_OverlayFX) );
		m_OverlayFX.dwSize = sizeof(m_OverlayFX);
		
		m_dwOverlayFlags = DDOVER_SHOW;
		
		if (m_ddCaps.dwCKeyCaps & DDCKEYCAPS_DESTOVERLAY) {
			// 指定覆盖关键色
			DWORD dwDDSColor = convertGDIColor(m_colorKey);
			
			m_OverlayFX.dckDestColorkey.dwColorSpaceLowValue  = dwDDSColor;
			m_OverlayFX.dckDestColorkey.dwColorSpaceHighValue = dwDDSColor;
			
			m_dwOverlayFlags |= DDOVER_DDFX|DDOVER_KEYDESTOVERRIDE ;
			
			ZeroMemory(&m_ddbltfx, sizeof(m_ddbltfx));
			m_ddbltfx.dwSize = sizeof(m_ddbltfx);
			m_ddbltfx.dwFillColor = dwDDSColor;
		}
		
	}
	
	if (FAILED(createClipper(m_hWnd))) {
		err = -5;
	}
	
err_return:
	return err;
}

HRESULT MyVideoRender::createDrawSurface(bool overlay)
{
	MYDDSURFACEDESC ddsd;
	DDSCAPS2       ddscaps;
	HRESULT		   hr;

	SAFE_RELEASE(m_pDDVideoSurface); 

	/*
	 * 创建主绘图表面
	 */
	ZeroMemory(&ddsd, sizeof(ddsd) );
	ddsd.dwSize = sizeof(ddsd);

#define DDO_FLAG \
	(DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_BACKBUFFERCOUNT)
#define DDO_CAPS_OFFSCREEN \
	(DDSCAPS_OFFSCREENPLAIN|DDSCAPS_FLIP|DDSCAPS_COMPLEX|DDSCAPS_VIDEOMEMORY)
#define DDO_CAPS_OVERLAY \
	(DDSCAPS_OVERLAY|DDSCAPS_FLIP|DDSCAPS_COMPLEX|DDSCAPS_VIDEOMEMORY)
	
	ddsd.dwFlags			= DDO_FLAG;
	ddsd.ddsCaps.dwCaps		= overlay?DDO_CAPS_OVERLAY:DDO_CAPS_OFFSCREEN;
	ddsd.dwWidth			= m_width;
	ddsd.dwHeight			= m_height;
	ddsd.dwBackBufferCount	= 1;

	if (overlay) {
		int i = 0;
		while (i<3){
			ddsd.ddpfPixelFormat   = ddpfPixelFormats[i];
			hr = m_pDD->CreateSurface(&ddsd, &m_pDDVideoSurface, NULL);
			if (FAILED(hr)) {
				i++;
			} else {
				break;
			}
		}
		
		if (i<3) {
			m_colorConvert = ccfunc[i];
		} else {
			return hr;
		}
	} else {
		/*
		DDPIXELFORMAT ddpf;
	
		ZeroMemory(&ddpf,sizeof(ddpf));

		ddpf.dwSize			= sizeof(ddpf);
		ddpf.dwFlags		= DDPF_RGB;
		ddpf.dwRGBBitCount	= 32;
		ddpf.dwGBitMask		= 0xFF00;
		ddpf.dwRBitMask		= 0xFF0000;
		ddpf.dwBBitMask		= 0xFF;
		
		ddsd.ddpfPixelFormat= ddpf;
		*/
		ddsd.ddpfPixelFormat= ddpfPixelFormats[5]; // RGB32

		hr = m_pDD->CreateSurface(&ddsd, &m_pDDVideoSurface, NULL);
		if (FAILED(hr)) {
			return hr;
		}
	}
	
	/*
	 *	获取后台表面
	 */
	ZeroMemory(&ddscaps, sizeof(ddscaps));
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	
	hr = m_pDDVideoSurface->GetAttachedSurface(&ddscaps, &m_pDDVideoSurfaceBack);
	
	return hr;
}

HRESULT MyVideoRender::createClipper(HWND hwnd)
{
	HRESULT		   hr;
	LPDIRECTDRAWCLIPPER pClipper = NULL;

	if(FAILED(hr = m_pDD->CreateClipper(0, &pClipper, NULL))){
		return hr;
	}
	
	if(FAILED(hr = pClipper->SetHWnd(0, hwnd))){
		return hr;
	}

	if(FAILED( hr = m_pDDSPrimary->SetClipper(pClipper))){
		return hr;
	}

	SAFE_RELEASE(pClipper);

	return S_OK;
}

HRESULT MyVideoRender::destroyDDObjects()
{
	SAFE_RELEASE(m_pDDVideoSurface);
	SAFE_RELEASE(m_pDDSPrimary);

	if (m_pDD){
		m_pDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL);
	}

	SAFE_RELEASE( m_pDD );

	return S_OK;
}

BOOL MyVideoRender::hasOverLaySupport()
{
	ZeroMemory(&m_ddCaps, sizeof(m_ddCaps));
	m_ddCaps.dwSize = sizeof(m_ddCaps);
	m_pDD->GetCaps(&m_ddCaps, NULL);
	
	if (m_ddCaps.dwCaps&DDCAPS_OVERLAY){
		return (m_ddCaps.dwCaps&DDCAPS_OVERLAYSTRETCH);
	} 
	
	return FALSE;
}

BOOL MyVideoRender::hasFourCCSupport(LPMYDIRECTDRAWSURFACE lpdds)
{
	MYDDSURFACEDESC ddsd;

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	lpdds->GetSurfaceDesc(&ddsd);

	if (ddsd.ddpfPixelFormat.dwFlags == DDPF_FOURCC)
		return TRUE;

	return FALSE;
}

int MyVideoRender::DisplayBYGDI(
	unsigned char *py, unsigned char *pu, unsigned char *pv, 
	int width, int height)
{
	int err = 0;
	HDC hDC = 0;
	HDC hMemDC = 0 ;
	HBITMAP hBitMap = 0;
	
	//获得窗口设备,窗口DC为目标DC
	hDC = GetWindowDC(m_hWnd);
	if (hDC==NULL ) {
		err = -1;
		goto ret;
	}

	//建立一个和窗口设备兼容的内存设备
	hMemDC = CreateCompatibleDC(hDC);
	if (hMemDC==NULL) {
		ReleaseDC(m_hWnd, hDC);
		err = -1;
		goto ret;
	}

	// 准备数据
	m_rgb.copy(py,pu,pv,width,height);

	// 创建一个跟源一样大小的位图对象, 将位图对象选进内存DC
	// 准备COPY
	BITMAPINFO  bitmapinfo;
    BITMAPFILEHEADER bmpHeader;
	
    bmpHeader.bfType = 'MB';
    bmpHeader.bfSize = width*height*4+ \
		sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
    bmpHeader.bfReserved1 = 0;
    bmpHeader.bfReserved2 = 0;
    bmpHeader.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);

    bitmapinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapinfo.bmiHeader.biWidth = width;
    bitmapinfo.bmiHeader.biHeight = height;
    bitmapinfo.bmiHeader.biPlanes = 1;
    bitmapinfo.bmiHeader.biBitCount = 32;
    bitmapinfo.bmiHeader.biCompression = BI_RGB;
    bitmapinfo.bmiHeader.biSizeImage = width*height*4;
    bitmapinfo.bmiHeader.biXPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biClrUsed = 0;
    bitmapinfo.bmiHeader.biClrImportant = 0;
	
	//创建一个和窗口设备兼容的位图对象
	hBitMap = CreateCompatibleBitmap(
		hDC,m_destRect.right-m_destRect.left,m_destRect.bottom-m_destRect.top);

	if (hBitMap==NULL ) {
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
    StretchDIBits(
		hMemDC, 0, 0, 
		m_destRect.right-m_destRect.left,m_destRect.bottom-m_destRect.top,
        0, 0,m_rgb.w,m_rgb.h, m_rgb.getLine(0),
		&bitmapinfo, DIB_RGB_COLORS, SRCCOPY); 

	// 局部放大

	//OSD叠加,在内存设备上写字
	// ShowString(hMemDC); 
	if (m_callback) {
		m_callback(m_index, hMemDC, m_userData);
	}

	int srcX, srcY, srcW, srcH;
	int dstW, dstH;

	dstW = m_destRect.right-m_destRect.left;
	dstH = m_destRect.bottom-m_destRect.top;
	
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
	
	DeleteObject(hBitMap);
	DeleteDC(hMemDC);
    ReleaseDC(m_hWnd, hDC);

	err = 0;

ret:	
	return err;
}

int MyVideoRender::render(
	unsigned char *py,unsigned char *pu,unsigned char *pv,int width,int height)
{
	if (py==0||pu==0||pv==0||width<=0||height<=0) {
		return 0;
	}

	resize();
	
	// 如果图像最小化
	if (m_destRect.left<0&&m_destRect.top<0
		&&m_destRect.right<0&&m_destRect.bottom<0) 
	{
		return 0;
	}

	// 绘制客户端遮挡区域,最多32个
	drawMaskRect(py,pu,pv,width,height);

//	drawMotion(py,pu,pv,width,height);

	if (m_type>1||m_pDDVideoSurface==NULL) {
		return DisplayBYGDI(py,pu,pv,width,height);
	}

	// 图象大小变更处理
	if ((width!=m_width)||(height!=m_height)
		||m_pDDVideoSurface==NULL||m_pDDSPrimary==NULL) 
	{
		
		dbg_print("(%d) RENDER VIDEO MESSAGE(RESIZE:%d,%d)\n",
			m_index,width,height);
		
		clean();
		
		m_width = width;
		m_height = height;
		
		if (init(m_index,m_hWnd,m_width,m_height,m_callback,m_userData)<0) {
			return DisplayBYGDI(py,pu,pv,width,height);
		}
		
		m_updataOverlay = false;
	}
	
	if (m_type==0) {
		return DisplayByOffscreen(py,pu,pv,width,height);
	} else if (m_type==1) {
		return DisplayByOverlay(py,pu,pv,width,height);
	} 

	return 0;
}

int MyVideoRender::DisplayByOverlay(unsigned char *py, unsigned char *pu, unsigned char *pv, int width, int height)
{
	HRESULT hr;
	MYDDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	// 复制YUV数据到缓冲区
	hr = m_pDDVideoSurfaceBack->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT, NULL);

	if(hr == DDERR_SURFACELOST) 
	{
		hr = RestoreAll();

		if(hr == DDERR_WRONGMODE){
			if (init(m_index, m_hWnd, m_width, m_height, m_callback, m_userData)<0) {
				return DisplayBYGDI(py,pu,pv,width,height);
			}	
		}

		m_updataOverlay = false;

		return DisplayBYGDI(py,pu,pv,width,height);
	}

	m_colorConvert(py, pu, pv, (unsigned char *)ddsd.lpSurface, ddsd.lPitch, ddsd.dwWidth, ddsd.dwHeight);

	hr = m_pDDVideoSurfaceBack->Unlock(NULL);

	// 绘图表面翻转
	hr = m_pDDVideoSurface->Flip(NULL, DDFLIP_WAIT);

	RECT rcSrc;
	SetRect(&rcSrc,0,0,m_width,m_height);

	adjustRect(rcSrc, m_destRect);

	//孙杰的屏蔽这段代码; 其他人的不屏蔽
	hr = m_pDDSPrimary->Blt(NULL,NULL,NULL, DDBLT_WAIT|DDBLT_COLORFILL,&m_ddbltfx);

	if(!m_updataOverlay) {
		hr = m_pDDVideoSurface->UpdateOverlay(
			&rcSrc, m_pDDSPrimary, &m_destRect, 
			m_dwOverlayFlags, &m_OverlayFX);
		m_updataOverlay = true;
	}

	if (hr == DDERR_SURFACELOST) {
		hr = RestoreAll();
	}

	if (m_callback) {
		HDC hDC = NULL;
		// if (m_pDDSPrimary->GetDC(&hDC)) 
		if (hDC = GetDC(m_hWnd))
		{
			RECT rect;
			GetClientRect(m_hWnd, &rect);
			m_callback(m_index, hDC, m_userData);
			
			ReleaseDC(m_hWnd, hDC);
		}
	}

	return hr;
}

int MyVideoRender::DisplayByOffscreen(unsigned char *py, unsigned char *pu, unsigned char *pv, int width, int height)
{
	HRESULT hr;
	
	MYDDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	
	hr = m_pDDVideoSurfaceBack->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT, NULL);

	if (hr == DDERR_SURFACELOST)
	{
		hr = RestoreAll();

		if (hr == DDERR_WRONGMODE){
			if (init(m_index, m_hWnd, m_width, m_height, m_callback, m_userData)<0) {
				return DisplayBYGDI(py,pu,pv,width,height);
			}
		} 

		return 0;
	}

	YUV_TO_RGB32(py,pu,pv,ddsd.dwWidth,(unsigned char *)ddsd.lpSurface,
		ddsd.dwWidth, ddsd.dwHeight, ddsd.dwWidth*4);

	HDC hDC;
	
	if (m_pDDVideoSurfaceBack->GetDC(&hDC)==DD_OK) {
		m_osd.drawText(hDC);
		
		if (m_callback) {
			m_callback(m_index, hDC, m_userData);
		}
		
		m_pDDVideoSurfaceBack->ReleaseDC(hDC);
	}
	
	hr = m_pDDVideoSurfaceBack->Unlock(NULL);

	m_pDDVideoSurface->Flip(NULL, DDFLIP_WAIT);

	RECT srcRect; // 源矩形框大小

	if (!IsRectEmpty(&m_partRect)) {
		CopyRect(&srcRect,&m_partRect);
	} else {
		SetRect(&srcRect,0,0,m_width,m_height);
	}
	/*
	DDBLTFX bltfx;
	memset(&bltfx,0,sizeof(DDBLTFX));
	bltfx.dwDDFX=DDBLTFX_NOTEARING;
	*/
	hr = m_pDDSPrimary->Blt(&m_destRect, m_pDDVideoSurface, &srcRect, DDBLT_ASYNC|DDBLT_WAIT,0);

	if (hr == DDERR_SURFACELOST) {
		hr = RestoreAll();
	}
	
	return hr;
}

int MyVideoRender::SetString(
	char * pcStr,
	int x, int y, 
	COLORREF OSDColor,bool bTransparent,COLORREF BackColor,
	HFONT OSDFont)
{
	m_osd.setText(pcStr);
	m_osd.setFont(OSDFont);
	m_osd.setPosition(x,y);
	m_osd.setColor(OSDColor,BackColor,bTransparent);
	
	return 1;
}

int MyVideoRender::SetShowRect(int left, int top, int right, int bottom)
{
	dbg_print("left=%d,top=%d,right=%d,bottom=%d,m_width=%d,m_height=%d",
		left, top, right, bottom, m_width, m_height);

	if (right<left||bottom<top) { 
		return -1;
	}

	if (left<0||right<0||top<0||bottom<0) {
		SetRectEmpty(&m_partRect);
	} else {
		m_partRect.left	= left;
		m_partRect.top	= top;
		m_partRect.right	= (right-left)%2==0?right:(right+1);
		m_partRect.bottom	= (bottom-top)%2==0?bottom:(bottom+1);

		if (left<0) {
			m_partRect.left		+= -left;
			m_partRect.right	+= -left;
		}
		
		if (right>m_width) {
			m_partRect.left		+= (right-m_width);
			m_partRect.right	+= (right-m_width);
		}

		if (top<0) {
			m_partRect.top		+= -top;
			m_partRect.bottom	+= -top;
		} 
		
		if (bottom>m_height) {
			m_partRect.top		+= (bottom-m_height);
			m_partRect.bottom	+= (bottom-m_height);
		}
	}

	return 0;
}

int MyVideoRender::SetMaskRect(int maskid, int left, int top, int right, int bottom)
{
	if (maskid<0||maskid>=MAX_MASK_COUNT) {
		return -1;
	}

	if (right<left||bottom<top) {
		return -1;
	}
	
	SetRect(&m_maskRect[maskid],left,top,right,bottom);
	
	m_showMask = 1;

	return 0;
}

int MyVideoRender::CleanMaskRect()
{
	for (int i = 0; i<MAX_MASK_COUNT;++i) {
		SetRectEmpty(&m_maskRect[i]);
	}

	m_showMask = 0;
	
	return 0;
}

void MyVideoRender::drawMaskRect(
	unsigned char *py, unsigned char *pu, unsigned char *pv,
	int width, int height)
{
	if (!m_showMask) {
		return;
	}

	int ysize = width*height;
	int uvsize = ysize/4;
		
	int xpos,xwidth;
		
	for (int i = 0;i<MAX_MASK_COUNT;++i) {
		if (m_maskRect[i].left>0&&m_maskRect[i].top>0
			&&m_maskRect[i].bottom>0&&m_maskRect[i].right>0) 
		{
			for (int j=m_maskRect[i].top;j<=m_maskRect[i].bottom;j++){
				xpos = j*width+m_maskRect[i].left;
				xwidth = m_maskRect[i].right-m_maskRect[i].left;
				
				memset(py+xpos, 0, xwidth);
				
				if (j%2){
					xpos = j/2*width/2+m_maskRect[i].left/2;
					xwidth = m_maskRect[i].right/2-m_maskRect[i].left/2;
					
					memset(pu+xpos, 128, xwidth);
					
					xpos = j/2*width/2+m_maskRect[i].left/2;
					memset(pv+xpos, 128, xwidth);
				}
			}
		}
	}
}

int m[16*12] = {
	0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
	0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,
	0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,
	0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,
	0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,
	0,0,0,1,1,1,0,0,0,0,1,1,1,0,0,0,
	0,0,0,1,1,1,0,0,0,0,1,1,1,0,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
	0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0
};

#define ADJUST_OFFSET 3

void MyVideoRender::drawMotion(
	unsigned char *py,unsigned char *pu,unsigned char *pv, 
	int width, int height)
{
	int i,j;
	int x,y;

	int uv;

	i = j = 0;
	x = y = 0;

	for (i=0;i<height;i++){ // 逐行扫描
		for (j=0;j<width;j++) {
			if (i%24==0||j%22==0) {
				x = j/22;
				y = i/24; // 0-12
				py[i*width+j] = 0;
			} else {
				if (m[y*16+x]) {
					uv = i/2*width/2+j/2;
					pu[uv] = pu[uv]+ADJUST_OFFSET;
					pv[uv] = pv[uv]-ADJUST_OFFSET;
				}
			}
		}
	}
}

HWND MyVideoRender::GetWind()
{
	return m_hWnd;
}

HRESULT MyVideoRender::RestoreAll()
{
	HRESULT hr = DD_OK;

	hr = m_pDDSPrimary->Restore();
	hr = m_pDDVideoSurface->Restore();
	hr = m_pDDVideoSurfaceBack->Restore();

	return hr;
}

void MyVideoRender::adjustRect(RECT &rcSrc, RECT &rcDest)
{
	// 窗口在屏幕左边超出.
	// 源RECT的Left要发生变化
	
	int sdx = rcSrc.right - rcSrc.left; // 源的宽.
	int sdy = rcSrc.bottom - rcSrc.top; // 源的高.
	
	int ddx = rcDest.right - rcDest.left; // 目标的宽
	int ddy = rcDest.bottom - rcDest.top; // 目标的高
	
	if(rcDest.left < 0) {
		rcSrc.left = (-rcDest.left) * sdx / ddx  ;	
		rcDest.left = 0;
	}
    
	if(rcDest.top < 0) {
		rcSrc.top = -rcDest.top ;
		rcDest.top =0;
	}
	
	if(rcDest.right >m_screenWidth) {
		rcSrc.right = rcSrc.right - ((rcDest.right - m_screenWidth) * sdy / ddy);
		if(rcSrc.right < 30)
			rcSrc.right = 30;
		rcDest.right = m_screenWidth;
	}
	
	if(rcDest.bottom > m_screenHeight) {
		rcSrc.bottom = rcSrc.bottom - ((rcDest.bottom - m_screenHeight) * sdx / ddx);
		rcDest.bottom = m_screenHeight;
	}
}

DWORD MyVideoRender::convertGDIColor( COLORREF dwGDIColor )
{
	if (m_pDDSPrimary == NULL) 
	{
		return 0x00000000;
	}
	
	COLORREF       rgbT;
	HDC            hdc;
	DWORD          dw = CLR_INVALID;
	MYDDSURFACEDESC ddsd;
	HRESULT        hr;
	
	if (dwGDIColor != CLR_INVALID && m_pDDSPrimary->GetDC(&hdc) == DD_OK)
	{
		rgbT = GetPixel(hdc, 0, 0);
		SetPixel(hdc, 0, 0, dwGDIColor);
		m_pDDSPrimary->ReleaseDC(hdc);
	}
	
	ddsd.dwSize = sizeof(ddsd);
	hr = m_pDDSPrimary->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
	if (hr == DD_OK) 
	{
		dw = *(DWORD *) ddsd.lpSurface; 
		if( ddsd.ddpfPixelFormat.dwRGBBitCount < 32 )
			dw &= ( 1 << ddsd.ddpfPixelFormat.dwRGBBitCount ) - 1;  
		m_pDDSPrimary->Unlock(NULL);
	}
	
	if (dwGDIColor != CLR_INVALID && m_pDDSPrimary->GetDC(&hdc) == DD_OK)
	{
		SetPixel( hdc, 0, 0, rgbT );
		m_pDDSPrimary->ReleaseDC(hdc);
	}
	
	return dw;    
}