#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
//SDL
#include "SDL.h"
#include "SDL_thread.h"
};
 
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
 
#define OUTPUT_PCM 1
#define USE_SDL 1
 
static  Uint8  *audio_chunk; 
static  Uint32  audio_len; 
static  Uint8  *audio_pos; 
 
void  fill_audio(void *udata,Uint8 *stream,int len){ 
	//SDL 2.0
	SDL_memset(stream, 0, len);
	if(audio_len==0)		/*  Only  play  if  we  have  data  left  */ 
			return; 
	len=(len>audio_len?audio_len:len);	/*  Mix  as  much  data  as  possible  */ 
 
	SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);
	audio_pos += len; 
	audio_len -= len; 
} 
//-----------------
 
int getADTSframe(unsigned char* buffer, int buf_size, unsigned char* data ,int* data_size){
	int size = 0;
 
	if(!buffer || !data || !data_size ){
		return -1;
	}
 
	while(1){
		if(buf_size  < 7 ){
			return -1;
		}
		//Sync words
		if((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0) ){
			size |= ((buffer[3] & 0x03) <<11);     //high 2 bit
			size |= buffer[4]<<3;                //middle 8 bit
			size |= ((buffer[5] & 0xe0)>>5);        //low 3bit
			break;
		}
		--buf_size;
		++buffer;
	}
 
	if(buf_size < size){
		return 1;
	}
 
	memcpy(data, buffer, size);
	*data_size = size;
 
	return 0;
}
 
int nWriteBytes=0;
int _tmain(const char *filename,const char *outname)
{
	AVFormatContext	*pFormatCtx;
	int				i, audioStream;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
 
 
	av_register_all();
	pCodec=avcodec_find_decoder(AV_CODEC_ID_AAC);
	if(pCodec==NULL){
		printf("Codec not found.\n");
		return -1;
	}
 
	pCodecCtx = avcodec_alloc_context3(pCodec);
 
	// Open codec
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
		printf("Could not open codec.\n");
		return -1;
	}
 
	FILE *pFile=NULL;
	FILE *pInFile = NULL;
#if OUTPUT_PCM
	pFile=fopen(outname, "wb");
	pInFile=fopen(filename, "rb");
#endif
 
	//Out Audio Param
	uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
	int out_nb_samples=1024;
	AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;
	int out_sample_rate=44100;
	int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);
	//输出内存大小
	int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);
	uint8_t *out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
 
	AVFrame *pFrame = av_frame_alloc();
	av_frame_unref(pFrame);
 
//SDL------------------
#if USE_SDL
	//Init
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	}
	//SDL_AudioSpec
	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = out_sample_rate; 
	wanted_spec.format = AUDIO_S16SYS; 
	wanted_spec.channels = out_channels; 
	wanted_spec.silence = 0; 
	wanted_spec.samples = out_nb_samples/*pCodecCtx->frame_size*/; 
	wanted_spec.callback = fill_audio; 
	wanted_spec.userdata = pCodecCtx; 
		
	if (SDL_OpenAudio(&wanted_spec, NULL)<0){ 
		printf("can't open audio.\n"); 
		return -1; 
	} 
