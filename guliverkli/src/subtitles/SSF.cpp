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
 *  TODO: 
 *  - merge and draw glyphs together which have the same style (~ fix semi-transparent overlapping shapes)
 *  - fill effect
 *  - box background
 *  - collision detection
 *  - super slow, make it faster
 *
 */

#include "stdafx.h"
#include "SSF.h"
#include "..\subpic\MemSubPic.h"

//

HFONT CRenderedSSF::CFontCache::Create(const LOGFONT& lf)
{
	CString hash;

	hash.Format(_T("%s,%d,%d,%d,%d,%d"), 
		lf.lfFaceName, lf.lfHeight, lf.lfWeight, 
		lf.lfItalic, lf.lfUnderline, lf.lfStrikeOut);

	HFONT hFont;

	if(Lookup(hash, hFont))
	{
		return hFont;
	}

	if(!(hFont = CreateFontIndirect(&lf)))
	{
		ASSERT(0);
		return NULL;
	}

	if(GetCount() > 10) RemoveAll(); // LAME

	SetAt(hash, hFont);

	return hFont;
}

//

CRenderedSSF::CRenderedSSF(CCritSec* pLock)
	: ISubPicProviderImpl(pLock)
{
	m_hDC = CreateCompatibleDC(NULL);
	SetBkMode(m_hDC, TRANSPARENT); 
	SetTextColor(m_hDC, 0xffffff); 
	SetMapMode(m_hDC, MM_TEXT);
}

CRenderedSSF::~CRenderedSSF()
{
	DeleteDC(m_hDC);
}

bool CRenderedSSF::Open(CString fn, CString name)
{
	m_fn.Empty();
	m_name.Empty();
	m_psf.Free();

	if(name.IsEmpty())
	{
		CString str = fn;
		str.Replace('\\', '/');
		name = str.Left(str.ReverseFind('.'));
		name = name.Mid(name.ReverseFind('/')+1);
		name = name.Mid(name.ReverseFind('.')+1);
	}

	try
	{
		if(Open(ssf::FileStream(fn), name)) 
		{
			m_fn = fn;
			return true;
		}
	}
	catch(ssf::Exception& e)
	{
		TRACE(_T("%s\n"), e.ToString());
	}

	return false;	
}

bool CRenderedSSF::Open(ssf::Stream& s, CString name)
{
	m_fn.Empty();
	m_name.Empty();
	m_psf.Free();

	try
	{
		m_psf.Attach(new ssf::SubtitleFile());
		m_psf->Parse(s);

#if 0 // def DEBUG
		m_psf->Dump(ssf::PLow);
		double at = 0;
		for(int i = 9000; i < 12000; i += 10)
		{
			double at = (double)i/1000;
			CAutoPtrList<ssf::Subtitle> subs;
			m_psf->Lookup(at, subs);
			POSITION pos = subs.GetHeadPosition();
			while(pos)
			{
				const ssf::Subtitle* s = subs.GetNext(pos);

				POSITION pos = s->m_text.GetHeadPosition();
				while(pos)
				{
					const ssf::Text& t = s->m_text.GetNext(pos);
					TRACE(_T("%.3f: [%.2f] %s\n"), at, t.style.font.size, CString(t.str));
				}
			}
		}
#endif
		m_name = name;
		return true;
	}
	catch(ssf::Exception& e)
	{
		TRACE(_T("%s\n"), e.ToString());
	}

	return false;
}

STDMETHODIMP CRenderedSSF::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);
    *ppv = NULL;

    return 
		QI(IPersist)
		QI(ISubStream)
		QI(ISubPicProvider)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

STDMETHODIMP_(POSITION) CRenderedSSF::GetStartPosition(REFERENCE_TIME rt, double fps)
{
	size_t k;
	return m_psf && m_psf->m_segments.Lookup((double)rt/10000000, k) ? (POSITION)(++k) : NULL;
}

STDMETHODIMP_(POSITION) CRenderedSSF::GetNext(POSITION pos)
{
	size_t k = (size_t)pos;
	return m_psf && m_psf->m_segments.GetSegment(k) ? (POSITION)(++k) : NULL;
}

