/* 
 *	Copyright (C) 2003 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "DirectVobSubFilter.h"
#include "DirectVobSubOutputPin.h"
#include "VSFilter.h"
#include "..\..\..\DSUtil\DSUtil.h"

CDirectVobSubOutputPin::CDirectVobSubOutputPin(CDirectVobSubFilter* pFilter, HRESULT* phr) 
	: CTransformOutputPin(NAME("CDirectVobSubOutputPin"), pFilter, phr, L"XForm Out")
	, m_pFilter(pFilter)
{
}

STDMETHODIMP CDirectVobSubOutputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = S_FALSE;

	IPin* pIn = m_pFilter->m_pInput->GetConnected();
	if(!pIn) return hr;

	BITMAPINFOHEADER bih;
	ExtractBIH(pmt, &bih);

	if(m_pFilter->m_sizeSub.cy != abs(bih.biHeight)
	|| (bih.biHeight > 0 && m_pFilter->m_sizeSub.cx != bih.biWidth))
		return hr;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	bool fForceRGB = !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_FORCERGB), 0);
	if(fForceRGB && bih.biCompression > 3) return hr;

	BeginEnumMediaTypes(pIn, pEMT, pmt2)
	{
		if(S_FALSE != hr) break;

		const CMediaType mt(*pmt2);
		if(pmt->subtype == pmt2->subtype && m_pFilter->CheckInputType(&mt) == S_OK)
			hr = pIn->QueryAccept(pmt2); // this shouldn't fail normally... (but it does, e.g. when divx5.02 is set to its yv12 mode, it offers rgb but won't switch...)
	}
	EndEnumMediaTypes(pmt2)

	return(hr);
}
