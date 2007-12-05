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

#include "StdAfx.h"
#include "GSState.h"
#include "GSUtil.h"
#include "GSSettingsDlg.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(GSState, CWnd)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

GSState::GSState() 
	: m_osd(true)
	, m_field(0)
	, m_irq(NULL)
	, m_q(1.0f)
	, m_crc(0)
	, m_options(0)
	, m_path3hack(0)
	, m_frameskip(0)
{
	m_interlace = AfxGetApp()->GetProfileInt(_T("Settings"), _T("interlace"), 0);
	m_aspectratio = AfxGetApp()->GetProfileInt(_T("Settings"), _T("aspectratio"), 1);
	m_filter = AfxGetApp()->GetProfileInt(_T("Settings"), _T("filter"), 1);
	m_nloophack = AfxGetApp()->GetProfileInt(_T("Settings"), _T("nloophack"), 2) == 1;
	m_vsync = !!AfxGetApp()->GetProfileInt(_T("Settings"), _T("vsync"), FALSE);

//	m_regs.pCSR->rREV = 0x20;

	m_env.PRMODECONT.AC = 1;

	m_pPRIM = &m_env.PRIM;

	m_pTransferBuffer = (BYTE*)_aligned_malloc(1024*1024*4, 16);
	m_nTransferBytes = 0;

	ResetHandlers();

	Reset();
}

GSState::~GSState()
{
	Reset();

	_aligned_free(m_pTransferBuffer);

	DestroyWindow();
}

bool GSState::Create(LPCTSTR title)
{
	CRect r;

	GetDesktopWindow()->GetWindowRect(r);

	CSize s(r.Width() / 3, r.Width() / 4);

	r = CRect(r.CenterPoint() - CSize(s.cx / 2, s.cy / 2), s);

	LPCTSTR wc = AfxRegisterWndClass(CS_VREDRAW|CS_HREDRAW|CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW), 0, 0);

	if(!CreateEx(0, wc, title, WS_OVERLAPPEDWINDOW, r, NULL, 0))
	{
		return false;
	}

	if(!m_dev.Create(m_hWnd))
	{
		return false;
	}

	Reset();

	return true;
}

void GSState::Show()
{
	SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	SetForegroundWindow();
	ShowWindow(SW_SHOWNORMAL);
}

void GSState::Hide()
{
	ShowWindow(SW_HIDE);
}

bool GSState::OnMsg(const MSG& msg)
{
	if(msg.message == WM_KEYDOWN)
	{
		int step = (::GetAsyncKeyState(VK_SHIFT) & 0x80000000) ? -1 : 1;

		if(msg.wParam == VK_F5)
		{
			m_interlace = (m_interlace + 7 + step) % 7;
			return true;
		}

		if(msg.wParam == VK_F6)
		{
			m_aspectratio = (m_aspectratio + 3 + step) % 3;
			return true;
		}			

		if(msg.wParam == VK_F7)
		{
			SetWindowText(_T("PCSX2"));
			m_osd = !m_osd;
			return true;
		}
	}

	return false;
}

void GSState::OnClose()
{
	Hide();

	PostMessage(WM_QUIT);
}

void GSState::Reset()
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	memset(&m_env, 0, sizeof(m_env));
	memset(m_path, 0, sizeof(m_path));
	memset(&m_v, 0, sizeof(m_v));

//	m_env.PRMODECONT.AC = 1;
//	m_pPRIM = &m_env.PRIM;

	m_context = &m_env.CTXT[0];

	m_env.CTXT[0].ftbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].FRAME.PSM];
	m_env.CTXT[0].ztbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].ZBUF.PSM];
	m_env.CTXT[0].ttbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].TEX0.PSM];

	m_env.CTXT[1].ftbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].FRAME.PSM];
	m_env.CTXT[1].ztbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].ZBUF.PSM];
	m_env.CTXT[1].ttbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].TEX0.PSM];
}