STDMETHODIMP_(REFERENCE_TIME) CRenderedSSF::GetStart(POSITION pos, double fps)
{
	size_t k = (size_t)pos-1;
	const ssf::SubtitleFile::Segment* s = m_psf ? m_psf->m_segments.GetSegment(k) : NULL;
	return s ? (REFERENCE_TIME)(s->m_start*10000000) : 0;
}

STDMETHODIMP_(REFERENCE_TIME) CRenderedSSF::GetStop(POSITION pos, double fps)
{
	CheckPointer(m_psf, 0);

	size_t k = (size_t)pos-1;
	const ssf::SubtitleFile::Segment* s = m_psf ? m_psf->m_segments.GetSegment(k) : NULL;
	return s ? (REFERENCE_TIME)(s->m_stop*10000000) : 0;
}

STDMETHODIMP_(bool) CRenderedSSF::IsAnimated(POSITION pos)
{
	return true;
}

// 

#include <math.h>
#include "SSFRasterizer.h"

#define deg2rad(d) (3.14159/180*(d))

class CGlyph
{
public:
	WCHAR c;
	const ssf::Style* style;
	int ascent, descent, width, spacing, fillwidth;
	int pathcount;
	CAutoVectorPtr<BYTE> pathtypes;
	CAutoVectorPtr<POINT> pathpoints;
	ssf::Point tl, tls;
	SSFRasterizer ras, ras2;

	CGlyph()
	{
		c = 0;
		style = NULL;
		ascent = descent = width = spacing = fillwidth = 0;
		pathcount = 0;
		tl.x = tl.y = tls.x = tls.y = 0;
	}

	void CGlyph::Rasterize(ssf::Point org, ssf::Size scale)
	{
		org.x -= tl.x;
		org.y -= tl.y;

		double sx = style->font.scale.cx;
		double sy = style->font.scale.cy;

		double caz = cos(deg2rad(style->placement.angle.z));
		double saz = sin(deg2rad(style->placement.angle.z));
		double cax = cos(deg2rad(style->placement.angle.x));
		double sax = sin(deg2rad(style->placement.angle.x));
		double cay = cos(deg2rad(style->placement.angle.y));
		double say = sin(deg2rad(style->placement.angle.y));

		for(int i = 0; i < pathcount; i++)
		{
			double x, y, z, xx, yy, zz;

			x = sx * (pathpoints[i].x - org.x);
			y = sy * (pathpoints[i].y - org.y);
			z = 0;

			xx = x*caz + y*saz;
			yy = -(x*saz - y*caz);
			zz = z;

			x = xx;
			y = yy*cax + zz*sax;
			z = yy*sax - zz*cax;

			xx = x*cay + z*say;
			yy = y;
			zz = x*say - z*cay;

			zz = max(zz, -19000);

			x = (xx * 20000) / (zz + 20000);
			y = (yy * 20000) / (zz + 20000);

			pathpoints[i].x = (LONG)(x + org.x + 0.5);
			pathpoints[i].y = (LONG)(y + org.y + 0.5);
		}

		ras.ScanConvert(pathcount, pathtypes, pathpoints);

		if(style->background.type == L"outline" && style->background.size > 0)
		{
			ras.CreateWidenedRegion((int)(style->background.size * (scale.cx + scale.cy) / 2 / 8));
		}

		ras.Rasterize((int)tl.x >> 3, (int)tl.y >> 3);

		if(style->shadow.depth > 0)
		{
			ras2.Clone(ras);

			double depth = style->shadow.depth * (scale.cx + scale.cy) / 2;

			tls.x = tl.x + (int)(depth * cos(deg2rad(style->shadow.angle)) + 0.5);
			tls.y = tl.y + (int)(depth * -sin(deg2rad(style->shadow.angle)) + 0.5);

			ras2.Rasterize((int)tls.x >> 3, (int)tls.y >> 3);
		}
	}
};

class CRow : public CAtlList<CGlyph*>
{
public:
	int ascent, descent, width;
	CRow() {ascent = descent = width = 0;}
};

template <class T>
void ReverseList(T& l)
{
	POSITION pos = l.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		l.GetNext(pos);
		l.AddHead(l.GetAt(cur));
		l.RemoveAt(cur);
	}
}

