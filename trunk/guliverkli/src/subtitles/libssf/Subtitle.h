#pragma once

#include "File.h"

namespace ssf
{
	struct Point {double x, y; bool auto_x, auto_y;};
	struct Size {double cx, cy;};
	struct Rect {double t, r, b, l;};
	struct Align {double v, h;};
	struct Angle {double x, y, z;};
	struct Frame {CString reference; Size resolution;};
	struct Direction {CString primary, secondary;};
	struct Time {double start, stop;};
	struct Background {double color[4], size; CString type;};
	struct Shadow {double color[4], depth, angle, blur;};
	struct Placement {Rect clip, margin; Align align; Point pos, offset; Angle angle; Point TopLeft(Rect r, Size s);};

	struct Font
	{
		CString face;
		double size, weight;
		double color[4];
		bool underline, strikethrough, italic;
		double spacing;
		Size scale;
	};

	struct Fill
	{
		static unsigned int gen_id;
		int id;
		double color[4];
		double width;
		struct Fill() : id(0) {}
	};

	struct Style
	{
		CString linebreak;
		Placement placement;
		Font font;
		Background background;
		Shadow shadow;
		Fill fill;
	};

	struct Text
	{
		enum {SP = 0x20, NBSP = 0xa0, LSEP = 0x0a};
		Style style;
		CString str;
	};

	class Subtitle
	{
		static struct n2n_t {CAtlStringMap<double> align[2], weight, transition;} m_n2n;

		File* m_pFile;

		void GetStyle(Definition* pDef, Style& style);
		double GetMixWeight(Definition* pDef, double at, CAtlStringMap<double>& offset, int default_id);
		template<class T> bool MixValue(Definition& def, T& value, double t);
		template<> bool MixValue(Definition& def, double& value, double t);
		template<class T> bool MixValue(Definition& def, T& value, double t, CAtlStringMap<T>* n2n);
		template<> bool MixValue(Definition& def, double& value, double t, CAtlStringMap<double>* n2n);
		void MixStyle(Definition* pDef, Style& dst, double t);

		void Parse(Stream& s, Style style, double at, CAtlStringMap<double> offset);

		void AddChar(Text& t, TCHAR c);
		void AddText(Text& t);

	public:
		Frame m_frame;
		Direction m_direction;
		CString m_wrap;
		double m_layer;
		Time m_time;
		CAtlList<Text> m_text;

	public:
		Subtitle(File* pFile);
		virtual ~Subtitle();

		bool Parse(Definition* pDef, double start, double stop, double at);
	};
};