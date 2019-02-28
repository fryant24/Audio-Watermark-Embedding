#include <stdio.h>
 
#define __STDC_CONSTANT_MACROS
 
 
extern "C"
{
#include "libavformat/avformat.h"
};
 
 
 
//��װ��ʽMKV/MP4/FLV�������AAC�������������Щ��װ��ʽ�а���AudioSpecificConfig��
//��������Ƶ����AVCodecContext->extradata ���棬��Ҫ����Ȼ���װADTS����
 
 
 
#define DEMUXER_AAC 1
#define DEMUXER_MP3 0
 
#define  ADTS_HEADER_SIZE (7)
 
//FLV��װ����Ƶ AAC��װ�ڵ�һ��AAC TAG���װһ��AudioSpecificConfig�ṹ 
//AudioSpecificConfig������������ڸýṹ����
typedef struct  
{
	int write_adts;  
	int objecttype;  
	int sample_rate_index;  
	int channel_conf;  
 
}ADTSContext;  
 
 
//����AudioSpecificConfig
int aac_decode_extradata(ADTSContext *adts, unsigned char *pbuf, int bufsize)  
{  
	int aot, aotext, samfreindex;  
	int i, channelconfig;  
	unsigned char *p = pbuf;  
	if (!adts || !pbuf || bufsize<2)  
	{  
		return -1;  
	}  
	aot = (p[0]>>3)&0x1f;  
	if (aot == 31)  
	{  
		aotext = (p[0]<<3 | (p[1]>>5)) & 0x3f;  
		aot = 32 + aotext;  
		samfreindex = (p[1]>>1) & 0x0f;   
		if (samfreindex == 0x0f)  
		{  
			channelconfig = ((p[4]<<3) | (p[5]>>5)) & 0x0f;  
		}  
		else  
		{  
			channelconfig = ((p[1]<<3)|(p[2]>>5)) & 0x0f;  
		}  
	}  
	else  
	{  
		samfreindex = ((p[0]<<1)|p[1]>>7) & 0x0f;  
		if (samfreindex == 0x0f)  
		{  
			channelconfig = (p[4]>>3) & 0x0f;  
		}  
		else  
		{  
			channelconfig = (p[1]>>3) & 0x0f;  
		}  
	}  
#ifdef AOT_PROFILE_CTRL  
	if (aot < 2) aot = 2;  
#endif  
	adts->objecttype = aot-1;  
	adts->sample_rate_index = samfreindex;  
	adts->channel_conf = channelconfig;  
	adts->write_adts = 1;  
	return 0;  
}  
 
//���ADTSͷ
int aac_set_adts_head(ADTSContext *acfg, unsigned char *buf, int size)  
{         
	unsigned char byte;    
	if (size < ADTS_HEADER_SIZE)  
	{  
		return -1;  
	}       
	buf[0] = 0xff;  
	buf[1] = 0xf1;  
	byte = 0;  
	byte |= (acfg->objecttype & 0x03) << 6;  
	byte |= (acfg->sample_rate_index & 0x0f) << 2;  
	byte |= (acfg->channel_conf & 0x07) >> 2;  
	buf[2] = byte;  
	byte = 0;  
	byte |= (acfg->channel_conf & 0x07) << 6;  
	byte |= (ADTS_HEADER_SIZE + size) >> 11;  
	buf[3] = byte;  
	byte = 0;  
	byte |= (ADTS_HEADER_SIZE + size) >> 3;  
	buf[4] = byte;  
	byte = 0;  
	byte |= ((ADTS_HEADER_SIZE + size) & 0x7) << 5;  
	byte |= (0x7ff >> 6) & 0x1f;  
	buf[5] = byte;  
	byte = 0;  
	byte |= (0x7ff & 0x3f) << 2;  
	buf[6] = byte;     
	return 0;  
}  
 
 
 
 
 
 
int demux2(const char *in_filename,const char *out_filename_v,const char *out_filename_a)
{
 
	AVFormatContext *ifmt_ctx = NULL;
	AVPacket pkt;
	int ret, i;
	int videoindex=-1,audioindex=-1;

	/*
	#if DEMUXER_AAC
	const char *in_filename    = "titanic.flv";		//Input file URL
	const char *out_filename_v = "titanic.h264";	//Output file URL
	const char *out_filename_a = "titanic.aac";
	#endif
 
	#if DEMUXER_MP3
	const char *in_filename    = "titanic.flv";		//Input file URL
	const char *out_filename_v = "titanic.h264";	//Output file URL
	const char *out_filename_a = "titanic.mp3";
	#endif
	*/
 
 
	av_register_all();
	//Input
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) 
	{
		printf( "Could not open input file.");
		return -1;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) 
	{
		printf( "Failed to retrieve input stream information");
		return -1;
	}
 
	videoindex=-1;
	for(i=0; i<ifmt_ctx->nb_streams; i++) 
	{
		if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoindex=i;
		}else if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
		{
			audioindex=i;
		}
	}
 
	FILE *fp_audio=fopen(out_filename_a,"wb+");  
	FILE *fp_video=fopen(out_filename_v,"wb+");  
 
	//FLV/MP4/MKV�Ƚṹ�У�h264��Ҫh264_mp4toannexb�������SPS/PPS����Ϣ��FLV��װʱ�����԰�
	//���NALU����һ��VIDEO TAG��,�ṹΪ4B NALU����+NALU1+4B NALU����+NALU2+...,��Ҫ���Ĵ����4B
	//���Ȼ���00000001����000001
 
	AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb"); 
 
#if DEMUXER_AAC
	ADTSContext stADTSContext;
	unsigned char pAdtsHead[7];
#endif
 
	while(av_read_frame(ifmt_ctx, &pkt)>=0)
	{
		if(pkt.stream_index==videoindex)
		{
 
			av_bitstream_filter_filter(h264bsfc, ifmt_ctx->streams[videoindex]->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
 
			printf("Write Video Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
			fwrite(pkt.data,1,pkt.size,fp_video);
		}
		else if(pkt.stream_index==audioindex)
		{
			//AAC�ڷ�װ�ṹMKV/FLV/MP4�ṹ�У���Ҫ�ֶ����ADTS
	#if DEMUXER_AAC
			aac_decode_extradata(&stADTSContext, ifmt_ctx->streams[audioindex]->codec->extradata, ifmt_ctx->streams[audioindex]->codec->extradata_size);
			aac_set_adts_head(&stADTSContext, pAdtsHead, pkt.size);
			fwrite(pAdtsHead, 1, 7, fp_audio);
	#endif
			//һ��ṹ��MP3ֱ��д�ļ�����
			
			printf("Write Audio Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
			fwrite(pkt.data,1,pkt.size,fp_audio);
		}
		av_free_packet(&pkt);
	}
 
 
	av_bitstream_filter_close(h264bsfc);  
 
 
	fclose(fp_video);
	fclose(fp_audio);
 
	avformat_close_input(&ifmt_ctx);
 
	if (ret < 0 && ret != AVERROR_EOF) 
	{
		printf( "Error occurred.\n");
		return -1;
	}
	return 0;
}