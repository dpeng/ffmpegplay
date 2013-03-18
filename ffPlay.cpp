#include "ffPlay.h"

CffPlay::CffPlay(void)
{
	m_ffmpegDecHandler = NULL;
	m_YuvDataList.clear();
	m_YuvDataList.init(MAX_FRAME_BUFFER_SIZE, MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT*3/2);
	m_PcmDataList.clear();
	m_PcmDataList.init(MAX_FRAME_BUFFER_SIZE, AUDIOBUFLEN);
	m_item = new AVFrameBuffer ;
	memset(m_item, 0, sizeof(AVFrameBuffer));
}

CffPlay::~CffPlay(void)
{
	m_ffmpegDecHandler = NULL;
	m_YuvDataList.clear();
	m_PcmDataList.clear();
	delete m_item;
	m_item = NULL;
}
DWORD CffPlay::ffmpegRenderPro(LPVOID pParam) 
{
	CffPlay* pThis = (CffPlay*)pParam;
	while(1)
	{
		//Sleep(10);
		pThis->m_YuvDataList.read(pThis->m_item);
		int renderVideoRet = render_video(0, pThis->m_item->context, 
			pThis->m_item->context + pThis->m_item->width*pThis->m_item->height, 
			pThis->m_item->context +pThis->m_item->width*pThis->m_item->height*5/4, 
			pThis->m_item->width, 
			pThis->m_item->height);

		pThis->m_PcmDataList.read(pThis->m_item);
		int renderAudioRet = render_audio(0, pThis->m_item->context, pThis->m_item->frameLen, 16, pThis->m_item->sampleRate);
	}

}
#if 0
FILE* f_video ;
f_video = fopen("D:\\123.yuv","w+b") ;
fwrite(frame->base[0], (frame->width)*(frame->height)*3/2, 1, f_video);
#endif
DWORD CffPlay::ffmpegDecPro(LPVOID pParam)  
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
		int eof = 0;

		(void)memset(pkt, 0, sizeof(pkt));
		(void)memset(frame, 0, sizeof(frame));
		int ret = av_read_frame(is->ic, pkt);
		if (ret == AVERROR_EOF)
		{
			eof = 1;
			break;
		}
		avcodec_get_frame_defaults(frame);
		if (pkt->stream_index == is->video_stream)
		{
			if(avcodec_decode_video2(is->video_st->codec, frame, &pts, pkt) < 0)
				continue;
			if(frame->data[0] == NULL)
				continue;
			
			int a=0,i;   
			(void)memset(buf, 0, sizeof(buf));
			height = is->ic->streams[is->video_stream]->codec->height;  
			width = is->ic->streams[is->video_stream]->codec->width;  
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

			//int renderVideoRet = render_video(0, buf, buf+width*height, buf+width*height*5/4, width, height);
			pThis->m_item->context = buf;
			pThis->m_item->width = width;
			pThis->m_item->height = height;
			pThis->m_item->frameType = pkt->stream_index;
			while (pThis->m_YuvDataList.write(pThis->m_item) == false)
			{
				Sleep(10);
			}
		}
		if (pkt->stream_index == is->audio_stream)
		{
			if (avcodec_decode_audio4(is->audio_st->codec, frame, &pts, pkt) < 0)
			{
				continue;
			}
			int data_size = av_samples_get_buffer_size(NULL, is->audio_st->codec->channels,
				frame->nb_samples,
				is->audio_st->codec->sample_fmt, 0);
			(void)memset(buf, 0, sizeof(buf));
			memcpy(buf, frame->data[0], data_size);   
			//int renderAudioRet = render_audio(0, buf, data_size, 16, frame->sample_rate);
			pThis->m_item->context = buf;
			pThis->m_item->frameLen = data_size;
			pThis->m_item->sampleRate = frame->sample_rate;
			pThis->m_item->frameType = pkt->stream_index;
			while (pThis->m_PcmDataList.write(pThis->m_item) == false)
			{
				Sleep(10);
			}
		}

	}
	delete [] buf;  
	av_free_packet(pkt);
	avcodec_free_frame(&frame);
	return 0;
}
/* Called from the main */
void CffPlay::playMpegFile(char* fileName, HWND hWnd)
{
	playClose();
	avcodec_register_all();
	av_register_all();

	av_init_packet(&m_flushPkt);

	AVFormatContext *ic = NULL;
	AVCodec *codec;
	int st_index[AVMEDIA_TYPE_NB];
	ic = avformat_alloc_context();
	int err = avformat_open_input(&ic, fileName, NULL, NULL);
	if (err < 0) 
	{
		return;
	}

	av_dump_format(ic, 0, fileName, 0);
	for (int i = 0; i < ic->nb_streams; i++)
		ic->streams[i]->discard = AVDISCARD_ALL;
	m_currentStream.ic = ic;
	st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, st_index[AVMEDIA_TYPE_VIDEO], NULL, 0);
	stream_component_open(&m_currentStream, st_index[AVMEDIA_TYPE_VIDEO]);
	stream_component_open(&m_currentStream, st_index[AVMEDIA_TYPE_AUDIO]);


	int initRet = render_init((long)hWnd);
	int openRet = render_open(0, hWnd, 704, 576, NULL, NULL, ByDDOffscreen, NULL);

	DWORD dw;
	m_ffmpegDecHandler = CreateThread(NULL,0,CffPlay::ffmpegDecPro,this,0,&dw);
	m_ffmpegRenderHandler = CreateThread(NULL,0,CffPlay::ffmpegRenderPro,this,0,&dw);
	return;
}

void CffPlay::playPause()
{
	//toggle_pause(cur_stream);
}

void CffPlay::playClose()
{
	CloseHandle(m_ffmpegDecHandler);
	m_ffmpegDecHandler = NULL;
}

double CffPlay::playGetTotalTime()
{
	/*
	double totalTimeInSec;
	if ((NULL != cur_stream) && (NULL != cur_stream->ic))
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
        break;
    case AVMEDIA_TYPE_VIDEO:
        is->video_st = ic->streams[stream_index];
		is->video_stream = stream_index;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        break;
    default:
        break;
    }
    return 0;
}