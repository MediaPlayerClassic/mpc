#include "stdafx.h"
#include "GSTransferThread.h"

GSTransferThread::GSTransferThread(GSState* gs)
	: m_gs(gs)
	, m_evThreadQuit(FALSE)
	, m_evThreadIdle(TRUE)
	, m_queue(100)
{
	m_ThreadId = 0;
	m_hThread = ::CreateThread(NULL, 0, StaticThreadProc, (LPVOID)this, 0, &m_ThreadId);

	m_evThreadIdle.Set();
}

GSTransferThread::~GSTransferThread()
{
	m_evThreadQuit.Set();
		
	if(WaitForSingleObject(m_hThread, 30000) == WAIT_TIMEOUT) 
	{
		ASSERT(0); 
		TerminateThread(m_hThread, (DWORD)-1);
	}
}

void GSTransferThread::Transfer(BYTE* mem, UINT32 size, int index)
{
	GSTransferBuffer* buff = new GSTransferBuffer();

	buff->m_data = mem;
	buff->m_size = size;
	buff->m_index = index;

	GIFPath& path = m_path[index];

	while(size > 0)
	{
		bool fEOP = false;

		if(path.tag.NLOOP == 0)
		{
			path.tag = *(GIFTag*)mem;
			path.nreg = 0;

			mem += sizeof(GIFTag);
			size--;

			if(path.tag.EOP)
			{
				fEOP = true;
			}
			else if(path.tag.NLOOP == 0)
			{
				if(index == 0)
				{
					continue;
				}

				fEOP = true;
			}
		}

		UINT32 size_msb = size & (1<<31);

		switch(path.tag.FLG)
		{
		case GIF_FLG_PACKED:

			for(GIFPackedReg* r = (GIFPackedReg*)mem; path.tag.NLOOP > 0 && size > 0; r++, size--, mem += sizeof(GIFPackedReg))
			{
				if((path.nreg = (path.nreg + 1) & 0xf) == path.tag.NREG) 
				{
					path.nreg = 0; 
					path.tag.NLOOP--;
				}
			}

			break;

		case GIF_FLG_REGLIST:

			size *= 2;

			for(GIFReg* r = (GIFReg*)mem; path.tag.NLOOP > 0 && size > 0; r++, size--, mem += sizeof(GIFReg))
			{
				if((path.nreg = (path.nreg + 1) & 0xf) == path.tag.NREG)
				{
					path.nreg = 0; 
					path.tag.NLOOP--;
				}
			}
			
			if(size & 1) mem += sizeof(GIFReg);

			size /= 2;
			size |= size_msb; // a bit lame :P
			
			break;

		case GIF_FLG_IMAGE2: // hmmm
			
			path.tag.NLOOP = 0;

			break;

		case GIF_FLG_IMAGE:

			{
				int len = min(size, path.tag.NLOOP);

				//ASSERT(!(len&3));

				mem += len*16;
				path.tag.NLOOP -= len;
				size -= len;
			}

			break;

		default: 
			__assume(0);
		}

		if(fEOP && (INT32)size <= 0)
		{
			break;
		}
	}

	size = (UINT32)(mem - buff->m_data);

	ASSERT(size > 0);

	memcpy(buff->m_data = new BYTE[size], mem - size, size);

	m_queue.Enqueue(buff);
}

void GSTransferThread::Reset()
{
	memset(m_path, 0, sizeof(m_path));
}

void GSTransferThread::Wait()
{
	m_queue.GetDequeueEvent().Wait();

	m_evThreadIdle.Wait();
}

DWORD WINAPI GSTransferThread::StaticThreadProc(LPVOID lpParam)
{
	HRESULT hr = ::CoInitializeEx(0, COINIT_MULTITHREADED);

	DWORD ret = ((GSTransferThread*)lpParam)->ThreadProc();

	if(SUCCEEDED(hr)) ::CoUninitialize();

	return ret;
}

DWORD GSTransferThread::ThreadProc()
{
	const HANDLE events[] = {m_evThreadQuit, m_queue.GetEnqueueEvent()};

	while(1)
	{
		switch(WaitForMultipleObjects(countof(events), events, FALSE, INFINITE))
		{
		case WAIT_OBJECT_0+0:
			return 0;

		case WAIT_OBJECT_0+1:

			m_evThreadIdle.Reset();

			while(m_queue.GetCount() > 0)
			{
				GSTransferBuffer* tb = m_queue.Dequeue();
				m_gs->Transfer(tb->m_data, tb->m_size, tb->m_index);
				delete tb;
			}

			m_evThreadIdle.Set();

			break;

		default:
			return -1;
		}
	}

	ASSERT(0);

	return -1;
}
