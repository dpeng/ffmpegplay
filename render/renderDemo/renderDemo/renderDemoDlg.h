
// renderDemoDlg.h : header file
//
#include "../../SRC/render.h"
#pragma once


// CrenderDemoDlg dialog
class CrenderDemoDlg : public CDialogEx
{
// Construction
public:
	CrenderDemoDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_RENDERDEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	unsigned char* m_bufvideo;
	unsigned char* m_bufaudio;
	unsigned char* m_py; 
	unsigned char* m_pu;
	unsigned char* m_pv;
	int m_nWidth;
	int m_nHight;
	CString m_videoFileName;
	CString m_audioFileName;
	void startPlay();
	void stopPlay();
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
