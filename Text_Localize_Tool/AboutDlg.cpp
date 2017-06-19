// aboutdlg.cpp : implementation of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"

CString ini_cstrWidthID, ini_cstrWidthJP, ini_cstrWidthCN, ini_cstrWidthCOM;
int ini_nBytePerChar, ini_nTextBufLen, ini_nItemPerPage, ini_nWriteFilePerTime, ini_nRE2FintSize;
BOOL ini_bEditCtrlchar, ini_bHalf2Full, ini_bSpliterPos, ini_bJPShowCNFont, ini_bCNShowDiff, ini_bOnePageSlider;
BOOL ini_bCNCOMNULL, ini_bOutCtrlLen;
CString ini_cstrClrNorText, ini_cstrClrCtrlText, ini_cstrClrDiffText, ini_cstrClrOverLenBg;
CString ini_cstrClrNorTr, ini_cstrClrSelTr, ini_cstrClrTabBorder, ini_cstrClrBodyBg, ini_cstrClrHiLiREBg;

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());

	//宽度
	SetDlgItemText(IDC_EDIT1, ini_cstrWidthID);
	SetDlgItemText(IDC_EDIT2, ini_cstrWidthJP);
	SetDlgItemText(IDC_EDIT3, ini_cstrWidthCN);
	SetDlgItemText(IDC_EDIT8, ini_cstrWidthCOM);

	//高级
	SetDlgItemInt(IDC_EDIT4, ini_nBytePerChar);
	SetDlgItemInt(IDC_EDIT5, ini_nTextBufLen);
	SetDlgItemInt(IDC_EDIT6, ini_nItemPerPage);
	SetDlgItemInt(IDC_EDIT7, ini_nWriteFilePerTime);
	if(ini_bEditCtrlchar)
		CheckDlgButton(IDC_CHECK2, BST_CHECKED);
	else
		CheckDlgButton(IDC_CHECK2, BST_UNCHECKED);
	if(ini_bHalf2Full)
		CheckDlgButton(IDC_CHECK3, BST_CHECKED);
	else
		CheckDlgButton(IDC_CHECK3, BST_UNCHECKED);
	if(ini_bSpliterPos)
		CheckDlgButton(IDC_CHECK4, BST_CHECKED);
	else
		CheckDlgButton(IDC_CHECK4, BST_UNCHECKED);
	if(ini_bJPShowCNFont)
		CheckDlgButton(IDC_CHECK5, BST_CHECKED);
	else
		CheckDlgButton(IDC_CHECK5, BST_UNCHECKED);
	if(ini_bCNShowDiff)
		CheckDlgButton(IDC_CHECK6, BST_CHECKED);
	else
		CheckDlgButton(IDC_CHECK6, BST_UNCHECKED);
	if(ini_bOnePageSlider)
		CheckDlgButton(IDC_CHECK7, BST_CHECKED);
	else
		CheckDlgButton(IDC_CHECK7, BST_UNCHECKED);
	SetDlgItemInt(IDC_EDIT9, ini_nRE2FintSize);
	if(ini_bCNCOMNULL)
		CheckDlgButton(IDC_CHECK8, BST_CHECKED);
	else
		CheckDlgButton(IDC_CHECK8, BST_UNCHECKED);
	if(ini_bOutCtrlLen)
		CheckDlgButton(IDC_CHECK9, BST_CHECKED);
	else
		CheckDlgButton(IDC_CHECK9, BST_UNCHECKED);

	//颜色
	CRE2Clr1=GetDlgItem(IDC_RICHEDIT21);
	IniClr2Str(ini_cstrClrNorText, clr1, false);
	CRE2Clr1.SetBackgroundColor(clr1);
	CRE2Clr2=GetDlgItem(IDC_RICHEDIT22);
	IniClr2Str(ini_cstrClrCtrlText, clr2, false);
	CRE2Clr2.SetBackgroundColor(clr2);
	CRE2Clr3=GetDlgItem(IDC_RICHEDIT23);
	IniClr2Str(ini_cstrClrDiffText, clr3, false);
	CRE2Clr3.SetBackgroundColor(clr3);
	CRE2Clr4=GetDlgItem(IDC_RICHEDIT24);
	IniClr2Str(ini_cstrClrOverLenBg, clr4, false);
	CRE2Clr4.SetBackgroundColor(clr4);
	CRE2Clr5=GetDlgItem(IDC_RICHEDIT25);
	IniClr2Str(ini_cstrClrNorTr, clr5, false);
	CRE2Clr5.SetBackgroundColor(clr5);
	CRE2Clr6=GetDlgItem(IDC_RICHEDIT26);
	IniClr2Str(ini_cstrClrSelTr, clr6, false);
	CRE2Clr6.SetBackgroundColor(clr6);
	CRE2Clr7=GetDlgItem(IDC_RICHEDIT27);
	IniClr2Str(ini_cstrClrTabBorder, clr7, false);
	CRE2Clr7.SetBackgroundColor(clr7);
	CRE2Clr8=GetDlgItem(IDC_RICHEDIT28);
	IniClr2Str(ini_cstrClrBodyBg, clr8, false);
	CRE2Clr8.SetBackgroundColor(clr8);
	CRE2Clr9=GetDlgItem(IDC_RICHEDIT29);
	IniClr2Str(ini_cstrClrHiLiREBg, clr9, false);
	CRE2Clr9.SetBackgroundColor(clr9);

	//恢复默认
	CCbbDefault=GetDlgItem(IDC_COMBO1);
	CCbbDefault.AddString(_T("全部"));
	CCbbDefault.AddString(_T("宽度"));
	CCbbDefault.AddString(_T("高级"));
	CCbbDefault.AddString(_T("颜色"));
	CCbbDefault.SetCurSel(0);
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (IDOK==wID)
	{
		//宽度
		GetDlgItemText(IDC_EDIT1, ini_cstrWidthID.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN);
		ini_cstrWidthID.ReleaseBuffer();
		GetDlgItemText(IDC_EDIT2, ini_cstrWidthJP.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN);
		ini_cstrWidthJP.ReleaseBuffer();
		GetDlgItemText(IDC_EDIT3, ini_cstrWidthCN.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN);
		ini_cstrWidthCN.ReleaseBuffer();
		GetDlgItemText(IDC_EDIT8, ini_cstrWidthCOM.GetBuffer(WIDTHBUFLEN), WIDTHBUFLEN);
		ini_cstrWidthCOM.ReleaseBuffer();

		//高级
		ini_nBytePerChar=GetDlgItemInt(IDC_EDIT4);
		ini_nTextBufLen=GetDlgItemInt(IDC_EDIT5);
		ini_nItemPerPage=GetDlgItemInt(IDC_EDIT6);
		ini_nWriteFilePerTime=GetDlgItemInt(IDC_EDIT7);
		if(BST_CHECKED==IsDlgButtonChecked(IDC_CHECK2))
			ini_bEditCtrlchar=TRUE;
		else
			ini_bEditCtrlchar=FALSE;
		if(BST_CHECKED==IsDlgButtonChecked(IDC_CHECK3))
			ini_bHalf2Full=TRUE;
		else
			ini_bHalf2Full=FALSE;
		if(BST_CHECKED==IsDlgButtonChecked(IDC_CHECK4))
			ini_bSpliterPos=TRUE;
		else
			ini_bSpliterPos=FALSE;
		if(BST_CHECKED==IsDlgButtonChecked(IDC_CHECK5))
			ini_bJPShowCNFont=TRUE;
		else
			ini_bJPShowCNFont=FALSE;
		if(BST_CHECKED==IsDlgButtonChecked(IDC_CHECK6))
			ini_bCNShowDiff=TRUE;
		else
			ini_bCNShowDiff=FALSE;
		if(BST_CHECKED==IsDlgButtonChecked(IDC_CHECK7))
			ini_bOnePageSlider=TRUE;
		else
			ini_bOnePageSlider=FALSE;
		ini_nRE2FintSize=GetDlgItemInt(IDC_EDIT9);
		if(BST_CHECKED==IsDlgButtonChecked(IDC_CHECK8))
			ini_bCNCOMNULL=TRUE;
		else
			ini_bCNCOMNULL=FALSE;
		if(BST_CHECKED==IsDlgButtonChecked(IDC_CHECK9))
			ini_bOutCtrlLen=TRUE;
		else
			ini_bOutCtrlLen=FALSE;

		//颜色
		IniClr2Str(ini_cstrClrNorText, clr1, true);
		IniClr2Str(ini_cstrClrCtrlText, clr2, true);
		IniClr2Str(ini_cstrClrDiffText, clr3, true);
		IniClr2Str(ini_cstrClrOverLenBg, clr4, true);
		IniClr2Str(ini_cstrClrNorTr, clr5, true);
		IniClr2Str(ini_cstrClrSelTr, clr6, true);
		IniClr2Str(ini_cstrClrTabBorder, clr7, true);
		IniClr2Str(ini_cstrClrBodyBg, clr8, true);
		IniClr2Str(ini_cstrClrHiLiREBg, clr9, true);
	}
	EndDialog(wID);
	return 0;
}

