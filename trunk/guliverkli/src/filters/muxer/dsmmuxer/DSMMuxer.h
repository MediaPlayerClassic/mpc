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

#include "..\basemuxer\basemuxer.h"
#include "..\..\..\..\include\dsm\dsm.h"

[uuid("8D0256EA-993A-4ACC-9C87-26D04F0C6A6C")]
interface IDSMMuxerFilter : public IUnknown
{
	STDMETHOD(SetOutputRawStreams) (BOOL b) = 0;
};

[uuid("C6590B76-587E-4082-9125-680D0693A97B")]
class CDSMMuxerFilter 
	: public CBaseMuxerFilter
	, public IDSMMuxerFilter
{
	bool m_fAutoChap, m_fAutoRes;

	BOOL m_bOutputRawStreams;
	struct file_t {FILE* pFile; int iPacket;};
	CAtlMap<CBaseMuxerInputPin*, file_t> m_pPinToFile;

	struct SyncPoint {BYTE id; REFERENCE_TIME rtStart, rtStop; __int64 fp;};
	struct IndexedSyncPoint {BYTE id; REFERENCE_TIME rt, rtfp; __int64 fp;};
	CList<SyncPoint> m_sps;
	CList<IndexedSyncPoint> m_isps;
	REFERENCE_TIME m_rtPrevSyncPoint;
	void IndexSyncPoint(const MuxerPacket* p, __int64 fp);

	void MuxPacketHeader(IBitStream* pBS, dsmp_t type, UINT64 len);
	void MuxFileInfo(IBitStream* pBS);
	void MuxStreamInfo(IBitStream* pBS, CBaseMuxerInputPin* pPin);

protected:
	void MuxInit();

	void MuxHeader(IBitStream* pBS);
	void MuxPacket(IBitStream* pBS, const MuxerPacket* pPacket);
	void MuxFooter(IBitStream* pBS);

	void MuxHeader();
	void MuxPacket(const MuxerPacket* pPacket);
	void MuxFooter();

public:
	CDSMMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr, bool fAutoChap = true, bool fAutoRes = true);
	virtual ~CDSMMuxerFilter();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IDSMMuxerFilter

	STDMETHODIMP SetOutputRawStreams(BOOL b);
};
