#include "stdafx.h"
#include "mplayerc.h"
#include "resource.h"
#include "MainFrm.h"
#include <atlisapi.h>
#include "WebServer.h"

class CServerSocket : public CAsyncSocket
{
	CWebServer* m_pWebServer;

public:
	CServerSocket(CWebServer* pWebServer, int port = 13579) : m_pWebServer(pWebServer) {Create(port); Listen();}
	virtual ~CServerSocket() {}

protected:
	void OnAccept(int nErrorCode)
	{
		if(nErrorCode == 0 && m_pWebServer) 
			m_pWebServer->OnAccept(this);

		__super::OnAccept(nErrorCode);
	}
};

template<> UINT AFXAPI HashKey<CString>(CString key)
{
	UINT nHash = 0;
	for(int i = 0, len = key.GetLength(); i < len; i++)
		nHash = (nHash<<5) + nHash + key[i];
	return nHash;
}

class CClientSocket : public CAsyncSocket
{
	CWebServer* m_pWebServer;

	CString m_hdr;
	CMapStringToString m_hdrlines;

	struct cookie_attribs {CString path, expire, domain;};
	CMap<CString,CString,cookie_attribs,cookie_attribs&> m_cookieattribs;

public:
	CString m_sessid;
	CString m_cmd, m_path, m_ver;
	CMapStringToString m_get, m_post, m_cookie;
	CMapStringToString m_request;

	bool SetCookie(CString name, CString value = _T(""), __time64_t expire = -1, CString path = _T("/"), CString domain = _T(""))
	{
		if(name.IsEmpty())
			return(false);

		if(value.IsEmpty())
		{
			m_cookie.RemoveKey(name);
			return(true);
		}

		m_cookie[name] = value;

		cookie_attribs attribs;
		if(expire >= 0)
		{
			CTime t(expire);
			SYSTEMTIME st;
			t.GetAsSystemTime(st);
			CStringA timestr;
			SystemTimeToHttpDate(st, timestr);
			attribs.expire = timestr;
		}
		attribs.path = path;
		attribs.domain = domain;
		m_cookieattribs[name] = attribs;

		return(true);
	}

private:
	void Clear()
	{
		m_hdr.Empty();
		m_hdrlines.RemoveAll();

		m_cmd.Empty();
		m_path.Empty();
		m_ver.Empty();
		m_get.RemoveAll();
		m_post.RemoveAll();
		m_cookie.RemoveAll();
		m_request.RemoveAll();
	}