UINT32 GSState::Freeze(freezeData* fd, bool fSizeOnly)
{
	int size = sizeof(m_version)
		+ sizeof(m_env)
		+ sizeof(m_v) 
		+ sizeof(m_x) 
		+ sizeof(m_y) 
		+ 1024*1024*4
		+ sizeof(m_path) 
		+ sizeof(m_q)
		/*+ sizeof(m_vl)*/;

	if(fSizeOnly)
	{
		fd->size = size;
		return 0;
	}
	else if(!fd->data || fd->size < size)
	{
		return -1;
	}

	Flush();

	BYTE* data = fd->data;
	memcpy(data, &m_version, sizeof(m_version)); data += sizeof(m_version);
	memcpy(data, &m_env, sizeof(m_env)); data += sizeof(m_env); 
	memcpy(data, &m_v, sizeof(m_v)); data += sizeof(m_v);
	memcpy(data, &m_x, sizeof(m_x)); data += sizeof(m_x);
	memcpy(data, &m_y, sizeof(m_y)); data += sizeof(m_y);
	memcpy(data, m_mem.GetVM(), 1024*1024*4); data += 1024*1024*4;
	memcpy(data, m_path, sizeof(m_path)); data += sizeof(m_path);
	memcpy(data, &m_q, sizeof(m_q)); data += sizeof(m_q);
	// memcpy(data, &m_vl, sizeof(m_vl)); data += sizeof(m_vl);

	return 0;
}

UINT32 GSState::Defrost(const freezeData* fd)
{
	if(!fd || !fd->data || fd->size == 0) 
		return -1;

	int size = sizeof(m_version)
		+ sizeof(m_env) 
		+ sizeof(m_v) 
		+ sizeof(m_x) 
		+ sizeof(m_y) 
		+ 1024*1024*4
		+ sizeof(m_path)
		+ sizeof(m_q)
		/*+ sizeof(m_vl)*/;

	if(fd->size != size) 
		return -1;

	BYTE* data = fd->data;

	int version = 0;
	memcpy(&version, data, sizeof(version)); data += sizeof(version);
	if(m_version != version) return -1;

	Flush();

	memcpy(&m_env, data, sizeof(m_env)); data += sizeof(m_env); 
	memcpy(&m_v, data, sizeof(m_v)); data += sizeof(m_v);
	memcpy(&m_x, data, sizeof(m_x)); data += sizeof(m_x);
	memcpy(&m_y, data, sizeof(m_y)); data += sizeof(m_y);
	memcpy(m_mem.GetVM(), data, 1024*1024*4); data += 1024*1024*4;
	memcpy(&m_path, data, sizeof(m_path)); data += sizeof(m_path);
	memcpy(&m_q, data, sizeof(m_q)); data += sizeof(m_q);
	// memcpy(&m_vl, data, sizeof(m_vl)); data += sizeof(m_vl);

	m_pPRIM = !m_env.PRMODECONT.AC ? (GIFRegPRIM*)&m_env.PRMODE : &m_env.PRIM;

	m_context = &m_env.CTXT[m_pPRIM->CTXT];

	m_env.CTXT[0].ftbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].FRAME.PSM];
	m_env.CTXT[0].ztbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].ZBUF.PSM];
	m_env.CTXT[0].ttbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].TEX0.PSM];

	m_env.CTXT[1].ftbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].FRAME.PSM];
	m_env.CTXT[1].ztbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].ZBUF.PSM];
	m_env.CTXT[1].ttbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].TEX0.PSM];

// 
	m_perfmon.SetFrame(4999);

	return 0;
}

void GSState::WriteCSR(UINT32 csr)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	m_regs.pCSR->ai32[1] = csr;
}

void GSState::ReadFIFO(BYTE* mem, UINT32 size)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	Flush();

	ReadTransfer(mem, size * 16);
}

