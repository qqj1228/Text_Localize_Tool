// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"
#include "LDistance.h"
#include "IRichEditOleCallback.h"
#include "CallScript.h"
#include "JumpDlg.h"
#include "Hash.h"

IEDATA g_IEdata;
JUMPDATA g_JumpData;
HWND g_hWndMain, g_hWndRichEdit2, g_hWndJumpDlg=NULL;
WNDPROC g_wpOrigRichEditProc;
extern CString ini_cstrWidthID, ini_cstrWidthJP, ini_cstrWidthCN, ini_cstrWidthCOM;
extern int ini_nBytePerChar, ini_nTextBufLen, ini_nItemPerPage, ini_nWriteFilePerTime, ini_nRE2FintSize;
extern BOOL ini_bEditCtrlchar, ini_bHalf2Full, ini_bSpliterPos, ini_bJPShowCNFont, ini_bCNShowDiff, ini_bOnePageSlider;
extern BOOL ini_bCNCOMNULL, ini_bOutCtrlLen;
extern CString ini_cstrClrNorText, ini_cstrClrCtrlText, ini_cstrClrDiffText, ini_cstrClrOverLenBg;
extern CString ini_cstrClrNorTr, ini_cstrClrSelTr, ini_cstrClrTabBorder, ini_cstrClrBodyBg, ini_cstrClrHiLiREBg;

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{    
	// This was stolen from an SDI app using a form view.
	//
	// Pass keyboard messages along to the child window that has the focus.
	// When the browser has the focus, the message is passed to its containing
	// CAxWindow, which lets the control handle the message.

	if((pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
		(pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST))
		return FALSE;

	HWND hWndCtl = ::GetFocus();

	if(IsChild(hWndCtl))
	{
		// find a direct child of the dialog from the window that has focus
		while(::GetParent(hWndCtl) != m_hWnd)
			hWndCtl = ::GetParent(hWndCtl);

		// give control a chance to translate this message
		if(::SendMessage(hWndCtl, WM_FORWARDMSG, 0, (LPARAM)pMsg) != 0)
			return TRUE;
	}

	// A normal control has the focus, so call IsDialogMessage() so that
	// the dialog shortcut keys work (TAB, etc.)
	return (CWindow::IsDialogMessage(pMsg) || ::IsDialogMessage(g_hWndJumpDlg, pMsg));
}

LRESULT CMainDlg::OnParentNotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	/*CString cstrBuffer;

	if (WM_LBUTTONDOWN==LOWORD(wParam))
	{
		POINT pt;
		pt.x=LOWORD(lParam);
		pt.y=HIWORD(lParam);
		ClientToScreen(&pt);
		m_wndIE.ScreenToClient(&pt);
		if (GetValueFromPt(pt.x, pt.y, cstrBuffer))
		{
			//SetDlgItemText(IDC_RICHEDIT21, cstrBuffer);
		}
	}*/
	return 0;
}

bool CMainDlg::GetValueFromPt(int cx, int cy, CString &cstrValue)
{
	CComPtr<IDispatch> pDisp;
	hr = m_pWB2->get_Document(&pDisp);
	if (SUCCEEDED(hr))   
	{
		CComQIPtr<IHTMLDocument2> pDoc2=pDisp;
		CComPtr<IHTMLElement> pElemHit;
		hr = pDoc2->elementFromPoint(cx, cy, &pElemHit);
		if (SUCCEEDED(hr))
		{
			BSTR b;
			pElemHit->get_id(&b);
			cstrValue=(b == NULL ? _T("") : b);
			SysFreeString(b);
			return true;
		}
	}
	return false;
}

BOOL CMainDlg::OnIdle()
{
	return FALSE;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);
	//加载字符串
	AtlLoadString(IDR_MAINFRAME, m_tcsTitle, MAX_PATH);

	//使主对话框窗口可改变大小
	SetWindowLong(GWL_STYLE, GetWindowLong(GWL_STYLE) | WS_THICKFRAME | WS_CLIPCHILDREN);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	g_hWndMain=this->m_hWnd;

	GetCurrentDirectory(MAX_PATH, m_tcsOriPath);
	m_cstrJPPath=_T("\\jp-text\\");
	m_cstrJPPath=m_tcsOriPath+m_cstrJPPath;
	if(NULL!=::GetTempPath(MAX_PATH, m_cstrTemp.GetBuffer(MAX_PATH)))
	{
		m_cstrTemp.ReleaseBuffer();
		if(NULL!=::GetLongPathName(m_cstrTemp, m_cstrTemp.GetBuffer(MAX_PATH), MAX_PATH))
		{
			m_cstrTemp.ReleaseBuffer();
			m_cstrTemp=m_cstrTemp+_T("temp.html");
		}
	}
	else
	{
		MessageBox(_T("Use application temp folder!"), m_tcsTitle);
		m_cstrTemp=_T("\\temp\\temp.html");
		m_cstrTemp=m_tcsOriPath+m_cstrTemp;
	}

	//初始化各成员函数
	bFixFirstChar=false;
	bFixMidChar=false;
	m_nLastRowNo=0;
	m_nOverLenCount=0;
	m_bTextIsDirty=false;
	m_lpByteRow=NULL;
	m_bShouldScrol=false;
	m_bDocDone=false;
	m_bCtrlHide=false;
	m_bHtmlChanged=true;
	FR_lpcstrText=NULL;
	FR_lpDlg=NULL;
	FR_nItemAll=0;
	FR_nRow=0;
	FR_nPos=0;
	FR_bNextPage=false;
	FR_nLastRow=0;
	FR_nLastPos=0;
	//读取INI文件
	ReadINI();
	//读取控制符效果文件
	ReadCtrlEffect();
	//读取控制符长度文件
	ReadCtrlLen();

	//初始化各个控件
	//IE
	m_wndIE = GetDlgItem(IDC_EXPLORER1);
	CComPtr<IAxWinAmbientDispatch> spAmbient;
	HRESULT hr = m_wndIE.QueryHost(&spAmbient);
	if( SUCCEEDED(hr) )
	{
		spAmbient->put_AllowContextMenu(VARIANT_TRUE);//setup the context menu
		spAmbient->put_DocHostFlags(docHostUIFlagFLAT_SCROLLBAR);//setup the scrollbar
	}

	hr = m_wndIE.QueryControl ( &m_pWB2 );
	if ( m_pWB2 )
	{
		CComVariant v;  // empty variant
		CString cstrResHTML;
		GetModuleFileName((HMODULE)GetWindowLong(GWL_HINSTANCE), cstrResHTML.GetBuffer(MAX_PATH), MAX_PATH);
		cstrResHTML.ReleaseBuffer();
		cstrResHTML=_T("res://")+cstrResHTML+_T("/208");
		start = ::GetTickCount();
		m_pWB2->Navigate ( CComBSTR(cstrResHTML), &v, &v, &v, &v );
	}
	//RichEdit2
	m_CRichEdit2=GetDlgItem(IDC_RICHEDIT21);
	m_CRichEdit2.SetFont(AtlGetDefaultGuiFont());
	CHARFORMAT cf;
	m_CRichEdit2.GetDefaultCharFormat(cf);
	cf.dwMask=CFM_SIZE;
	cf.yHeight=ini_nRE2FintSize*20;
	m_CRichEdit2.SetDefaultCharFormat(cf);
	m_CRichEdit2.SetEventMask(ENM_CHANGE | ENM_PROTECTED | ENM_KEYEVENTS);
	m_IRichEditOleCallback = new CIRichEditOleCallback;
	m_CRichEdit2.SetOleCallback(m_IRichEditOleCallback);
	g_hWndRichEdit2=m_CRichEdit2.m_hWnd;
	//ListView
	m_CFileList=GetDlgItem(IDC_LIST2);
	m_CFileList.SetExtendedListViewStyle(m_CFileList.GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT );
	//ListBox
	m_CInfoList=GetDlgItem(IDC_LIST1);
	//TrackBar
	m_CTrackBar=GetDlgItem(IDC_SLIDER1);
	m_CTrackBar.SetTipSide(TBTS_TOP);
	m_CTrackBar.SetRange(1, 1);
	m_CTrackBar.SetPageSize(1);
	if (!ini_bOnePageSlider)
		m_CTrackBar.EnableWindow(FALSE);

	//计算IE控件和RichEdit2的边距尺寸
	RECT rcClient, rcIE, rcRichEdit2, rcTrackBar;
	GetClientRect(&rcClient);
	m_wndIE.GetWindowRect(&rcIE);
	m_CRichEdit2.GetWindowRect(&rcRichEdit2);
	m_CTrackBar.GetWindowRect(&rcTrackBar);
	ScreenToClient(&rcIE);
	ScreenToClient(&rcRichEdit2);
	ScreenToClient(&rcTrackBar);
	m_rcMarginIE.left=rcIE.left;
	m_rcMarginIE.top=rcIE.top;
	m_rcMarginIE.right=rcClient.right-rcIE.right;
	m_rcMarginIE.bottom=rcClient.bottom-rcIE.bottom;
	m_rcMarginRE2.left=rcRichEdit2.left;
	m_rcMarginRE2.top=rcRichEdit2.top-rcIE.bottom;
	m_rcMarginRE2.right=rcClient.right-rcRichEdit2.right;
	m_rcMarginRE2.bottom=rcClient.bottom-rcRichEdit2.bottom;

	//设置分隔窗口
	const DWORD dwSplitStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	const DWORD dwSplitExStyle =NULL;// WS_EX_CLIENTEDGE;
	RECT rcSplit;
	rcSplit.left=m_rcMarginIE.left;
	rcSplit.right=rcClient.right-m_rcMarginIE.right;
	rcSplit.top=rcIE.top;
	rcSplit.bottom=rcClient.bottom-m_rcMarginRE2.bottom;
	m_wndHorSplit.Create(*this, rcSplit, NULL, dwSplitStyle, dwSplitExStyle);
	m_CRichEdit2.SetParent(m_wndHorSplit);
	m_wndHorSplit.SetSplitterPane(SPLIT_PANE_BOTTOM, m_CRichEdit2);
	if (ini_bSpliterPos)
	{
		//计算分割条位置
		int nTotal=m_wndHorSplit.m_rcSplitter.bottom-m_wndHorSplit.m_rcSplitter.top-m_wndHorSplit.m_cxySplitBar-m_wndHorSplit.m_cxyBarEdge;
		m_nSpliterPos=::MulDiv(m_nSpliterPer, nTotal, 100);
		m_wndHorSplit.SetSplitterPos(m_nSpliterPos);
		BOOL b;
		OnIESize(NULL, NULL, NULL, b);
	}
	else
		m_wndHorSplit.SetSplitterPos(rcIE.bottom-rcIE.top);

	//子类化RichEdit2
	g_wpOrigRichEditProc=(WNDPROC)m_CRichEdit2.SetWindowLong(GWL_WNDPROC, (LONG)RichEditSubclassProc);

	//填充折叠数据
	RECT rcHide;
	::GetWindowRect(GetDlgItem(IDC_BUTTON3), &rcHide);
	::GetWindowRect(GetDlgItem(IDC_BUTTON9), &m_rcNorPosButton);
	::GetWindowRect(GetDlgItem(IDC_STATIC_PAGE), &m_rcNorPosStatic);
	::GetWindowRect(GetDlgItem(IDC_SLIDER1), &m_rcNorPosSlider);
	ScreenToClient(&rcHide);
	ScreenToClient(&m_rcNorPosButton);
	ScreenToClient(&m_rcNorPosStatic);
	ScreenToClient(&m_rcNorPosSlider);
	m_nHidePos=rcHide.top;

	m_CFileList.AddColumn(_T("<===文件名===>"), 0);
	m_CFileList.AddColumn(_T("<=完成条目=>"), 1);
	m_CFileList.AddColumn(_T("<=条目总数=>"), 2);
	//模拟用户按下了"刷新"按钮
	SendMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON2, BN_CLICKED), (LPARAM)::GetDlgItem(m_hWnd, IDC_BUTTON2));

	AtlAdviseSinkMap(this, true);

	if (ini_bEditCtrlchar)
	{
		COLORREF clr;
		IniClr2Str(ini_cstrClrHiLiREBg, clr, false);
		m_CRichEdit2.SetBackgroundColor(clr);
	}
	else
		m_CRichEdit2.SetBackgroundColor(GetSysColor(COLOR_WINDOW));

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	//移除RichEdit2子类化
	m_CRichEdit2.SetWindowLong(GWL_WNDPROC, (LONG)g_wpOrigRichEditProc);

	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	return 0;
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	int nResult=dlg.DoModal();
	if (IDOK==nResult)
	{
		if (ini_bEditCtrlchar)
		{
			COLORREF clr;
			IniClr2Str(ini_cstrClrHiLiREBg, clr, false);
			m_CRichEdit2.SetBackgroundColor(clr);
		}
		else
			m_CRichEdit2.SetBackgroundColor(GetSysColor(COLOR_WINDOW));

		CHARFORMAT cf;
		m_CRichEdit2.GetDefaultCharFormat(cf);
		cf.dwMask=CFM_SIZE;
		cf.yHeight=ini_nRE2FintSize*20;
		m_CRichEdit2.SetDefaultCharFormat(cf);

		ReadCtrlLen();

		BSTR b;
		m_pWB2->get_LocationName(&b);
		CString cstrName(b);
		SysFreeString(b);
		if (cstrName==_T("temp.html"))
		{
			m_bShouldScrol=true;
			m_CFileList.SetFocus();
			//模拟用户按下了空格键
			::keybd_event(VK_SPACE, 0, 0, 0);
			::keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
		}
	}
	return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	//CloseDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_bTextIsDirty)
	{
		SaveText(m_nLastRowNo);
		m_bTextIsDirty=false;
	}
	if (NULL!=m_lpByteRow)
	{
		free(m_lpByteRow);
		m_lpByteRow=NULL;
	}
	WriteINI();
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	AtlAdviseSinkMap(this, false);

	DestroyWindow();
	::PostQuitMessage(nVal);
}

bool CMainDlg::LoadHTMLResource(int nIDR, CString &cstrHTML)
{
	// 查找，加载，锁定资源
	HRSRC res=::FindResource(NULL, MAKEINTRESOURCE(nIDR), RT_HTML);
	if (!res)
	{
		MessageBox(_T("FindResource failed!"), m_tcsTitle);
		return false;
	}
	DWORD dwSize = ::SizeofResource(NULL, res);
	if (!dwSize)
	{
		MessageBox(_T("SizeofResource failed!"), m_tcsTitle);
		return false;
	}
	HGLOBAL gl=::LoadResource(NULL, res);
	if (!gl)
	{
		MessageBox(_T("LoadResource failed!"), m_tcsTitle);
		return false;
	}
	LPVOID lp=::LockResource(gl);
	if (!lp)
	{
		MessageBox(_T("LockResource failed!"), m_tcsTitle);
		return false;
	}
	memcpy(cstrHTML.GetBuffer(dwSize/sizeof(TCHAR)), lp, dwSize);
	cstrHTML.ReleaseBuffer();
	TCHAR *lptch;
	lptch=cstrHTML.GetBuffer(dwSize/sizeof(TCHAR)+1);
	cstrHTML.ReleaseBuffer();
	lptch[dwSize/sizeof(TCHAR)]=_T('\0');
	::FreeResource(gl);
	return true;
}

HRESULT CMainDlg::LoadWebBrowserFromStream(IWebBrowser2* pWebBrowser, IStream* pStream)
{
	HRESULT hr;
	IDispatch* pHtmlDoc = NULL;
	IPersistStreamInit* pPersistStreamInit = NULL;

	// 返回文档对象.
	hr = pWebBrowser->get_Document( &pHtmlDoc );

	if ( SUCCEEDED(hr) )
	{
		// >查询 IPersistStreamInit接口
		hr = pHtmlDoc->QueryInterface( IID_IPersistStreamInit,  (void**)&pPersistStreamInit );
		if ( SUCCEEDED(hr) )
		{
			// 初始化文档.
			hr = pPersistStreamInit->InitNew();
			if ( SUCCEEDED(hr) )
			{
				// 载入流内容
				hr = pPersistStreamInit->Load( pStream );
			}
			pPersistStreamInit->Release();
		}
	}
	return hr;
}

void __stdcall CMainDlg::DocumentCompleteExplorer1(LPDISPATCH pDisp, VARIANT* URL)
{
	// TODO: Add your message handler code here
	//载入页面完成后触发（刷新页面完成后不触发，第二次触发）
	IUnknown* pUnkBrowser = NULL;
	IUnknown* pUnkDisp = NULL;

	//判断是否是顶层框架的DocumentComplete事件（在本例中并非必须的）
	// Is this the DocumentComplete event for the top frame window?
	// Check COM identity: compare IUnknown interface pointers.
	hr = m_pWB2->QueryInterface(IID_IUnknown, (void**)&pUnkBrowser);
	if (SUCCEEDED(hr))
	{
		hr = pDisp->QueryInterface(IID_IUnknown, (void**)&pUnkDisp);
		if (SUCCEEDED(hr))
		{
			if (pUnkBrowser == pUnkDisp)
			{
				// This is the DocumentComplete event for the top frame.
				// This page is loaded, so we can access the DHTML Object Model.
				if (m_bHtmlChanged)
					ShowHtmlComplete();
				m_bDocDone=true;
			}
			pUnkDisp->Release();
		}
		pUnkBrowser->Release();
	}
}

bool CMainDlg::ShowHtmlComplete()
{
	m_CRichEdit2.SetWindowText(_T(""));
	m_bHtmlChanged=true;

	if(m_bShouldScrol)
	{
		CString cstrID;
		cstrID.Format(_T("%d"), m_nLastRowNo);
		//Call JS
		if ( m_CallScript.DocumentSet() == FALSE)
		{
			IDispatch* d = NULL;
			m_pWB2->get_Document(&d);
			m_CallScript.SetDocument(d);
			d->Release();
		}
		m_CallScript.Run(L"monitor_core", (LPCTSTR)cstrID);
		m_bShouldScrol=false;
	}
	else
		m_nLastRowNo=0;

	end = ::GetTickCount();
	CString cstrBuffer;
	cstrBuffer.Format(_T("%d"), end-start);
	m_CInfoList.AddString(_T("显示页面耗时:")+cstrBuffer+_T("ms"));
	m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);

	::EnableWindow(GetDlgItem(ID_APP_ABOUT), TRUE);
	::EnableWindow(GetDlgItem(IDC_CHECK1), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON1), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON2), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON3), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON4), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON5), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON6), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON7), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON8), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON10), TRUE);

	return true;
}

bool CMainDlg::RenderSpec(CString& cstrText)
{
	//用HTML字符实体处理特殊字符
	cstrText.Replace(_T("&"), _T("&amp;"));//必须放在第一个处理
	cstrText.Replace(_T(" "), _T("&nbsp;"));
	cstrText.Replace(_T("<"), _T("&lt;"));
	return true;
}
bool CMainDlg::RenderCtrl(CString& cstrText)
{
	int nPos=0, nStart, nEnd, nLenBefore, nLenAfter;

	//着色控制符
	if (BST_UNCHECKED==IsDlgButtonChecked(IDC_CHECK1))
	{
		while (1)
		{
			nLenBefore=cstrText.GetLength();
			nStart=cstrText.Find(_T('{'), nPos);
			nEnd=cstrText.Find(_T('}'), nPos)+1;
			if (-1==nStart && 0==nEnd)//没有需要处理的控制符了
				break;
			if (nStart>=nEnd || -1==nStart)
			{
				MessageBox(_T("'{'&'}' don't match!"), m_tcsTitle);
				return false;
			}
			nLenAfter=cstrText.Insert(nStart, _T("<font color=")+ini_cstrClrCtrlText+_T(">"));
			nLenAfter=cstrText.Insert(nEnd+(nLenAfter-nLenBefore), _T("</font>"));
			nPos=nEnd+(nLenAfter-nLenBefore);
		}
	}
	else
	{
		if (!CtrlEffect(cstrText))
		{
			MessageBox(_T("CtrlEffect failed!"), m_tcsTitle);
			return false;
		}
		while (1)
		{
			nLenBefore=cstrText.GetLength();
			nStart=cstrText.Find(_T('{'), nPos);
			nEnd=cstrText.Find(_T('}'), nPos)+1;
			if (-1==nStart && 0==nEnd)//没有需要处理的控制符了
				break;
			if (nStart>=nEnd || -1==nStart)
			{
				MessageBox(_T("'{'&'}' don't match!"), m_tcsTitle);
				return false;
			}
			nLenAfter=cstrText.Delete(nStart, nEnd-nStart);
			nPos=nEnd+(nLenAfter-nLenBefore);
		}
	}
	return true;
}

