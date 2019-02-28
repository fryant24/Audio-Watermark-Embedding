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

struct WaveFileHead
{
	char   riff_id[4];     //"RIFF"
	int    size0;          //���ο�Ĵ�С
	char   wave_fmt[8];    //"wave" and "fmt"
	int    size1;          //��ʽ��Ĵ�С
	short  fmttag;         //���α����ʽ
	short  channel;        //�����ļ������е�ͨ����
	int    sampl;          //�����ļ��Ĳ�����
	int    bytepersecblockalign;  //ƽ��ÿ�벨����Ƶ����Ҫ�ļ�¼���ֽ���
    short  blockalign;      //һ����������Ҫ���ֽ���
    short  bitpersamples;   //�����ļ����ݵ�ÿ��������λ��
	char   data[4];         //"data"
	int    datasize;        //���ݿ��С
};


void encode1(){
	ULONG nSampleRate ;  // ������
    UINT nChannels ;         // ������
    UINT nPCMBitSize ;      // ������λ��
    ULONG nInputSamples = 0;
    ULONG nMaxOutputBytes = 0;
 
    int nRet;
    faacEncHandle hEncoder;
    faacEncConfigurationPtr pConfiguration; 
 
    int nBytesRead;
    int nPCMBufferSize;
    BYTE* pbPCMBuffer;
    BYTE* pbAACBuffer;
 
    FILE* fpIn; // WAV file for input
    FILE* fpOut; // AAC file for output
    
    fopen_s(&fpIn,"MARKED.wav", "rb");
    fopen_s(&fpOut,"ENCODED.aac", "wb");
	WaveFileHead wavehead;
	fread(&wavehead,sizeof(struct WaveFileHead),1,fpIn);//��ȡ�ļ�ͷ����wavehead
	nPCMBitSize=wavehead.bitpersamples;
	nSampleRate=wavehead.sampl;
	nChannels=wavehead.channel;
    // (1) Open FAAC engine
    hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples, &nMaxOutputBytes);
    if(hEncoder == NULL)
    {
        AfxMessageBox("[ERROR] Failed to call faacEncOpen()\n");
    }
    nPCMBufferSize = nInputSamples * nPCMBitSize / 8;
    pbPCMBuffer = new BYTE [nPCMBufferSize];
    pbAACBuffer = new BYTE [nMaxOutputBytes];
 
    // (2.1) Get current encoding configuration
    pConfiguration = faacEncGetCurrentConfiguration(hEncoder);
    pConfiguration->inputFormat = FAAC_INPUT_16BIT;
	pConfiguration->aacObjectType = LOW; //LC����
	pConfiguration->mpegVersion = MPEG4;//
	pConfiguration->useTns = 1 ;//ʱ����������,��ž���������
	pConfiguration->allowMidside =0 ;//
//pConfiguration->bitRate = m_nBitRate/m_uChannelNums;
	pConfiguration->bandWidth = 0 ; //Ƶ��
//pConfiguration->outputFormat = isADTS; //����Ƿ����ADTSͷ

//pConfiguration->shortctl = 0 ;
//pConfiguration->quantqual = 50 ;
 
    // (2.2) Set encoding configuration
    nRet = faacEncSetConfiguration(hEncoder, pConfiguration);
    for(int i = 0; 1; i++)
    {
        // �����ʵ���ֽ�������󲻻ᳬ��nPCMBufferSize��һ��ֻ�ж����ļ�βʱ�Ų������ֵ
        nBytesRead = fread(pbPCMBuffer, 1, nPCMBufferSize, fpIn);
 
        // ��������������ʵ�ʶ����ֽ������㣬һ��ֻ�ж����ļ�βʱ�Ų���nPCMBufferSize/(nPCMBitSize/8);
        nInputSamples = nBytesRead / (nPCMBitSize / 8);
 
        // (3) Encode
        nRet = faacEncEncode(
        hEncoder, (int*) pbPCMBuffer, nInputSamples, pbAACBuffer, nMaxOutputBytes);
 
        fwrite(pbAACBuffer, 1, nRet, fpOut);
 
        if(nBytesRead <= 0)
        {
            break;
        }
    }
 

 
    // (4) Close FAAC engine
    nRet = faacEncClose(hEncoder);
 
    delete[] pbPCMBuffer;
    delete[] pbAACBuffer;
    fclose(fpIn);
    fclose(fpOut);

}