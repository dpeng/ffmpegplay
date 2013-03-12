// render.cpp : Defines the entry point for the DLL application.
//

#include "render.h"
#include "manager.h"
#include "..\..\..\..\DecodeSdk\Trunk\SRC\global.h"

static char modulePath[256];

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	char *str = 0;

	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			GetModuleFileName((HMODULE)hModule, modulePath, 255);
			str = strrchr(modulePath, '\\');
			str[0] = 0;

			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}


static DhRenderManager g_RenderManager;
DhRenderManager *s_manager = &g_RenderManager;

RENDER_API int render_init(long hWnd)
{
	if (s_manager) 
	{
		if (s_manager->init((HWND)hWnd, modulePath) < 0) 
		{
			delete s_manager;
			return RENDER_ERR_FATAL;
		}
	}

	return 0;
}

RENDER_API int render_open(int index, HWND hWnd, int width, int height, draw_callback cb, long udata,VideoRenderMethod vrm, DWORD ck)
{
	if (s_manager != 0) 
	{	
		if( index >= 0 && index < RENDERITEMCOUNT )
		{
			return s_manager->open(index, hWnd, width, height, cb, udata, vrm, ck);
		}
		else
		{
			return RENDER_ERR_CHANNEL_INDEX;
		}
	}
	
	return RENDER_ERR_FATAL;
}

RENDER_API int render_audio(int index, unsigned char *pcm, int len, int bits, int sampling)
{
	if (s_manager != 0) 
	{
		if( index >= 0 && index < RENDERITEMCOUNT )
		{
			return s_manager->renderAudio(index, pcm, len, bits, sampling);
		}
		else
		{
			return RENDER_ERR_CHANNEL_INDEX;
		}
	}

	return RENDER_ERR_FATAL;
}

RENDER_API int render_video(int index, unsigned char *py, unsigned char *pu, unsigned char *pv, int width, int height)
{
	if (s_manager != 0) 
	{
		if( index >= 0 && index < RENDERITEMCOUNT )
		{
			return s_manager->renderVideo(index, py, pu, pv, width, height);
		}
		else
		{
			return RENDER_ERR_CHANNEL_INDEX;
		}
	}

	return RENDER_ERR_FATAL;
}

RENDER_API int render_close_all()
{
	if (s_manager != 0) 
	{
		return 0;
	}

	return RENDER_ERR_FATAL;
}

RENDER_API int render_close(int index)
{
	if (s_manager != 0) 
	{
		if( index >= 0 && index < RENDERITEMCOUNT )
		{
			s_manager->close(index);
			return 0;
		}
		else
		{
			return RENDER_ERR_CHANNEL_INDEX;
		}
	}

	return RENDER_ERR_FATAL;
}

RENDER_API int render_grab_picture(int index, const char *path, const char *basename, int format, int count)
{
	if (s_manager) 
	{
		if( index >= 0 && index < RENDERITEMCOUNT )
		{
			return s_manager->grabPicture(index, path, basename, format, count);
		}
		else
		{
			return RENDER_ERR_CHANNEL_INDEX;
		}
	}

	return RENDER_ERR_FATAL;
}

RENDER_API int render_version(int *major, int *minor, int *patcher)
{
	*major = 3;
	*minor = 0;
	*patcher = 4;

	return 0;
}

RENDER_API int render_set_volume(int index ,DWORD lVolume )
{
	if (s_manager != 0) 
	{
		if( index >= 0 && index < RENDERITEMCOUNT )
		{
			return s_manager->setvolume(index, lVolume);
		}
		else
		{
			return RENDER_ERR_CHANNEL_INDEX;
		}
	}

	return RENDER_ERR_FATAL;
}

RENDER_API int render_audio_set_style(int style)
{
	if (s_manager != 0) 
	{
		s_manager->setaudiorenderstyle( style );
	}
	
	return RENDER_ERR_FATAL;
}

RENDER_API int render_show_string(int index,  char *pString, int x, int y,  COLORREF OSDColor, bool bTransparent, COLORREF BackColor, HFONT OSDFont)
{	
	if (s_manager != 0) 
	{
		if( index >= 0 && index < RENDERITEMCOUNT )
		{
			return s_manager->showstring(index, pString, x, y, OSDColor, bTransparent, BackColor, OSDFont);
		}
		else
		{
			return RENDER_ERR_CHANNEL_INDEX;
		}
	}
	
	return RENDER_ERR_FATAL;
}

RENDER_API int render_show_rect(int index, int left, int top, int right, int bottom)
{
	if (s_manager != 0) 
	{
		if( index >= 0 && index < RENDERITEMCOUNT )
		{
			return s_manager->showrect(index, left, top, right, bottom);
		}
		else
		{
			return RENDER_ERR_CHANNEL_INDEX;
		}
	}
	
	return RENDER_ERR_FATAL;
}

RENDER_API int render_show_mask(int index, int enabled, int maskid, int left, int top, int right, int bottom)
{
	if (s_manager != 0) 
	{
		if( index >= 0 && index < RENDERITEMCOUNT )
		{
			return s_manager->showmask(index, enabled, maskid, left, top, right, bottom);
		}
		else
		{
			return RENDER_ERR_CHANNEL_INDEX;
		}
	}
	
	return RENDER_ERR_FATAL;
}






















