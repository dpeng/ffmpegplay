#include "pcmrender1.h"

#include <dsound.h>
#include <stdio.h>

PcmRender1::PcmRender1()
{
	m_pDS = NULL;
	
	for (int i = 0; i<DSOUND_BUFFER_COUNT; ++i)
	{
		m_pDSBuffer[i] = NULL;
		m_hasData[i] = NULL;
		m_hasNoData[i] = NULL;
	}

	m_readIndex = m_writeIndex = 0;	

	m_terminating = FALSE;

	m_hThread = INVALID_HANDLE_VALUE;
	m_threadId = 0;
}

PcmRender1::~PcmRender1()
{
	clean();
}

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

BOOL PcmRender1::inited()
{
	return (m_pDS != NULL && m_pDSBuffer[0] != NULL);
}

int PcmRender1::init(int channels, int samplePerSec, int bitsPerSample, HWND hwnd)
{
	HRESULT hr;

	if (FAILED(hr = DirectSoundCreate(NULL, &m_pDS, NULL))) 
	{
		return -1;
	}

	if (FAILED(hr = m_pDS->SetCooperativeLevel(hwnd, DSSCL_PRIORITY)))
	{
		return -2;
	}
	
	//设置PCMWAVEFORMAT结构 
	memset(&m_waveFormat, 0, sizeof(PCMWAVEFORMAT)); 

	m_waveFormat.wf.wFormatTag = WAVE_FORMAT_PCM; 
	m_waveFormat.wf.nChannels = channels; 
	m_waveFormat.wf.nSamplesPerSec = samplePerSec; 
	m_waveFormat.wf.nBlockAlign = 1; 
	m_waveFormat.wf.nAvgBytesPerSec = m_waveFormat.wf.nSamplesPerSec * m_waveFormat.wf.nBlockAlign; 
	m_waveFormat.wBitsPerSample = bitsPerSample; 


	//设置DSBUFFERDESC结构 
	DSBUFFERDESC dsbdesc; 
	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); //将结构置0. 
	dsbdesc.dwSize = sizeof(DSBUFFERDESC); 

	//控制音量
	dsbdesc.dwFlags = 0x000000E0 | 0x00040000; 
	
	dsbdesc.dwBufferBytes = 3200;//m_waveFormat.wf.nAvgBytesPerSec * m_waveFormat.wf.nChannels; 
	dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&m_waveFormat; 

	//创建缓冲 
	for (int i = 0; i<DSOUND_BUFFER_COUNT; ++i) {
		hr = m_pDS->CreateSoundBuffer(&dsbdesc, &m_pDSBuffer[i], NULL);

		m_hasData[i] = CreateEvent(0, FALSE, FALSE, 0);
		m_hasNoData[i] = CreateEvent(0, FALSE, FALSE, 0);
	}

	reset();
	start();

	return S_OK;
}

int PcmRender1::clean()
{
	for (int i = 0; i<DSOUND_BUFFER_COUNT; ++i)
	{
		CloseHandle(m_hasData[i]);
		m_hasData[i] = NULL;

		CloseHandle(m_hasNoData[i]);
		m_hasNoData[i] = NULL;
	}
	
	return destroyDSObjects();
}

HRESULT PcmRender1::destroyDSObjects()
{
	for (int i = 0; i<DSOUND_BUFFER_COUNT; ++i) {
		SAFE_RELEASE(m_pDSBuffer[i]);
	}

	SAFE_RELEASE(m_pDS);

	return S_OK;
}

int PcmRender1::write(unsigned char *pcm, int len)
{
	if (!inited()) {
		return -1;
	}
	
	HRESULT hr; 

	VOID*   pDSLockedBuffer      = NULL;
	DWORD   dwDSLockedBufferSize = 0;
	
	WaitForSingleObject(m_hasNoData[m_writeIndex], INFINITE);

	if (FAILED(hr = m_pDSBuffer[m_writeIndex]->Lock(0, len, &pDSLockedBuffer, &dwDSLockedBufferSize, NULL, NULL, 0L)))
	{
		return -1;
	}

	if (len > dwDSLockedBufferSize) {
		m_pDSBuffer[m_writeIndex]->Unlock(pDSLockedBuffer, dwDSLockedBufferSize, NULL, NULL);
		return -2;
	}

	memcpy(pDSLockedBuffer, pcm, len);

	m_pDSBuffer[m_writeIndex]->Unlock(pDSLockedBuffer, dwDSLockedBufferSize, NULL, NULL);

	m_pDSBuffer[m_writeIndex]->SetCurrentPosition(0);

	SetEvent(m_hasData[m_writeIndex]);

	printf("Buffer (%d) is Ready.\n", m_writeIndex);

	m_writeIndex = (m_writeIndex + 1) % DSOUND_BUFFER_COUNT;

	return 0;
}

void PcmRender1::terminating()
{
	m_terminating = true;
	
	WaitForSingleObject(m_hThread, 200);
	::TerminateThread(m_hThread, 0);
	
	m_hThread = INVALID_HANDLE_VALUE;
	m_threadId = 0;
}

void PcmRender1::loop()
{
	while (!m_terminating) {
		printf("SoundBuffer Play Wait....\n");
		WaitForSingleObject(m_hasData[m_readIndex], INFINITE);
		m_pDSBuffer[m_readIndex]->Play(0, 0xFFFFFFFF, 0);
		printf("SoundBuffer Play Ok.\n");
		SetEvent(m_hasNoData[m_readIndex]);
		m_readIndex = (m_readIndex + 1) % DSOUND_BUFFER_COUNT;
	}
}

void PcmRender1::reset()
{
	for (int i = 0; i<DSOUND_BUFFER_COUNT; ++i)
	{
		ResetEvent(m_hasData[i]);
		SetEvent(m_hasNoData[i]);
	}

	m_readIndex = m_writeIndex = 0;
}

static void thread_function(void *param)
{
    PcmRender1 *render = (PcmRender1 *)param;
    render->loop();
}

/************************************************************************
 * 启动线程
************************************************************************/
int PcmRender1::start()
{
	if (m_hThread == INVALID_HANDLE_VALUE) {
        m_hThread = ::CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE)thread_function,
            this,
            0,
            &m_threadId);
        
        if (INVALID_HANDLE_VALUE == m_hThread)
            return -1;
    }
	
	return 0;
}