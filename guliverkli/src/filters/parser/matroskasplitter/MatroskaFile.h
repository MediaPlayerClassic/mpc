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

#include <atlbase.h>
#include <atlcoll.h>
#include <afxtempl.h>

namespace MatroskaReader
{
	class CMatroskaNode;

	typedef unsigned __int64 QWORD;

	class CBinary : public CArray<BYTE>
	{
	public:
		CBinary& operator = (const CBinary& b) {Copy(b); return(*this);}
		operator BYTE*() {return (BYTE*)GetData();}
		CStringA ToString() {return CStringA((LPCSTR)GetData(), GetCount());}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class CANSI : public CStringA {public: HRESULT Parse(CMatroskaNode* pMN);};
	class CUTF8 : public CStringW {public: HRESULT Parse(CMatroskaNode* pMN);};

	template<class T, class BASE>
	class CSimpleVar
	{
	protected:
		T m_val;
		bool m_fValid;
	public:
		CSimpleVar(T val = 0) : m_val(val), m_fValid(false) {}
		BASE& operator = (const BASE& v) {m_val = v.m_val; m_fValid = true; return(*this);}
		BASE& operator = (T val) {m_val = val; m_fValid = true; return(*this);}
		operator T() {return m_val;}
		BASE& Set(T val) {m_val = val; m_fValid = true; return(*(BASE*)this);}
		bool IsValid() {return m_fValid;}
		virtual HRESULT Parse(CMatroskaNode* pMN);
	};

	class CUInt : public CSimpleVar<UINT64, CUInt> {public: HRESULT Parse(CMatroskaNode* pMN);};
	class CInt : public CSimpleVar<INT64, CInt> {public: HRESULT Parse(CMatroskaNode* pMN);};
	class CByte : public CSimpleVar<BYTE, CByte> {};
	class CShort : public CSimpleVar<short, CShort> {};
	class CFloat : public CSimpleVar<float, CFloat> {};
	class CID : public CSimpleVar<DWORD, CID> {public: HRESULT Parse(CMatroskaNode* pMN);};
	class CLength : public CSimpleVar<UINT64, CLength> {public: HRESULT Parse(CMatroskaNode* pMN);};

	template<class T>
	class CNode : public CAutoPtrList<T> {public: HRESULT Parse(CMatroskaNode* pMN);};

	class EBML
	{
	public:
		CUInt EBMLVersion, EBMLReadVersion;
		CUInt EBMLMaxIDLength, EBMLMaxSizeLength;
		CANSI DocType;
		CUInt DocTypeVersion, DocTypeReadVersion;

		HRESULT Parse(CMatroskaNode* pMN);
	};

		class Info
		{
		public:
			CBinary SegmentUID, PrevUID, NextUID;
			CUTF8 SegmentFilename, PrevFilename, NextFilename;
			CUInt TimeCodeScale; // [ns], default: 1.000.000
			CFloat Duration;
			CInt DateUTC;
			CUTF8 Title, MuxingApp, WritingApp;

			Info() {TimeCodeScale.Set(1000000ui64);}
			HRESULT Parse(CMatroskaNode* pMN);
		};

			class SeekHead
			{
			public:
				CID SeekID;
				CUInt SeekPosition;

				HRESULT Parse(CMatroskaNode* pMN);
			};

		class Seek
		{
		public:
			CNode<SeekHead> SeekHeads;

			HRESULT Parse(CMatroskaNode* pMN);
		};

				class TimeSlice
				{
				public:
					CUInt LaceNumber, FrameNumber;
					CUInt Delay, Duration;

					HRESULT Parse(CMatroskaNode* pMN);
				};

			class Block
			{
			public:
				CLength TrackNumber;
				CInt TimeCode;
				CByte Lacing;
				CAutoPtrList<CBinary> BlockData;
//				BlockVirtual
				CUInt BlockDuration;
				CUInt ReferencePriority;
				CInt ReferenceBlock;
				CInt ReferenceVirtual;
				CBinary CodecState;
				CNode<TimeSlice> TimeSlices;

				HRESULT Parse(CMatroskaNode* pMN, bool fFull);
			};

			class CBlockNode : public CNode<Block>
			{
			public:
				HRESULT Parse(CMatroskaNode* pMN, bool fFull);
			};

		class Cluster
		{
		public:
			CUInt TimeCode, Position, PrevSize;
			CBlockNode Blocks;

			HRESULT Parse(CMatroskaNode* pMN);
			HRESULT ParseTimeCode(CMatroskaNode* pMN);
		};

				class Video
				{
				public:
					CUInt FlagInterlaced, StereoMode;
					CUInt PixelWidth, PixelHeight, DisplayWidth, DisplayHeight, DisplayUnit;
					CUInt AspectRatioType;
					CUInt ColourSpace;
					CFloat GammaValue;
					CFloat FramePerSec;

					HRESULT Parse(CMatroskaNode* pMN);
				};

				class Audio
				{
				public:
					CFloat SamplingFrequency;
					CUInt Channels;
					CBinary ChannelPositions;
					CUInt BitDepth;

					Audio() {SamplingFrequency.Set(8000.0); Channels.Set(1);}
					HRESULT Parse(CMatroskaNode* pMN);
				};

