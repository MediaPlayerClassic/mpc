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
#include "GSPerfMon.h"

GSPerfMon::GSPerfMon()
	: m_frame(0)
	, m_lastframe(0)
{
}

void GSPerfMon::Put(counter_t c, double val)
{
	if(c == Frame)
	{
		clock_t now = clock();
		
		if(m_lastframe != 0)
		{
			m_counters[c].AddTail(now - m_lastframe);
		}

		m_lastframe = now;

		m_frame++;
	}
	else
	{
		m_counters[c].AddTail(val);
	}
}

void GSPerfMon::Update()
{
	for(int i = 0; i < countof(m_counters); i++)
	{
		double sum = 0;

		POSITION pos = m_counters[i].GetHeadPosition();
		
		while(pos)
		{
			sum += m_counters[i].GetNext(pos);
		}

		m_stats[i] = sum / m_counters[Frame].GetCount();
	}
}
