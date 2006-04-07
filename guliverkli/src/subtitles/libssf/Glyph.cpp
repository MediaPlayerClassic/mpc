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

#include "stdafx.h"
#include "Glyph.h"

#define deg2rad(d) (float)(M_PI/180*(d))

namespace ssf
{
	Glyph::Glyph()
	{
		c = 0;
		font = NULL;
		ascent = descent = width = spacing = fill = 0;
		tl.x = tl.y = tls.x = tls.y = 0;
	}

	float Glyph::GetBackgroundSize() const
	{
		return style.background.size * (scale.cx + scale.cy) / 2;
	}

	float Glyph::GetShadowDepth() const
	{
		return style.shadow.depth * (scale.cx + scale.cy) / 2;
	}

	CRect Glyph::GetClipRect() const
	{
		CRect r = bbox + tl;

		int size = (int)(GetBackgroundSize() + 0.5);
		int depth = (int)(GetShadowDepth() + 0.5);

		r.InflateRect(size, size);
		r.InflateRect(depth, depth);

		r.left >>= 6;
		r.top >>= 6;
		r.right = (r.right + 32) >> 6;
		r.bottom = (r.bottom + 32) >> 6;

		return r;
	}

	void Glyph::Transform(GlyphPath& path, CPoint org)
	{
		// TODO: add sse code path

		float sx = style.font.scale.cx;
		float sy = style.font.scale.cy;

		bool rotate = style.placement.angle.x || style.placement.angle.y || style.placement.angle.z;
		bool scale = rotate || sx != 1 || sy != 1;

		float caz = cos(deg2rad(style.placement.angle.z));
		float saz = sin(deg2rad(style.placement.angle.z));
		float cax = cos(deg2rad(style.placement.angle.x));
		float sax = sin(deg2rad(style.placement.angle.x));
		float cay = cos(deg2rad(style.placement.angle.y));
		float say = sin(deg2rad(style.placement.angle.y));

		for(size_t i = 0, j = path.types.GetCount(); i < j; i++)
		{
			CPoint p = path.points[i];

			if(scale)
			{
				float x, y, z, xx, yy, zz;

				x = sx * (p.x - org.x);
				y = sy * (p.y - org.y);
				z = 0;

				if(rotate)
				{
					xx = x*caz + y*saz;
					yy = -(x*saz - y*caz);
					zz = z;

					x = xx;
					y = yy*cax + zz*sax;
					z = yy*sax - zz*cax;

					xx = x*cay + z*say;
					yy = y;
					zz = x*say - z*cay;

					zz = 1.0f / (max(zz, -19000) + 20000);

					x = (xx * 20000) * zz;
					y = (yy * 20000) * zz;
				}

				p.x = (int)(x + org.x + 0.5);
				p.y = (int)(y + org.y + 0.5);
				
				path.points[i] = p;
			}

			if(p.x < bbox.left) bbox.left = p.x;
			if(p.x > bbox.right) bbox.right = p.x;
			if(p.y < bbox.top) bbox.top = p.y;
			if(p.y > bbox.bottom) bbox.bottom = p.y;
		}
	}

	void Glyph::Transform(CPoint org)
	{
		org -= tl;

		bbox.SetRect(INT_MAX, INT_MAX, INT_MIN, INT_MIN);

		if(style.background.type == L"enlarge" && style.background.size > 0)
		{
			path_bkg.Enlarge(path, GetBackgroundSize());
			Transform(path_bkg, org);
		}
		else if(style.background.type == L"box" && style.background.size >= 0)
		{
			if(c != ssf::Text::LSEP)
			{
				int s = (int)(GetBackgroundSize() + 0.5);
				int x0 = (!vertical ? -spacing/2 : ascent - row_ascent);
				int y0 = (!vertical ? ascent - row_ascent : -spacing/2);
				int x1 = x0 + (!vertical ? width + spacing : row_ascent + row_descent);
				int y1 = y0 + (!vertical ? row_ascent + row_descent : width + spacing);
				path_bkg.types.SetCount(4);
				path_bkg.types[0] = PT_MOVETO;
				path_bkg.types[1] = PT_LINETO;
				path_bkg.types[2] = PT_LINETO;
				path_bkg.types[3] = PT_LINETO;
				path_bkg.points.SetCount(4);
				path_bkg.points[0] = CPoint(x0-s, y0-s);
				path_bkg.points[1] = CPoint(x1+s, y0-s);
				path_bkg.points[2] = CPoint(x1+s, y1+s);
				path_bkg.points[3] = CPoint(x0-s, y1+s);
				Transform(path_bkg, org);
			}
		}

		Transform(path, org);

		bbox |= CRect(0, 0, 0, 0);
	}

	void Glyph::Rasterize()
	{
		Rasterizer* r = &ras;

		if(style.background.type == L"enlarge" && style.background.size > 0
		|| style.background.type == L"box" && style.background.size >= 0)
		{
			ras_bkg.ScanConvert(path_bkg, bbox);
			ras_bkg.Rasterize(tl.x >> 3, tl.y >> 3);
			ras_bkg.Blur(style.background.blur, 0);
			r = &ras_bkg;
		}

		ras.ScanConvert(path, bbox);

		if(style.background.type == L"outline" && style.background.size > 0)
		{
			ras.CreateWidenedRegion((int)(GetBackgroundSize() / 8));
		}
		
		ras.Rasterize(tl.x >> 3, tl.y >> 3);

		if(style.background.type == L"outline" && style.background.size > 0)
		{
			ras.Blur(style.background.blur, 1);
		}

		if(style.shadow.depth > 0)
		{
			ras_shadow.Reuse(*r);

			float depth = GetShadowDepth();

			tls.x = tl.x + (int)(depth * cos(deg2rad(style.shadow.angle)) + 0.5);
			tls.y = tl.y + (int)(depth * -sin(deg2rad(style.shadow.angle)) + 0.5);

			ras_shadow.Rasterize(tls.x >> 3, tls.y >> 3);

			if(style.background.type == L"enlarge" && style.background.size > 0
			|| style.background.type == L"box" && style.background.size >= 0)
			{
				ras_shadow.Blur(style.shadow.blur, 0);
			}
			else if(style.background.type == L"outline" && style.background.size > 0)
			{
				ras_shadow.Blur(style.shadow.blur, 1);
			}
		}
	}

}