#include "stdafx.h"
#include <atlbase.h>
#include "MediaFormats.h"

//
// CMediaFormatCategory
//

CMediaFormatCategory::CMediaFormatCategory()
	: m_fAudioOnly(false)
{
}

CMediaFormatCategory::CMediaFormatCategory(
	CString label, const CStringList& exts, bool fAudioOnly,
	CString specreqnote, engine_t engine)
{
	m_label = label;
	m_exts.AddTail((CStringList*)&exts);
	m_backupexts.AddTail(&m_exts);
	m_specreqnote = specreqnote;
	m_fAudioOnly = fAudioOnly;
	m_engine = engine;
}

CMediaFormatCategory::CMediaFormatCategory(
	CString label, CString exts, bool fAudioOnly,
	CString specreqnote, engine_t engine)
{
	m_label = label;
	int i = 0;
	for(CString token = exts.Tokenize(_T(" "), i); !token.IsEmpty(); token = exts.Tokenize(_T(" "), i))
		m_exts.AddTail(token.TrimLeft('.'));
	m_backupexts.AddTail(&m_exts);
	m_specreqnote = specreqnote;
	m_fAudioOnly = fAudioOnly;
	m_engine = engine;
}

CMediaFormatCategory::~CMediaFormatCategory()
{
}

void CMediaFormatCategory::UpdateData(bool fSave)
{
	if(fSave)
	{
		AfxGetApp()->WriteProfileString(_T("FileFormats"), m_label, GetExts());
	}
	else
	{
		SetExts(AfxGetApp()->GetProfileString(_T("FileFormats"), m_label, GetExts()));
	}
}

CMediaFormatCategory::CMediaFormatCategory(const CMediaFormatCategory& mfc)
{
	*this = mfc;
}

CMediaFormatCategory& CMediaFormatCategory::operator = (const CMediaFormatCategory& mfc)
{
	m_label = mfc.m_label;
	m_specreqnote = mfc.m_specreqnote;
	m_exts.RemoveAll();
	m_exts.AddTail((CStringList*)&mfc.m_exts);
	m_backupexts.RemoveAll();
	m_backupexts.AddTail((CStringList*)&mfc.m_backupexts);
	m_fAudioOnly = mfc.m_fAudioOnly;
	m_engine = mfc.m_engine;

	return(*this);
}

void CMediaFormatCategory::RestoreDefaultExts()
{
	m_exts.RemoveAll();
	m_exts.AddTail((CStringList*)&m_backupexts);
}

void CMediaFormatCategory::SetExts(CStringList& exts)
{
	m_exts.RemoveAll();
	m_exts.AddTail((CStringList*)&exts);
}

void CMediaFormatCategory::SetExts(CString exts)
{
	m_exts.RemoveAll();
	int i = 0;
	for(CString token = exts.Tokenize(_T(" "), i); !token.IsEmpty(); token = exts.Tokenize(_T(" "), i))
		m_exts.AddTail(token.TrimLeft('.'));
}

CString CMediaFormatCategory::GetFilter()
{
	CString filter;
	POSITION pos = m_exts.GetHeadPosition();
	while(pos) filter += _T("*.") + m_exts.GetNext(pos) + _T(";");
	filter.TrimRight(_T(";")); // cheap...
	return(filter);
}

CString CMediaFormatCategory::GetExts()
{
	CString exts;
	POSITION pos = m_exts.GetHeadPosition();
	while(pos) exts += m_exts.GetNext(pos) + _T(" ");
	exts.TrimRight(_T(" ")); // cheap...
	return(exts);
}

CString CMediaFormatCategory::GetExtsWithPeriod()
{
	CString exts;
	POSITION pos = m_exts.GetHeadPosition();
	while(pos) exts += _T(".") + m_exts.GetNext(pos) + _T(" ");
	exts.TrimRight(_T(" ")); // cheap...
	return(exts);
}

//
// 	CMediaFormats
//

CMediaFormats::CMediaFormats()
{
}

CMediaFormats::~CMediaFormats()
{
}

