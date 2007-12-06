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

extern "C" unsigned __int64 __rdtsc();

GSPerfMon::GSPerfMon()
	: m_total(0)
	, m_begin(0)
	, m_frame(0)
	, m_lastframe(0)
{
	memset(m_stats, 0, sizeof(m_stats));
	memset(m_warnings, 0, sizeof(m_warnings));
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
	size_t frames = m_counters[Frame].GetCount();

	for(int i = 0; i < countof(m_counters); i++)
	{
		double sum = 0;

		POSITION pos = m_counters[i].GetHeadPosition();
		
		while(pos)
		{
			sum += m_counters[i].GetNext(pos);
		}

		m_stats[i] = sum / frames;

		m_counters[i].RemoveAll();
	}
}

void GSPerfMon::Start()
{
	m_start = __rdtsc();

	if(m_begin == 0)
	{
		m_begin = m_start;
	}
}

void GSPerfMon::Stop()
{
	if(m_start > 0)
	{
		m_total += __rdtsc() - m_start;
		m_start = 0;
	}
}

int GSPerfMon::CPU()
{
	int percent = (int)(100 * m_total / (__rdtsc() - m_begin));

	m_begin = 0;
	m_start = 0;
	m_total = 0;

	return percent;
}