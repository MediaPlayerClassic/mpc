#pragma once

#include "File.h"
#include "Subtitle.h"

namespace ssf
{
	class SubtitleFile : public File
	{
		static LPCTSTR s_predef;

	public:
		struct SegmentItem
		{
			Definition* pDef;
			double start, stop;
		};

		class Segment : public CAtlList<SegmentItem>
		{
		public:
			double m_start, m_stop; 
			Segment() {}
			Segment(double start, double stop, const SegmentItem* si = NULL);
			Segment(const Segment& s);
			void operator = (const Segment& s);
		};

		class SegmentList : public CAtlList<Segment> 
		{
			CAtlArray<Segment*> m_index;
			size_t Index(bool fForce = false);

		public:
			void RemoveAll();
			void Insert(double start, double stop, Definition* pDef);
			void Lookup(double at, CAtlList<SegmentItem>& sis);
			bool Lookup(double at, size_t& k);
			const Segment* GetSegment(size_t k);
		};

		SegmentList m_segments;

	public:
		SubtitleFile();
		virtual ~SubtitleFile();

		void Parse(Stream& s);
		bool Lookup(double at, CAutoPtrList<Subtitle>& subs);
	};
}