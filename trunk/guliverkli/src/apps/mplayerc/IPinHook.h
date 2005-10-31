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

#pragma once

interface IPinC;

typedef struct IPinCVtbl
{
    BEGIN_INTERFACE
    HRESULT ( STDMETHODCALLTYPE *QueryInterface )( IPinC * This, /* [in] */ REFIID riid, /* [iid_is][out] */ void **ppvObject );
    ULONG ( STDMETHODCALLTYPE *AddRef )( IPinC * This );
    ULONG ( STDMETHODCALLTYPE *Release )( IPinC * This );
    HRESULT ( STDMETHODCALLTYPE *Connect )( IPinC * This, /* [in] */ IPinC *pReceivePin, /* [in] */ const AM_MEDIA_TYPE *pmt );
    HRESULT ( STDMETHODCALLTYPE *ReceiveConnection )( IPinC * This, /* [in] */ IPinC *pConnector, /* [in] */ const AM_MEDIA_TYPE *pmt );
    HRESULT ( STDMETHODCALLTYPE *Disconnect )( IPinC * This );
    HRESULT ( STDMETHODCALLTYPE *ConnectedTo )( IPinC * This, /* [out] */ IPinC **pPin );
    HRESULT ( STDMETHODCALLTYPE *ConnectionMediaType )( IPinC * This, /* [out] */ AM_MEDIA_TYPE *pmt );
    HRESULT ( STDMETHODCALLTYPE *QueryPinInfo )( IPinC * This, /* [out] */ PIN_INFO *pInfo );
    HRESULT ( STDMETHODCALLTYPE *QueryDirection )( IPinC * This, /* [out] */ PIN_DIRECTION *pPinDir );
    HRESULT ( STDMETHODCALLTYPE *QueryId )( IPinC * This, /* [out] */ LPWSTR *Id );
    HRESULT ( STDMETHODCALLTYPE *QueryAccept )( IPinC * This, /* [in] */ const AM_MEDIA_TYPE *pmt );
    HRESULT ( STDMETHODCALLTYPE *EnumMediaTypes )( IPinC * This, /* [out] */ IEnumMediaTypes **ppEnum );
    HRESULT ( STDMETHODCALLTYPE *QueryInternalConnections )( IPinC * This, /* [out] */ IPinC **apPin, /* [out][in] */ ULONG *nPin );
    HRESULT ( STDMETHODCALLTYPE *EndOfStream )( IPinC * This );
    HRESULT ( STDMETHODCALLTYPE *BeginFlush )( IPinC * This );
    HRESULT ( STDMETHODCALLTYPE *EndFlush )( IPinC * This );
    HRESULT ( STDMETHODCALLTYPE *NewSegment )( IPinC * This, /* [in] */ REFERENCE_TIME tStart, /* [in] */ REFERENCE_TIME tStop, /* [in] */ double dRate );
    END_INTERFACE
} IPinCVtbl;

interface IPinC
{
    CONST_VTBL struct IPinCVtbl *lpVtbl;
};

extern void HookNewSegment(IPinC* pPinC);
extern REFERENCE_TIME g_tSegmentStart;
