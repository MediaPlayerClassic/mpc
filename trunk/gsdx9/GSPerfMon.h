#pragma once

#include "x86.h"

class GSPerfMon
{
	UINT64 m_begin, m_end, m_start, m_total;

public:
	GSPerfMon();

	void Start(), Stop();
	float CpuUsage();
};

class GSPerfMonAutoTimer
{
	GSPerfMon* m_pm;

public:
	GSPerfMonAutoTimer(GSPerfMon& pm) {(m_pm = &pm)->Start();}
	~GSPerfMonAutoTimer() {m_pm->Stop();}
};

class GSStats
{
	int m_frame, m_prim, m_write, m_read, m_texwrite;
	float m_fps, m_pps, m_ppf, m_wps, m_wpf, m_rps, m_rpf, m_twps, m_twpf, m_cpu;
	CList<clock_t> m_clk;
	CList<int> m_prims, m_writes, m_reads, m_texwrites;
	CList<float> m_cpus;

public:
	GSStats() {Reset();}
	void Reset() {m_frame = m_prim = m_write = m_read = m_texwrite = 0; m_fps = m_pps = m_ppf = m_wps = m_wpf = m_rps = m_rpf = m_twps = m_twpf = m_cpu = 0;}
	void IncPrims(int n) {m_prim += n;}
	void IncWrites(int bytes) {m_write += bytes;}
	void IncReads(int bytes) {m_read += bytes;}
	void IncTexWrite(int bytes) {m_texwrite += bytes;}
	void VSync(float cpu)
	{
		m_prims.AddTail(m_prim);
		m_writes.AddTail(m_write);
		m_reads.AddTail(m_read);
		m_texwrites.AddTail(m_texwrite);
		m_cpus.AddTail(cpu);

		m_clk.AddTail(clock());
		m_frame++;

		if(int dt = clock() - m_clk.GetHead())
		{
			m_fps = 1000.0f * m_clk.GetCount() / dt;

			int prims = 0;
			POSITION pos = m_prims.GetHeadPosition();
			while(pos) prims += m_prims.GetNext(pos);
			m_pps = 1000.0f * prims / dt;
			m_ppf = 1.0f * prims / m_clk.GetCount();

			int bytes = 0;
			pos = m_writes.GetHeadPosition();
			while(pos) bytes += m_writes.GetNext(pos);
			m_wps = 1000.0f * bytes / dt;
			m_wpf = 1.0f * bytes / m_clk.GetCount();

			bytes = 0;
			pos = m_reads.GetHeadPosition();
			while(pos) bytes += m_reads.GetNext(pos);
			m_rps = 1000.0f * bytes / dt;
			m_rpf = 1.0f * bytes / m_clk.GetCount();

			bytes = 0;
			pos = m_texwrites.GetHeadPosition();
			while(pos) bytes += m_texwrites.GetNext(pos);
			m_twps = 1000.0f * bytes / dt;
			m_twpf = 1.0f * bytes / m_clk.GetCount();

			cpu = 0.0f;
			pos = m_cpus.GetHeadPosition();
			while(pos) cpu += m_cpus.GetNext(pos);
			m_cpu = cpu / m_clk.GetCount();
		}

		if(m_clk.GetCount() > 10)
		{
			m_clk.RemoveHead();
			m_prims.RemoveHead();
			m_writes.RemoveHead();
			m_reads.RemoveHead();
			m_texwrites.RemoveHead();
			m_cpus.RemoveHead();
		}

		m_prim = 0;
		m_write = 0;
		m_read = 0;
		m_texwrite = 0;
	}
	int GetFrame() {return m_frame;}
	CString ToString(int fps)
	{
		CString str; 
		str.Format(_T("frame: %d | cpu: %d%% | %.2f fps (%d%%) | %d ppf | %.2f kbpf | %.2f kbpf | %.2f kbpf"), 
			m_frame, (int)(100*m_cpu), m_fps, (int)(100*m_fps/fps),
			(int)m_ppf, m_wpf / 1024, m_rpf / 1024, m_twpf / 1024); 
		return str;
	}
};
