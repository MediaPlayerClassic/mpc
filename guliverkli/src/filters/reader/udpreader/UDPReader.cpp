/* 
 *	Copyright (C) 2003-2006 Gabest
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
#include "udpreader.h"
#include "..\..\..\DSUtil\DSUtil.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudOpPin[] =
{
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CUDPReader), L"UDP Reader", MERIT_UNLIKELY, countof(sudOpPin), sudOpPin}
};

CFactoryTemplate g_Templates[] =
{
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CUDPReader>, NULL, &sudFilter[0]}
};

int g_cTemplates = countof(g_Templates);

STDAPI DllRegisterServer()
{
	SetRegKeyValue(_T("udp"), 0, _T("Source Filter"), CStringFromGUID(__uuidof(CUDPReader)));

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	// TODO

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, dwReason, 0); // "DllMain" of the dshow baseclasses;
}

#endif

#define BUFF_SIZE (256*1024)
#define BUFF_SIZE_FIRST (4*BUFF_SIZE)

//
// CUDPReader
//

CUDPReader::CUDPReader(IUnknown* pUnk, HRESULT* phr)
	: CAsyncReader(NAME("CUDPReader"), pUnk, &m_stream, phr, __uuidof(this))
{
	if(phr) *phr = S_OK;
}

CUDPReader::~CUDPReader()
{
}

STDMETHODIMP CUDPReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IFileSourceFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IFileSourceFilter

STDMETHODIMP CUDPReader::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt) 
{
	if(!m_stream.Load(pszFileName))
		return E_FAIL;

	m_fn = pszFileName;

	CMediaType mt;
	mt.majortype = MEDIATYPE_Stream;
	mt.subtype = m_stream.GetSubType();
	m_mt = mt;

	return S_OK;
}

STDMETHODIMP CUDPReader::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	if(!ppszFileName) return E_POINTER;
	
	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_fn.GetLength()+1)*sizeof(WCHAR))))
		return E_OUTOFMEMORY;

	wcscpy(*ppszFileName, m_fn);

	return S_OK;
}

// CUDPStream

CUDPStream::CUDPStream()
{
	m_port = 0;
	m_socket = -1;
	m_subtype = MEDIASUBTYPE_NULL;
}

CUDPStream::~CUDPStream()
{
	Clear();
}

void CUDPStream::Clear()
{
	if(m_socket >= 0) {closesocket(m_socket); m_socket = -1;}
	if(CAMThread::ThreadExists())
	{
		CAMThread::CallWorker(CMD_EXIT);
		CAMThread::Close();
	}
	while(!m_packets.IsEmpty()) delete m_packets.RemoveHead();
	m_pos = m_len = 0;
	m_drop = false;
}

void CUDPStream::Append(BYTE* buff, int len)
{
	CAutoLock cAutoLock(&m_csLock);

	if(m_packets.GetCount() > 1)
	{
		__int64 size = m_packets.GetTail()->m_end - m_packets.GetHead()->m_start;

		if(!m_drop && (m_pos >= BUFF_SIZE_FIRST && size >= BUFF_SIZE_FIRST || size >= 2*BUFF_SIZE_FIRST)) 
		{
			m_drop = true;
			TRACE(_T("DROP ON\n"));
		}
		else if(m_drop && size <= BUFF_SIZE_FIRST) 
		{
			m_drop = false;
			TRACE(_T("DROP OFF\n"));
		}
		
		if(m_drop) return;
	}

	m_packets.AddTail(new packet_t(buff, m_len, m_len + len));
	m_len += len;
}

bool CUDPStream::Load(const WCHAR* fnw)
{
	Clear();

//#ifdef DEBUG
//	CStringW url = L"udp://@:4321"; 
//#else
	CStringW url = CStringW(fnw);
//#endif

	CList<CStringW> sl;
	Explode(url, sl, ':');
	if(sl.GetCount() != 3) return false;

	CStringW protocol = sl.RemoveHead();
	if(protocol != L"udp") return false;

	if(FAILED(GUIDFromCString(CString(sl.RemoveHead()).TrimLeft('/'), m_subtype)))
		m_subtype = MEDIASUBTYPE_NULL; // TODO

	int port = _wtoi(sl.RemoveHead());
	if(port < 0 || port > 0xffff) return false;

	m_port = port;

	CAMThread::Create();
	if(FAILED(CAMThread::CallWorker(CMD_RUN)))
	{
		Clear();
		return false;
	}

	return true;
}

HRESULT CUDPStream::SetPointer(LONGLONG llPos)
{
	CAutoLock cAutoLock(&m_csLock);

	if(m_packets.IsEmpty() && llPos != 0
	|| !m_packets.IsEmpty() && llPos < m_packets.GetHead()->m_start 
	|| !m_packets.IsEmpty() && llPos > m_packets.GetTail()->m_end)
	{
		ASSERT(0); 
		return E_FAIL;
	}

	m_pos = llPos;

	return S_OK;
}

HRESULT CUDPStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
	CAutoLock cAutoLock(&m_csLock);

	DWORD len = dwBytesToRead;
	BYTE* ptr = pbBuffer;

	while(len > 0 && !m_packets.IsEmpty())
	{
		POSITION pos = m_packets.GetHeadPosition();
		while(pos && len > 0)
		{
			packet_t* p = m_packets.GetNext(pos);

			if(p->m_start <= m_pos && m_pos < p->m_end)
			{
				int size;

				if(m_pos < p->m_start)
				{
					ASSERT(0);
					size = min(len, p->m_start - m_pos);
					memset(ptr, 0, size);
				}
				else
				{
					size = min(len, p->m_end - m_pos);
					memcpy(ptr, &p->m_buff[m_pos - p->m_start], size);
				}

				m_pos += size;

				ptr += size;
				len -= size;
			}

			if(p->m_end <= m_pos - 2048 && BUFF_SIZE_FIRST <= m_pos)
			{
				while(m_packets.GetHeadPosition() != pos)
					delete m_packets.RemoveHead();
			}

		}
	}

	if(pdwBytesRead)
		*pdwBytesRead = ptr - pbBuffer;

	return S_OK;
}

LONGLONG CUDPStream::Size(LONGLONG* pSizeAvailable)
{
	CAutoLock cAutoLock(&m_csLock);
	if(pSizeAvailable) *pSizeAvailable = m_len;
	return 0;
}

DWORD CUDPStream::Alignment()
{
    return 1;
}

void CUDPStream::Lock()
{
    m_csLock.Lock();
}

void CUDPStream::Unlock()
{
    m_csLock.Unlock();
}

DWORD CUDPStream::ThreadProc()
{
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if((m_socket = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
	{
/*		u_long argp = 1;
		ioctlsocket(m_socket, FIONBIO, &argp);
*/
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons((u_short)m_port);

	    if(bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		{
			closesocket(m_socket);
			m_socket = -1;
		}
	}

	SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);

	FILE* dump = NULL;
	// dump = _tfopen(_T("c:\\rip\\dump.ts"), _T("wb"));

	while(1)
	{
		DWORD cmd = GetRequest();

		switch(cmd)
		{
		default:
		case CMD_EXIT: 
			if(m_socket >= 0) {closesocket(m_socket); m_socket = -1;}
			if(err == 0) WSACleanup();
			if(dump) fclose(dump);
			Reply(S_OK);
			return 0;
		case CMD_RUN:
			Reply(m_socket >= 0 ? S_OK : E_FAIL);

			{
				char buff[65536*2];
				int buffsize = 0;
				sockaddr_in from;

				for(unsigned int i = 0; ; i++)
				{
					if(!(i&0xff))
					{
						if(CheckRequest(NULL))
							break;
					}

					int fromlen = sizeof(from);
					int len = recvfrom(m_socket, &buff[buffsize], 65536, 0, (SOCKADDR*)&from, &fromlen);
					if(len <= 0) {Sleep(1); continue;}

					buffsize += len;
					
					if(buffsize >= 65536 || m_len == 0)
					{
						Append((BYTE*)buff, buffsize);
						if(dump) fwrite(buff, buffsize, 1, dump);
						buffsize = 0;
					}
				}
			}
			break;
		}
	}

	ASSERT(0);
	return -1;
}