#endif
	////printf("Bitrate:\t %3d\n", pFormatCtx->bit_rate);
	//printf("Decoder Name:\t %s\n", pCodecCtx->codec->long_name);
	//printf("Channels:\t %d\n", pCodecCtx->channels);
	//printf("Sample per Second\t %d \n", pCodecCtx->sample_rate);
 
	uint32_t ret,len = 0;
	int got_picture;
	int index = 0;
	struct SwrContext *au_convert_ctx;
	au_convert_ctx = swr_alloc();
	//au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,
	//	pCodecCtx->channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);
	//swr_init(au_convert_ctx);
	
	int data_size = 0;
	int size = 0;
	int cnt=0;
	int offset=0;
 
	FILE *myout=stdout;
 
	unsigned char *aacframe=(unsigned char *)malloc(1024*5);
	unsigned char *aacbuffer=(unsigned char *)malloc(1024*1024);
 
	FILE *ifile = fopen(filename, "rb");
	if(!ifile){
		printf("Open file error");
		return -1;
	}
 
	printf(" aac NUM - Profile - Frequency - Size \n");
 
	while(!feof(ifile)){
		data_size = fread(aacbuffer+offset, 1, 1024*1024-offset, ifile);
		unsigned char* input_data = aacbuffer+offset;
 
		while(1)
		{
			int ret=getADTSframe(input_data, data_size, aacframe, &size);
			if(ret==-1){
				break;
			}else if(ret==1){
				memcpy(aacbuffer,input_data,data_size);
				offset=data_size;
				break;
			}
 
			char profile_str[10]={0};
			char frequence_str[10]={0};
 
			unsigned char profile=aacframe[2]&0xC0;
			profile=profile>>6;
			switch(profile){
			case 0: sprintf(profile_str,"Main");break;
			case 1: sprintf(profile_str,"LC");break;
			case 2: sprintf(profile_str,"SSR");break;
			default:sprintf(profile_str,"unknown");break;
			}
 
			unsigned char sampling_frequency_index=aacframe[2]&0x3C;
			sampling_frequency_index=sampling_frequency_index>>2;
			switch(sampling_frequency_index){
			case 0: sprintf(frequence_str,"96000Hz");break;
			case 1: sprintf(frequence_str,"88200Hz");break;
			case 2: sprintf(frequence_str,"64000Hz");break;
			case 3: sprintf(frequence_str,"48000Hz");break;
			case 4: sprintf(frequence_str,"44100Hz");break;
			case 5: sprintf(frequence_str,"32000Hz");break;
			case 6: sprintf(frequence_str,"24000Hz");break;
			case 7: sprintf(frequence_str,"22050Hz");break;
			case 8: sprintf(frequence_str,"16000Hz");break;
			case 9: sprintf(frequence_str,"12000Hz");break;
			case 10: sprintf(frequence_str,"11025Hz");break;
			case 11: sprintf(frequence_str,"8000Hz");break;
			default:sprintf(frequence_str,"unknown");break;
			}
			
			AVPacket avpkt;
			av_init_packet(&avpkt);
			avpkt.data = (uint8_t *)aacframe;  
			avpkt.size = size;
 
			while(avpkt.size>0)
			{
				ret = avcodec_decode_audio4( pCodecCtx, pFrame,&got_picture, &avpkt);
				if ( ret < 0 ) {
					printf("Error in decoding audio frame.\n");
					return -1;
				}
 
				//需要解码后初始化，否则pCodecCtx的参数尚未获取到（纯净版不用预先指定参数）
				static bool bSwrInit=false;
				if(!bSwrInit){
					au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,
						pCodecCtx->channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);
					swr_init(au_convert_ctx);
					bSwrInit=true;
				}
				
				if ( got_picture > 0 ){					
					int nBytes = swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);
					if(nBytes<0){
						printf("swr_convert failed \n");
						return 0;
					}
					nWriteBytes = av_samples_get_buffer_size(NULL, out_channels, nBytes, out_sample_fmt, 1);
					if(nWriteBytes<0){
						printf("av_samples_get_buffer_size failed \n");
						return 0;
					}
					fwrite(out_buffer, 1, nWriteBytes, pFile);
				}
 
				//设置音频数据缓冲,PCM数据
				audio_chunk = (Uint8 *) out_buffer; 
				//设置音频数据长度
				audio_len = nWriteBytes;
				audio_pos = audio_chunk;
				//回放音频数据 
				SDL_PauseAudio(0);
				while(audio_len>0)//等待直到音频数据播放完毕! 
					SDL_Delay(1); 
	
				avpkt.data+=ret;
				avpkt.size-=ret;
			}
			av_free_packet(&avpkt);
			
			data_size -= size;
			input_data += size;
			cnt++;
		}   
 
	}
	fclose(ifile);
	free(aacbuffer);
	fclose(pFile);	
 
#if USE_SDL
	SDL_CloseAudio();//关闭音频设备 
	SDL_Quit();
#endif
	
	swr_free(&au_convert_ctx);
	av_free(out_buffer);
	avcodec_close(pCodecCtx);
 
	return 0;
}