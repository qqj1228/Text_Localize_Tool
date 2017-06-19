// CPPÎÄ¼þ
#include "stdafx.h"
#include "CallScript.h"

#define CHECK_POINTER(p)\
ATLASSERT(p != NULL);\
if(p == NULL)\
{\
	ShowError("NULL pointer");\
	return FALSE;\
}

const CComBSTR GetSystemErrorMessage(DWORD dwError)
{
	CComBSTR strError;
	LPTSTR lpBuffer;

	if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,  dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
		(LPTSTR) &lpBuffer, 0, NULL))
	{
		strError = "FormatMessage Netive Error" ;
	}
	else
	{
		strError = lpBuffer;
		LocalFree(lpBuffer);
	}
	return strError;
}

CCallScript::CCallScript()
{
	m_bDocumentSet = FALSE;
}

CCallScript::~CCallScript()
{
}

BOOL CCallScript::SetDocument(IDispatch* pDisp)
{
	CHECK_POINTER(pDisp);

	m_spDoc = NULL;

	CComPtr<IDispatch> spDisp = pDisp;

	HRESULT hr = spDisp->QueryInterface(IID_IHTMLDocument2,(void**)&m_spDoc);
	if(FAILED(hr))
	{
		ShowError("Failed to get HTML document COM object");
		return FALSE;
	}
	m_bDocumentSet = TRUE;
	return TRUE;
}

BOOL CCallScript::GetScript(CComPtr<IDispatch>& spDisp)
{
	CHECK_POINTER(m_spDoc);
	HRESULT hr = m_spDoc->get_Script(&spDisp);
	ATLASSERT(SUCCEEDED(hr));
	return SUCCEEDED(hr);
}

BOOL CCallScript::GetScripts(CComPtr<IHTMLElementCollection>& spColl)
{
	CHECK_POINTER(m_spDoc);
	HRESULT hr = m_spDoc->get_scripts(&spColl);
	ATLASSERT(SUCCEEDED(hr));
	return SUCCEEDED(hr);
}

BOOL CCallScript::Run(const CComBSTR strFunc,CComVariant* pVarResult)
{
	CSimpleArray<CComBSTR>  paramArray;
	return Run(strFunc,paramArray,pVarResult);
}

BOOL CCallScript::Run(const CComBSTR strFunc,const CComBSTR strArg1,CComVariant* pVarResult)
{
	CSimpleArray<CComBSTR>  paramArray;
	paramArray.Add((CComBSTR &)strArg1);
	return Run(strFunc,paramArray,pVarResult);
}

BOOL CCallScript::Run(const CComBSTR strFunc,const CComBSTR strArg1,const CComBSTR strArg2,CComVariant* pVarResult)
{
	CSimpleArray<CComBSTR>  paramArray;
	paramArray.Add((CComBSTR &)strArg1);
	paramArray.Add((CComBSTR &)strArg2);
	return Run(strFunc,paramArray,pVarResult);
}

BOOL CCallScript::Run(const CComBSTR strFunc,const CComBSTR strArg1,const CComBSTR strArg2,const CComBSTR strArg3,CComVariant* pVarResult)
{
	CSimpleArray<CComBSTR>  paramArray;
	paramArray.Add((CComBSTR &)strArg1);
	paramArray.Add((CComBSTR &)strArg2);
	paramArray.Add((CComBSTR &)strArg3);
	return Run(strFunc,paramArray,pVarResult);
}

BOOL CCallScript::Run(const CComBSTR strFunc, const CSimpleArray<CComBSTR>& paramArray,CComVariant* pVarResult)
{
	CComPtr<IDispatch> spScript;
	if(!GetScript(spScript))
	{
		ShowError("Cannot GetScript");
		return FALSE;
	}
	CComBSTR bstrMember(strFunc);
	DISPID dispid = NULL;
	HRESULT hr = spScript->GetIDsOfNames(IID_NULL,&bstrMember,1,
		LOCALE_SYSTEM_DEFAULT,&dispid);
	if(FAILED(hr))
	{
		ShowError(GetSystemErrorMessage(hr));
		return FALSE;
	}

	//const int arraySize = paramArray.GetCount();
	const int arraySize = paramArray.GetSize();

	DISPPARAMS dispparams;
	memset(&dispparams, 0, sizeof dispparams);
	dispparams.cArgs = arraySize;
	dispparams.rgvarg = new VARIANT[dispparams.cArgs];
	//__asm {int 3}
	CComBSTR bstr;
	for( int i = 0; i < arraySize; i++)
	{
		bstr.Empty();
		//CComBSTR bstr = paramArray.GetAt(arraySize - 1 - i); // back reading
		bstr = paramArray[arraySize - 1 - i]; // back reading
		//bstr.CopyTo(&dispparams.rgvarg[i].bstrVal); //memory leak
		dispparams.rgvarg[i].bstrVal = bstr.m_str; //also cause problem when paras are more than 1
		dispparams.rgvarg[i].vt = VT_BSTR;
	}
	dispparams.cNamedArgs = 0;

	EXCEPINFO excepInfo;
	memset(&excepInfo, 0, sizeof excepInfo);
	CComVariant vaResult;
	UINT nArgErr = (UINT)-1;  // initialize to invalid arg

	hr = spScript->Invoke(dispid,IID_NULL,0,
		DISPATCH_METHOD,&dispparams,&vaResult,&excepInfo,&nArgErr);

	/**//////////////// bug fix memory leak code start ///////////////
	//    for( int j = 0; j < arraySize; j++)
	//        ::SysFreeString(dispparams.rgvarg[j].bstrVal);
	/**//////////////// bug fix memory leak code end ///////////////

	delete [] dispparams.rgvarg;
	if(FAILED(hr))
	{
		ShowError(GetSystemErrorMessage(hr));
		return FALSE;
	}

	if(pVarResult)
	{
		*pVarResult = vaResult;
	}
	return FALSE;
}