static ssf::Point GetAlignPoint(const ssf::Rect& frame, const ssf::Placement& placement, const ssf::Size& scale, const ssf::Size& size)
{
	ssf::Point p;

	p.x = frame.l;
	p.x += placement.pos.auto_x 
		? ((frame.r - frame.l) - size.cx) * placement.align.h 
		: placement.pos.x * scale.cx - size.cx * placement.align.h;

	p.y = frame.t;
	p.y += placement.pos.auto_y 
		? ((frame.b - frame.t) - size.cy) * placement.align.v 
		: placement.pos.y * scale.cy - size.cy * placement.align.v;

	return p;
}

static ssf::Point GetAlignPoint(const ssf::Rect& frame, const ssf::Placement& placement, const ssf::Size& scale)
{
	ssf::Size size = {0, 0};
	return GetAlignPoint(frame, placement, scale, size);
}

STDMETHODIMP CRenderedSSF::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
	CheckPointer(m_psf, E_UNEXPECTED);

	if(spd.type != MSP_RGB32) return E_INVALIDARG;

	CAutoLock csAutoLock(m_pLock);

	double at = (double)rt/10000000;

	CRect bbox2 = CRect(0, 0, 0, 0);

	HFONT hOldFont = (HFONT)GetCurrentObject(m_hDC, OBJ_FONT);

	CAutoPtrList<ssf::Subtitle> subs;
	m_psf->Lookup(at, subs);

	POSITION pos = subs.GetHeadPosition();
	while(pos)
	{
		const ssf::Subtitle* s = subs.GetNext(pos);

		if(s->m_text.IsEmpty()) continue;

		const ssf::Style& style = s->m_text.GetHead().style;

		//

		CRect spdrc = s->m_frame.reference == _T("video") ? spd.vidrect : CRect(0, 0, spd.w, spd.h);

		ssf::Size scale;

		scale.cx = (double)(spdrc.right - spdrc.left) / s->m_frame.resolution.cx;
		scale.cy = (double)(spdrc.bottom - spdrc.top) / s->m_frame.resolution.cy;

		ssf::Rect frame;

		frame.l = spdrc.left + style.placement.margin.l * scale.cx;
		frame.t = spdrc.top + style.placement.margin.t * scale.cy;
		frame.r = spdrc.right - style.placement.margin.r * scale.cx;
		frame.b = spdrc.bottom - style.placement.margin.b * scale.cy;

		CRect clip;

		if(style.placement.clip.l == -1) clip.left = 0;
		else clip.left = (int)(spdrc.left + style.placement.clip.l * scale.cx);
		if(style.placement.clip.t == -1) clip.top = 0;
		else clip.top = (int)(spdrc.top + style.placement.clip.t * scale.cy); 
		if(style.placement.clip.r == -1) clip.right = spd.w;
		else clip.right = (int)(spdrc.left + style.placement.clip.r * scale.cx);
		if(style.placement.clip.b == -1) clip.bottom = spd.h;
		else clip.bottom = (int)(spdrc.top + style.placement.clip.b * scale.cy);

		clip.left = max(clip.left, 0);
		clip.top = max(clip.top, 0);
		clip.right = min(clip.right, spd.w);
		clip.bottom = min(clip.bottom, spd.h);

		scale.cx *= 64;
		scale.cy *= 64;

		frame.l *= 64;
		frame.t *= 64;
		frame.r *= 64;
		frame.b *= 64;

		bool fVertical = s->m_direction.primary == _T("down") || s->m_direction.primary == _T("up");

		// 

		CAutoPtrList<CGlyph> glyphs;

		POSITION pos = s->m_text.GetHeadPosition();
		while(pos)
		{
			const ssf::Text& t = s->m_text.GetNext(pos);

			LOGFONT lf;
			memset(&lf, 0, sizeof(lf));
			lf.lfCharSet = DEFAULT_CHARSET;
			_tcscpy_s(lf.lfFaceName, CString(t.style.font.face));
			lf.lfHeight = (LONG)(t.style.font.size * scale.cy + 0.5);
			lf.lfWeight = (LONG)(t.style.font.weight + 0.5);
			lf.lfItalic = !!t.style.font.italic;
			lf.lfUnderline = !!t.style.font.underline;
			lf.lfStrikeOut = !!t.style.font.strikethrough;
			lf.lfOutPrecision = OUT_TT_PRECIS;
			lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			lf.lfQuality = ANTIALIASED_QUALITY;
			lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;

			HFONT hFont;

			if(!(hFont = m_fonts.Create(lf)))
			{
				_tcscpy_s(lf.lfFaceName, _T("Arial"));

				if(!(hFont = m_fonts.Create(lf)))
				{
					ASSERT(0);
					continue;
				}
			}

			SelectObject(m_hDC, hFont);

			TEXTMETRIC tm;
			GetTextMetrics(m_hDC, &tm);

			for(LPCWSTR c = t.str; *c; c++)
			{
				CAutoPtr<CGlyph> g(new CGlyph());

				g->c = *c;
				g->style = &t.style;

				CSize extent;
				GetTextExtentPoint32W(m_hDC, c, 1, &extent);

				ASSERT(extent.cx >= 0 && extent.cy >= 0);

				if(fVertical)
				{
					g->spacing = (int)(t.style.font.spacing * scale.cy + 0.5);
					g->ascent = extent.cx / 2;
					g->descent = extent.cx - g->ascent;
					g->width = extent.cy;

					// TESTME
					if(g->c == ssf::Text::SP)
					{
						g->width /= 2;
					}
				}
				else
				{
					g->spacing = (int)(t.style.font.spacing * scale.cx + 0.5);
					g->ascent = tm.tmAscent;
					g->descent = tm.tmDescent;
					g->width = extent.cx;
				}

				if(g->c == ssf::Text::LSEP)
				{
					g->width = g->spacing = 0;
					g->ascent /= 2;
					g->descent /= 2;
				}
				else
				{
					// TODO: cache outline of font+char

					BeginPath(m_hDC);
					TextOutW(m_hDC, 0, 0, c, 1);
					CloseFigure(m_hDC);
					if(!EndPath(m_hDC)) {AbortPath(m_hDC); ASSERT(0); continue;}

					g->pathcount = GetPath(m_hDC, NULL, NULL, 0);

					if(g->pathcount > 0)
					{
						g->pathpoints.Allocate(g->pathcount);
						g->pathtypes.Allocate(g->pathcount);

						if(g->pathcount != GetPath(m_hDC, g->pathpoints, g->pathtypes, g->pathcount))
						{
							ASSERT(0);
							continue;
						}
					}
				}

				glyphs.AddTail(g);
			}
		}

		// break glyphs into rows

		CAutoPtrList<CRow> rows;
		CAutoPtr<CRow> row;

		pos = glyphs.GetHeadPosition();
		while(pos)
		{
			CGlyph* g = glyphs.GetNext(pos);
			if(!row) row.Attach(new CRow());
			row->AddTail(g);
			if(g->c == ssf::Text::LSEP || !pos) rows.AddTail(row);
		}

		// wrap rows

		if(s->m_wrap == _T("normal") || s->m_wrap == _T("even"))
		{
			int maxwidth = abs((int)(fVertical ? frame.b - frame.t : frame.r - frame.l));

			for(POSITION rpos = rows.GetHeadPosition(); rpos; rows.GetNext(rpos))
			{
				CRow* r = rows.GetAt(rpos);
				
				POSITION brpos = NULL;

				if(s->m_wrap == _T("even"))
				{
					int fullwidth = 0;

					for(POSITION gpos = r->GetHeadPosition(); gpos; r->GetNext(gpos))
					{
						const CGlyph* g = r->GetAt(gpos);

						fullwidth += g->width + g->spacing;
					}

					fullwidth = abs(fullwidth);
					
					if(fullwidth > maxwidth)
					{
						maxwidth = fullwidth / ((fullwidth / maxwidth) + 1);
					}
				}

				int width = 0;

				for(POSITION gpos = r->GetHeadPosition(); gpos; r->GetNext(gpos))
				{
					const CGlyph* g = r->GetAt(gpos);

					width += g->width + g->spacing;

					if(brpos && abs(width) > maxwidth && g->c != ssf::Text::SP)
					{
						row.Attach(new CRow());
						POSITION next = brpos;
						r->GetNext(next);
						do {row->AddHead(r->GetPrev(brpos));} while(brpos);
						rows.InsertBefore(rpos, row);
						while(!r->IsEmpty() && r->GetHeadPosition() != next) r->RemoveHeadNoReturn();
						g = r->GetAt(gpos = next);
						width = g->width + g->spacing;
					}

					if(g->style->linebreak == _T("char")
					|| g->style->linebreak == _T("word") && g->c == ssf::Text::SP)
					{
						brpos = gpos;
					}
				}
			}
		}

		// trim rows

		for(POSITION pos = rows.GetHeadPosition(); pos; rows.GetNext(pos))
		{
			CRow* r = rows.GetAt(pos);

			while(!r->IsEmpty() && r->GetHead()->c == ssf::Text::SP)
				r->RemoveHead();

			while(!r->IsEmpty() && r->GetTail()->c == ssf::Text::SP)
				r->RemoveTail();
		}

		// calc fill width for each glyph

		CRow glypsh2fill;
		int fill_id = 0;

		for(POSITION pos = rows.GetHeadPosition(); pos; rows.GetNext(pos))
		{
			CRow* r = rows.GetAt(pos);

			POSITION gpos = r->GetHeadPosition();
			while(gpos)
			{
				CGlyph* g = r->GetNext(gpos);

				if(!glypsh2fill.IsEmpty() && fill_id && (g->style->fill.id != fill_id || !pos && !gpos))
				{
					int fillwidth = (int)(g->style->fill.width * glypsh2fill.width + 0.5);

					while(!glypsh2fill.IsEmpty())
					{
						CGlyph* g = glypsh2fill.RemoveTail();
						glypsh2fill.width -= g->width;
						g->fillwidth = fillwidth - glypsh2fill.width;
					}

					ASSERT(glypsh2fill.IsEmpty());
					ASSERT(glypsh2fill.width == 0);

					glypsh2fill.RemoveAll();
					glypsh2fill.width = 0;
				}

				fill_id = g->style->fill.id;

				if(g->style->fill.id)
				{
					glypsh2fill.AddTail(g);
					glypsh2fill.width += g->width;
				}
			}
		}

		// calc row sizes and total subtitle size

		ssf::Size size = {0, 0};

		if(s->m_direction.secondary == _T("left") || s->m_direction.secondary == _T("up"))
			ReverseList(rows);

		for(POSITION pos = rows.GetHeadPosition(); pos; rows.GetNext(pos))
		{
			CRow* r = rows.GetAt(pos);

			if(s->m_direction.primary == _T("left") || s->m_direction.primary == _T("up"))
				ReverseList(*r);

			int w = 0, h = 0;

			r->width = 0;

			for(POSITION gpos = r->GetHeadPosition(); gpos; r->GetNext(gpos))
			{
				const CGlyph* g = r->GetAt(gpos);

				w += g->width + g->spacing;
				h = max(h, g->ascent + g->descent);

				r->width += g->width;
				if(gpos) r->width += g->spacing;
				r->ascent = max(r->ascent, g->ascent);
				r->descent = max(r->descent, g->descent);
			}

			if(fVertical)
			{
				size.cx += h;
				size.cy = max(size.cy, w);
			}
			else
			{
				size.cx = max(size.cx, w);
				size.cy += h;
			}
		}

		// align rows and calc glyph positions

		CAtlList<CGlyph*> glyphs2render;

		ssf::Point p = GetAlignPoint(frame, style.placement, scale, size);
		ssf::Point org = GetAlignPoint(frame, style.placement, scale);

		// TODO: do collision detection here and move p+org if needed

		for(POSITION pos = rows.GetHeadPosition(); pos; rows.GetNext(pos))
		{
			CRow* r = rows.GetAt(pos);

			ssf::Size rowsize;
			rowsize.cx = rowsize.cy = r->width;

			if(fVertical)
			{
				p.y = GetAlignPoint(frame, style.placement, scale, rowsize).y;

				for(POSITION gpos = r->GetHeadPosition(); gpos; r->GetNext(gpos))
				{
					CGlyph* g = r->GetAt(gpos);

					g->tl.x = p.x + g->style->placement.offset.x * scale.cx + r->ascent - g->ascent;
					g->tl.y = p.y + g->style->placement.offset.y * scale.cy;

					glyphs2render.AddTail(g);

					p.y += g->width + g->spacing;
				}

				p.x += r->ascent + r->descent;
			}
			else
			{
				p.x = GetAlignPoint(frame, style.placement, scale, rowsize).x;

				for(POSITION gpos = r->GetHeadPosition(); gpos; r->GetNext(gpos))
				{
					CGlyph* g = r->GetAt(gpos);

					g->tl.x = p.x + g->style->placement.offset.x * scale.cx;
					g->tl.y = p.y + g->style->placement.offset.y * scale.cy + r->ascent - g->ascent;

					glyphs2render.AddTail(g);

					p.x += g->width + g->spacing;
				}

				p.y += r->ascent + r->descent;
			}
		}

		// rasterize

		pos = glyphs2render.GetHeadPosition();
		while(pos) glyphs2render.GetNext(pos)->Rasterize(org, scale);

		// draw shadow

		pos = glyphs2render.GetHeadPosition();
		while(pos)
		{
			CGlyph* g = glyphs2render.GetNext(pos);

			if(g->style->shadow.depth <= 0) continue;

			DWORD c = 
				(min(max((DWORD)g->style->shadow.color.b, 0), 255) <<  0) |
				(min(max((DWORD)g->style->shadow.color.g, 0), 255) <<  8) |
				(min(max((DWORD)g->style->shadow.color.r, 0), 255) << 16) |
				(min(max((DWORD)g->style->shadow.color.a, 0), 255) << 24);

			bool outline = g->style->background.type == L"outline" && g->style->background.size > 0;

			DWORD sw[6] = {c, -1};

			bbox2 |= g->ras2.Draw(spd, clip, (int)g->tls.x >> 3, (int)g->tls.y >> 3, sw, true, outline);
		}

		// draw outline

		pos = glyphs2render.GetHeadPosition();
		while(pos)
		{
			CGlyph* g = glyphs2render.GetNext(pos);

			if(g->style->background.size <= 0) continue;

			DWORD c = 
				(min(max((DWORD)g->style->background.color.b, 0), 255) <<  0) |
				(min(max((DWORD)g->style->background.color.g, 0), 255) <<  8) |
				(min(max((DWORD)g->style->background.color.r, 0), 255) << 16) |
				(min(max((DWORD)g->style->background.color.a, 0), 255) << 24);

			bool body = !g->style->font.color.a && !g->style->background.color.a;

			DWORD sw[6] = {c, -1};

			bbox2 |= g->ras.Draw(spd, clip, (int)g->tl.x >> 3, (int)g->tl.y >> 3, sw, body, true);
		}

		// draw body

		pos = glyphs2render.GetHeadPosition();
		while(pos)
		{
			CGlyph* g = glyphs2render.GetNext(pos);

			DWORD c = 
				(min(max((DWORD)g->style->font.color.b, 0), 255) <<  0) |
				(min(max((DWORD)g->style->font.color.g, 0), 255) <<  8) |
				(min(max((DWORD)g->style->font.color.r, 0), 255) << 16) |
				(min(max((DWORD)g->style->font.color.a, 0), 255) << 24);

			DWORD sw[6] = {c, -1}; // TODO: fill

			bbox2 |= g->ras.Draw(spd, clip, (int)g->tl.x >> 3, (int)g->tl.y >> 3, sw, true, false);
		}
	}

	SelectObject(m_hDC, hOldFont);

	bbox = bbox2 & CRect(0, 0, spd.w, spd.h);

	return S_OK;
}

// IPersist

STDMETHODIMP CRenderedSSF::GetClassID(CLSID* pClassID)
{
	return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CRenderedSSF::GetStreamCount()
{
	return 1;
}

STDMETHODIMP CRenderedSSF::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
{
	if(iStream != 0) return E_INVALIDARG;

	if(ppName)
	{
		if(!(*ppName = (WCHAR*)CoTaskMemAlloc((m_name.GetLength()+1)*sizeof(WCHAR))))
			return E_OUTOFMEMORY;

		wcscpy(*ppName, CStringW(m_name));
	}

	if(pLCID)
	{
		*pLCID = 0; // TODO
	}

	return S_OK;
}

STDMETHODIMP_(int) CRenderedSSF::GetStream()
{
	return 0;
}

STDMETHODIMP CRenderedSSF::SetStream(int iStream)
{
	return iStream == 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CRenderedSSF::Reload()
{
	CAutoLock csAutoLock(m_pLock);

	return !m_fn.IsEmpty() && Open(m_fn, m_name) ? S_OK : E_FAIL;
}
