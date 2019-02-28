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

static int write_wav_header(int iBitPerSample,int iChans,unsigned char ucFormat,int iSampleRate,int iTotalSamples,FILE*pFile)
{
	unsigned char header[44];
	unsigned char* p = header;
	unsigned int bytes = (iBitPerSample + 7) / 8;
	float data_size = (float)bytes * iTotalSamples;
	unsigned long word32;
 
	*p++ = 'R'; *p++ = 'I'; *p++ = 'F'; *p++ = 'F';
 
	word32 = (data_size + (44 - 8) < (float)MAXWAVESIZE) ?
		(unsigned long)data_size + (44 - 8)  :  (unsigned long)MAXWAVESIZE;
	*p++ = (unsigned char)(word32 >>  0);
	*p++ = (unsigned char)(word32 >>  8);
	*p++ = (unsigned char)(word32 >> 16);
	*p++ = (unsigned char)(word32 >> 24);
 
	*p++ = 'W'; *p++ = 'A'; *p++ = 'V'; *p++ = 'E';
 
	*p++ = 'f'; *p++ = 'm'; *p++ = 't'; *p++ = ' ';
 
	*p++ = 0x10; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
 
	if (ucFormat == AV_SAMPLE_FMT_FLT)
	{
		*p++ = 0x03; *p++ = 0x00;
	} else {
		*p++ = 0x01; *p++ = 0x00;
	}
 
	*p++ = (unsigned char)(iChans >> 0);
	*p++ = (unsigned char)(iChans >> 8);
 
	word32 = (unsigned long)(iSampleRate + 0.5);
	*p++ = (unsigned char)(word32 >>  0);
	*p++ = (unsigned char)(word32 >>  8);
	*p++ = (unsigned char)(word32 >> 16);
	*p++ = (unsigned char)(word32 >> 24);
 
	word32 = iSampleRate * bytes * iChans;
	*p++ = (unsigned char)(word32 >>  0);
	*p++ = (unsigned char)(word32 >>  8);
	*p++ = (unsigned char)(word32 >> 16);
	*p++ = (unsigned char)(word32 >> 24);
 
	word32 = bytes * iChans;
	*p++ = (unsigned char)(word32 >>  0);
	*p++ = (unsigned char)(word32 >>  8);
 
	*p++ = (unsigned char)(iBitPerSample >> 0);
	*p++ = (unsigned char)(iBitPerSample >> 8);
 
	*p++ = 'd'; *p++ = 'a'; *p++ = 't'; *p++ = 'a';
 
	word32 = data_size < MAXWAVESIZE ?
		(unsigned long)data_size : (unsigned long)MAXWAVESIZE;
	*p++ = (unsigned char)(word32 >>  0);
	*p++ = (unsigned char)(word32 >>  8);
	*p++ = (unsigned char)(word32 >> 16);
	*p++ = (unsigned char)(word32 >> 24);
 
	return fwrite(header, sizeof(header), 1, pFile);
}