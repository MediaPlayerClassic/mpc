/* 
 *	Media Player Classic.  Copyright (C) 2003 Gabest
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
#include "mplayerc.h"
#include "GraphBuilder.h"
#include "..\..\DSUtil\DSUtil.h"
#include "..\..\filters\filters.h"
#include "..\..\..\include\moreuuids.h"
#include "..\..\..\include\Ogg\OggDS.h"
#include "..\..\..\include\matroska\matroska.h"
#include "DX7AllocatorPresenter.h"
#include "DX9AllocatorPresenter.h"

#include "C:\WMSDK\WMFSDK9\include\wmsdk.h"

#include <initguid.h>
#include <dmodshow.h>

#include <D3d9.h>
#include <Vmr9.h>

static void CheckStupidSharedFilesourceFilter()
{
	CRegKey key;
	CString str(_T("CLSID\\{E436EBB5-524F-11CE-9F53-0020AF0BA770}\\InprocServer32"));

	if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, str, KEY_READ))
	{
		ULONG nChars = 0;
		if(ERROR_SUCCESS == key.QueryStringValue(NULL, NULL, &nChars))
		{
			CString dll;
			if(ERROR_SUCCESS == key.QueryStringValue(NULL, dll.GetBuffer(nChars), &nChars))
			{
				dll.ReleaseBuffer(nChars);
				dll.MakeLower();
				if(dll.Find(_T("shared_filesource.ax")) >= 0
				|| dll.Find(_T("shared~1.ax")) >= 0)
				{
					if(IDYES == AfxMessageBox(
						_T("Warning: MPC has detected that DirectShow's \"File Source (async)\" filter\n")
						_T("was replaced by shared_filesource.ax. Please run \"regsvr32.exe quartz.dll\"\n")
						_T("in your system folder to restore it. If you are a user with administrator\n")
						_T("rights, MPC can also do this for you. Do you want to re-register quartz.dll now?"), 
						MB_YESNO|MB_ICONEXCLAMATION))
					{
						TCHAR buff[MAX_PATH+1];
						GetSystemDirectory(buff, sizeof(buff));
					
						CString path = CString(buff) + _T("\\quartz.dll");

						HRESULT hr = E_FAIL;

						if(HMODULE hModule = LoadLibrary(path))
						{
							typedef HRESULT (__stdcall * PDllRegisterServer)();
							if(PDllRegisterServer p = (PDllRegisterServer)GetProcAddress(hModule, "DllRegisterServer"))
								hr = p();
						}

						if(SUCCEEDED(hr))
							AfxMessageBox(_T("Successfully re-registered quartz.dll, you may need to restart the player now."), MB_OK);
						else
							AfxMessageBox(_T("Failed to re-register quartz.dll!"), MB_OK|MB_ICONEXCLAMATION);
					}
				}
			}
		}
	}
}

//
// CGraphBuilder
//

CGraphBuilder::CGraphBuilder(IGraphBuilder* pGB, HWND hWnd)
	: m_pGB(pGB), m_hWnd(hWnd)
	, m_nTotalStreams(0)
	, m_nCurrentStream(0)
	, m_VRMerit(LMERIT(MERIT_PREFERRED+1)+0x100)
	, m_ARMerit(LMERIT(MERIT_PREFERRED+1)+0x100)
{
	AppSettings& s = AfxGetAppSettings();

	m_pFM.CoCreateInstance(CLSID_FilterMapper2);
	if(!m_pFM) return;

	{
		CArray<GUID> guids;
		CComPtr<IEnumMoniker> pEM;

		m_VRMerit = LMERIT(MERIT_PREFERRED);

		guids.Add(MEDIATYPE_Video);
		guids.Add(MEDIASUBTYPE_NULL);
		if(guids.GetCount() > 0
		&& SUCCEEDED(m_pFM->EnumMatchingFilters(
			&pEM, 0, FALSE, MERIT_DO_NOT_USE+1,
			TRUE, guids.GetCount()/2, guids.GetData(), NULL, NULL, TRUE,
			FALSE, 0, NULL, NULL, NULL)))
		{
			for(CComPtr<IMoniker> pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
			{
				CGraphRegFilter gf(pMoniker);
				m_VRMerit = max(m_VRMerit, gf.GetMerit());
			}
		}
		guids.RemoveAll();
		pEM = NULL;

		m_VRMerit += 0x100;

		m_ARMerit = LMERIT(MERIT_PREFERRED);

		guids.Add(MEDIATYPE_Audio);
		guids.Add(MEDIASUBTYPE_NULL);
		if(guids.GetCount() > 0
		&& SUCCEEDED(m_pFM->EnumMatchingFilters(
			&pEM, 0, FALSE, MERIT_DO_NOT_USE+1,
			TRUE, guids.GetCount()/2, guids.GetData(), NULL, NULL, TRUE,
			FALSE, 0, NULL, NULL, NULL)))
		{
			for(CComPtr<IMoniker> pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
			{
				CGraphRegFilter gf(pMoniker);
				m_ARMerit = max(m_ARMerit, gf.GetMerit());
			}
		}

		BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker)
		{
			CGraphRegFilter gf(pMoniker);
			m_ARMerit = max(m_ARMerit, gf.GetMerit());
		}
		EndEnumSysDev

		guids.RemoveAll();
		pEM = NULL;

		m_ARMerit += 0x100;
	}

	// transform filters

	CList<GUID> guids;

	guids.AddTail(MEDIATYPE_Audio);
	guids.AddTail(MEDIASUBTYPE_WAVE_DOLBY_AC3);
	guids.AddTail(MEDIATYPE_Audio);
	guids.AddTail(MEDIASUBTYPE_WAVE_DTS);
	AddFilter(new CGraphCustomFilter(__uuidof(CAVI2AC3Filter), guids, L"AVI<->AC3/DTS", LMERIT(0x00680000)+1));
	guids.RemoveAll();

	{
		guids.AddTail(MEDIATYPE_Stream);
		guids.AddTail(MEDIASUBTYPE_Matroska);
		AddFilter(new CGraphCustomFilter(__uuidof(CMatroskaSplitterFilter), guids, 
			(s.SrcFilters&SRC_MATROSKA) ? L"Matroska Splitter" : L"Matroska Splitter (low merit)", 
			(s.SrcFilters&SRC_MATROSKA) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_Stream);
		guids.AddTail(MEDIASUBTYPE_RealMedia);
		AddFilter(new CGraphCustomFilter(__uuidof(CRealMediaSplitterFilter), guids, 
			(s.SrcFilters&SRC_REALMEDIA) ? L"RealMedia Splitter" : L"RealMedia Splitter (low merit)", 
			(s.SrcFilters&SRC_REALMEDIA) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_Stream);
		guids.AddTail(MEDIASUBTYPE_Avi);
		AddFilter(new CGraphCustomFilter(__uuidof(CAviSplitterFilter), guids, 
			(s.SrcFilters&SRC_AVI) ? L"Avi Splitter" : L"Avi Splitter (low merit)",
			(s.SrcFilters&SRC_AVI) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_Stream);
		guids.AddTail(MEDIASUBTYPE_Bink);
		guids.AddTail(MEDIATYPE_Stream);
		guids.AddTail(MEDIASUBTYPE_Smacker);
		AddFilter(new CGraphCustomFilter(__uuidof(CRadGtSplitterFilter), guids, 
			(s.SrcFilters&SRC_RADGT) ? L"RadGt Splitter" : L"RadGt Splitter (low merit)",
			(s.SrcFilters&SRC_RADGT) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_Stream);
		guids.AddTail(MEDIASUBTYPE_RoQ);
		AddFilter(new CGraphCustomFilter(__uuidof(CRoQSplitterFilter), guids, 
			(s.SrcFilters&SRC_ROQ) ? L"RoQ Splitter" : L"RoQ Splitter (low merit)",
			(s.SrcFilters&SRC_ROQ) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_Stream);
		guids.AddTail(MEDIASUBTYPE_Ogg);
		AddFilter(new CGraphCustomFilter(__uuidof(COggSplitterFilter), guids, 
			(s.SrcFilters&SRC_OGG) ? L"Ogg Splitter" : L"Ogg Splitter (low merit)",
			(s.SrcFilters&SRC_OGG) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_Video);
		guids.AddTail(MEDIASUBTYPE_MPEG1Packet);
		guids.AddTail(MEDIATYPE_Video);
		guids.AddTail(MEDIASUBTYPE_MPEG1Payload);
		AddFilter(new CGraphCustomFilter(__uuidof(CMpeg2DecFilter), guids, 
			(s.TraFilters&TRA_MPEG1) ? L"MPEG-1 Video Decoder" : L"MPEG-1 Video Decoder (low merit)", 
			(s.TraFilters&TRA_MPEG1) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_DVD_ENCRYPTED_PACK);
		guids.AddTail(MEDIASUBTYPE_MPEG2_VIDEO);
		guids.AddTail(MEDIATYPE_MPEG2_PACK);
		guids.AddTail(MEDIASUBTYPE_MPEG2_VIDEO);
		guids.AddTail(MEDIATYPE_MPEG2_PES);
		guids.AddTail(MEDIASUBTYPE_MPEG2_VIDEO);
		guids.AddTail(MEDIATYPE_Video);
		guids.AddTail(MEDIASUBTYPE_MPEG2_VIDEO);
		AddFilter(new CGraphCustomFilter(__uuidof(CMpeg2DecFilter), guids,
			(s.TraFilters&TRA_MPEG2) ? L"MPEG-2 Video Decoder" : L"MPEG-2 Video Decoder (low merit)", 
			(s.TraFilters&TRA_MPEG2) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_MP3);
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_MPEG1AudioPayload);
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_MPEG1Payload);
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_MPEG1Packet);
		AddFilter(new CGraphCustomFilter(__uuidof(CMpaDecFilter), guids, 
			(s.TraFilters&TRA_MPEGAUD) ? L"MPEG-1 Audio Decoder" : L"MPEG-1 Audio Decoder (low merit)",
			(s.TraFilters&TRA_MPEGAUD) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_DVD_ENCRYPTED_PACK);
		guids.AddTail(MEDIASUBTYPE_MPEG2_AUDIO);
		guids.AddTail(MEDIATYPE_MPEG2_PACK);
		guids.AddTail(MEDIASUBTYPE_MPEG2_AUDIO);
		guids.AddTail(MEDIATYPE_MPEG2_PES);
		guids.AddTail(MEDIASUBTYPE_MPEG2_AUDIO);
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_MPEG2_AUDIO);
		AddFilter(new CGraphCustomFilter(__uuidof(CMpaDecFilter), guids, 
			(s.TraFilters&TRA_MPEGAUD) ? L"MPEG-2 Audio Decoder" : L"MPEG-2 Audio Decoder (low merit)",
			(s.TraFilters&TRA_MPEGAUD) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_DVD_ENCRYPTED_PACK);
		guids.AddTail(MEDIASUBTYPE_DVD_LPCM_AUDIO);
		guids.AddTail(MEDIATYPE_MPEG2_PACK);
		guids.AddTail(MEDIASUBTYPE_DVD_LPCM_AUDIO);
		guids.AddTail(MEDIATYPE_MPEG2_PES);
		guids.AddTail(MEDIASUBTYPE_DVD_LPCM_AUDIO);
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_DVD_LPCM_AUDIO);
		AddFilter(new CGraphCustomFilter(__uuidof(CMpaDecFilter), guids, 
			(s.TraFilters&TRA_MPEGAUD) ? L"LPCM Audio Decoder" : L"LPCM Audio Decoder (low merit)",
			(s.TraFilters&TRA_MPEGAUD) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_Video);
		guids.AddTail(MEDIASUBTYPE_RV10);
		guids.AddTail(MEDIATYPE_Video);
		guids.AddTail(MEDIASUBTYPE_RV20);
		guids.AddTail(MEDIATYPE_Video);
		guids.AddTail(MEDIASUBTYPE_RV30);
		guids.AddTail(MEDIATYPE_Video);
		guids.AddTail(MEDIASUBTYPE_RV40);
		AddFilter(new CGraphCustomFilter(__uuidof(CRealVideoDecoder), guids,
			(s.TraFilters&TRA_REALVID) ? L"RealVideo Decoder" : L"RealVideo Decoder (low merit)",
			(s.TraFilters&TRA_REALVID) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_14_4);
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_28_8);
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_ATRC);
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_COOK);
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_DNET);
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_SIPR);
		AddFilter(new CGraphCustomFilter(__uuidof(CRealAudioDecoder), guids, 
			(s.TraFilters&TRA_REALAUD) ? L"RealAudio Decoder" : L"RealAudio Decoder (low merit)",
			(s.TraFilters&TRA_REALAUD) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_Video);
		guids.AddTail(MEDIASUBTYPE_RoQV);
		AddFilter(new CGraphCustomFilter(__uuidof(CRoQVideoDecoder), guids,
			(s.TraFilters&TRA_REALVID) ? L"RoQ Video Decoder" : L"RoQ Video Decoder (low merit)",
			(s.TraFilters&TRA_REALVID) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	{
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_RoQA);
		AddFilter(new CGraphCustomFilter(__uuidof(CRoQAudioDecoder), guids,
			(s.TraFilters&TRA_REALVID) ? L"RoQ Audio Decoder" : L"RoQ Audio Decoder (low merit)",
			(s.TraFilters&TRA_REALVID) ? LMERIT_ABOVE_DSHOW : LMERIT_DO_USE));
		guids.RemoveAll();
	}

	// renderer filters

	switch(s.iDSVideoRendererType)
	{
		default: 
		case VIDRNDT_DS_DEFAULT: 
			break;
		case VIDRNDT_DS_OLDRENDERER: 
			AddFilter(new CGraphRegFilter(CLSID_VideoRenderer, m_VRMerit));
			break;
		case VIDRNDT_DS_OVERLAYMIXER:
			AddFilter(new CGraphRendererFilter(CLSID_OverlayMixer, m_hWnd, L"Overlay Mixer", m_VRMerit));
			break;
		case VIDRNDT_DS_VMR7WINDOWED:
			AddFilter(new CGraphRendererFilter(CLSID_VideoMixingRenderer, m_hWnd, L"Video Mixing Render 7 (Windowed)", m_VRMerit));
			break;
		case VIDRNDT_DS_VMR9WINDOWED:
			AddFilter(new CGraphRendererFilter(CLSID_VideoMixingRenderer9, m_hWnd, L"Video Mixing Render 9 (Windowed)", m_VRMerit));
			break;
		case VIDRNDT_DS_VMR7RENDERLESS:
			AddFilter(new CGraphRendererFilter(CLSID_VMR7AllocatorPresenter, m_hWnd, L"Video Mixing Render 7 (Renderless)", m_VRMerit));
			break;
		case VIDRNDT_DS_VMR9RENDERLESS:
			AddFilter(new CGraphRendererFilter(CLSID_VMR9AllocatorPresenter, m_hWnd, L"Video Mixing Render 9 (Renderless)", m_VRMerit));
			break;
		case VIDRNDT_DS_NULL_COMP:
			guids.AddTail(MEDIATYPE_Video);
			guids.AddTail(MEDIASUBTYPE_NULL);
			AddFilter(new CGraphCustomFilter(__uuidof(CNullVideoRenderer), guids, L"Null Video Renderer (Any)", LMERIT_ABOVE_DSHOW+1));
			guids.RemoveAll();
			break;
		case VIDRNDT_DS_NULL_UNCOMP:
			guids.AddTail(MEDIATYPE_Video);
			guids.AddTail(MEDIASUBTYPE_NULL);
			AddFilter(new CGraphCustomFilter(__uuidof(CNullUVideoRenderer), guids, L"Null Video Renderer (Uncompressed)", LMERIT_ABOVE_DSHOW+1));
			guids.RemoveAll();
			break;
	}

	if(s.AudioRendererDisplayName == AUDRNDT_NULL_COMP)
	{
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_NULL);
		AddFilter(new CGraphCustomFilter(__uuidof(CNullAudioRenderer), guids, AUDRNDT_NULL_COMP, LMERIT_ABOVE_DSHOW+1));
		guids.RemoveAll();
	}
	else if(s.AudioRendererDisplayName == AUDRNDT_NULL_UNCOMP)
	{
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_NULL);
		AddFilter(new CGraphCustomFilter(__uuidof(CNullUAudioRenderer), guids, AUDRNDT_NULL_UNCOMP, LMERIT_ABOVE_DSHOW+1));
		guids.RemoveAll();
	}
	else if(!s.AudioRendererDisplayName.IsEmpty())
	{
		AddFilter(new CGraphRegFilter(s.AudioRendererDisplayName, m_ARMerit));
	}

	WORD lowmerit = 1;
	POSITION pos = s.filters.GetTailPosition();
	while(pos)
	{
		Filter* f = s.filters.GetPrev(pos);

		if(f->fDisabled
		|| f->type == Filter::EXTERNAL && !CPath(MakeFullPath(f->path)).FileExists()) 
			continue;

		ULONGLONG merit = 
			f->iLoadType == Filter::PREFERRED ? LMERIT_ABOVE_DSHOW : 
			f->iLoadType == Filter::MERIT ? LMERIT(f->dwMerit) : 
			LMERIT_DO_NOT_USE; // f->iLoadType == Filter::BLOCKED

		merit += lowmerit++;

		CGraphFilter* gf = NULL;

		if(f->type == Filter::REGISTERED)
		{
			gf = new CGraphRegFilter(f->dispname, merit);
		}
		else if(f->type == Filter::EXTERNAL)
		{
			gf = new CGraphFileFilter(f->clsid, f->guids, f->path, CStringW(f->name), merit);
		}

		if(gf)
		{
			gf->SetGUIDs(f->guids);
			AddFilter(gf);
		}
	}

	guids.AddTail(MEDIATYPE_Text);
	guids.AddTail(MEDIASUBTYPE_NULL);
	guids.AddTail(MEDIATYPE_Subtitle);
	guids.AddTail(MEDIASUBTYPE_NULL);
	AddFilter(new CGraphCustomFilter(__uuidof(CNullTextRenderer), guids, L"NullTextRenderer", LMERIT_DO_USE));
	guids.RemoveAll();

	// FIXME: "Subtitle Mixer" makes an access violation around 
	// the 11-12th media type when enumerating them on its output.
	CLSID CLSID_SubtitlerMixer = GUIDFromCString(_T("{00A95963-3BE5-48C0-AD9F-3356D67EA09D}"));
	AddFilter(new CGraphRegFilter(CLSID_SubtitlerMixer, LMERIT_DO_NOT_USE));

	// ISCR suxx
	AddFilter(new CGraphRegFilter(GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}")), LMERIT_DO_NOT_USE));
}

CGraphBuilder::~CGraphBuilder()
{
}
/*
void CGraphBuilder::LOG(LPCTSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	if(TCHAR* buff = new TCHAR[_vsctprintf(fmt, args) + 1])
	{
		_vstprintf(buff, fmt, args);
		m_log.AddTail(buff);
		TRACE(_T("GraphBuilder: Stream[%d]: %s\n"), m_iStream, buff);
		if(FILE* f = _tfopen(_T("c:\\mpcgb.txt"), _T("at")))
		{
			fseek(f, 0, 2);
			_ftprintf(f, _T("Stream[%d]: %s\n"), m_iStream, buff);
			fclose(f);
		}
		delete [] buff;
	}
	va_end(args);
}
*/

