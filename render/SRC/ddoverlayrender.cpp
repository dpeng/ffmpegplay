#include "ddoverlayrender.h"
#include "debugout.h"

#include <tchar.h>
#include <time.h>
#include <stdio.h>

DDPIXELFORMAT ddpfOverlayPixelFormats[] = 
{
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('U','Y','V','Y'),0,0,0,0,0}, // UYVY
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('Y','U','Y','2'),0,0,0,0,0},  // YUY2
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('Y','V','1','2'),0,0,0,0,0},  // YV12
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('Y','V','U','9'),0,0,0,0,0},  // YVU9
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC,MAKEFOURCC('I','F','0','9'),0,0,0,0,0},  // IF09
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0x00FF0000,0x0000FF00,0x000000FF, 0} //RGB32
};

DDOverlayRender::DDOverlayRender(DWORD ck)
: m_pDD(0), m_pDDSPrimary(0), m_pDDSVideoSurface(0)
, m_pDDSVideoSurfaceBack(0)
{
	m_index		= -1;
	m_hWnd		= 0;
	m_width		= 352;
	m_height	= 288;
	m_colorKey	= ck;

	m_callback	= 0;
	m_userData	= 0;

	m_hasOverlaySupport	= FALSE;
	m_hasFourCCSupport	= FALSE;

	m_colorConvert = NULL;

	m_destRect.top		= 0;
	m_destRect.left		= 0;
	m_destRect.right	= 0;
	m_destRect.bottom	= 0;

	m_updataOverlay = false;

	m_showMask = 0;
}

DDOverlayRender::~DDOverlayRender()
{
	clean();
}

int DDOverlayRender::init(int index, HWND hWnd, int width, int height, draw_callback cb, long udata)
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
//	LONG Style = GetWindowLong(hWnd, GWL_STYLE);
//	SetWindowLong(hWnd,GWL_STYLE,Style|WS_CLIPSIBLINGS);
	int ret = initDirectDraw();

	resize();

	return ret;
}

void DDOverlayRender::resize()
{
	// 获取窗口大小
	RECT rect;
	GetWindowRect(m_hWnd, &rect);

	if (rect.right != m_destRect.right 
		|| rect.bottom != m_destRect.bottom 
		|| rect.top != m_destRect.top 
		|| rect.left != m_destRect.left)
	{
		m_destRect.left		= rect.left;
		m_destRect.top		= rect.top;
		m_destRect.right	= rect.right;
		m_destRect.bottom	= rect.bottom;
	

		m_updataOverlay = false;
	}
}

int DDOverlayRender::clean()
{
	int ret = destroyDDObjects();
	return ret;
}

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

/************************************************************************
 * 初始化DirectDraw的一般步骤
 * 1. 创建DirectDraw对象，COM接口为IID_IDirectDraw7。
 * 2. 设置协作级别，协作级别如果为DDSCL_FULLSCREEN则还要调用SetDisplayMode()
 * 3. 创建主表面
 * 4. 创建后台主绘图表面（OFFSCREEN或者OVERLAY表面）
 * 5. 获取后台主绘图表面的附加翻转表面（可以多个）
 * 6. 如果是窗口模式，那么这里要设置裁剪区域
************************************************************************/
int DDOverlayRender::initDirectDraw()
{
	int err = 0;
	HRESULT hr = 0;

	destroyDDObjects();

	hr = DirectDrawCreateEx(NULL,(VOID**)&m_pDD,IID_IDirectDraw7,NULL);
	if (FAILED(hr)){
		err = -1;
		goto err_return;
	}

	// 协作级别，如果是全屏那么协作级别参数为DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN
	hr = m_pDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL);
	if (FAILED(hr)) {
		err = -2;
		goto err_return;
	}

	// 创建主表面，填充表面描述结构体
	MYDDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));

	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	//创建主表面    
	hr = m_pDD->CreateSurface(&ddsd, &m_pDDSPrimary, NULL);
	if (FAILED(hr)) {
		err = -3;
		goto err_return;
	}

	m_pDD->GetDisplayMode(&ddsd);

	m_screenWidth = ddsd.dwWidth;
	m_screenHeight = ddsd.dwHeight;
	
	m_hasOverlaySupport = hasOverLaySupport();

    // 创建绘图表面
	if (FAILED(createDrawSurface())) {
		err = -4;
		goto err_return;
	}

err_return:
	return err;
}

