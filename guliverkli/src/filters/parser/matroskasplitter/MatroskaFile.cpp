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
#include "MatroskaFile.h"
#include "..\..\..\DSUtil\DSUtil.h"

static void LOG(LPCTSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	if(FILE* f = _tfopen(_T("c:\\matroskasplitterlog.txt"), _T("at")))
	{
		fseek(f, 0, 2);
		_vftprintf(f, fmt, args);
		fclose(f);
	}
	va_end(args);
}

using namespace MatroskaReader;

#define BeginChunk	\
	CheckPointer(pMN0, E_POINTER); \
\
	CAutoPtr<CMatroskaNode> pMN = pMN0->Child(); \
	if(!pMN) return S_FALSE; \
\
	do \
	{ \
		switch(pMN->m_id) \
		{ \

#define EndChunk \
		} \
	} \
	while(pMN->Next()); \
\
	return S_OK; \

static void bswap(BYTE* s, int len)
{
	for(BYTE* d = s + len-1; s < d; s++, d--)
		*s ^= *d, *d ^= *s, *s ^= *d;
}

//
// CMatroskaFile
//

CMatroskaFile::CMatroskaFile(IAsyncReader* pAsyncReader, HRESULT& hr) 
	: m_pAsyncReader(pAsyncReader)
	, m_pos(0), m_length(0)
	, m_cachepos(0), m_cachelen(0)
{
	if(!pAsyncReader)
	{
		hr = E_POINTER; 
		return;
	}

	LONGLONG total, available;
	if(FAILED(m_pAsyncReader->Length(&total, &available)) || total != available || total < 0)
	{
		hr = E_FAIL;
		return;
	}

	m_pos = 0;
	m_length = (QWORD)total;

	DWORD dw;
	if(FAILED(Read(dw)) || dw != 0x1A45DFA3)
	{
		hr = E_FAIL;
		return;
	}

	CMatroskaNode Root(this);
	if(FAILED(hr = Parse(&Root)))
		return;
}

QWORD CMatroskaFile::GetPos()
{
	return m_pos;
}

QWORD CMatroskaFile::GetLength()
{
	return m_length;
}

HRESULT CMatroskaFile::SeekTo(QWORD pos)
{
	CheckPointer(m_pAsyncReader, E_NOINTERFACE);
//	if(m_pos != pos) LOG(_T("SeekTo [%I64d] -> [%I64d]\n"), m_pos, pos);
	m_pos = pos;
	return S_OK;
}

template <class T>
HRESULT CMatroskaFile::Read(T& var)
{
	HRESULT hr = Read((BYTE*)&var, sizeof(var));
	if(S_OK == hr) bswap((BYTE*)&var, sizeof(var));
	return hr;
}

HRESULT CMatroskaFile::Read(BYTE* pData, QWORD len)
{
	CheckPointer(m_pAsyncReader, E_NOINTERFACE);
	ASSERT(len <= LONG_MAX);

	HRESULT hr = S_OK;

	if(m_cachepos <= m_pos && m_pos < m_cachepos+m_cachelen)
	{
		QWORD minlen = min(len, m_cachelen - (m_pos-m_cachepos));
		memcpy(pData, &m_cache[m_pos - m_cachepos], (size_t)minlen);

		len -= minlen;
		m_pos += minlen;
		pData += minlen;
	}

	while(len > sizeof(m_cache))
	{
//LOG(_T("Read [%I64d] <-> [%I64d]\n"), m_pos, m_pos+sizeof(m_cache));
		hr = m_pAsyncReader->SyncRead(m_pos, sizeof(m_cache), pData);
		if(S_OK != hr) return hr;

		len -= sizeof(m_cache);
		m_pos += sizeof(m_cache);
		pData += sizeof(m_cache);
	}

	while(len > 0)
	{
		QWORD maxlen = min(m_length - m_pos, sizeof(m_cache));
		QWORD minlen = min(len, maxlen);
		if(minlen <= 0) return S_FALSE;
//LOG(_T("Read [%I64d] <-> [%I64d]\n"), m_pos, m_pos+maxlen);
		hr = m_pAsyncReader->SyncRead(m_pos, (LONG)maxlen, m_cache);
		if(S_OK != hr) return hr;

		m_cachepos = m_pos;
		m_cachelen = maxlen;

		memcpy(pData, m_cache, (size_t)minlen);
		len -= minlen;
		m_pos += minlen;
		pData += minlen;
	}

//	HRESULT hr = m_pAsyncReader->SyncRead(m_pos, (LONG)len, pData);
//LOG(_T("Read [%I64d] <-> [%I64d]\n"), m_pos, m_pos+len);
//	if(S_OK == hr) m_pos += len;
	return hr;
}

