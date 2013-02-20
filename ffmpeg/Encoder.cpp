#include "Encoder.h"

CEncoder::CEncoder()
{
	m_pSamples				= NULL;
	m_pTmpSamples			= NULL;
	m_pAudioOutBuf		= NULL;
	m_AudioOutbufSize	= 0;
	m_pReSampleContext		= NULL;
	m_pAudioFifoBuffer				= NULL;
	m_bConverting		= TRUE;
	m_pFmt					= NULL;
	m_pOc					= NULL;
	m_pAudioStream			= NULL;
	m_pVideoStream			= NULL;
	InitializeCriticalSection(&m_encLock) ;
}

CEncoder::~CEncoder()
{
	DeleteCriticalSection(&m_encLock) ;
}

AVStream* CEncoder::add_audio_stream(AVFormatContext *oc, enum CodecID codec_id)
{
	AVStream *st;
	st = av_new_stream(oc, 1);
	if (!st) 
	{
		m_bConverting = FALSE;
		return NULL;
	}

	st->codec->codec_id = codec_id;
	st->codec->codec_type = AVMEDIA_TYPE_AUDIO;

	st->codec->sample_fmt = AV_SAMPLE_FMT_FLT;
	st->codec->bit_rate = 64000;
	st->codec->sample_rate = 32000;//44100;//22050;
	st->codec->channels = 1;
	st->codec->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	if(oc->oformat->flags & AVFMT_GLOBALHEADER)
		st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	m_pReSampleContext = av_audio_resample_init(1, 1, 32000, 8000, AV_SAMPLE_FMT_FLT, AV_SAMPLE_FMT_S16, 
		16, 10, 0, 0.8);
	m_pAudioFifoBuffer = av_fifo_alloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
	return st;
}
static const enum AVSampleFormat sample_fmts[] = {
	AV_SAMPLE_FMT_FLT, AV_SAMPLE_FMT_NONE
};
void CEncoder::open_audio(AVFormatContext *oc, AVStream *st)
{
	AVCodecContext *c;
	AVCodec *codec;

	c = st->codec;

	codec = avcodec_find_encoder(c->codec_id);
	if (!codec) 
	{
		m_bConverting = FALSE;
		return;
	}

	codec->sample_fmts = sample_fmts;
	if (avcodec_open(c, codec) < 0) 
	{
		m_bConverting = FALSE;
		return;
	}

	m_AudioOutbufSize = c->frame_size*c->channels*av_get_bytes_per_sample(c->sample_fmt);
	m_pAudioOutBuf = (uint8_t *)av_malloc(m_AudioOutbufSize);

	m_pSamples = (int16_t *)av_malloc(m_AudioOutbufSize);
	m_pTmpSamples = (int16_t *)av_malloc(m_AudioOutbufSize);
}

void CEncoder::write_audio_frame(AVFormatContext *oc, AVStream *st, char* pBuf, int32_t nSize)
{
	int frame_bytes = st->codec->frame_size*2;
	av_fifo_generic_write(m_pAudioFifoBuffer, pBuf, nSize, NULL);
	while( av_fifo_size(m_pAudioFifoBuffer) >= frame_bytes/4)
	{
		av_fifo_generic_read(m_pAudioFifoBuffer, m_pTmpSamples, frame_bytes/4, NULL);
		int len = audio_resample(m_pReSampleContext, m_pSamples, m_pTmpSamples, frame_bytes/8);

		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.size= avcodec_encode_audio(st->codec, m_pAudioOutBuf, frame_bytes, m_pSamples);

		if (st->codec->coded_frame && st->codec->coded_frame->pts != AV_NOPTS_VALUE)
			pkt.pts= av_rescale_q(st->codec->coded_frame->pts, st->codec->time_base, st->time_base);

		pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index= st->index;
		pkt.data= m_pAudioOutBuf;

		if (av_interleaved_write_frame(oc, &pkt) != 0) 
		{
			m_bConverting = FALSE;
			return;
		}
		av_free_packet(&pkt);
	}
}

void CEncoder::close_audio(AVFormatContext *oc, AVStream *st)
{
	av_free(m_pSamples);
	m_pSamples = NULL;
	av_free(m_pTmpSamples);
	m_pTmpSamples = NULL;
	av_free(m_pAudioOutBuf);
	m_pAudioOutBuf = NULL;
	audio_resample_close(m_pReSampleContext);
	m_pReSampleContext = NULL;
	av_fifo_free(m_pAudioFifoBuffer);
	m_pAudioFifoBuffer = NULL;
	avcodec_close(st->codec);
	st->codec = NULL;
}

