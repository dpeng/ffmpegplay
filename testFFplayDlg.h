// testFFplayDlg.h : header file
//

#if !defined(AFX_TESTFFPLAYDLG_H__C1520DE0_080B_4523_B34C_2B1410BE530A__INCLUDED_)
#define AFX_TESTFFPLAYDLG_H__C1520DE0_080B_4523_B34C_2B1410BE530A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include"ffPlay.h"
#include "afxcmn.h"

/////////////////////////////////////////////////////////////////////////////
// CTestFFplayDlg dialog
//#include "ffplay.h"
class CTestFFplayDlg : public CDialog
{
// Construction
public:
	CTestFFplayDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CTestFFplayDlg)
	enum { IDD = IDD_TESTFFPLAY_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestFFplayDlg)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL
// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CTestFFplayDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBtnplay();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnOpenfile();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CString m_strFileName;
	CffPlay m_ffPlay;
	afx_msg void OnBnClickedBtnpause();
	afx_msg void OnBnClickedBtnclose();
	CSliderCtrl m_sliderPlay;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTFFPLAYDLG_H__C1520DE0_080B_4523_B34C_2B1410BE530A__INCLUDED_)
