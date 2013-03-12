#include "manager.h"
#include "videorender.h"
#include "ddoffscreenrender.h"
#include "ddoverlayrender.h"
#include "gdirender.h"
#include "pcmrender.h"
#include "debugout.h"

DhRenderManager::DhRenderManager()
{
	m_grabIndex		= -1;	// 无效通道
	m_grabCount		= 0;	// 不抓图
	m_grabFinished	= 0;
	m_indexUseOverlay = -1;

	m_AudioRenderStyle = PCM_OUT;

	memset(m_baseName, 0, 256);

	for (int i = 0; i < RENDERITEMCOUNT; i++)
	{
		m_items[i].VideoRenderLock = 0;
		m_items[i].AudioRenderLock = 0;	
	}

	m_inited = false;
}

DhRenderManager::~DhRenderManager()
{
	close();

	for (int i = 0; i < RENDERITEMCOUNT; i++)
	{
		if(m_items[i].VideoRenderLock)
		{
			CloseHandle(m_items[i].VideoRenderLock);
			CloseHandle(m_items[i].AudioRenderLock);
			m_items[i].VideoRenderLock = 0;
			m_items[i].AudioRenderLock = 0;			
		}
	}
}

int DhRenderManager::init(HWND hWnd, char *path)
{
	if (m_inited) {
		return 0;
	}

	for (int i = 0; i < RENDERITEMCOUNT; i++)
	{
		m_items[i].m_videoRender = 0;
		m_items[i].m_audioRender = 0;
		m_items[i].VideoRenderLock = CreateMutex(0, false, 0);
		m_items[i].AudioRenderLock = CreateMutex(0, false, 0);
	}

	sprintf(m_jpegPath, "%s\\cjpeg.exe", path);

	m_inited = true;

	return 0;
}

int DhRenderManager::open(int index, HWND hWnd, int width, int height, draw_callback cb, long udata, VideoRenderMethod vrm, DWORD ck)
{
	bool overlayChannel = false;

	VideoRender *vr = 0;

	if (hWnd!=NULL) {
	
#if 1
		if(vrm == ByDDOverlay) {
			// overlay,没有字符叠加功能.
			if (m_indexUseOverlay >= 0)	{
				vr = new MyVideoRender;
			} else {
				vr = new MyVideoRender(1);
				overlayChannel = true;
				m_indexUseOverlay = 1;
			}
		} else if(vrm == ByDDOffscreen) {
			vr = new MyVideoRender;
		} else {
			vr = new MyVideoRender(2);
		}
#else
	//	vr = new MyVideoRender(2); // GDI模式
	//	vr = new MyVideoRender;
	/*
		vr = new MyVideoRender(1); // OVERLAY模式
		overlayChannel = true;
		m_indexUseOverlay =1;
	*/

#endif

		if(vr && vr->init(index, hWnd, width, height, cb, udata) < 0) {
			delete vr;
			return -1;
		}

		m_items[index].m_videoRender = vr;
	}

	//如果音频已经打开，就不需要在次打开
#define SUPPORT_AUDIO
	
#ifdef SUPPORT_AUDIO
		CAudioRender * ar = new CSoundChannel_Win32;
		
		if(ar && ar->init(1, 8000, 8, hWnd) < 0) {
			delete ar;
		} else {
			m_items[index].m_audioRender = ar;
		}

#endif	

	return 0;
}

int DhRenderManager::close()
{
	for (int i = 0; i < RENDERITEMCOUNT; ++i) 
	{
		if (m_items[i].m_videoRender != 0)
		{
			m_items[i].m_videoRender->clean();
			delete m_items[i].m_videoRender;
			m_items[i].m_videoRender = 0;
		}

		if (m_items[i].m_audioRender!= 0)
		{
			m_items[i].m_audioRender->terminating();
			m_items[i].m_audioRender->clean();
			delete m_items[i].m_audioRender;
			m_items[i].m_audioRender = 0;
		}
	}

	return 0;
}

int DhRenderManager::close(int index)
{
	if (m_items[index].m_videoRender != 0) 
	{		
		WaitForSingleObject(m_items[index].VideoRenderLock, INFINITE);

	//	m_items[index].m_videoRender->clean();
		delete m_items[index].m_videoRender;
		m_items[index].m_videoRender = 0;

		ReleaseMutex(m_items[index].VideoRenderLock);
	}

	if( m_items[index].m_audioRender != 0 )
	{
		WaitForSingleObject(m_items[index].AudioRenderLock, INFINITE);
		
		m_items[index].m_audioRender->terminating();
		m_items[index].m_audioRender->clean();
		delete m_items[index].m_audioRender;
		m_items[index].m_audioRender = 0;
		
		ReleaseMutex(m_items[index].AudioRenderLock);
	}

	if ( m_indexUseOverlay == 1)
	{
		m_indexUseOverlay = -1;
	}
	
	return 0;
}

