#pragma once

#include "..\..\subtitles\STS.h"

namespace DSObjects
{

[uuid("E2BA9B7B-B65D-4804-ACB2-89C3E55511DB")]
class CTextPassThruFilter : public CTransformFilter
{
	REFERENCE_TIME m_rtOffset;

	CMainFrame* m_pMainFrame;
	CSimpleTextSubtitle* m_pSTS;
	CCritSec* m_pSubLock;

	void SetName();

public:
	CTextPassThruFilter(CMainFrame* pMainFrame, CSimpleTextSubtitle* pSTS, CCritSec* pSubLock);
	virtual ~CTextPassThruFilter();

    HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
	HRESULT CompleteConnect(PIN_DIRECTION dir, IPin* pReceivePin);
    HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
};

}
using namespace DSObjects;