HRESULT CMatroskaFile::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x1A45DFA3: m_ebml.Parse(pMN); break;
	case 0x18538067: m_segment.ParseMinimal(pMN); break;
	EndChunk
}

//

HRESULT EBML::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x4286: EBMLVersion.Parse(pMN); break;
	case 0x42F7: EBMLReadVersion.Parse(pMN); break;
	case 0x42F2: EBMLMaxIDLength.Parse(pMN); break;
	case 0x42F3: EBMLMaxSizeLength.Parse(pMN); break;
	case 0x4282: DocType.Parse(pMN); break;
	case 0x4287: DocTypeVersion.Parse(pMN); break;
	case 0x4285: DocTypeReadVersion.Parse(pMN); break;
	EndChunk
}

HRESULT Segment::Parse(CMatroskaNode* pMN0)
{
	pos = pMN0->GetPos();

	BeginChunk
	case 0x1549A966: SegmentInfo.Parse(pMN); break;
	case 0x114D9B74: MetaSeekInfo.Parse(pMN); break;
	case 0x1654AE6B: Tracks.Parse(pMN); break;
	case 0x1F43B675: Clusters.Parse(pMN); break;
	case 0x1C53BB6B: Cues.Parse(pMN); break;
	case 0x1941A469: Attachments.Parse(pMN); break;
	case 0x1043A770: Chapters.Parse(pMN); break;
//	case 0x1254C367: Tags.Parse(pMN); break;
	EndChunk
}

HRESULT Segment::ParseMinimal(CMatroskaNode* pMN0)
{
	CheckPointer(pMN0, E_POINTER);

	pos = pMN0->GetPos();

	CAutoPtr<CMatroskaNode> pMN = pMN0->Child();
	if(!pMN) return S_FALSE;

	int n = 0;

	do
	{
		switch(pMN->m_id)
		{
		case 0x1549A966: SegmentInfo.Parse(pMN); n++; break;
		case 0x114D9B74: MetaSeekInfo.Parse(pMN); n++; break;
		case 0x1654AE6B: Tracks.Parse(pMN); n++; break;
		case 0x1C53BB6B: Cues.Parse(pMN); break;
		}
	}
	while(n < 3 && pMN->Next());

	while(QWORD pos = pMN->FindPos(0x114D9B74, pMN->GetPos()))
	{
		pMN->SeekTo(pos);
		pMN->Parse();
		if(pMN->m_id != 0x114D9B74) {ASSERT(0); break;}
		MetaSeekInfo.Parse(pMN);
	}

	if(n == 3)
	{
		if(Cues.IsEmpty() && (pMN = pMN0->Child(0x1C53BB6B, false)))
		{
			do {Cues.Parse(pMN);} while(pMN->Next(true));
		}

		if(Chapters.IsEmpty() && (pMN = pMN0->Child(0x1043A770, false)))
		{
			do {Chapters.Parse(pMN); /*BIG UGLY HACK:*/ break;} while(pMN->Next(true));
		}
	}

	return S_OK;
}

