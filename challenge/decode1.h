#include "stdafx.h"
#include "challenge.h"
#include "challengeDlg.h"
#include "afxdialogex.h"
#include <stdlib.h>
#include <string.h>
#include "audio.h"  
#include <faac.h>
#include "neaacdec.h" 
#include "write_wav_headerh.h"
#include <stdio.h>  
#include <string>


#define __STDC_CONSTANT_MACROS  

  

extern "C"  
{  
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>  
	#include <libswresample/swresample.h>
	#include <libswscale/swscale.h>   
	#include <libavutil/imgutils.h>    
	#include <libavutil/opt.h>       
	#include <libavutil/mathematics.h>     
	#include <libavutil/samplefmt.h>  
};  


typedef unsigned long   ULONG;
typedef unsigned int    UINT;
typedef unsigned char   BYTE;
typedef char            _TCHAR;
#define USE_H264BSF 0 
#define INPUT_FILE_NAME		 ("DEMUXED.aac")
#define OUTPUT_FILE_NAME	 ("DECODED.wav")
#define  MAXWAVESIZE         (4294967040LU)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
using namespace std;


void decode1(const char *filename,const char *outname){
	FILE *pOutFile=NULL;
 
	pOutFile = fopen(outname, "wb");
	if(pOutFile == NULL)
	{
		AfxMessageBox( "出错！\n");  
	}
	//注册解码器
	av_register_all();
	avformat_network_init();
 
	AVFormatContext	* pFormatCtx = avformat_alloc_context();
	//打开封装格式
	if(avformat_open_input(&pFormatCtx,filename,NULL,NULL)!=0)
	{
		AfxMessageBox( "出错！\n");  
	}
	if(avformat_find_stream_info(pFormatCtx,NULL)<0)
	{
		AfxMessageBox("封装格式查找流失败\n");
	}
	// Dump valid information onto standard error
	av_dump_format(pFormatCtx, 0, filename, false);
 
	// Find the first audio stream
	int audioStream = -1;
	for(int i=0; i < pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
		{
			audioStream=i;
			break;
		}
	}
	if(audioStream==-1)
	{
		AfxMessageBox("Didn't find a audio stream.\n");
	}
 
	// Get a pointer to the codec context for the audio stream
	AVCodecContext	*pCodecCtx=pFormatCtx->streams[audioStream]->codec;
 
	// Find the decoder for the audio stream
	AVCodec			*pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL)
	{
		AfxMessageBox("Codec not found.\n");
	}
 
	// Open codec
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
	{
		AfxMessageBox("Could not open codec.\n");
	}
 
	//对AAC编码文件来说，编码根据音频参数编码，解码根据音频参数重新构建声波，FFMPEG构建的
	//音频存储方式不一定支持播放，所以需要重采样样本 如AAC解码的样本格式是AV_SAMPLE_FMT_FLTP
 
	uint64_t iInputLayout				= av_get_default_channel_layout(pCodecCtx->channels);
	int      iInputChans				= pCodecCtx->channels;
	AVSampleFormat eInputSampleFormat   = pCodecCtx->sample_fmt;
	int	     iInputSampleRate			= pCodecCtx->sample_rate;
 
 
	uint64_t iOutputLayout				= av_get_default_channel_layout(pCodecCtx->channels);
	int      iOutputChans				= pCodecCtx->channels;
	AVSampleFormat eOutputSampleFormat  = AV_SAMPLE_FMT_S16;
	int	     iOutputSampleRate			= pCodecCtx->sample_rate;
 
	SwrContext *pSwrCtx = swr_alloc_set_opts(NULL,iOutputLayout, eOutputSampleFormat, iOutputSampleRate,
		iInputLayout,eInputSampleFormat , iInputSampleRate,0, NULL);
	swr_init(pSwrCtx);
 
	//AVPacket读取原始解码前的数据
	AVPacket *packet=(AVPacket *)malloc(sizeof(AVPacket));
	av_init_packet(packet);
 
 
	//1帧数据样本数
	int iFrameSamples = pCodecCtx->frame_size;
 
	// 存储原始数据 
	int iRawLineSize = 0;
	int iRawBuffSize  = av_samples_get_buffer_size(&iRawLineSize, iInputChans, iFrameSamples, eInputSampleFormat, 0);
	uint8_t *pRawBuff = (uint8_t *)av_malloc(iRawBuffSize);
 
	//原始数据保存在AVFrame结构体中
	AVFrame* pRawframe = av_frame_alloc();
 
	pRawframe->nb_samples	= iFrameSamples;
	pRawframe->format		= eInputSampleFormat;
	pRawframe->channels     = iInputChans;
 
 
	int iReturn = avcodec_fill_audio_frame(pRawframe, iInputChans, eInputSampleFormat, (const uint8_t*)pRawBuff, iRawBuffSize, 0);
	if(iReturn<0)
	{
		AfxMessageBox( "出错！\n");  
	}
 
 
	// 存储转换后数据 
	int iConvertLineSize = 0;
	int iConvertBuffSize  = av_samples_get_buffer_size(&iConvertLineSize, iOutputChans, iFrameSamples, eOutputSampleFormat, 0);
	uint8_t *pConvertBuff = (uint8_t *)av_malloc(iConvertBuffSize);
 
	//转换后数据保存在AVFrame结构体中
	AVFrame* pConvertframe = av_frame_alloc();
	pConvertframe->nb_samples	= iFrameSamples;
	pConvertframe->format		= eOutputSampleFormat;
	pConvertframe->channels     = iOutputChans;
 
	iReturn = avcodec_fill_audio_frame(pConvertframe, iOutputChans, eOutputSampleFormat, (const uint8_t*)pConvertBuff, iConvertBuffSize, 0);
	if(iReturn<0)
	{
		AfxMessageBox( "出错！\n");  
	}
	int iGetPicture;
	int iDecodeRet;
	int iFrameNo = 0;
 
	write_wav_header(16,iOutputChans,eOutputSampleFormat,iOutputSampleRate,0,pOutFile);
 
 
	while(av_read_frame(pFormatCtx, packet)>=0)
	{
		if(packet->stream_index==audioStream)
		{
 
			iDecodeRet = avcodec_decode_audio4( pCodecCtx, pRawframe,&iGetPicture, packet);
			if ( iDecodeRet < 0 ) 
			{
                AfxMessageBox("Error in decoding audio frame.\n");
            }
			if ( iGetPicture > 0 )
			{
				swr_convert(pSwrCtx, (uint8_t**)pConvertframe->data, iFrameSamples ,(const uint8_t**)pRawframe->data, iFrameSamples );
		
				fwrite(pConvertframe->data[0],pConvertframe->linesize[0],1,pOutFile);
				iFrameNo++;
			}
		}
		av_free_packet(packet);
	}


	av_free(pRawBuff);
	av_free(pConvertBuff);
	swr_free(&pSwrCtx);
	avcodec_close(pCodecCtx);
	
	avformat_close_input(&pFormatCtx);



	fclose(pOutFile);
	AfxMessageBox("已成功由aac解码为wav!\n");
}