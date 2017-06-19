#pragma once
#include "richole.h"
#include "resource.h"

extern HWND g_hWndMain, g_hWndRichEdit2;

class CIRichEditOleCallback :
	public IRichEditOleCallback
{
public:

	CIRichEditOleCallback(void)
	{
	}

	~CIRichEditOleCallback(void)
	{
	}

	// Methods of the IUnknown interface
	STDMETHOD_(ULONG, AddRef)       (void)                                  {   mRefCounter++;                              return mRefCounter; }
	STDMETHOD_(ULONG, Release)      (void)                                  {   if ( --mRefCounter == 0 )   delete this;    return mRefCounter; }
	STDMETHOD(QueryInterface)       (REFIID iid, void** ppvObject)
	{
		if (iid == IID_IUnknown || iid == IID_IRichEditOleCallback)         {   *ppvObject = this;  AddRef();   return S_OK;    }
		else                                                                    return E_NOINTERFACE;
	}

	// Methods of the IRichEditOleCallback interface
	STDMETHOD(ContextSensitiveHelp) (BOOL fEnterMode)                                                                                           {   return E_NOTIMPL;   }
	STDMETHOD(DeleteObject)         (LPOLEOBJECT lpoleobj)                                                                                      {   return E_NOTIMPL;   }
	STDMETHOD(GetClipboardData)     (CHARRANGE FAR *lpchrg, DWORD reco, LPDATAOBJECT FAR *lplpdataobj)                                          {   return E_NOTIMPL;   }
	STDMETHOD(GetContextMenu)       (WORD seltype, LPOLEOBJECT lpoleobj, CHARRANGE FAR *lpchrg, HMENU FAR *lphmenu);
	STDMETHOD(GetDragDropEffect)    (BOOL fDrag, DWORD grfKeyState, LPDWORD pdwEffect)                                                          {   return E_NOTIMPL;   }
	STDMETHOD(GetInPlaceContext)    (LPOLEINPLACEFRAME FAR *lplpFrame, LPOLEINPLACEUIWINDOW FAR *lplpDoc, LPOLEINPLACEFRAMEINFO lpFrameInfo)    {   return E_NOTIMPL;   }
	STDMETHOD(GetNewStorage)        (LPSTORAGE FAR *lplpstg)																					{   return E_NOTIMPL;   }
	STDMETHOD(QueryAcceptData)      (LPDATAOBJECT lpdataobj, CLIPFORMAT FAR *lpcfFormat, DWORD reco, BOOL fReally, HGLOBAL hMetaPict)           {   return E_NOTIMPL;   }
	STDMETHOD(QueryInsertObject)    (LPCLSID lpclsid, LPSTORAGE lpstg, LONG cp)                                                                 {   return S_OK;        }
	STDMETHOD(ShowContainerUI)      (BOOL fShow)                                                                                                {   return E_NOTIMPL;   }
	
	// Data
private:
	DWORD      mRefCounter;

};

STDMETHODIMP CIRichEditOleCallback::GetContextMenu(WORD seltype, LPOLEOBJECT lpoleobj, CHARRANGE FAR *lpchrg, HMENU FAR *lphmenu)
{
	CMenu CMenuBar, CPopUpMenu;
	POINT pt;

	::GetCursorPos( &pt );
	CMenuBar.LoadMenu(IDR_MENU1);
	CPopUpMenu=CMenuBar.GetSubMenu(0);
	DWORD dwCmd=CPopUpMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_VERPOSANIMATION,
								pt.x, pt.y, g_hWndMain);
	//*lphmenu=CMenuBar.m_hMenu;
	//*lphmenu=CPopUpMenu.m_hMenu;
	//::SendMessage(g_hWndRichEdit2, EM_EXSETSEL, 0, (LPARAM)lpchrg);
	switch (dwCmd)
	{
	case ID_EDIT_UNDO://Ctrl+Z
		::SendMessage(g_hWndRichEdit2, EM_UNDO, 0, 0);
		//::keybd_event(VK_CONTROL, 0, 0, 0);
		//::keybd_event(_T('Z'), 0, 0, 0);
		//::keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
		//::keybd_event(_T('Z'), 0, KEYEVENTF_KEYUP, 0);
		break;
	case ID_EDIT_REDO://Ctrl+Y
		::SendMessage(g_hWndRichEdit2, EM_REDO, 0, 0);
		//::keybd_event(VK_CONTROL, 0, 0, 0);
		//::keybd_event(_T('Y'), 0, 0, 0);
		//::keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
		//::keybd_event(_T('Y'), 0, KEYEVENTF_KEYUP, 0);
		break;
	case ID_EDIT_COPY://Ctrl+C
		::SendMessage(g_hWndRichEdit2, WM_COPY, 0, 0);
		//::keybd_event(VK_CONTROL, 0, 0, 0);
		//::keybd_event(_T('C'), 0, 0, 0);
		//::keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
		//::keybd_event(_T('C'), 0, KEYEVENTF_KEYUP, 0);
		break;
	case ID_EDIT_PASTE://Ctrl+V
		::SendMessage(g_hWndRichEdit2, WM_PASTE, 0, 0);
		//::keybd_event(VK_CONTROL, 0, 0, 0);
		//::keybd_event(_T('V'), 0, 0, 0);
		//::keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
		//::keybd_event(_T('V'), 0, KEYEVENTF_KEYUP, 0);
		break;
	case ID_EDIT_CUT://Ctrl+X
		::SendMessage(g_hWndRichEdit2, WM_CUT, 0, 0);
		//::keybd_event(VK_CONTROL, 0, 0, 0);
		//::keybd_event(_T('X'), 0, 0, 0);
		//::keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
		//::keybd_event(_T('X'), 0, KEYEVENTF_KEYUP, 0);
		break;
	case ID_EDIT_DEL://Delete
		::SendMessage(g_hWndRichEdit2, WM_CLEAR, 0, 0);
		//::keybd_event(VK_DELETE , 0, 0, 0);
		//::keybd_event(VK_DELETE , 0, KEYEVENTF_KEYUP, 0);
		break;
	case ID_EDIT_ALL://Ctrl+A
		::keybd_event(VK_CONTROL, 0, 0, 0);
		::keybd_event(_T('A'), 0, 0, 0);
		::keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
		::keybd_event(_T('A'), 0, KEYEVENTF_KEYUP, 0);
		break;
	}
	return S_OK;
}