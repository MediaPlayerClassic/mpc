// Media Player Classic.  Copyright 2003 Gabest.
// http://www.gabest.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html

#pragma once

#include <atlcoll.h>
#include <afxtempl.h>

typedef enum {DirectShow = 0, RealMedia, QuickTime, ShockWave} engine_t;

class CMediaFormatCategory
{
public:
protected:
	CString m_label, m_specreqnote;
	CStringList m_exts, m_backupexts;
	bool m_fAudioOnly;
	engine_t m_engine;

public:
	CMediaFormatCategory();
	CMediaFormatCategory(
		CString label, const CStringList& exts, bool fAudioOnly = false,
		CString specreqnote =  _T(""), engine_t e = DirectShow);
	CMediaFormatCategory(
		CString label, CString exts, bool fAudioOnly = false,
		CString specreqnote =  _T(""), engine_t e = DirectShow);
	virtual ~CMediaFormatCategory();

	void UpdateData(bool fSave);

	CMediaFormatCategory(const CMediaFormatCategory& mfc);
	CMediaFormatCategory& operator = (const CMediaFormatCategory& mfc);

	void RestoreDefaultExts();
	void SetExts(CStringList& exts);
	void SetExts(CString exts);

	bool FindExt(CString ext) {return m_exts.Find(ext.TrimLeft(_T(".")).MakeLower()) != NULL;}

	CString GetLabel() {return m_label;}
	CString GetFilter(), GetExts(), GetExtsWithPeriod();
	CString GetSpecReqNote() {return m_specreqnote;}
	bool IsAudioOnly() {return m_fAudioOnly;}
	engine_t GetEngineType() {return m_engine;}
};

class CMediaFormats : public CArray<CMediaFormatCategory>
{
protected:
	engine_t m_iRtspHandler;
	bool m_fRtspFileExtFirst;

public:
	CMediaFormats();
	virtual ~CMediaFormats();

	void UpdateData(bool fSave);

	engine_t GetRtspHandler(bool& fRtspFileExtFirst);
	void SetRtspHandler(engine_t e, bool fRtspFileExtFirst);

	bool IsUsingEngine(CString path, engine_t e);
	engine_t GetEngine(CString path);

	bool FindExt(CString ext, bool fAudioOnly = false);
};