void GSState::Transfer(BYTE* mem, UINT32 size, int index)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	GIFPath& path = m_path[index];

	while(size > 0)
	{
		bool eop = false;

		if(path.tag.NLOOP == 0)
		{
			path.tag = *(GIFTag*)mem;
			path.nreg = 0;

			mem += sizeof(GIFTag);
			size--;

			m_q = 1.0f;

			if(index == 2 && path.tag.EOP)
			{
				m_path3hack = 1;
			}

			if(path.tag.PRE)
			{
				GIFReg r;
				r.i64 = path.tag.PRIM;
				(this->*m_fpGIFRegHandlers[GIF_A_D_REG_PRIM])(&r);
			}

			if(path.tag.EOP)
			{
				eop = true;
			}
			else if(path.tag.NLOOP == 0)
			{
				if(index == 0 && m_nloophack)
				{
					continue;
				}

				eop = true;
			}
		}

		switch(path.tag.FLG)
		{
		case GIF_FLG_PACKED:

			for(GIFPackedReg* r = (GIFPackedReg*)mem; path.tag.NLOOP > 0 && size > 0; r++, size--, mem += sizeof(GIFPackedReg))
			{
				(this->*m_fpGIFPackedRegHandlers[path.GetGIFReg()])(r);

				if((path.nreg = (path.nreg + 1) & 0xf) == path.tag.NREG) 
				{
					path.nreg = 0; 
					path.tag.NLOOP--;
				}
			}

			break;

		case GIF_FLG_REGLIST:

			size *= 2;

			for(GIFReg* r = (GIFReg*)mem; path.tag.NLOOP > 0 && size > 0; r++, size--, mem += sizeof(GIFReg))
			{
				(this->*m_fpGIFRegHandlers[path.GetGIFReg()])(r);

				if((path.nreg = (path.nreg + 1) & 0xf) == path.tag.NREG)
				{
					path.nreg = 0; 
					path.tag.NLOOP--;
				}
			}
			
			if(size & 1) mem += sizeof(GIFReg);

			size /= 2;
			
			break;

		case GIF_FLG_IMAGE2: // hmmm

			path.tag.NLOOP = 0;

			break;

		case GIF_FLG_IMAGE:
			{
				int len = min(size, path.tag.NLOOP);

				//ASSERT(!(len&3));

				switch(m_env.TRXDIR.XDIR)
				{
				case 0:
					WriteTransfer(mem, len*16);
					break;
				case 1: 
					ReadTransfer(mem, len*16); // TODO: writing access violation with aqtime
					break;
				case 2: 
					MoveTransfer();
					break;
				case 3: 
					ASSERT(0);
					break;
				default: 
					__assume(0);
				}

				mem += len*16;
				path.tag.NLOOP -= len;
				size -= len;
			}

			break;

		default: 
			__assume(0);
		}

		if(eop && ((int)size <= 0 || index == 0))
		{
			break;
		}
	}

	// FIXME: dq8, pcsx2 error probably

	if(index == 0)
	{
		if(!path.tag.EOP && path.tag.NLOOP > 0)
		{
			path.tag.NLOOP = 0;

			TRACE(_T("path1 hack\n"));
		}
	}
}