void CGraphBuilder::ExtractMediaTypes(IPin* pPin, CArray<GUID>& guids)
{
	guids.RemoveAll();

    BeginEnumMediaTypes(pPin, pEM, pmt)
	{
		bool fFound = false;

		for(int i = 0; !fFound && i < guids.GetCount(); i += 2)
		{
			if(guids[i] == pmt->majortype && guids[i+1] == pmt->subtype)
				fFound = true;
		}

		if(!fFound)
		{
			guids.Add(pmt->majortype);
			guids.Add(pmt->subtype);
		}
	}
	EndEnumMediaTypes(pmt)
}

void CGraphBuilder::ExtractMediaTypes(IPin* pPin, CList<CMediaType>& mts)
{
	mts.RemoveAll();

    BeginEnumMediaTypes(pPin, pEM, pmt)
	{
		bool fFound = false;

		POSITION pos = mts.GetHeadPosition();
		while(!fFound && pos)
		{
			CMediaType& mt = mts.GetNext(pos);
			if(mt.majortype == pmt->majortype && mt.subtype == pmt->subtype)
				fFound = true;
		}

		if(!fFound)
		{
			mts.AddTail(CMediaType(*pmt));
		}
	}
	EndEnumMediaTypes(pmt)
}

void CGraphBuilder::SaveFilters(CInterfaceList<IBaseFilter>& bfl)
{
	bfl.RemoveAll();
	BeginEnumFilters(m_pGB, pEF, pBF)
		bfl.AddTail(pBF);
	EndEnumFilters
}

