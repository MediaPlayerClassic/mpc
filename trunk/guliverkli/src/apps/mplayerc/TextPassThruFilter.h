// Media Player Classic.  Copyright 2003 Gabest.
// http://www.gabest.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html

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