bool CMainDlg::RenderDiff(CString& cstrText, BYTE *lpMark, BOOL bFullShowDiff)
{
	int nStart, nEnd, nLenAfter;

	//着色比较文本的不同之处
	if (NULL!=lpMark)
	{
		int nLen=cstrText.GetLength();
		if (bFullShowDiff)
		{
			for (int i=0; i<nLen-1; i++)
			{
				nLenAfter=cstrText.GetLength();
				if (90<=lpMark[i]-lpMark[i+1])//不同之处开始的地方
				{
					nStart=i+1;
					nLenAfter=cstrText.Insert(nStart+(nLenAfter-nLen), _T("<font color=")+ini_cstrClrDiffText+_T("><U>"));
				}
				if (-90>=lpMark[i]-lpMark[i+1])//不同之处结束的地方
				{
					nEnd=i+1;
					nLenAfter=cstrText.Insert(nEnd+(nLenAfter-nLen), _T("</U></font>"));
				}
			}
			if (0==lpMark[0] || 10==lpMark[0])
				cstrText=_T("<font color=")+ini_cstrClrDiffText+_T("><U>")+cstrText;
			if (0==lpMark[nLen-1] || 10==lpMark[nLen-1])
				cstrText=cstrText+_T("</U></font>");
		} 
		else
		{
			for (int i=0; i<nLen-1; i++)
			{
				nLenAfter=cstrText.GetLength();
				if (10==lpMark[i]-lpMark[i+1] || 100==lpMark[i]-lpMark[i+1])//不同之处开始的地方
				{
					nStart=i+1;
					nLenAfter=cstrText.Insert(nStart+(nLenAfter-nLen), _T("<font color=")+ini_cstrClrDiffText+_T("><U>"));
				}
				if (-10==lpMark[i]-lpMark[i+1] || -100==lpMark[i]-lpMark[i+1])//不同之处结束的地方
				{
					nEnd=i+1;
					nLenAfter=cstrText.Insert(nEnd+(nLenAfter-nLen), _T("</U></font>"));
				}
			}
			if (0==lpMark[0])
				cstrText=_T("<font color=")+ini_cstrClrDiffText+_T("><U>")+cstrText;
			if (0==lpMark[nLen-1])
				cstrText=cstrText+_T("</U></font>");
		}
	}

	return true;
}

bool CMainDlg::SetHTMLIDValue(LPCTSTR lpID, LPCTSTR lpValue)
{
	CComPtr<IDispatch> pDisp;
	hr = m_pWB2->get_Document(&pDisp);
	if (SUCCEEDED(hr))   
	{
		CComQIPtr<IHTMLDocument2> pDoc2=pDisp;
		CComPtr<IHTMLElementCollection> pAll;
		hr = pDoc2->get_all(&pAll);
		if (SUCCEEDED(hr))
		{
			LPCOLESTR pszElementID;
			pszElementID=CComBSTR(lpID);
			CComVariant varID = pszElementID;
			CComPtr<IDispatch> pDispItem;
			hr = pAll->item( varID, CComVariant(0), &pDispItem );
			if (SUCCEEDED(hr) && NULL!=pDispItem)//若NULL==pDispItem就是指未找到指定的ID
			{
				CComPtr<IHTMLElement> pElem;
				hr = pDispItem->QueryInterface(&pElem);
				if (SUCCEEDED(hr))
				{
					pElem->put_innerHTML(CComBSTR(lpValue));
					return true;
				}
			}
		}
	}
	return false;
}

bool CMainDlg::GetHTMLIDValue(LPCTSTR lpID, CString &cstrValue)
{
	CComPtr<IDispatch> pDisp;
	hr = m_pWB2->get_Document(&pDisp);
	if (SUCCEEDED(hr))   
	{
		CComQIPtr<IHTMLDocument2> pDoc2=pDisp;
		CComPtr<IHTMLElementCollection> pAll;
		hr = pDoc2->get_all(&pAll);
		if (SUCCEEDED(hr))
		{
			LPCOLESTR pszElementID;
			pszElementID=CComBSTR(lpID);
			CComVariant varID = pszElementID;
			CComPtr<IDispatch> pDispItem;
			hr = pAll->item( varID, CComVariant(0), &pDispItem );
			if (SUCCEEDED(hr) && NULL!=pDispItem)//若NULL==pDispItem就是指未找到指定的ID
			{
				CComPtr<IHTMLElement> pElem;
				hr = pDispItem->QueryInterface(&pElem);
				if (SUCCEEDED(hr))
				{
					BSTR b;
					pElem->get_innerHTML(&b);
					cstrValue=(b == NULL ? _T("") : b);
					SysFreeString(b);
					return true;
				}
			}
		}
	}
	return false;
}

bool CMainDlg::GetInputValue(LPCTSTR lpInput, CString& cstrValue)
{
	CComPtr<IDispatch> pDisp;
	hr = m_pWB2->get_Document(&pDisp);
	if (SUCCEEDED(hr))   
	{
		CComQIPtr<IHTMLDocument2> pDoc2=pDisp;
		CComPtr<IHTMLElementCollection> pAll;
		hr = pDoc2->get_all(&pAll);
		if (SUCCEEDED(hr))
		{
			CComVariant varID = _T("input");
			CComPtr<IDispatch> pDispItem;
			hr = pAll->tags(varID, &pDispItem);
			if (SUCCEEDED(hr))
			{
				CComQIPtr<IHTMLElementCollection> pInputAll=pDispItem;
				CComVariant varInputID = lpInput;
				CComPtr<IDispatch> pInputItem;
				hr = pInputAll->item(varInputID, CComVariant(0), &pInputItem);
				if (SUCCEEDED(hr) && NULL!=pInputItem)
				{
					CComPtr<IHTMLInputElement> pInputElem;
					hr = pInputItem->QueryInterface(&pInputElem);
					if (SUCCEEDED(hr))
					{
						BSTR b;
						pInputElem->get_value(&b);
						cstrValue=(b == NULL ? _T("") : b);
						SysFreeString(b);
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool CMainDlg::SetInputValue(LPCTSTR lpInput, LPCTSTR lpValue)
{
	CComPtr<IDispatch> pDisp;
	hr = m_pWB2->get_Document(&pDisp);
	if (SUCCEEDED(hr))   
	{
		CComQIPtr<IHTMLDocument2> pDoc2=pDisp;
		CComPtr<IHTMLElementCollection> pAll;
		hr = pDoc2->get_all(&pAll);
		if (SUCCEEDED(hr))
		{
			CComVariant varID = _T("input");
			CComPtr<IDispatch> pDispItem;
			hr = pAll->tags(varID, &pDispItem);
			if (SUCCEEDED(hr))
			{
				CComQIPtr<IHTMLElementCollection> pInputAll=pDispItem;
				CComVariant varInputID = lpInput;
				CComPtr<IDispatch> pInputItem;
				hr = pInputAll->item(varInputID, CComVariant(0), &pInputItem);
				if (SUCCEEDED(hr) && NULL!=pInputItem)
				{
					CComPtr<IHTMLInputElement> pInputElem;
					hr = pInputItem->QueryInterface(&pInputElem);
					if (SUCCEEDED(hr))
					{
						pInputElem->put_value(CComBSTR(lpValue));
						return true;
					}
				}
			}
		}
	}
	return false;
}

LRESULT CMainDlg::OnSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RECT rcClient, rcSplit,/* rcIE, rcRichEdit2,*/ rcTrackBar, rcListBox, rcFileList, rcButton;
	int nPlusX;

	if (SIZE_MINIMIZED!=wParam)
	{
		GetClientRect(&rcClient);

		/*m_CRichEdit2.GetWindowRect(&rcRichEdit2);
		ScreenToClient(&rcRichEdit2);
		rcRichEdit2.top=rcClient.bottom-m_rcMarginIE.bottom+m_rcMarginRE2.top;
		rcRichEdit2.bottom=rcClient.bottom-m_rcMarginRE2.bottom;
		rcRichEdit2.right=rcClient.right-m_rcMarginIE.right;
		m_CRichEdit2.SetWindowPos(NULL, &rcRichEdit2, SWP_NOZORDER);*/

		/*m_wndIE.GetWindowRect(&rcIE);
		ScreenToClient(&rcIE);
		rcIE.right=rcClient.right-m_rcMarginIE.right;
		rcIE.bottom=rcClient.bottom-m_rcMarginIE.bottom;
		m_wndIE.SetWindowPos(NULL, &rcIE, SWP_NOMOVE | SWP_NOZORDER);*/

		m_wndHorSplit.GetWindowRect(&rcSplit);
		ScreenToClient(&rcSplit);
		rcSplit.right=rcClient.right-m_rcMarginIE.right;
		rcSplit.bottom=rcClient.bottom-m_rcMarginRE2.bottom;
		m_wndHorSplit.SetWindowPos(NULL, &rcSplit, SWP_NOMOVE | SWP_NOZORDER);

		m_CInfoList.GetWindowRect(&rcListBox);
		ScreenToClient(&rcListBox);
		//计算X方向增量
		nPlusX=rcClient.right-m_rcMarginIE.right-rcListBox.right;
		rcListBox.left+=nPlusX/2;
		rcListBox.right=rcClient.right-m_rcMarginIE.right;
		m_CInfoList.SetWindowPos(NULL, &rcListBox, SWP_NOZORDER);

		m_CFileList.GetWindowRect(&rcFileList);
		ScreenToClient(&rcFileList);
		rcFileList.right+=nPlusX/2;
		m_CFileList.SetWindowPos(NULL, &rcFileList, SWP_NOMOVE | SWP_NOZORDER);

		::GetWindowRect(GetDlgItem(IDC_BUTTON1), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX/2;
		::SetWindowPos(GetDlgItem(IDC_BUTTON1), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(IDC_BUTTON1), NULL, TRUE);

		::GetWindowRect(GetDlgItem(IDC_BUTTON2), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX/2;
		::SetWindowPos(GetDlgItem(IDC_BUTTON2), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(IDC_BUTTON2), NULL, TRUE);

		::GetWindowRect(GetDlgItem(IDC_BUTTON3), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX/2;
		::SetWindowPos(GetDlgItem(IDC_BUTTON3), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(IDC_BUTTON3), NULL, TRUE);

		::GetWindowRect(GetDlgItem(IDC_BUTTON4), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX/2;
		::SetWindowPos(GetDlgItem(IDC_BUTTON4), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(IDC_BUTTON4), NULL, TRUE);

		::GetWindowRect(GetDlgItem(IDC_BUTTON5), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX/2;
		::SetWindowPos(GetDlgItem(IDC_BUTTON5), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(IDC_BUTTON5), NULL, TRUE);

		::GetWindowRect(GetDlgItem(IDC_BUTTON6), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX/2;
		::SetWindowPos(GetDlgItem(IDC_BUTTON6), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(IDC_BUTTON6), NULL, TRUE);

		::GetWindowRect(GetDlgItem(ID_APP_ABOUT), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX/2;
		::SetWindowPos(GetDlgItem(ID_APP_ABOUT), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(ID_APP_ABOUT), NULL, TRUE);

		::GetWindowRect(GetDlgItem(IDC_BUTTON10), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX/2;
		::SetWindowPos(GetDlgItem(IDC_BUTTON10), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(IDC_BUTTON10), NULL, TRUE);

		m_CTrackBar.GetWindowRect(&rcTrackBar);
		ScreenToClient(&rcTrackBar);
		rcTrackBar.right+=nPlusX;
		m_CTrackBar.SetWindowPos(NULL, &rcTrackBar, SWP_NOMOVE | SWP_NOZORDER);

		::GetWindowRect(GetDlgItem(IDC_BUTTON9), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX;
		::SetWindowPos(GetDlgItem(IDC_BUTTON9), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(IDC_BUTTON9), NULL, TRUE);

		::GetWindowRect(GetDlgItem(IDC_CHECK1), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX;
		::SetWindowPos(GetDlgItem(IDC_CHECK1), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(IDC_CHECK1), NULL, TRUE);

		::GetWindowRect(GetDlgItem(IDC_BUTTON7), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX;
		::SetWindowPos(GetDlgItem(IDC_BUTTON7), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(IDC_BUTTON7), NULL, TRUE);

		::GetWindowRect(GetDlgItem(IDC_BUTTON8), &rcButton);
		ScreenToClient(&rcButton);
		rcButton.left+=nPlusX;
		::SetWindowPos(GetDlgItem(IDC_BUTTON8), NULL, rcButton.left, rcButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		::InvalidateRect(GetDlgItem(IDC_BUTTON8), NULL, TRUE);

		::SendMessage(g_hWndMain, WM_USER+1, wParam, NULL);//修正主窗口边框且拖动量很小时，IE控件不会改变大小的bug
	}
	return TRUE;
}

bool CMainDlg::BuildHTML(LPCTSTR lpJP)
{
	CString cstrBody=_T(""), cstrBodyEnd=_T(""), cstrRowOri=_T(""), cstrRowBuf=_T(""), cstrRowAll=_T(""), cstrJS=_T("");
	errno_t err;
	bool bHr, bCN, bComp;
	int nNo=0, nPos, nCounter=0, nSn=0, nStartItem, nOverLen;
	CString cstrAdd=_T(""), cstrLen=_T(""), cstrText=_T(""), cstrTextCN=_T(""), cstrTextComp=_T("");
	CString cstrBuffer=_T(""), cstrBufferCN=_T(""), cstrBufferComp=_T("");
	FILE *fpJP, *fpTemp, *fpCN, *fpComp;
	BYTE *lpS=NULL, *lpT=NULL;
	CString cstrDelCol;
	int nDelStart, nDelLen;

	start = GetTickCount();

	nStartItem=m_CTrackBar.GetPos();
	nStartItem=ini_nItemPerPage*(nStartItem-1)+1;
	bHr=LoadHTMLResource(IDR_HTMLBODY, cstrBody);
	if(!bHr)
	{
		MessageBox(_T("LoadHTMLResource(IDR_HTMLBODY) failed!"), m_tcsTitle);
		return false;
	}
	/*if(_T("0")==ini_cstrWidthID)
	{
		cstrDelCol=_T("<td align=center width=WIDTH_ID NOWRAP>NO.</td>");
		nDelLen=cstrDelCol.GetLength();
		nDelStart=cstrBody.Find(cstrDelCol);
		cstrBody.Delete(nDelStart, nDelLen);
	}
	if(_T("0")==ini_cstrWidthJP)
	{
		cstrDelCol=_T("<td align=center width=WIDTH_JP class=\"JP\" NOWRAP>日文(jp-text)</td>");
		nDelLen=cstrDelCol.GetLength();
		nDelStart=cstrBody.Find(cstrDelCol);
		cstrBody.Delete(nDelStart, nDelLen);
	}*/
	if(_T("0")==ini_cstrWidthCN)
	{
		cstrDelCol=_T("<td align=center width=WIDTH_CN class=\"CN\" NOWRAP>中文(cn-text)</td>");
		nDelLen=cstrDelCol.GetLength();
		nDelStart=cstrBody.Find(cstrDelCol);
		cstrBody.Delete(nDelStart, nDelLen);
		bCN=false;
	}
	else
		bCN=true;
	if(_T("0")==ini_cstrWidthCOM)
	{
		cstrDelCol=_T("<td align=center width=WIDTH_COM class=\"CN\" NOWRAP>对照译文(cn-compare)</td>");
		nDelLen=cstrDelCol.GetLength();
		nDelStart=cstrBody.Find(cstrDelCol);
		cstrBody.Delete(nDelStart, nDelLen);
		bComp=false;
	}
	else
		bComp=true;
	cstrBody.Replace(_T("WIDTH_ID"), ini_cstrWidthID);
	cstrBody.Replace(_T("WIDTH_JP"), ini_cstrWidthJP);
	cstrBody.Replace(_T("WIDTH_CN"), ini_cstrWidthCN);
	cstrBody.Replace(_T("WIDTH_COM"), ini_cstrWidthCOM);
	cstrBody.Replace(_T("CLR_NOR_TEXT"), ini_cstrClrNorText);
	cstrBody.Replace(_T("CLR_SEL_TR"), ini_cstrClrSelTr);
	cstrBody.Replace(_T("CLR_TAB_BORDER"), ini_cstrClrTabBorder);
	cstrBody.Replace(_T("CLR_BODY_BG"), ini_cstrClrBodyBg);
	if(ini_bJPShowCNFont)
		cstrBody.Replace(_T("JP_FONT"), _T("宋体"));
	else
		cstrBody.Replace(_T("JP_FONT"), _T("MS Gothic"));

	bHr=LoadHTMLResource(IDR_HTMLROW, cstrRowOri);
	if(!bHr)
	{
		MessageBox(_T("LoadHTMLResource(IDR_HTMLROW) failed!"), m_tcsTitle);
		return false;
	}
	cstrRowOri.TrimLeft(0xfeff);
	/*if(_T("0")==ini_cstrWidthID)
	{
		cstrDelCol=_T("<td width=WIDTH_ID ID=\"TD_NO_TEMPLATE_ID\" class=\"NO\" bgcolor=\"TEMPLATE_MARK_COLOR\">TEMPLATE_ADDR</td>");
		nDelLen=cstrDelCol.GetLength();
		nDelStart=cstrRowOri.Find(cstrDelCol);
		cstrRowOri.Delete(nDelStart, nDelLen);
	}
	if(_T("0")==ini_cstrWidthJP)
	{
		cstrDelCol=_T("<td width=WIDTH_JP ID=\"TD_J_TEMPLATE_ID\" class=\"JP\">TEMPLATE_JP</td>");
		nDelLen=cstrDelCol.GetLength();
		nDelStart=cstrRowOri.Find(cstrDelCol);
		cstrRowOri.Delete(nDelStart, nDelLen);
	}*/
	if(!bCN)
	{
		cstrDelCol=_T("<td width=WIDTH_CN ID=\"TD_PREVIEW_TEMPLATE_ID\" class=\"CN\">TEMPLATE_CN</td>");
		nDelLen=cstrDelCol.GetLength();
		nDelStart=cstrRowOri.Find(cstrDelCol);
		cstrRowOri.Delete(nDelStart, nDelLen);
	}
	if(!bComp)
	{
		cstrDelCol=_T("<td width=WIDTH_COM ID=\"TD_COMPARE_TEMPLATE_ID\" class=\"CN\">TEMPLATE_COMPARE</td>");
		nDelLen=cstrDelCol.GetLength();
		nDelStart=cstrRowOri.Find(cstrDelCol);
		cstrRowOri.Delete(nDelStart, nDelLen);
	}
	cstrRowOri.Replace(_T("CLR_NOR_TR"), ini_cstrClrNorTr);

	bHr=LoadHTMLResource(IDR_HTMLBODYEND, cstrBodyEnd);
	if(!bHr)
	{
		MessageBox(_T("LoadHTMLResource(IDR_HTMLBODYEND) failed!"), m_tcsTitle);
		return false;
	}

	bHr=LoadHTMLResource(IDR_HTMLJS, cstrJS);
	if(!bHr)
	{
		MessageBox(_T("LoadHTMLResource(IDR_HTMLJS) failed!"), m_tcsTitle);
		return false;
	}
	cstrJS.TrimLeft(0xfeff);
	cstrJS.Replace(_T("CLR_NOR_TR"), ini_cstrClrNorTr);
	cstrJS.Replace(_T("CLR_SEL_TR"), ini_cstrClrSelTr);

	if((err=_tfopen_s(&fpJP, lpJP, _T("rb")))!=NULL)
	{
		MessageBox(_T("Open jp-text file failed!"), m_tcsTitle);
		return false;
	}
	if(0xfeff!=fgetwc(fpJP))
	{
		MessageBox(_T("The jp-text file isn't unicode type."), m_tcsTitle);
		fclose(fpJP);
		return false;
	}

	if (bCN)
	{
		cstrBuffer=lpJP;
		cstrBuffer.Replace(_T("\\jp-text"), _T("\\cn-text"));
		if(err=_tfopen_s(&fpCN, cstrBuffer, _T("rb"))!=NULL)
			bCN=false;
		else
			bCN=true;
		if (bCN)
		{
			if(0xfeff!=fgetwc(fpCN))
				MessageBox(_T("The cn-text file isn't unicode type."), m_tcsTitle);
		}
	} 

	if (bComp)
	{
		cstrBuffer.Empty();
		cstrBuffer=lpJP;
		cstrBuffer.Replace(_T("\\jp-text"), _T("\\cn-compare"));
		if((err=_tfopen_s(&fpComp, cstrBuffer, _T("rb")))!=NULL)
			bComp=false;
		else
			bComp=true;
		if (bComp)
		{
			if(0xfeff!=fgetwc(fpComp))
				MessageBox(_T("The cn-compare file isn't unicode type."), m_tcsTitle);
		}
	}

	if((err=_tfopen_s(&fpTemp, (LPCTSTR)m_cstrTemp, _T("wb")))!=NULL)
	{
		MessageBox(_T("Create temp file failed!"), m_tcsTitle);
		return false;
	}
	fwrite(cstrBody, cstrBody.GetLength()*sizeof(TCHAR), 1, fpTemp);

	cstrBuffer.Empty();
	while (NULL!=fgetws(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpJP))
	{
		cstrBuffer.ReleaseBuffer();
		//处理日文文本
		if (0==cstrBuffer.Compare(_T("\r\n")))//若是空行则跳过
		{
			cstrBuffer.Empty();
			continue;
		}
		//处理中文文本
		if (bCN)
		{
			if (NULL==fgetws(cstrBufferCN.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpCN))
			{
				cstrBufferCN.ReleaseBuffer();
				MessageBox(_T("The cn-text file doesn't match jp-text file!"), m_tcsTitle);
				fclose(fpCN);
				if (bComp)
					fclose(fpComp);
				fclose(fpTemp);
				fclose(fpJP);
				return false;
			}
			cstrBufferCN.ReleaseBuffer();
			if (0==cstrBufferCN.Compare(_T("\r\n")))//若是空行则继续读取下一行
			{
				cstrBufferCN.Empty();
				if (NULL==fgetws(cstrBufferCN.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpCN))
				{
					cstrBufferCN.ReleaseBuffer();
					MessageBox(_T("The cn-text file doesn't match jp-text file!"), m_tcsTitle);
					fclose(fpCN);
					if (bComp)
						fclose(fpComp);
					fclose(fpTemp);
					fclose(fpJP);
					return false;
				}
				cstrBufferCN.ReleaseBuffer();
			}
		}
		//处理比较文本
		if (bComp)
		{
			if (NULL==fgetws(cstrBufferComp.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpComp))
			{
				cstrBufferComp.ReleaseBuffer();
				MessageBox(_T("The cn-compare file doesn't match jp-text file!"), m_tcsTitle);
				if (bCN)
					fclose(fpCN);
				if (bComp)
					fclose(fpComp);
				fclose(fpTemp);
				fclose(fpJP);
				return false;
			}
			cstrBufferComp.ReleaseBuffer();
			if (0==cstrBufferComp.Compare(_T("\r\n")))//若是空行则继续读取下一行
			{
				cstrBufferComp.Empty();
				if (NULL==fgetws(cstrBufferComp.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpComp))
				{
					cstrBufferComp.ReleaseBuffer();
					MessageBox(_T("The cn-compare file doesn't match jp-text file!"), m_tcsTitle);
					if (bCN)
						fclose(fpCN);
					if (bComp)
						fclose(fpComp);
					fclose(fpTemp);
					fclose(fpJP);
					return false;
				}
				cstrBufferComp.ReleaseBuffer();
			}
		}

		nNo++;
		if (nNo<nStartItem)//若还未到当前页内容时则跳过
		{
			cstrBuffer.Empty();
			continue;
		}
		cstrBuffer.TrimRight(_T("\r\n"));
		nPos=cstrBuffer.Find(_T(','));
		if (-1==nPos)
		{
			cstrBuffer.Format(_T("%05d"), nNo);
			MessageBox(_T("The format of the jp-text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
			if (bCN)
				fclose(fpCN);
			if (bComp)
				fclose(fpComp);
			fclose(fpTemp);
			fclose(fpJP);
			return false;
		}
		cstrAdd=cstrBuffer.Left(nPos);
		cstrBuffer=cstrBuffer.Mid(nPos+1);
		nPos=cstrBuffer.Find(L',');
		if (-1==nPos)
		{
			cstrBuffer.Format(_T("%05d"), nNo);
			MessageBox(_T("The format of the jp-text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
			if (bCN)
				fclose(fpCN);
			if (bComp)
				fclose(fpComp);
			fclose(fpTemp);
			fclose(fpJP);
			return false;
		}
		cstrLen=cstrBuffer.Left(nPos);
		cstrText=cstrBuffer.Mid(nPos+1);

		//截取中文文本
		if (bCN)
		{
			if (nNo>=nStartItem)//若是当前页内容时才截取文本
			{
				cstrBufferCN.TrimRight(_T("\r\n"));
				nPos=cstrBufferCN.Find(_T(','));
				if (-1==nPos)
				{
					cstrBufferCN.Format(_T("%05d"), nNo);
					MessageBox(_T("The format of the cn-text is wrong!\nAt No.")+cstrBufferCN, m_tcsTitle);
					if (bCN)
						fclose(fpCN);
					if (bComp)
						fclose(fpComp);
					fclose(fpTemp);
					fclose(fpJP);
					return false;
				}
				cstrBufferCN=cstrBufferCN.Mid(nPos+1);
				nPos=cstrBufferCN.Find(L',');
				if (-1==nPos)
				{
					cstrBufferCN.Format(_T("%05d"), nNo);
					MessageBox(_T("The format of the cn-text is wrong!\nAt No.")+cstrBufferCN, m_tcsTitle);
					if (bCN)
						fclose(fpCN);
					if (bComp)
						fclose(fpComp);
					fclose(fpTemp);
					fclose(fpJP);
					return false;
				}
				cstrTextCN=cstrBufferCN.Mid(nPos+1);
			}
		}
		else
			cstrTextCN=_T("");

		//截取比较文本
		if (bComp)
		{
			if (nNo>=nStartItem)//若是当前页内容时才截取文本
			{
				cstrBufferComp.TrimRight(_T("\r\n"));
				nPos=cstrBufferComp.Find(_T(','));
				if (-1==nPos)
				{
					cstrBufferComp.Format(_T("%05d"), nNo);
					MessageBox(_T("The format of the cn-compare is wrong!\nAt No.")+cstrBufferComp, m_tcsTitle);
					if (bCN)
						fclose(fpCN);
					if (bComp)
						fclose(fpComp);
					fclose(fpTemp);
					fclose(fpJP);
					return false;
				}
				cstrBufferComp=cstrBufferComp.Mid(nPos+1);
				nPos=cstrBufferComp.Find(L',');
				if (-1==nPos)
				{
					cstrBufferComp.Format(_T("%05d"), nNo);
					MessageBox(_T("The format of the cn-compare is wrong!\nAt No.")+cstrBufferComp, m_tcsTitle);
					if (bCN)
						fclose(fpCN);
					if (bComp)
						fclose(fpComp);
					fclose(fpTemp);
					fclose(fpJP);
					return false;
				}
				cstrTextComp=cstrBufferComp.Mid(nPos+1);
			}
		}
		else
			cstrTextComp=_T("");

		nOverLen=OverLen(_tstoi(cstrLen), cstrTextCN);

		RenderSpec(cstrText);
		RenderSpec(cstrTextCN);
		RenderSpec(cstrTextComp);
		//对文本进行着色处理，必须先着色不同文本再着色控制符！
		if ((!cstrTextCN.IsEmpty() && !cstrTextComp.IsEmpty()) || ini_bCNCOMNULL)
		{
			lpS=(BYTE*)malloc(cstrTextCN.GetLength()*sizeof(BYTE));
			memset(lpS, 0, cstrTextCN.GetLength()*sizeof(BYTE));
			lpT=(BYTE*)malloc(cstrTextComp.GetLength()*sizeof(BYTE));
			memset(lpT, 0, cstrTextComp.GetLength()*sizeof(BYTE));
			int ld=LD(cstrTextCN, cstrTextComp, lpS, lpT);
			if (0==ld)
				cstrTextComp=_T("+-+-+-+-+-+");
			else
			{
				RenderDiff(cstrTextCN, lpS, ini_bCNShowDiff);
				RenderDiff(cstrTextComp, lpT, TRUE);
			}
		}
		if(!RenderCtrl(cstrText))
		{
			MessageBox(_T("RenderCtrl(cstrText) failed!"), m_tcsTitle);
			if (bCN)
				fclose(fpCN);
			if (bComp)
				fclose(fpComp);
			fclose(fpTemp);
			fclose(fpJP);
			return false;
		}
		if(!RenderCtrl(cstrTextCN))
		{
			MessageBox(_T("RenderCtrl(cstrTextCN) failed!"), m_tcsTitle);
			if (bCN)
				fclose(fpCN);
			if (bComp)
				fclose(fpComp);
			fclose(fpTemp);
			fclose(fpJP);
			return false;
		}
		if(!RenderCtrl(cstrTextComp))
		{
			MessageBox(_T("RenderCtrl(cstrTextComp) failed!"), m_tcsTitle);
			if (bCN)
				fclose(fpCN);
			if (bComp)
				fclose(fpComp);
			fclose(fpTemp);
			fclose(fpJP);
			return false;
		}
		if (NULL!=lpS)
		{
			free(lpS);
			lpS=NULL;
		}
		if (NULL!=lpT)
		{
			free(lpT);
			lpT=NULL;
		}

		cstrBuffer.Empty();
		cstrBufferCN.Empty();
		cstrBufferComp.Empty();

		//着色文本越界的句子
		if (nOverLen>0)
		{
			if (0x00==m_lpByteRow[nNo])
			{
				m_nOverLenCount++;//必须在加入字符串到m_CInfoList之前执行
				m_lpByteRow[nNo]=0x01;
			}
			cstrTextCN=_T("<font style=\"background-color:")+ini_cstrClrOverLenBg+_T("\">")+cstrTextCN+_T("</font>");
			cstrBuffer.Format(_T("第%d行超长%d字节！"), nNo, nOverLen);
			int nIndex;
			nIndex=m_CInfoList.AddString(cstrBuffer);
			m_CInfoList.SetItemData(nIndex, (DWORD)nNo);
			m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);
			cstrBuffer.Empty();
		}

		CString cstrNo=_T(""), cstrID=_T("");
		cstrNo.Format(_T("%05d"), nNo);
		cstrID.Format(_T("%d"), nNo);
		cstrRowBuf=cstrRowOri;
		cstrRowBuf.Replace(_T("WIDTH_ID"), ini_cstrWidthID);
		cstrRowBuf.Replace(_T("WIDTH_JP"), ini_cstrWidthJP);
		cstrRowBuf.Replace(_T("WIDTH_CN"), ini_cstrWidthCN);
		cstrRowBuf.Replace(_T("WIDTH_COM"), ini_cstrWidthCOM);
		if(nOverLen>0)
			cstrRowBuf.Replace(_T("TEMPLATE_ADDR"), _T("<font style=\"background-color:")+ini_cstrClrOverLenBg+_T("\">[")+cstrNo+_T("]")+cstrAdd+_T(",")+cstrLen+_T("</font>"));
		else
			cstrRowBuf.Replace(_T("TEMPLATE_ADDR"), _T("[")+cstrNo+_T("]")+cstrAdd+_T(",")+cstrLen);
		cstrRowBuf.Replace(_T("TEMPLATE_ID"), cstrID);
		cstrRowBuf.Replace(_T("TEMPLATE_JP"), cstrText);
		cstrRowBuf.Replace(_T("TEMPLATE_CN"), cstrTextCN);
		cstrRowBuf.Replace(_T("TEMPLATE_COMPARE"), cstrTextComp);
		cstrRowBuf.Replace(_T("TEMPLATE_MARK_COLOR"), ini_cstrClrSelTr);
		cstrRowBuf.Replace(_T("DISPLAY_FLAG"), _T(""));
		nSn++;//一条记录处理完毕，计数加一

		nCounter++;
		if (ini_nWriteFilePerTime==nCounter)
		{
			cstrRowAll.Insert(0, _T("<table border=\"1\" cellspacing=\"0\" cellpadding=\"3\" bordercolor=\"")+ini_cstrClrTabBorder+_T("\" align=\"center\" style=\"table-layout:fixed;border-collapse:collapse\" frame=below>\r\n"));
			cstrRowAll.Insert(cstrRowAll.GetLength(), _T("\r\n</table>\r\n"));
			fwrite(cstrRowAll, cstrRowAll.GetLength()*sizeof(TCHAR), 1, fpTemp);
			nCounter=0;
			cstrRowAll.Empty();
		}
		cstrRowAll+=cstrRowBuf;
		if(ini_nItemPerPage==nSn)
		{
			nSn=0;
			break;
		}
	}
	cstrRowAll.Insert(0, _T("<table border=\"1\" cellspacing=\"0\" cellpadding=\"3\" bordercolor=\"")+ini_cstrClrTabBorder+_T("\" align=\"center\" style=\"table-layout:fixed;border-collapse:collapse\" frame=below>\r\n"));
	cstrRowAll.Insert(cstrRowAll.GetLength(), _T("\r\n</table>\r\n"));
	cstrRowAll+=cstrBodyEnd;
	cstrRowAll+=cstrJS;
	fwrite(cstrRowAll, cstrRowAll.GetLength()*sizeof(TCHAR), 1, fpTemp);
	cstrRowAll.Empty();
	if (bCN)
		fclose(fpCN);
	if (bComp)
		fclose(fpComp);
	fclose(fpTemp);
	fclose(fpJP);
	return true;
}

LRESULT CMainDlg::OnBnClickedButton2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CFindFile finder;
	CString cstrPattern=_T("\\jp-text\\*.txt"), cstrBuffer, cstrFilePath;
	int nItem=0, n;

	::EnableWindow(GetDlgItem(ID_APP_ABOUT), FALSE);
	::EnableWindow(GetDlgItem(IDC_CHECK1), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON1), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON3), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON4), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON5), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON6), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON7), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON8), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON10), FALSE);
	cstrPattern=m_tcsOriPath+cstrPattern;
	m_CFileList.DeleteAllItems();
	if ( finder.FindFile ( cstrPattern ) )
	{
		do
		{
			if(!finder.IsDots())
			{
				m_CFileList.AddItem(nItem, 0, finder.GetFileName());
				cstrFilePath=finder.GetFilePath();
				cstrFilePath.Replace(_T("\\jp-text"), _T("\\cn-text"));
				n=CountItem(cstrFilePath);
				//cstrBuffer.Format(_T("%d"), n<0?0:n);
				cstrBuffer.Format(_T("%d"), n);
				m_CFileList.AddItem(nItem, 1, cstrBuffer);
				cstrBuffer.Empty();
				n=CountItem(finder.GetFilePath());
				//cstrBuffer.Format(_T("%d"), n<0?0:n);
				cstrBuffer.Format(_T("%d"), n);
				m_CFileList.AddItem(nItem, 2, cstrBuffer);
				nItem++;
			}
		}
		while ( finder.FindNextFile() );
		finder.Close();
		::EnableWindow(GetDlgItem(ID_APP_ABOUT), TRUE);
		::EnableWindow(GetDlgItem(IDC_CHECK1), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON1), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON3), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON4), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON5), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON6), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON7), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON8), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON10), TRUE);
		return 0;
	}
	finder.Close();
	MessageBox(_T("No jp-text file."), m_tcsTitle);
	::EnableWindow(GetDlgItem(ID_APP_ABOUT), TRUE);
	::EnableWindow(GetDlgItem(IDC_CHECK1), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON1), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON3), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON4), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON5), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON6), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON7), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON8), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON10), TRUE);
	return 0;
}

