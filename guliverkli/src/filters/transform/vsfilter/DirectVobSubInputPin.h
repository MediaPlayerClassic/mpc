#pragma once

#include "..\..\..\DSUtil\DSUtil.h"

class CDirectVobSubFilter;

class CDirectVobSubInputPin : public CTransformInputPin/*, IPinConnection*/
{
    CDirectVobSubFilter* m_pFilter;

public:
    CDirectVobSubInputPin(CDirectVobSubFilter* pFilter, HRESULT* phr);

	// IPin
	STDMETHODIMP Disconnect();
	STDMETHODIMP GetAllocator(IMemAllocator** ppAllocator);
	STDMETHODIMP NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly);

	STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt);
};

