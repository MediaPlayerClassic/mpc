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

#pragma once

#include "GS.h"

struct GSRegSet
{
	GSRegPMODE*		pPMODE;
	GSRegSMODE1*	pSMODE1;
	GSRegSMODE2*	pSMODE2;
	GSRegDISPFB*	pDISPFB[2];
	GSRegDISPLAY*	pDISPLAY[2];
	GSRegEXTBUF*	pEXTBUF;
	GSRegEXTDATA*	pEXTDATA;
	GSRegEXTWRITE*	pEXTWRITE;
	GSRegBGCOLOR*	pBGCOLOR;
	GSRegCSR*		pCSR;
	GSRegIMR*		pIMR;
	GSRegBUSDIR*	pBUSDIR;
	GSRegSIGLBLID*	pSIGLBLID;

	struct GSRegSet()
	{
		memset(this, 0, sizeof(*this));

		extern BYTE* g_pBasePS2Mem;

		ASSERT(g_pBasePS2Mem);

		pPMODE = (GSRegPMODE*)(g_pBasePS2Mem + GS_PMODE);
		pSMODE1 = (GSRegSMODE1*)(g_pBasePS2Mem + GS_SMODE1);
		pSMODE2 = (GSRegSMODE2*)(g_pBasePS2Mem + GS_SMODE2);
		// pSRFSH = (GSRegPMODE*)(g_pBasePS2Mem + GS_SRFSH);
		// pSYNCH1 = (GSRegPMODE*)(g_pBasePS2Mem + GS_SYNCH1);
		// pSYNCH2 = (GSRegPMODE*)(g_pBasePS2Mem + GS_SYNCH2);
		// pSYNCV = (GSRegPMODE*)(g_pBasePS2Mem + GS_SYNCV);
		pDISPFB[0] = (GSRegDISPFB*)(g_pBasePS2Mem + GS_DISPFB1);
		pDISPFB[1] = (GSRegDISPFB*)(g_pBasePS2Mem + GS_DISPFB2);
		pDISPLAY[0] = (GSRegDISPLAY*)(g_pBasePS2Mem + GS_DISPLAY1);
		pDISPLAY[1] = (GSRegDISPLAY*)(g_pBasePS2Mem + GS_DISPLAY2);
		pEXTBUF = (GSRegEXTBUF*)(g_pBasePS2Mem + GS_EXTBUF);
		pEXTDATA = (GSRegEXTDATA*)(g_pBasePS2Mem + GS_EXTDATA);
		pEXTWRITE = (GSRegEXTWRITE*)(g_pBasePS2Mem + GS_EXTWRITE);
		pBGCOLOR = (GSRegBGCOLOR*)(g_pBasePS2Mem + GS_BGCOLOR);
		pCSR = (GSRegCSR*)(g_pBasePS2Mem + GS_CSR);
		pIMR = (GSRegIMR*)(g_pBasePS2Mem + GS_IMR);
		pBUSDIR = (GSRegBUSDIR*)(g_pBasePS2Mem + GS_BUSDIR);
		pSIGLBLID = (GSRegSIGLBLID*)(g_pBasePS2Mem + GS_SIGLBLID);
	}

	CSize GetDispSize(int i)
	{
		ASSERT(i >= 0 && i < 2);

		CSize size;

		size.cx = (pDISPLAY[i]->DW + 1) / (pDISPLAY[i]->MAGH + 1);
		size.cy = (pDISPLAY[i]->DH + 1) / (pDISPLAY[i]->MAGV + 1);

		//if(pSMODE2->INT && pSMODE2->FFMD && size.cy > 1) size.cy >>= 1;

		return size;
	}

	CRect GetDispRect(int i)
	{
		ASSERT(i >= 0 && i < 2);

		return CRect(CPoint(pDISPFB[i]->DBX, pDISPFB[i]->DBY), GetDispSize(i));
	}

	bool IsEnabled(int i)
	{
		ASSERT(i >= 0 && i < 2);

		if(i == 0 && pPMODE->EN1) 
		{
			return pDISPLAY[0]->DW || pDISPLAY[0]->DH;
		}
		else if(i == 1 && pPMODE->EN2) 
		{
			return pDISPLAY[1]->DW || pDISPLAY[1]->DH;
		}

		return false;
	}

	int GetFPS()
	{
		return ((pSMODE1->CMOD & 1) ? 50 : 60) / (pSMODE2->INT ? 1 : 2);
	}
};