UINT64 Segment::GetMasterTrack()
{
	UINT64 TrackNumber = 0, AltTrackNumber = 0;

	POSITION pos1 = Tracks.GetHeadPosition();
	while(pos1 && TrackNumber == 0)
	{
		Track* pT = Tracks.GetNext(pos1);
		
		POSITION pos2 = pT->TrackEntries.GetHeadPosition();
		while(pos2 && TrackNumber == 0)
		{
			TrackEntry* pTE = pT->TrackEntries.GetNext(pos2);

			if(pTE->TrackType == TrackEntry::TypeVideo)
			{
				TrackNumber = pTE->TrackNumber;
				break;
			}
			else if(pTE->TrackType == TrackEntry::TypeAudio && AltTrackNumber == 0)
			{
				AltTrackNumber = pTE->TrackNumber;
			}
		}
	}

	if(TrackNumber == 0) TrackNumber = AltTrackNumber;
	if(TrackNumber == 0) TrackNumber = 1;

	return TrackNumber;
}

ChapterAtom* ChapterAtom::FindChapterAtom(UINT64 id)
{
	if(ChapterUID == id)
		return(this);

	POSITION pos = ChapterAtoms.GetHeadPosition();
	while(pos)
	{
		ChapterAtom* ca = ChapterAtoms.GetNext(pos)->FindChapterAtom(id);
		if(ca) return ca;
	}

	return(NULL);
}

ChapterAtom* Segment::FindChapterAtom(UINT64 id, int nEditionEntry)
{
	POSITION pos1 = Chapters.GetHeadPosition();
	while(pos1)
	{
		Chapter* c = Chapters.GetNext(pos1);

		POSITION pos2 = c->EditionEntries.GetHeadPosition();
		while(pos2)
		{
			EditionEntry* ee = c->EditionEntries.GetNext(pos2);

			if(nEditionEntry-- == 0)
			{
				return id == 0 ? ee : ee->FindChapterAtom(id);
			}
		}
	}

	return(NULL);
}

HRESULT Info::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x73A4: SegmentUID.Parse(pMN); break;
	case 0x7384: SegmentFilename.Parse(pMN); break;
	case 0x3CB923: PrevUID.Parse(pMN); break;
	case 0x3C83AB: PrevFilename.Parse(pMN); break;
	case 0x3EB923: NextUID.Parse(pMN); break;
	case 0x3E83BB: NextFilename.Parse(pMN); break;
	case 0x2AD7B1: TimeCodeScale.Parse(pMN); break;
	case 0x4489: Duration.Parse(pMN); break;
	case 0x4461: DateUTC.Parse(pMN); break;
	case 0x7BA9: Title.Parse(pMN); break;
	case 0x4D80: MuxingApp.Parse(pMN); break;
	case 0x5741: WritingApp.Parse(pMN); break;
	EndChunk
}

HRESULT Seek::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x4DBB: SeekHeads.Parse(pMN); break;
	EndChunk
}

HRESULT SeekHead::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x53AB: SeekID.Parse(pMN); break;
	case 0x53AC: SeekPosition.Parse(pMN); break;
	EndChunk
}

HRESULT Track::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xAE: TrackEntries.Parse(pMN); break;
	EndChunk
}

HRESULT TrackEntry::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xD7: TrackNumber.Parse(pMN); break;
	case 0x73C5: TrackUID.Parse(pMN); break;
	case 0x83: TrackType.Parse(pMN); break;
	case 0xB9: FlagEnabled.Parse(pMN); break;
	case 0x88: FlagDefault.Parse(pMN); break;
	case 0x9C: FlagLacing.Parse(pMN); break;
	case 0x6DE7: MinCache.Parse(pMN); break;
	case 0x6DF8: MaxCache.Parse(pMN); break;
	case 0x536E: Name.Parse(pMN); break;
	case 0x22B59C: Language.Parse(pMN); break;
	case 0x86: CodecID.Parse(pMN); break;
	case 0x63A2: CodecPrivate.Parse(pMN); break;
	case 0x258688: CodecName.Parse(pMN); break;
	case 0x3A9697: CodecSettings.Parse(pMN); break;
	case 0x3B4040: CodecInfoURL.Parse(pMN); break;
	case 0x26B240: CodecDownloadURL.Parse(pMN); break;
	case 0xAA: CodecDecodeAll.Parse(pMN); break;
	case 0x6FAB: TrackOverlay.Parse(pMN); break;
	case 0x23E383: case 0x2383E3: DefaultDuration.Parse(pMN); break;
	case 0x23314F: TrackTimecodeScale.Parse(pMN); break;
	case 0xE0: if(S_OK == v.Parse(pMN)) DescType |= DescVideo; break;
	case 0xE1: if(S_OK == a.Parse(pMN)) DescType |= DescAudio; break;
	EndChunk
}

