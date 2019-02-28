
// challengeDlg.h : 头文件
//

#pragma once


// CchallengeDlg 对话框
class CchallengeDlg : public CDialogEx
{
// 构造
public:
	CchallengeDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_CHALLENGE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButtonOpen();
	afx_msg void OnBnClickedButtonDemux();
	afx_msg void OnBnClickedButtonMux();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangeEditLocation();
	afx_msg void OnBnClickedButtonDecode();
	afx_msg void OnBnClickedButtonencode();
	afx_msg void OnStnClickedStaticwaiting();
	afx_msg void OnBnClickedButtonWatermark();
};