LRESULT CAboutDlg::OnEnKillfocusEdit6(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	int nItemPerPage, nWriteFilePerTime, nScale;

	nItemPerPage=GetDlgItemInt(IDC_EDIT6);
	nWriteFilePerTime=GetDlgItemInt(IDC_EDIT7);
	nScale=nItemPerPage/nWriteFilePerTime;
	if (nItemPerPage%nWriteFilePerTime>0)
		nScale++;
	nItemPerPage=nScale*nWriteFilePerTime;
	SetDlgItemInt(IDC_EDIT6, nItemPerPage);
	return 0;
}

LRESULT CAboutDlg::OnEnKillfocusEdit7(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	int nItemPerPage, nWriteFilePerTime, nScale;

	nItemPerPage=GetDlgItemInt(IDC_EDIT6);
	nWriteFilePerTime=GetDlgItemInt(IDC_EDIT7);
	nScale=nItemPerPage/nWriteFilePerTime;
	if (nItemPerPage%nWriteFilePerTime>0)
		nScale++;
	nItemPerPage=nScale*nWriteFilePerTime;
	SetDlgItemInt(IDC_EDIT6, nItemPerPage);
	return 0;
}

bool IniClr2Str(CString &cstrClr, COLORREF &clr, bool bClr2Str)
{
	BYTE r, g, b;

	if (bClr2Str)
	{
		r=GetRValue(clr);
		g=GetGValue(clr);
		b=GetBValue(clr);
		cstrClr.Format(_T("#%02x%02x%02x"), r, g, b);
	} 
	else
	{
		r=(BYTE)_tcstoul(cstrClr.Mid(1, 2), NULL, 16);
		g=(BYTE)_tcstoul(cstrClr.Mid(3, 2), NULL, 16);
		b=(BYTE)_tcstoul(cstrClr.Mid(5, 2), NULL, 16);
		clr=RGB(r,g, b);
	}
	return true;
}

