#pragma once

class GSCapture
{
	CComPtr<IDirect3DSurface9> m_pRTSurf, m_pSysMemSurf;

	CComPtr<IGraphBuilder> m_pGB;
	CComPtr<IBaseFilter> m_pSrc;

public:
	GSCapture();

	bool BeginCapture(IDirect3DDevice9* pD3Dev, int fps);
	bool BeginFrame(int& w, int& h, IDirect3DSurface9** pRTSurf);
	bool EndFrame();
	bool EndCapture();

	bool IsCapturing() {return !!m_pRTSurf;}
};
