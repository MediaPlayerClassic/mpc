#pragma once

#include <vector>
#include "..\SubPic\ISubPic.h"

#define PT_MOVETONC 0xfe
#define PT_BSPLINETO 0xfc
#define PT_BSPLINEPATCHTO 0xfa 

class Rasterizer
{
	bool fFirstSet;
	CPoint firstp, lastp;

protected:
	BYTE* mpPathTypes;
	POINT* mpPathPoints;
	int mPathPoints;

private:
	int mWidth, mHeight;

	typedef std::pair<unsigned __int64, unsigned __int64> tSpan;
	typedef std::vector<tSpan> tSpanBuffer;

	tSpanBuffer mOutline;
	tSpanBuffer mWideOutline;
	int mWideBorder;

	struct Edge {
		int next;
		int posandflag;
	} *mpEdgeBuffer;
	unsigned mEdgeHeapSize;
	unsigned mEdgeNext;

	unsigned __int64* mpScanBuffer;

	typedef unsigned char byte;

protected:
	byte *mpOverlayBuffer;
	int mOverlayWidth, mOverlayHeight;
	int mPathOffsetX, mPathOffsetY;
	int mOffsetX, mOffsetY;

private:
	void _TrashPath();
	void _TrashOverlay();
	void _ReallocEdgeBuffer(int edges);
	void _EvaluateBezier(int ptbase, bool fBSpline);
	void _EvaluateLine(int pt1idx, int pt2idx);
	void _EvaluateLine(int x0, int y0, int x1, int y1);
	static void _OverlapRegion(tSpanBuffer& dst, tSpanBuffer& src, int dx, int dy);

public:
	Rasterizer();
	virtual ~Rasterizer();

	bool BeginPath(HDC hdc);
	bool EndPath(HDC hdc);
	bool PartialBeginPath(HDC hdc, bool bClearPath);
	bool PartialEndPath(HDC hdc, long dx, long dy);
	bool ScanConvert();
	bool CreateWidenedRegion(int border);
	void DeleteOutlines();
	bool Rasterize(int xsub, int ysub, bool fBorder, bool fBlur);
	CRect Draw(SubPicDesc& spd, CRect& clipRect, byte* pAlphaMask, int xsub, int ysub, const long* switchpts, bool fBody, bool fBorder);
};

