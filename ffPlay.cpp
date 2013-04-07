#include "ffPlay.h"

CffPlay::CffPlay(void)
{
	m_hreadProcess = NULL;
	m_videoDecHandler = NULL;
	m_audioDecHandler = NULL;
	m_ffmpegRenderHandler = NULL;
	m_videoDataList.clear();
	m_audioDataList.clear();
	m_videoDataList.init(MAX_FRAME_BUFFER_SIZE, MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT*3/2);
	m_audioDataList.init(MAX_FRAME_BUFFER_SIZE, AUDIOBUFLEN);
	m_videoItem = new AVFrameBuffer ;
	memset(m_videoItem, 0, sizeof(AVFrameBuffer));
	m_audioItem = new AVFrameBuffer ;
	memset(m_audioItem, 0, sizeof(AVFrameBuffer));
	m_hWnd = NULL;
	m_closeffPlay = FALSE;
}

CffPlay::~CffPlay(void)
{
	m_hreadProcess = NULL;
	m_videoDecHandler = NULL;
	m_audioDecHandler = NULL;
	m_ffmpegRenderHandler = NULL;
	m_videoDataList.clear();
	m_audioDataList.clear();
	delete m_videoItem;
	m_videoItem = NULL;
	delete m_audioItem;
	m_audioItem = NULL;
	m_hWnd = NULL;
	m_closeffPlay = TRUE;
}

