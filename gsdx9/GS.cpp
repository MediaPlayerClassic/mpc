/* 
 *	Copyright (C) 2003-2004 Gabest
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
#include "GSWnd.h"
#include "GSSettingsDlg.h"

#define PS2E_LT_GS 0x01
#define PS2E_GS_VERSION 0x0004
#define PS2E_DLL_VERSION 0x04

EXPORT_C_(UINT32) PS2EgetLibType()
{
	return PS2E_LT_GS;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return "GSdx9"
#if _M_IX86_FP >= 2
	" (SSE2)";
#elif _M_IX86_FP >= 1
	" (SSE)";
#endif
	;
}

static BYTE and_eax_ffff0000h_cmp_eax_30000h[] = {0x25, 0x00, 0x00, 0xFF, 0xFF, 0x3D, 0x00, 0x00, 0x03, 0x00};
static BYTE mov_ecx_eax_shr_ecx_10h_cmp_ecx_3[] = {0x8B, 0xC8, 0xC1, 0xE9, 0x10, 0x83, 0xF9, 0x03};
static void* test_gs_lib_ver = NULL;

EXPORT_C_(__declspec(naked) UINT32) PS2EgetLibVersion2(UINT32 type)
{
//	return (PS2E_GS_VERSION<<16)|(0x00<<8)|PS2E_DLL_VERSION;

	__asm
	{
		mov eax, [esp]
		mov test_gs_lib_ver, eax
		push ebx
		push ecx 
		push edx 
		push esi 
		push edi
	}

	if(!memcmp(test_gs_lib_ver, and_eax_ffff0000h_cmp_eax_30000h, sizeof(and_eax_ffff0000h_cmp_eax_30000h))
	|| !memcmp(test_gs_lib_ver, mov_ecx_eax_shr_ecx_10h_cmp_ecx_3, sizeof(mov_ecx_eax_shr_ecx_10h_cmp_ecx_3)))
	{
		__asm mov eax, (0x0003<<16)|(0x00<<8)|PS2E_DLL_VERSION
	}
	else
	{
		__asm mov eax, (PS2E_GS_VERSION<<16)|(0x00<<8)|PS2E_DLL_VERSION
	}

	__asm
	{
		pop edi 
		pop esi 
		pop edx 
		pop ecx 
		pop ebx
		ret 4
	}
}

static CAutoPtr<GSState> s_gs;
static void (*s_fpGSirq)();

EXPORT_C_(INT32) GSinit()
{
	return 0;
}

EXPORT_C GSshutdown()
{
}

static CGSWnd s_hWnd;

EXPORT_C_(INT32) GSopen(void* pDsp, char* Title)
{
	ASSERT(!s_gs);
	s_gs.Free();

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if(!s_hWnd.Create(Title) || !(*(HWND*)pDsp = s_hWnd))
		return -1;

	HRESULT hr;

	switch(AfxGetApp()->GetProfileInt(_T("Settings"), _T("Renderer"), RENDERER_D3D_HW))
	{
	default:
	case RENDERER_D3D_HW: s_gs.Attach(new GSRendererHW(s_hWnd, hr)); break;
	case RENDERER_D3D_SW_FX: s_gs.Attach(new GSRendererSoftFX(s_hWnd, hr)); break;
	case RENDERER_D3D_SW_FP: s_gs.Attach(new GSRendererSoftFP(s_hWnd, hr)); break;
	case RENDERER_D3D_NULL: s_gs.Attach(new GSRendererNull(s_hWnd, hr)); break;
	}

	if(!s_gs || FAILED(hr))
	{
		s_gs.Free();
		s_hWnd.DestroyWindow();
		return -1;
	}

	s_hWnd.Show();

	s_gs->GSirq(s_fpGSirq);

	return 0;
}

EXPORT_C GSclose()
{
	ASSERT(s_gs);
	s_gs.Free();

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	s_hWnd.DestroyWindow();
}

EXPORT_C GSwrite32(GS_REG mem, UINT32 value)
{
	ASSERT(0); // should not be called! (there are no references from pcsx2, ... this may be obsolite or something)
}

EXPORT_C GSwrite64(GS_REG mem, UINT64 value)
{
	s_gs->Write64(mem, (GSReg*)&value);
}

EXPORT_C_(UINT32) GSread32(GS_REG mem)
{
	return s_gs->Read32(mem);
}

EXPORT_C_(UINT64) GSread64(GS_REG mem)
{
	return s_gs->Read64(mem);
}

EXPORT_C GSreadFIFO(BYTE* pMem)
{
	s_gs->ReadFIFO(pMem);
}

EXPORT_C GSgifTransfer1(BYTE* pMem)
{
	s_gs->Transfer(pMem);
}

EXPORT_C GSgifTransfer2(BYTE* pMem, UINT32 size)
{
	s_gs->Transfer(pMem, size);
}

EXPORT_C GSgifTransfer3(BYTE* pMem, UINT32 size)
{
	s_gs->Transfer(pMem, size);
}

EXPORT_C GSvsync()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while(msg.message != WM_QUIT)
	{
		if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// TODO
			// Sleep(40);
			s_gs->VSync();
			break;
		}
	}
}

////////

EXPORT_C_(UINT32) GSmakeSnapshot(char* path)
{
	return s_gs->MakeSnapshot(path);
}

EXPORT_C GSkeyEvent(keyEvent* ev)
{
	if(ev->event != KEYPRESS) return;
/*
	switch(ev->key)
	{
		default:
			break;
	}
*/
}

EXPORT_C_(INT32) GSfreeze(int mode, freezeData* data)
{
	if(mode == FREEZE_SAVE)
	{
		return -1;//s_gs->Freeze(data);
	}
	else if(mode == FREEZE_LOAD)
	{
		return -1;//s_gs->Defrost(data);
	}

	return 0;
}

EXPORT_C GSconfigure()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CGSSettingsDlg().DoModal();
}

EXPORT_C_(INT32) GStest()
{
	return 0;
}

EXPORT_C GSabout()
{
}

EXPORT_C GSirqCallback(void (*fpGSirq)())
{
	s_fpGSirq = fpGSirq;
	// s_gs->GSirq(fpGSirq);
}

EXPORT_C_(INT32) GSsetWindowInfo(winInfo* info)
{
	return -1;
}
