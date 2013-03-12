#ifndef PCMRENDER_H
#define PCMRENDER_H

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#define PCM_BUF_COUNT	32
#define PCM_BUF_SIZE	(1024*8)

class CAudioRender
{
public:
	virtual int init(int channels, int samplePerSec, int bitPerSample, HWND hWnd)=0;
	virtual int clean() = 0;
	// 写数据
	virtual int write(unsigned char *pcm, int len) = 0;
	virtual int read(unsigned char *pcm, int *len) = 0;
	// 跟工作线程相关的方法
	virtual void terminating() = 0; // 中止工作
	virtual bool SetVolume(DWORD wNewVolume) = 0;
	virtual DWORD GetVolume() = 0;

	int m_bitsPerSample ;
	int m_samplesPerSecond ;
};

class CSoundChannel_Win32 : public CAudioRender
{
	struct PcmBuffer {
		PcmBuffer() 
		{
			hWaveOut = NULL;
			hWaveIn = NULL;

			memset(&header, 0, sizeof(header));
			header.dwFlags |= WHDR_DONE;
		}
		
		DWORD Prepare(HWAVEIN hIn,unsigned int count) 
		{
			Release();
			
			memset(&header, 0, sizeof(header));
			
			header.lpData = data;
			header.dwBufferLength = count;
			header.dwUser = (DWORD)this;
			
			hWaveIn = hIn;
			
			return waveInPrepareHeader(hWaveIn, &header, sizeof(header));
		}

		DWORD Prepare(HWAVEOUT hOut, unsigned int count)
		{
			Release();
			
			memset(&header, 0, sizeof(header));
			
			header.lpData = data;
			header.dwBufferLength = count;
			header.dwUser = (DWORD)this;

			hWaveOut = hOut;

			return waveOutPrepareHeader(hWaveOut, &header, sizeof(header));
		}

		DWORD Release()
		{
			DWORD err = MMSYSERR_NOERROR;
			
			if (hWaveOut!=NULL) {
				err = waveOutUnprepareHeader(hWaveOut, &header, sizeof(header));
				if (err== WAVERR_STILLPLAYING) {
					return err;
				}

				hWaveOut = NULL;
			}

			if (hWaveIn!=NULL) {
				err = waveInUnprepareHeader(hWaveIn, &header, sizeof(header));
				if (err== WAVERR_STILLPLAYING) {
					return err;
				}
				
				hWaveIn = NULL;
			}
			
			header.dwFlags |= WHDR_DONE;
			
			return err;
		}

		char data[PCM_BUF_SIZE];

		WAVEHDR header;

		HWAVEOUT hWaveOut;
		HWAVEIN	 hWaveIn;
	};
	
public:
	CSoundChannel_Win32(int dir = 0);
	~CSoundChannel_Win32();

	// 参数分别为通道数，采样率
	virtual int init(int channels, int samplePerSec, int bitPerSample, HWND hWnd);
	virtual int clean();

	// 写数据
	virtual int write(unsigned char *pcm, int len);
	virtual int read(unsigned char *pcm, int *len);
	
	// 跟工作线程相关的方法
	virtual void terminating(); // 中止工作

	virtual bool SetVolume(DWORD wNewVolume);
	virtual DWORD GetVolume();
	
private:
	// 内部方法
	void initParam(); // 初始化参数
	void setFormat(int channels, int samplePerSec, int bitPerSample);
	
	void reset(); // 重置缓冲块状态
	int start(); // 启动
	
	int Abort();
private:
	// 公共信息

	int m_direction; // 方向，0-播放，1-录音

	WAVEFORMATEX m_waveFormat;

    HWAVEOUT m_hWaveOut;	// 播放句柄
	HWAVEIN  m_hWaveIn;		// 录音句柄

	HANDLE m_hEventDone;	// 硬件通知，共用

	int m_bufferIndex;

	HANDLE m_mutex;
	
	// 播放缓冲区 
	PcmBuffer m_buffer[PCM_BUF_COUNT];

};

#endif /* PCMRENDER_H */
