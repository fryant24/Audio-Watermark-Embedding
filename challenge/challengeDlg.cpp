
// challengeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "challenge.h"
#include "challengeDlg.h"
#include "afxdialogex.h"
#include <stdlib.h>
#include <string.h>
#include "audio.h"  
#include <faac.h>
#include "neaacdec.h" 
#include "demux1.h"
#include "demux2.h"
#include "write_wav_headerh.h"
#include "encode1.h"
#include "encode2.h"
#include "decode1.h"
#include "decode2.h"
#include "mux1.h"
#include "mux2.h"
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


#define DEMUXER_MP4 1
#define DEMUXER_FLV 0
#define MUXER_MP4 1
#define MUXER_TS 0
#define ENCODER_1 1
#define ENCODER_2 0
#define DECODER_1 1
#define DECODER_2 0
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
	int    size0;          //波形块的大小
	char   wave_fmt[8];    //"wave" and "fmt"
	int    size1;          //格式块的大小
	short  fmttag;         //波形编码格式
	short  channel;        //波形文件数据中的通道数
	int    sampl;          //波形文件的采样率
	int    bytepersecblockalign;  //平均每秒波形音频所需要的记录的字节数
    short  blockalign;      //一个采样所需要的字节数
    short  bitpersamples;   //声音文件数据的每个采样的位数
	char   data[4];         //"data"
	int    datasize;        //数据块大小
};
CString strFileName;
CString ssub;
char *out_filename = "done？";//Output file URL
AVOutputFormat *ofmt_a = NULL,*ofmt_v = NULL;  
    //（Input AVFormatContext and Output AVFormatContext）  
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx_a = NULL, *ofmt_ctx_v = NULL;  
    AVPacket pkt;  
    int ret, i;  
    int videoindex=-1,audioindex=-1;  
    int frame_index=0;  
  
    const char *in_filename ;//Input file URL   
    const char *out_filename_v = "DEMUXED.h264";//Output file URL   
    const char *out_filename_a = "DEMUXED.aac";  

	AVOutputFormat *ofmt = NULL;  
    //Input AVFormatContext and Output AVFormatContext  
    AVFormatContext *ifmt_ctx_v = NULL, *ifmt_ctx_a = NULL,*ofmt_ctx = NULL;  
    AVPacket pkt2;  
    int ret2, i2;  
    int videoindex_v=-1,videoindex_out=-1;  
    int audioindex_a=-1,audioindex_out=-1;  
    int frame_index2=0;  
    int64_t cur_pts_v=0,cur_pts_a=0;  
    const char *in_filename_v = "DEMUXED.h264";  
    const char *in_filename_a = "ENCODED.aac";    

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CchallengeDlg 对话框



CchallengeDlg::CchallengeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CchallengeDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

void CchallengeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CchallengeDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CchallengeDlg::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_DEMUX, &CchallengeDlg::OnBnClickedButtonDemux)
	ON_BN_CLICKED(IDC_BUTTON_MUX, &CchallengeDlg::OnBnClickedButtonMux)

	ON_BN_CLICKED(IDC_BUTTON_DECODE, &CchallengeDlg::OnBnClickedButtonDecode)
	ON_BN_CLICKED(IDC_BUTTON_encode, &CchallengeDlg::OnBnClickedButtonencode)
END_MESSAGE_MAP()


// CchallengeDlg 消息处理程序

BOOL CchallengeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}


void CchallengeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CchallengeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CchallengeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CchallengeDlg::OnBnClickedButtonOpen()
{
       CFileDialog dlg(TRUE, _T("*.mp4"), NULL,NULL,_T("视频文件(*.MP4,*.FLV,*.MKV,*.TS)|*.mp4;*.MP4;*.flv;*.FLV;*.mkv;*.MKV;*.ts;*.TS|"),this);
		//CString strFileName;//记录选择文件路径
       if (!dlg.DoModal() == IDOK) return;
       strFileName = dlg.GetPathName();
	   int nPos = strFileName.Find('\\');
		CString sSubStr = strFileName;
		 while (nPos)
			{
			 sSubStr = sSubStr.Mid(nPos+1,sSubStr.GetLength()-nPos);  //取'\'右边字符串
		  nPos = sSubStr.Find('\\');   //不包含'\'，函数值返回-1 

		     if (nPos==-1)    
			    {
				        nPos = 0;
			   }
		 }
		ssub="NEW_"+sSubStr;
		out_filename=(LPSTR)(LPCSTR)(ssub);
        SetDlgItemText(IDC_EDIT_LOCATION,dlg.GetPathName()); 

}

void CchallengeDlg::OnBnClickedButtonDemux()
{
	CStringA temp = strFileName.GetBuffer(0); //通过转化，temp接受了原来字符串的多字节形式  
    string dst = temp.GetBuffer(0); //现在就可以将值直接赋给string类型了   
	in_filename = dst.c_str() ;

	#if DEMUXER_MP4
	demux1(in_filename,out_filename_v,out_filename_a);
	#endif	

	#if DEMUXER_FLV
	demux2(in_filename,out_filename_v,out_filename_a);
	#endif

}



void CchallengeDlg::OnBnClickedButtonDecode()
{
	#if DECODER_1
	decode1(INPUT_FILE_NAME,OUTPUT_FILE_NAME);
	#endif	

	#if DECODER_2
	decode2(INPUT_FILE_NAME,OUTPUT_FILE_NAME);
	#endif

}


void CchallengeDlg::OnBnClickedButtonencode()
{
	GetDlgItem(IDC_STATIC_waiting)->ShowWindow(true);
	UpdateWindow();

	#if ENCODER_1
	encode1();
	#endif	

	#if ENCODER_2
	encode2();
	#endif

	GetDlgItem(IDC_STATIC_waiting)->ShowWindow(false);
	UpdateWindow();
    AfxMessageBox("已成功由wav编码为aac!\n");
}

void CchallengeDlg::OnBnClickedButtonMux()
{

	#if MUXER_MP4
	mux1(in_filename_v,in_filename_a,out_filename);
	#endif	

	#if MUXER_TS
	mux2(in_filename_v,in_filename_a,out_filename);
	#endif

}
