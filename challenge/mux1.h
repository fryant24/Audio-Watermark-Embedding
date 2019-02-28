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
void mux1(const char *in_filename_v,const char *in_filename_a,const char *out_filename){

	AVOutputFormat *ofmt = NULL;  
    //Input AVFormatContext and Output AVFormatContext  
    AVFormatContext *ifmt_ctx_v = NULL, *ifmt_ctx_a = NULL,*ofmt_ctx = NULL;  
    AVPacket pkt2;  
    int ret2, i2;  
    int videoindex_v=-1,videoindex_out=-1;  
    int audioindex_a=-1,audioindex_out=-1;  
    int frame_index2=0;  
    int64_t cur_pts_v=0,cur_pts_a=0; 

	av_register_all();
	if ((ret2 = avformat_open_input(&ifmt_ctx_v, in_filename_v, 0, 0)) < 0) {  
        AfxMessageBox( "Could not open input file.");  
        goto end2;  
    }  
    if ((ret2 = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {  
        AfxMessageBox( "Failed to retrieve input stream information");  
        goto end2;  
    }  
  
    if ((ret2 = avformat_open_input(&ifmt_ctx_a, in_filename_a, 0, 0)) < 0) {  
        AfxMessageBox( "Could not open input file.");  
        goto end2;  
    }  
    if ((ret2 = avformat_find_stream_info(ifmt_ctx_a, 0)) < 0) {  
        AfxMessageBox( "Failed to retrieve input stream information");  
        goto end2;  
    }  
    av_dump_format(ifmt_ctx_v, 0, in_filename_v, 0);  
    av_dump_format(ifmt_ctx_a, 0, in_filename_a, 0);   
    //Output  
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);  
    if (!ofmt_ctx) {  
        AfxMessageBox( "Could not create output context\n");  
        ret2 = AVERROR_UNKNOWN;  
        goto end2;  
    }  
    ofmt = ofmt_ctx->oformat;  
  
    for (i2 = 0; i2 < ifmt_ctx_v->nb_streams; i2++) {  
        //Create output AVStream according to input AVStream  
        if(ifmt_ctx_v->streams[i2]->codec->codec_type==AVMEDIA_TYPE_VIDEO){  
        AVStream *in_stream = ifmt_ctx_v->streams[i2];  
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);  
        videoindex_v=i2;  
        if (!out_stream) {  
            AfxMessageBox( "Failed allocating output stream\n");  
            ret2 = AVERROR_UNKNOWN;  
            goto end2;  
        }  
        videoindex_out=out_stream->index;  
        //Copy the settings of AVCodecContext  
        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {  
            AfxMessageBox ("Failed to copy context from input to output stream codec context\n");  
            goto end2;  
        }  
        out_stream->codec->codec_tag = 0;  
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)  
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;  
        break;  
        }  
    }  
  
    for (i2 = 0; i2 < ifmt_ctx_a->nb_streams; i2++) {  
        //Create output AVStream according to input AVStream  
        if(ifmt_ctx_a->streams[i2]->codec->codec_type==AVMEDIA_TYPE_AUDIO){  
            AVStream *in_stream = ifmt_ctx_a->streams[i2];  
            AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);  
            audioindex_a=i2;  
            if (!out_stream) {  
                AfxMessageBox( "Failed allocating output stream\n");  
                ret2 = AVERROR_UNKNOWN;  
                goto end2;  
            }  
            audioindex_out=out_stream->index;  
            //Copy the settings of AVCodecContext  
            if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {  
                AfxMessageBox( "Failed to copy context from input to output stream codec context\n");  
                goto end2;  
            }  
            out_stream->codec->codec_tag = 0;  
            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)  
                out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;  
  
            break;  
        }  
    }  
  

    av_dump_format(ofmt_ctx, 0, out_filename, 1);   
    //Open output file  
    if (!(ofmt->flags & AVFMT_NOFILE)) {  
        if (avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {  
           AfxMessageBox( "Could not open output file \n");  
            goto end2;  
        }  
    }  
    //Write file header  
    if (avformat_write_header(ofmt_ctx, NULL) < 0) {  
        AfxMessageBox( "Error occurred when opening output file\n");  
        goto end2;  
    }  
  
  
    //FIX  
#if USE_H264BSF  
    AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");   
#endif  
#if USE_AACBSF  
    AVBitStreamFilterContext* aacbsfc =  av_bitstream_filter_init("aac_adtstoasc");   
#endif  
  
    while (1) {  
        AVFormatContext *ifmt_ctx;  
        int stream_index=0;  
        AVStream *in_stream, *out_stream;  
  
        //Get an AVPacket  
        if(av_compare_ts(cur_pts_v,ifmt_ctx_v->streams[videoindex_v]->time_base,cur_pts_a,ifmt_ctx_a->streams[audioindex_a]->time_base) <= 0){  
            ifmt_ctx=ifmt_ctx_v;  
            stream_index=videoindex_out;  
  
            if(av_read_frame(ifmt_ctx, &pkt2) >= 0){  
                do{  
                    in_stream  = ifmt_ctx->streams[pkt2.stream_index];  
                    out_stream = ofmt_ctx->streams[stream_index];  
  
                    if(pkt2.stream_index==videoindex_v){  
                        //FIX£ºNo PTS (Example: Raw H.264)  
                        //Simple Write PTS  
                        if(pkt2.pts==AV_NOPTS_VALUE){  
                            //Write PTS  
                            AVRational time_base1=in_stream->time_base;  
                            //Duration between 2 frames (us)  
                            int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(in_stream->r_frame_rate);  
                            //Parameters  
                            pkt2.pts=(double)(frame_index2*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);  
                            pkt2.dts=pkt2.pts;  
                            pkt2.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);  
                            frame_index2++;  
                        }  
  
                        cur_pts_v=pkt2.pts;  
                        break;  
                    }  
                }while(av_read_frame(ifmt_ctx, &pkt2) >= 0);  
            }else{  
                break;  
            }  
        }else{  
            ifmt_ctx=ifmt_ctx_a;  
            stream_index=audioindex_out;  
            if(av_read_frame(ifmt_ctx, &pkt2) >= 0){  
                do{  
                    in_stream  = ifmt_ctx->streams[pkt2.stream_index];  
                    out_stream = ofmt_ctx->streams[stream_index];  
  
                    if(pkt2.stream_index==audioindex_a){  
  
                        //FIX£ºNo PTS  
                        //Simple Write PTS  
                        if(pkt2.pts==AV_NOPTS_VALUE){  
                            //Write PTS  
                            AVRational time_base1=in_stream->time_base;  
                            //Duration between 2 frames (us)  
                            int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(in_stream->r_frame_rate);  
                            //Parameters  
                            pkt2.pts=(double)(frame_index2*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);  
                            pkt2.dts=pkt2.pts;  
                            pkt2.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);  
                            frame_index2++;  
                        }  
                        cur_pts_a=pkt2.pts;  
  
                        break;  
                    }  
                }while(av_read_frame(ifmt_ctx, &pkt2) >= 0);  
            }else{  
                break;  
            }  
  
        }  
  
        //FIX:Bitstream Filter  