static AVPacket flush_pkt;
static int packet_queue_put_private(PacketQueue *q, AVPacket *pkt)
{
	AVPacketList *pkt1;

	if (q->abort_request)
		return -1;

	pkt1 = (AVPacketList *)av_malloc(sizeof(AVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	if (!q->last_pkt)
		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size + sizeof(*pkt1);
	return 0;
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
	int ret;

	/* duplicate the packet */
	if (pkt != &flush_pkt && av_dup_packet(pkt) < 0)
		return -1;

	EnterCriticalSection(&(q->criticalSection));
	ret = packet_queue_put_private(q, pkt);
	LeaveCriticalSection(&(q->criticalSection));

	if (pkt != &flush_pkt && ret < 0)
		av_free_packet(pkt);

	return ret;
}

/* packet queue handling */
static void packet_queue_init(PacketQueue *q)
{
	memset(q, 0, sizeof(PacketQueue));
	InitializeCriticalSection(&(q->criticalSection));
	q->abort_request = 1;
}

static void packet_queue_flush(PacketQueue *q)
{
	AVPacketList *pkt, *pkt1;

	EnterCriticalSection(&(q->criticalSection));
	for (pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
		pkt1 = pkt->next;
		av_free_packet(&pkt->pkt);
		av_freep(&pkt);
	}
	q->last_pkt = NULL;
	q->first_pkt = NULL;
	q->nb_packets = 0;
	q->size = 0;
	LeaveCriticalSection(&(q->criticalSection));
}

static void packet_queue_destroy(PacketQueue *q)
{
	packet_queue_flush(q);
	DeleteCriticalSection(&(q->criticalSection));
}

static void packet_queue_abort(PacketQueue *q)
{
	EnterCriticalSection(&(q->criticalSection));
	q->abort_request = 1;
	LeaveCriticalSection(&(q->criticalSection));
}

static void packet_queue_start(PacketQueue *q)
{
	EnterCriticalSection(&(q->criticalSection));
	q->abort_request = 0;
	packet_queue_put_private(q, &flush_pkt);
	LeaveCriticalSection(&(q->criticalSection));
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
	AVPacketList *pkt1;
	int ret;

	EnterCriticalSection(&(q->criticalSection));

	for (;;) {
		if (q->abort_request) {
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size + sizeof(*pkt1);
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		} else if (!block) {
			ret = 0;
			break;
		} else {
			Sleep(10);
			ret = 0;
			break;
		}
	}
	LeaveCriticalSection(&(q->criticalSection));
	return ret;
}

DWORD CffPlay::ffmpegRenderPro(LPVOID pParam) 
{
	CffPlay* pThis = (CffPlay*)pParam;
	VideoState *is = &(pThis->m_currentStream);
	double videoPts = 0.0;
	double audioPts = 0.0;
	while(1)
	{
		if (pThis->m_closeffPlay)
		{
			break;
		}
		if(videoPts < audioPts)
		{
			if (pThis->m_videoDataList.read(pThis->m_videoItem))
			{
				int renderVideoRet = render_video(0, 
					pThis->m_videoItem->context, 
					pThis->m_videoItem->context + pThis->m_videoItem->width*pThis->m_videoItem->height, 
					pThis->m_videoItem->context +pThis->m_videoItem->width*pThis->m_videoItem->height*5/4, 
					pThis->m_videoItem->width, 
					pThis->m_videoItem->height);
				videoPts = pThis->m_videoItem->pts;
			}
		}
		if (videoPts >= audioPts)
		{
			if (pThis->m_audioDataList.read(pThis->m_audioItem))
			{
				//bitsPerSample, samplesPerSecond
				int renderAudioRet = render_audio(0, 
					pThis->m_audioItem->context, 
					pThis->m_audioItem->size, 
					is->audio_st->codec->channels * av_get_bytes_per_sample(is->audio_st->codec->sample_fmt) * 8, 
					pThis->m_audioItem->sampleRate);
				audioPts = pThis->m_audioItem->pts;
			}
		}
	}
	return 0;
}

DWORD CffPlay::audioDecPro(LPVOID pParam)  
{
	CffPlay* pThis = (CffPlay*)pParam;
	AVPacket pkt1, *pkt = &pkt1;
	VideoState *is = &(pThis->m_currentStream);
	AVFrame *frame = avcodec_alloc_frame();
	int pts;
	unsigned char *buf = new unsigned char[AUDIOBUFLEN]; 
	while(1)
	{
		if (pThis->m_closeffPlay)
		{
			break;
		}
		if (packet_queue_get(&is->audioq, pkt, 1) < 0)
			continue;
		avcodec_get_frame_defaults(frame);
		if (avcodec_decode_audio4(is->audio_st->codec, frame, &pts, pkt) < 0)
			continue;
		int data_size = av_samples_get_buffer_size(NULL, 
			is->audio_st->codec->channels,
			frame->nb_samples,
			is->audio_st->codec->sample_fmt, 1);
		if (data_size <= 0)
			continue;
		is->audio_clock += (double)data_size / (is->audio_st->codec->channels * is->audio_st->codec->sample_rate * av_get_bytes_per_sample(is->audio_st->codec->sample_fmt));
		(void)memset(buf, 0, sizeof(buf));
		memcpy(buf, frame->data[0], data_size);   
		pThis->m_audioItem->context = buf;
		pThis->m_audioItem->size = data_size;
		pThis->m_audioItem->sampleRate = frame->sample_rate;
		pThis->m_audioItem->frameType = pkt->stream_index;
		pThis->m_audioItem->pts = is->audio_clock;
		while (pThis->m_audioDataList.write(pThis->m_audioItem) == false)
		{
			Sleep(10);
		}
	}
	delete [] buf; 
	av_free_packet(pkt);
	avcodec_free_frame(&frame);
	return 0;
}

DWORD CffPlay::videoDecPro(LPVOID pParam)  
{
	CffPlay* pThis = (CffPlay*)pParam;
	AVPacket pkt1, *pkt = &pkt1;
	VideoState *is = &(pThis->m_currentStream);
	AVFrame *frame = avcodec_alloc_frame();
	int pts;
	int height = MAX_IMAGE_HEIGHT;  
	int width = MAX_IMAGE_WIDTH;  
	unsigned char *buf = new unsigned char[height*width*3/2]; 
	while(1)
	{
		if (pThis->m_closeffPlay)
		{
			break;
		}
		(void)memset(pkt, 0, sizeof(pkt));
		(void)memset(frame, 0, sizeof(frame));
		if (packet_queue_get(&is->videoq, pkt, 1) < 0)
			continue;
		avcodec_get_frame_defaults(frame);
		if(avcodec_decode_video2(is->video_st->codec, frame, &pts, pkt) < 0)
			continue;
		if(frame->data[0] == NULL)
			continue;
		int a=0,i;   
		(void)memset(buf, 0, sizeof(buf));
		height = is->ic->streams[is->video_stream]->codec->height;  
		width = is->ic->streams[is->video_stream]->codec->width; 
		is->video_current_pts = av_frame_get_best_effort_timestamp(frame)*av_q2d(is->video_st->time_base);
		for (i=0; i<height; i++)
		{   
			memcpy(buf+a,frame->data[0] + i * frame->linesize[0], width);   
			a+=width;   
		}   
		for (i=0; i<height/2; i++)   
		{   
			memcpy(buf+a,frame->data[1] + i * frame->linesize[1], width/2);   
			a+=width/2;   
		}   
		for (i=0; i<height/2; i++)   
		{   
			memcpy(buf+a,frame->data[2] + i * frame->linesize[2], width/2);   
			a+=width/2;   
		}  

		pThis->m_videoItem->context = buf;
		pThis->m_videoItem->width = width;
		pThis->m_videoItem->height = height;
		pThis->m_videoItem->frameType = pkt->stream_index;
		pThis->m_videoItem->pts = is->video_current_pts;
		while (pThis->m_videoDataList.write(pThis->m_videoItem) == false)
		{
			Sleep(10);
		}

	}
	delete [] buf;  
	av_free_packet(pkt);
	avcodec_free_frame(&frame);
	return 0;
}

DWORD CffPlay::ffmpegReadPro(LPVOID pParam)  
{
	CffPlay* pThis = (CffPlay*)pParam;
	AVPacket pkt1, *pkt = &pkt1;
	VideoState *is = &(pThis->m_currentStream);
	while(1)
	{
		if (pThis->m_closeffPlay)
		{
			break;
		}
		if (is->videoq.nb_packets > 100 || is->audioq.nb_packets > 100)
		{
			continue;
		}
		(void)memset(pkt, 0, sizeof(pkt));
		int ret = av_read_frame(is->ic, pkt);
		if (ret == AVERROR_EOF)
		{
			pThis->m_closeffPlay = TRUE;
			break;
		}
		if (pkt->stream_index == is->video_stream)
		{
			packet_queue_put(&is->videoq, pkt);
		}
		if (pkt->stream_index == is->audio_stream)
		{
			packet_queue_put(&is->audioq, pkt);
		}

	}
	av_free_packet(pkt);
	return 0;
}
/* Called from the main */
void CffPlay::playMpegFile(char* fileName, HWND hWnd)
{
	//playClose();
	avcodec_register_all();
	av_register_all();

	av_init_packet(&m_flushPkt);

	AVFormatContext *ic = NULL;
	int st_index[AVMEDIA_TYPE_NB];
	ic = avformat_alloc_context();
	int err = avformat_open_input(&ic, fileName, NULL, NULL);
	if (err < 0) 
	{
		return;
	}

	av_dump_format(ic, 0, fileName, 0);
	for (unsigned int i = 0; i < ic->nb_streams; i++)
		ic->streams[i]->discard = AVDISCARD_ALL;
	m_currentStream.ic = ic;
	st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, st_index[AVMEDIA_TYPE_VIDEO], NULL, 0);
	packet_queue_init(&(m_currentStream.videoq));
	packet_queue_init(&(m_currentStream.audioq));
	stream_component_open(&m_currentStream, st_index[AVMEDIA_TYPE_VIDEO]);
	stream_component_open(&m_currentStream, st_index[AVMEDIA_TYPE_AUDIO]);

	m_hWnd = hWnd;

	initDirectDraw(hWnd, 0, 0);
	m_closeffPlay = FALSE;
	DWORD dw;
	m_hreadProcess = CreateThread(NULL,0,CffPlay::ffmpegReadPro,this,0,&dw);
	m_videoDecHandler = CreateThread(NULL,0,CffPlay::videoDecPro,this,0,&dw);
	m_audioDecHandler = CreateThread(NULL,0,CffPlay::audioDecPro,this,0,&dw);
	m_ffmpegRenderHandler = CreateThread(NULL,0,CffPlay::ffmpegRenderPro,this,0,&dw);
	return;
}

void CffPlay::playPause()
{
	//toggle_pause(cur_stream);
}

void CffPlay::playClose()
{
	m_closeffPlay = TRUE;
	Sleep(10);
	CloseHandle(m_hreadProcess);
	m_hreadProcess = NULL;
	CloseHandle(m_videoDecHandler);
	m_videoDecHandler = NULL;
	CloseHandle(m_audioDecHandler);
	m_audioDecHandler = NULL;
	CloseHandle(m_ffmpegRenderHandler);
	packet_queue_destroy(&(m_currentStream.videoq));
	packet_queue_destroy(&(m_currentStream.audioq));
	m_ffmpegRenderHandler = NULL;
	m_hWnd = NULL;
}

double CffPlay::playGetTotalTime()
{
	/*
	double totalTimeInSec;
	if ((NULL != cur_stream) && 
	m_hWnd = NULL;am->ic))
	{
		totalTimeInSec = cur_stream->ic->duration / AV_TIME_BASE;
	} 
	else
	{
		totalTimeInSec = 0;
	}
	return totalTimeInSec;
	*/
	return 0;
}
double CffPlay::playGetCurrentTime()
{
	/*
	double currentTimeInSec;
	if ((NULL != cur_stream) && (NULL != cur_stream->ic))
	{
		currentTimeInSec = cur_stream->video_current_pts - cur_stream->ic->start_time;
	}
	else
	{
		currentTimeInSec = 0;
	}
	return currentTimeInSec;
	*/
	return 0;
}

void CffPlay::playSetSeekPosition(unsigned int pos)
{
	/*
	if ((NULL != cur_stream) && (NULL != cur_stream->ic))
	{
		int64_t ts;
		ts = (pos * cur_stream->ic->duration)/100;
		if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
			ts += cur_stream->ic->start_time;
		stream_seek(cur_stream, ts, 0, 0);
	}
	else
	{
	}
	*/
}

void CffPlay::playOnlyAudio(BOOL isOnlyAudio)
{
	/*
	if (isOnlyAudio)
	{
		cur_stream->show_mode = SHOW_MODE_WAVES;
	} 
	else
	{
		cur_stream->show_mode = SHOW_MODE_VIDEO;
	}
	*/
}

int CffPlay::stream_component_open(VideoState *is, int stream_index)
{
	
    AVFormatContext *ic = is->ic;
    AVCodecContext *avctx;
    AVCodec *codec;
    AVDictionary *opts;
    AVDictionaryEntry *t = NULL;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return -1;
    avctx = ic->streams[stream_index]->codec;

    codec = avcodec_find_decoder(avctx->codec_id);
    opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);

    if (!codec)
        return -1;

    if (!codec || avcodec_open2(avctx, codec, &opts) < 0)
        return -1;


    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        is->audio_st = ic->streams[stream_index];
		is->audio_stream = stream_index;
		is->audio_clock = 0.0;
		packet_queue_start(&is->audioq);
        break;
    case AVMEDIA_TYPE_VIDEO:
        is->video_st = ic->streams[stream_index];
		is->video_stream = stream_index;
		is->video_current_pts = 0.0;
		packet_queue_start(&is->videoq);
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        break;
    default:
        break;
    }
    return 0;
}

inline int CffPlay::initDirectDraw(HWND hWnd, int nWidth, int nHight)
{
	int initRet = render_init((long)hWnd);
	if (initRet < 0)
	{
		return RENDER_ERR_FATAL;
	} 
	else
	{
		initRet = render_open(0, hWnd, nWidth, nHight, NULL, NULL, ByDDOffscreen, NULL);
		if (initRet < 0)
		{
			initRet = render_open(0, hWnd, nWidth, nHight, NULL, NULL, ByDDOverlay, NULL);
			if (initRet < 0)
			{
				initRet = render_open(0, hWnd, nWidth, nHight, NULL, NULL, ByGDI, NULL);
				if (initRet < 0)
				{
					return RENDER_ERR_FATAL;
				} 
				else
				{
					return RENDER_ERR_NOERROR;
				}
			} 
			else
			{
				return RENDER_ERR_NOERROR;
			}
		} 
		else
		{
			return RENDER_ERR_NOERROR;
		}
	}
}