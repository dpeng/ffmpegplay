#ifndef PCMRENDER1_H
#define PCMRENDER1_H

#include <dsound.h>
#include <windows.h>

#define DSOUND_BUFFER_COUNT 4

class PcmRender1 
{
public:
	PcmRender1();
	virtual ~PcmRender1();
	
	// 参数分别为通道数，采样率
	int init(int channels, int samplePerSec, int bitsPerSample, HWND hwnd);
	int clean();
	
	// 写数据
	int write(unsigned char *pcm, int len);
	
	// 跟工作线程相关的方法
	void terminating(); // 中止工作
	void loop(); // 开始工作循环（不一定进入线程）
	
	// 检查是否可用
    BOOL inited();

protected:
	void reset(); // 重置缓冲块状态
	int start(); // 启动

	HRESULT destroyDSObjects();

private:
	LPDIRECTSOUND m_pDS;

	LPDIRECTSOUNDBUFFER m_pDSBuffer[DSOUND_BUFFER_COUNT];
	HANDLE m_hasData[DSOUND_BUFFER_COUNT];
	HANDLE m_hasNoData[DSOUND_BUFFER_COUNT];

	int m_readIndex;
	int m_writeIndex;

	PCMWAVEFORMAT m_waveFormat;

	BOOL m_terminating;
	HANDLE m_hThread;
	unsigned long m_threadId;
};
#endif // PCMRENDER1_H