HRESULT Video::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x9A: FlagInterlaced.Parse(pMN); break;
	case 0x53B8: StereoMode.Parse(pMN); break;
	case 0xB0: PixelWidth.Parse(pMN); break;
	case 0xBA: PixelHeight.Parse(pMN); break;
	case 0x54B0: DisplayWidth.Parse(pMN); break;
	case 0x54BA: DisplayHeight.Parse(pMN); break;
	case 0x54B2: DisplayUnit.Parse(pMN); break;
	case 0x54B3: AspectRatioType.Parse(pMN); break;
	case 0x2EB524: ColourSpace.Parse(pMN); break;
	case 0x2FB523: GammaValue.Parse(pMN); break;
	case 0x2383E3: FramePerSec.Parse(pMN); break;
	EndChunk
}

HRESULT Audio::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xB5: SamplingFrequency.Parse(pMN); break;
	case 0x9F: Channels.Parse(pMN); break;
	case 0x7D7B: ChannelPositions.Parse(pMN); break;
	case 0x6264: BitDepth.Parse(pMN); break;
	EndChunk
}

HRESULT Cluster::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xE7: TimeCode.Parse(pMN); break;
	case 0xA7: Position.Parse(pMN); break;
	case 0xAB: PrevSize.Parse(pMN); break;
	case 0xA0: Blocks.Parse(pMN, true); break;
	EndChunk
}

HRESULT Cluster::ParseTimeCode(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xE7: TimeCode.Parse(pMN); return S_OK;
	EndChunk
}

HRESULT Block::Parse(CMatroskaNode* pMN0, bool fFull)
{
	BeginChunk
	case 0xA1:
	{
		TrackNumber.Parse(pMN); 
		CShort s; s.Parse(pMN); TimeCode.Set(s); 
		Lacing.Parse(pMN);
		
		if(!fFull) break;

		CList<QWORD> lens;
		QWORD tlen = 0;
		if(Lacing&0x02)
		{
			BYTE n;
			pMN->Read(n);
			while(n-- > 0)
			{
				BYTE b;
				QWORD len = 0;
				do {pMN->Read(b); len += b;} while(b == 0xff);
				lens.AddTail(len);
				tlen += len;
			}
		}
		lens.AddTail((pMN->m_start+pMN->m_len) - (pMN->GetPos()+tlen));

		POSITION pos = lens.GetHeadPosition();
		while(pos)
		{
			QWORD len = lens.GetNext(pos);
			CAutoPtr<CBinary> p(new CBinary());
			p->SetSize((INT_PTR)len);
			pMN->Read(p->GetData(), len);
			BlockData.AddTail(p);
		}

		break;
	}
	case 0xA2: /* TODO: multiple virt blocks? */; break;
	case 0x9B: BlockDuration.Parse(pMN); break;
	case 0xFA: ReferencePriority.Parse(pMN); break;
	case 0xFB: ReferenceBlock.Parse(pMN); break;
	case 0xFD: ReferenceVirtual.Parse(pMN); break;
	case 0xA4: CodecState.Parse(pMN); break;
	case 0xE8: TimeSlices.Parse(pMN); break;
	EndChunk
}

HRESULT TimeSlice::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xCC: LaceNumber.Parse(pMN); break;
	case 0xCD: FrameNumber.Parse(pMN); break;
	case 0xCE: Delay.Parse(pMN); break;
	case 0xCF: Duration.Parse(pMN); break;
	EndChunk
}

