#include "stdafx.h"
#include "keyprovider.h"
#include "..\..\DSUtil\DSUtil.h"
#include "c:\WMSDK\WMFSDK9\include\wmsdk.h"

CKeyProvider::CKeyProvider() 
	: CUnknown(NAME("CKeyProvider"), NULL)
{
}

STDMETHODIMP CKeyProvider::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IServiceProvider)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CKeyProvider::QueryService(REFIID siid, REFIID riid, void **ppv)
{
    if(siid == __uuidof(IWMReader) && riid == IID_IUnknown)
	{
        CComPtr<IUnknown> punkCert;
        HRESULT hr = WMCreateCertificate(&punkCert);
        if(SUCCEEDED(hr)) 
			*ppv = (void*)punkCert.Detach();
        return hr;
    }

	return E_NOINTERFACE;
}