HRESULT DDOverlayRender::createDrawSurface()
{
	MYDDSURFACEDESC ddsd;
	MYDDSCAPS       ddscaps;
	HRESULT		   hr;

	SAFE_RELEASE(m_pDDSVideoSurface); 

	// 创建主绘图表面,可以是离屏表面或者是Overlay表面
	ZeroMemory(&ddsd, sizeof(ddsd) );
	ddsd.dwSize = sizeof(ddsd);

	// 根据执行的类型创建表面
	// 创建Overlay表面
	ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps    = DDSCAPS_OVERLAY|DDSCAPS_FLIP|DDSCAPS_COMPLEX|DDSCAPS_VIDEOMEMORY;
	ddsd.dwBackBufferCount = 1;
	ddsd.dwWidth           = m_width;
	ddsd.dwHeight          = m_height;

	int i = 0;
	while (i<3){
		ddsd.ddpfPixelFormat   = ddpfOverlayPixelFormats[i];
		hr = m_pDD->CreateSurface(&ddsd, &m_pDDSVideoSurface, NULL);
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

	ZeroMemory(&ddscaps, sizeof(ddscaps));
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;

	hr = m_pDDSVideoSurface->GetAttachedSurface(&ddscaps, &m_pDDSVideoSurfaceBack);
	if (FAILED(hr)){
		return hr;
	}	
	
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
	
	return createClipper(m_hWnd);
}

HRESULT DDOverlayRender::createClipper(HWND hwnd)
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

DWORD DDOverlayRender::convertGDIColor( COLORREF dwGDIColor )
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

HRESULT DDOverlayRender::destroyDDObjects()
{
	SAFE_RELEASE(m_pDDSVideoSurfaceBack);
	SAFE_RELEASE(m_pDDSVideoSurface);
	SAFE_RELEASE(m_pDDSPrimary);

	if (m_pDD) 
	{
		m_pDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL);
	}

	SAFE_RELEASE( m_pDD );

	return S_OK;
}

BOOL DDOverlayRender::hasOverLaySupport()
{
	ZeroMemory(&m_ddCaps, sizeof(m_ddCaps));
	m_ddCaps.dwSize = sizeof(m_ddCaps);
	m_pDD->GetCaps(&m_ddCaps, NULL);

	if (m_ddCaps.dwCaps&DDCAPS_OVERLAY){
		return (m_ddCaps.dwCaps&DDCAPS_OVERLAYSTRETCH);
	} 

	return FALSE;
}

BOOL DDOverlayRender::hasFourCCSupport(LPMYDIRECTDRAWSURFACE lpdds)
{
	MYDDSURFACEDESC ddsd;

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	lpdds->GetSurfaceDesc(&ddsd);

	return (ddsd.ddpfPixelFormat.dwFlags==DDPF_FOURCC);
}

DWORD DDOverlayRender::colorMatch(IMyDirectDrawSurface * pdds, COLORREF rgb)
{
    COLORREF                rgbTemp;
    HDC                     hdc;
    DWORD                   dw = CLR_INVALID;
    MYDDSURFACEDESC         ddsd;
    HRESULT                 hr;
    
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK) {
        rgbTemp = GetPixel(hdc, 0, 0);     // Save current pixel value
        SetPixel(hdc, 0, 0, rgb);       // Set our value
        pdds->ReleaseDC(hdc);
    }
    
    ddsd.dwSize = sizeof(ddsd);
    while ((hr = pdds->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING) {
    }
	    
    if (hr == DD_OK) {
        dw = *(DWORD *) ddsd.lpSurface;
        if (ddsd.ddpfPixelFormat.dwRGBBitCount < 32) {
            dw &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount) - 1;
		}

        pdds->Unlock(NULL);
    }
    
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        SetPixel(hdc, 0, 0, rgbTemp);
        pdds->ReleaseDC(hdc);
    }
    
    return dw;
}

