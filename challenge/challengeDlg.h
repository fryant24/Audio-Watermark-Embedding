
// challengeDlg.h : ͷ�ļ�
//

#pragma once


// CchallengeDlg �Ի���
class CchallengeDlg : public CDialogEx
{
// ����
public:
	CchallengeDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_CHALLENGE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
