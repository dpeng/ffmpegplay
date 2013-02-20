// testFFplay.h : main header file for the TESTFFPLAY application
//

#if !defined(AFX_TESTFFPLAY_H__2A2B6ED1_7D96_454E_8EE7_1115ABAE7A85__INCLUDED_)
#define AFX_TESTFFPLAY_H__2A2B6ED1_7D96_454E_8EE7_1115ABAE7A85__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTestFFplayApp:
// See testFFplay.cpp for the implementation of this class
//

class CTestFFplayApp : public CWinApp
{
public:
	CTestFFplayApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestFFplayApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTestFFplayApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTFFPLAY_H__2A2B6ED1_7D96_454E_8EE7_1115ABAE7A85__INCLUDED_)