HRESULT Cue::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xBB: CuePoints.Parse(pMN); break;
	EndChunk
}

HRESULT CuePoint::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xB3: CueTime.Parse(pMN); break;
	case 0xB7: CueTrackPositions.Parse(pMN); break;
	EndChunk
}

HRESULT CueTrackPosition::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xF7: CueTrack.Parse(pMN); break;
	case 0xF1: CueClusterPosition.Parse(pMN); break;
	case 0x5387: CueBlockNumber.Parse(pMN); break;
	case 0xEA: CueCodecState.Parse(pMN); break;
	case 0xDB: CueReferences.Parse(pMN); break;
	EndChunk
}

HRESULT CueReference::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x96: CueRefTime.Parse(pMN); break;
	case 0x97: CueRefCluster.Parse(pMN); break;
	case 0x535F: CueRefNumber.Parse(pMN); break;
	case 0xEB: CueRefCodecState.Parse(pMN); break;
	EndChunk
}

HRESULT Attachment::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
		// TODO
	EndChunk
}

HRESULT AttachedFile::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
		// TODO
	EndChunk
}

HRESULT Chapter::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x45B9: EditionEntries.Parse(pMN); break;
	EndChunk
}

HRESULT EditionEntry::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xB6: ChapterAtoms.Parse(pMN); break;
	EndChunk
}

HRESULT ChapterAtom::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x73C4: ChapterUID.Parse(pMN); break;
	case 0x91: ChapterTimeStart.Parse(pMN); break;
	case 0x92: ChapterTimeEnd.Parse(pMN); break;
//	case 0x8F: // TODO 
	case 0x80: ChapterDisplays.Parse(pMN); break;
	case 0xB6: ChapterAtoms.Parse(pMN); break;
	EndChunk
}

HRESULT ChapterDisplay::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x85: ChapString.Parse(pMN); break;
	case 0x437C: ChapLanguage.Parse(pMN); break;
	case 0x437E: ChapCountry.Parse(pMN); break;
	EndChunk
}

//

HRESULT CBinary::Parse(CMatroskaNode* pMN)
{
	ASSERT(pMN->m_len <= INT_MAX);
	SetSize((INT_PTR)pMN->m_len);
	return pMN->Read(GetData(), pMN->m_len);
}

HRESULT CANSI::Parse(CMatroskaNode* pMN)
{
	Empty();

	QWORD len = pMN->m_len;
	CHAR c;
	while(len-- > 0 && SUCCEEDED(pMN->Read(c))) 
		*this += c;

	return(len == -1 ? S_OK : E_FAIL);
}

HRESULT CUTF8::Parse(CMatroskaNode* pMN)
{
	Empty();
	CAutoVectorPtr<BYTE> buff;
	if(!buff.Allocate((UINT)pMN->m_len + 1) || S_OK != pMN->Read(buff, pMN->m_len))
		return E_FAIL;
	buff[pMN->m_len] = 0;
	CStringW::operator = (UTF8To16((LPCSTR)(BYTE*)buff));
	return S_OK;
}

HRESULT CUInt::Parse(CMatroskaNode* pMN)
{
	m_val = 0;
	for(int i = 0; i < (int)pMN->m_len; i++)
	{
		m_val <<= 8;
		HRESULT hr = pMN->Read(*(BYTE*)&m_val);
		if(FAILED(hr)) return hr;
	}
	m_fValid = true;
	return S_OK;
}

HRESULT CInt::Parse(CMatroskaNode* pMN)
{
	m_val = 0;
	for(int i = 0; i < (int)pMN->m_len; i++)
	{
		HRESULT hr = pMN->Read(*((BYTE*)&m_val+7-i));
		if(FAILED(hr)) return hr;
	}
	m_val >>= (8-pMN->m_len)*8;
	m_fValid = true;
	return S_OK;
}


template<class T, class BASE>
HRESULT CSimpleVar<T, BASE>::Parse(CMatroskaNode* pMN)
{
	m_val = 0;
	m_fValid = true;
	return pMN->Read(m_val);
}