int DDOverlayRender::render(unsigned char *py, unsigned char *pu, unsigned char *pv, int width, int height)
{
	int fill = false;

	if (py==0||pu==0||pv==0) {
		fill = true;
		goto user_draw;
	}

	int w, h;
	int i, j;

	w = width;
	h = height;

	if (m_showMask) {
		int ysize = width*height;
		int uvsize = ysize/4;
		
		int xpos,xwidth;
		
		for (i = 0 ; i < MAX_MASK_COUNT; ++i) 
		{
			if (m_maskRect[i].left>0&&m_maskRect[i].top>0&&m_maskRect[i].bottom>0&&m_maskRect[i].right>0) 
			{
				for (j=m_maskRect[i].top;j<=m_maskRect[i].bottom;j++)
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
	}

	// 如果有局部放大,	
	if (m_showPart>0) 
	{
		dbg_print("Render Video (Show Part Rect).");

		// 计算
		w = m_partRect.right-m_partRect.left;
		h = m_partRect.bottom-m_partRect.top;

		unsigned char *dst = m_tempYuvData;

		// copy 数据
		int offset,len;
		
		// COPY Y
		len = m_partRect.right-m_partRect.left;

		for (i = m_partRect.top; i<= m_partRect.bottom;++i) 
		{
			offset = width*i+m_partRect.left;
			memcpy(dst, py+offset, len);
			dst += len;
		}

		len = len/2;
		for (i = m_partRect.top/2;i<=m_partRect.bottom/2;++i) 
		{
			offset = width/2 *i+m_partRect.left/2;
			memcpy(dst, pu+offset, len);
			dst += len;
		}

		for (i = m_partRect.top/2;i<=m_partRect.bottom/2;++i) 
		{
			offset = width/2*i+m_partRect.left/2;
			memcpy(dst, pv+offset, len);
			dst += len;
		}
	}
	
	if ((w != m_width)||(h != m_height)) 
	{
		dbg_print("Render Video (Resize).");

		clean();
		
		m_width = w;
		m_height = h;

		if (init(m_index, m_hWnd, m_width, m_height, m_callback, m_userData) < 0) 
		{
			dbg_print("init error.\n");
			return -1;
		}
		m_updataOverlay = false;
	}

	HRESULT hr;
	MYDDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	
	// 复制YUV数据到缓冲区
	hr = m_pDDSVideoSurfaceBack->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT, NULL);

	if(hr == DDERR_SURFACELOST){
		hr = m_pDDSPrimary->Restore();
		if(hr == DDERR_WRONGMODE){
			 init(m_index, m_hWnd, m_width, m_height, m_callback, m_userData);
			 m_updataOverlay = false;
		} else {
			 m_pDDSVideoSurface->Restore();	
		}
	
		return 0;
	}

	if (m_showPart>0) {
		unsigned char *ty = m_tempYuvData;
		unsigned char *tu = ty + m_width *m_height;
		unsigned char *tv = tu + m_width/2 *m_height/2;

		m_colorConvert(ty, tu, tv, (unsigned char *)ddsd.lpSurface, ddsd.lPitch, ddsd.dwWidth, ddsd.dwHeight);
	} else {
		m_colorConvert(py, pu, pv, (unsigned char *)ddsd.lpSurface, ddsd.lPitch, ddsd.dwWidth, ddsd.dwHeight);
	}
	
	hr = m_pDDSVideoSurfaceBack->Unlock(NULL);
	
	// 绘图表面翻转
	hr = m_pDDSVideoSurface->Flip(NULL, DDFLIP_WAIT);
	
	

	rcSrc.left = 0;
	rcSrc.top = 0;
	rcSrc.right = m_width;
	rcSrc.bottom = m_height;
	
	resize();

	if (m_destRect.left<0&&m_destRect.top<0&&m_destRect.right<0&&m_destRect.bottom<0)
	{
		return 0;
	}
	
	adjustRect(rcSrc, m_destRect);

	//孙杰的屏蔽这段代码; 其他人的不屏蔽
	hr = m_pDDSPrimary->Blt(NULL,NULL,NULL, DDBLT_WAIT|DDBLT_COLORFILL,&m_ddbltfx);

	if(!m_updataOverlay) {
		hr = m_pDDSVideoSurface->UpdateOverlay(
			&rcSrc, m_pDDSPrimary, &m_destRect, 
			m_dwOverlayFlags, &m_OverlayFX);
		m_updataOverlay = true;
	}

	if (hr == DDERR_SURFACELOST)
	{
		m_pDDSPrimary->Restore();
		m_pDDSVideoSurface->Restore();
	}

user_draw: 
	
	if (m_callback) {
		HDC hDC = GetDC(m_hWnd);
		RECT rect;
		GetClientRect(m_hWnd, &rect);
		if (fill) {
			FillRect(hDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
		} 
		
		m_callback(m_index, hDC, m_userData);
		
		ReleaseDC(m_hWnd, hDC);
	}
	
	return hr;
}

void DDOverlayRender::adjustRect(RECT &rcSrc, RECT &rcDest)
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

int DDOverlayRender::SetString(char * pcStr,
						 int x,
						 int y, 
						 COLORREF OSDColor, 
						 bool bTransparent,
						 COLORREF BackColor,
						 HFONT OSDFont)
{
	return FALSE;
}

void DDOverlayRender::ShowString()
{
}

int DDOverlayRender::SetShowRect(int left, int top, int right, int bottom)
{
	if (right<left||bottom<top) 
	{
		return -1;
	}
	
	if (left<0||right<0||top<0||bottom<0)
	{
		m_partRect.left	= -1;
		m_partRect.bottom	= -1;
		m_partRect.top	= -1;
		m_partRect.right	= -1;

		m_showPart = 0;
	} 
	else 
	{
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

		m_showPart = 1;
		/*
		dbg_print("x=%d,y=%d,w=%d,h=%d",m_partRect.left,m_partRect.top,
			m_partRect.right-m_partRect.left,
			m_partRect.bottom-m_partRect.top);
		*/
	}
	
	return 0;
}

int DDOverlayRender::SetMaskRect(int maskid, int left, int top, int right, int bottom)
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

int DDOverlayRender::CleanMaskRect()
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

HWND DDOverlayRender::GetWind()
{
	return m_hWnd;
}