#if USE_H264BSF  
        av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);  
#endif  
#if USE_AACBSF  
        av_bitstream_filter_filter(aacbsfc, out_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);  
#endif  
  
  
        //Convert PTS/DTS  
        pkt2.pts = av_rescale_q_rnd(pkt2.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));  
        pkt2.dts = av_rescale_q_rnd(pkt2.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));  
        pkt2.duration = av_rescale_q(pkt2.duration, in_stream->time_base, out_stream->time_base);  
        pkt2.pos = -1;  
        pkt2.stream_index=stream_index;  
  
        //Write  
        if (av_interleaved_write_frame(ofmt_ctx, &pkt2) < 0) {  
           AfxMessageBox( "Error muxing packet\n");  
            break;  
        }  
        av_free_packet(&pkt2);  
  
    }  
    //Write file trailer  
    av_write_trailer(ofmt_ctx);  
	AfxMessageBox( "¸´ÓÃÍê³É"); 
  
#if USE_H264BSF  
    av_bitstream_filter_close(h264bsfc);  
#endif  
#if USE_AACBSF  
    av_bitstream_filter_close(aacbsfc);  
#endif  
  
end2:  
    avformat_close_input(&ifmt_ctx_v);  
    avformat_close_input(&ifmt_ctx_a);  
    /* close output */  
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))  
        avio_close(ofmt_ctx->pb);  
    avformat_free_context(ofmt_ctx);  
    if (ret2 < 0 && ret2 != AVERROR_EOF) {  
       AfxMessageBox( "Error occurred.\n");  
    }  
}