HRESULT CID::Parse(CMatroskaNode* pMN)
{
	m_val = 0;

	BYTE b = 0;
	HRESULT hr = pMN->Read(b);
	if(FAILED(hr)) return hr;

	int nMoreBytes = 0;

	if((b&0x80) == 0x80) {m_val = b&0xff; nMoreBytes = 0;}
	else if((b&0xc0) == 0x40) {m_val = b&0x7f; nMoreBytes = 1;}
	else if((b&0xe0) == 0x20) {m_val = b&0x3f; nMoreBytes = 2;}
	else if((b&0xf0) == 0x10) {m_val = b&0x1f; nMoreBytes = 3;}
	else return E_FAIL;

	while(nMoreBytes-- > 0)
	{
		m_val <<= 8;
		hr = pMN->Read(*(BYTE*)&m_val);
		if(FAILED(hr)) return hr;
	}

	m_fValid = true;

	return S_OK;
}

HRESULT CLength::Parse(CMatroskaNode* pMN)
{
	m_val = 0;

	BYTE b = 0;
	HRESULT hr = pMN->Read(b);
	if(FAILED(hr)) return hr;

	int nMoreBytes = 0, nMoreBytesTmp = 0;

	if((b&0x80) == 0x80) {m_val = b&0x7f; nMoreBytes = 0;}
	else if((b&0xc0) == 0x40) {m_val = b&0x3f; nMoreBytes = 1;}
	else if((b&0xe0) == 0x20) {m_val = b&0x1f; nMoreBytes = 2;}
	else if((b&0xf0) == 0x10) {m_val = b&0x0f; nMoreBytes = 3;}
	else if((b&0xf8) == 0x08) {m_val = b&0x07; nMoreBytes = 4;}
	else if((b&0xfc) == 0x04) {m_val = b&0x03; nMoreBytes = 5;}
	else if((b&0xfe) == 0x02) {m_val = b&0x01; nMoreBytes = 6;}
	else if((b&0xff) == 0x01) {m_val = b&0x00; nMoreBytes = 7;}
	else return E_FAIL;

	nMoreBytesTmp = nMoreBytes;

	QWORD UnknownSize = (1i64<<(7*(nMoreBytes+1)))-1;

	while(nMoreBytes-- > 0)
	{
		m_val <<= 8;
		hr = pMN->Read(*(BYTE*)&m_val);
		if(FAILED(hr)) return hr;
	}

	if(m_val == UnknownSize)
	{
		m_val = pMN->GetLength() - pMN->GetPos();
		TRACE(_T("CLength: Unspecified chunk size at %I64d (corrected to %I64d)\n"), pMN->GetPos(), m_val);
	}

	m_fValid = true;

	return S_OK;
}

template<class T>
HRESULT CNode<T>::Parse(CMatroskaNode* pMN)
{
	CAutoPtr<T> p(new T());
	HRESULT hr = E_OUTOFMEMORY;
	if(!p || FAILED(hr = p->Parse(pMN))) return hr;
	AddTail(p);
	return S_OK;
}

HRESULT CBlockNode::Parse(CMatroskaNode* pMN, bool fFull)
{
	CAutoPtr<Block> p(new Block());
	HRESULT hr = E_OUTOFMEMORY;
	if(!p || FAILED(hr = p->Parse(pMN, fFull))) return hr;
	AddTail(p);
	return S_OK;
}

///////////////////////////////

CMatroskaNode::CMatroskaNode(CMatroskaFile* pMF)
	: m_pMF(pMF)
	, m_pParent(NULL)
{
	ASSERT(m_pMF);
	m_start = m_filepos = 0;
	m_len.Set(m_pMF ? m_pMF->GetLength() : 0); 
}

CMatroskaNode::CMatroskaNode(CMatroskaNode* pParent)
	: m_pMF(pParent->m_pMF)
	, m_pParent(pParent)
{
	Parse();
}

