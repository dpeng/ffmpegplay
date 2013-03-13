
// renderDemoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "renderDemo.h"
#include "renderDemoDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
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


// CrenderDemoDlg dialog




CrenderDemoDlg::CrenderDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CrenderDemoDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CrenderDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CrenderDemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CrenderDemoDlg message handlers

BOOL CrenderDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_audioFileName = _T("d:\\sys_audio.pcm");
	m_videoFileName = _T("d:\\sys_video.yuv");
	m_nWidth = 704;
	m_nHight = 576;
	UpdateData(false);
	startPlay();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CrenderDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CrenderDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CrenderDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CrenderDemoDlg::startPlay()
{
	UpdateData(true);
	RECT rc;
	GetDlgItem(IDC_STATIC_PLAY)->GetWindowRect(&rc);
	int initRet = render_init((long)GetDlgItem(IDC_STATIC_PLAY)->GetSafeHwnd());
	int openRet = render_open(0, GetDlgItem(IDC_STATIC_PLAY)->GetSafeHwnd(), 704, 576, NULL, NULL, ByDDOffscreen, NULL);
	SetTimer(1,40,NULL);
}

FILE *fpVideo = NULL;
FILE *fpAudio = NULL;
#define AUDIO_SIZE 1024*4
void CrenderDemoDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default

	if (fpVideo == NULL)
	{
		if ((fpVideo = fopen(m_videoFileName,"rb")) != NULL)
		{
			m_bufvideo = new unsigned char[m_nWidth*m_nHight*3/2];
			memset(m_bufvideo , 0 , m_nWidth * m_nHight * 3 / 2);
		}
		else
		{
			KillTimer(1);
			return;
		}
	}
	if (fpAudio == NULL)
	{
		if ((fpAudio = fopen(m_audioFileName,"rb")) != NULL)
		{
			m_bufaudio = new unsigned char[AUDIO_SIZE];
			memset(m_bufaudio , 0 , AUDIO_SIZE);
		}
		else
		{
			KillTimer(1);
			return;
		}
	}
	fread(m_bufvideo, 1, m_nWidth*m_nHight*3/2, fpVideo);
	fread(m_bufaudio, 1, AUDIO_SIZE, fpAudio);
	if (feof(fpVideo))
	{
		stopPlay();
		MessageBox("File End", "Fuck", MB_OK);
		return;
	}
	RECT rc;
	GetDlgItem(IDC_STATIC_PLAY)->GetWindowRect(&rc);
	int renderVideoRet = render_video(0, m_bufvideo, m_bufvideo + m_nHight*m_nWidth, m_bufvideo + m_nHight*m_nWidth*5/4, 704, 576);
	int renderAudioRet = render_audio(0, m_bufaudio, AUDIO_SIZE, 8, 16000);
	CDialogEx::OnTimer(nIDEvent);
}

void CrenderDemoDlg::stopPlay()
{
	KillTimer(1);
	if (fpVideo)
	{
		fclose(fpVideo);
		fpVideo = NULL;
	}
	if (m_bufvideo)
	{
		delete m_bufvideo;
		m_bufvideo = NULL;
	}
	if (fpAudio)
	{
		fclose(fpAudio);
		fpAudio = NULL;
	}
	if (m_bufaudio)
	{
		delete m_bufaudio;
		m_bufaudio = NULL;
	}
	render_close_all();
	GetDlgItem(IDC_STATIC_PLAY)->RedrawWindow();
}