LRESULT CAboutDlg::OnBnClickedButton1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CColorDialog ClrDlg(clr1, CC_FULLOPEN | CC_RGBINIT, this->m_hWnd);
	ClrDlg.DoModal();
	clr1=ClrDlg.GetColor();
	CRE2Clr1.SetBackgroundColor(clr1);
	return 0;
}

LRESULT CAboutDlg::OnBnClickedButton2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CColorDialog ClrDlg(clr2, CC_FULLOPEN | CC_RGBINIT, this->m_hWnd);
	ClrDlg.DoModal();
	clr2=ClrDlg.GetColor();
	CRE2Clr2.SetBackgroundColor(clr2);
	return 0;
}

LRESULT CAboutDlg::OnBnClickedButton3(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CColorDialog ClrDlg(clr3, CC_FULLOPEN | CC_RGBINIT, this->m_hWnd);
	ClrDlg.DoModal();
	clr3=ClrDlg.GetColor();
	CRE2Clr3.SetBackgroundColor(clr3);
	return 0;
}

LRESULT CAboutDlg::OnBnClickedButton4(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CColorDialog ClrDlg(clr4, CC_FULLOPEN | CC_RGBINIT, this->m_hWnd);
	ClrDlg.DoModal();
	clr4=ClrDlg.GetColor();
	CRE2Clr4.SetBackgroundColor(clr4);
	return 0;
}

LRESULT CAboutDlg::OnBnClickedButton5(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CColorDialog ClrDlg(clr5, CC_FULLOPEN | CC_RGBINIT, this->m_hWnd);
	ClrDlg.DoModal();
	clr5=ClrDlg.GetColor();
	CRE2Clr5.SetBackgroundColor(clr5);
	return 0;
}

LRESULT CAboutDlg::OnBnClickedButton6(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CColorDialog ClrDlg(clr6, CC_FULLOPEN | CC_RGBINIT, this->m_hWnd);
	ClrDlg.DoModal();
	clr6=ClrDlg.GetColor();
	CRE2Clr6.SetBackgroundColor(clr6);
	return 0;
}

