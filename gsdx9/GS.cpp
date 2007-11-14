/* 
 *	Copyright (C) 2003-2005 Gabest
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
#include "GSdx9.h"
#include "GS.h"
#include "GSRendererHW.h"
#include "GSRendererSoft.h"
#include "GSRendererNull.h"
#include "GSSettingsDlg.h"
#include "GSTransferThread.h"

#define PS2E_LT_GS 0x01
#define PS2E_GS_VERSION 0x0006
#define PS2E_DLL_VERSION 10
#define PS2E_X86 0x01   // 32 bit
#define PS2E_X86_64 0x02   // 64 bit

EXPORT_C_(UINT32) PS2EgetLibType()
{
	return PS2E_LT_GS;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	CString str = _T("GSdx9");

#if _M_AMD64
	str += _T(" 64-bit");
#endif

	CAtlList<CString> sl;

#ifdef __INTEL_COMPILER
	CString s;
	s.Format(_T("Intel C++ %d.%02d"), __INTEL_COMPILER/100, __INTEL_COMPILER%100);
	sl.AddTail(s);
#elif _MSC_VER
	CString s;
	s.Format(_T("MSVC %d.%02d"), _MSC_VER/100, _MSC_VER%100);
	sl.AddTail(s);
#endif

#if _M_IX86_FP >= 2
	sl.AddTail(_T("SSE2"));
#elif _M_IX86_FP >= 1
	sl.AddTail(_T("SSE"));
#endif

#ifdef _OPENMP
	sl.AddTail(_T("OpenMP"));
#endif

	POSITION pos = sl.GetHeadPosition();

	while(pos)
	{
		if(pos == sl.GetHeadPosition()) str += _T(" (");
		str += sl.GetNext(pos);
		str += pos ? _T(", ") : _T(")");
	}

	static char buff[256];
	strncpy(buff, CStringA(str), min(countof(buff)-1, str.GetLength()));
	return buff;
}

EXPORT_C_(UINT32) PS2EgetLibVersion2(UINT32 type)
{
	return (PS2E_GS_VERSION<<16) | (0<<8) | PS2E_DLL_VERSION;
}

EXPORT_C_(UINT32) PS2EgetCpuPlatform()
{
#if _M_AMD64
	return PS2E_X86_64;
#else
	return PS2E_X86;
#endif
}

//////////////////

static HRESULT s_hrCoInit = E_FAIL;
static GSState* s_gs;
// static GSTransferThread* s_gst;
static void (*s_irq)() = NULL;

BYTE* g_pBasePS2Mem = NULL;

EXPORT_C GSsetBaseMem(BYTE* pBasePS2Mem)
{
	g_pBasePS2Mem = pBasePS2Mem - 0x12000000;
}

EXPORT_C_(INT32) GSinit()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	return 0;
}

EXPORT_C GSshutdown()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
}

EXPORT_C GSclose()
{
	delete s_gs; s_gs = NULL;
	// delete s_gst; s_gst = NULL;

	if(SUCCEEDED(s_hrCoInit))
	{
		::CoUninitialize();

		s_hrCoInit = E_FAIL;
	}
}

EXPORT_C_(INT32) GSopen(void* dsp, char* title, int mt)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	GSclose();

	switch(AfxGetApp()->GetProfileInt(_T("Settings"), _T("Renderer"), RENDERER_D3D_HW))
	{
	case RENDERER_D3D_HW: s_gs = new GSRendererHW(); break;
	case RENDERER_D3D_SW_FP: s_gs = new GSRendererSoftFP(); break;
	// case RENDERER_D3D_SW_FX: s_gs = new GSRendererSoftFX(); break;
	case RENDERER_D3D_NULL: s_gs = new GSRendererNull(); break;
	default: return -1;
	}

	s_hrCoInit = ::CoInitialize(0);

	if(!s_gs->Create(CString(title)))
	{
		GSclose();
		return -1;
	}

	// s_gst = new GSTransferThread(s_gs);

	s_gs->SetIrq(s_irq);
	s_gs->SetMT(!!mt);
	s_gs->Show();

	*(HWND*)dsp = *s_gs;

	return 0;
}

EXPORT_C GSreset()
{
	// s_gst->Wait();
	// s_gst->Reset();

	s_gs->Reset();
}

EXPORT_C GSwriteCSR(UINT32 csr)
{
	s_gs->WriteCSR(csr);
}

EXPORT_C GSreadFIFO(BYTE* mem)
{
	// s_gst->Wait();

	s_gs->ReadFIFO(mem);
}

EXPORT_C GSgifTransfer1(BYTE* mem, UINT32 addr)
{
	// s_gst->Transfer(mem + addr, -1, 0);

	s_gs->Transfer(mem + addr, -1, 0);
}

EXPORT_C GSgifTransfer2(BYTE* mem, UINT32 size)
{
	// s_gst->Transfer(mem, size, 1);

	s_gs->Transfer(mem, size, 1);
}

EXPORT_C GSgifTransfer3(BYTE* mem, UINT32 size)
{
	// s_gst->Transfer(mem, size, 2);

	s_gs->Transfer(mem, size, 2);
}

EXPORT_C GSvsync(int field)
{
	// s_gst->Wait();

	MSG msg;

	memset(&msg, 0, sizeof(msg));

	while(msg.message != WM_QUIT)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(s_gs->OnMsg(msg))
			{
				continue;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			s_gs->VSync(field);
			break;
		}
	}
}

EXPORT_C_(UINT32) GSmakeSnapshot(char* path)
{
	return s_gs->MakeSnapshot(path);
}

EXPORT_C GSkeyEvent(keyEvent* ev)
{
}

EXPORT_C_(INT32) GSfreeze(int mode, freezeData* data)
{
	// s_gst->Wait();

	if(mode == FREEZE_SAVE)
	{
		return s_gs->Freeze(data, false);
	}
	else if(mode == FREEZE_SIZE)
	{
		return s_gs->Freeze(data, true);
	}
	else if(mode == FREEZE_LOAD)
	{
		return s_gs->Defrost(data);
	}

	return 0;
}

EXPORT_C GSconfigure()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if(IDOK == CGSSettingsDlg().DoModal())
	{
		GSshutdown();
		GSinit();
	}
}

EXPORT_C_(INT32) GStest()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	int ret = 0;

	D3DCAPS9 caps;
	memset(&caps, 0, sizeof(caps));
	caps.PixelShaderVersion = D3DPS_VERSION(0, 0);

	if(CComPtr<IDirect3D9> pD3D = Direct3DCreate9(D3D_SDK_VERSION))
	{
		pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

		LPCTSTR yep = _T("^_^"), nope = _T(":'(");

		CString str, tmp;

		if(caps.VertexShaderVersion < D3DVS_VERSION(2, 0))
			ret = -1;

		tmp.Format(_T("%s Vertex Shader version %d.%d\n"), 
			caps.VertexShaderVersion >= D3DVS_VERSION(2, 0) ? yep : nope,
			D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion),
			D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion));
		str += tmp;

		if(caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
			ret = -1;

		tmp.Format(_T("%s Pixel Shader version %d.%d\n"), 
			caps.PixelShaderVersion >= D3DPS_VERSION(2, 0) ? yep : nope,
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion),
			D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));
		str += tmp;

		if(!(caps.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND))
			ret = -1;

		tmp.Format(_T("%s Separate Alpha Blend\n"), 
			!!(caps.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND) ? yep : nope);
		str += tmp;

		if(!(caps.SrcBlendCaps & D3DPBLENDCAPS_BLENDFACTOR)
		|| !(caps.DestBlendCaps & D3DPBLENDCAPS_BLENDFACTOR))
			ret = -1;

		tmp.Format(_T("%s Source Blend Factor\n"), 
			!!(caps.SrcBlendCaps & D3DPBLENDCAPS_BLENDFACTOR) ? yep : nope);
		str += tmp;

		tmp.Format(_T("%s Destination Blend Factor\n"), 
			!!(caps.DestBlendCaps & D3DPBLENDCAPS_BLENDFACTOR) ? yep : nope);
		str += tmp;

		AfxMessageBox(str);
	}
	else
	{
		ret = -1;
	}

	return ret;
}

EXPORT_C GSabout()
{
}

EXPORT_C GSirqCallback(void (*irq)())
{
	s_irq = irq;
}

EXPORT_C GSsetGameCRC(int crc, int options)
{
	s_gs->SetGameCRC(crc, options);
}

EXPORT_C GSgetLastTag(UINT32* tag) 
{
	s_gs->GetLastTag(tag);
}

/////////////////
/*
EXPORT_C GSReplay(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	if(!GSinit())
	{
		HWND hWnd = NULL;
		if(!GSopen((void*)&hWnd, REPLAY_TITLE))
		{
			if(FILE* sfp = _tfopen(lpszCmdLine, _T("rb")))
			{
				BYTE* buff = (BYTE*)_aligned_malloc(4*1024*1024, 16);

				while(!feof(sfp))
				{
					switch(fgetc(sfp))
					{
					case ST_WRITE:
						{
						GS_REG mem;
						UINT64 value, mask;
						fread(&mem, 4, 1, sfp);
						fread(&value, 8, 1, sfp);
						fread(&mask, 8, 1, sfp);
						switch(mask)
						{
						case 0xff: GSwrite8(mem, (UINT8)value); break;
						case 0xffff: GSwrite16(mem, (UINT16)value); break;
						case 0xffffffff: GSwrite32(mem, (UINT32)value); break;
						case 0xffffffffffffffff: GSwrite64(mem, value); break;
						}
						break;
						}
					case ST_TRANSFER:
						{
						UINT32 size = 0;
						fread(&size, 4, 1, sfp);
						UINT32 len = 0;
						fread(&len, 4, 1, sfp);
						if(len > 4*1024*1024) {ASSERT(0); break;}
						fread(buff, len, 1, sfp);
						GSgifTransfer3(buff, size);
						break;
						}
					case ST_VSYNC:
						GSvsync();
						break;
					}
				}

				_aligned_free(buff);
			}

			GSclose();
		}
		
		GSshutdown();
	}
}
*/