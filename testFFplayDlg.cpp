// testFFplayDlg.cpp : implementation file
//

#include "stdafx.h"
#include "testFFplay.h"
#include "testFFplayDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestFFplayDlg dialog

CTestFFplayDlg::CTestFFplayDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestFFplayDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTestFFplayDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestFFplayDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTestFFplayDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_SLIDERPLAYPROGRESS, m_sliderPlay);
}

BEGIN_MESSAGE_MAP(CTestFFplayDlg, CDialog)
	//{{AFX_MSG_MAP(CTestFFplayDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTNPLAY, OnBtnplay)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_OPENFILE, OnOpenfile)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTNPAUSE, &CTestFFplayDlg::OnBnClickedBtnpause)
	ON_BN_CLICKED(IDC_BTNCLOSE, &CTestFFplayDlg::OnBnClickedBtnclose)
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestFFplayDlg message handlers

BOOL CTestFFplayDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	m_sliderPlay.SetRangeMin(0);
	m_sliderPlay.SetRangeMax(100);
	m_sliderPlay.SetPos(0);
	m_sliderPlay.SetLineSize(1);
	m_sliderPlay.SetPageSize(5);
	SetTimer(1,100,NULL) ;
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTestFFplayDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTestFFplayDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTestFFplayDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CTestFFplayDlg::OnBtnplay() 
{
	// TODO: Add your control notification handler code here
	RECT rc = {0};
	GetDlgItem(IDC_STATIC_PLAT)->GetWindowRect(&rc);
	
	char variable[256];
	sprintf(variable, "SDL_WINDOWID=0x%lx", GetDlgItem(IDC_STATIC_PLAT)->GetSafeHwnd());
	m_ffPlay.playMpegFile(m_strFileName.GetBuffer(0), &rc, variable);
}

BOOL CTestFFplayDlg::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class

	return CDialog::Create(IDD, pParentWnd);
}

void CTestFFplayDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here

}

void CTestFFplayDlg::OnOpenfile() 
{
	// TODO: Add your control notification handler code here
	CString tempfilename = _T("");
	CFileDialog FileChooser(TRUE, 
		NULL,
		NULL, 
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("mp4 files(*.mp4)|*.mp4||"));
	
	if (FileChooser.DoModal()==IDOK)    tempfilename = FileChooser.GetPathName() ;
	else    return ;
	
	m_strFileName = tempfilename;
	tempfilename = _T("");
	//after open play the media
	Sleep(10);
	OnBtnplay();
}

void CTestFFplayDlg::OnBnClickedBtnpause()
{
	// TODO: Add your control notification handler code here
	m_ffPlay.playPause();
}

void CTestFFplayDlg::OnBnClickedBtnclose()
{
	// TODO: Add your control notification handler code here
	m_ffPlay.playClose();
	m_sliderPlay.SetPos(0);
}

void CTestFFplayDlg::OnClose() 
{
	OnBnClickedBtnclose();
	CDialog::OnClose();
}

void CTestFFplayDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	m_sliderPlay.SetPos(m_ffPlay.playGetCurrentTime()*100/m_ffPlay.playGetTotalTime());
	CDialog::OnTimer(nIDEvent);
}
