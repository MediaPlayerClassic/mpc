/* 
 *	Copyright (C) 2003-2004 Gabest
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

#include "..\..\..\DSUtil\DSUtil.h"

class CDirectVobSubFilter;

class CDirectVobSubInputPin : public CTransformInputPin/*, IPinConnection*/
{
    CDirectVobSubFilter* m_pFilter;

public:
    CDirectVobSubInputPin(CDirectVobSubFilter* pFilter, HRESULT* phr);

	// IPin
	STDMETHODIMP GetAllocator(IMemAllocator** ppAllocator);
	STDMETHODIMP NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly);

	STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt);
};

