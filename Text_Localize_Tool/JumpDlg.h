#pragma once

extern HWND g_hWndJumpDlg;

typedef struct tagJUMPDATA
{
	int ini_nTextBufLen;
	LPCTSTR lpctCN;
	LPCTSTR lpctTitle;
	CComPtr<IWebBrowser2> pWB2;
} JUMPDATA, *LPJUMPDATA;

class CJumpDlg : public CDialogImpl<CJumpDlg>
{
public:
	enum { IDD = IDD_JUMPDLG };

	BEGIN_MSG_MAP(CJumpDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_HANDLER(IDC_BUTTON1, BN_CLICKED, OnBnClickedButton1)
		COMMAND_HANDLER(IDC_BUTTON2, BN_CLICKED, OnBnClickedButton2)
		COMMAND_HANDLER(IDC_LIST1, LBN_DBLCLK, OnLbnDblclkList1)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LPJUMPDATA m_lpJumpData;
	CListBox m_CNULLRowList;
	CCallScript m_CallScript;

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		CString cstrBuffer, cstrTextCN, cstrTitle;
		int ini_nTextBufLen, nNo=0, nPos, nIndex=0;
		FILE *fpCN;
		errno_t err;

		CenterWindow(GetParent());
		m_lpJumpData=(LPJUMPDATA)lParam;
		ini_nTextBufLen=m_lpJumpData->ini_nTextBufLen;
		cstrBuffer=m_lpJumpData->lpctCN;
		cstrTitle=m_lpJumpData->lpctTitle;
		m_CNULLRowList=GetDlgItem(IDC_LIST1);

		if(err=_tfopen_s(&fpCN, cstrBuffer, _T("rb"))!=NULL)
		{
			MessageBox(_T("Open cn-text file failed!"), cstrTitle);
			BOOL b;
			OnCloseCmd(NULL,-1, NULL, b);
			return TRUE;
		}
		if(0xfeff!=fgetwc(fpCN))
		{
			MessageBox(_T("The cn-text file isn't unicode type."), cstrTitle);
			fclose(fpCN);
			BOOL b;
			OnCloseCmd(NULL,-1, NULL, b);
			return TRUE;
		}

		while (NULL!=fgetws(cstrBuffer.GetBuffer(ini_nTextBufLen), ini_nTextBufLen, fpCN))
		{
			cstrBuffer.ReleaseBuffer();
			if (0==cstrBuffer.Compare(_T("\r\n")))//若是空行则跳过
			{
				cstrBuffer.Empty();
				continue;
			}
			nNo++;
			cstrBuffer.TrimRight(_T("\r\n"));
			nPos=cstrBuffer.Find(_T(','));
			if (-1==nPos)
			{
				cstrBuffer.Format(_T("%05d"), nNo);
				MessageBox(_T("The format of the cn-text is wrong!\nAt No.")+cstrBuffer, cstrTitle);
				fclose(fpCN);
				BOOL b;
				OnCloseCmd(NULL,-1, NULL, b);
				return TRUE;
			}
			cstrBuffer=cstrBuffer.Mid(nPos+1);
			nPos=cstrBuffer.Find(L',');
			if (-1==nPos)
			{
				cstrBuffer.Format(_T("%05d"), nNo);
				MessageBox(_T("The format of the cn-text is wrong!\nAt No.")+cstrBuffer, cstrTitle);
				fclose(fpCN);
				BOOL b;
				OnCloseCmd(NULL,-1, NULL, b);
				return TRUE;
			}
			cstrTextCN=cstrBuffer.Mid(nPos+1);
			if (_T("")==cstrTextCN)
			{
				cstrBuffer.Format(_T("第%05d行"), nNo);
				nIndex=m_CNULLRowList.AddString(cstrBuffer);
				m_CNULLRowList.SetItemData(nIndex, (DWORD)nNo);
			}
			cstrBuffer.Empty();
		}
		fclose(fpCN);
		return TRUE;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CComPtr<IWebBrowser2> pWB2;
		CString cstrID;
		int nRow;

		nRow=GetDlgItemInt(IDC_EDIT1);
		cstrID.Format(_T("%d"), nRow);
		pWB2=m_lpJumpData->pWB2;
		//Call JS
		if ( m_CallScript.DocumentSet() == FALSE)
		{
			IDispatch* d = NULL;
			pWB2->get_Document(&d);
			m_CallScript.SetDocument(d);
			d->Release();
		}
		m_CallScript.Run(L"monitor_core", (LPCTSTR)cstrID);
		return 0;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		DestroyWindow();
		g_hWndJumpDlg=NULL;
		return wID;
	}

