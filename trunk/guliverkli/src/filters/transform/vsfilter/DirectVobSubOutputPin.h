#pragma once

class CDirectVobSubFilter;

class CDirectVobSubOutputPin : public CTransformOutputPin
{
    CDirectVobSubFilter* m_pFilter;

public:
    CDirectVobSubOutputPin(CDirectVobSubFilter* pFilter, HRESULT* phr);

	STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE* pmt);
};
