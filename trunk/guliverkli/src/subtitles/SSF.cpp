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
}

CRenderedSSF::~CRenderedSSF()
{
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
					TRACE(_T("%.3f: [%.2f] %s\n"), at, t.style.font.size, t.str);
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

class CGlyph
{
public:
	TCHAR c;
	const ssf::Style* style;
	int ascent, descent, width, spacing;
	CAutoVectorPtr<BYTE> pathtypes;
	CAutoVectorPtr<POINT> pathpoints;
	ssf::Rect rect;

	CGlyph()
	{
		c = 0;
		style = NULL;
		ascent = descent = width = 0;
		rect.l = rect.t = rect.r = rect.b = 0;
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

STDMETHODIMP CRenderedSSF::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
	CheckPointer(m_psf, E_UNEXPECTED);

	if(spd.type != MSP_RGB32) return E_INVALIDARG;

	CAutoLock csAutoLock(m_pLock);

	double at = (double)rt/10000000;

	CRect bbox2 = CRect(spd.w, spd.h, 0, 0);

	HDC hDC = GetDC(0);
	HFONT hOldFont = (HFONT)GetCurrentObject(hDC, OBJ_FONT);

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

		ssf::Rect clip;
		clip.l = spdrc.left + style.placement.clip.l * scale.cx;
		clip.t = spdrc.top + style.placement.clip.t * scale.cy;
		clip.r = spdrc.left + style.placement.clip.r * scale.cx;
		clip.b = spdrc.top + style.placement.clip.b * scale.cy;
		clip.l = max(clip.l, 0);
		clip.t = max(clip.t, 0);
		clip.r = min(clip.r, spd.w);
		clip.b = min(clip.b, spd.h);

		scale.cx *= 64;
		scale.cy *= 64;

		frame.l *= 64;
		frame.t *= 64;
		frame.r *= 64;
		frame.b *= 64;

		clip.l *= 64;
		clip.t *= 64;
		clip.r *= 64;
		clip.b *= 64;

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
			_tcscpy_s(lf.lfFaceName, t.style.font.face);
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

			SelectObject(hDC, hFont);

			TEXTMETRIC tm;
			GetTextMetrics(hDC, &tm);

			for(LPCTSTR c = t.str; *c; c++)
			{
				CAutoPtr<CGlyph> g(new CGlyph());

				g->c = *c;
				g->style = &t.style;

				CSize extent;
				GetTextExtentPoint32(hDC, c, 1, &extent);

				ASSERT(extent.cx >= 0 && extent.cy >= 0);

				if(fVertical)
				{
					g->spacing = (int)(t.style.font.spacing * scale.cy + 0.5);
					g->ascent = extent.cx / 2;
					g->descent = extent.cx - g->ascent;
					g->width = extent.cy;
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

					BeginPath(hDC);
					TextOut(hDC, 0, 0, c, 1);
					CloseFigure(hDC);
					if(!EndPath(hDC)) {AbortPath(hDC); ASSERT(0); continue;}

					int points = GetPath(hDC, NULL, NULL, 0);

					if(points > 0)
					{
						g->pathpoints.Allocate(points);
						g->pathtypes.Allocate(points);

						if(points != GetPath(hDC, g->pathpoints, g->pathtypes, points))
						{
							ASSERT(0);
							continue;
						}
					}
				}

				glyphs.AddTail(g);
			}
		}

		//

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

		// 

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

		// TODO: calc fill width for each glyph here, later the lists may get reversed

		ssf::Size size = {0, 0};

		if(s->m_direction.secondary == _T("left") || s->m_direction.secondary == _T("up"))
			ReverseList(rows);

		for(POSITION pos = rows.GetHeadPosition(); pos; rows.GetNext(pos))
		{
			CRow* r = rows.GetAt(pos);

			while(!r->IsEmpty() && r->GetHead()->c == ssf::Text::SP)
				r->RemoveHead();

			while(!r->IsEmpty() && r->GetTail()->c == ssf::Text::SP)
				r->RemoveTail();

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

		//

		CAtlList<const CGlyph*> glyphs2render;

		ssf::Point p = GetAlignPoint(frame, style.placement, scale, size);

		// TODO: do collision detection here and move p if needed

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

					g->rect.l = p.x + g->style->placement.offset.x * scale.cx + r->ascent - g->ascent;
					g->rect.t = p.y + g->style->placement.offset.y * scale.cy;
					g->rect.r = g->rect.l + g->ascent + g->descent;
					g->rect.b = g->rect.t + g->width;

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

					g->rect.l = p.x + g->style->placement.offset.x * scale.cx;
					g->rect.t = p.y + g->style->placement.offset.y * scale.cy + r->ascent - g->ascent;
					g->rect.r = g->rect.l + g->width;
					g->rect.b = g->rect.t + g->ascent + g->descent;

					glyphs2render.AddTail(g);

					p.x += g->width + g->spacing;
				}

				p.y += r->ascent + r->descent;
			}
		}

		//

		ssf::Point org;

		org.x = frame.l;
		org.x += style.placement.pos.auto_x
			? (frame.r - frame.l) * style.placement.align.h
			: style.placement.pos.x * scale.cx;

		org.y = frame.t;
		org.y += style.placement.pos.auto_y 
			? (frame.b - frame.t) * style.placement.align.v
			: style.placement.pos.y * scale.cy;

		pos = glyphs2render.GetHeadPosition();
		while(pos)
		{
			const CGlyph* g = glyphs2render.GetNext(pos);

			if(g->rect.r < clip.l || g->rect.l >= clip.r
			|| g->rect.b < clip.t || g->rect.t >= clip.b)
				continue;

			// TODO: render g

			CRect r;
			r.left = (int)(g->rect.l) / 64;
			r.top = (int)(g->rect.t) / 64;
			r.right = (int)(g->rect.r) / 64;
			r.bottom = (int)(g->rect.b) / 64;

			if(r.IsRectEmpty())
				continue;

			bbox2 |= r;

			TRACE(_T("%c (%d, %d, %d, %d)\n"), g->c, r);

			if(g->c != ssf::Text::SP)
			{
				BYTE* p = ((BYTE*)spd.bits + r.top*spd.pitch);
				for(int y = r.top; y < r.bottom; y++, p += spd.pitch)
					for(int x = r.left; x < r.right; x++)
						((DWORD*)p)[x] = 0x80ff00ff;
			}
		}
	}

	SelectObject(hDC, hOldFont);
	ReleaseDC(0, hDC);

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
