#pragma once

#include <atlbase.h>
#include <atlcoll.h>
#include <afxtempl.h>

namespace Matroska
{
	class CMatroskaNode;

	typedef unsigned __int64 QWORD;

	class CBinary : public CArray<BYTE>
	{
	public:
		CBinary& operator = (const CBinary& b) {Copy(b); return(*this);}
		operator BYTE*() {return (BYTE*)GetData();}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class CANSI : public CStringA {public: HRESULT Parse(CMatroskaNode* pMN);};
	class CUTF8 : public CStringW {public: HRESULT Parse(CMatroskaNode* pMN);};

	template<class T, class BASE>
	class CSimpleVar
	{
	protected:
		T m_val;
	public:
		CSimpleVar(T val = 0) : m_val(val) {}
		BASE& operator = (const BASE& v) {m_val = v.m_val; return(*this);}
		BASE& operator = (T val) {m_val = val; return(*this);}
		operator T() {return m_val;}
		BASE& Set(T val) {m_val = val; return(*(BASE*)this);}
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

				enum {INVALIDDURATION = -1};

				Block() {BlockDuration.Set(INVALIDDURATION);}
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

					HRESULT Parse(CMatroskaNode* pMN);
				};

				class Audio
				{
				public:
					CFloat SamplingFrequency;
					CUInt Channels;
					CBinary ChannelPositions;
					CUInt BitDepth;

					HRESULT Parse(CMatroskaNode* pMN);
				};

			class TrackEntry
			{
			public:
				enum {TypeVideo = 1, TypeAudio = 2, TypeComplex = 3, TypeLogo = 0x10, TypeSubtitle = 0x11, TypeControl = 0x20};
				CUInt TrackNumber, TrackUID, TrackType;
				CUInt FlagEnabled, FlagDefault, FlagLacing;
				CUInt MinCache, MaxCache;
				CUInt DefaultDuration; // ? should be level 4 but no level 4 node includes it here
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
				enum {NoDesc = 0, VideoDesc = 1, AudioDesc = 2};
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
		// TODO: Chapters
		// TODO: Tags

		HRESULT Parse(CMatroskaNode* pMN);
		HRESULT ParseMinimal(CMatroskaNode* pMN);
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

		QWORD FindId(DWORD id, QWORD start = 0);

	public:
		CID m_id;
		CLength m_len;
		QWORD m_start;

		HRESULT Parse();

	public:
		CMatroskaNode(CMatroskaFile* pMF); // creates the root
		CMatroskaNode(CMatroskaNode* pParent);

		CAutoPtr<CMatroskaNode> Child(DWORD id = 0);
		bool Next(bool fSame = false);
		bool Find(DWORD id);

		HRESULT SeekTo(QWORD pos);
		QWORD GetPos(), GetLength();
		template <class T> HRESULT Read(T& var);
		HRESULT Read(BYTE* pData, QWORD len);
	};
}