void CGraphBuilder::RestoreFilters(CInterfaceList<IBaseFilter>& bfl)
{
	BeginEnumFilters(m_pGB, pEF, pBF)
		if(!bfl.Find(pBF)) {m_pGB->RemoveFilter(pBF); pEF->Reset();}
	EndEnumFilters
}

HRESULT CGraphBuilder::SafeAddFilter(IBaseFilter* pBF, LPCWSTR pName)
{
	if(!m_pGB || !pBF)
		return E_FAIL;

	bool fFound = false;
	BeginEnumFilters(m_pGB, pEF, pBF2)
		if(pBF == pBF2) fFound = true;
	EndEnumFilters
	if(fFound) return S_OK;

	CFilterInfo fi;
	if(SUCCEEDED(pBF->QueryFilterInfo(&fi)))
	{
		if(!fi.pGraph) // not in graph yet?
		{
			CStringW name;
			
			if(pName && wcslen(pName) > 0)
			{
				name = pName;
			}
			else
			{
				if(CComQIPtr<IFileSourceFilter> pFSF = pBF)
				{
					CMediaType mt;
					LPOLESTR str = NULL;
					if(SUCCEEDED(pFSF->GetCurFile(&str, &mt))) name = str;
					if(str) CoTaskMemFree(str);
				}

				if(name.IsEmpty())
				{
					CLSID clsid = GetCLSID(pBF);
					name = clsid == GUID_NULL ? L"Unknown Filter" : CStringFromGUID(clsid);
				}
			}

			if(FAILED(m_pGB->AddFilter(pBF, name)))
			{
				return E_FAIL;
			}
		}

		return S_OK;
	}

	return E_FAIL;
}

void CGraphBuilder::Reset()
{
	m_nTotalStreams = 0;
	m_nCurrentStream = 0;
	m_pUnks.RemoveAll();
	m_DeadEnds.RemoveAll();
//	m_log.RemoveAll();
}