LRESULT CAboutDlg::OnBnClickedButton7(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CColorDialog ClrDlg(clr7, CC_FULLOPEN | CC_RGBINIT, this->m_hWnd);
	ClrDlg.DoModal();
	clr7=ClrDlg.GetColor();
	CRE2Clr7.SetBackgroundColor(clr7);
	return 0;
}

LRESULT CAboutDlg::OnBnClickedButton8(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CColorDialog ClrDlg(clr8, CC_FULLOPEN | CC_RGBINIT, this->m_hWnd);
	ClrDlg.DoModal();
	clr8=ClrDlg.GetColor();
	CRE2Clr8.SetBackgroundColor(clr8);
	return 0;
}

LRESULT CAboutDlg::OnBnClickedButton9(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CColorDialog ClrDlg(clr9, CC_FULLOPEN | CC_RGBINIT, this->m_hWnd);
	ClrDlg.DoModal();
	clr9=ClrDlg.GetColor();
	CRE2Clr9.SetBackgroundColor(clr9);
	return 0;
}

LRESULT CAboutDlg::OnBnClickedButton10(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	int nIndex;
	CString cstrBuffer;

	nIndex=CCbbDefault.GetCurSel();
	switch (nIndex)
	{
	case 0:
		//宽度
		SetDlgItemText(IDC_EDIT1, DEFAULT_WIDTH_ID);
		SetDlgItemText(IDC_EDIT2, DEFAULT_WIDTH_JP);
		SetDlgItemText(IDC_EDIT3, DEFAULT_WIDTH_CN);
		SetDlgItemText(IDC_EDIT8, DEFAULT_WIDTH_COM);
		//高级
		SetDlgItemInt(IDC_EDIT4, DEFAULT_BYTE_PER_CHAR);
		SetDlgItemInt(IDC_EDIT5, DEFAULT_TEXT_BUF_LEN);
		SetDlgItemInt(IDC_EDIT6, DEFAULT_ITEM_PER_PAGE);
		SetDlgItemInt(IDC_EDIT7, DEFAULT_WRITE_FILE_PER_TIME);
		if(DEFAULT_EDIT_CTRLCHAR)
			CheckDlgButton(IDC_CHECK2, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK2, BST_UNCHECKED);
		if(DEFAULT_HALF_TO_FULL)
			CheckDlgButton(IDC_CHECK3, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK3, BST_UNCHECKED);
		if(DEFAULT_SAVE_SPLITER_POS)
			CheckDlgButton(IDC_CHECK4, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK4, BST_UNCHECKED);
		if(DEFAULT_JP_SHOW_CNFONT)
			CheckDlgButton(IDC_CHECK5, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK5, BST_UNCHECKED);
		if(DEFAULT_CN_SHOW_DIFF)
			CheckDlgButton(IDC_CHECK6, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK6, BST_UNCHECKED);
		if(DEFAULT_ONE_PAGE_SLIDER)
			CheckDlgButton(IDC_CHECK7, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK7, BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT9, DEFAULT_RE2_FONT_SIZE);
		if(DEFAULT_CN_COM_NULL)
			CheckDlgButton(IDC_CHECK8, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK8, BST_UNCHECKED);
		if(DEFAULT_OUT_CTRL_LEN)
			CheckDlgButton(IDC_CHECK9, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK9, BST_UNCHECKED);
		//颜色
		cstrBuffer=DEFAULT_NOR_TEXT;
		IniClr2Str(cstrBuffer, clr1, false);
		CRE2Clr1.SetBackgroundColor(clr1);
		cstrBuffer=DEFAULT_CTRL_TEXT;
		IniClr2Str(cstrBuffer, clr2, false);
		CRE2Clr2.SetBackgroundColor(clr2);
		cstrBuffer=DEFAULT_DIFF_TEXT;
		IniClr2Str(cstrBuffer, clr3, false);
		CRE2Clr3.SetBackgroundColor(clr3);
		cstrBuffer=DEFAULT_OVER_LEN_BG;
		IniClr2Str(cstrBuffer, clr4, false);
		CRE2Clr4.SetBackgroundColor(clr4);
		cstrBuffer=DEFAULT_NOR_TR;
		IniClr2Str(cstrBuffer, clr5, false);
		CRE2Clr5.SetBackgroundColor(clr5);
		cstrBuffer=DEFAULT_SEL_TR;
		IniClr2Str(cstrBuffer, clr6, false);
		CRE2Clr6.SetBackgroundColor(clr6);
		cstrBuffer=DEFAULT_TAB_BORDER;
		IniClr2Str(cstrBuffer, clr7, false);
		CRE2Clr7.SetBackgroundColor(clr7);
		cstrBuffer=DEFAULT_BODY_BG;
		IniClr2Str(cstrBuffer, clr8, false);
		CRE2Clr8.SetBackgroundColor(clr8);
		cstrBuffer=DEFAULT_HILIRE_BG;
		IniClr2Str(cstrBuffer, clr9, false);
		CRE2Clr9.SetBackgroundColor(clr9);
		break;
	case 1:
		//宽度
		SetDlgItemText(IDC_EDIT1, DEFAULT_WIDTH_ID);
		SetDlgItemText(IDC_EDIT2, DEFAULT_WIDTH_JP);
		SetDlgItemText(IDC_EDIT3, DEFAULT_WIDTH_CN);
		SetDlgItemText(IDC_EDIT8, DEFAULT_WIDTH_COM);
		break;
	case 2:
		//高级
		SetDlgItemInt(IDC_EDIT4, DEFAULT_BYTE_PER_CHAR);
		SetDlgItemInt(IDC_EDIT5, DEFAULT_TEXT_BUF_LEN);
		SetDlgItemInt(IDC_EDIT6, DEFAULT_ITEM_PER_PAGE);
		SetDlgItemInt(IDC_EDIT7, DEFAULT_WRITE_FILE_PER_TIME);
		if(DEFAULT_EDIT_CTRLCHAR)
			CheckDlgButton(IDC_CHECK2, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK2, BST_UNCHECKED);
		if(DEFAULT_HALF_TO_FULL)
			CheckDlgButton(IDC_CHECK3, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK3, BST_UNCHECKED);
		if(DEFAULT_SAVE_SPLITER_POS)
			CheckDlgButton(IDC_CHECK4, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK4, BST_UNCHECKED);
		if(DEFAULT_JP_SHOW_CNFONT)
			CheckDlgButton(IDC_CHECK5, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK5, BST_UNCHECKED);
		if(DEFAULT_CN_SHOW_DIFF)
			CheckDlgButton(IDC_CHECK6, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK6, BST_UNCHECKED);
		if(DEFAULT_ONE_PAGE_SLIDER)
			CheckDlgButton(IDC_CHECK7, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK7, BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT9, DEFAULT_RE2_FONT_SIZE);
		if(DEFAULT_CN_COM_NULL)
			CheckDlgButton(IDC_CHECK8, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK8, BST_UNCHECKED);
		if(DEFAULT_OUT_CTRL_LEN)
			CheckDlgButton(IDC_CHECK9, BST_CHECKED);
		else
			CheckDlgButton(IDC_CHECK9, BST_UNCHECKED);
		break;
	case 3:
		//颜色
		cstrBuffer=DEFAULT_NOR_TEXT;
		IniClr2Str(cstrBuffer, clr1, false);
		CRE2Clr1.SetBackgroundColor(clr1);
		cstrBuffer=DEFAULT_CTRL_TEXT;
		IniClr2Str(cstrBuffer, clr2, false);
		CRE2Clr2.SetBackgroundColor(clr2);
		cstrBuffer=DEFAULT_DIFF_TEXT;
		IniClr2Str(cstrBuffer, clr3, false);
		CRE2Clr3.SetBackgroundColor(clr3);
		cstrBuffer=DEFAULT_OVER_LEN_BG;
		IniClr2Str(cstrBuffer, clr4, false);
		CRE2Clr4.SetBackgroundColor(clr4);
		cstrBuffer=DEFAULT_NOR_TR;
		IniClr2Str(cstrBuffer, clr5, false);
		CRE2Clr5.SetBackgroundColor(clr5);
		cstrBuffer=DEFAULT_SEL_TR;
		IniClr2Str(cstrBuffer, clr6, false);
		CRE2Clr6.SetBackgroundColor(clr6);
		cstrBuffer=DEFAULT_TAB_BORDER;
		IniClr2Str(cstrBuffer, clr7, false);
		CRE2Clr7.SetBackgroundColor(clr7);
		cstrBuffer=DEFAULT_BODY_BG;
		IniClr2Str(cstrBuffer, clr8, false);
		CRE2Clr8.SetBackgroundColor(clr8);
		cstrBuffer=DEFAULT_HILIRE_BG;
		IniClr2Str(cstrBuffer, clr9, false);
		CRE2Clr9.SetBackgroundColor(clr9);
		break;
	}
	return 0;
}
