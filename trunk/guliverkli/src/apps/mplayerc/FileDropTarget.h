
#pragma once

#include <afxole.h>

class CDropTarget
{
public:
	CDropTarget() {}

	virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) {return DROPEFFECT_NONE;}
	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) {return DROPEFFECT_NONE;}
	virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) {return FALSE;}
	virtual DROPEFFECT OnDropEx(COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point) {return (DROPEFFECT)-1;}
	virtual void OnDragLeave() {}
	virtual DROPEFFECT OnDragScroll(DWORD dwKeyState, CPoint point) {return DROPEFFECT_NONE;}
};

// CFileDropTarget command target

class CFileDropTarget : public COleDropTarget
{
//	DECLARE_DYNAMIC(CFileDropTarget)

private:
	CDropTarget* m_pDropTarget;

public:
	CFileDropTarget(CDropTarget* pDropTarget);
	virtual ~CFileDropTarget();

protected:
	DECLARE_MESSAGE_MAP()

	DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	DROPEFFECT OnDropEx(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point);
	void OnDragLeave(CWnd* pWnd);
	DROPEFFECT OnDragScroll(CWnd* pWnd, DWORD dwKeyState, CPoint point);
};


