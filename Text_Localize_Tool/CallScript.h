// Í·ÎÄ¼þ
#pragma once
#include <atlbase.h>
#include <Mshtml.h>

class CCallScript
{
public:
	CCallScript();
	virtual ~CCallScript();
	BOOL DocumentSet(){return(m_bDocumentSet);}
	BOOL SetDocument(IDispatch* pDisp);
	LPDISPATCH GetHtmlDocument() const;
	const CComBSTR GetLastError() const;
	BOOL GetScript(CComPtr<IDispatch>& spDisp);
	BOOL GetScripts(CComPtr<IHTMLElementCollection>& spColl);

	BOOL Run(const CComBSTR strFunc,CComVariant* pVarResult = NULL);
	BOOL Run(const CComBSTR strFunc,const CComBSTR strArg1,CComVariant* pVarResult = NULL);
	BOOL Run(const CComBSTR strFunc,const CComBSTR strArg1,const CComBSTR strArg2,CComVariant* pVarResult = NULL);
	BOOL Run(const CComBSTR strFunc,const CComBSTR strArg1,const CComBSTR strArg2,const CComBSTR strArg3,CComVariant* pVarResult = NULL);
	BOOL Run(const CComBSTR strFunc,const CSimpleArray<CComBSTR> & paramArray,CComVariant* pVarResult = NULL);
private:
	BOOL m_bDocumentSet;
protected:


	void ShowError(CComBSTR lpszText);

protected:

	CComPtr<IHTMLDocument2>    m_spDoc;
	CComBSTR    m_strError;
};

inline void CCallScript::ShowError(CComBSTR lpszText)
{
	m_strError = "Error: ";
	m_strError.Append(lpszText);
}
inline const CComBSTR CCallScript::GetLastError() const
{
	return m_strError;
}
inline LPDISPATCH CCallScript::GetHtmlDocument() const
{
	return m_spDoc;
}