// Media Player Classic.  Copyright 2003 Gabest.
// http://www.gabest.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html

#pragma once

#include "PPageFileInfoClip.h"
#include "PPageFileInfoDetails.h"

interface IFilterGraph;

// CPPageFileInfoSheet

class CPPageFileInfoSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPPageFileInfoSheet)

private:
	CPPageFileInfoClip m_clip;
	CPPageFileInfoDetails m_details;

public:
	CPPageFileInfoSheet(CString fn, IFilterGraph* pFG, CWnd* pParentWnd = NULL);
	virtual ~CPPageFileInfoSheet();

protected:
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};