unsigned int __stdcall ThreadProc(LPVOID lpParameter)
{
	LPIEDATA lpIEdata=(LPIEDATA)lpParameter;
	CComVariant v;  // empty variant
	lpIEdata->m_pWB2->Navigate ( CComBSTR(lpIEdata->lpTemp), &v, &v, &v, &v );
	return 0;
}

LRESULT CMainDlg::OnNMDblclkList2(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	TCHAR tszFileName[MAX_PATH], tszItemAll[MAX_PATH];
	LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) pNMHDR;
	CString cstrBuffer;

	if(-1==(lpnmitem->iItem))
		return 0;

	if(m_bTextIsDirty)
	{
		SaveText(m_nLastRowNo);
		m_bTextIsDirty=false;
	}
	m_CInfoList.ResetContent();
	m_nOverLenCount=0;

	start = ::GetTickCount();

	m_CFileList.GetItemText(lpnmitem->iItem, 0, tszFileName, MAX_PATH);
	m_CFileList.GetItemText(lpnmitem->iItem, 2, tszItemAll, MAX_PATH);

	if (!m_bShouldScrol)
	{
		if (m_cstrLastFileName==tszFileName)
			m_bShouldScrol=true;
	}

	cstrBuffer.Empty();
	cstrBuffer=m_tcsTitle;
	cstrBuffer=cstrBuffer+_T(" - ")+tszFileName;
	SetWindowText(cstrBuffer);
	int nItemAll=_tstoi(tszItemAll);
	PreBuildHTML(tszFileName, nItemAll);

	//从内存中加载网页
	/*errno_t err;
	HGLOBAL hHTMLText;
	IStream* pStream = NULL;

	if(err=_wfopen_s(&fpTemp, (LPCTSTR)cstrTemp, _T("rb"))!=NULL)
	{
		MessageBox(_T("Open Temp file failed!"), wcsTitle);
		return 0;
	}
	fseek(fpTemp, 0L, 2);
	int nSize=ftell(fpTemp);
	rewind(fpTemp);
	fread(cstrBuffer.GetBuffer(nSize/sizeof(TCHAR)), nSize, 1, fpTemp);
	cstrBuffer.ReleaseBuffer();
	fclose(fpTemp);

	hHTMLText = GlobalAlloc( GPTR, (cstrBuffer.GetLength()+1)*sizeof(TCHAR));
	if (hHTMLText)
	{
		//StringCchCopy((TCHAR*)hHTMLText, cstrBuffer.GetLength()+1, cstrBuffer);
		memcpy(hHTMLText, cstrBuffer, (cstrBuffer.GetLength()+1)*sizeof(TCHAR));
		hr = CreateStreamOnHGlobal( hHTMLText, TRUE, &pStream );
		if ( SUCCEEDED(hr) )
		{
			LoadWebBrowserFromStream( m_pWB2, pStream );
			pStream->Release();
		}
		GlobalFree( hHTMLText );
	}*/
	return 0;
}

bool CMainDlg::PreBuildHTML(LPCTSTR lpctFileName, int nItemAll)
{
	CString cstrJP, cstrPageAll, cstrCurrPage;
	int nRest;

	m_CInfoList.AddString(_T("==============="));
	m_CInfoList.AddString(_T("开始生成页面..."));
	m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);
	cstrJP=m_cstrJPPath+lpctFileName;
	m_cstrCurrJP=cstrJP;

	::EnableWindow(GetDlgItem(ID_APP_ABOUT), FALSE);
	::EnableWindow(GetDlgItem(IDC_CHECK1), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON1), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON2), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON3), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON4), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON5), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON6), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON7), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON8), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON10), FALSE);

	//初始化行信息数组
	if (NULL!=m_lpByteRow)
	{
		free(m_lpByteRow);
		m_lpByteRow=NULL;
	}
	m_lpByteRow=(BYTE*)malloc((nItemAll+1)*sizeof(BYTE));
	memset(m_lpByteRow, 0, (nItemAll+1)*sizeof(BYTE));

	nRest=nItemAll%ini_nItemPerPage!=0?1:0;
	m_CTrackBar.SetRange(1, nItemAll/ini_nItemPerPage+nRest);
	cstrPageAll.Format(_T("%02d"), nItemAll/ini_nItemPerPage+nRest);
	cstrCurrPage.Format(_T("%02d"), m_CTrackBar.GetPos());
	SetDlgItemText(IDC_STATIC_PAGE, _T("第")+cstrCurrPage+_T("页,共")+cstrPageAll+_T("页"));
	if (ini_bOnePageSlider || m_CTrackBar.GetRangeMax()>1)
		m_CTrackBar.EnableWindow(TRUE);
	else
		m_CTrackBar.EnableWindow(FALSE);

	if(!BuildHTML(cstrJP))
	{
		MessageBox(_T("BuildHTML() failed!"), m_tcsTitle);
		return false;
	}
	m_cstrLastFileName=lpctFileName;
	end = ::GetTickCount();
	CString cstrBuffer;
	cstrBuffer.Format(_T("%d"), end-start);
	m_CInfoList.AddString(_T("生成页面耗时:")+cstrBuffer+_T("ms"));
	m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);
	start = ::GetTickCount();

	m_CInfoList.AddString(_T("==============="));
	m_CInfoList.AddString(_T("开始显示页面..."));
	m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);

	BSTR b;
	m_pWB2->get_LocationName(&b);
	CString cstrName(b);
	SysFreeString(b);
	if (cstrName==_T("temp.html"))
	{
		m_bHtmlChanged=false;
		SetHTMLScrollTopLeft(0L, 0L);
		m_pWB2->Refresh();
		Sleep(0);
	}
	else
	{
		g_IEdata.m_pWB2=m_pWB2;
		g_IEdata.lpTemp=(LPCTSTR)m_cstrTemp;
		m_bHtmlChanged=true;
		HANDLE hThread=(HANDLE)_beginthreadex(NULL, 0, ThreadProc, &g_IEdata, 0, NULL);
		Sleep(0);
		if (NULL!=hThread)
			CloseHandle(hThread);
		else
			MessageBox(_T("CreateThread failed!"), m_tcsTitle);
	}
	return true;
}