HRESULT CMatroskaNode::Parse()
{
	m_filepos = GetPos();
	if(FAILED(m_id.Parse(this)) || FAILED(m_len.Parse(this)))
		return E_FAIL;
	m_start = GetPos();
	return S_OK;
}

CAutoPtr<CMatroskaNode> CMatroskaNode::Child(DWORD id, bool fSearch)
{
	SeekTo(m_start);
	CAutoPtr<CMatroskaNode> pMN(new CMatroskaNode(this));
	if(id && !pMN->Find(id, fSearch)) pMN.Free();
	return pMN;
}

bool CMatroskaNode::Next(bool fSame)
{
	if(!m_pParent)
		return(false);

	CID id = m_id;

	while(SUCCEEDED(SeekTo(m_start+m_len)) && GetPos() < m_pParent->m_start+m_pParent->m_len)
	{
		if(FAILED(Parse()))
		{
			if(!Resync())
				return(false);
		}

		if(!fSame || m_id == id) 
			return(true);
	}

	return(false);
}

bool CMatroskaNode::Find(DWORD id, bool fSearch)
{
	QWORD pos = m_pParent && m_pParent->m_id == 0x18538067 /*segment?*/ 
		? FindPos(id) 
		: 0;

	if(pos && SUCCEEDED(SeekTo(pos)))
	{
		Parse();
	}
	else if(fSearch)
	{
		while(m_id != id && Next());
	}

	return(m_id == id);
}

HRESULT CMatroskaNode::SeekTo(QWORD pos) {return m_pMF->SeekTo(pos);}
QWORD CMatroskaNode::GetPos() {return m_pMF->GetPos();}
QWORD CMatroskaNode::GetLength() {return m_pMF->GetLength();}
template <class T> 
HRESULT CMatroskaNode::Read(T& var) {return m_pMF->Read(var);}
HRESULT CMatroskaNode::Read(BYTE* pData, QWORD len) {return m_pMF->Read(pData, len);}

QWORD CMatroskaNode::FindPos(DWORD id, QWORD start)
{
	Segment& sm = m_pMF->m_segment;

	POSITION pos = sm.MetaSeekInfo.GetHeadPosition();
	while(pos)
	{
		Seek* s = sm.MetaSeekInfo.GetNext(pos);

		POSITION pos2 = s->SeekHeads.GetHeadPosition();
		while(pos2)
		{
			SeekHead* sh = s->SeekHeads.GetNext(pos2);
			if(sh->SeekID == id && sh->SeekPosition+sm.pos >= start)
				return sh->SeekPosition+sm.pos;
		}
	}

	return 0;
}

CAutoPtr<CMatroskaNode> CMatroskaNode::Copy()
{
	CAutoPtr<CMatroskaNode> pNewNode(new CMatroskaNode(m_pMF));
	pNewNode->m_pParent = m_pParent;
	pNewNode->m_id.Set(m_id);
	pNewNode->m_len.Set(m_len);
	pNewNode->m_filepos = m_filepos;
	pNewNode->m_start = m_start;
	return(pNewNode);
}

bool CMatroskaNode::Resync()
{
	if(m_pParent->m_id == 0x18538067) /*segment?*/ 
	{
		SeekTo(m_filepos);

		for(BYTE b = 0; S_OK == Read(b); b = 0)
		{
			if((b&0xf0) != 0x10)
				continue;

            DWORD dw = b;
			Read((BYTE*)&dw+1, 3);
			bswap((BYTE*)&dw, 4);

			switch(dw)
			{
			case 0x1549A966: // SegmentInfo
			case 0x114D9B74: // MetaSeekInfo
			case 0x1654AE6B: // Tracks
			case 0x1F43B675: // Clusters
			case 0x1C53BB6B: // Cues
			case 0x1941A469: // Attachments
			case 0x1043A770: // Chapters
			case 0x1254C367: // Tags
				SeekTo(GetPos()-4);
				return(SUCCEEDED(Parse()));
			default:
				SeekTo(GetPos()-3);
				break;
			}
		}
	}

	return(false);
}
