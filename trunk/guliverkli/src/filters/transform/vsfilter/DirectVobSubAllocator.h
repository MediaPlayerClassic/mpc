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

#pragma once

class CDirectVobSubAllocator : public CMemAllocator
{
protected:
    CBaseFilter* m_pFilter;				// Delegate reference counts to
    CMediaType m_mt;
	bool m_fMediaTypeChanged;

public:
	CDirectVobSubAllocator(CBaseFilter* pFilter, HRESULT* phr);
#ifdef DEBUG
	~CDirectVobSubAllocator();
#endif

	STDMETHODIMP_(ULONG) NonDelegatingAddRef() {return m_pFilter->AddRef();}
	STDMETHODIMP_(ULONG) NonDelegatingRelease() {return m_pFilter->Release();}

	void NotifyMediaType(CMediaType mt);

	STDMETHODIMP GetBuffer(IMediaSample** ppBuffer,
		REFERENCE_TIME* pStartTime,
		REFERENCE_TIME* pEndTime,
		DWORD dwFlags);
};
