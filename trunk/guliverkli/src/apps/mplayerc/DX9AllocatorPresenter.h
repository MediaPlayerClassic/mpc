#pragma once

#include "..\..\SubPic\ISubPic.h"

// {4E4834FA-22C2-40e2-9446-F77DD05D245E}
DEFINE_GUID(CLSID_VMR9AllocatorPresenter, 
0x4e4834fa, 0x22c2, 0x40e2, 0x94, 0x46, 0xf7, 0x7d, 0xd0, 0x5d, 0x24, 0x5e);

extern HRESULT CreateAP9(const CLSID& clsid, HWND hWnd, ISubPicAllocatorPresenter** ppAP);
