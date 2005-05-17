#pragma once

#include "x86.h"

class GSPerfMon
{
public:
	enum counter_t {c_frame, c_prim, c_swizzle, c_unswizzle, c_texture, c_last};

protected:
	CAtlList<double> m_counters[c_last];
	UINT64 m_begin, m_total, m_start, m_frame;
	clock_t m_lastframe;

public:
	GSPerfMon();

	void IncCounter(counter_t c, double val = 0);
	void StartTimer(), StopTimer();
	CString ToString(double expected_fps);
	UINT64 GetFrame() {return m_frame;}
};

class GSPerfMonAutoTimer
{
	GSPerfMon* m_pm;

public:
	GSPerfMonAutoTimer(GSPerfMon& pm) {(m_pm = &pm)->StartTimer();}
	~GSPerfMonAutoTimer() {m_pm->StopTimer();}
};