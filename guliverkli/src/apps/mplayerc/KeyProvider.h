#pragma once

#include <Servprov.h>

// Declare and implement a key provider class derived from IServiceProvider.

class CKeyProvider 
	: public CUnknown
	, public IServiceProvider
{
public:
    CKeyProvider();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    // IServiceProvider
    STDMETHODIMP QueryService(REFIID siid, REFIID riid, void **ppv);
};