int CMainDlg::CountItem(LPCTSTR tszFile)
{
	int nItem=0, nPos;
	errno_t err;
	FILE *fpIn;
	CString cstrBuffer;

	if((err=_tfopen_s(&fpIn, tszFile, _T("rb")))!=NULL)
		return -1;//Can't open file
	if(0xfeff!=fgetwc(fpIn))
	{
		fclose(fpIn);
		return -2;//Not unicode file
	}

	while (NULL!=fgetws(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpIn))
	{
		cstrBuffer.ReleaseBuffer();
		if (0==cstrBuffer.Compare(_T("\r\n")))//若是空行则跳过
		{
			cstrBuffer.Empty();
			continue;
		}
		cstrBuffer.TrimRight(_T("\r\n"));
		nPos=cstrBuffer.Find(_T(','));
		if (-1==nPos)
		{
			fclose(fpIn);
			return -3;//Format is wrong
		}
		cstrBuffer=cstrBuffer.Mid(nPos+1);
		nPos=cstrBuffer.Find(L',');
		if (-1==nPos)
		{
			fclose(fpIn);
			return -3;//Format is wrong
		}
		if(_T("")==cstrBuffer.Mid(nPos+1))
		{
			cstrBuffer.Empty();
			continue;
		}
		nItem++;
	}

	fclose(fpIn);
	return nItem;
}
LRESULT CMainDlg::OnNMReleasedcaptureSlider(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	m_bShouldScrol=true;
	m_CFileList.SetFocus();
	//模拟用户按下了空格键
	::keybd_event(VK_SPACE, 0, 0, 0);
	::keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
	return 0;
}
LRESULT CMainDlg::OnLvnKeydownList2(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here

	if(-1==m_CFileList.GetSelectionMark())
		return 0;

	//VK_SPACE英文空格，VK_PROCESSKEY中文空格
	if (VK_SPACE==pLVKeyDow->wVKey || VK_PROCESSKEY==pLVKeyDow->wVKey)
	{
		if(m_bTextIsDirty)
		{
			SaveText(m_nLastRowNo);
			m_bTextIsDirty=false;
		}
		//m_CInfoList.ResetContent();
		m_nOverLenCount=0;

		TCHAR tszFileName[MAX_PATH], tszItemAll[MAX_PATH];
		m_CFileList.GetItemText(m_CFileList.GetSelectionMark(), 0, tszFileName, MAX_PATH);
		m_CFileList.GetItemText(m_CFileList.GetSelectionMark(), 2, tszItemAll, MAX_PATH);
		CString cstrBuffer=m_tcsTitle;
		cstrBuffer=cstrBuffer+_T(" - ")+tszFileName;
		SetWindowText(cstrBuffer);
		int nItemAll=_tstoi(tszItemAll);
		PreBuildHTML(tszFileName, nItemAll);
	}
	return 0;
}

LRESULT CMainDlg::OnBnClickedButton1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	m_CInfoList.ResetContent();
	return 0;
}
void __stdcall CMainDlg::BeforeNavigate2Explorer1(LPDISPATCH pDisp, VARIANT* URL, VARIANT* Flags, VARIANT* TargetFrameName, VARIANT* PostData, VARIANT* Headers, BOOL* Cancel)
{
	// TODO: Add your message handler code here
	CString strUrl=	URL->bstrVal;
	if(strUrl.Left(4) == _T("app:"))
	{
		SaveText(m_nLastRowNo);
		UpdatePreview(m_nLastRowNo);
		ShowFormatText(_tstoi(strUrl.Mid(4)));
		m_nLastRowNo=_tstoi(strUrl.Mid(4));
		m_bTextIsDirty=false;
		*Cancel=TRUE;
	}
	else
		start = ::GetTickCount();
}

