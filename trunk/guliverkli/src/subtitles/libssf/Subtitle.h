/* 
 *	Copyright (C) 2003-2006 Gabest
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

#include "File.h"

namespace ssf
{
	struct Point {double x, y; bool auto_x, auto_y;};
	struct Size {double cx, cy;};
	struct Rect {double t, r, b, l;};
	struct Align {double v, h;};
	struct Angle {double x, y, z;};
	struct Color {double a, r, g, b;};
	struct Frame {CStringW reference; Size resolution;};
	struct Direction {CStringW primary, secondary;};
	struct Time {double start, stop;};
	struct Background {Color color; double size; CStringW type;};
	struct Shadow {Color color; double depth, angle, blur;};
	struct Placement {Rect clip, margin; Align align; Point pos, offset; Angle angle;};

	struct Font
	{
		CStringW face;
		double size, weight;
		Color color;
		bool underline, strikethrough, italic;
		double spacing;
		Size scale;
	};

	struct Fill
	{
		static unsigned int gen_id;
		int id;
		Color color;
		double width;
		struct Fill() : id(0) {}
	};

	struct Style
	{
		CStringW linebreak;
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
		CStringW str;
	};

	class Subtitle
	{
		static struct n2n_t {CAtlStringMapW<double> align[2], weight, transition;} m_n2n;

		File* m_pFile;

		void GetStyle(Definition* pDef, Style& style);
		double GetMixWeight(Definition* pDef, double at, CAtlStringMapW<double>& offset, int default_id);
		template<class T> bool MixValue(Definition& def, T& value, double t);
		template<> bool MixValue(Definition& def, double& value, double t);
		template<class T> bool MixValue(Definition& def, T& value, double t, CAtlStringMapW<T>* n2n);
		template<> bool MixValue(Definition& def, double& value, double t, CAtlStringMapW<double>* n2n);
		void MixStyle(Definition* pDef, Style& dst, double t);

		void Parse(Stream& s, Style style, double at, CAtlStringMapW<double>& offset, Reference* pParentRef);

		void AddChar(Text& t, WCHAR c);
		void AddText(Text& t);

	public:
		Frame m_frame;
		Direction m_direction;
		CStringW m_wrap;
		double m_layer;
		Time m_time;
		CAtlList<Text> m_text;

	public:
		Subtitle(File* pFile);
		virtual ~Subtitle();

		bool Parse(Definition* pDef, double start, double stop, double at);
	};
};