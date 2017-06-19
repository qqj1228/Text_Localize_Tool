// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "CallScript.h"
#include "JumpDlg.h"

class CMainDlg : public CAxDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>,
	public CMessageFilter, public CIdleHandler,
	public IDispEventImpl<IDC_EXPLORER1,CMainDlg>
{
public:
	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		COMMAND_HANDLER(IDC_BUTTON2, BN_CLICKED, OnBnClickedButton2)
		NOTIFY_HANDLER(IDC_LIST2, NM_DBLCLK, OnNMDblclkList2)
		NOTIFY_HANDLER(IDC_SLIDER1, NM_RELEASEDCAPTURE, OnNMReleasedcaptureSlider)
		NOTIFY_HANDLER(IDC_LIST2, LVN_KEYDOWN, OnLvnKeydownList2)
		COMMAND_HANDLER(IDC_BUTTON1, BN_CLICKED, OnBnClickedButton1)
		MESSAGE_HANDLER(WM_PARENTNOTIFY, OnParentNotify)
		NOTIFY_HANDLER(IDC_RICHEDIT21, EN_PROTECTED, OnEnProtectedRichedit21)
		COMMAND_HANDLER(IDC_RICHEDIT21, EN_CHANGE, OnEnChangeRichedit21)
		NOTIFY_HANDLER(IDC_RICHEDIT21, EN_MSGFILTER, OnEnMsgfilterRichedit21)
		COMMAND_HANDLER(IDC_CHECK1, BN_CLICKED, OnBnClickedCheck1)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, OnCtlColorListbox)
		COMMAND_HANDLER(IDC_LIST1, LBN_DBLCLK, OnLbnDblclkList1)
		COMMAND_HANDLER(IDC_BUTTON4, BN_CLICKED, OnBnClickedButton4)
		COMMAND_HANDLER(IDC_BUTTON5, BN_CLICKED, OnBnClickedButton5)
		COMMAND_HANDLER(IDC_BUTTON6, BN_CLICKED, OnBnClickedButton6)
		MESSAGE_HANDLER(CFindReplaceDialog::GetFindReplaceMsg(), OnFindReplaceMsg)
		COMMAND_HANDLER(IDC_BUTTON3, BN_CLICKED, OnBnClickedButton3)
		MESSAGE_HANDLER(WM_USER+1, OnIESize)
		COMMAND_HANDLER(IDC_BUTTON9, BN_CLICKED, OnBnClickedButton9)
		COMMAND_HANDLER(IDC_BUTTON7, BN_CLICKED, OnBnClickedButton7)
		COMMAND_HANDLER(IDC_BUTTON8, BN_CLICKED, OnBnClickedButton8)
		MESSAGE_HANDLER(WM_SIZING, OnSizing)
		COMMAND_HANDLER(IDC_BUTTON10, BN_CLICKED, OnBnClickedButton10)
	END_MSG_MAP()

	BEGIN_SINK_MAP(CMainDlg)
		SINK_ENTRY(IDC_EXPLORER1, 259, DocumentCompleteExplorer1)
		SINK_ENTRY(IDC_EXPLORER1, 250, BeforeNavigate2Explorer1)
		SINK_ENTRY(IDC_EXPLORER1, 104, DownloadCompleteExplorer1)
	END_SINK_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void CloseDialog(int nVal);

	CAxWindow m_wndIE;
	CComPtr<IWebBrowser2> m_pWB2;
	CRichEditCtrl m_CRichEdit2;
	RECT m_rcMarginIE, m_rcMarginRE2;
	CListViewCtrl m_CFileList;
	CListBox m_CInfoList;
	CTrackBarCtrl m_CTrackBar;
	TCHAR m_tcsOriPath[MAX_PATH], m_tcsTitle[MAX_PATH];
	CString m_cstrTemp, m_cstrJPPath, m_cstrCurrJP, m_cstrCtrlEffect, m_cstrLastFileName, m_cstrCtrlLen;
	int m_nLastRowNo, m_nOverLenCount;
	bool m_bTextIsDirty, m_bShouldScrol, m_bDocDone, m_bCtrlHide, m_bHtmlChanged;
	IRichEditOleCallback* m_IRichEditOleCallback;
	BYTE *m_lpByteRow;
	CCallScript m_CallScript;
	CHorSplitterWindow  m_wndHorSplit;
	int m_nHidePos, m_nSpliterPos, m_nSpliterPer;
	RECT m_rcNorPosButton, m_rcNorPosStatic, m_rcNorPosSlider;
	CJumpDlg Jumpdlg;

	//FindReplaceDialog
	CString *FR_lpcstrText;
	CFindReplaceDialog *FR_lpDlg;
	int FR_nItemAll, FR_nRow/*以0开始*/, FR_nPos, FR_nLastRow/*以0开始*/, FR_nLastPos;
	bool FR_bNextPage;

	HRESULT hr;
	DWORD start, end;
	bool bFixFirstChar, bFixMidChar;
	int nCurPos;

	HRESULT LoadWebBrowserFromStream(IWebBrowser2* pWebBrowser, IStream* pStream);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	bool BuildHTML(LPCTSTR lpJP);
	LRESULT OnBnClickedButton2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	bool LoadHTMLResource(int nIDR, CString &cstrHTML);
	LRESULT OnNMDblclkList2(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
	void __stdcall DocumentCompleteExplorer1(LPDISPATCH pDisp, VARIANT* URL);
	int CountItem(LPCTSTR tszFile);
	LRESULT OnNMReleasedcaptureSlider(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
	LRESULT OnLvnKeydownList2(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
	bool PreBuildHTML(LPCTSTR lpctFileName, int nItemAll);
	LRESULT OnBnClickedButton1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	bool SetHTMLIDValue(LPCTSTR lpID, LPCTSTR lpValue);
	bool GetHTMLIDValue(LPCTSTR lpID, CString &cstrValue);
	bool GetInputValue(LPCTSTR lpInput, CString& cstrValue);
	bool SetInputValue(LPCTSTR lpInput, LPCTSTR lpValue);
	LRESULT OnParentNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	bool GetValueFromPt(int cx, int cy, CString &cstrValue);
	void __stdcall BeforeNavigate2Explorer1(LPDISPATCH pDisp, VARIANT* URL, VARIANT* Flags, VARIANT* TargetFrameName, VARIANT* PostData, VARIANT* Headers, BOOL* Cancel);
	bool ShowFormatText(int nRowNo);
	bool SaveText(int nRowNo);
	bool UpdatePreview(int m_nLastRowNo);
	bool SaveNewFile(LPCTSTR lpNewFile, int nRowNo);
	bool FormatText(LPCTSTR lpText);
	LRESULT OnEnProtectedRichedit21(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
	LRESULT OnEnChangeRichedit21(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnMsgfilterRichedit21(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
	bool RenderCtrl(CString& cstrText);
	bool RenderDiff(CString& cstrText, BYTE *lpMark, BOOL bFullShowDiff);
	bool RenderSpec(CString& cstrText);
	LRESULT OnBnClickedCheck1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	bool ReadINI();
	bool WriteINI();
	int GetTextFromFile(FILE *fpIn, int nRowNo, CString &cstrText);//返回值为文件中的文本长度, nRowNo是以1为基数
	bool Half2Full(CString &cstrText);
	int OverLen(int nLen, LPCTSTR lpText);
	LRESULT OnCtlColorListbox(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	long GetHTMLScrollTop();
	bool SetHTMLScrollTopLeft(long scroll_top, long scroll_left);
	long GetHTMLIDTop(LPCTSTR lpID);
	LRESULT OnLbnDblclkList1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	bool CtrlEffect(CString &cstrText);
	bool ReadCtrlEffect();
	bool ReadCtrlLen();
	int GetOutCtrlLen(LPCTSTR lpctCtrl);
	LRESULT OnBnClickedButton4(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	bool SaveTextIn(FILE *fpOut, int nRowNo, CString &cstrOutFile);
	LRESULT OnBnClickedButton5(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedButton6(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFindReplaceMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	int Sunday(LPCTSTR lpctS, LPCTSTR lpctT, BOOL bNoCase, int nPos);
	LRESULT OnBnClickedButton3(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnIESize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedButton9(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedButton7(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	bool ShowHtmlComplete();
	void __stdcall DownloadCompleteExplorer1();
	LRESULT OnBnClickedButton8(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	bool TextFormatCheck(CString &cstrText, bool bShowMessage);
	LRESULT OnSizing(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedButton10(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};

typedef struct tagIEDATA
{
	CComPtr<IWebBrowser2> m_pWB2;
	LPCTSTR lpTemp;
} IEDATA, *LPIEDATA;

unsigned int __stdcall ThreadProc(LPVOID lpParameter);
LRESULT APIENTRY RichEditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
