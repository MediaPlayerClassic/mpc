/* 
 *	Copyright (C) Gabest - December 2002
 *
 *  DeCSSFilter.ax is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  DeCSSFilter.ax is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include "stdafx.h"
#include <atlbase.h>
#include <ks.h>
#include <ksmedia.h>
#include "DeCSSFilter.h"

#include "..\..\..\decss\CSSauth.h"
#include "..\..\..\decss\CSSscramble.h"
#include "..\..\..\DSUtil\DSUtil.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
    { L"Input",             // Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesIn)/sizeof(sudPinTypesIn[0]), // Number of types
      sudPinTypesIn		// Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesOut)/sizeof(sudPinTypesOut[0]), // Number of types
      sudPinTypesOut		// Pin information
    }
};

const AMOVIESETUP_FILTER sudFilter =
{
    &__uuidof(CDeCSSFilter),		// Filter CLSID
    L"DeCSSFilter",			// String name
    MERIT_DO_NOT_USE,       // Filter merit // MERIT_PREFERRED+1
    sizeof(sudpPins)/sizeof(sudpPins[0]),                      // Number of pins
    sudpPins                // Pin information
};

CFactoryTemplate g_Templates[] =
{
    { L"DeCSSFilter"
    , &__uuidof(CDeCSSFilter)
    , CDeCSSFilter::CreateInstance
    , NULL
    , &sudFilter }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

//
// CDeCSSFilter
//

CUnknown* WINAPI CDeCSSFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CDeCSSFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

CDeCSSFilter::CDeCSSFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CTransformFilter(NAME("CDeCSSFilter"), lpunk, __uuidof(this))
{
	if(phr) *phr = S_OK;

	if(!(m_pInput = new CDeCSSInputPin(this, phr))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr)) return;

	if(!(m_pOutput = new CTransformOutputPin(NAME("Transform output pin"), this, phr, L"Out"))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr))  {delete m_pInput, m_pInput = NULL; return;}
}

CDeCSSFilter::~CDeCSSFilter()
{
}

HRESULT CDeCSSFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;

	pIn->GetPointer(&pDataIn);
	pOut->GetPointer(&pDataOut);

	long len = pIn->GetActualDataLength();
	long size = pOut->GetSize();

	if(!pDataIn || !pDataOut || len > size || len <= 0) return S_FALSE;

	memcpy(pDataOut, pDataIn, min(len, size));
	pOut->SetActualDataLength(min(len, size));

	return S_OK;
}

HRESULT CDeCSSFilter::CheckInputType(const CMediaType* mtIn)
{
	return (mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CDeCSSFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return (mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && mtOut->majortype == MEDIATYPE_MPEG2_PES) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CDeCSSFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	pProperties->cbAlign = 1;
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 2048;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
		return hr;

    return(pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR);
}

HRESULT CDeCSSFilter::GetMediaType(int iPosition, CMediaType* pMediaType)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	CopyMediaType(pMediaType, &m_pInput->CurrentMediaType());
	pMediaType->majortype = MEDIATYPE_MPEG2_PES;

	return S_OK;
}

//
// CDeCSSInputPin
//

CDeCSSInputPin::CDeCSSInputPin(CTransformFilter* pFilter, HRESULT* phr)
	: CTransformInputPin(NAME("CDeCSSInputPin"), pFilter, phr, L"In")
{
	m_varient = -1;
	memset(m_Challenge, 0, sizeof(m_Challenge));
	memset(m_KeyCheck, 0, sizeof(m_KeyCheck));
	memset(m_DiscKey, 0, sizeof(m_DiscKey));
	memset(m_TitleKey, 0, sizeof(m_TitleKey));
}

STDMETHODIMP CDeCSSInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IKsPropertySet)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

// IMemInputPin

STDMETHODIMP CDeCSSInputPin::Receive(IMediaSample* pSample)
{
	if(m_mt.majortype == MEDIATYPE_DVD_ENCRYPTED_PACK
	&& pSample->GetActualDataLength() == 2048)
	{
		BYTE* pBuffer = NULL;
		if(SUCCEEDED(pSample->GetPointer(&pBuffer)) && (pBuffer[0x14]&0x30))
		{
			CSSdescramble(pBuffer, m_TitleKey);
			pBuffer[0x14] &= ~0x30;

			if(CComQIPtr<IMediaSample2> pMS2 = pSample)
			{
				AM_SAMPLE2_PROPERTIES props;
				memset(&props, 0, sizeof(props));
				if(SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props))
				&& (props.dwTypeSpecificFlags & AM_UseNewCSSKey))
				{
					props.dwTypeSpecificFlags &= ~AM_UseNewCSSKey;
					pMS2->SetProperties(sizeof(props), (BYTE*)&props);
				}
			}
		}
	}

	return(CTransformInputPin::Receive(pSample));
}

// IKsPropertySet

STDMETHODIMP CDeCSSInputPin::Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength)
{
	if(PropSet != AM_KSPROPSETID_CopyProt)
		return E_NOTIMPL;

	switch(Id)
	{
	case AM_PROPERTY_COPY_MACROVISION:
		break;
	case AM_PROPERTY_DVDCOPY_CHLG_KEY: // 3. auth: receive drive nonce word, also store and encrypt the buskey made up of the two nonce words
		{
			AM_DVDCOPY_CHLGKEY* pChlgKey = (AM_DVDCOPY_CHLGKEY*)pPropertyData;
			for(int i = 0; i < 10; i++)
				m_Challenge[i] = pChlgKey->ChlgKey[9-i];

			CSSkey2(m_varient, m_Challenge, &m_Key[5]);

			CSSbuskey(m_varient, m_Key, m_KeyCheck);
		}
		break;
	case AM_PROPERTY_DVDCOPY_DISC_KEY: // 5. receive the disckey
		{
			AM_DVDCOPY_DISCKEY* pDiscKey = (AM_DVDCOPY_DISCKEY*)pPropertyData; // pDiscKey->DiscKey holds the disckey encrypted with itself and the 408 disckeys encrypted with the playerkeys

			bool fSuccess = false;

			for(int j = 0; j < g_nPlayerKeys; j++)
			{
				for(int k = 1; k < 409; k++)
				{
					BYTE DiscKey[6];
					for(int i = 0; i < 5; i++)
						DiscKey[i] = pDiscKey->DiscKey[k*5+i] ^ m_KeyCheck[4-i];
					DiscKey[5] = 0;

					CSSdisckey(DiscKey, g_PlayerKeys[j]);

					BYTE Hash[6];
					for(int i = 0; i < 5; i++)
						Hash[i] = pDiscKey->DiscKey[i] ^ m_KeyCheck[4-i];
					Hash[5] = 0;

					CSSdisckey(Hash, DiscKey);

					if(!memcmp(Hash, DiscKey, 6))
					{
						memcpy(m_DiscKey, DiscKey, 6);
						j = g_nPlayerKeys;
						fSuccess = true;
						break;
					}
				}
			}

			if(!fSuccess)
				return E_FAIL;
		}
		break;
	case AM_PROPERTY_DVDCOPY_DVD_KEY1: // 2. auth: receive our drive-encrypted nonce word and decrypt it for verification
		{
			AM_DVDCOPY_BUSKEY* pKey1 = (AM_DVDCOPY_BUSKEY*)pPropertyData;
			for(int i = 0; i < 5; i++)
				m_Key[i] =  pKey1->BusKey[4-i];

			m_varient = -1;

			for(int i = 31; i >= 0; i--)
			{
				CSSkey1(i, m_Challenge, m_KeyCheck);

				if(memcmp(m_KeyCheck, &m_Key[0], 5) == 0)
					m_varient = i;
			}
		}
		break;
	case AM_PROPERTY_DVDCOPY_REGION:
		break;
	case AM_PROPERTY_DVDCOPY_SET_COPY_STATE:
		break;
	case AM_PROPERTY_DVDCOPY_TITLE_KEY: // 6. receive the title key and decrypt it with the disc key
		{
			AM_DVDCOPY_TITLEKEY* pTitleKey = (AM_DVDCOPY_TITLEKEY*)pPropertyData;
			for(int i = 0; i < 5; i++)
				m_TitleKey[i] = pTitleKey->TitleKey[i] ^ m_KeyCheck[4-i];
			m_TitleKey[5] = 0;
			CSStitlekey(m_TitleKey, m_DiscKey);
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}

	return S_OK;
}

STDMETHODIMP CDeCSSInputPin::Get(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength, ULONG* pBytesReturned)
{
	if(PropSet != AM_KSPROPSETID_CopyProt)
		return E_NOTIMPL;

	switch(Id)
	{
	case AM_PROPERTY_DVDCOPY_CHLG_KEY: // 1. auth: send our nonce word
		{
			AM_DVDCOPY_CHLGKEY* pChlgKey = (AM_DVDCOPY_CHLGKEY*)pPropertyData;
			for(int i = 0; i < 10; i++)
				pChlgKey->ChlgKey[i] = 9 - (m_Challenge[i] = i);
			*pBytesReturned = sizeof(AM_DVDCOPY_CHLGKEY);
		}
		break;
	case AM_PROPERTY_DVDCOPY_DEC_KEY2: // 4. auth: send back the encrypted drive nonce word to finish the authentication
		{
			AM_DVDCOPY_BUSKEY* pKey2 = (AM_DVDCOPY_BUSKEY*)pPropertyData;
			for(int i = 0; i < 5; i++)
				pKey2->BusKey[4-i] = m_Key[5+i];
			*pBytesReturned = sizeof(AM_DVDCOPY_BUSKEY);
		}
		break;
	case AM_PROPERTY_DVDCOPY_REGION:
		{
			DVD_REGION* pRegion = (DVD_REGION*)pPropertyData;
			pRegion->RegionData = 0;
			pRegion->SystemRegion = 0;
			*pBytesReturned = sizeof(DVD_REGION);
		}
		break;
	case AM_PROPERTY_DVDCOPY_SET_COPY_STATE:
		{
			AM_DVDCOPY_SET_COPY_STATE* pState = (AM_DVDCOPY_SET_COPY_STATE*)pPropertyData;
			pState->DVDCopyState = AM_DVDCOPYSTATE_AUTHENTICATION_REQUIRED;
			*pBytesReturned = sizeof(AM_DVDCOPY_SET_COPY_STATE);
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}

	return S_OK;
}

STDMETHODIMP CDeCSSInputPin::QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport)
{
	if(PropSet != AM_KSPROPSETID_CopyProt)
		return E_NOTIMPL;

	switch(Id)
	{
	case AM_PROPERTY_COPY_MACROVISION:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_CHLG_KEY:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_DEC_KEY2:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_PROPERTY_DVDCOPY_DISC_KEY:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_DVD_KEY1:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_REGION:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_SET_COPY_STATE:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_TITLE_KEY:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
	
	return S_OK;
}
