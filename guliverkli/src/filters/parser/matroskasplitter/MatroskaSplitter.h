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

#include <atlbase.h>
#include <atlcoll.h>
#include <afxtempl.h>
#include "MatroskaFile.h"
#include "..\BaseSplitter\BaseSplitter.h"
#include "..\..\..\..\include\ITrackInfo.h"

class MatroskaPacket : public Packet
{
protected:
	int GetSize()
	{
		int size = 0;
		POSITION pos = b->BlockData.GetHeadPosition();
		while(pos) {size += b->BlockData.GetNext(pos)->GetSize();}
		return size;
	}
public:
	CAutoPtr<MatroskaReader::Block> b;
};

class CMatroskaSplitterOutputPin : public CBaseSplitterOutputPin
{
	HRESULT DeliverBlock(MatroskaPacket* p);

	int m_nMinCache;
	REFERENCE_TIME m_rtDefaultDuration;

	CCritSec m_csQueue;
	CAutoPtrList<MatroskaPacket> m_packets;
	CList<MatroskaPacket*> m_rob;

	typedef struct {REFERENCE_TIME rtStart, rtStop;} timeoverride;
	CList<timeoverride> m_tos;

protected:
	HRESULT DeliverPacket(CAutoPtr<Packet> p);

public:
	CMatroskaSplitterOutputPin(
		int nMinCache, REFERENCE_TIME rtDefaultDuration, 
		CArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CMatroskaSplitterOutputPin();

	HRESULT DeliverEndFlush();
	HRESULT DeliverEndOfStream();
};

[uuid("149D2E01-C32E-4939-80F6-C07B81015A7A")]
class CMatroskaSplitterFilter : public CBaseSplitterFilter, public ITrackInfo
{
	void SendVorbisHeaderSample();

	static HANDLE (WINAPI *pAddFontMemResourceEx)(PVOID,DWORD,PVOID,DWORD*);
	static BOOL (WINAPI *pRemoveFontMemResourceEx)(HANDLE);
	CArray<HANDLE> m_Fonts;
	void InstallFonts();

	CAutoPtr<MatroskaReader::CMatroskaNode> m_pSegment, m_pCluster, m_pBlock;

protected:
	CAutoPtr<MatroskaReader::CMatroskaFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	CMap<DWORD, DWORD, MatroskaReader::TrackEntry*, MatroskaReader::TrackEntry*> m_pTrackEntryMap;
	CArray<MatroskaReader::TrackEntry* > m_pOrderedTrackArray;
	MatroskaReader::TrackEntry* GetTrackEntryAt(UINT aTrackIdx);

	bool InitDeliverLoop();
	void SeekDeliverLoop(REFERENCE_TIME rt);
	void DoDeliverLoop();

public:
	CMatroskaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CMatroskaSplitterFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IMediaSeeking

	STDMETHODIMP GetDuration(LONGLONG* pDuration);

	// IChapterInfo

	STDMETHODIMP_(UINT) GetChapterCount(UINT aChapterID);
	STDMETHODIMP_(UINT) GetChapterId(UINT aParentChapterId, UINT aIndex);
	STDMETHODIMP_(UINT) GetChapterCurrentId();
	STDMETHODIMP_(BOOL) GetChapterInfo(UINT aChapterID, struct ChapterElement* pStructureToFill);
	STDMETHODIMP_(BSTR) GetChapterStringInfo(UINT aChapterID, CHAR PreferredLanguage[3], CHAR CountryCode[2]);

	// IKeyFrameInfo

	STDMETHODIMP GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);

	// ITrackInfo

	STDMETHODIMP_(UINT) GetTrackCount();
	STDMETHODIMP_(BOOL) GetTrackInfo(UINT aTrackIdx, struct TrackElement* pStructureToFill);	
	STDMETHODIMP_(BOOL) GetTrackExtendedInfo(UINT aTrackIdx, void* pStructureToFill);
	STDMETHODIMP_(BSTR) GetTrackName(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecID(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecName(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecInfoURL(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecDownloadURL(UINT aTrackIdx);
};

[uuid("0A68C3B5-9164-4a54-AFAF-995B2FF0E0D4")]
class CMatroskaSourceFilter : public CMatroskaSplitterFilter
{
public:
	CMatroskaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif
};