	void Header()
	{
		if(m_cmd.IsEmpty())
		{
			if(m_hdr.IsEmpty()) return;

			CList<CString> lines;
			Explode(m_hdr, lines, '\n');
			CString str = lines.RemoveHead();

			CList<CString> sl;
			Explode(str, sl, ' ', 3);
			m_cmd = sl.RemoveHead().MakeUpper();
			m_path = sl.RemoveHead();
			m_ver = sl.RemoveHead().MakeUpper();
			ASSERT(sl.GetCount() == 0);

			POSITION pos = lines.GetHeadPosition();
			while(pos)
			{
				Explode(lines.GetNext(pos), sl, ':', 2);
				if(sl.GetCount() == 2)
					m_hdrlines[sl.GetHead().MakeLower()] = sl.GetTail();
			}
		}

		// remember new cookies
		{
			POSITION pos = m_hdrlines.GetStartPosition();
			while(pos)
			{
				CString key, value;
				m_hdrlines.GetNextAssoc(pos, key, value);
				if(key == _T("cookie"))
				{
					CList<CString> sl;
					Explode(value, sl, ';');
					POSITION pos2 = sl.GetHeadPosition();
					while(pos2)
					{
						CList<CString> sl2;
						Explode(sl.GetNext(pos2), sl2, '=', 2);
						m_cookie[sl2.GetHead()] = sl2.GetCount() == 2 ? sl2.GetTail() : _T("");
					}
				}
			}

			// start new session

			if(!m_cookie.Lookup(_T("MPCSESSIONID"), m_sessid))
			{
				srand(time(NULL));
				m_sessid.Format(_T("%08x"), rand()*0x12345678);
				SetCookie(_T("MPCSESSIONID"), m_sessid);
			}
		}

		CStringA reshdr, resbody;

		if(m_cmd == _T("POST"))
		{
			CString str;
			if(m_hdrlines.Lookup(_T("content-length"), str))
			{
				int len = _tcstol(str, NULL, 10);
				str.Empty();
				char c;
				while(len-- > 0 && Receive(&c, 1))
				{
					if(c == '\r') continue;
					str += c;
					if(c == '\n' || len == 0)
					{
						CList<CString> sl;
						Explode(UrlDecode(str), sl, '&');
						POSITION pos = sl.GetHeadPosition();
						while(pos)
						{
							CList<CString> sl2;
							Explode(sl.GetNext(pos), sl2, '=', 2);
							m_post[sl2.GetHead().MakeLower()] = sl2.GetCount() == 2 ? sl2.GetTail() : _T("");
						}
						str.Empty();
					}
				}

				// FIXME: with IE it will only work if I read +2 bytes (?), btw Receive will just return -1
				Receive(&c, 1);
				Receive(&c, 1);
			}
		}
		
		if(m_cmd == _T("GET") || m_cmd == _T("HEAD") || m_cmd == _T("POST"))
		{
			CList<CString> sl;
			
			Explode(m_path, sl, '?', 2);
			m_path = sl.RemoveHead();

			if(!sl.IsEmpty())
			{
				Explode(UrlDecode(Explode(sl.GetTail(), sl, '#', 2)), sl, '&'); // oh yeah
				POSITION pos = sl.GetHeadPosition();
				while(pos)
				{
					CList<CString> sl2;
					Explode(sl.GetNext(pos), sl2, '=', 2);
					m_get[sl2.GetHead()] = sl2.GetCount() == 2 ? sl2.GetTail() : _T("");
				}
			}

			// m_request <-- m_get+m_post+m_cookie
			{
                CString key, value;
				POSITION pos;
				pos = m_get.GetStartPosition();
				while(pos) {m_get.GetNextAssoc(pos, key, value); m_request[key] = value;}
				pos = m_post.GetStartPosition();
				while(pos) {m_post.GetNextAssoc(pos, key, value); m_request[key] = value;}
				pos = m_cookie.GetStartPosition();
				while(pos) {m_cookie.GetNextAssoc(pos, key, value); m_request[key] = value;}
			}

			reshdr = 
				"HTTP/1.0 200 OK\r\n"
				"Content-Type: text/html\r\n";

			m_pWebServer->OnRequest(this, reshdr, resbody);
		}
		else
		{
			reshdr = "HTTP/1.0 400 Bad Request\r\n";
		}

		if(!reshdr.IsEmpty())
		{
#ifdef DEBUG
			// TMP
			{
				resbody += "<hr>";
				CString key, value;
				POSITION pos;
				pos = m_hdrlines.GetStartPosition();
				while(pos) {m_hdrlines.GetNextAssoc(pos, key, value); resbody += "HEADER[" + key + "] = " + value + "<br>\r\n";}
				resbody += "m_cmd: " + m_cmd + "<br>\r\n";
				resbody += "m_path: " + m_path + "<br>\r\n";
				resbody += "m_ver: " + m_ver + "<br>\r\n";
				pos = m_get.GetStartPosition();
				while(pos) {m_get.GetNextAssoc(pos, key, value); resbody += "GET[" + key + "] = " + value + "<br>\r\n";}
				pos = m_post.GetStartPosition();
				while(pos) {m_post.GetNextAssoc(pos, key, value); resbody += "POST[" + key + "] = " + value + "<br>\r\n";}
				pos = m_cookie.GetStartPosition();
				while(pos) {m_cookie.GetNextAssoc(pos, key, value); resbody += "COOKIE[" + key + "] = " + value + "<br>\r\n";}
				pos = m_request.GetStartPosition();
				while(pos) {m_request.GetNextAssoc(pos, key, value); resbody += "REQUEST[" + key + "] = " + value + "<br>\r\n";}
			}
#endif
			// append cookies to reshdr
			{
				POSITION pos = m_cookie.GetStartPosition();
				while(pos)
				{
					CString key, value;
					m_cookie.GetNextAssoc(pos, key, value);
					reshdr += "Set-Cookie: " + key + "=" + value;
					POSITION pos2 = m_cookieattribs.GetStartPosition();
					while(pos2)
					{
						CString key;
						cookie_attribs value;
						m_cookieattribs.GetNextAssoc(pos2, key, value);
						if(!value.path.IsEmpty()) reshdr += " path=" + value.path;
						if(!value.expire.IsEmpty()) reshdr += " expire=" + value.expire;
						if(!value.domain.IsEmpty()) reshdr += " domain=" + value.domain;
					}
					reshdr += "\r\n";
				}
			}
/*
			// TMP
			{
				CStringA fmt("Content-Length: %d\r\n");
				CStringA str;
				str.Format(fmt, resbody.GetLength());
				reshdr += str;
			}
*/
			reshdr += 
				"Server: MPC WebServer\r\n"
				"Connection: close\r\n"
				"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
				"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
				"Pragma: no-cache\r\n"
				"\r\n";
			Send(reshdr, reshdr.GetLength());

			if(!resbody.IsEmpty() && m_cmd != _T("HEAD"))
			{
				Send(resbody, resbody.GetLength());
			}

			Clear();
		}
	}

protected:
	void OnReceive(int nErrorCode)
	{
		if(nErrorCode == 0)
		{
			char c;
			while(Receive(&c, 1) > 0)
			{
				if(c == '\r') continue;
				else m_hdr += c;

				int len = m_hdr.GetLength();
				if(len >= 2 && m_hdr[len-2] == '\n' && m_hdr[len-1] == '\n')
				{
					Header();
					OnClose(0);
					return;
				}
			}
		}

		CAsyncSocket::OnReceive(nErrorCode);
	}

