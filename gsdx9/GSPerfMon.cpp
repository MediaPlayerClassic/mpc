#include "StdAfx.h"
#include "GSPerfMon.h"

GSPerfMon::GSPerfMon()
	: m_total(0), m_begin(0)
{
}

void GSPerfMon::Start()
{
	m_start = rdtsc(); 
	if(m_begin == 0) m_begin = m_start;
}

void GSPerfMon::Stop()
{
	if(m_start > 0)
	{
		m_total += rdtsc() - m_start;
		m_start = 0;
	}
}

float GSPerfMon::CpuUsage()
{
	Stop();
	m_end = rdtsc(); 
	float ret = (float)m_total / (m_end - m_begin); 
	m_total = m_begin = 0;
	return ret;
}