			class TrackEntry
			{
			public:
				enum {TypeVideo = 1, TypeAudio = 2, TypeComplex = 3, TypeLogo = 0x10, TypeSubtitle = 0x11, TypeControl = 0x20};
				CUInt TrackNumber, TrackUID, TrackType;
				CUInt FlagEnabled, FlagDefault, FlagLacing;
				CUInt MinCache, MaxCache;
				CUTF8 Name;
				CANSI Language;
				CBinary CodecID;
				CBinary CodecPrivate;
				CUTF8 CodecName;
				CUTF8 CodecSettings;
				CANSI CodecInfoURL;
				CANSI CodecDownloadURL;
				CUInt CodecDecodeAll;
				CUInt TrackOverlay;
				CUInt DefaultDuration;
				CFloat TrackTimecodeScale;
				enum {NoDesc = 0, DescVideo = 1, DescAudio = 2};
				int DescType;
				Video v;
				Audio a;

				TrackEntry() {DescType = NoDesc;}
				HRESULT Parse(CMatroskaNode* pMN);
			};

		class Track
		{
		public:
			CNode<TrackEntry> TrackEntries;

			HRESULT Parse(CMatroskaNode* pMN);
		};

				class CueReference
				{
				public:
					CUInt CueRefTime, CueRefCluster, CueRefNumber, CueRefCodecState;

					HRESULT Parse(CMatroskaNode* pMN);
				};

			class CueTrackPosition
			{
			public:
				CUInt CueTrack, CueClusterPosition, CueBlockNumber, CueCodecState;
				CNode<CueReference> CueReferences;

				HRESULT Parse(CMatroskaNode* pMN);
			};

		class CuePoint
		{
		public:
			CUInt CueTime;
			CNode<CueTrackPosition> CueTrackPositions;

			HRESULT Parse(CMatroskaNode* pMN);
		};

		class Cue
		{
		public:
			CNode<CuePoint> CuePoints;

			HRESULT Parse(CMatroskaNode* pMN);
		};

			class AttachedFile
			{
			public:
				CUTF8 FileDescription;
				CUTF8 FileName;
				CANSI FileMimeType;
				QWORD FileDataPos, FileDataLen; // BYTE* FileData

				AttachedFile() {FileDataPos = FileDataLen = 0;}
				HRESULT Parse(CMatroskaNode* pMN);
			};

		class Attachment
		{
		public:
			CNode<AttachedFile> AttachedFiles;

			HRESULT Parse(CMatroskaNode* pMN);
		};

					class ChapterDisplay
					{
					public:
						CUTF8 ChapString;
						CANSI ChapLanguage;
						CANSI ChapCountry;

						ChapterDisplay() {ChapLanguage.CStringA::operator = ("eng");}
						HRESULT Parse(CMatroskaNode* pMN);
					};

				class ChapterAtom
				{
				public:
					CUInt ChapterUID;
					CUInt ChapterTimeStart, ChapterTimeEnd;
//					CNode<CUInt> ChapterTracks; // TODO
					CNode<ChapterDisplay> ChapterDisplays;
					CNode<ChapterAtom> ChapterAtoms;
					
					ChapterAtom() {ChapterUID.Set(rand());}
					HRESULT Parse(CMatroskaNode* pMN);
					ChapterAtom* FindChapterAtom(UINT64 id);
				};

			class EditionEntry : public ChapterAtom
			{
			public:
				HRESULT Parse(CMatroskaNode* pMN);
			};

		class Chapter
		{
		public:
			CNode<EditionEntry> EditionEntries;

			HRESULT Parse(CMatroskaNode* pMN);
		};

	class Segment
	{
	public:
		QWORD pos;
		Info SegmentInfo;
		CNode<Seek> MetaSeekInfo;
		CNode<Cluster> Clusters;
		CNode<Track> Tracks;
		CNode<Cue> Cues;
		CNode<Attachment> Attachments;
		CNode<Chapter> Chapters;
		// TODO: Chapters
		// TODO: Tags

		HRESULT Parse(CMatroskaNode* pMN);
		HRESULT ParseMinimal(CMatroskaNode* pMN);

		UINT64 GetMasterTrack();

		REFERENCE_TIME GetRefTime(INT64 t) {return t*(REFERENCE_TIME)(SegmentInfo.TimeCodeScale)/100;}
		ChapterAtom* FindChapterAtom(UINT64 id, int nEditionEntry = 0);
	};

	class CMatroskaFile
	{
	protected:
		CComPtr<IAsyncReader> m_pAsyncReader;
		QWORD m_pos, m_length;

	public:
		CMatroskaFile(IAsyncReader* pAsyncReader, HRESULT& hr);
		virtual ~CMatroskaFile() {}

		HRESULT SeekTo(QWORD pos);
		QWORD GetPos(), GetLength();
		template <class T> HRESULT Read(T& var);
		HRESULT Read(BYTE* pData, QWORD len);

		EBML m_ebml;
		Segment m_segment;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class CMatroskaNode
	{
		CMatroskaNode* m_pParent;
		CMatroskaFile* m_pMF;

	public:
		CID m_id;
		CLength m_len;
		QWORD m_filepos, m_start;

		HRESULT Parse();

	public:
		CMatroskaNode(CMatroskaFile* pMF); // creates the root
		CMatroskaNode(CMatroskaNode* pParent);

		CMatroskaNode* Parent() {return m_pParent;}
		CAutoPtr<CMatroskaNode> Child(DWORD id = 0, bool fSearch = true);
		bool Next(bool fSame = false);
		bool Find(DWORD id, bool fSearch = true);

		QWORD FindPos(DWORD id, QWORD start = 0);

		HRESULT SeekTo(QWORD pos);
		QWORD GetPos(), GetLength();
		template <class T> HRESULT Read(T& var);
		HRESULT Read(BYTE* pData, QWORD len);

		CAutoPtr<CMatroskaNode> Copy();
	};
}
