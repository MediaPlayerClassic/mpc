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

//#include "Rasterizer.h"
#include "..\SubPic\ISubPic.h"
#include ".\libssf\SubtitleFile.h"
#include "SSFRasterizer.h"

#pragma once

[uuid("E0593632-0AB7-47CA-8BE1-E9D2A6A4825E")]
class CRenderedSSF : public ISubPicProviderImpl, public ISubStream
{
	CString m_fn, m_name;
	CAutoPtr<ssf::SubtitleFile> m_psf;
	HDC m_hDC;

	template<class T>
	class CCache
	{
	protected:
		ssf::CAtlStringMapW<T> m_key2obj;
		CAtlList<CStringW> m_objs;
		size_t m_limit;

		void Add(const CStringW& key, T& obj)
		{
			if(ssf::CAtlStringMapW<T>::CPair* p = m_key2obj.Lookup(key))
			{
				delete p->m_value;
			}
			else
			{
				m_objs.AddTail(key);
			}

			m_key2obj[key] = obj;

			while(m_objs.GetCount() > m_limit)
			{
				CStringW key = m_objs.RemoveHead();
				ASSERT(m_key2obj.Lookup(key));
				delete m_key2obj[key];
				m_key2obj.RemoveKey(key);
			}
		}

	public:
		CCache(size_t limit)
		{
			m_limit = max(1, limit);
		}

		virtual ~CCache() 
		{
			RemoveAll();
		}

		void RemoveAll()
		{
			POSITION pos = m_key2obj.GetStartPosition();
			while(pos) delete m_key2obj.GetNextValue(pos);
			m_key2obj.RemoveAll();
			m_objs.RemoveAll();
		}

		bool Lookup(const CStringW& key, T& val)
		{
			return m_key2obj.Lookup(key, val) && val;
		}

		void Invalidate(const CStringW& key)
		{
			T val;
			if(m_key2obj.Lookup(key, val))
			{
				delete val;
				m_key2obj[key] = NULL;
			}
		}
	};

	class CFontWrapper
	{
		HFONT m_hFont;
		CStringW m_key;

	public:
		CFontWrapper(HFONT hFont, const CStringW& key) : m_hFont(hFont), m_key(key) {}
		virtual ~CFontWrapper() {DeleteFont(m_hFont);}
		operator HFONT() const {return m_hFont;}
		operator LPCWSTR() const {return m_key;}
	};

	class CFontCache : public CCache<CFontWrapper*> 
	{
	public:
		CFontCache() : CCache(20) {}
		CFontWrapper* Create(const LOGFONT& lf);
	};

	class CGlyphPath
	{
	public:
		CGlyphPath() {}
		virtual ~CGlyphPath() {}

		CGlyphPath(const CGlyphPath& path);
		void operator = (const CGlyphPath& path);

		void Enlarge(const CGlyphPath& src, float size);

		CAtlArray<BYTE> types;
		CAtlArray<POINT> points;
	};

	class CGlyphPathCache : public CCache<CGlyphPath*>
	{
	public:
		CGlyphPathCache() : CCache(100) {}
		CGlyphPath* Create(HDC hDC, const CFontWrapper* f, WCHAR c);
	};

	class CGlyph
	{
	public:
		WCHAR c;
		ssf::Style style;
		ssf::Size scale;
		bool vertical;
		int ascent, descent, width, spacing, fill;
		int row_ascent, row_descent;
		CGlyphPath path, path_bkg;
		CRect bbox;
		CPoint tl, tls;
		SSFRasterizer ras, ras_bkg, ras_shadow;

		float GetBackgroundSize();
		float GetShadowDepth();

		void Transform(CGlyphPath& path, CPoint org);

	public:
		CGlyph();
		void Transform(CPoint org);
		void Rasterize();
		CRect GetClipRect();
	};

	class CRow : public CAutoPtrList<CGlyph>
	{
	public:
		int ascent, descent, width;
		CRow() {ascent = descent = width = 0;}
	};

	class CSubtitle
	{
	public:
		CRect m_spdrc;
		CRect m_clip;
		CAutoPtrList<CGlyph> m_glyphs;

		CSubtitle(const CRect& spdrc, const CRect& clip) : m_spdrc(spdrc), m_clip(clip) {}
		virtual ~CSubtitle() {}
	};

	class CSubtitleCache : public CCache<CSubtitle*>
	{
	public:
		CSubtitleCache() : CCache(10) {}
		void Create(const CStringW& name, CSubtitle* sub);
	};

	CFontCache m_fontcache;
	CGlyphPathCache m_glyphpathcache;
	CSubtitleCache m_subtitlecache;

public:
	CRenderedSSF(CCritSec* pLock);
	virtual ~CRenderedSSF();

	bool Open(CString fn, CString name = _T(""));
	bool Open(ssf::InputStream& s, CString name);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicProvider
	STDMETHODIMP_(POSITION) GetStartPosition(REFERENCE_TIME rt, double fps);
	STDMETHODIMP_(POSITION) GetNext(POSITION pos);
	STDMETHODIMP_(REFERENCE_TIME) GetStart(POSITION pos, double fps);
	STDMETHODIMP_(REFERENCE_TIME) GetStop(POSITION pos, double fps);
	STDMETHODIMP_(bool) IsAnimated(POSITION pos);
	STDMETHODIMP Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox);

	// IPersist
	STDMETHODIMP GetClassID(CLSID* pClassID);

	// ISubStream
	STDMETHODIMP_(int) GetStreamCount();
	STDMETHODIMP GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID);
	STDMETHODIMP_(int) GetStream();
	STDMETHODIMP SetStream(int iStream);
	STDMETHODIMP Reload();
};