AVStream* CEncoder::add_video_stream(AVFormatContext *oc, enum CodecID codec_id, unsigned int width, unsigned int height)
{
	AVCodecContext *c;
	AVStream *st;

	st = av_new_stream(oc, 0);
	if (!st) 
	{
		m_bConverting = FALSE;
		return NULL;
	}

	c = st->codec;
	c->codec_id = codec_id;
	c->codec_type = AVMEDIA_TYPE_VIDEO;

	c->bit_rate = 3200000;
	c->qmax = 51;
	c->qmin = 1;

	c->width = width;
	c->height = height;
	c->time_base.den = m_nFrameRate;
	if (m_nFrameRate >= 29970) c->time_base.num = 1001;
	else c->time_base.num = 1000;
	c->gop_size = 12;
	c->pix_fmt = STREAM_PIX_FMT;
	if (c->codec_id == CODEC_ID_MPEG2VIDEO) 
		c->max_b_frames = 2;
	if (c->codec_id == CODEC_ID_MPEG1VIDEO)
		c->mb_decision=2;
	if(oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	return st;
}

AVFrame* CEncoder::alloc_picture(enum PixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	uint8_t *picture_buf;
	int size;

	picture = avcodec_alloc_frame();
	if (!picture)
		return NULL;
	size = avpicture_get_size(pix_fmt, width, height);
	picture_buf = (unsigned char*)av_malloc(size);
	if (!picture_buf) 
	{
		av_free(picture);
		return NULL;
	}
	avpicture_fill((AVPicture *)picture, picture_buf, pix_fmt, width, height);
	if (picture_buf)
	{
		av_free(picture_buf);
	}
	return picture;
}

void CEncoder::open_video(AVFormatContext *oc, AVStream *st)
{
	AVCodec *codec;
	AVCodecContext *c;

	c = st->codec;

	codec = avcodec_find_encoder(c->codec_id);
	if (!codec) 
	{
		m_bConverting = FALSE;
		return;
	}

	if (avcodec_open(c, codec) < 0) 
	{
		m_bConverting = FALSE;
		return;
	}

	m_pVideoOutbuf = NULL;
	if (!(oc->oformat->flags & AVFMT_RAWPICTURE)) 
	{
		m_VideoOutbufSize = 200000;
		m_pVideoOutbuf = (unsigned char*)av_malloc(m_VideoOutbufSize);
	}

	m_Picture = alloc_picture(c->pix_fmt, c->width, c->height);
	if (!m_Picture) 
	{
		m_bConverting = FALSE;
		return;
	}

	m_TmpPicture = NULL;
	if (c->pix_fmt != PIX_FMT_YUV420P) 
	{
		m_TmpPicture = alloc_picture(PIX_FMT_YUV420P, c->width, c->height);
		if (!m_TmpPicture) 
		{
			m_bConverting = FALSE;
			return;
		}
	}
}

void CEncoder::fill_yuv_image(AVFrame *pict, int width, int height, char* pBuf)
{
	int size = width*height;

	pict->data[0] = (uint8_t*)pBuf;
	pict->data[1] = pict->data[0] + size;
	pict->data[2] = pict->data[1] + size / 4;
	pict->linesize[0] = width;
	pict->linesize[1] = width / 2;
	pict->linesize[2] = width / 2;
}

void CEncoder::write_video_frame(AVFormatContext *oc, AVStream *st, char* pBuf, int32_t nWidth, int32_t nHeight)
{
	int out_size, ret;
	AVCodecContext *c;
	static struct SwsContext *img_convert_ctx;
	AVPacket pkt;

	c = st->codec;

	if (c->pix_fmt != PIX_FMT_YUV420P) 
	{
		if (img_convert_ctx == NULL) 
		{
			img_convert_ctx = sws_getContext(c->width, c->height,
				PIX_FMT_YUV420P,
				c->width, c->height,
				c->pix_fmt,
				SWS_BICUBIC, NULL, NULL, NULL);
			if (img_convert_ctx == NULL) 
			{
				m_bConverting = FALSE;
				return;
			}
		}
		fill_yuv_image(m_TmpPicture, nWidth, nHeight, pBuf);
		sws_scale(img_convert_ctx, m_TmpPicture->data, m_TmpPicture->linesize,
			0, c->height, m_Picture->data, m_Picture->linesize);
	} 
	else 
	{
		fill_yuv_image(m_Picture, nWidth, nHeight, pBuf);
	}

	if (oc->oformat->flags & AVFMT_RAWPICTURE) 
	{
		av_init_packet(&pkt);

		pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index= st->index;
		pkt.data= (uint8_t *)m_Picture;
		pkt.size= sizeof(AVPicture);

		ret = av_interleaved_write_frame(oc, &pkt);
	} 
	else 
	{
		out_size = avcodec_encode_video(c, m_pVideoOutbuf, m_VideoOutbufSize, m_Picture);
		if (out_size > 0) 
		{
			av_init_packet(&pkt);

			if (c->coded_frame->pts != AV_NOPTS_VALUE)
				pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
			if(c->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.stream_index= st->index;
			pkt.data= m_pVideoOutbuf;
			pkt.size= out_size;

			ret = av_interleaved_write_frame(oc, &pkt);
		} 
		else 
		{
			ret = 0;
		}
	}
	if (ret != 0) 
	{
		m_bConverting = FALSE;
		return;
	}
	av_free_packet(&pkt);
}

void CEncoder::close_video(AVFormatContext *oc, AVStream *st)
{
	memset(m_Picture, 0, sizeof(AVFrame));
	av_free(m_Picture->data[0]);
	av_free(m_Picture);
	m_Picture = NULL;
	if (m_TmpPicture)
	{
		av_free(m_TmpPicture->data[0]);
		av_free(m_TmpPicture);
		m_TmpPicture = NULL;
	}
	av_free(m_pVideoOutbuf);
	m_pVideoOutbuf = NULL;
	avcodec_close(st->codec);
	st->codec = NULL;
}

static void av_log_encoder(void *ptr, int level, const char *fmt, va_list vargs)
{
	CString buf = _T("");
	buf.Format(fmt, vargs);
	OutputDebugString(buf);
}
void CEncoder::avEncodeInit(const char *filename, unsigned int width, unsigned int height, BOOL bNTSC)
{
	EnterCriticalSection(&m_encLock);
#if ENCODER_DEBUG_ENABLE
	av_log_set_callback(av_log_encoder);
#endif
	m_pFmt = av_guess_format(NULL, filename, NULL);
	if (!m_pFmt) 
	{
		m_pFmt = av_guess_format("mpeg", NULL, NULL);
	}
	if (!m_pFmt) 
	{
		AfxMessageBox(_T("avEncodeInit::Could not find suitable output format!"));
		m_bConverting = FALSE;
		LeaveCriticalSection(&m_encLock);
		return;
	}

	m_pOc = avformat_alloc_context();
	if (!m_pOc) 
	{
		AfxMessageBox(_T("avEncodeInit::Memory error!"));
		m_bConverting = FALSE;
		LeaveCriticalSection(&m_encLock);
		return;
	}
	m_pOc->oformat = m_pFmt;
	if (bNTSC)
	{
		m_nFrameRate = NTSC_FRAME_RATE;
	} 
	else
	{
		m_nFrameRate = PAL_FRAME_RATE;
	}

	m_pVideoStream = NULL;
	m_pAudioStream = NULL;

	if (m_pFmt->video_codec != CODEC_ID_NONE) 
		m_pVideoStream = add_video_stream(m_pOc, m_pFmt->video_codec, width, height);
	if (m_pFmt->audio_codec != CODEC_ID_NONE)
		m_pAudioStream = add_audio_stream(m_pOc, m_pFmt->audio_codec);

	av_dump_format(m_pOc, 0, filename, 1);

	if (m_pVideoStream)
		open_video(m_pOc, m_pVideoStream);
	if (m_pAudioStream)
		open_audio(m_pOc, m_pAudioStream);

	if (!(m_pFmt->flags & AVFMT_NOFILE)) 
	{
		if (avio_open(&m_pOc->pb, filename, AVIO_FLAG_READ_WRITE) < 0) 
		{
			AfxMessageBox(_T("avEncodeInit::Could not open file!"));
			m_bConverting = FALSE;
			LeaveCriticalSection(&m_encLock);
			return;
		}
	}
	avformat_write_header(m_pOc, NULL);
	LeaveCriticalSection(&m_encLock);
}

void CEncoder::avEncode(BOOL bVideo, char* buf, int32_t nWidth, int32_t nHeight, int32_t nSize)
{
	EnterCriticalSection(&m_encLock) ;
	if (bVideo)
	{
		write_video_frame(m_pOc, m_pVideoStream, buf, nWidth, nHeight);
	} 
	else
	{
		write_audio_frame(m_pOc, m_pAudioStream, buf, nSize);
	}
	LeaveCriticalSection(&m_encLock);
}

void CEncoder::avEncodeFree()
{	
	EnterCriticalSection(&m_encLock) ;
	__try 
	{
		if(m_pOc)
		{
			av_write_trailer(m_pOc);
			if (m_pVideoStream)
			{
				close_video(m_pOc, m_pVideoStream);
				memset(m_pVideoStream, 0, sizeof(AVStream));
				m_pVideoStream = NULL;
			}
			if (m_pAudioStream)
			{
				close_audio(m_pOc, m_pAudioStream);
				memset(m_pAudioStream, 0, sizeof(AVStream));
				m_pAudioStream = NULL;
			}

			for(unsigned int i = 0; i < m_pOc->nb_streams; i++) 
			{
				if (m_pOc->streams[i]->codec)
					av_freep(&m_pOc->streams[i]->codec);
				if (m_pOc->streams[i])
					av_freep(&m_pOc->streams[i]);
			}

			if (!(m_pFmt->flags & AVFMT_NOFILE)) 
			{
				avio_close(m_pOc->pb);
				m_pOc->pb = NULL;
			}

			av_free(m_pOc);
			m_pOc = NULL;
		}
	}
	__except (0, 1)
	{
		int pdf = 0;
	}
	LeaveCriticalSection(&m_encLock);
}