int DhRenderManager::renderAudio(int index, unsigned char *pcm, int len,int bitsPerSample, int samplesPerSecond)
{
	int ret = 0;
	WaitForSingleObject(m_items[index].AudioRenderLock, INFINITE);
	
	if (m_items[index].m_audioRender != 0) 
	{
		if (m_items[index].m_audioRender->m_bitsPerSample != bitsPerSample || m_items[index].m_audioRender->m_samplesPerSecond != samplesPerSecond) 
		{
			m_items[index].m_audioRender->clean();
			if (m_items[index].m_audioRender->init(1, samplesPerSecond, bitsPerSample, NULL) < 0)
			{
				ret = -1;
			}
			else
			{
				ret = m_items[index].m_audioRender->write(pcm, len);
			}
		}
		else
		{
			ret = m_items[index].m_audioRender->write(pcm, len);
		}
	}
	else
	{
		ret = -1;
	}
	
	ReleaseMutex(m_items[index].AudioRenderLock);
	return ret;
}

int DhRenderManager::renderVideo(int index, unsigned char *py, unsigned char *pu, unsigned char *pv, int width, int height)
{
	int ret = 0;

	WaitForSingleObject(m_items[index].VideoRenderLock, INFINITE);

	if ( m_grabFinished < m_grabCount) 
	{
		if (index == m_grabIndex) 
		{
			m_bmpWriter.execute(m_baseName, m_grabFormat, py, pu, pv, width, height);
/*
			char param[256];

			char temp[256];
			int len = strlen(m_baseName)-3;
			strncpy(temp, m_baseName,len);
			temp[len-1] = 0;
	
			sprintf(param,"%s %s.jpg",m_baseName,temp);
			unsigned long ret = (unsigned long) ShellExecute(NULL, "open", m_jpegPath, param, "./", SW_HIDE);
*/
			m_grabFinished++;
		}	
	}

	if (m_items[index].m_videoRender != 0) 
	{
		m_items[index].m_videoRender->render(py, pu, pv, width, height);
		ret = 0;
	}
	else
	{
		ret = -1;
	}
		
	ReleaseMutex(m_items[index].VideoRenderLock);
	return ret;
}

int DhRenderManager::grabPicture(int index, const char *path, const char *basename, int format, int count)
{
	if (m_baseName) 
	{
		memset(m_baseName, 0, 256);
	}

	if (path) 
	{
		sprintf(m_baseName, "%s", path);
	} 
	else if (basename) 
	{
		sprintf(m_baseName, "%s", basename);
	} 
	else 
	{
		return -1;
	}
	
	m_grabIndex		= index;
	m_grabCount		= count;
	m_grabFinished	= 0;
	m_grabFormat	= format;
	
	return 0;
}

bool DhRenderManager::setvolume(int index, DWORD lVolume )
{
	if (m_items[index].m_audioRender == 0) 
	{
		return false;
	}
	else
	{
		return m_items[index].m_audioRender->SetVolume(lVolume);
	}
}

DWORD DhRenderManager::getvolume( int index )
{
	return m_items[index].m_audioRender->GetVolume();
}

void DhRenderManager::setaudiorenderstyle( int style)
{
	m_AudioRenderStyle = style;
}

int DhRenderManager::showstring(int index, 
								char *pString,
								int x, 
								int y,  
								COLORREF OSDColor, 
								bool bTransparent,
								COLORREF BackColor,
								HFONT OSDFont)
{
	if (m_items[index].m_videoRender != 0) 
	{
		return m_items[index].m_videoRender->SetString(pString, x, y,OSDColor,bTransparent,BackColor,OSDFont);
	}

	return -1;
}

int DhRenderManager::showrect(int index, int left, int top, int right, int bottom)
{
	if (m_items[index].m_videoRender != 0) 
	{
		return m_items[index].m_videoRender->SetShowRect(left, top, right, bottom);
	}
	
	return -1;
}

int DhRenderManager::showmask(int index, int enabled, int maskid, int left, int top, int right, int bottom)
{
	if (m_items[index].m_videoRender != 0) 
	{
		if (enabled)
			return m_items[index].m_videoRender->SetMaskRect(maskid, left, top, right, bottom);
		else
			return m_items[index].m_videoRender->CleanMaskRect();
	}
	
	return -1;
}