	void OnClose(int nErrorCode)
	{
		m_pWebServer->OnClose(this);
		CAsyncSocket::OnClose(nErrorCode);
	}


public:
	CClientSocket(CWebServer* pWebServer) : m_pWebServer(pWebServer) {}
	virtual ~CClientSocket() {}
};

////////////

CWebServer::CWebServer(CFrameWnd* pMainFrm)
	: m_pMainFrm(pMainFrm)
{
	m_ThreadId = 0;
    m_hThread = ::CreateThread(NULL, 0, ThreadProc, (LPVOID)this, 0, &m_ThreadId);
}

CWebServer::~CWebServer()
{
    if(m_hThread != NULL)
	{
		PostThreadMessage(m_ThreadId, WM_QUIT, 0, 0);
        WaitForSingleObject(m_hThread, 10000);
        EXECUTE_ASSERT(CloseHandle(m_hThread));
    }
}

DWORD WINAPI CWebServer::ThreadProc(LPVOID lpParam)
{
	if(!AfxSocketInit(NULL))
		return -1;

	CServerSocket s((CWebServer*)lpParam);

	MSG msg;
	while((int)GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

void CWebServer::OnAccept(CServerSocket* pServer)
{
	CAutoPtr<CClientSocket> p(new CClientSocket(this));
	if(pServer->Accept(*p))
		m_clients.AddTail(p);
}

void CWebServer::OnClose(CClientSocket* pClient)
{
	POSITION pos = m_clients.GetHeadPosition();
	while(pos)
	{
		POSITION tmp = pos;
		if(m_clients.GetNext(pos) == pClient)
		{
			m_clients.RemoveAt(tmp);
			break;
		}
	}
}

void CWebServer::OnRequest(CClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	body += 
		"<html>\r\n"
		"<head>\r\n"
		"<title>MPC WebServer</title>\r\n"
		"<style type=\"text/css\"><!--\r\n"
		"--></style>\r\n"
		"</head>\r\n"
		"<body>\r\n";

#ifdef DEBUG
	// TMP
	body += 
		"<form action=\"/\" method=\"POST\">\r\n"
		"<input type=\"text\" value=\"Hello World!\" name=\"textbox\">\r\n"
		"<input type=\"submit\" value=\"Push Me!\" name=\"submitbutton\">\r\n"
		"</form>\r\n";
#endif

	body += 
		"</body>\r\n"
		"</html>\r\n";
}