HRESULT CGraphBuilder::Render(LPCTSTR lpsz)
{
	if(!m_pGB) return E_FAIL;

	CString fn = CString(lpsz).Trim();
	if(fn.IsEmpty()) return E_FAIL;
	CStringW fnw = fn;
	CString ext = CPath(fn).GetExtension().MakeLower();

	AppSettings& s = AfxGetAppSettings();

	HRESULT hr;

	CComQIPtr<IBaseFilter> pBF;

	CComQIPtr<IGraphEngine> pGE = m_pGB;
	if(!pGE || pGE->GetEngine() == DirectShow)
	{
		if((s.SrcFilters&SRC_CDDA) && !pBF && ext == _T(".cda"))
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CCDDAReader(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if((s.SrcFilters&SRC_CDXA) && !pBF)
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CCDXAReader(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if((s.SrcFilters&SRC_VTS) && !pBF) //&& ext == _T(".ifo"))
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CVTSReader(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if((s.SrcFilters&SRC_FLIC) && !pBF) //&& (ext == _T(".fli") || ext == _T(".flc")))
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CFLICSource(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if((s.SrcFilters&SRC_DVD2AVI) && !pBF) //&& ext == _T(".d2v"))
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CD2VSource(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if((s.SrcFilters&SRC_DTSAC3) && !pBF) //&& (ext == _T(".dts") || ext == _T(".ac3")))
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CDTSAC3Source(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if((s.SrcFilters&SRC_SHOUTCAST) && !pBF && fn.Find(_T("://")) >= 0)
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CShoutcastSource(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if((s.SrcFilters&SRC_MATROSKA) && !pBF) //&& (ext == _T(".mkv") || ext == _T(".mka") || ext == _T(".mks")))
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CMatroskaSourceFilter(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if((s.SrcFilters&SRC_REALMEDIA) && !pBF)
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CRealMediaSourceFilter(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if((s.SrcFilters&SRC_AVI) && !pBF)
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CAviSourceFilter(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if((s.SrcFilters&SRC_RADGT) && !pBF)
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CRadGtSourceFilter(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if((s.SrcFilters&SRC_ROQ) && !pBF)
		{
			hr = S_OK;
			CComPtr<IFileSourceFilter> pReader = new CRoQSourceFilter(NULL, &hr);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}

		if(!pBF && AfxGetAppSettings().fUseWMASFReader && fn.Find(_T("://")) < 0)
		{
			bool fWindowsMedia = (ext == _T(".asf") || ext == _T(".wmv") || ext == _T(".wma"));

			if(!fWindowsMedia)
			{
				CFile f;
				if(f.Open(fn, CFile::modeRead))
				{
					BYTE buff[4];
					memset(buff, 0, sizeof(buff));
					f.Read(buff, sizeof(buff));
					if(*(DWORD*)buff == 0x75b22630)
						fWindowsMedia = true;
				}
			}

			if(fWindowsMedia)
			{
				CComPtr<IFileSourceFilter> pReader;
				hr = pReader.CoCreateInstance(CLSID_WMAsfReader);
				if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
					pBF = pReader;
			}
		}
/*
		if(!pBF && fn.Find(_T("://")) < 0)
		{
			CFile f;
			if(f.Open(fn, CFile::modeRead|CFile::shareDenyWrite))
			{
				ULONGLONG len = f.GetLength();
				BYTE buff[12];
				memset(buff, 0, sizeof(buff));
				f.Read(buff, sizeof(buff));
				if(*((DWORD*)&buff[0]) == 'FFIR' && *((DWORD*)&buff[8]) == ' IVA')
				{
					if(len < *((DWORD*)&buff[4])+8)
					{
						MessageBeep(-1);

						CComPtr<IFileSourceFilter> pReader;
						hr = pReader.CoCreateInstance(CLSID_AVIDoc);
						if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
							pBF = pReader;
					}
				}
			}
		}
*/
	}

	if(!pBF)
	{
		CheckStupidSharedFilesourceFilter();

		if(FAILED(hr = m_pGB->AddSourceFilter(fnw, fnw, &pBF)))
			return hr;

		if(SUCCEEDED(hr) && !pBF) // custom graphs don't need filters
			return hr;
	}

	ASSERT(pBF);

	return Render(pBF);
}

HRESULT CGraphBuilder::Render(IBaseFilter* pBF)
{
	if(!m_pGB || !m_pFM || !pBF)
		return E_FAIL;

	if(FAILED(SafeAddFilter(pBF, NULL)))
		return E_FAIL;

	CInterfaceList<IPin> pOutputs;
	BeginEnumPins(pBF, pEP, pPin)
	{
		PIN_DIRECTION dir;
		CComPtr<IPin> pTmp;
		CPinInfo pi;
		if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT	// not an output?
		|| SUCCEEDED(pPin->ConnectedTo(&pTmp))						// already connected?
		|| FAILED(pPin->QueryPinInfo(&pi)) || pi.achName[0] == '~')		// hidden?
			continue;

		pOutputs.AddTail(pPin);
	}
	EndEnumPins

	int nRendered = 0;

	if(pOutputs.GetCount() > 0)
	{
		POSITION pos = pOutputs.GetHeadPosition();
		while(pos)
		{
			if(SUCCEEDED(Render(pOutputs.GetNext(pos)))) nRendered++;
			if(pOutputs.GetCount() > 1) m_nCurrentStream++;
		}
	}
	else
	{
		m_nTotalStreams++;

		for(int i = 0; i < m_DeadEnds.GetCount(); i++)
		{
			if(m_DeadEnds[i]->nStream == m_nCurrentStream)
				m_DeadEnds.RemoveAt(i--);
		}
	}

	return 
		nRendered == pOutputs.GetCount() ? (nRendered > 0 ? S_OK : S_FALSE) :
		nRendered > 0 ? VFW_S_PARTIAL_RENDER :
		VFW_E_CANNOT_RENDER;
}

typedef struct {CGraphFilter* pFilter; bool fExactMatch;} SortFilter;

static int compare(const void* arg1, const void* arg2)
{
	SortFilter* sf1 = (SortFilter*)arg1;
	SortFilter* sf2 = (SortFilter*)arg2;
/*
	if(sf1->pFilter->GetMerit() < LMERIT(MERIT_PREFERRED) && sf2->pFilter->GetMerit() < LMERIT(MERIT_PREFERRED))
	{
		if(!sf1->fExactMatch && sf2->fExactMatch) return 1;
		else if(sf1->fExactMatch && !sf2->fExactMatch) return -1;
	}
*/
	if(sf1->pFilter->GetCLSID() == sf2->pFilter->GetCLSID())
	{
		// prefer externals
		CGraphFileFilter* f1 = dynamic_cast<CGraphFileFilter*>(sf1->pFilter);
		CGraphFileFilter* f2 = dynamic_cast<CGraphFileFilter*>(sf2->pFilter);
		if(!f1 && f2) return 1;
		if(f1 && !f2) return -1;
	}

	if(sf1->pFilter->GetMerit() < sf2->pFilter->GetMerit()) return 1;
	else if(sf1->pFilter->GetMerit() > sf2->pFilter->GetMerit()) return -1;

	if(!sf1->fExactMatch && sf2->fExactMatch) return 1;
	else if(sf1->fExactMatch && !sf2->fExactMatch) return -1;

	return 0;
}

HRESULT CGraphBuilder::Render(IPin* pPin)
{
	if(!m_pGB || !m_pFM || !pPin)
		return E_FAIL;

	PIN_DIRECTION dir;
	CComPtr<IPin> pPinTmp;
	if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT
	|| SUCCEEDED(pPin->ConnectedTo(&pPinTmp)))
		return E_UNEXPECTED;

	if(GetCLSID(GetFilterFromPin(pPin)) == CLSID_MPEG2Demultiplexer)
	{
		CComQIPtr<IMediaSeeking> pMS = pPin;
		if(!pMS) return E_FAIL;
		REFERENCE_TIME rtDur = 0;
		if(FAILED(pMS->GetDuration(&rtDur)) || rtDur <= 0)
			 return E_FAIL;
		rtDur = rtDur;
	}

	bool fDeadEnd = true;

	// 1. stream builder interface

	if(CComQIPtr<IStreamBuilder> pSB = pPin)
	{
		CInterfaceList<IBaseFilter> bfl;
		SaveFilters(bfl);

		if(SUCCEEDED(pSB->Render(pPin, m_pGB)))
			return S_OK;

		pSB->Backout(pPin, m_pGB);

		RestoreFilters(bfl);
	}

	// 2. filter cache

	if(CComQIPtr<IGraphConfig> pGC = m_pGB)
	{
		CComPtr<IEnumFilters> pEF;
		if(SUCCEEDED(pGC->EnumCacheFilter(&pEF)))
		{
			for(CComPtr<IBaseFilter> pBF; S_OK == pEF->Next(1, &pBF, NULL); pBF = NULL)
			{
				CInterfaceList<IBaseFilter> bfl;
				SaveFilters(bfl);

				pGC->RemoveFilterFromCache(pBF);

				if(SUCCEEDED(ConnectDirect(pPin, pBF)))
				{
					fDeadEnd = false;

					HRESULT hr;
					if(SUCCEEDED(hr = Render(pBF)))
						return hr;
				}

				pGC->AddFilterToCache(pBF);

				RestoreFilters(bfl);
			}
		}
	}

	// 3. disconnected inputs

	CInterfaceList<IBaseFilter> pFilters;
	SaveFilters(pFilters);

	POSITION pos;

	// FIXME: ffdshow likes to connect to the audio capture filter's fake input pins
	if(!dynamic_cast<CGraphBuilderCapture*>(this))
	{
	pos = pFilters.GetHeadPosition();
	while(pos)
	{
		CComPtr<IBaseFilter> pBF = pFilters.GetNext(pos);

		if(SUCCEEDED(ConnectDirect(pPin, pBF)))
		{
			fDeadEnd = false;

			HRESULT hr;
			if(SUCCEEDED(hr = Render(pBF)))
				return hr;
		}
	}
	}

	// 4. registry+internal+external

	// TODO: try media types one-by-one and pass pmt to ConnectDirect (this may not be better than the current!!!)

	CArray<GUID> guids;
    ExtractMediaTypes(pPin, guids);

	if(guids.GetCount() == 2 && guids[0] == MEDIATYPE_Stream && guids[1] == MEDIASUBTYPE_NULL)
	{
		if(CComQIPtr<IAsyncReader> pReader = pPin)
		{
			BYTE buff[4];
			if(S_OK == pReader->SyncRead(0, 4, buff) && *(DWORD*)buff == 'SggO')
				guids[1] = MEDIASUBTYPE_Ogg;
		}
	}
	else if(guids.GetCount() == 2 && guids[0] == MEDIATYPE_Video && guids[1] == FOURCCMap('MGXD'))
	{
		guids[1] = FOURCCMap('XVID');
	}

	CAutoPtrList<CGraphFilter> autogfs;
	CArray<CGraphFilter*> gfs;

	CComPtr<IEnumMoniker> pEM;
	if(guids.GetCount() > 0 
	&& SUCCEEDED(m_pFM->EnumMatchingFilters(
		&pEM, 0, FALSE, MERIT_DO_NOT_USE+1, 
		TRUE, guids.GetCount()/2, guids.GetData(), NULL, NULL, FALSE,
		FALSE, 0, NULL, NULL, NULL)))
	{
		for(CComPtr<IMoniker> pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
		{
			CAutoPtr<CGraphFilter> pGraphFilter(new CGraphRegFilter(pMoniker));

			if(pGraphFilter)
			{
				POSITION pos = m_pMoreFilters.GetHeadPosition();
				while(pos)
				{
					CGraphRegFilter* f = dynamic_cast<CGraphRegFilter*>((CGraphFilter*)m_pMoreFilters.GetNext(pos));
					if(!f) continue;

					if(f->GetMoniker() && S_OK == pMoniker->IsEqual(f->GetMoniker())
					|| f->GetCLSID() != GUID_NULL && f->GetCLSID() == pGraphFilter->GetCLSID())
					{
						pGraphFilter.Free();
						break;
					}
				}
			}

			if(pGraphFilter)
			{
				autogfs.AddTail(pGraphFilter);
			}
		}
	}

	pos = autogfs.GetHeadPosition();
	while(pos)
	{
		CGraphFilter* f = autogfs.GetNext(pos);

		bool fFound = false;

		POSITION pos2 = m_chain.GetHeadPosition();
		while(pos2)
		{
			if(f->GetCLSID() == m_chain.GetNext(pos2)->GetCLSID())
			{
				fFound = true;
				break;
			}
		}

		if(!fFound)
			gfs.Add(f);
	}

	pos = m_pMoreFilters.GetHeadPosition();
	while(pos)
	{
		CGraphFilter* f = m_pMoreFilters.GetNext(pos);
		if(f->GetMerit() >= LMERIT_DO_USE && !m_chain.Find(f) && f->IsCompatible(guids))
			gfs.Add(f);
	}

	CArray<SortFilter> sfs;
	sfs.SetSize(gfs.GetCount());
	for(int i = 0; i < gfs.GetCount(); i++)
	{
		sfs[i].pFilter = gfs[i];
		sfs[i].fExactMatch = gfs[i]->IsExactMatch(guids);
	}
	if(sfs.GetCount() > 1) qsort(sfs.GetData(), sfs.GetCount(), sizeof(SortFilter), compare);

	for(int i = 0; i < sfs.GetCount(); i++)
	{
		CGraphFilter* gf = sfs[i].pFilter;
		if(!gf) continue;

		CComPtr<IBaseFilter> pBF;
		CComPtr<IUnknown> pUnk;
		if(FAILED(gf->Create(&pBF, &pUnk)))
			continue;

		CInterfaceList<IBaseFilter> bfl;
		SaveFilters(bfl);

		if(FAILED(SafeAddFilter(pBF, gf->GetName())))
			continue;

		if(gf->GetCLSID() == CLSID_DMOWrapperFilter)
		{
			if(CComQIPtr<IPropertyBag> pPBag = pBF)
			{
				CComVariant var(true);
				pPBag->Write(CComBSTR(L"_HIRESOUTPUT"), &var);
			}
		}

		if(CComQIPtr<IMpeg2DecFilter> pMpeg2DecFilter = pBF)
		{
			pMpeg2DecFilter->EnablePlanarYUV(AfxGetAppSettings().mpegplanaryuv);
		}

		HRESULT hr = E_FAIL;

		if(guids.GetCount() == 2 && guids[0] == MEDIATYPE_Stream && guids[1] != MEDIATYPE_NULL)
		{
			CMediaType mt;
			mt.majortype = guids[0];
			mt.subtype = guids[1];
			mt.formattype = FORMAT_None;
			hr = ConnectDirect(pPin, pBF, &mt);
		}

		if(SUCCEEDED(hr) || SUCCEEDED(ConnectDirect(pPin, pBF, NULL)))
		{
			fDeadEnd = false;

			int nCurrentStream = m_nCurrentStream;

			m_chain.AddTail(gf);

			hr = Render(pBF);

			m_chain.RemoveTail();

			if(SUCCEEDED(hr))
			{
				if(pUnk) m_pUnks.AddTail(pUnk);

				// maybe the application should do this...
				if(CComQIPtr<IMixerPinConfig, &IID_IMixerPinConfig> pMPC = pUnk)
					pMPC->SetAspectRatioMode(AM_ARMODE_STRETCHED);
				if(CComQIPtr<IVMRAspectRatioControl> pARC = pBF)
					pARC->SetAspectRatioMode(VMR_ARMODE_NONE);
				if(CComQIPtr<IVMRAspectRatioControl9> pARC = pBF)
					pARC->SetAspectRatioMode(VMR_ARMODE_NONE);

				return hr;
			}

			m_nCurrentStream = nCurrentStream;
		}

		m_pGB->RemoveFilter(pBF);

		RestoreFilters(bfl);
	}

	// 5. record filter/pin/mts if this was the end of a chain

	if(fDeadEnd)
	{
		CAutoPtr<DeadEnd> de(new DeadEnd);
		de->nStream = m_nCurrentStream;
		de->filter = GetFilterName(GetFilterFromPin(pPin));
		de->pin = GetPinName(pPin);
		ExtractMediaTypes(pPin, de->mts);
		if(!(m_chain.GetCount() == 0 && de->mts.GetCount() == 1
		&& de->mts.GetHead().majortype == MEDIATYPE_Stream && de->mts.GetHead().subtype == MEDIASUBTYPE_NULL))
		{
			CString str;
			str.Format(_T("Stream %d"), m_nCurrentStream);
			de->path.AddTail(str);
			POSITION pos = m_chain.GetHeadPosition();
			while(pos) de->path.AddTail(CString(m_chain.GetNext(pos)->GetName()));
			m_DeadEnds.Add(de);
		}
	}

	return E_FAIL;
}

HRESULT CGraphBuilder::ConnectDirect(IPin* pPin, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt)
{
	if(!pPin || !pBF)
		return E_INVALIDARG;

	if(FAILED(SafeAddFilter(pBF, NULL)))
		return E_FAIL;

	BeginEnumPins(pBF, pEP, pPinTo)
	{
		PIN_DIRECTION dir;
		CComPtr<IPin> pTmp;
		if(FAILED(pPinTo->QueryDirection(&dir)) || dir != PINDIR_INPUT
		|| SUCCEEDED(pPinTo->ConnectedTo(&pTmp)))
			continue;

		if(SUCCEEDED(m_pGB->ConnectDirect(pPin, pPinTo, pmt)))
			return S_OK;
	}
	EndEnumPins

	return VFW_E_CANNOT_CONNECT;
}

HRESULT CGraphBuilder::FindInterface(REFIID iid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	POSITION pos = m_pUnks.GetHeadPosition();
	while(pos)
	{
		IUnknown* pUnk = m_pUnks.GetNext(pos);
		HRESULT hr;
		if(SUCCEEDED(hr = pUnk->QueryInterface(iid, ppv)))
			return hr;
	}

	return E_NOINTERFACE;
}

//
// CGraphBuilderFile
//

CGraphBuilderFile::CGraphBuilderFile(IGraphBuilder* pGB, HWND hWnd)
	: CGraphBuilder(pGB, hWnd)
{
	CList<GUID> guids;

	if(AfxGetAppSettings().fEnableAudioSwitcher)
	{
		guids.AddTail(MEDIATYPE_Audio);
		guids.AddTail(MEDIASUBTYPE_NULL);
		AddFilter(new CGraphCustomFilter(__uuidof(CAudioSwitcherFilter), guids, L"Audio Switcher", m_ARMerit + 0x100));
		guids.RemoveAll();

		CLSID CLSID_MorganStreamSwitcher = GUIDFromCString(_T("{D3CD7858-971A-4838-ACEC-40CA5D529DC8}"));
		AddFilter(new CGraphRegFilter(CLSID_MorganStreamSwitcher, LMERIT_DO_NOT_USE));
	}
}

//
// CGraphBuilderDVD
//

CGraphBuilderDVD::CGraphBuilderDVD(IGraphBuilder* pGB, HWND hWnd)
	: CGraphBuilderFile(pGB, hWnd)
{
	CList<GUID> guids;
/*
	// there wasn't much use of this and the users were just 
	// confused by "decss" being in the report after a failed 
	// dvd nav pin rendering

	guids.AddTail(MEDIATYPE_DVD_ENCRYPTED_PACK);
	guids.AddTail(MEDIASUBTYPE_NULL);
	AddFilter(new CGraphCustomFilter(__uuidof(CDeCSSFilter), guids, L"DeCSS", LMERIT_UNLIKELY));
	guids.RemoveAll();
*/
	DWORD ver = ::GetVersion();
	bool m_fXp = (int)ver >= 0 && (((ver<<8)&0xff00)|((ver>>8)&0xff)) >= 0x0501;

	AppSettings& s = AfxGetAppSettings();
	if(!m_fXp && s.iDSVideoRendererType != VIDRNDT_DS_OVERLAYMIXER 
	|| s.iDSVideoRendererType == VIDRNDT_DS_OLDRENDERER)
        AddFilter(new CGraphRendererFilter(CLSID_OverlayMixer, m_hWnd, L"Overlay Mixer", m_VRMerit-1));

	// this filter just simply sucks for dvd playback (atm)
	CLSID CLSID_ElecardMpeg2 = GUIDFromCString(_T("{F50B3F13-19C4-11CF-AA9A-02608C9BABA2}"));
	AddFilter(new CGraphRegFilter(CLSID_ElecardMpeg2, LMERIT_DO_NOT_USE));
}

#include "..\..\decss\VobFile.h"

class CResetDVD : public CDVDSession
{
public:
	CResetDVD(LPCTSTR path)
	{
		if(Open(path))
		{
			if(BeginSession())
			{
				Authenticate();
				// GetDiscKey();
				EndSession();
			}

			Close();
		}
	}
};

HRESULT CGraphBuilderDVD::Render(CString fn, CString& path)
{
	if(!m_pGB) return E_INVALIDARG;

	HRESULT hr;

	CComPtr<IBaseFilter> pBF;
	if(FAILED(hr = pBF.CoCreateInstance(CLSID_DVDNavigator)))
		return E_FAIL;

	if(FAILED(hr = m_pGB->AddFilter(pBF, L"DVD Navigator")))
		return VFW_E_CANNOT_LOAD_SOURCE_FILTER;

	CComQIPtr<IDvdControl2> pDVDC;
	CComQIPtr<IDvdInfo2> pDVDI;

	if(!((pDVDC = pBF) && (pDVDI = pBF)))
		return E_NOINTERFACE;

	WCHAR buff[MAX_PATH];
	ULONG len;
	if((!fn.IsEmpty()
		&& FAILED(hr = pDVDC->SetDVDDirectory(CStringW(fn)))
		&& FAILED(hr = pDVDC->SetDVDDirectory(CStringW(fn + _T("VIDEO_TS"))))
		&& FAILED(hr = pDVDC->SetDVDDirectory(CStringW(fn + _T("\\VIDEO_TS")))))
	|| FAILED(hr = pDVDI->GetDVDDirectory(buff, MAX_PATH, &len)) || len == 0)
		return VFW_E_CANNOT_LOAD_SOURCE_FILTER;

	path = buff;

	pDVDC->SetOption(DVD_ResetOnStop, FALSE);
	pDVDC->SetOption(DVD_HMSF_TimeCodeEvents, TRUE);

	m_pUnks.AddTail(pDVDC);
	m_pUnks.AddTail(pDVDI);

	CResetDVD tmp(path);

	return __super::Render(pBF);
}

//
// CGraphBuilderCapture
//

CGraphBuilderCapture::CGraphBuilderCapture(IGraphBuilder* pGB, HWND hWnd)
	: CGraphBuilderFile(pGB, hWnd)
{
	CLSID CLSID_MorganStreamSwitcher = GUIDFromCString(_T("{D3CD7858-971A-4838-ACEC-40CA5D529DC8}"));
	AddFilter(new CGraphRegFilter(CLSID_MorganStreamSwitcher, LMERIT_DO_NOT_USE));
}

//
// CGraphFilter
//

CGraphFilter::CGraphFilter(CStringW name, ULONGLONG merit)
	: m_name(name), m_clsid(GUID_NULL)
{
	m_merit.val = merit;
}

bool CGraphFilter::IsExactMatch(CArray<GUID>& guids)
{
	POSITION pos = m_guids.GetHeadPosition();
	while(pos)
	{
		GUID& major = m_guids.GetNext(pos);
		if(!pos) {ASSERT(0); break;}
		GUID& sub = m_guids.GetNext(pos);

		for(int i = 0; i < (guids.GetCount()&~1); i += 2)
		{
			if(major == guids[i] && major != GUID_NULL
			&& sub == guids[i+1] && sub != GUID_NULL)
            	return(true);
		}
	}

	return(false);
}

bool CGraphFilter::IsCompatible(CArray<GUID>& guids)
{
	POSITION pos = m_guids.GetHeadPosition();
	while(pos)
	{
		GUID& major = m_guids.GetNext(pos);
		if(!pos) {ASSERT(0); break;}
		GUID& sub = m_guids.GetNext(pos);

		for(int i = 0; i < (guids.GetCount()&~1); i += 2)
		{
			if((major == GUID_NULL || guids[i] == GUID_NULL || major == guids[i])
			&& (sub == GUID_NULL || guids[i+1] == GUID_NULL || sub == guids[i+1]))
            	return(true);
		}
	}

	return(false);
}

//
// CGraphRegFilter
//

CGraphRegFilter::CGraphRegFilter(IMoniker* pMoniker, ULONGLONG merit) 
	: CGraphFilter(L"", merit), m_pMoniker(pMoniker)
{
	if(!m_pMoniker) return;

	LPOLESTR str = NULL;
	if(FAILED(m_pMoniker->GetDisplayName(0, 0, &str))) return;
	m_dispname = m_name = str;
	CoTaskMemFree(str), str = NULL;

	CComPtr<IPropertyBag> pPB;
	if(SUCCEEDED(m_pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
	{
		CComVariant var;
		if(SUCCEEDED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
		{
			m_name = var.bstrVal;
			var.Clear();
		}

		if(SUCCEEDED(pPB->Read(CComBSTR(_T("CLSID")), &var, NULL)))
		{
			CLSIDFromString(var.bstrVal, &m_clsid);
			var.Clear();
		}

		if(SUCCEEDED(pPB->Read(CComBSTR(_T("FilterData")), &var, NULL)))
		{			
			BSTR* pstr;
			if(SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pstr)))
			{
				ExtractFilterData((BYTE*)pstr, var.parray->cbElements*(var.parray->rgsabound[0].cElements));
				SafeArrayUnaccessData(var.parray);
			}
			var.Clear();
		}
	}

	if(merit != LMERIT_DO_USE) m_merit.val = merit;
}

CGraphRegFilter::CGraphRegFilter(CStringW m_dispname, ULONGLONG merit) 
	: CGraphFilter(L"", merit), m_dispname(m_dispname)
{
	if(m_dispname.IsEmpty()) return;

	CComPtr<IBindCtx> pBC;
	CreateBindCtx(0, &pBC);

	ULONG chEaten;
	if(S_OK != MkParseDisplayName(pBC, CComBSTR(m_dispname), &chEaten, &m_pMoniker))
		return;

	CComPtr<IPropertyBag> pPB;
	if(SUCCEEDED(m_pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
	{
		CComVariant var;
		if(SUCCEEDED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
		{
			m_name = var.bstrVal;
			var.Clear();
		}

		if(SUCCEEDED(pPB->Read(CComBSTR(_T("CLSID")), &var, NULL)))
		{
			CLSIDFromString(var.bstrVal, &m_clsid);
			var.Clear();
		}

		if(SUCCEEDED(pPB->Read(CComBSTR(_T("FilterData")), &var, NULL)))
		{			
			BSTR* pstr;
			if(SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pstr)))
			{
				ExtractFilterData((BYTE*)pstr, var.parray->cbElements*(var.parray->rgsabound[0].cElements));
				SafeArrayUnaccessData(var.parray);
			}
			var.Clear();
		}
	}

	if(merit != LMERIT_DO_USE) m_merit.val = merit;
}

CGraphRegFilter::CGraphRegFilter(const CLSID& clsid, ULONGLONG merit) 
	: CGraphFilter(L"", merit)
{
	if((m_clsid = clsid) == GUID_NULL) return;

	CString guid = CStringFromGUID(m_clsid);
	CString str = CString(_T("CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance\\")) + guid;
	CString str2 = CString(_T("CLSID\\")) + guid;

	CRegKey key;

	if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, str2, KEY_READ))
	{
		ULONG nChars = 0;
		if(ERROR_SUCCESS == key.QueryStringValue(NULL, NULL, &nChars))
		{
			CString name;
			if(ERROR_SUCCESS == key.QueryStringValue(NULL, name.GetBuffer(nChars), &nChars))
			{
				name.ReleaseBuffer(nChars);
				m_name = name;
			}
		}

		key.Close();
	}

	if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, str, KEY_READ))
	{
		ULONG nChars = 0;
		if(ERROR_SUCCESS == key.QueryStringValue(_T("FriendlyName"), NULL, &nChars))
		{
			CString name;
			if(ERROR_SUCCESS == key.QueryStringValue(_T("FriendlyName"), name.GetBuffer(nChars), &nChars))
			{
				name.ReleaseBuffer(nChars);
				m_name = name;
			}
		}

		ULONG nBytes = 0;
		if(ERROR_SUCCESS == key.QueryBinaryValue(_T("FilterData"), NULL, &nBytes))
		{
			CAutoVectorPtr<BYTE> buff;
			if(buff.Allocate(nBytes) && ERROR_SUCCESS == key.QueryBinaryValue(_T("FilterData"), buff, &nBytes))
			{
				ExtractFilterData(buff, nBytes);
			}
		}

		key.Close();
	}

	if(merit != LMERIT_DO_USE) m_merit.val = merit;
}

[uuid("97f7c4d4-547b-4a5f-8332-536430ad2e4d")]
interface IAMFilterData : public IUnknown
{
	STDMETHOD (ParseFilterData) (BYTE* rgbFilterData, ULONG cb, BYTE** prgbRegFilter2) PURE;
	STDMETHOD (CreateFilterData) (REGFILTER2* prf2, BYTE** prgbFilterData, ULONG* pcb) PURE;
};

void CGraphRegFilter::ExtractFilterData(BYTE* p, UINT len)
{
	CComPtr<IAMFilterData> pFD;
	BYTE* ptr = NULL;
	if(SUCCEEDED(pFD.CoCreateInstance(CLSID_FilterMapper2))
	&& SUCCEEDED(pFD->ParseFilterData(p, len, (BYTE**)&ptr)))
	{
		REGFILTER2* prf = (REGFILTER2*)*(DWORD*)ptr; // this is f*cked up

		m_merit.mid = prf->dwMerit;

		if(prf->dwVersion == 1)
		{
			for(UINT i = 0; i < prf->cPins; i++)
			{
				if(prf->rgPins[i].bOutput)
					continue;

				for(UINT j = 0; j < prf->rgPins[i].nMediaTypes; j++)
				{
					if(!prf->rgPins[i].lpMediaType[j].clsMajorType || !prf->rgPins[i].lpMediaType[j].clsMinorType)
						break;
					m_guids.AddTail(*(GUID*)prf->rgPins[i].lpMediaType[j].clsMajorType);
					m_guids.AddTail(*(GUID*)prf->rgPins[i].lpMediaType[j].clsMinorType);
				}
			}
		}
		else if(prf->dwVersion == 2)
		{
			for(UINT i = 0; i < prf->cPins2; i++)
			{
				if(prf->rgPins2[i].dwFlags&REG_PINFLAG_B_OUTPUT)
					continue;

				for(UINT j = 0; j < prf->rgPins2[i].nMediaTypes; j++)
				{
					if(!prf->rgPins2[i].lpMediaType[j].clsMajorType || !prf->rgPins2[i].lpMediaType[j].clsMinorType)
						break;
					m_guids.AddTail(*(GUID*)prf->rgPins2[i].lpMediaType[j].clsMajorType);
					m_guids.AddTail(*(GUID*)prf->rgPins2[i].lpMediaType[j].clsMinorType);
				}
			}
		}

		CoTaskMemFree(prf);
	}
	else
	{
		BYTE* base = p;

#define ChkLen(size) if(p - base + size > (int)len) return;

ChkLen(4)
		if(*(DWORD*)p != 0x00000002) return; // only version 2 supported, no samples found for 1
		p += 4;

ChkLen(4)
		m_merit.mid = *(DWORD*)p; p += 4;

		m_guids.RemoveAll();

ChkLen(8)
		DWORD nPins = *(DWORD*)p; p += 8;
		while(nPins-- > 0)
		{
ChkLen(1)
			BYTE n = *p-0x30; p++;
ChkLen(2)
			WORD pi = *(WORD*)p; p += 2;
			ASSERT(pi == 'ip');
ChkLen(1)
			BYTE x33 = *p; p++;
			ASSERT(x33 == 0x33);
ChkLen(8)
			bool fOutput = !!(*p&REG_PINFLAG_B_OUTPUT);
			p += 8;
ChkLen(12)
			DWORD nTypes = *(DWORD*)p; p += 12;
			while(nTypes-- > 0)
			{
ChkLen(1)
				BYTE n = *p-0x30; p++;
ChkLen(2)
				WORD ty = *(WORD*)p; p += 2;
				ASSERT(ty == 'yt');
ChkLen(5)
				BYTE x33 = *p; p++;
				ASSERT(x33 == 0x33);
				p += 4;

ChkLen(8)
				if(*(DWORD*)p < (p-base+8) || *(DWORD*)p >= len 
				|| *(DWORD*)(p+4) < (p-base+8) || *(DWORD*)(p+4) >= len)
				{
					p += 8;
					continue;
				}

				GUID guid;
				memcpy(&guid, &base[*(DWORD*)p], sizeof(GUID)); p += 4;
				if(!fOutput) m_guids.AddTail(guid); 
				memcpy(&guid, &base[*(DWORD*)p], sizeof(GUID)); p += 4;
				if(!fOutput) m_guids.AddTail(guid); 
			}
		}

#undef ChkLen

	}
/*
	if(CLSID_MMSPLITTER == m_clsid && m_merit.val == LMERIT(MERIT_NORMAL-1))
	{
		m_merit.val = LMERIT(MERIT_NORMAL)+1; // take this mpeg2 demux...
	}
*/
}

HRESULT CGraphRegFilter::Create(IBaseFilter** ppBF, IUnknown** ppUnk)
{
	CheckPointer(ppBF, E_POINTER);

	if(ppUnk) *ppUnk = NULL;

	HRESULT hr = E_FAIL;

	
	if(m_pMoniker)
	{
		if(SUCCEEDED(hr = m_pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)ppBF)))
			m_clsid = ::GetCLSID(*ppBF);
	}
	else if(m_clsid != GUID_NULL)
	{
		CComPtr<IBaseFilter> pBF;
		if(FAILED(pBF.CoCreateInstance(m_clsid))) return E_FAIL;
		*ppBF = pBF.Detach();
		hr = S_OK;
	}

	return hr;
};

//
// CGraphCustomFilter
//

CGraphCustomFilter::CGraphCustomFilter(const CLSID& clsid, CList<GUID>& guids, CStringW name, ULONGLONG merit) 
	: CGraphFilter(name, merit)
{
	m_clsid = clsid;
	ASSERT(guids.GetCount() > 0);
	m_guids.AddTail(&guids);
}

HRESULT CGraphCustomFilter::Create(IBaseFilter** ppBF, IUnknown** ppUnk)
{
	CheckPointer(ppBF, E_POINTER);

	if(ppUnk) *ppUnk = NULL;

	HRESULT hr = S_OK;

	*ppBF = 
		m_clsid == __uuidof(CAVI2AC3Filter) ? (IBaseFilter*)new CAVI2AC3Filter(NULL, &hr) : 
		m_clsid == __uuidof(CDeCSSFilter) ? (IBaseFilter*)new CDeCSSFilter(NULL, &hr) : 
		m_clsid == __uuidof(CAudioSwitcherFilter) ? (IBaseFilter*)new CAudioSwitcherFilter(NULL, &hr) :
		m_clsid == __uuidof(CMatroskaSplitterFilter) ? (IBaseFilter*)new CMatroskaSplitterFilter(NULL, &hr) :
		m_clsid == __uuidof(CRealMediaSplitterFilter) ? (IBaseFilter*)new CRealMediaSplitterFilter(NULL, &hr) :
		m_clsid == __uuidof(CRealVideoDecoder) ? (IBaseFilter*)new CRealVideoDecoder(NULL, &hr) :
		m_clsid == __uuidof(CRealAudioDecoder) ? (IBaseFilter*)new CRealAudioDecoder(NULL, &hr) :
		m_clsid == __uuidof(CRoQVideoDecoder) ? (IBaseFilter*)new CRoQVideoDecoder(NULL, &hr) :
		m_clsid == __uuidof(CRoQAudioDecoder) ? (IBaseFilter*)new CRoQAudioDecoder(NULL, &hr) :
		m_clsid == __uuidof(CAviSplitterFilter) ? (IBaseFilter*)new CAviSplitterFilter(NULL, &hr) :
		m_clsid == __uuidof(CRadGtSplitterFilter) ? (IBaseFilter*)new CRadGtSplitterFilter(NULL, &hr) :
		m_clsid == __uuidof(CRoQSplitterFilter) ? (IBaseFilter*)new CRoQSplitterFilter(NULL, &hr) :
		m_clsid == __uuidof(COggSplitterFilter) ? (IBaseFilter*)new COggSplitterFilter(NULL, &hr) :
		m_clsid == __uuidof(CMpeg2DecFilter) ? (IBaseFilter*)new CMpeg2DecFilter(NULL, &hr) :
		m_clsid == __uuidof(CMpaDecFilter) ? (IBaseFilter*)new CMpaDecFilter(NULL, &hr) :
		m_clsid == __uuidof(CNullVideoRenderer) ? (IBaseFilter*)new CNullVideoRenderer() :
		m_clsid == __uuidof(CNullAudioRenderer) ? (IBaseFilter*)new CNullAudioRenderer() :
		m_clsid == __uuidof(CNullUVideoRenderer) ? (IBaseFilter*)new CNullUVideoRenderer() :
		m_clsid == __uuidof(CNullUAudioRenderer) ? (IBaseFilter*)new CNullUAudioRenderer() :
		m_clsid == __uuidof(CNullTextRenderer) ? (IBaseFilter*)new CNullTextRenderer(NULL, &hr) :
		NULL;

	if(!*ppBF) hr = E_FAIL;
	else (*ppBF)->AddRef();

	if(!*ppBF)
	{
		CComPtr<IBaseFilter> pBF;
		if(SUCCEEDED(hr = pBF.CoCreateInstance(m_clsid)))
			*ppBF = pBF.Detach();
	}

	if(SUCCEEDED(hr) && ppUnk)
	{
		if(m_clsid == __uuidof(CAudioSwitcherFilter))
		{
			*ppUnk = (IUnknown*)CComQIPtr<IAudioSwitcherFilter>(*ppBF).Detach();
			CComQIPtr<IAudioSwitcherFilter> pASF = *ppUnk;

			if(pASF)
			{
				AppSettings& s = AfxGetAppSettings();
				pASF->SetSpeakerConfig(s.fCustomChannelMapping, s.pSpeakerToChannelMap);
				pASF->EnableDownSamplingTo441(s.fDownSampleTo441);
				pASF->SetAudioTimeShift(s.fAudioTimeShift ? 10000i64*s.tAudioTimeShift : 0);
			}
		}
	}

	return hr;
}

//
// CGraphFileFilter
//

CGraphFileFilter::CGraphFileFilter(const CLSID& clsid, CList<GUID>& guids, CString path, CStringW name, ULONGLONG merit)
	: CGraphCustomFilter(clsid, guids, name, merit), m_path(path), m_hInst(NULL)
{
}

HRESULT CGraphFileFilter::Create(IBaseFilter** ppBF, IUnknown** ppUnk)
{
	CheckPointer(ppBF, E_POINTER);

	if(ppUnk) *ppUnk = NULL;

	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = __super::Create(ppBF, ppUnk)))
		return hr;

	return LoadExternalFilter(m_path, m_clsid, ppBF);
}

//
// CGraphRendererFilter
//

CGraphRendererFilter::CGraphRendererFilter(const CLSID& clsid, HWND hWnd, CStringW name, ULONGLONG merit) 
	: CGraphFilter(name, merit), m_clsid(clsid), m_hWnd(hWnd)
{
	m_guids.AddTail(MEDIATYPE_Video);
	m_guids.AddTail(MEDIASUBTYPE_NULL);
}

HRESULT CGraphRendererFilter::Create(IBaseFilter** ppBF, IUnknown** ppUnk)
{
	CheckPointer(ppBF, E_POINTER);

	HRESULT hr = S_OK;

	if(m_clsid == CLSID_OverlayMixer)
	{
		CComPtr<IBaseFilter> pBF;
		if(SUCCEEDED(pBF.CoCreateInstance(CLSID_OverlayMixer)))
		{
			BeginEnumPins(pBF, pEP, pPin)
			{
				if(CComQIPtr<IMixerPinConfig, &IID_IMixerPinConfig> pMPC = pPin)
				{
					if(ppUnk) *ppUnk = pMPC.Detach();
					break;
				}
			}
			EndEnumPins

			*ppBF = pBF.Detach();
		}
	}
	else if(m_clsid == CLSID_VideoMixingRenderer)
	{
		CComPtr<IBaseFilter> pBF;
		if(SUCCEEDED(pBF.CoCreateInstance(CLSID_VideoMixingRenderer)))
			*ppBF = pBF.Detach();
	}
	else if(m_clsid == CLSID_VideoMixingRenderer9)
	{
		CComPtr<IBaseFilter> pBF;
		if(SUCCEEDED(pBF.CoCreateInstance(CLSID_VideoMixingRenderer9)))
			*ppBF = pBF.Detach();
	}
	else if(m_clsid == CLSID_VMR7AllocatorPresenter)
	{
		CComPtr<ISubPicAllocatorPresenter> pCAP;
		CComPtr<IUnknown> pRenderer;
		if(SUCCEEDED(hr = CreateAP7(CLSID_VMR7AllocatorPresenter, m_hWnd, &pCAP))
		&& SUCCEEDED(hr = pCAP->CreateRenderer(&pRenderer)))
		{
			*ppBF = CComQIPtr<IBaseFilter>(pRenderer).Detach();
			if(ppUnk) *ppUnk = (IUnknown*)pCAP.Detach();
		}
	}
	else if(m_clsid == CLSID_VMR9AllocatorPresenter)
	{
		CComPtr<ISubPicAllocatorPresenter> pCAP;
		CComPtr<IUnknown> pRenderer;
		if(SUCCEEDED(hr = CreateAP9(CLSID_VMR9AllocatorPresenter, m_hWnd, &pCAP))
		&& SUCCEEDED(hr = pCAP->CreateRenderer(&pRenderer)))
		{
			*ppBF = CComQIPtr<IBaseFilter>(pRenderer).Detach();
			if(ppUnk) *ppUnk = (IUnknown*)pCAP.Detach();
		}
	}

	if(!*ppBF) hr = E_FAIL;

	return hr;
}