void GSState::VSync(int field)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	m_field = !!field;

	MSG msg;

	memset(&msg, 0, sizeof(msg));

	while(msg.message != WM_QUIT && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if(!OnMsg(msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Flush();

	Flip();

	Present();
}

UINT32 GSState::MakeSnapshot(char* path)
{
	CString fn;
	fn.Format(_T("%sgsdx10_%s.bmp"), CString(path), CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S")));
	return D3DX10SaveTextureToFile(m_dev.m_tex_current, D3DX10_IFF_BMP, fn);
}

void GSState::SetGameCRC(int crc, int options)
{
	m_crc = crc;
	m_options = options;

	if(AfxGetApp()->GetProfileInt(_T("Settings"), _T("nloophack"), 2) == 2)
	{
		switch(crc)
		{
		case 0xa39517ab: // ffx pal/eu
		case 0xa39517ae: // ffx pal/fr
		case 0x941bb7d9: // ffx pal/de
		case 0xa39517a9: // ffx pal/it
		case 0x941bb7de: // ffx pal/es
		case 0xbb3d833a: // ffx ntsc/us
		case 0x6a4efe60: // ffx ntsc/j
		case 0x3866ca7e: // ffx int. ntsc/asia (SLPM-67513, some kind of a asia version) 
		case 0x658597e2: // ffx int. ntsc/j
		case 0x9aac5309: // ffx-2 pal/e
		case 0x9aac530c: // ffx-2 pal/fr
		case 0x9aac530a: // ffx-2 pal/fr? (maybe belgium or luxembourg version)
		case 0x9aac530d: // ffx-2 pal/de
		case 0x9aac530b: // ffx-2 pal/it
		case 0x48fe0c71: // ffx-2 ntsc/us
		case 0xe1fd9a2d: // ffx-2 int+lm ntsc/j
		case 0xf0a6d880: // harvest moon ntsc/us
			m_nloophack = true;
			break;
		}
	}
}

void GSState::SetFrameSkip(int frameskip)
{
	if(m_frameskip != frameskip)
	{
		m_frameskip = frameskip;

		if(frameskip)
		{
/*			m_fpGIFPackedRegHandlers[GIF_REG_XYZF2] = &GSState::GIFPackedRegHandlerNOP;
			m_fpGIFPackedRegHandlers[GIF_REG_XYZ2] = &GSState::GIFPackedRegHandlerNOP;
			m_fpGIFPackedRegHandlers[GIF_REG_XYZF3] = &GSState::GIFPackedRegHandlerNOP;
			m_fpGIFPackedRegHandlers[GIF_REG_XYZ3] = &GSState::GIFPackedRegHandlerNOP;

			m_fpGIFRegHandlers[GIF_A_D_REG_XYZF2] = &GSState::GIFRegHandlerNOP;
			m_fpGIFRegHandlers[GIF_A_D_REG_XYZ2] = &GSState::GIFRegHandlerNOP;
			m_fpGIFRegHandlers[GIF_A_D_REG_XYZF3] = &GSState::GIFRegHandlerNOP;
			m_fpGIFRegHandlers[GIF_A_D_REG_XYZ3] = &GSState::GIFRegHandlerNOP;
*/
/*			for(int i = 0; i < countof(m_fpGIFPackedRegHandlers); i++)
			{
				m_fpGIFPackedRegHandlers[i] = &GSState::GIFPackedRegHandlerNOP;
			}

			m_fpGIFPackedRegHandlers[GIF_REG_TEX0_1] = &GSState::GIFPackedRegHandlerTEX0_1;
			m_fpGIFPackedRegHandlers[GIF_REG_TEX0_2] = &GSState::GIFPackedRegHandlerTEX0_2;
			m_fpGIFPackedRegHandlers[GIF_REG_NOP] = &GSState::GIFPackedRegHandlerNOP;

			for(int i = 0; i < countof(m_fpGIFRegHandlers); i++)
			{
				m_fpGIFRegHandlers[i] = &GSState::GIFRegHandlerNOP;
			}

			m_fpGIFRegHandlers[GIF_A_D_REG_PRIM] = &GSState::GIFRegHandlerPRIM;
			m_fpGIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GSState::GIFRegHandlerRGBAQ;
			m_fpGIFRegHandlers[GIF_A_D_REG_ST] = &GSState::GIFRegHandlerST;
			m_fpGIFRegHandlers[GIF_A_D_REG_UV] = &GSState::GIFRegHandlerUV;
			m_fpGIFRegHandlers[GIF_A_D_REG_XYZF2] = &GSState::GIFRegHandlerXYZF2;
			m_fpGIFRegHandlers[GIF_A_D_REG_XYZ2] = &GSState::GIFRegHandlerXYZ2;
			m_fpGIFRegHandlers[GIF_A_D_REG_XYZF3] = &GSState::GIFRegHandlerXYZF3;
			m_fpGIFRegHandlers[GIF_A_D_REG_XYZ3] = &GSState::GIFRegHandlerXYZ3;
			m_fpGIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GSState::GIFRegHandlerPRMODECONT;
			m_fpGIFRegHandlers[GIF_A_D_REG_PRMODE] = &GSState::GIFRegHandlerPRMODE;
*/
		}
		else
		{
			ResetHandlers();
		}
	}
}

void GSState::FinishFlip(FlipInfo src[2])
{
	CSize fs(0, 0);
	CSize ds(0, 0);

	for(int i = 0; i < 2; i++)
	{
		if(src[i].t)
		{
			CSize s = m_regs.GetFrameSize(i);

			s.cx = (int)(src[i].s.x * s.cx);
			s.cy = (int)(src[i].s.y * s.cy);

			ASSERT(fs.cx == 0 || fs.cx == s.cx);
			ASSERT(fs.cy == 0 || fs.cy == s.cy || fs.cy + 1 == s.cy);

			fs.cx = s.cx;
			fs.cy = s.cy;

			if(m_regs.pSMODE2->INT && m_regs.pSMODE2->FFMD) s.cy *= 2;

			ASSERT(ds.cx == 0 || ds.cx == s.cx);
			ASSERT(ds.cy == 0 || ds.cy == s.cy || ds.cy + 1 == s.cy);

			ds.cx = s.cx;
			ds.cy = s.cy;
		}
	}

	if(fs.cx == 0 || fs.cy == 0)
	{
		return;
	}

	// merge

	if(!m_dev.m_tex_merge || m_dev.m_tex_merge.m_desc.Width != fs.cx || m_dev.m_tex_merge.m_desc.Height != fs.cy)
	{
		m_dev.CreateRenderTarget(m_dev.m_tex_merge, fs.cx, fs.cy);
	}

	Merge(src, m_dev.m_tex_merge);

	ID3D10Texture2D* current = m_dev.m_tex_merge;

	if(m_regs.pSMODE2->INT && m_interlace > 0)
	{
		int field = 1 - ((m_interlace - 1) & 1);
		int mode = (m_interlace - 1) >> 1;

		current = m_dev.Interlace(m_dev.m_tex_merge, ds, m_field ^ field, mode, src[1].s.y);

		if(!current) return;
	}

	m_dev.m_tex_current = current;
}

void GSState::Merge(FlipInfo src[2], GSTexture2D& dst)
{
	// om

	m_dev.OMSet(m_dev.m_convert.dss, 0, m_dev.m_convert.bs, 0);

	m_dev.OMSet(dst, NULL);

	// ia

	CRect r[2];
	
	r[0] = m_regs.GetFrameRect(0);
	r[1] = m_regs.GetFrameRect(1);

	VertexPT2 vertices[] =
	{
		{-1, +1, 0.5f, 1.0f, 
			src[0].s.x * r[0].left / src[0].t.m_desc.Width, src[0].s.y * r[0].top / src[0].t.m_desc.Height,
			src[1].s.x * r[1].left / src[1].t.m_desc.Width, src[1].s.y * r[1].top / src[1].t.m_desc.Height},
		{+1, +1, 0.5f, 1.0f, 
			src[0].s.x * r[0].right / src[0].t.m_desc.Width, src[0].s.y * r[0].top / src[0].t.m_desc.Height,
			src[1].s.x * r[1].right / src[1].t.m_desc.Width, src[1].s.y * r[1].top / src[1].t.m_desc.Height},
		{-1, -1, 0.5f, 1.0f, 
			src[0].s.x * r[0].left / src[0].t.m_desc.Width, src[0].s.y * r[0].bottom / src[0].t.m_desc.Height,
			src[1].s.x * r[1].left / src[1].t.m_desc.Width, src[1].s.y * r[1].bottom / src[1].t.m_desc.Height}, 
		{+1, -1, 0.5f, 1.0f,
			src[0].s.x * r[0].right / src[0].t.m_desc.Width, src[0].s.y * r[0].bottom / src[0].t.m_desc.Height,
			src[1].s.x * r[1].right / src[1].t.m_desc.Width, src[1].s.y * r[1].bottom / src[1].t.m_desc.Height}, 
	};

	m_dev.IASet(m_dev.m_merge.vb, 4, vertices, m_dev.m_merge.il, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// vs

	m_dev->VSSetShader(m_dev.m_merge.vs);

	// gs

	m_dev.GSSet(NULL);

	// ps
	
	MergeCB* cb = NULL;
	
	if(SUCCEEDED(m_dev.m_merge.cb->Map(D3D10_MAP_WRITE_DISCARD, NULL, (void**)&cb)))
	{
		cb->BGColor.x = (float)m_regs.pBGCOLOR->R / 255;
		cb->BGColor.y = (float)m_regs.pBGCOLOR->G / 255;
		cb->BGColor.z = (float)m_regs.pBGCOLOR->B / 255;
		cb->BGColor.w = 0;
		cb->Alpha = (float)m_regs.pPMODE->ALP / 255;
		cb->EN1 = (float)m_regs.IsEnabled(0);
		cb->EN2 = (float)m_regs.IsEnabled(1);
		cb->MMOD = !!m_regs.pPMODE->MMOD;
		cb->SLBG = !!m_regs.pPMODE->SLBG;

		m_dev.m_merge.cb->Unmap();
	}
	
	m_dev->PSSetConstantBuffers(0, 1, &m_dev.m_merge.cb.p);

	ID3D10ShaderResourceView* srvs[] = 
	{
		src[0].t ? src[0].t : m_dev.m_tex_1x1,
		src[1].t ? src[1].t : m_dev.m_tex_1x1,
	};

	m_dev->PSSetShaderResources(0, 2, srvs);

	m_dev.PSSet(m_dev.m_merge.ps, m_dev.m_ss_linear);

	// rs

	m_dev.RSSet(dst.m_desc.Width, dst.m_desc.Height);

	//

	m_dev->Draw(4, 0);

	m_dev.EndScene();
}

void GSState::Present()
{
	m_perfmon.Put(GSPerfMon::Frame);

	HRESULT hr;

	CRect cr;
	GetClientRect(&cr);

	D3D10_TEXTURE2D_DESC desc;
	memset(&desc, 0, sizeof(desc));
	m_dev.m_backbuffer->GetDesc(&desc);

	if(desc.Width != cr.Width() || desc.Height != cr.Height())
	{
		ResetDevice();

		m_dev.ResetDevice(cr.Width(), cr.Height());		
	}

	CComPtr<ID3D10RenderTargetView> rtv;

	hr = m_dev->CreateRenderTargetView(m_dev.m_backbuffer, NULL, &rtv.p);

	float color[4] = {0, 0, 0, 0};

	m_dev->ClearRenderTargetView(rtv, color);

	if(m_dev.m_tex_current)
	{
		static int ar[][2] = {{0, 0}, {4, 3}, {16, 9}};

		int arx = ar[m_aspectratio][0];
		int ary = ar[m_aspectratio][1];

		CRect r = cr;

		if(arx > 0 && ary > 0)
		{
			if(r.Width() * ary > r.Height() * arx)
			{
				int w = r.Height() * arx / ary;
				r.left = r.CenterPoint().x - w / 2;
				if(r.left & 1) r.left++;
				r.right = r.left + w;
			}
			else
			{
				int h = r.Width() * ary / arx;
				r.top = r.CenterPoint().y - h / 2;
				if(r.top & 1) r.top++;
				r.bottom = r.top + h;
			}
		}

		r &= cr;

		m_dev.StretchRect(
			GSTexture2D(m_dev.m_tex_current), 
			GSTexture2D(m_dev.m_backbuffer), 
			D3DXVECTOR4(r.left, r.top, r.right, r.bottom));
	}

	// osd

	static UINT64 s_frame = 0;
	static CString s_stats;

	if(m_perfmon.GetFrame() - s_frame >= 30)
	{
		m_perfmon.Update();

		s_frame = m_perfmon.GetFrame();

		double fps = 1000.0f / m_perfmon.Get(GSPerfMon::Frame);
		
		s_stats.Format(
			_T("%I64d | %d x %d | %.2f fps (%d%%) | %s - %s | %s | %d/%d | %d%% CPU | %.2f | %.2f/%.2f | %.2f"), 
			m_perfmon.GetFrame(), m_regs.GetDisplaySize().cx, m_regs.GetDisplaySize().cy, fps, (int)(100.0 * fps / m_regs.GetFPS()),
			m_regs.pSMODE2->INT ? (CString(_T("Interlaced ")) + (m_regs.pSMODE2->FFMD ? _T("(frame)") : _T("(field)"))) : _T("Progressive"),
			g_interlace[m_interlace].name,
			g_aspectratio[m_aspectratio].name,
			(int)m_perfmon.Get(GSPerfMon::Prim),
			(int)m_perfmon.Get(GSPerfMon::Draw),
			m_perfmon.CPU(),
			m_perfmon.Get(GSPerfMon::Swizzle) / 1024,
			m_perfmon.Get(GSPerfMon::Unswizzle) / 1024,
			m_perfmon.Get(GSPerfMon::Unswizzle2) / 1024,
			m_perfmon.Get(GSPerfMon::Texture) / 1024
			);

		if(m_osd /*&& m_d3dpp.Windowed*/)
		{
			SetWindowText(s_stats);
		}
	}
/*
	if(m_osd && !m_d3dpp.Windowed)
	{
		hr = m_dev->BeginScene();

		hr = m_dev->SetRenderTarget(0, pBackBuffer);
		hr = m_dev->SetDepthStencilSurface(NULL);

		CRect r;
		
		GetClientRect(r);

		D3DCOLOR c = D3DCOLOR_ARGB(255, 0, 255, 0);

		CString str = s_stats;

		str += _T("\n\nF5: interlace mode\nF6: aspect ratio\nF7: OSD");

		if(m_pD3DXFont->DrawText(NULL, str, -1, &r, DT_CALCRECT|DT_LEFT|DT_WORDBREAK, c))
		{
			m_pD3DXFont->DrawText(NULL, str, -1, &r, DT_LEFT|DT_WORDBREAK, c);
		}

		hr = m_dev->EndScene();
	}
*/
	m_dev.Present();
}

void GSState::Flush()
{
	FlushWriteTransfer();

	FlushPrim();
}