bool CMainDlg::ShowFormatText(int nRowNo)
{
	errno_t err;
	FILE *fpJP, *fpCN;
	CString cstrJP, cstrBuffer, cstrBufferCN, cstrText, cstrTextCN;
	bool bCN;
	int nNo=0, nPos;

	cstrJP=m_cstrCurrJP;
	if((err=_tfopen_s(&fpJP, cstrJP, _T("rb")))!=NULL)
	{
		MessageBox(_T("Open jp-text file failed!"), m_tcsTitle);
		return false;
	}
	if(0xfeff!=fgetwc(fpJP))
	{
		MessageBox(_T("The jp-text file isn't unicode type."), m_tcsTitle);
		fclose(fpJP);
		return false;
	}
	cstrJP.Replace(_T("\\jp-text"), _T("\\cn-text"));
	if((err=_tfopen_s(&fpCN, cstrJP, _T("rb")))!=NULL)
		bCN=false;
	else
		bCN=true;
	if (bCN)
	{
		if(0xfeff!=fgetwc(fpCN))
		{
			MessageBox(_T("The cn-text file isn't unicode type."), m_tcsTitle);
			fclose(fpCN);
			bCN=false;
		}
	}
	while (NULL!=fgetws(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpJP))
	{
		cstrBuffer.ReleaseBuffer();
		if (0==cstrBuffer.Compare(_T("\r\n")))//若是空行则跳过
		{
			cstrBuffer.Empty();
			continue;
		}
		if (bCN)
		{
			if (NULL==fgetws(cstrBufferCN.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpCN))
			{
				cstrBufferCN.ReleaseBuffer();
				MessageBox(_T("The cn-text file doesn't match jp-text file!"), m_tcsTitle);
				if (bCN)
					fclose(fpCN);
				fclose(fpJP);
				return false;
			}
			cstrBufferCN.ReleaseBuffer();
			if (0==cstrBufferCN.Compare(_T("\r\n")))//若是空行则继续读取下一行
			{
				cstrBufferCN.Empty();
				if (NULL==fgetws(cstrBufferCN.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpCN))
				{
					cstrBufferCN.ReleaseBuffer();
					MessageBox(_T("The cn-text file doesn't match jp-text file!"), m_tcsTitle);
					if (bCN)
						fclose(fpCN);
					fclose(fpJP);
					return false;
				}
				cstrBufferCN.ReleaseBuffer();
			}
		}
		nNo++;
		if (nNo!=nRowNo)//若还未到选择行时则跳过
		{
			cstrBuffer.Empty();
			continue;
		}
		cstrBuffer.TrimRight(_T("\r\n"));
		nPos=cstrBuffer.Find(_T(','));
		if (-1==nPos)
		{
			cstrBuffer.Format(_T("%05d"), nNo);
			MessageBox(_T("The format of the jp-text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
			if (bCN)
				fclose(fpCN);
			fclose(fpJP);
			return false;
		}
		cstrBuffer=cstrBuffer.Mid(nPos+1);
		nPos=cstrBuffer.Find(L',');
		if (-1==nPos)
		{
			cstrBuffer.Format(_T("%05d"), nNo);
			MessageBox(_T("The format of the jp-text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
			if (bCN)
				fclose(fpCN);
			fclose(fpJP);
			return false;
		}
		cstrText=cstrBuffer.Mid(nPos+1);

		if (bCN)
		{
			if (nNo==nRowNo)//若是选择行时才截取文本
			{
				cstrBufferCN.TrimRight(_T("\r\n"));
				nPos=cstrBufferCN.Find(_T(','));
				if (-1==nPos)
				{
					cstrBufferCN.Format(_T("%05d"), nNo);
					MessageBox(_T("The format of the cn-text is wrong!\nAt No.")+cstrBufferCN, m_tcsTitle);
					if (bCN)
						fclose(fpCN);
					fclose(fpJP);
					return false;
				}
				cstrBufferCN=cstrBufferCN.Mid(nPos+1);
				nPos=cstrBufferCN.Find(L',');
				if (-1==nPos)
				{
					cstrBufferCN.Format(_T("%05d"), nNo);
					MessageBox(_T("The format of the cn-text is wrong!\nAt No.")+cstrBufferCN, m_tcsTitle);
					if (bCN)
						fclose(fpCN);
					fclose(fpJP);
					return false;
				}
				cstrTextCN=cstrBufferCN.Mid(nPos+1);
			}
		}
		else
			cstrTextCN=_T("");

		if (cstrTextCN.Compare(_T("")))
		{
			m_CRichEdit2.SetWindowText(cstrTextCN);
			//SetDlgItemText(IDC_RICHEDIT21, cstrTextCN);
			FormatText(cstrTextCN);
		}
		else
		{
			m_CRichEdit2.SetWindowText(cstrText);
			//SetDlgItemText(IDC_RICHEDIT21, cstrText);
			FormatText(cstrText);
		}
		m_CRichEdit2.EmptyUndoBuffer();
		break;
	}
	fclose(fpJP);
	if(bCN)
		fclose(fpCN);
	return true;
}

int CMainDlg::GetTextFromFile(FILE *fpIn, int nRowNo, CString &cstrText)
{
	CString cstrBuffer, cstrLen;
	int nNo=0, nPos;

	if(0xfeff!=fgetwc(fpIn))
	{
		MessageBox(_T("The text file isn't unicode type."), m_tcsTitle);
		return -1;
	}
	while (NULL!=fgetws(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpIn))
	{
		cstrBuffer.ReleaseBuffer();
		if (0==cstrBuffer.Compare(_T("\r\n")))//若是空行则跳过
		{
			cstrBuffer.Empty();
			continue;
		}
		nNo++;
		if (nNo!=nRowNo)//若还未到选择行时则跳过
		{
			cstrBuffer.Empty();
			continue;
		}
		cstrBuffer.TrimRight(_T("\r\n"));
		nPos=cstrBuffer.Find(_T(','));
		if (-1==nPos)
		{
			cstrBuffer.Format(_T("%05d"), nNo);
			MessageBox(_T("The format of the text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
			return -1;
		}
		cstrBuffer=cstrBuffer.Mid(nPos+1);
		nPos=cstrBuffer.Find(L',');
		if (-1==nPos)
		{
			cstrBuffer.Format(_T("%05d"), nNo);
			MessageBox(_T("The format of the text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
			return -1;
		}
		cstrLen=cstrBuffer.Left(nPos);
		cstrText=cstrBuffer.Mid(nPos+1);
	}
	rewind(fpIn);
	return _tstoi(cstrLen);
}

bool CMainDlg::FormatText(LPCTSTR lpText)
{
	int nStartChar, nEndChar, nPos=0;
	CString cstrBuffer=lpText;
	CHARFORMAT cf;

	memset(&cf, 0, sizeof(CHARFORMAT));
	cf.cbSize=sizeof(CHARFORMAT);
	cf.dwMask=CFM_COLOR | CFM_PROTECTED;
	COLORREF clr;
	IniClr2Str(ini_cstrClrCtrlText, clr, false);
	cf.crTextColor=clr;
	cf.dwEffects=CFE_PROTECTED;
	while (1)
	{
		nStartChar=cstrBuffer.Find(_T('{'), nPos);
		nEndChar=cstrBuffer.Find(_T('}'), nPos)+1;
		nPos=nEndChar;
		if (-1==nStartChar && 0==nEndChar)//没有需要处理的控制符了
			break;
		if (nStartChar>=nEndChar || -1==nStartChar)
		{
			MessageBox(_T("'{'&'}' don't match!"), m_tcsTitle);
			return false;
		}
		m_CRichEdit2.SetSel(nStartChar, nEndChar);
		m_CRichEdit2.SetSelectionCharFormat(cf);
	}
	//取消选择，光标定位在句首
	nStartChar=0;
	nEndChar=0;
	m_CRichEdit2.SetSel(nStartChar, nEndChar);
	return true;
}

bool CMainDlg::Half2Full(CString &cstrText)
{
	int nLen;
	TCHAR tchC;
	bool bCanConv=true;

	if (!ini_bHalf2Full)
		return true;

	nLen=cstrText.GetLength();
	for (int i=0; i<nLen; i++)
	{
		tchC=cstrText.GetAt(i);
		if (_T('{')==tchC)
		{
			bCanConv=false;
			continue;
		}
		else if (_T('}')==tchC)
		{
			bCanConv=true;
			continue;
		}
		else if (bCanConv && 0x0020==tchC)//空格
		{
			cstrText.SetAt(i, 0x3000);
		}
		else if (bCanConv && 0x0021<=tchC && tchC<=0x007e)
		{
			cstrText.SetAt(i, tchC+0xfee0);
		}
	}
	return true;
}

bool CMainDlg::SaveText(int nRowNo)
{
	CString cstrBuffer, cstrOutFile, cstrAdd, cstrLen;
	FILE *fpOut;
	errno_t err;
	int nNo=0, nPos, nStrLen;
	DWORD dwFilePos, dwBlockSize;
	BYTE *byTemp=NULL;

	if (m_bTextIsDirty)
	{
		m_CInfoList.AddString(_T("==============="));
		m_CInfoList.AddString(_T("开始保存文件..."));
		m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);
		start=::GetTickCount();

		cstrOutFile=m_cstrCurrJP;
		cstrOutFile.Replace(_T("\\jp-text"), _T("\\cn-text"));
		if(((err=_tfopen_s(&fpOut, cstrOutFile, _T("rb+"))))!=NULL)
		{
			if (ENOENT==err)
			{
				if (SaveNewFile(cstrOutFile, nRowNo))
				{
					end=::GetTickCount();
					cstrBuffer.Format(_T("%d"), end-start);
					m_CInfoList.AddString(_T("保存文件耗时:")+cstrBuffer+_T("ms"));
					m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);
					return true;
				}
				else
				{
					MessageBox(_T("SaveNewFile failed!"), m_tcsTitle);
					return false;
				}
			}
			else
			{
				MessageBox(_T("Open cn-text file failed!"), m_tcsTitle);
				return false;
			}
		}
		if(0xfeff!=fgetwc(fpOut))
		{
			MessageBox(_T("The cn-text file isn't unicode type."), m_tcsTitle);
			fclose(fpOut);
			return false;
		}
		while (NULL!=fgetws(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpOut))
		{
			cstrBuffer.ReleaseBuffer();
			if (0==cstrBuffer.Compare(_T("\r\n")))//若是空行则跳过
			{
				cstrBuffer.Empty();
				continue;
			}
			nNo++;
			if (nNo<nRowNo)//若还未到当前行内容时则跳过
			{
				cstrBuffer.Empty();
				continue;
			}
			nStrLen=cstrBuffer.GetLength();
			dwFilePos=ftell(fpOut);
			fseek(fpOut, 0L, 2);
			dwBlockSize=ftell(fpOut)-dwFilePos;
			byTemp=(BYTE*)malloc(dwBlockSize);
			if (NULL!=byTemp)
			{
				fseek(fpOut, dwFilePos, 0);
				fread(byTemp, dwBlockSize, 1, fpOut);

				//获取改动过的文本
				cstrOutFile.Empty();
				m_CRichEdit2.GetWindowText(cstrOutFile.GetBuffer(ini_nTextBufLen), ini_nTextBufLen);
				cstrOutFile.ReleaseBuffer();
				//格式检查
				TextFormatCheck(cstrOutFile, false);
				//半角转全角
				Half2Full(cstrOutFile);
				//获取地址，长度
				cstrBuffer.TrimRight(_T("\r\n"));
				nPos=cstrBuffer.Find(_T(','));
				if (-1==nPos)
				{
					cstrBuffer.Format(_T("%05d"), nNo);
					MessageBox(_T("The format of the cn-text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
					fclose(fpOut);
					return false;
				}
				cstrAdd=cstrBuffer.Left(nPos);
				cstrBuffer=cstrBuffer.Mid(nPos+1);
				nPos=cstrBuffer.Find(L',');
				if (-1==nPos)
				{
					cstrBuffer.Format(_T("%05d"), nNo);
					MessageBox(_T("The format of the cn-text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
					fclose(fpOut);
					return false;
				}
				cstrLen=cstrBuffer.Left(nPos);

				cstrBuffer.Empty();
				cstrBuffer=cstrAdd+_T(",")+cstrLen+_T(",")+cstrOutFile+_T("\r\n");
				fseek(fpOut, dwFilePos-nStrLen*sizeof(TCHAR), 0);
				fwrite(cstrBuffer, cstrBuffer.GetLength()*sizeof(TCHAR), 1, fpOut);
				fwrite(byTemp, dwBlockSize, 1, fpOut);
				dwFilePos=ftell(fpOut);
				_chsize_s(_fileno(fpOut), (int)dwFilePos);
			}
			else
			{
				MessageBox(_T("There is insufficient memory available when save file!"), m_tcsTitle);
				fclose(fpOut);
				return false;
			}
			break;
		}
		free(byTemp);
		byTemp=NULL;
		fclose(fpOut);

		end=::GetTickCount();
		cstrBuffer.Empty();
		cstrBuffer.Format(_T("%d"), end-start);
		m_CInfoList.AddString(_T("保存文件耗时:")+cstrBuffer+_T("ms"));
		m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);
	}
	return true;
}

bool CMainDlg::UpdatePreview(int m_nLastRowNo)
{
	CString cstrID, cstrText, cstrTextCN, cstrTextComp, cstrComp;
	BYTE *lpS=NULL, *lpT=NULL;
	errno_t err;
	FILE *fpComp, *fpJP;
	bool bComp=true;
	int nOverLen=-1;

	if (m_bTextIsDirty)
	{
		m_CRichEdit2.GetWindowText(cstrTextCN.GetBuffer(ini_nTextBufLen), ini_nTextBufLen);
		cstrTextCN.ReleaseBuffer();
		TextFormatCheck(cstrTextCN, true);//格式检查
		Half2Full(cstrTextCN);//半角转全角
		if((err=_tfopen_s(&fpJP, m_cstrCurrJP, _T("rb")))==NULL)
		{
			nOverLen=OverLen(GetTextFromFile(fpJP, m_nLastRowNo, cstrText), cstrTextCN);
		}
		cstrComp=m_cstrCurrJP;
		cstrComp.Replace(_T("\\jp-text"), _T("\\cn-compare"));
		if((err=_tfopen_s(&fpComp, cstrComp, _T("rb")))!=NULL)
			bComp=false;
		if (bComp)
		{
			if(-1==GetTextFromFile(fpComp, m_nLastRowNo, cstrTextComp))
			{
				MessageBox(_T("GetTextFromFile failed!"), m_tcsTitle);
				fclose(fpComp);
				return false;
			}

			RenderSpec(cstrTextCN);
			RenderSpec(cstrTextComp);
			//必须先着色不同文本再着色控制符！
			if ((!cstrTextCN.IsEmpty() && !cstrTextComp.IsEmpty()) || ini_bCNCOMNULL)
			{
				lpS=(BYTE*)malloc(cstrTextCN.GetLength()*sizeof(BYTE));
				memset(lpS, 0, cstrTextCN.GetLength()*sizeof(BYTE));
				lpT=(BYTE*)malloc(cstrTextComp.GetLength()*sizeof(BYTE));
				memset(lpT, 0, cstrTextComp.GetLength()*sizeof(BYTE));
				int ld=LD(cstrTextCN, cstrTextComp, lpS, lpT);
				if (0==ld)
					cstrTextComp=_T("+-+-+-+-+-+");
				else
				{
					RenderDiff(cstrTextCN, lpS, ini_bCNShowDiff);
					RenderDiff(cstrTextComp, lpT, TRUE);
				}
			}
			if(!RenderCtrl(cstrTextCN))
			{
				MessageBox(_T("RenderCtrl(cstrTextCN) failed!"), m_tcsTitle);
				fclose(fpComp);
				return false;
			}
			if(!RenderCtrl(cstrTextComp))
			{
				MessageBox(_T("RenderCtrl(cstrTextComp) failed!"), m_tcsTitle);
				fclose(fpComp);
				return false;
			}
			if (NULL!=lpS)
			{
				free(lpS);
				lpS=NULL;
			}
			if (NULL!=lpT)
			{
				free(lpT);
				lpT=NULL;
			}

			cstrID.Format(_T("TD_COMPARE_%d"), m_nLastRowNo);
			if(!SetHTMLIDValue(cstrID, cstrTextComp))
			{
				MessageBox(_T("SetHTMLIDValue failed!"), m_tcsTitle);
				fclose(fpComp);
				return false;
			}
			cstrID.Empty();
		}
		else
		{
			if(!RenderCtrl(cstrTextCN))
			{
				MessageBox(_T("RenderCtrl(cstrTextCN) failed!"), m_tcsTitle);
				fclose(fpComp);
				return false;
			}
		}


		//着色文本越界的句子
		if (nOverLen>0)
		{
			if (0x00==m_lpByteRow[m_nLastRowNo])
			{
				m_nOverLenCount++;//必须在加入字符串到m_CInfoList之前执行
				m_lpByteRow[m_nLastRowNo]=0x01;
				//着色NO列
				cstrID.Format(_T("TD_NO_%d"), m_nLastRowNo);
				if(!GetHTMLIDValue(cstrID, cstrText))
				{
					MessageBox(_T("GetHTMLIDValue failed!"), m_tcsTitle);
					if(bComp)
						fclose(fpComp);
					return false;
				}
				cstrText=_T("<font style=\"background-color:")+ini_cstrClrOverLenBg+_T("\">")+cstrText+_T("</font>");
				if(!SetHTMLIDValue(cstrID, cstrText))
				{
					MessageBox(_T("SetHTMLIDValue failed!"), m_tcsTitle);
					if(bComp)
						fclose(fpComp);
					return false;
				}
				cstrID.Empty();
			}
			cstrTextCN=_T("<font style=\"background-color:")+ini_cstrClrOverLenBg+_T("\">")+cstrTextCN+_T("</font>");
			cstrText.Empty();
			cstrText.Format(_T("第%d行超长%d字节！"), m_nLastRowNo, nOverLen);
			m_CInfoList.AddString(_T("==============="));
			int nIndex;
			nIndex=m_CInfoList.AddString(cstrText);
			m_CInfoList.SetItemData(nIndex, (DWORD)m_nLastRowNo);
			m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);
		}
		else if (0x01==m_lpByteRow[m_nLastRowNo])
		{
			m_nOverLenCount--;
			m_lpByteRow[m_nLastRowNo]=0x00;
			//着色NO列
			cstrID.Format(_T("TD_NO_%d"), m_nLastRowNo);
			if(!GetHTMLIDValue(cstrID, cstrText))
			{
				MessageBox(_T("GetHTMLIDValue failed!"), m_tcsTitle);
				if(bComp)
					fclose(fpComp);
				return false;
			}
			cstrText.TrimLeft(_T("<FONT style=\"BACKGROUND-COLOR:")+ini_cstrClrOverLenBg+_T("\">"));
			cstrText.TrimRight(_T("</FONT>"));
			if(!SetHTMLIDValue(cstrID, cstrText))
			{
				MessageBox(_T("SetHTMLIDValue failed!"), m_tcsTitle);
				if(bComp)
					fclose(fpComp);
				return false;
			}
			cstrID.Empty();
			cstrText.Empty();
			cstrText.Format(_T("已修正第%d行超长问题"), m_nLastRowNo);
			m_CInfoList.AddString(_T("==============="));
			m_CInfoList.AddString(cstrText);
			m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);
		}

		cstrID.Format(_T("TD_PREVIEW_%d"), m_nLastRowNo);
		if(!SetHTMLIDValue(cstrID, cstrTextCN))
		{
			MessageBox(_T("SetHTMLIDValue failed!"), m_tcsTitle);
			if(bComp)
				fclose(fpComp);
			return false;
		}
		if(bComp)
			fclose(fpComp);
	}
	return true;
}

bool CMainDlg::SaveNewFile(LPCTSTR lpNewFile, int nRowNo)
{
	FILE *fpJP, *fpOut;
	errno_t err;
	CString cstrBuffer, cstrAdd, cstrLen;
	int nNo=0, nPos;

	if((err=_tfopen_s(&fpJP, m_cstrCurrJP, _T("rb")))!=NULL)
	{
		MessageBox(_T("Open jp-text file failed!"), m_tcsTitle);
		return false;
	}
	if(0xfeff!=fgetwc(fpJP))
	{
		MessageBox(_T("The jp-text file isn't unicode type."), m_tcsTitle);
		fclose(fpJP);
		return false;
	}
	if((err=_tfopen_s(&fpOut, lpNewFile, _T("wb")))!=NULL)
	{
		MessageBox(_T("Create cn-text file failed!"), m_tcsTitle);
		return false;
	}
	fputwc(0xfeff, fpOut);
	while (NULL!=fgetws(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpJP))
	{
		cstrBuffer.ReleaseBuffer();
		if (0==cstrBuffer.Compare(_T("\r\n")))//若是空行则跳过
		{
			cstrBuffer.Empty();
			continue;
		}
		//获取地址，长度
		cstrBuffer.TrimRight(_T("\r\n"));
		nPos=cstrBuffer.Find(_T(','));
		if (-1==nPos)
		{
			cstrBuffer.Format(_T("%05d"), nNo);
			MessageBox(_T("The format of the jp-text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
			fclose(fpOut);
			return false;
		}
		cstrAdd=cstrBuffer.Left(nPos);
		cstrBuffer=cstrBuffer.Mid(nPos+1);
		nPos=cstrBuffer.Find(L',');
		if (-1==nPos)
		{
			cstrBuffer.Format(_T("%05d"), nNo);
			MessageBox(_T("The format of the jp-text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
			fclose(fpOut);
			return false;
		}
		cstrLen=cstrBuffer.Left(nPos);

		cstrBuffer=cstrAdd+_T(",")+cstrLen+_T(",");
		fputws(cstrBuffer, fpOut);
		nNo++;
		if (nNo==nRowNo)//若到了当前行内容时
		{
			//获取改动过的文本
			cstrBuffer.Empty();
			m_CRichEdit2.GetWindowText(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen);
			cstrBuffer.ReleaseBuffer();
			//格式检查
			TextFormatCheck(cstrBuffer, false);
			//半角转全角
			Half2Full(cstrBuffer);
			fputws(cstrBuffer, fpOut);
		}
		fputws(_T("\r\n\r\n"), fpOut);
		cstrBuffer.Empty();
	}
	fclose(fpOut);
	fclose(fpJP);
	return true;
}
LRESULT CMainDlg::OnEnProtectedRichedit21(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	BYTE byState;
	ENPROTECTED *pEnProtected = reinterpret_cast<ENPROTECTED *>(pNMHDR);
	// TODO:  The control will not send this notification unless you override the
	// __super::OnInitDialog() function to send the EM_SETEVENTMASK message
	// to the control with the ENM_PROTECTED flag ORed into the lParam mask.

	// TODO:  Add your control notification handler code here
	//修正插入点在控制符前且是首字符文本格式异常的问题
	byState=::GetKeyState(VK_DELETE)>>8;
	if (0==pEnProtected->chrg.cpMin && 0==pEnProtected->chrg.cpMax && 0xff!=byState || bFixFirstChar)
	{
		bFixFirstChar=true;
		return 0;
	}
	//修正插入点在控制符后文本格式异常的问题
	CHARFORMAT cf;
	m_CRichEdit2.SetSel(pEnProtected->chrg.cpMin, pEnProtected->chrg.cpMax+1);
	m_CRichEdit2.GetSelectionCharFormat(cf);
	m_CRichEdit2.SetSel(pEnProtected->chrg.cpMin, pEnProtected->chrg.cpMax);
	if (CFE_PROTECTED!=(cf.dwEffects & CFE_PROTECTED))
	{
		byState=::GetKeyState(VK_BACK)>>8;
		if(0xff==byState && !ini_bEditCtrlchar)
			return -1;
		else
			return 0;
	}
	//修正插入点在两个控制符之间无法输入字符的问题
	CString cstrBuffer;
	m_CRichEdit2.GetWindowText(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen);
	cstrBuffer.ReleaseBuffer();
	if (_T('}')==cstrBuffer.GetAt(pEnProtected->chrg.cpMin-1) && _T('{')==cstrBuffer.GetAt(pEnProtected->chrg.cpMax))
	{
		byState=::GetKeyState(VK_DELETE)>>8;
		if (0xff!=byState)
		{
			byState=::GetKeyState(VK_BACK)>>8;
			if (0xff!=byState)
			{
				bFixMidChar=true;
				nCurPos=pEnProtected->chrg.cpMin;
				return 0;
			}
		}
	}

	byState=::GetKeyState(VK_CONTROL)>>8;
	if(0xff==byState)//修复按下"Ctrl"+"<"或">"时可能会失效的问题
		return 0;
	if (::GetFocus()==pEnProtected->nmhdr.hwndFrom && m_CRichEdit2.GetModify())
	{
		if(ini_bEditCtrlchar)
			return 0;
		else
			return -1;
	}
	return 0;
}

LRESULT CMainDlg::OnEnChangeRichedit21(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the __super::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	LONG nStartChar, nEndChar;
	CString cstrBuffer;
	CHARFORMAT cf;

	if (::GetFocus()==hWndCtl)
	{
		if (bFixFirstChar)
		{
			nStartChar=0;
			m_CRichEdit2.GetWindowText(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen);
			cstrBuffer.ReleaseBuffer();
			nEndChar=cstrBuffer.Find(_T('{'), 0);
			m_CRichEdit2.SetSel(nStartChar, nEndChar);
			m_CRichEdit2.GetSelectionCharFormat(cf);
			cf.dwMask=CFM_COLOR | CFM_PROTECTED;
			cf.dwEffects=CFE_AUTOCOLOR;
			cf.crTextColor=GetSysColor(COLOR_WINDOWTEXT);
			m_CRichEdit2.SetSelectionCharFormat(cf);
			bFixFirstChar=false;
			//取消选择，光标定位在插入的字符后
			nStartChar=nEndChar;
			m_CRichEdit2.SetSel(nStartChar, nEndChar);
		}
		if (bFixMidChar)
		{
			m_CRichEdit2.GetWindowText(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen);
			cstrBuffer.ReleaseBuffer();
			nEndChar=cstrBuffer.Find(_T('{'), nCurPos);
			m_CRichEdit2.SetSel(nCurPos, nEndChar);
			m_CRichEdit2.GetSelectionCharFormat(cf);
			cf.dwMask=CFM_COLOR | CFM_PROTECTED;
			cf.dwEffects=CFE_AUTOCOLOR;
			cf.crTextColor=GetSysColor(COLOR_WINDOWTEXT);
			m_CRichEdit2.SetSelectionCharFormat(cf);
			bFixMidChar=false;
			//取消选择，光标定位在插入的字符后
			m_CRichEdit2.SetSel(nEndChar, nEndChar);
		}
		m_bTextIsDirty=true;
	}
	return 0;
}

LRESULT CMainDlg::OnEnMsgfilterRichedit21(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	MSGFILTER *pMsgFilter = reinterpret_cast<MSGFILTER *>(pNMHDR);
	// TODO:  The control will not send this notification unless you override the
	// __super::OnInitDialog() function to send the EM_SETEVENTMASK message
	// to the control with either the ENM_KEYEVENTS or ENM_MOUSEEVENTS flag 
	// ORed into the lParam mask.

	// TODO:  Add your control notification handler code here
	if (WM_KEYUP==pMsgFilter->msg)
	{
		if (VK_OEM_PERIOD==pMsgFilter->wParam || VK_OEM_COMMA==pMsgFilter->wParam)//">"或者"<"键被释放
		{
			BYTE byState=::GetKeyState(VK_CONTROL)>>8;
			if (0xff==byState)//并且"Ctrl"键同时被按下
			{
				if (VK_OEM_PERIOD==pMsgFilter->wParam)
				{
					SetInputValue(_T("NEXT_ID"), _T("+"));
					//Call JS
					if ( m_CallScript.DocumentSet() == FALSE)
					{
						IDispatch* d = NULL;
						m_pWB2->get_Document(&d);
						m_CallScript.SetDocument(d);
						d->Release();
					}
					m_CallScript.Run(L"monitor_core", "");
				}
				else
				{
					SetInputValue(_T("NEXT_ID"), _T("-"));
					//Call JS
					if ( m_CallScript.DocumentSet() == FALSE)
					{
						IDispatch* d = NULL;
						m_pWB2->get_Document(&d);
						m_CallScript.SetDocument(d);
						d->Release();
					}
					m_CallScript.Run(L"monitor_core", "");
				}
				pMsgFilter->wParam=0;//取消原有键的功能
				return -1;
			}
		}
	}
	return 0;
}

LRESULT CMainDlg::OnBnClickedCheck1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
 	m_bShouldScrol=true;
	m_CFileList.SetFocus();
	//模拟用户按下了空格键
	::keybd_event(VK_SPACE, 0, 0, 0);
	::keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
	return 0;
}

bool CMainDlg::ReadINI()
{
	//宽度
	::GetPrivateProfileString(_T("WIDTH"), _T("WIDTH_ID"), DEFAULT_WIDTH_ID, ini_cstrWidthID.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrWidthID.ReleaseBuffer();
	::GetPrivateProfileString(_T("WIDTH"), _T("WIDTH_JP"), DEFAULT_WIDTH_JP, ini_cstrWidthJP.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrWidthJP.ReleaseBuffer();
	::GetPrivateProfileString(_T("WIDTH"), _T("WIDTH_CN"), DEFAULT_WIDTH_CN, ini_cstrWidthCN.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrWidthCN.ReleaseBuffer();
	::GetPrivateProfileString(_T("WIDTH"), _T("WIDTH_COM"), DEFAULT_WIDTH_COM, ini_cstrWidthCOM.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrWidthCOM.ReleaseBuffer();

	//高级
	ini_nBytePerChar=::GetPrivateProfileInt(_T("ADVANCE"), _T("BYTE_PER_CHAR"), DEFAULT_BYTE_PER_CHAR, _T(".\\Config.ini"));
	ini_nTextBufLen=::GetPrivateProfileInt(_T("ADVANCE"), _T("TEXT_BUF_LEN"), DEFAULT_TEXT_BUF_LEN, _T(".\\Config.ini"));
	ini_nItemPerPage=::GetPrivateProfileInt(_T("ADVANCE"), _T("ITEM_PER_PAGE"), DEFAULT_ITEM_PER_PAGE, _T(".\\Config.ini"));
	ini_nWriteFilePerTime=::GetPrivateProfileInt(_T("ADVANCE"), _T("WRITE_FILE_PER_TIME"), DEFAULT_WRITE_FILE_PER_TIME, _T(".\\Config.ini"));
	ini_bEditCtrlchar=::GetPrivateProfileInt(_T("ADVANCE"), _T("EDIT_CTRLCHAR"), DEFAULT_EDIT_CTRLCHAR, _T(".\\Config.ini"));
	ini_bHalf2Full=::GetPrivateProfileInt(_T("ADVANCE"), _T("HALF_TO_FULL"), DEFAULT_HALF_TO_FULL, _T(".\\Config.ini"));
	ini_bSpliterPos=::GetPrivateProfileInt(_T("ADVANCE"), _T("SAVE_SPLITER_POS"), DEFAULT_SAVE_SPLITER_POS, _T(".\\Config.ini"));
	m_nSpliterPer=::GetPrivateProfileInt(_T("ADVANCE"), _T("SPLITER_PERCENT"), DEFAULT_SPLITER_PERCENT, _T(".\\Config.ini"));
	ini_bJPShowCNFont=::GetPrivateProfileInt(_T("ADVANCE"), _T("JP_SHOW_CNFONT"), DEFAULT_JP_SHOW_CNFONT, _T(".\\Config.ini"));
	ini_bCNShowDiff=::GetPrivateProfileInt(_T("ADVANCE"), _T("CN_SHOW_DIFF"), DEFAULT_CN_SHOW_DIFF, _T(".\\Config.ini"));
	ini_bOnePageSlider=::GetPrivateProfileInt(_T("ADVANCE"), _T("ONE_PAGE_SLIDER"), DEFAULT_ONE_PAGE_SLIDER, _T(".\\Config.ini"));
	ini_nRE2FintSize=::GetPrivateProfileInt(_T("ADVANCE"), _T("RE2_FONT_SIZE"), DEFAULT_RE2_FONT_SIZE, _T(".\\Config.ini"));
	ini_bCNCOMNULL=::GetPrivateProfileInt(_T("ADVANCE"), _T("CN_COM_NULL"), DEFAULT_CN_COM_NULL, _T(".\\Config.ini"));
	ini_bOutCtrlLen=::GetPrivateProfileInt(_T("ADVANCE"), _T("OUT_CTRL_LEN"), DEFAULT_OUT_CTRL_LEN, _T(".\\Config.ini"));

	//颜色
	::GetPrivateProfileString(_T("COLOR"), _T("NOR_TEXT"), DEFAULT_NOR_TEXT, ini_cstrClrNorText.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrClrNorText.ReleaseBuffer();
	ini_cstrClrNorText.MakeLower();
	::GetPrivateProfileString(_T("COLOR"), _T("CTRL_TEXT"), DEFAULT_CTRL_TEXT, ini_cstrClrCtrlText.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrClrCtrlText.ReleaseBuffer();
	ini_cstrClrCtrlText.MakeLower();
	::GetPrivateProfileString(_T("COLOR"), _T("DIFF_TEXT"), DEFAULT_DIFF_TEXT, ini_cstrClrDiffText.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrClrDiffText.ReleaseBuffer();
	ini_cstrClrDiffText.MakeLower();
	::GetPrivateProfileString(_T("COLOR"), _T("OVER_LEN_BG"), DEFAULT_OVER_LEN_BG, ini_cstrClrOverLenBg.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrClrOverLenBg.ReleaseBuffer();
	ini_cstrClrOverLenBg.MakeLower();
	::GetPrivateProfileString(_T("COLOR"), _T("NOR_TR"), DEFAULT_NOR_TR, ini_cstrClrNorTr.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrClrNorTr.ReleaseBuffer();
	ini_cstrClrNorTr.MakeLower();
	::GetPrivateProfileString(_T("COLOR"), _T("SEL_TR"), DEFAULT_SEL_TR, ini_cstrClrSelTr.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrClrSelTr.ReleaseBuffer();
	ini_cstrClrSelTr.MakeLower();
	::GetPrivateProfileString(_T("COLOR"), _T("TAB_BORDER"), DEFAULT_TAB_BORDER, ini_cstrClrTabBorder.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrClrTabBorder.ReleaseBuffer();
	ini_cstrClrTabBorder.MakeLower();
	::GetPrivateProfileString(_T("COLOR"), _T("BODY_BG"), DEFAULT_BODY_BG, ini_cstrClrBodyBg.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrClrBodyBg.ReleaseBuffer();
	ini_cstrClrBodyBg.MakeLower();
	::GetPrivateProfileString(_T("COLOR"), _T("HILIRE_BG"), DEFAULT_HILIRE_BG, ini_cstrClrHiLiREBg.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN, _T(".\\Config.ini"));
	ini_cstrClrHiLiREBg.ReleaseBuffer();
	ini_cstrClrHiLiREBg.MakeLower();
	return true;
}

bool CMainDlg::WriteINI()
{
	CString cstrBuffer;

	//宽度
	::WritePrivateProfileString(_T("WIDTH"), _T("WIDTH_ID"), ini_cstrWidthID, _T(".\\Config.ini"));
	::WritePrivateProfileString(_T("WIDTH"), _T("WIDTH_JP"), ini_cstrWidthJP, _T(".\\Config.ini"));
	::WritePrivateProfileString(_T("WIDTH"), _T("WIDTH_CN"), ini_cstrWidthCN, _T(".\\Config.ini"));
	::WritePrivateProfileString(_T("WIDTH"), _T("WIDTH_COM"), ini_cstrWidthCOM, _T(".\\Config.ini"));

	//高级
	cstrBuffer.Format(_T("%d"), ini_nBytePerChar);
	::WritePrivateProfileString(_T("ADVANCE"), _T("BYTE_PER_CHAR"), cstrBuffer, _T(".\\Config.ini"));
	cstrBuffer.Format(_T("%d"), ini_nTextBufLen);
	::WritePrivateProfileString(_T("ADVANCE"), _T("TEXT_BUF_LEN"), cstrBuffer, _T(".\\Config.ini"));
	cstrBuffer.Format(_T("%d"), ini_nItemPerPage);
	::WritePrivateProfileString(_T("ADVANCE"), _T("ITEM_PER_PAGE"), cstrBuffer, _T(".\\Config.ini"));
	cstrBuffer.Format(_T("%d"), ini_nWriteFilePerTime);
	::WritePrivateProfileString(_T("ADVANCE"), _T("WRITE_FILE_PER_TIME"), cstrBuffer, _T(".\\Config.ini"));
	if(ini_bEditCtrlchar)
		::WritePrivateProfileString(_T("ADVANCE"), _T("EDIT_CTRLCHAR"), _T("1"), _T(".\\Config.ini"));
	else
		::WritePrivateProfileString(_T("ADVANCE"), _T("EDIT_CTRLCHAR"), _T("0"), _T(".\\Config.ini"));
	if(ini_bHalf2Full)
		::WritePrivateProfileString(_T("ADVANCE"), _T("HALF_TO_FULL"), _T("1"), _T(".\\Config.ini"));
	else
		::WritePrivateProfileString(_T("ADVANCE"), _T("HALF_TO_FULL"), _T("0"), _T(".\\Config.ini"));
	if(ini_bSpliterPos)
		::WritePrivateProfileString(_T("ADVANCE"), _T("SAVE_SPLITER_POS"), _T("1"), _T(".\\Config.ini"));
	else
		::WritePrivateProfileString(_T("ADVANCE"), _T("SAVE_SPLITER_POS"), _T("0"), _T(".\\Config.ini"));
	cstrBuffer.Format(_T("%d"), m_nSpliterPer);
	::WritePrivateProfileString(_T("ADVANCE"), _T("SPLITER_PERCENT"), cstrBuffer, _T(".\\Config.ini"));
	if(ini_bJPShowCNFont)
		::WritePrivateProfileString(_T("ADVANCE"), _T("JP_SHOW_CNFONT"), _T("1"), _T(".\\Config.ini"));
	else
		::WritePrivateProfileString(_T("ADVANCE"), _T("JP_SHOW_CNFONT"), _T("0"), _T(".\\Config.ini"));
	if(ini_bCNShowDiff)
		::WritePrivateProfileString(_T("ADVANCE"), _T("CN_SHOW_DIFF"), _T("1"), _T(".\\Config.ini"));
	else
		::WritePrivateProfileString(_T("ADVANCE"), _T("CN_SHOW_DIFF"), _T("0"), _T(".\\Config.ini"));
	if(ini_bOnePageSlider)
		::WritePrivateProfileString(_T("ADVANCE"), _T("ONE_PAGE_SLIDER"), _T("1"), _T(".\\Config.ini"));
	else
		::WritePrivateProfileString(_T("ADVANCE"), _T("ONE_PAGE_SLIDER"), _T("0"), _T(".\\Config.ini"));
	cstrBuffer.Format(_T("%d"), ini_nRE2FintSize);
	::WritePrivateProfileString(_T("ADVANCE"), _T("RE2_FONT_SIZE"), cstrBuffer, _T(".\\Config.ini"));
	if(ini_bCNCOMNULL)
		::WritePrivateProfileString(_T("ADVANCE"), _T("CN_COM_NULL"), _T("1"), _T(".\\Config.ini"));
	else
		::WritePrivateProfileString(_T("ADVANCE"), _T("CN_COM_NULL"), _T("0"), _T(".\\Config.ini"));
	if(ini_bOutCtrlLen)
		::WritePrivateProfileString(_T("ADVANCE"), _T("OUT_CTRL_LEN"), _T("1"), _T(".\\Config.ini"));
	else
		::WritePrivateProfileString(_T("ADVANCE"), _T("OUT_CTRL_LEN"), _T("0"), _T(".\\Config.ini"));

	//颜色
	::WritePrivateProfileString(_T("COLOR"), _T("NOR_TEXT"), ini_cstrClrNorText, _T(".\\Config.ini"));
	::WritePrivateProfileString(_T("COLOR"), _T("CTRL_TEXT"), ini_cstrClrCtrlText, _T(".\\Config.ini"));
	::WritePrivateProfileString(_T("COLOR"), _T("DIFF_TEXT"), ini_cstrClrDiffText, _T(".\\Config.ini"));
	::WritePrivateProfileString(_T("COLOR"), _T("OVER_LEN_BG"), ini_cstrClrOverLenBg, _T(".\\Config.ini"));
	::WritePrivateProfileString(_T("COLOR"), _T("NOR_TR"), ini_cstrClrNorTr, _T(".\\Config.ini"));
	::WritePrivateProfileString(_T("COLOR"), _T("SEL_TR"), ini_cstrClrSelTr, _T(".\\Config.ini"));
	::WritePrivateProfileString(_T("COLOR"), _T("TAB_BORDER"), ini_cstrClrTabBorder, _T(".\\Config.ini"));
	::WritePrivateProfileString(_T("COLOR"), _T("BODY_BG"), ini_cstrClrBodyBg, _T(".\\Config.ini"));
	::WritePrivateProfileString(_T("COLOR"), _T("HILIRE_BG"), ini_cstrClrHiLiREBg, _T(".\\Config.ini"));
	return true;
}

int CMainDlg::OverLen(int nLen, LPCTSTR lpText)
{
	int nStartChar, nEndChar, nPos=0, nP;
	CString cstrBuffer;
	int nCtrlLen=0, nHalfLen=0, nOverLen, nOutCtrlLen=0;

	cstrBuffer=lpText;

	//处理控制符
	while (1)
	{
		nStartChar=cstrBuffer.Find(_T('{'), nPos);
		nEndChar=cstrBuffer.Find(_T('}'), nPos)+1;
		nPos=nStartChar;
		if (-1==nStartChar && 0==nEndChar)//没有需要处理的控制符了
			break;
		if (nStartChar>=nEndChar || -1==nStartChar)
		{
			MessageBox(_T("'{'&'}' don't match!"), m_tcsTitle);
			return -1;
		}
		if (ini_bOutCtrlLen)//外部控制符长度
		{
			CString cstrCtrl;
			cstrCtrl=cstrBuffer.Mid(nStartChar, nEndChar-nStartChar);
			nOutCtrlLen=GetOutCtrlLen(cstrCtrl);
			nCtrlLen+=nOutCtrlLen;
		}
		if (0==nOutCtrlLen)//内部控制符长度，或外部控制符长度未找到
		{
			nP=cstrBuffer.Find(_T(','), nStartChar);
			if (nP>=nEndChar || -1==nP)
				nCtrlLen+=8;
			else
			{
				LPCTSTR lp=(LPCTSTR)(cstrBuffer)+nP+1;
				nCtrlLen+=_tstoi(lp);
			}
		}
		cstrBuffer.Delete(nStartChar, nEndChar-nStartChar);
	}
	//处理半角字符
	for (TCHAR tchC=0x0020; tchC<=0x007e; tchC++)
	{
		nHalfLen+=cstrBuffer.Remove(tchC);
	}

	nOverLen=nLen-(cstrBuffer.GetLength()*ini_nBytePerChar+nCtrlLen+nHalfLen);
	if(nOverLen>=0)
		return 0;
	else
		return -nOverLen;
}
LRESULT CMainDlg::OnCtlColorListbox(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// TODO: Add your message handler code here and/or call default
	HDC hDc=(HDC)wParam;
	COLORREF clrBk;
	HBRUSH hBrushBk;

	hBrushBk=::GetSysColorBrush(COLOR_WINDOW);
	::SetBkMode(hDc, TRANSPARENT);
	if (m_nOverLenCount>0)
	{
		IniClr2Str(ini_cstrClrOverLenBg, clrBk, false);
		hBrushBk=CreateSolidBrush(clrBk);
	}
	return (LRESULT)hBrushBk;
}

long CMainDlg::GetHTMLScrollTop()
{
	CComPtr<IDispatch> pDisp;
	hr = m_pWB2->get_Document(&pDisp);
	if (SUCCEEDED(hr))   
	{
		CComQIPtr<IHTMLDocument2> pDoc2=pDisp;
		CComPtr<IHTMLElement> pBody;
		hr = pDoc2->get_body(&pBody);
		if (SUCCEEDED(hr))
		{
			CComPtr<IHTMLElement2> pElem2;
			hr = pBody->QueryInterface(&pElem2);
			if (SUCCEEDED(hr))
			{
				long scroll_top;
				pElem2->get_scrollTop( &scroll_top );
				return scroll_top;
			}
		}
	}
	return -1L;
}

bool CMainDlg::SetHTMLScrollTopLeft(long scroll_top, long scroll_left)
{
	CComPtr<IDispatch> pDisp;
	hr = m_pWB2->get_Document(&pDisp);
	if (SUCCEEDED(hr))   
	{
		CComQIPtr<IHTMLDocument2> pDoc2=pDisp;
		CComPtr<IHTMLElement> pBody;
		hr = pDoc2->get_body(&pBody);
		if (SUCCEEDED(hr))
		{
			CComPtr<IHTMLElement2> pElem2;
			hr = pBody->QueryInterface(&pElem2);
			if (SUCCEEDED(hr))
			{
				pElem2->put_scrollTop( scroll_top );
				pElem2->put_scrollLeft( scroll_left );
				return true;
			}
		}
	}
	return false;
}

long CMainDlg::GetHTMLIDTop(LPCTSTR lpID)
{
	CComPtr<IDispatch> pDisp;
	hr = m_pWB2->get_Document(&pDisp);
	if (SUCCEEDED(hr))   
	{
		CComQIPtr<IHTMLDocument2> pDoc2=pDisp;
		CComPtr<IHTMLElementCollection> pAll;
		hr = pDoc2->get_all(&pAll);
		if (SUCCEEDED(hr))
		{
			LPCOLESTR pszElementID;
			pszElementID=CComBSTR(lpID);
			CComVariant varID = pszElementID;
			CComPtr<IDispatch> pDispItem;
			hr = pAll->item( varID, CComVariant(0), &pDispItem );
			if (SUCCEEDED(hr) && NULL!=pDispItem)//若NULL==pDispItem就是指未找到指定的ID
			{
				CComPtr<IHTMLElement> pElem;
				CComPtr<IHTMLElement> pElemParent;
				hr = pDispItem->QueryInterface(&pElem);
				if (SUCCEEDED(hr))
				{
					long offsetTop, offsetTopParent;
					hr = pElem->get_offsetParent(&pElemParent);
					if (SUCCEEDED(hr))
					{
						hr = pElemParent->get_offsetTop(&offsetTopParent);
						if (SUCCEEDED(hr))
						{
							hr = pElem->get_offsetTop(&offsetTop);
							if (SUCCEEDED(hr))
							{
								offsetTop+=offsetTopParent;
								return offsetTop;
							}
						}
					}
				}
			}
		}
	}
	return -1;
}

LRESULT CMainDlg::OnLbnDblclkList1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	int nSelIndex;
	DWORD dwItemData;

	nSelIndex=m_CInfoList.GetCurSel();
	dwItemData=m_CInfoList.GetItemData(nSelIndex);
	if (dwItemData>0)
	{
		CString cstrID;
		cstrID.Format(_T("%d"), dwItemData);
		//Call JS
		if ( m_CallScript.DocumentSet() == FALSE)
		{
			IDispatch* d = NULL;
			m_pWB2->get_Document(&d);
			m_CallScript.SetDocument(d);
			d->Release();
		}
		m_CallScript.Run(L"monitor_core", (LPCTSTR)cstrID);
	}
	return 0;
}

bool CMainDlg::CtrlEffect(CString &cstrText)
{
	CString cstrBuffer, cstrOld, cstrNew;
	int nStart=0, nEnd, nPos;

	while(1)
	{
		nEnd=m_cstrCtrlEffect.Find(_T("\r\n"), nStart);
		if (-1==nEnd)
			break;
		cstrBuffer=m_cstrCtrlEffect.Mid(nStart, nEnd-nStart);
		if (_T("")==cstrBuffer)
		{
			nStart=nEnd+2;
			continue;
		}
		nPos=cstrBuffer.Find(_T('='));
		if (-1==nPos)
		{
			MessageBox(_T("The format of the Ctrl_Effect.txt is wrong!"), m_tcsTitle);
			return false;
		}
		cstrOld=cstrBuffer.Left(nPos);
		cstrNew=cstrBuffer.Mid(nPos+1);
		cstrText.Replace(cstrOld, cstrNew);
		nStart=nEnd+2;
	}
	return true;
}

int CMainDlg::GetOutCtrlLen(LPCTSTR lpctCtrl)
{
	/*CString cstrBuffer, cstrCtrl, cstrLen;
	int nStart=0, nEnd, nPos;

	while(ini_bOutCtrlLen)
	{
		nEnd=m_cstrCtrlLen.Find(_T("\r\n"), nStart);
		if (-1==nEnd)
				break;
		cstrBuffer=m_cstrCtrlLen.Mid(nStart, nEnd-nStart);
		if (_T("")==cstrBuffer)
		{
			nStart=nEnd+2;
			continue;
		}
		nPos=cstrBuffer.Find(_T('='));
		if (-1==nPos)
		{
			MessageBox(_T("The format of the Ctrl_Len.txt is wrong!"), m_tcsTitle);
			return -1;
		}
		cstrCtrl=cstrBuffer.Left(nPos);
		cstrLen=cstrBuffer.Mid(nPos+1);
		if (lpctCtrl==cstrCtrl)
			return _tstoi(cstrLen);
		nStart=nEnd+2;
	}*/
	return GetHashValue(lpctCtrl);
}

bool CMainDlg::ReadCtrlEffect()
{
	FILE *fpIn;
	errno_t err;
	DWORD dwSize;

	if((err=_tfopen_s(&fpIn, _T("Ctrl_Effect.txt"), _T("rb")))!=NULL)
	{
		MessageBox(_T("No Ctrl_Effect.txt file!"), m_tcsTitle);
		return false;
	}
	if(0xfeff!=fgetwc(fpIn))
	{
		MessageBox(_T("The Ctrl_Effect.txt isn't unicode type."), m_tcsTitle);
		fclose(fpIn);
		return false;
	}
	fseek(fpIn, 0L, 2);
	dwSize=ftell(fpIn)-2;
	fseek(fpIn, 2L, 0);
	fread(m_cstrCtrlEffect.GetBuffer(dwSize/sizeof(TCHAR)), dwSize, 1, fpIn);
	m_cstrCtrlEffect.ReleaseBuffer();
	TCHAR *lptch;
	lptch=m_cstrCtrlEffect.GetBuffer(dwSize/sizeof(TCHAR)+3);
	m_cstrCtrlEffect.ReleaseBuffer();
	lptch[dwSize/sizeof(TCHAR)]=_T('\r');
	lptch[dwSize/sizeof(TCHAR)+1]=_T('\n');
	lptch[dwSize/sizeof(TCHAR)+2]=_T('\0');
	fclose(fpIn);
	return true;
}

bool CMainDlg::ReadCtrlLen()
{
	FILE *fpIn;
	errno_t err;
	CString cstrBuffer, cstrCtrl, cstrLen;
	int nPos;

	if (!ini_bOutCtrlLen)
	{
		m_cstrCtrlLen=_T("");
		return false;
	}

	if((err=_tfopen_s(&fpIn, _T("Ctrl_Len.txt"), _T("rb")))!=NULL)
	{
		MessageBox(_T("No Ctrl_Len.txt file!"), m_tcsTitle);
		return false;
	}
	if(0xfeff!=fgetwc(fpIn))
	{
		MessageBox(_T("The Ctrl_Len.txt isn't unicode type."), m_tcsTitle);
		fclose(fpIn);
		return false;
	}

	/*DWORD dwSize;
	fseek(fpIn, 0L, 2);
	dwSize=ftell(fpIn)-2;
	fseek(fpIn, 2L, 0);
	fread(m_cstrCtrlLen.GetBuffer(dwSize/sizeof(TCHAR)), dwSize, 1, fpIn);
	m_cstrCtrlLen.ReleaseBuffer();
	TCHAR *lptch;
	lptch=m_cstrCtrlLen.GetBuffer(dwSize/sizeof(TCHAR)+3);
	m_cstrCtrlLen.ReleaseBuffer();
	lptch[dwSize/sizeof(TCHAR)]=_T('\r');
	lptch[dwSize/sizeof(TCHAR)+1]=_T('\n');
	lptch[dwSize/sizeof(TCHAR)+2]=_T('\0');*/

	while (NULL!=fgetws(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpIn))
	{
		cstrBuffer.ReleaseBuffer();
		if (0==cstrBuffer.Compare(_T("\r\n")))//若是空行则跳过
		{
			cstrBuffer.Empty();
			continue;
		}
		cstrBuffer.TrimRight(_T("\r\n"));
		nPos=cstrBuffer.Find(_T('='));
		if (-1==nPos)
		{
			MessageBox(_T("The format of the Ctrl_Len.txt is wrong!"), m_tcsTitle);
			fclose(fpIn);
			return false;
		}
		cstrCtrl=cstrBuffer.Left(nPos);
		cstrLen=cstrBuffer.Mid(nPos+1);
		if(!BuildHashTable(cstrCtrl, _tstoi(cstrLen)))
		{
			MessageBox(_T("BuildHashTable failed!"), m_tcsTitle);
			fclose(fpIn);
			return false;
		}
	}
	fclose(fpIn);
	return true;
}

LRESULT CMainDlg::OnBnClickedButton4(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	LVFINDINFO lvfi;
	TCHAR tszFileName[MAX_PATH], tszItemAll[MAX_PATH];
	int nIndex, nPos, nItemAll, nLenNew, nLenOld, *pnLen, k=0, nLastRowNo;
	CString cstrBuffer, cstrNew, cstrOld, *pcstrText;
	FILE *fpJP, *fpCN;
	errno_t err;
	bool bAllCancel=false;
	int nTran=0, nOver=0, nCancel=0;

	if (0==m_nLastRowNo)
		return 0;

	::EnableWindow(GetDlgItem(ID_APP_ABOUT), FALSE);
	::EnableWindow(GetDlgItem(IDC_CHECK1), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON1), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON2), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON3), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON5), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON6), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON7), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON8), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON10), FALSE);

	/*start = ::GetTickCount();
	m_CInfoList.AddString(_T("==============="));
	m_CInfoList.AddString(_T("开始翻译重复文本..."));
	m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);*/

	nPos=m_cstrCurrJP.ReverseFind(_T('\\'));
	cstrBuffer=m_cstrCurrJP.Right(m_cstrCurrJP.GetLength()-nPos-1);
	memset(&lvfi, 0, sizeof(LVFINDINFO));
	lvfi.flags=LVFI_STRING;
	lvfi.psz=cstrBuffer;
	nIndex=m_CFileList.FindItem(&lvfi, -1);
	if (-1!=nIndex)
	{
		m_CFileList.GetItemText(nIndex, 0, tszFileName, MAX_PATH);
		m_CFileList.GetItemText(nIndex, 2, tszItemAll, MAX_PATH);
		nItemAll=_tstoi(tszItemAll);

		if((err=_tfopen_s(&fpJP, m_cstrCurrJP, _T("rb")))!=NULL)
		{
			MessageBox(_T("Open jp-text file failed!"), m_tcsTitle);
			::EnableWindow(GetDlgItem(ID_APP_ABOUT), TRUE);
			::EnableWindow(GetDlgItem(IDC_CHECK1), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON1), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON2), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON3), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON5), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON6), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON7), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON8), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON10), TRUE);
			return 0;
		}
		cstrBuffer=m_cstrCurrJP;
		cstrBuffer.Replace(_T("\\jp-text"), _T("\\cn-text"));
		if((err=_tfopen_s(&fpCN, cstrBuffer, _T("rb+")))!=NULL)
		{
			m_CInfoList.AddString(_T("==============="));
			m_CInfoList.AddString(_T("无译文文本文件！"));
			m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);
			fclose(fpJP);
			::EnableWindow(GetDlgItem(ID_APP_ABOUT), TRUE);
			::EnableWindow(GetDlgItem(IDC_CHECK1), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON1), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON2), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON3), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON5), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON6), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON7), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON8), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON10), TRUE);
			return 0;
		}

		//构造pnLen和pcstrText数组
		pnLen=(int*)malloc(nItemAll*sizeof(int));
		if (NULL==pnLen)
		{
			MessageBox(_T("There is insufficient memory available!"), m_tcsTitle);
			fclose(fpCN);
			fclose(fpJP);
			::EnableWindow(GetDlgItem(ID_APP_ABOUT), TRUE);
			::EnableWindow(GetDlgItem(IDC_CHECK1), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON1), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON2), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON3), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON5), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON6), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON7), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON8), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON10), TRUE);
			return 0;
		}
		memset(pnLen, 0, nItemAll*sizeof(int));
		pcstrText=new CString[nItemAll];
		if(0xfeff!=fgetwc(fpJP))
		{
			MessageBox(_T("The text file isn't unicode type."), m_tcsTitle);
			free(pnLen);
			delete [] pcstrText;
			fclose(fpCN);
			fclose(fpJP);
			::EnableWindow(GetDlgItem(ID_APP_ABOUT), TRUE);
			::EnableWindow(GetDlgItem(IDC_CHECK1), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON1), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON2), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON3), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON5), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON6), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON7), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON8), TRUE);
			::EnableWindow(GetDlgItem(IDC_BUTTON10), TRUE);
			return -1;
		}

		//填充pnLen和pcstrText数组
		cstrBuffer.Empty();
		while (NULL!=fgetws(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpJP))
		{
			cstrBuffer.ReleaseBuffer();
			if (0==cstrBuffer.Compare(_T("\r\n")))//若是空行则跳过
			{
				cstrBuffer.Empty();
				continue;
			}
			cstrBuffer.TrimRight(_T("\r\n"));
			nPos=cstrBuffer.Find(_T(','));
			if (-1==nPos)
			{
				cstrBuffer.Format(_T("%05d"), k+1);
				MessageBox(_T("The format of the text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
				free(pnLen);
				delete [] pcstrText;
				fclose(fpCN);
				fclose(fpJP);
				::EnableWindow(GetDlgItem(ID_APP_ABOUT), TRUE);
				::EnableWindow(GetDlgItem(IDC_CHECK1), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON1), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON2), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON3), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON5), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON6), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON7), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON8), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON10), TRUE);
				return 0;
			}
			cstrBuffer=cstrBuffer.Mid(nPos+1);
			nPos=cstrBuffer.Find(L',');
			if (-1==nPos)
			{
				cstrBuffer.Format(_T("%05d"), k+1);
				MessageBox(_T("The format of the text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
				free(pnLen);
				delete [] pcstrText;
				fclose(fpCN);
				fclose(fpJP);
				::EnableWindow(GetDlgItem(ID_APP_ABOUT), TRUE);
				::EnableWindow(GetDlgItem(IDC_CHECK1), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON1), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON2), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON3), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON5), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON6), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON7), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON8), TRUE);
				::EnableWindow(GetDlgItem(IDC_BUTTON10), TRUE);
				return 0;
			}
			pnLen[k]=_tstoi(cstrBuffer.Left(nPos));
			pcstrText[k]=cstrBuffer.Mid(nPos+1);
			k++;
		}

		nLastRowNo=m_nLastRowNo;//因为m_nLastRowNo在遇到已存在译文时会改变故必须事先存入另一个变量
		//开始比较重复文本
		for (int i=0; nLastRowNo+i<=nItemAll; i++)
		{
			cstrNew=pcstrText[nLastRowNo+i-1];
			nLenNew=pnLen[nLastRowNo+i-1];
			for (int j=1; j<nLastRowNo; j++)
			{
				cstrOld=pcstrText[j-1];
				nLenOld=pnLen[j-1];
				if (nLenNew==nLenOld)
				{
					if (cstrNew==cstrOld)
					{
						//用找到的第一句重复文本的非空译文来翻译
						cstrOld.Empty();
						GetTextFromFile(fpCN, j, cstrOld);
						if (cstrOld.IsEmpty())
							continue;
						else
						{
							cstrNew.Empty();
							GetTextFromFile(fpCN, nLastRowNo+i, cstrNew);
							if (cstrNew.IsEmpty())
							{
								SaveTextIn(fpCN, nLastRowNo+i, cstrOld);
								nTran++;
							}
							else if (!bAllCancel)
							{
								CString cstrID;
								cstrID.Format(_T("%d"), nLastRowNo+i);
								//Call JS
								if ( m_CallScript.DocumentSet() == FALSE)
								{
									IDispatch* d = NULL;
									m_pWB2->get_Document(&d);
									m_CallScript.SetDocument(d);
									d->Release();
								}
								m_CallScript.Run(L"monitor_core", (LPCTSTR)cstrID);

								int nResult=MessageBox(_T("第")+cstrID+_T("句译文已存在，是否要覆盖？")+\
														_T("\n是：覆盖该句。否：不覆盖。取消：当前句后找到的已存在译文的句子均作不覆盖处理。")+\
														_T("=============================================================================\n")+\
														_T("用于覆盖的句子：")+cstrOld+\
														_T("\n将被覆盖的句子：")+cstrNew, m_tcsTitle, MB_ICONWARNING | MB_YESNOCANCEL);
								if (IDYES==nResult)
								{
									SaveTextIn(fpCN, nLastRowNo+i, cstrOld);
									nOver++;
								}
								else if (IDNO==nResult)
									nCancel++;
								else if(IDCANCEL==nResult)
								{
									bAllCancel=true;
									nCancel++;
								}
							}
							else
								nCancel++;
							break;
						}
					}
				}
			}
		}

		/*end = ::GetTickCount();
		CString cstrTime;
		cstrTime.Format(_T("%d"), end-start);
		m_CInfoList.AddString(_T("==============="));
		m_CInfoList.AddString(_T("翻译重复文本耗时:")+cstrTime+_T("ms"));
		m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);*/

		m_nLastRowNo=nLastRowNo;//重设为最初的当前项
		free(pnLen);
		delete [] pcstrText;
		fclose(fpCN);
		fclose(fpJP);

		cstrBuffer.Empty();
		cstrBuffer.Format(_T("重复文本翻译完毕！\n翻译%d句。\n覆盖%d句。\n未处理%d句。"), nTran, nOver, nCancel);
		MessageBox(cstrBuffer, m_tcsTitle, MB_ICONINFORMATION);

		//更新预览窗口
		m_bShouldScrol=true;
		m_CFileList.SetFocus();
		//模拟用户按下了空格键
		::keybd_event(VK_SPACE, 0, 0, 0);
		::keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
	}
	::EnableWindow(GetDlgItem(ID_APP_ABOUT), TRUE);
	::EnableWindow(GetDlgItem(IDC_CHECK1), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON1), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON2), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON3), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON5), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON6), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON7), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON8), TRUE);
	::EnableWindow(GetDlgItem(IDC_BUTTON10), TRUE);
	return 0;
}

