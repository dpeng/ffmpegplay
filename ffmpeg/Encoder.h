#ifndef __ENCODER_C_H__
#define __ENCODER_C_H__
#pragma once

#include "../StdAfx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "Singleton.h"

extern "C" 
{
	#include "avformat.h"
	#include "swscale.h"
	#include "fifo.h"
}

#define STREAM_PIX_FMT PIX_FMT_YUV420P
#define PAL_FRAME_RATE  25000
#define NTSC_FRAME_RATE 30000 //29970
#define ENCODER_DEBUG_ENABLE 0

class CEncoder
{
	friend class Singleton<CEncoder>;
public:
	CEncoder(void);
	~CEncoder(void);

	BOOL m_bConverting;

	void avEncodeInit(const char *filename, unsigned int width, unsigned int height, BOOL bNTSC);
	void avEncode(BOOL bVideo, char* buf, int32_t nWidth, int32_t nHeight, int32_t nSize);
	void avEncodeFree();
private:
	AVFormatContext	*m_pOc;
    int16_t			*m_pSamples;
	int16_t			*m_pTmpSamples;
	uint8_t			*m_pAudioOutBuf;
	int				m_AudioOutbufSize;
	ReSampleContext *m_pReSampleContext;
	AVFifoBuffer	*m_pAudioFifoBuffer;

	AVFrame *m_Picture;
    AVFrame *m_TmpPicture;
	uint8_t *m_pVideoOutbuf;
	int m_VideoOutbufSize;

	AVOutputFormat	*m_pFmt;
	AVStream		*m_pAudioStream;
	AVStream		*m_pVideoStream;
	int			    m_nFrameRate;
	CRITICAL_SECTION m_encLock;

	AVStream *add_audio_stream(AVFormatContext *oc, enum CodecID codec_id);
	void open_audio(AVFormatContext *oc, AVStream *st);
	void write_audio_frame(AVFormatContext *oc, AVStream *st, char* pBuf, int32_t nSize);
	void close_audio(AVFormatContext *oc, AVStream *st);

	AVStream *add_video_stream(AVFormatContext *oc, enum CodecID codec_id, unsigned int width, unsigned int height);
	AVFrame *alloc_picture(enum PixelFormat pix_fmt, int width, int height);
	void open_video(AVFormatContext *oc, AVStream *st);
	void fill_yuv_image(AVFrame *pict, int width, int height, char* pBuf);
	void write_video_frame(AVFormatContext *oc, AVStream *st, char* pBuf, int32_t nWidth, int32_t nHeight);
	void close_video(AVFormatContext *oc, AVStream *st);
};

typedef Singleton<CEncoder>CEncoderSingleton;

#endif