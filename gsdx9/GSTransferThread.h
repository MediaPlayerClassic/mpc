#pragma once

#include "GSState.h"
#include "GSQueue.h"

class GSTransferThread
{
	class GSTransferBuffer
	{
	public:
		BYTE* m_data;
		UINT32 m_size;
		int m_index;

		GSTransferBuffer()
		{
			m_data = NULL;
			m_size = 0;
		}

		~GSTransferBuffer()
		{
			delete [] m_data;
		}
	};

	GSQueue<GSTransferBuffer*> m_queue;

	CAMEvent m_evThreadQuit;
	CAMEvent m_evThreadIdle;

	HANDLE m_hThread;
	DWORD m_ThreadId;

	GSState* m_gs;
	GIFPath m_path[3];

	static DWORD WINAPI StaticThreadProc(LPVOID lpParam);

	DWORD ThreadProc();

public:
	GSTransferThread(GSState* gs);
	virtual ~GSTransferThread();

	void Transfer(BYTE* mem, UINT32 size, int index);
	void Reset();
	void Wait();
};