bool CMainDlg::SaveTextIn(FILE *fpOut, int nRowNo, CString &cstrOutFile)
{
	CString cstrBuffer, cstrAdd, cstrLen;
	int nNo=0, nPos, nStrLen;
	DWORD dwFilePos, dwBlockSize;
	BYTE *byTemp=NULL;

	if(0xfeff!=fgetwc(fpOut))
	{
		MessageBox(_T("The cn-text file isn't unicode type."), m_tcsTitle);
		return false;
	}
	while (NULL!=fgetws(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpOut))
	{
		cstrBuffer.ReleaseBuffer();
		if (0==cstrBuffer.Compare(_T("\r\n")))//若是空行则跳过
		{
			cstrBuffer.Empty();
			continue;
		}
		nNo++;
		if (nNo<nRowNo)//若还未到当前行内容时则跳过
		{
			cstrBuffer.Empty();
			continue;
		}
		nStrLen=cstrBuffer.GetLength();
		dwFilePos=ftell(fpOut);
		fseek(fpOut, 0L, 2);
		dwBlockSize=ftell(fpOut)-dwFilePos;
		byTemp=(BYTE*)malloc(dwBlockSize);
		if (NULL!=byTemp)
		{
			fseek(fpOut, dwFilePos, 0);
			fread(byTemp, dwBlockSize, 1, fpOut);

			//获取地址，长度
			cstrBuffer.TrimRight(_T("\r\n"));
			nPos=cstrBuffer.Find(_T(','));
			if (-1==nPos)
			{
				cstrBuffer.Format(_T("%05d"), nNo);
				MessageBox(_T("The format of the cn-text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
				return false;
			}
			cstrAdd=cstrBuffer.Left(nPos);
			cstrBuffer=cstrBuffer.Mid(nPos+1);
			nPos=cstrBuffer.Find(L',');
			if (-1==nPos)
			{
				cstrBuffer.Format(_T("%05d"), nNo);
				MessageBox(_T("The format of the cn-text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
				return false;
			}
			cstrLen=cstrBuffer.Left(nPos);

			cstrBuffer.Empty();
			cstrBuffer=cstrAdd+_T(",")+cstrLen+_T(",")+cstrOutFile+_T("\r\n");
			fseek(fpOut, dwFilePos-nStrLen*sizeof(TCHAR), 0);
			fwrite(cstrBuffer, cstrBuffer.GetLength()*sizeof(TCHAR), 1, fpOut);
			fwrite(byTemp, dwBlockSize, 1, fpOut);
			dwFilePos=ftell(fpOut);
			_chsize_s(_fileno(fpOut), (int)dwFilePos);
		}
		else
		{
			MessageBox(_T("There is insufficient memory available when save file!"), m_tcsTitle);
			return false;
		}
		break;
	}
	rewind(fpOut);
	free(byTemp);
	byTemp=NULL;
	return true;
}
LRESULT CMainDlg::OnBnClickedButton5(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	m_wndIE.SetFocus();
	::keybd_event(VK_CONTROL, 0 , WM_KEYDOWN, 0); 
	::keybd_event('F', 0, WM_KEYDOWN, 0); 
	::keybd_event('F', 0, KEYEVENTF_KEYUP, 0); 
	::keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0); 
	return 0;
}

LRESULT CMainDlg::OnBnClickedButton6(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here

	// FindDialog必须用new的方法，否则就得自己继承并用一个空函数重载OnFinalMessage
	if (NULL!=FR_lpDlg)
	{
		FR_lpDlg->SetActiveWindow();
		return 0;
	}
	FR_lpDlg = new CFindReplaceDialog;
	FR_lpDlg->m_fr.hInstance=GetModuleHandle(NULL);
	FR_lpDlg->m_fr.lpTemplateName=MAKEINTRESOURCE(1541);
	FR_lpDlg->Create(
		FALSE,							// TRUE for Find, FALSE for FindReplace
		_T(""),							// 初始要找的字符串
		NULL,							// 初始替换的字符串
										// 对话框标记，参考MSDN里的FINDREPLACE
		FR_ENABLETEMPLATE | FR_MATCHCASE | FR_HIDEUPDOWN | FR_HIDEWHOLEWORD,
		*this);							// Owner窗体
	FR_lpDlg->ShowWindow(SW_NORMAL);

	//构造pcstrText数组
	CString cstrBuffer;
	int nPos, nIndex, k=0;
	LVFINDINFO lvfi;
	TCHAR tszFileName[MAX_PATH], tszItemAll[MAX_PATH];
	FILE *fpCN;
	errno_t err;

	nPos=m_cstrCurrJP.ReverseFind(_T('\\'));
	cstrBuffer=m_cstrCurrJP.Right(m_cstrCurrJP.GetLength()-nPos-1);
	memset(&lvfi, 0, sizeof(LVFINDINFO));
	lvfi.flags=LVFI_STRING;
	lvfi.psz=cstrBuffer;
	nIndex=m_CFileList.FindItem(&lvfi, -1);
	if (-1!=nIndex)
	{
		m_CFileList.GetItemText(nIndex, 0, tszFileName, MAX_PATH);
		m_CFileList.GetItemText(nIndex, 2, tszItemAll, MAX_PATH);
		FR_nItemAll=_tstoi(tszItemAll);

		cstrBuffer=m_cstrCurrJP;
		cstrBuffer.Replace(_T("\\jp-text"), _T("\\cn-text"));
		if((err=_tfopen_s(&fpCN, cstrBuffer, _T("rb")))!=NULL)
		{
			m_CInfoList.AddString(_T("==============="));
			m_CInfoList.AddString(_T("无译文文本文件！"));
			m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);
			FR_lpDlg->DestroyWindow();
			FR_lpDlg=NULL;
			return -1;
		}
		if(0xfeff!=fgetwc(fpCN))
		{
			MessageBox(_T("The text file isn't unicode type."), m_tcsTitle);
			fclose(fpCN);
			FR_lpDlg->DestroyWindow();
			FR_lpDlg=NULL;
			return -1;
		}
		//填充m_lpcstrText数组
		FR_lpcstrText=new CString[FR_nItemAll];
		cstrBuffer.Empty();
		while (NULL!=fgetws(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpCN))
		{
			cstrBuffer.ReleaseBuffer();
			if (0==cstrBuffer.Compare(_T("\r\n")))//若是空行则跳过
			{
				cstrBuffer.Empty();
				continue;
			}
			if (k>=FR_nItemAll)
			{
				MessageBox(_T("The item of cn-text is bigger than jp-text!"), m_tcsTitle);
				delete [] FR_lpcstrText;
				FR_lpcstrText=NULL;
				fclose(fpCN);
				FR_lpDlg->DestroyWindow();
				FR_lpDlg=NULL;
				return -1;
			}
			cstrBuffer.TrimRight(_T("\r\n"));
			nPos=cstrBuffer.Find(_T(','));
			if (-1==nPos)
			{
				cstrBuffer.Format(_T("%05d"), k+1);
				MessageBox(_T("The format of the text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
				delete [] FR_lpcstrText;
				FR_lpcstrText=NULL;
				fclose(fpCN);
				FR_lpDlg->DestroyWindow();
				FR_lpDlg=NULL;
				return -1;
			}
			cstrBuffer=cstrBuffer.Mid(nPos+1);
			nPos=cstrBuffer.Find(L',');
			if (-1==nPos)
			{
				cstrBuffer.Format(_T("%05d"), k+1);
				MessageBox(_T("The format of the text is wrong!\nAt No.")+cstrBuffer, m_tcsTitle);
				delete [] FR_lpcstrText;
				FR_lpcstrText=NULL;
				fclose(fpCN);
				FR_lpDlg->DestroyWindow();
				FR_lpDlg=NULL;
				return -1;
			}
			FR_lpcstrText[k]=cstrBuffer.Mid(nPos+1);
			k++;
		}
		fclose(fpCN);
	}
	return 0;
}

LRESULT CMainDlg::OnFindReplaceMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	// 从lParam里取回CFindReplaceDialog对象
	CFindReplaceDialog *lpFRDlg = CFindReplaceDialog::GetNotifier(lParam);
	if (lpFRDlg)
	{
		CString cstrFind = lpFRDlg->GetFindString();//查找串
		CString cstrReplace = lpFRDlg->GetReplaceString();//替换串
		if (!m_bDocDone)
			return 0;

		if(lpFRDlg->IsTerminating())// 正在退出
		{
			FR_nRow=0;
			FR_nPos=0;
			FR_nLastRow=0;
			FR_nLastPos=0;
			lpFRDlg=NULL;
			FR_lpDlg=NULL;
			if (NULL!=FR_lpcstrText)
			{
				delete [] FR_lpcstrText;
				FR_lpcstrText=NULL;
			}
			return 0;
		}

		if(lpFRDlg->FindNext())// 按了FindNext按钮
		{
			if (NULL!=FR_lpcstrText)
			{
				CString cstrID;
				for (; FR_nRow<FR_nItemAll; FR_nRow++)
				{
					if (FR_bNextPage)
					{
						FR_nRow=(m_CTrackBar.GetPos()-1)*ini_nItemPerPage;
						FR_nPos=0;
						FR_bNextPage=false;
					}
					FR_nPos=Sunday(FR_lpcstrText[FR_nRow], cstrFind, !(lpFRDlg->MatchCase()), FR_nPos);
					if (FR_nPos>=0)
					{
						int nCurPage;
						nCurPage=1+FR_nRow/ini_nItemPerPage;
						if (nCurPage!=m_CTrackBar.GetPos())
						{
							m_bDocDone=false;
							FR_bNextPage=true;
							cstrID.Format(_T("请等待第%d页完全打开后再按\"查找下一个\"按钮。"), nCurPage);
							::MessageBox(lpFRDlg->m_hWnd, cstrID, m_tcsTitle, MB_ICONINFORMATION);
							m_CTrackBar.SetPos(nCurPage);
							m_CFileList.SetFocus();
							//模拟用户按下了空格键
							::keybd_event(VK_SPACE, 0, 0, 0);
							::keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
							return 0;
						}
						cstrID.Format(_T("%d"), FR_nRow+1);
						//Call JS
						if ( m_CallScript.DocumentSet() == FALSE)
						{
							IDispatch* d = NULL;
							m_pWB2->get_Document(&d);
							m_CallScript.SetDocument(d);
							d->Release();
						}
						m_CallScript.Run(L"monitor_core", (LPCTSTR)cstrID);
						m_CRichEdit2.SetSel(FR_nPos, FR_nPos+cstrFind.GetLength());
						FR_nPos+=cstrFind.GetLength();
						return 0;
					}
					else
						FR_nPos=0;
				}
				FR_nPos=0;
				FR_nRow=0;
				FR_bNextPage=false;
				::MessageBox(lpFRDlg->m_hWnd, _T("已经搜索至文件尾部！"), m_tcsTitle, MB_ICONINFORMATION);
			}
			return 0;
		}

		if(lpFRDlg->ReplaceCurrent())// 按了Replace按钮
		{
			if (NULL!=FR_lpcstrText)
			{
				CString cstrID, cstrSelText;
				m_CRichEdit2.GetSelText(cstrSelText.GetBuffer(ini_nTextBufLen));
				cstrSelText.ReleaseBuffer();
				if (cstrSelText==cstrFind)
				{
					m_CRichEdit2.ReplaceSel(cstrReplace);
					m_bTextIsDirty=true;
					FR_lpcstrText[FR_nLastRow]=FR_lpcstrText[FR_nLastRow].Left(FR_nLastPos-cstrFind.GetLength())+cstrReplace+FR_lpcstrText[FR_nLastRow].Mid(FR_nLastPos);
				}
				for (; FR_nRow<FR_nItemAll; FR_nRow++)
				{
					if (FR_bNextPage)
					{
						FR_nRow=(m_CTrackBar.GetPos()-1)*ini_nItemPerPage;
						FR_nPos=0;
						FR_bNextPage=false;
					}
					FR_nPos=Sunday(FR_lpcstrText[FR_nRow], cstrFind, !(lpFRDlg->MatchCase()), FR_nPos);
					if (FR_nPos>=0)
					{
						int nCurPage;
						nCurPage=1+FR_nRow/ini_nItemPerPage;
						if (nCurPage!=m_CTrackBar.GetPos())
						{
							m_bDocDone=false;
							FR_bNextPage=true;
							cstrID.Format(_T("请等待第%d页完全打开后再按\"替换\"按钮。"), nCurPage);
							::MessageBox(lpFRDlg->m_hWnd, cstrID, m_tcsTitle, MB_ICONINFORMATION);
							m_CTrackBar.SetPos(nCurPage);
							m_CFileList.SetFocus();
							//模拟用户按下了空格键
							::keybd_event(VK_SPACE, 0, 0, 0);
							::keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
							return 0;
						}
						cstrID.Format(_T("%d"), FR_nRow+1);
						//Call JS
						if ( m_CallScript.DocumentSet() == FALSE)
						{
							IDispatch* d = NULL;
							m_pWB2->get_Document(&d);
							m_CallScript.SetDocument(d);
							d->Release();
						}
						m_CallScript.Run(L"monitor_core", (LPCTSTR)cstrID);
						m_CRichEdit2.SetSel(FR_nPos, FR_nPos+cstrFind.GetLength());
						FR_nPos+=cstrFind.GetLength();
						FR_nLastPos=FR_nPos;
						FR_nLastRow=FR_nRow;
						return 0;
					}
					else
						FR_nPos=0;
				}
				//update preview for last row
				cstrID.Format(_T("%d"), FR_nLastRow+1);
				//Call JS
				if ( m_CallScript.DocumentSet() == FALSE)
				{
					IDispatch* d = NULL;
					m_pWB2->get_Document(&d);
					m_CallScript.SetDocument(d);
					d->Release();
				}
				m_CallScript.Run(L"monitor_core", (LPCTSTR)cstrID);
				FR_nPos=0;
				FR_nRow=0;
				FR_nLastRow=0;
				FR_nLastPos=0;
				FR_bNextPage=false;
				::MessageBox(lpFRDlg->m_hWnd, _T("已经搜索至文件尾部！"), m_tcsTitle, MB_ICONINFORMATION);
			}
			return 0;
		}

		if(lpFRDlg->ReplaceAll())// 按了ReplaceAll按钮
		{
			start = GetTickCount();
			m_CInfoList.AddString(_T("==============="));
			m_CInfoList.AddString(_T("开始替换译文..."));
			m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);

			CString cstrBuffer;
			FILE *fpCN;
			errno_t err;
			int nSize, nLastPos=0, i=0;
			cstrBuffer=m_cstrCurrJP;
			cstrBuffer.Replace(_T("\\jp-text"), _T("\\cn-text"));
			if((err=_tfopen_s(&fpCN, cstrBuffer, _T("rb+")))!=NULL)
			{
				m_CInfoList.AddString(_T("==============="));
				m_CInfoList.AddString(_T("无译文文本文件！"));
				m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);
				return -1;
			}
			if(0xfeff!=fgetwc(fpCN))
			{
				MessageBox(_T("The text file isn't unicode type."), m_tcsTitle);
				fclose(fpCN);
				return -1;
			}
			fseek(fpCN, 0L, 2);
			nSize=(int)ftell(fpCN);
			rewind(fpCN);
			cstrBuffer.Empty();
			fread(cstrBuffer.GetBuffer(nSize/sizeof(TCHAR)), nSize, 1, fpCN);
			cstrBuffer.ReleaseBuffer();
			TCHAR *lptch;
			lptch=cstrBuffer.GetBuffer(nSize/sizeof(TCHAR)+1);
			cstrBuffer.ReleaseBuffer();
			lptch[nSize/sizeof(TCHAR)]=_T('\0');
			rewind(fpCN);
			FR_nPos=0;
			while (FR_nPos<(int)(nSize/sizeof(TCHAR)))
			{
				FR_nPos=Sunday(cstrBuffer, cstrFind, !(lpFRDlg->MatchCase()), FR_nPos);
				if (FR_nPos>=0)
				{
					fwrite((LPCTSTR)cstrBuffer+nLastPos, (FR_nPos-nLastPos)*sizeof(TCHAR), 1, fpCN);
					fwrite(cstrReplace, cstrReplace.GetLength()*sizeof(TCHAR), 1, fpCN);
					FR_nPos+=cstrFind.GetLength();
					nLastPos=FR_nPos;
					i++;
				}
				else
				{
					fwrite((LPCTSTR)cstrBuffer+nLastPos, nSize-nLastPos*sizeof(TCHAR), 1, fpCN);
					nSize=ftell(fpCN);
					_chsize_s(_fileno(fpCN), nSize);
					break;
				}
			}
			fclose(fpCN);

			end = ::GetTickCount();
			cstrBuffer.Format(_T("%d"), end-start);
			m_CInfoList.AddString(_T("替换译文耗时:")+cstrBuffer+_T("ms"));
			m_CInfoList.SetCurSel(m_CInfoList.GetCount()-1);

			cstrBuffer.Format(_T("替换译文完毕！共替换%d处"), i);
			MessageBox(cstrBuffer, m_tcsTitle, MB_ICONINFORMATION);
			m_CFileList.SetFocus();
			//模拟用户按下了空格键
			::keybd_event(VK_SPACE, 0, 0, 0);
			::keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
			return 0;
		}
	}
	return 0;
}

int CMainDlg::Sunday(LPCTSTR lpctS, LPCTSTR lpctT, BOOL bNoCase, int nPos)
{
	int i, nSize;
	int nLenS, nLenT;
	CString cstrS, cstrT, cstrSTemp;
	int *nNext;//nNext数组，预处理初始化

	cstrS=lpctS;
	cstrT=lpctT;
	nLenS=cstrS.GetLength();
	nLenT=cstrT.GetLength();

	if(sizeof(TCHAR)==sizeof(char))
		nSize=0x100;
	else
		nSize=0x10000;
	nNext=(int*)malloc(nSize*sizeof(int));
	if(NULL==nNext)
		return -2;
	memset(nNext, 0, nSize*sizeof(int));

	for(i=0; i<nSize; i++)//初始化nNext数组
		nNext[i]=nLenT+1;
	for(i=0; i<nLenT; i++)//设置nNext数组
		nNext[cstrT.GetAt(i)]=nLenT-i;

	while( nPos<(nLenS-nLenT) )//遍历原串
	{
		cstrSTemp=cstrS.Mid(nPos, nLenT);
		if (bNoCase)
		{
			if (cstrSTemp.CompareNoCase(cstrT))//一旦不匹配，原串就按照nNext跳转
				nPos+=nNext[cstrS.GetAt(nPos+nLenT)];
			else
			{
				free(nNext);
				nNext=NULL;
				return nPos;
			}
		}
		else
		{
			if (cstrSTemp.Compare(cstrT))//一旦不匹配，原串就按照nNext跳转
				nPos+=nNext[cstrS.GetAt(nPos+nLenT)];
			else
			{
				free(nNext);
				nNext=NULL;
				return nPos;
			}
		}
	}
	if (nLenS-nLenT==nPos)//比较最后一次
	{
		cstrSTemp=cstrS.Mid(nPos, nLenT);
		if (bNoCase)
		{
			if (!cstrSTemp.CompareNoCase(cstrT))
			{
				free(nNext);
				nNext=NULL;
				return nPos;
			}
		}
		else
		{
			if (!cstrSTemp.Compare(cstrT))
			{
				free(nNext);
				nNext=NULL;
				return nPos;
			}
		}
	}
	free(nNext);
	nNext=NULL;
	return -1;//无子串则返回-1
}
LRESULT CMainDlg::OnBnClickedButton3(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CComVariant v;  // empty variant
	CString cstrResHTML;
	GetModuleFileName((HMODULE)GetWindowLong(GWL_HINSTANCE), cstrResHTML.GetBuffer(MAX_PATH), MAX_PATH);
	cstrResHTML.ReleaseBuffer();
	cstrResHTML=_T("res://")+cstrResHTML+_T("/208");
	start = ::GetTickCount();
	m_pWB2->Navigate ( CComBSTR(cstrResHTML), &v, &v, &v, &v );
	return 0;
}

LRESULT APIENTRY RichEditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (WM_SIZE==uMsg)
	{
		::SendMessage(g_hWndMain, WM_USER+1, wParam, lParam);
	}
	return CallWindowProc(g_wpOrigRichEditProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CMainDlg::OnIESize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RECT rcIE, rcSplit;

	m_wndHorSplit.GetWindowRect(&rcSplit);
	ScreenToClient(&rcSplit);
	if (rcSplit.left!=rcSplit.right)
	{
		m_nSpliterPos=m_wndHorSplit.GetSplitterPos();
		rcIE.left=rcSplit.left;
		rcIE.right=rcSplit.right;
		rcIE.top=rcSplit.top;
		rcIE.bottom=rcSplit.top+m_nSpliterPos;
		m_wndIE.SetWindowPos(NULL, &rcIE, SWP_NOMOVE | SWP_NOZORDER);
		//计算分割条位置百分比
		int nTotal=m_wndHorSplit.m_rcSplitter.bottom-m_wndHorSplit.m_rcSplitter.top-m_wndHorSplit.m_cxySplitBar-m_wndHorSplit.m_cxyBarEdge;
		m_nSpliterPer=::MulDiv(m_nSpliterPos, 100, nTotal);
	}
	return TRUE;
}
LRESULT CMainDlg::OnBnClickedButton9(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	RECT rcIE, rcSplit, rcCheck, rcButton7, rcButton8, rcButton9, rcStatic, rcSlider;

	if (m_bCtrlHide)
	{
		m_CFileList.ShowWindow(SW_SHOW);
		m_CInfoList.ShowWindow(SW_SHOW);
		::ShowWindow(GetDlgItem(IDC_BUTTON1), SW_SHOW);
		::ShowWindow(GetDlgItem(IDC_BUTTON2), SW_SHOW);
		::ShowWindow(GetDlgItem(IDC_BUTTON3), SW_SHOW);
		::ShowWindow(GetDlgItem(IDC_BUTTON4), SW_SHOW);
		::ShowWindow(GetDlgItem(IDC_BUTTON5), SW_SHOW);
		::ShowWindow(GetDlgItem(IDC_BUTTON6), SW_SHOW);
		::ShowWindow(GetDlgItem(ID_APP_ABOUT), SW_SHOW);
		::ShowWindow(GetDlgItem(IDC_BUTTON10), SW_SHOW);
		::SetWindowText(GetDlgItem(IDC_BUTTON9), _T("折叠↑↑"));
		m_bCtrlHide=false;
		//IE
		m_wndIE.GetWindowRect(&rcIE);
		ScreenToClient(&rcIE);
		rcIE.top=m_rcMarginIE.top;
		m_wndIE.SetWindowPos(NULL, &rcIE, SWP_NOZORDER);
		//Split
		m_wndHorSplit.GetWindowRect(&rcSplit);
		ScreenToClient(&rcSplit);
		rcSplit.top=m_rcMarginIE.top;
		m_wndHorSplit.SetWindowPos(NULL, &rcSplit, SWP_NOZORDER);
		m_wndHorSplit.SetSplitterPos(rcIE.bottom-rcIE.top);
		//Check
		::GetWindowRect(GetDlgItem(IDC_CHECK1), &rcCheck);
		ScreenToClient(&rcCheck);
		::SetWindowPos(GetDlgItem(IDC_CHECK1), NULL, rcCheck.left, m_rcNorPosButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		//Button7
		::GetWindowRect(GetDlgItem(IDC_BUTTON7), &rcButton7);
		ScreenToClient(&rcButton7);
		::SetWindowPos(GetDlgItem(IDC_BUTTON7), NULL, rcButton7.left, m_rcNorPosButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		//Button8
		::GetWindowRect(GetDlgItem(IDC_BUTTON8), &rcButton8);
		ScreenToClient(&rcButton8);
		::SetWindowPos(GetDlgItem(IDC_BUTTON8), NULL, rcButton8.left, m_rcNorPosButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		//Button9
		::GetWindowRect(GetDlgItem(IDC_BUTTON9), &rcButton9);
		ScreenToClient(&rcButton9);
		::SetWindowPos(GetDlgItem(IDC_BUTTON9), NULL, rcButton9.left, m_rcNorPosButton.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		//Static
		::GetWindowRect(GetDlgItem(IDC_STATIC_PAGE), &rcStatic);
		ScreenToClient(&rcStatic);
		rcStatic.top=m_rcNorPosStatic.top;
		::SetWindowPos(GetDlgItem(IDC_STATIC_PAGE), NULL, rcStatic.left, rcStatic.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		//Slider
		m_CTrackBar.GetWindowRect(&rcSlider);
		ScreenToClient(&rcSlider);
		rcSlider.top=m_rcNorPosSlider.top;
		m_CTrackBar.SetWindowPos(NULL, rcSlider.left, rcSlider.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
	}
	else
	{
		m_CFileList.ShowWindow(SW_HIDE);
		m_CInfoList.ShowWindow(SW_HIDE);
		::ShowWindow(GetDlgItem(IDC_BUTTON1), SW_HIDE);
		::ShowWindow(GetDlgItem(IDC_BUTTON2), SW_HIDE);
		::ShowWindow(GetDlgItem(IDC_BUTTON3), SW_HIDE);
		::ShowWindow(GetDlgItem(IDC_BUTTON4), SW_HIDE);
		::ShowWindow(GetDlgItem(IDC_BUTTON5), SW_HIDE);
		::ShowWindow(GetDlgItem(IDC_BUTTON6), SW_HIDE);
		::ShowWindow(GetDlgItem(ID_APP_ABOUT), SW_HIDE);
		::ShowWindow(GetDlgItem(IDC_BUTTON10), SW_HIDE);
		::SetWindowText(GetDlgItem(IDC_BUTTON9), _T("恢复↓↓"));
		m_bCtrlHide=true;
		//IE
		m_wndIE.GetWindowRect(&rcIE);
		ScreenToClient(&rcIE);
		rcIE.top=m_rcMarginIE.top-m_rcNorPosButton.top+m_nHidePos;
		m_wndIE.SetWindowPos(NULL, &rcIE, SWP_NOZORDER);
		//Split
		m_wndHorSplit.GetWindowRect(&rcSplit);
		ScreenToClient(&rcSplit);
		rcSplit.top=m_rcMarginIE.top-m_rcNorPosButton.top+m_nHidePos;
		m_wndHorSplit.SetWindowPos(NULL, &rcSplit, SWP_NOZORDER);
		m_wndHorSplit.SetSplitterPos(rcIE.bottom-rcIE.top);
		//Check
		::GetWindowRect(GetDlgItem(IDC_CHECK1), &rcCheck);
		ScreenToClient(&rcCheck);
		::SetWindowPos(GetDlgItem(IDC_CHECK1), NULL, rcCheck.left, m_nHidePos, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		//Button7
		::GetWindowRect(GetDlgItem(IDC_BUTTON7), &rcButton7);
		ScreenToClient(&rcButton7);
		::SetWindowPos(GetDlgItem(IDC_BUTTON7), NULL, rcButton7.left, m_nHidePos, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		//Button8
		::GetWindowRect(GetDlgItem(IDC_BUTTON8), &rcButton8);
		ScreenToClient(&rcButton8);
		::SetWindowPos(GetDlgItem(IDC_BUTTON8), NULL, rcButton8.left, m_nHidePos, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		//Button9
		::GetWindowRect(GetDlgItem(IDC_BUTTON9), &rcButton9);
		ScreenToClient(&rcButton9);
		::SetWindowPos(GetDlgItem(IDC_BUTTON9), NULL, rcButton9.left, m_nHidePos, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		//Static
		::GetWindowRect(GetDlgItem(IDC_STATIC_PAGE), &rcStatic);
		ScreenToClient(&rcStatic);
		rcStatic.top=m_nHidePos+m_rcNorPosStatic.top-m_rcNorPosButton.top;
		::SetWindowPos(GetDlgItem(IDC_STATIC_PAGE), NULL, rcStatic.left, rcStatic.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
		//Slider
		m_CTrackBar.GetWindowRect(&rcSlider);
		ScreenToClient(&rcSlider);
		rcSlider.top=m_nHidePos+m_rcNorPosSlider.top-m_rcNorPosButton.top;
		m_CTrackBar.SetWindowPos(NULL, rcSlider.left, rcSlider.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
	}
	::InvalidateRect(GetDlgItem(IDC_CHECK1), NULL, TRUE);
	::InvalidateRect(GetDlgItem(IDC_BUTTON7), NULL, TRUE);
	::InvalidateRect(GetDlgItem(IDC_BUTTON8), NULL, TRUE);
	::InvalidateRect(GetDlgItem(IDC_BUTTON9), NULL, TRUE);
	::InvalidateRect(GetDlgItem(IDC_STATIC_PAGE), NULL, TRUE);
	m_CTrackBar.Invalidate();
	return 0;
}

LRESULT CMainDlg::OnBnClickedButton7(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	errno_t err;
	FILE *fpJP, *fpCN;
	CString cstrJP, cstrText, cstrTextCN;
	bool bCN=false;
	int nTextLen;

	cstrJP=m_cstrCurrJP;
	if((err=_tfopen_s(&fpJP, cstrJP, _T("rb")))!=NULL)
	{
		MessageBox(_T("Open jp-text file failed!"), m_tcsTitle);
		return -1;
	}
	if(0xfeff!=fgetwc(fpJP))
	{
		MessageBox(_T("The jp-text file isn't unicode type."), m_tcsTitle);
		fclose(fpJP);
		return -1;
	}
	rewind(fpJP);
	cstrJP.Replace(_T("\\jp-text"), _T("\\cn-text"));
	if((err=_tfopen_s(&fpCN, cstrJP, _T("rb")))!=NULL)
		bCN=false;
	else
		bCN=true;
	if (bCN)
	{
		if(0xfeff!=fgetwc(fpCN))
		{
			MessageBox(_T("The cn-text file isn't unicode type."), m_tcsTitle);
			fclose(fpCN);
			bCN=false;
		}
	}
	rewind(fpCN);

	nTextLen=GetTextFromFile(fpJP, m_nLastRowNo, cstrText);
	if (nTextLen<=0)
		return -1; 

	nTextLen=GetTextFromFile(fpCN, m_nLastRowNo, cstrTextCN);
	if (nTextLen<=0)
		return -1;

	if (bCN)
	{
		if (cstrTextCN.Compare(_T("")))
		{
			if (IDYES==MessageBox(_T("是否覆盖译文？"), m_tcsTitle, MB_ICONWARNING | MB_YESNO))
			{
				m_CRichEdit2.SetWindowText(cstrText);
				FormatText(cstrText);
				m_bTextIsDirty=true;
			}
		}
		else
		{
			m_CRichEdit2.SetWindowText(cstrText);
			FormatText(cstrText);
			m_bTextIsDirty=true;
		}
	}
	else
	{
		m_CRichEdit2.SetWindowText(cstrText);
		FormatText(cstrText);
		m_bTextIsDirty=true;
	}
	fclose(fpJP);
	fclose(fpCN);
	UpdatePreview(m_nLastRowNo);
	return 0;
}

void __stdcall CMainDlg::DownloadCompleteExplorer1()
{
	// TODO: Add your message handler code here
	//载入页面或刷新页面完成后触发（第一次触发）
	if (!m_bHtmlChanged)
	{
		ShowHtmlComplete();
		start = ::GetTickCount();
	}
	m_bDocDone=true;
}

LRESULT CMainDlg::OnBnClickedButton8(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	errno_t err;
	FILE *fpComp, *fpCN;
	CString cstrComp, cstrTextComp, cstrTextCN;
	bool bCN=false;
	int nTextLen;

	cstrComp=m_cstrCurrJP;
	cstrComp.Replace(_T("\\jp-text"), _T("\\cn-compare"));

	if((err=_tfopen_s(&fpComp, cstrComp, _T("rb")))!=NULL)
	{
		MessageBox(_T("Open cn-compare file failed!"), m_tcsTitle);
		return -1;
	}
	if(0xfeff!=fgetwc(fpComp))
	{
		MessageBox(_T("The cn-compare file isn't unicode type."), m_tcsTitle);
		fclose(fpComp);
		return -1;
	}
	rewind(fpComp);
	cstrComp.Replace(_T("\\cn-compare"), _T("\\cn-text"));
	if((err=_tfopen_s(&fpCN, cstrComp, _T("rb")))!=NULL)
		bCN=false;
	else
		bCN=true;
	if (bCN)
	{
		if(0xfeff!=fgetwc(fpCN))
		{
			MessageBox(_T("The cn-text file isn't unicode type."), m_tcsTitle);
			fclose(fpCN);
			bCN=false;
		}
	}
	rewind(fpCN);

	nTextLen=GetTextFromFile(fpComp, m_nLastRowNo, cstrTextComp);
	if (nTextLen<=0)
		return -1; 

	nTextLen=GetTextFromFile(fpCN, m_nLastRowNo, cstrTextCN);
	if (nTextLen<=0)
		return -1;

	if (bCN)
	{
		if (cstrTextCN.Compare(_T("")))
		{
			if (IDYES==MessageBox(_T("是否覆盖译文？"), m_tcsTitle, MB_ICONWARNING | MB_YESNO))
			{
				m_CRichEdit2.SetWindowText(cstrTextComp);
				FormatText(cstrTextComp);
				m_bTextIsDirty=true;
			}
		}
		else
		{
			m_CRichEdit2.SetWindowText(cstrTextComp);
			FormatText(cstrTextComp);
			m_bTextIsDirty=true;
		}
	}
	else
	{
		m_CRichEdit2.SetWindowText(cstrTextComp);
		FormatText(cstrTextComp);
		m_bTextIsDirty=true;
	}
	fclose(fpComp);
	fclose(fpCN);
	UpdatePreview(m_nLastRowNo);
	return 0;
}

bool CMainDlg::TextFormatCheck(CString &cstrText, bool bShowMessage)
{
	int n, m;
	CString cstrBuffer;

	n=cstrText.Remove(_T('\r'));
	m=cstrText.Remove(_T('\n'));
	if ((n>0 || m>0) && bShowMessage)
	{
		cstrBuffer.Format(_T("该条目中含有的非法字符：\n%d个回车符(\\r)\n%d个换行符(\\n)\n已自动删除！"), n, m);
		MessageBox(cstrBuffer, m_tcsTitle, MB_ICONINFORMATION);
		return false;
	}
	return true;
}

LRESULT CMainDlg::OnSizing(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	LPRECT lpRect;
	lpRect=(LPRECT)lParam;
	if (lpRect->right-lpRect->left<MINWINWIDTH)
	{
		ScreenToClient(lpRect);
		lpRect->right=MINWINWIDTH;
		ClientToScreen(lpRect);
	}
	return TRUE;
}
LRESULT CMainDlg::OnBnClickedButton10(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CString cstrBuffer;

	cstrBuffer=m_cstrCurrJP;
	cstrBuffer.Replace(_T("\\jp-text"), _T("\\cn-text"));
	g_JumpData.lpctCN=cstrBuffer;
	g_JumpData.ini_nTextBufLen=ini_nTextBufLen;
	g_JumpData.lpctTitle=m_tcsTitle;
	g_JumpData.pWB2=m_pWB2;

	if(Jumpdlg.Create(g_hWndMain, (LPARAM)&g_JumpData) == NULL)
	{
		ATLTRACE(_T("Jump dialog creation failed!\n"));
		return 0;
	}
	Jumpdlg.ShowWindow(SW_SHOWDEFAULT);

	return 0;
}