void CMediaFormats::UpdateData(bool fSave)
{
	if(fSave)
	{
		AfxGetApp()->WriteProfileString(_T("FileFormats"), NULL, NULL);

		AfxGetApp()->WriteProfileInt(_T("FileFormats"), _T("RtspHandler"), m_iRtspHandler);
		AfxGetApp()->WriteProfileInt(_T("FileFormats"), _T("RtspFileExtFirst"), m_fRtspFileExtFirst);
	}
	else
	{
		RemoveAll();
#define ADDFMT(f) Add(CMediaFormatCategory##f)
		ADDFMT((_T("Windows Media file"), _T("wmv wmp wm asf")));
		ADDFMT((_T("Windows Media Audio file"), _T("wma"), true));
		ADDFMT((_T("Video file"), _T("avi")));
		ADDFMT((_T("Audio file"), _T("wav"), true));
		ADDFMT((_T("Movie file (MPEG)"), _T("mpg mpeg mpe m1v m2v mpv2 mp2v dat")));
		ADDFMT((_T("Movie audio file (MPEG)"), _T("mpa mp2 m1a m2a"), true));
		ADDFMT((_T("DVD file"), _T("vob ifo")));
		ADDFMT((_T("DVD Audio file"), _T("ac3 dts"), true));
		ADDFMT((_T("MP3 Format Sound"), _T("mp3"), true));
		ADDFMT((_T("MIDI file"), _T("mid midi rmi"), true));
		ADDFMT((_T("Indeo Video file"), _T("ivf")));
		ADDFMT((_T("AIFF Format Sound"), _T("aif aifc aiff"), true));
		ADDFMT((_T("AU Format Sound"), _T("au snd"), true));
		ADDFMT((_T("Ogg Media file"), _T("ogm")));
		ADDFMT((_T("Ogg Vorbis Audio file"), _T("ogg"), true));
		ADDFMT((_T("CD Audio Track"), _T("cda"), true, _T("Only for 2k/xp+")));
		ADDFMT((_T("FLIC file"), _T("fli flc flic")));
		ADDFMT((_T("DVD2AVI Project file"), _T("d2v")));
		ADDFMT((_T("MPEG4 file"), _T("mp4")));
		ADDFMT((_T("MPEG4 Audio file"), _T("aac"), true));
		ADDFMT((_T("Matroska Media file"), _T("mkv")));
		ADDFMT((_T("Matroska Audio file"), _T("mka"), true));
		ADDFMT((_T("Real Media file"), _T("rm rmvb rpm rt rp smi smil"), false, _T("with RealPlayer/RealOne or codec pack"), RealMedia));
		ADDFMT((_T("Real Audio file"), _T("ra ram"), true, _T("needs RealPlayer/RealOne or codec pack"), RealMedia));
		ADDFMT((_T("Shockwave Flash file"), _T("swf"), false, _T("ActiveX control must be installed"), ShockWave));
		ADDFMT((_T("Quicktime file"), _T("mov qt"), false, _T("needs QuickTime Player or codec pack"), QuickTime));
		ADDFMT((_T("Image file"), _T("jpeg jpg bmp gif pic png dib tiff tif")));
		ADDFMT((_T("Playlist file"), _T("asx m3u pls wvx wax wmx"), false));
#undef ADDFMT

		m_iRtspHandler = (engine_t)AfxGetApp()->GetProfileInt(_T("FileFormats"), _T("RtspHandler"), (int)RealMedia);
		m_fRtspFileExtFirst = !!AfxGetApp()->GetProfileInt(_T("FileFormats"), _T("RtspFileExtFirst"), 1);
	}

	for(int i = 0; i < GetCount(); i++)
		GetAt(i).UpdateData(fSave);
}

engine_t CMediaFormats::GetRtspHandler(bool& fRtspFileExtFirst)
{
	fRtspFileExtFirst = m_fRtspFileExtFirst;
	return m_iRtspHandler;
}

void CMediaFormats::SetRtspHandler(engine_t e, bool fRtspFileExtFirst)
{
	m_iRtspHandler = e;
	m_fRtspFileExtFirst = fRtspFileExtFirst;
}

bool CMediaFormats::IsUsingEngine(CString path, engine_t e)
{
	return(GetEngine(path) == e);
}

engine_t CMediaFormats::GetEngine(CString path)
{
	path.Trim().MakeLower();

	if(!m_fRtspFileExtFirst && path.Find(_T("rtsp://")) == 0)
		return m_iRtspHandler;

	CString ext = CPath(path).GetExtension();
	if(!ext.IsEmpty())
	{
		for(int i = 0; i < GetCount(); i++)
		{
			CMediaFormatCategory& mfc = GetAt(i);
			if(mfc.FindExt(ext))
				return mfc.GetEngineType();
		}
	}

	if(m_fRtspFileExtFirst && path.Find(_T("rtsp://")) == 0)
		return m_iRtspHandler;

	return DirectShow;
}

bool CMediaFormats::FindExt(CString ext, bool fAudioOnly)
{
	ext.TrimLeft(_T("."));

	if(!ext.IsEmpty())
	{
		for(int i = 0; i < GetCount(); i++)
		{
			CMediaFormatCategory& mfc = GetAt(i);
			if((!fAudioOnly || mfc.IsAudioOnly()) && mfc.FindExt(ext)) 
				return(true);
		}
	}

	return(false);
}
