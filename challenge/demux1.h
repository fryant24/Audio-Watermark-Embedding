#include "stdafx.h"
#include "challenge.h"
#include "challengeDlg.h"
#include "afxdialogex.h"
#include <stdlib.h>
#include <string.h>
#include "audio.h"  
#include <faac.h>
#include "neaacdec.h" 
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
void demux1(const char *in_filename,const char *out_filename_v,const char *out_filename_a){
	AVOutputFormat *ofmt_a = NULL,*ofmt_v = NULL;  
    //£¨Input AVFormatContext and Output AVFormatContext£©  
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx_a = NULL, *ofmt_ctx_v = NULL;  
    AVPacket pkt;  
    int ret, i;  
    int videoindex=-1,audioindex=-1;  
    int frame_index=0;  


	av_register_all();  
    //Input  
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {  
		AfxMessageBox(_T(in_filename));
        AfxMessageBox( "Could not open input file.");  
        goto end;  
    }  
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {  
        AfxMessageBox( "Failed to retrieve input stream information");  
        goto end;  
    }  
  
    //Output  
    avformat_alloc_output_context2(&ofmt_ctx_v, NULL, NULL, out_filename_v);  
    if (!ofmt_ctx_v) {  
        AfxMessageBox( "Could not create output context\n");  
        ret = AVERROR_UNKNOWN;  
        goto end;  
    }  
    ofmt_v = ofmt_ctx_v->oformat;  
  
    avformat_alloc_output_context2(&ofmt_ctx_a, NULL, NULL, out_filename_a);  
    if (!ofmt_ctx_a) {  
        AfxMessageBox( "Could not create output context\n");  
        ret = AVERROR_UNKNOWN;  
        goto end;  
    }  
    ofmt_a = ofmt_ctx_a->oformat;  
  
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {  
            //Create output AVStream according to input AVStream  
            AVFormatContext *ofmt_ctx;  
            AVStream *in_stream = ifmt_ctx->streams[i];  
            AVStream *out_stream = NULL;  
              
            if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){  
                videoindex=i;  
                out_stream=avformat_new_stream(ofmt_ctx_v, in_stream->codec->codec);  
                ofmt_ctx=ofmt_ctx_v;  
            }else if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){  
                audioindex=i;  
                out_stream=avformat_new_stream(ofmt_ctx_a, in_stream->codec->codec);  
                ofmt_ctx=ofmt_ctx_a;  
            }else{  
                break;  
            }  
              
            if (!out_stream) {  
                AfxMessageBox( "Failed allocating output stream\n");  
                ret = AVERROR_UNKNOWN;  
                goto end;  
            }  
            //Copy the settings of AVCodecContext  
            if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {  
                AfxMessageBox( "Failed to copy context from input to output stream codec context\n");  
                goto end;  
            }  
            out_stream->codec->codec_tag = 0;  
  
            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)  
                out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER; 
    }  
  
    //Dump Format------------------  
    av_dump_format(ifmt_ctx, 0, in_filename, 0);  
    av_dump_format(ofmt_ctx_v, 0, out_filename_v, 1);  
    av_dump_format(ofmt_ctx_a, 0, out_filename_a, 1);  
    //Open output file  
    if (!(ofmt_v->flags & AVFMT_NOFILE)) {  
        if (avio_open(&ofmt_ctx_v->pb, out_filename_v, AVIO_FLAG_WRITE) < 0) {  
           AfxMessageBox( "Could not open output file\n");  
            goto end;  
        }  
    }  
  
    if (!(ofmt_a->flags & AVFMT_NOFILE)) {  
        if (avio_open(&ofmt_ctx_a->pb, out_filename_a, AVIO_FLAG_WRITE) < 0) {  
            AfxMessageBox( "Could not open output file\n");  
            goto end;  
        }  
    }  
  
    //Write file header  
    if (avformat_write_header(ofmt_ctx_v, NULL) < 0) {  
        AfxMessageBox( "Error occurred when opening video output file\n");  
        goto end;  
    }  
    if (avformat_write_header(ofmt_ctx_a, NULL) < 0) {  
        AfxMessageBox( "Error occurred when opening audio output file\n");  
        goto end;  
    }  
      
#if USE_H264BSF  
    AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");   
#endif  
  
    while (1) {  
        AVFormatContext *ofmt_ctx;  
        AVStream *in_stream, *out_stream;  
        //Get an AVPacket  
        if (av_read_frame(ifmt_ctx, &pkt) < 0)  
            break;  
        in_stream  = ifmt_ctx->streams[pkt.stream_index];  
  
          
        if(pkt.stream_index==videoindex){  
            out_stream = ofmt_ctx_v->streams[0];  
            ofmt_ctx=ofmt_ctx_v;  
#if USE_H264BSF  
            av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);  
#endif  
        }else if(pkt.stream_index==audioindex){  
            out_stream = ofmt_ctx_a->streams[0];  
            ofmt_ctx=ofmt_ctx_a;   
        }else{  
            continue;  
        }  
  
  
        //Convert PTS/DTS  
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));  
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));  
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);  
        pkt.pos = -1;  
        pkt.stream_index=0;  
        //Write  
        if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {  
            AfxMessageBox( "Error muxing packet\n");  
            break;  
        }  
        //printf("Write %8d frames to output file\n",frame_index);  
        av_free_packet(&pkt);  
        frame_index++;  
    }  
  
#if USE_H264BSF  
    av_bitstream_filter_close(h264bsfc);    
#endif  
  
    //Write file trailer  
    av_write_trailer(ofmt_ctx_a);  
    av_write_trailer(ofmt_ctx_v);  
end:  
    avformat_close_input(&ifmt_ctx);  
   
    if (ofmt_ctx_a && !(ofmt_a->flags & AVFMT_NOFILE))  
        avio_close(ofmt_ctx_a->pb);  
  
    if (ofmt_ctx_v && !(ofmt_v->flags & AVFMT_NOFILE))  
        avio_close(ofmt_ctx_v->pb);  
  
    avformat_free_context(ofmt_ctx_a);  
    avformat_free_context(ofmt_ctx_v);  
  
  
    if (ret < 0 && ret != AVERROR_EOF) {  
        AfxMessageBox( "Error occurred.\n");  
          
    }  
	AfxMessageBox( "·ÖÀëÍê³É"); 
}