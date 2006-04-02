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
 *  - fill effect
 *  - box background
 *  - collision detection
 *  - super slow, make it faster
 *
 */

#include "stdafx.h"
#include "SSF.h"
#include "..\subpic\MemSubPic.h"

#define _USE_MATH_DEFINES
#include <math.h>

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
	m_subtitlecache.RemoveAll();

	try
	{
		m_psf.Attach(new ssf::SubtitleFile());
		m_psf->Parse(s);

#if 0 // def DEBUG
		m_psf->Dump();
/*
		float at = 0;
		for(int i = 0; i < 1000; i += 100)
		{
			float at = (float)i/1000;
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
					TRACE(_T("%.3f: [%.2f] %s\n"), at, t.style.font.scale.cx, CString(t.str));
				}
			}
		}
*/
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
	return m_psf && m_psf->m_segments.Lookup((float)rt/10000000, k) ? (POSITION)(++k) : NULL;
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

#define deg2rad(d) (3.14159f/180*(d))

CRenderedSSF::CGlyph::CGlyph()
{
	c = 0;
	ascent = descent = width = spacing = fill = 0;
	tl.x = tl.y = tls.x = tls.y = 0;
}

void CRenderedSSF::CGlyph::Transform(CGlyphPath& path, CPoint org)
{
	// TODO: add sse code path

	float sx = style.font.scale.cx;
	float sy = style.font.scale.cy;

	float caz = cos(deg2rad(style.placement.angle.z));
	float saz = sin(deg2rad(style.placement.angle.z));
	float cax = cos(deg2rad(style.placement.angle.x));
	float sax = sin(deg2rad(style.placement.angle.x));
	float cay = cos(deg2rad(style.placement.angle.y));
	float say = sin(deg2rad(style.placement.angle.y));

	for(size_t i = 0, j = path.types.GetCount(); i < j; i++)
	{
		float x, y, z, xx, yy, zz;

		x = sx * (path.points[i].x - org.x);
		y = sy * (path.points[i].y - org.y);
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

		zz = 1.0f / (max(zz, -19000) + 20000);

		x = (xx * 20000) * zz;
		y = (yy * 20000) * zz;

		path.points[i].x = (LONG)(x + org.x + 0.5);
		path.points[i].y = (LONG)(y + org.y + 0.5);
	}
}

void CRenderedSSF::CGlyph::Transform(const ssf::Size& scale, CPoint org)
{
	org -= tl;

	if(style.background.type == L"enlarge" && style.background.size > 0)
	{
		path_enlarge.Enlarge(path, style.background.size * (scale.cx + scale.cy) / 2);
		Transform(path_enlarge, org);
	}

	Transform(path, org);
}

void CRenderedSSF::CGlyphPath::Enlarge(const CGlyphPath& src, float size)
{
	types.SetCount(src.types.GetCount());
	points.SetCount(src.points.GetCount());

	memcpy(types.GetData(), src.types.GetData(), types.GetCount());

	size_t start = 0, end = 0;

	for(size_t i = 0, j = src.types.GetCount(); i <= j; i++)
	{
		if(i > 0 && (i == j || (src.types[i] & ~PT_CLOSEFIGURE) == PT_MOVETO))
		{
			end = i-1;

			bool cw = true; // TODO: determine orientation
			float rotate = cw ? -M_PI_2 : M_PI_2;

			CPoint prev = src.points[end];
			CPoint cur = src.points[start];

			for(size_t k = start; k <= end; k++)
			{
				CPoint next = k < end ? src.points[k+1] : src.points[start];

				for(int l = k-1; prev == cur; l--)
				{
					if(l < (int)start) l = end;
					prev = src.points[l];
				}

				for(int l = k+1; next == cur; l++)
				{
					if(l > (int)end) l = start;
					next = src.points[l];
				}

				CPoint in = cur - prev;
				CPoint out = next - cur;

				float angle_in = atan2((float)in.y, (float)in.x);
				float angle_out = atan2((float)out.y, (float)out.x);
				float angle_diff = angle_out - angle_in;
				if(angle_diff < 0) angle_diff += M_PI*2;
				if(angle_diff > M_PI) angle_diff -= M_PI*2;
				float scale = cos(angle_diff / 2);

				CPoint p;

				if(angle_diff < 0)
				{
					if(angle_diff > -M_PI/8) {if(scale < 1) scale = 1;}
					else {if(scale < 0.50) scale = 0.50;}
				}
				else
				{
					if(angle_diff < M_PI/8) {if(scale < 0.75) scale = 0.75;}
					else {if(scale < 0.25) scale = 0.25;}
				}

				if(scale < 0.1) scale = 0.1;

				float angle = angle_in + angle_diff / 2 - rotate;
				float radius = -size / scale; // FIXME
				p.x = radius * cos(angle);
				p.y = radius * sin(angle);

				points[k] = cur + p;

				prev = cur;
				cur = next;
			}

			start = end+1;
		}
	}
}

void CRenderedSSF::CGlyph::Rasterize(const ssf::Size& scale)
{
	SSFRasterizer* r = &ras;

	if(style.background.type == L"enlarge" && style.background.size > 0)
	{
		ras_enlarge.ScanConvert(path_enlarge.types.GetCount(), path_enlarge.types.GetData(), path_enlarge.points.GetData());
		ras_enlarge.Rasterize(tl.x >> 3, tl.y >> 3);
		r = &ras_enlarge;
	}

	ras.ScanConvert(path.types.GetCount(), path.types.GetData(), path.points.GetData());

	if(style.background.type == L"outline" && style.background.size > 0)
	{
		ras.CreateWidenedRegion((int)(style.background.size * (scale.cx + scale.cy) / 2 / 8));
	}
	
	ras.Rasterize(tl.x >> 3, tl.y >> 3);

	if(style.shadow.depth > 0)
	{
		ras_shadow.Reuse(*r);

		float depth = style.shadow.depth * (scale.cx + scale.cy) / 2;

		tls.x = tl.x + (int)(depth * cos(deg2rad(style.shadow.angle)) + 0.5);
		tls.y = tl.y + (int)(depth * -sin(deg2rad(style.shadow.angle)) + 0.5);

		ras_shadow.Rasterize(tls.x >> 3, tls.y >> 3);
	}
}

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

static CPoint GetAlignPoint(const ssf::Placement& placement, const ssf::Size& scale, const CRect& frame, const CSize& size)
{
	CPoint p;

	p.x = frame.left;
	p.x += placement.pos.auto_x 
		? placement.align.h * (frame.Width() - size.cx)
		: placement.pos.x * scale.cx - placement.align.h * size.cx;

	p.y = frame.top;
	p.y += placement.pos.auto_y 
		? placement.align.v * (frame.Height() - size.cy) 
		: placement.pos.y * scale.cy - placement.align.v * size.cy;

	return p;
}

static CPoint GetAlignPoint(const ssf::Placement& placement, const ssf::Size& scale, const CRect& frame)
{
	CSize size(0, 0);
	return GetAlignPoint(placement, scale, frame, size);
}

STDMETHODIMP CRenderedSSF::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
	CheckPointer(m_psf, E_UNEXPECTED);

	if(spd.type != MSP_RGB32) return E_INVALIDARG;

	CAutoLock csAutoLock(m_pLock);

	float at = (float)rt/10000000;

	CRect bbox2(0, 0, 0, 0);

	CAutoPtrList<ssf::Subtitle> subs;
	m_psf->Lookup(at, subs);

	POSITION spos = subs.GetHeadPosition();
	while(spos)
	{
		const ssf::Subtitle* s = subs.GetNext(spos);

		if(s->m_text.IsEmpty()) continue;

		const ssf::Style& style = s->m_text.GetHead().style;

		CRect spdrc = s->m_frame.reference == _T("video") ? spd.vidrect : CRect(0, 0, spd.w, spd.h);

		if(spdrc.IsRectEmpty()) return E_INVALIDARG;

		CSubtitle* sub = NULL;

		if(m_subtitlecache.Lookup(s->m_name, sub))
		{
			if(s->m_animated || sub->m_spdrc != spdrc)
			{
				m_subtitlecache.Invalidate(s->m_name);
				sub = NULL;
			}
		}

		if(!sub)
		{
			ssf::Size scale;

			scale.cx = (float)spdrc.Width() / s->m_frame.resolution.cx;
			scale.cy = (float)spdrc.Height() / s->m_frame.resolution.cy;

			CRect frame;

			frame.left = (int)(64.0f * (spdrc.left + style.placement.margin.l * scale.cx) + 0.5);
			frame.top = (int)(64.0f * (spdrc.top + style.placement.margin.t * scale.cy) + 0.5);
			frame.right = (int)(64.0f * (spdrc.right - style.placement.margin.r * scale.cx) + 0.5);
			frame.bottom = (int)(64.0f * (spdrc.bottom - style.placement.margin.b * scale.cy) + 0.5);

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

				CFontWrapper* font;

				if(!(font = m_fontcache.Create(lf)))
				{
					_tcscpy_s(lf.lfFaceName, _T("Arial"));

					if(!(font = m_fontcache.Create(lf)))
					{
						ASSERT(0);
						continue;
					}
				}

				HFONT hOldFont = SelectFont(m_hDC, *font);

				TEXTMETRIC tm;
				GetTextMetrics(m_hDC, &tm);

				for(LPCWSTR c = t.str; *c; c++)
				{
					CAutoPtr<CGlyph> g(new CGlyph());

					g->c = *c;
					g->style = t.style;

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
						g->spacing = 0;
						g->width = 0;
						g->ascent /= 2;
						g->descent /= 2;
					}
					else
					{
						CGlyphPath* path = m_glyphpathcache.Create(m_hDC, font, g->c);
						if(!path) {ASSERT(0); continue;}
						g->path = *path;
					}

					glyphs.AddTail(g);
				}

				SelectFont(m_hDC, hOldFont);
			}

			// break glyphs into rows

			CAutoPtrList<CRow> rows;
			CAutoPtr<CRow> row;

			pos = glyphs.GetHeadPosition();
			while(pos)
			{
				CAutoPtr<CGlyph> g = glyphs.GetNext(pos);
				if(!row) row.Attach(new CRow());
				WCHAR c = g->c;
				row->AddTail(g);
				if(c == ssf::Text::LSEP || !pos) rows.AddTail(row);
			}

			// wrap rows

			if(s->m_wrap == _T("normal") || s->m_wrap == _T("even"))
			{
				int maxwidth = abs((int)(fVertical ? frame.Height() : frame.Width()));
				int minwidth = 0;

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
							minwidth = maxwidth;
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

						if(abs(width) >= minwidth)
						{
							if(g->style.linebreak == _T("char")
							|| g->style.linebreak == _T("word") && g->c == ssf::Text::SP)
							{
								brpos = gpos;
							}
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

			CAtlList<CGlyph*> glypsh2fill;
			int fill_id = 0;
			int fill_width = 0;

			for(POSITION pos = rows.GetHeadPosition(); pos; rows.GetNext(pos))
			{
				CRow* r = rows.GetAt(pos);

				POSITION gpos = r->GetHeadPosition();
				while(gpos)
				{
					CGlyph* g = r->GetNext(gpos);

					if(!glypsh2fill.IsEmpty() && fill_id && (g->style.fill.id != fill_id || !pos && !gpos))
					{
						int w = (int)(g->style.fill.width * fill_width + 0.5);

						while(!glypsh2fill.IsEmpty())
						{
							CGlyph* g = glypsh2fill.RemoveTail();
							fill_width -= g->width;
							g->fill = w - fill_width;
						}

						ASSERT(glypsh2fill.IsEmpty());
						ASSERT(fill_width == 0);

						glypsh2fill.RemoveAll();
						fill_width = 0;
					}

					fill_id = g->style.fill.id;

					if(g->style.fill.id)
					{
						glypsh2fill.AddTail(g);
						fill_width += g->width;
					}
				}
			}

			// calc row sizes and total subtitle size

			CSize size(0, 0);

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

					w += g->width;
					if(gpos) w += g->spacing;
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

			sub = new CSubtitle(spdrc, clip);

			CPoint p = GetAlignPoint(style.placement, scale, frame, size);
			CPoint org = GetAlignPoint(style.placement, scale, frame);

			// TODO: do collision detection here and move p+org if needed

			for(POSITION pos = rows.GetHeadPosition(); pos; rows.GetNext(pos))
			{
				CRow* r = rows.GetAt(pos);

				CSize rsize;
				rsize.cx = rsize.cy = r->width;

				if(fVertical)
				{
					p.y = GetAlignPoint(style.placement, scale, frame, rsize).y;

					for(POSITION gpos = r->GetHeadPosition(); gpos; r->GetNext(gpos))
					{
						CAutoPtr<CGlyph> g = r->GetAt(gpos);
						g->tl.x = p.x + (int)(g->style.placement.offset.x * scale.cx + 0.5) + r->ascent - g->ascent;
						g->tl.y = p.y + (int)(g->style.placement.offset.y * scale.cy + 0.5);
						p.y += g->width + g->spacing;
						sub->m_glyphs.AddTail(g);
					}

					p.x += r->ascent + r->descent;
				}
				else
				{
					p.x = GetAlignPoint(style.placement, scale, frame, rsize).x;

					for(POSITION gpos = r->GetHeadPosition(); gpos; r->GetNext(gpos))
					{
						CAutoPtr<CGlyph> g = r->GetAt(gpos);
						g->tl.x = p.x + (int)(g->style.placement.offset.x * scale.cx + 0.5);
						g->tl.y = p.y + (int)(g->style.placement.offset.y * scale.cy + 0.5) + r->ascent - g->ascent;
						p.x += g->width + g->spacing;
						sub->m_glyphs.AddTail(g);
					}

					p.y += r->ascent + r->descent;
				}
			}

			//

			pos = sub->m_glyphs.GetHeadPosition();
			while(pos) sub->m_glyphs.GetNext(pos)->Transform(scale, org);

			// merge glyphs (TODO: merge 'fill' too)

			CGlyph* g0 = NULL;

			pos = sub->m_glyphs.GetHeadPosition();
			while(pos)
			{
				POSITION cur = pos;

				CGlyph* g = sub->m_glyphs.GetNext(pos);

				if(g0 && g0->style.IsSimilar(g->style))
				{
					CPoint o = g->tl - g0->tl;

					SSFRasterizer::MovePoints(g->path.points.GetData(), g->path.types.GetCount(), o.x, o.y);

					g0->path.types.Append(g->path.types);
					g0->path.points.Append(g->path.points);

					SSFRasterizer::MovePoints(g->path_enlarge.points.GetData(), g->path_enlarge.types.GetCount(), o.x, o.y);

					g0->path_enlarge.types.Append(g->path_enlarge.types);
					g0->path_enlarge.points.Append(g->path_enlarge.points);

					sub->m_glyphs.RemoveAt(cur);
				}
				else
				{
					g0 = g;
				}
			}

			// rasterize

			pos = sub->m_glyphs.GetHeadPosition();
			while(pos) sub->m_glyphs.GetNext(pos)->Rasterize(scale);

			m_subtitlecache.Create(s->m_name, sub);
		}

		// draw shadow

		POSITION pos = sub->m_glyphs.GetHeadPosition();
		while(pos)
		{
			CGlyph* g = sub->m_glyphs.GetNext(pos);

			if(g->style.shadow.depth <= 0) continue;

			DWORD c = 
				(min(max((DWORD)g->style.shadow.color.b, 0), 255) <<  0) |
				(min(max((DWORD)g->style.shadow.color.g, 0), 255) <<  8) |
				(min(max((DWORD)g->style.shadow.color.r, 0), 255) << 16) |
				(min(max((DWORD)g->style.shadow.color.a, 0), 255) << 24);

			DWORD sw[6] = {c, -1};

			bool outline = g->style.background.type == L"outline" && g->style.background.size > 0;

			bbox2 |= g->ras_shadow.Draw(spd, sub->m_clip, g->tls.x >> 3, g->tls.y >> 3, sw, true, outline);
		}

		// draw outline

		pos = sub->m_glyphs.GetHeadPosition();
		while(pos)
		{
			CGlyph* g = sub->m_glyphs.GetNext(pos);

			if(g->style.background.size <= 0) continue;

			DWORD c = 
				(min(max((DWORD)g->style.background.color.b, 0), 255) <<  0) |
				(min(max((DWORD)g->style.background.color.g, 0), 255) <<  8) |
				(min(max((DWORD)g->style.background.color.r, 0), 255) << 16) |
				(min(max((DWORD)g->style.background.color.a, 0), 255) << 24);

			DWORD sw[6] = {c, -1};

			if(g->style.background.type == L"outline")
			{
				bool body = !g->style.font.color.a && !g->style.background.color.a;

				bbox2 |= g->ras.Draw(spd, sub->m_clip, g->tl.x >> 3, g->tl.y >> 3, sw, body, true);
			}
			else if(g->style.background.type == L"enlarge")
			{
				bbox2 |= g->ras_enlarge.Draw(spd, sub->m_clip, g->tl.x >> 3, g->tl.y >> 3, sw, true, false);
			}
		}

		// draw body

		pos = sub->m_glyphs.GetHeadPosition();
		while(pos)
		{
			CGlyph* g = sub->m_glyphs.GetNext(pos);

			DWORD c = 
				(min(max((DWORD)g->style.font.color.b, 0), 255) <<  0) |
				(min(max((DWORD)g->style.font.color.g, 0), 255) <<  8) |
				(min(max((DWORD)g->style.font.color.r, 0), 255) << 16) |
				(min(max((DWORD)g->style.font.color.a, 0), 255) << 24);

			DWORD sw[6] = {c, -1}; // TODO: fill

			bbox2 |= g->ras.Draw(spd, sub->m_clip, g->tl.x >> 3, g->tl.y >> 3, sw, true, false);
		}
	}

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

//

CRenderedSSF::CFontWrapper* CRenderedSSF::CFontCache::Create(const LOGFONT& lf)
{
	CStringW key;

	key.Format(L"%s,%d,%d,%d,%d,%d", 
		CStringW(lf.lfFaceName), lf.lfHeight, lf.lfWeight, 
		lf.lfItalic, lf.lfUnderline, lf.lfStrikeOut);

	CFontWrapper* pFW = NULL;

	if(m_key2obj.Lookup(key, pFW))
	{
		return pFW;
	}

	HFONT hFont;

	if(!(hFont = CreateFontIndirect(&lf)))
	{
		ASSERT(0);
		return NULL;
	}

	pFW = new CFontWrapper(hFont, key);

	Add(key, pFW);

	return pFW;
}

//

CRenderedSSF::CGlyphPath::CGlyphPath(const CGlyphPath& path)
{
	*this = path;
}

void CRenderedSSF::CGlyphPath::operator = (const CGlyphPath& path)
{
	types.Copy(path.types);
	points.Copy(path.points);
}

//

CRenderedSSF::CGlyphPath* CRenderedSSF::CGlyphPathCache::Create(HDC hDC, const CFontWrapper* f, WCHAR c)
{
	CStringW key = CStringW((LPCWSTR)*f) + c;

	CGlyphPath* path = NULL;

	if(m_key2obj.Lookup(key, path))
	{
		return path;
	}

	BeginPath(hDC);
	TextOutW(hDC, 0, 0, &c, 1);
	CloseFigure(hDC);
	if(!EndPath(hDC)) {AbortPath(hDC); ASSERT(0); return NULL;}

	path = new CGlyphPath();

	int count = GetPath(hDC, NULL, NULL, 0);

	if(count > 0)
	{
		path->points.SetCount(count);
		path->types.SetCount(count);

		if(count != GetPath(hDC, path->points.GetData(), path->types.GetData(), count))
		{
			ASSERT(0);
			delete path;
			return NULL;
		}
	}

	Add(key, path);

	return path;
}

void CRenderedSSF::CSubtitleCache::Create(const CStringW& name, CSubtitle* sub)
{
	Add(name, sub);
}