	LRESULT OnBnClickedButton1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int nSelIndex, nAll;
		DWORD dwItemData;
		CComPtr<IWebBrowser2> pWB2;

		nSelIndex=m_CNULLRowList.GetCurSel();
		nAll=m_CNULLRowList.GetCount();
		if (0==nAll)
			return 0;
		if (nSelIndex<0)
			nSelIndex=0;
		else
			nSelIndex=(++nSelIndex)%nAll;
		ATLASSERT(nSelIndex<nAll);
		m_CNULLRowList.SetCurSel(nSelIndex);

		dwItemData=m_CNULLRowList.GetItemData(nSelIndex);
		if (dwItemData>0)
		{
			CString cstrID;
			cstrID.Format(_T("%d"), dwItemData);
			pWB2=m_lpJumpData->pWB2;
			//Call JS
			if ( m_CallScript.DocumentSet() == FALSE)
			{
				IDispatch* d = NULL;
				pWB2->get_Document(&d);
				m_CallScript.SetDocument(d);
				d->Release();
			}
			m_CallScript.Run(L"monitor_core", (LPCTSTR)cstrID);
		}
		return 0;
	}

	LRESULT OnBnClickedButton2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int nSelIndex, nAll;
		DWORD dwItemData;
		CComPtr<IWebBrowser2> pWB2;

		nSelIndex=m_CNULLRowList.GetCurSel();
		nAll=m_CNULLRowList.GetCount();
		if (0==nAll)
			return 0;
		if (nSelIndex<0)
			nSelIndex=nAll-1;
		else
			nSelIndex=((--nSelIndex)+nAll)%nAll;
		ATLASSERT(nSelIndex>=0);
		m_CNULLRowList.SetCurSel(nSelIndex);

		dwItemData=m_CNULLRowList.GetItemData(nSelIndex);
		if (dwItemData>0)
		{
			CString cstrID;
			cstrID.Format(_T("%d"), dwItemData);
			pWB2=m_lpJumpData->pWB2;
			//Call JS
			if ( m_CallScript.DocumentSet() == FALSE)
			{
				IDispatch* d = NULL;
				pWB2->get_Document(&d);
				m_CallScript.SetDocument(d);
				d->Release();
			}
			m_CallScript.Run(L"monitor_core", (LPCTSTR)cstrID);
		}
		return 0;
	}

	LRESULT OnLbnDblclkList1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int nSelIndex;
		DWORD dwItemData;
		CComPtr<IWebBrowser2> pWB2;

		nSelIndex=m_CNULLRowList.GetCurSel();
		dwItemData=m_CNULLRowList.GetItemData(nSelIndex);
		if (dwItemData>0)
		{
			CString cstrID;
			cstrID.Format(_T("%d"), dwItemData);
			pWB2=m_lpJumpData->pWB2;
			//Call JS
			if ( m_CallScript.DocumentSet() == FALSE)
			{
				IDispatch* d = NULL;
				pWB2->get_Document(&d);
				m_CallScript.SetDocument(d);
				d->Release();
			}
			m_CallScript.Run(L"monitor_core", (LPCTSTR)cstrID);
		}
		return 0;
	}
};
