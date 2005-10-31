/* 
 *	Copyright (C) 2003-2005 Gabest
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
#include "IPinHook.h"

REFERENCE_TIME g_tSegmentStart = 0;

static HRESULT (STDMETHODCALLTYPE * NewSegmentOrg)(IPinC * This, /* [in] */ REFERENCE_TIME tStart, /* [in] */ REFERENCE_TIME tStop, /* [in] */ double dRate) = NULL;

static HRESULT STDMETHODCALLTYPE NewSegmentMine(IPinC * This, /* [in] */ REFERENCE_TIME tStart, /* [in] */ REFERENCE_TIME tStop, /* [in] */ double dRate)
{
	g_tSegmentStart = tStart;
	return NewSegmentOrg(This, tStart, tStop, dRate);
}

void HookNewSegment(IPinC* pPinC)
{
	if(NewSegmentOrg == NULL)
	{
		NewSegmentOrg = pPinC->lpVtbl->NewSegment;
	}

	g_tSegmentStart = 0;

	BOOL res;
	DWORD flOldProtect = 0;
	res = VirtualProtect(pPinC->lpVtbl, sizeof(IPinC), PAGE_WRITECOPY, &flOldProtect);

	pPinC->lpVtbl->NewSegment = NewSegmentMine;

	res = VirtualProtect(pPinC->lpVtbl, sizeof(IPinC), PAGE_EXECUTE, &flOldProtect);
}