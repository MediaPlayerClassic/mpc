#include "stdafx.h"
#include "mplayerc.h"
#include "resource.h"
#include "MainFrm.h"
#include <atlisapi.h>
#include "WebServer.h"

// TODO: wrap sessions into a class with the possibility of storing some private data
// TODO: auto-set mimes

static bool LoadHtml(UINT resid, CStringA& str)
{
	return LoadResource(resid, str, RT_HTML);
}

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
			ExplodeMin(str, sl, ' ', 3);
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
				while(len-- > 0 && Receive(&c, 1) > 0)
				{
					if(c == '\r') continue;
					str += c;
					if(c == '\n' || len == 0)
					{
						CList<CString> sl;
						Explode(AToT(UrlDecode(TToA(str))), sl, '&'); // FIXME
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
				Explode(AToT(UrlDecode(TToA(Explode(sl.GetTail(), sl, '#', 2)))), sl, '&'); // oh yeah
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

		bool fHtml = reshdr.Find("text/html") >= 0;

		if(!reshdr.IsEmpty())
		{
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

			CStringA debug;
			{
				debug += "<hr>";
				CString key, value;
				POSITION pos;
				pos = m_hdrlines.GetStartPosition();
				while(pos) {m_hdrlines.GetNextAssoc(pos, key, value); debug += "HEADER[" + key + "] = " + value + "<br>\r\n";}
				debug += "cmd: " + m_cmd + "<br>\r\n";
				debug += "path: " + m_path + "<br>\r\n";
				debug += "ver: " + m_ver + "<br>\r\n";
				pos = m_get.GetStartPosition();
				while(pos) {m_get.GetNextAssoc(pos, key, value); debug += "GET[" + key + "] = " + value + "<br>\r\n";}
				pos = m_post.GetStartPosition();
				while(pos) {m_post.GetNextAssoc(pos, key, value); debug += "POST[" + key + "] = " + value + "<br>\r\n";}
				pos = m_cookie.GetStartPosition();
				while(pos) {m_cookie.GetNextAssoc(pos, key, value); debug += "COOKIE[" + key + "] = " + value + "<br>\r\n";}
				pos = m_request.GetStartPosition();
				while(pos) {m_request.GetNextAssoc(pos, key, value); debug += "REQUEST[" + key + "] = " + value + "<br>\r\n";}
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
				if(fHtml)
				{
					resbody.Replace("[path]", CStringA(m_path));
					resbody.Replace("[debug]", debug);
					// TODO: add more general tags to replace

				}

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

CWebServer::CWebServer(CMainFrame* pMainFrm, int nPort)
	: m_pMainFrm(pMainFrm)
	, m_nPort(nPort)
{
	m_internalpages[CString(_T("/default.css"))] = HandlerStyleSheetDefault;
	m_internalpages[CString(_T("/"))] = HandlerIndex;
	m_internalpages[CString(_T("/index.html"))] = HandlerIndex;
	m_internalpages[CString(_T("/browser.html"))] = HandlerBrowser;
	m_internalpages[CString(_T("/404.html"))] = Handler404;

	m_ThreadId = 0;
    m_hThread = ::CreateThread(NULL, 0, StaticThreadProc, (LPVOID)this, 0, &m_ThreadId);
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

DWORD WINAPI CWebServer::StaticThreadProc(LPVOID lpParam)
{
	return ((CWebServer*)lpParam)->ThreadProc();
}

DWORD CWebServer::ThreadProc()
{
	if(!AfxSocketInit(NULL))
		return -1;

	CServerSocket s(this, m_nPort);

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
	RequestHandler rh = NULL;
	
	if(m_internalpages.Lookup(pClient->m_path, rh)
	&& (this->*rh)(pClient, hdr, body))
		return;

	hdr = 
		"HTTP/1.0 301 OK\r\n"
		"Location: /404.html\r\n";
}

//////////////

bool CWebServer::HandlerStyleSheetDefault(CClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	hdr = 
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/css\r\n";

	LoadHtml(IDR_STYLESHEET_DEFAULT, body);

	return(true);
}

bool CWebServer::HandlerIndex(CClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	const char wmcname[] = "wm_command";
	CStringA wmcoptions;

	// process POST

	CString id;
	if(pClient->m_post.Lookup(CString(wmcname), id))
		m_pMainFrm->PostMessage(WM_COMMAND, _tcstol(id, NULL, 10));

	// generate page

	AppSettings& s = AfxGetAppSettings();
	POSITION pos = s.wmcmds.GetHeadPosition();
	while(pos)
	{
		wmcmd& wc = s.wmcmds.GetNext(pos);
		CStringA str;
		str.Format("%d", wc.cmd);
		wmcoptions += "<option value=\"" + str + "\"" 
			+ (str == CStringA(id) ? "selected" : "") + ">" 
			+ CStringA(wc.name) + "\r\n";
	}

	LoadHtml(IDR_HTML_INDEX, body);
	body.Replace("[wmcname]", wmcname);
	body.Replace("[wmcoptions]", wmcoptions);

	return(true);
}

bool CWebServer::HandlerBrowser(CClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	CList<CStringA> rootdrives;
	for(TCHAR drive[] = _T("A:"); drive[0] <= 'Z'; drive[0]++)
		if(GetDriveType(drive) != DRIVE_NO_ROOT_DIR)
			rootdrives.AddTail(CStringA(drive) + '\\');

	// process GET

	CString path;
	CFileStatus fs;
	if(pClient->m_get.Lookup(_T("path"), path))
	{
		path = WToT(UTF8To16(TToA(path)));

		if(CFile::GetStatus(path, fs) && !(fs.m_attribute&CFile::directory))
		{
			// TODO: make a new message for just opening files, this is a bit overkill now...

			int len = (path.GetLength()+1)*sizeof(TCHAR);

			CAutoVectorPtr<BYTE> buff;
			if(buff.Allocate(4+len))
			{
				BYTE* p = buff;
				*(DWORD*)p = 1; 
				p += sizeof(DWORD);
				memcpy(p, path, len);
				p += len;

				COPYDATASTRUCT cds;
				cds.dwData = 0x6ABE51;
				cds.cbData = p - buff;
				cds.lpData = (void*)(BYTE*)buff;
				m_pMainFrm->SendMessage(WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
			}

			CPath p(path);
			p.RemoveFileSpec();
			path = (LPCTSTR)p;
		}
	}
	else
	{
		path = m_pMainFrm->m_wndPlaylistBar.GetCur();
	}

	if(path.Find(_T("://")) >= 0)
		path.Empty();

	if(CFile::GetStatus(path, fs) && (fs.m_attribute&CFile::directory))
	{
		CPath p(path);
		p.Canonicalize();
		p.MakePretty();
		p.AddBackslash();
		path = (LPCTSTR)p;
	}

	CStringA files;

	if(path.IsEmpty())
	{
		POSITION pos = rootdrives.GetHeadPosition();
		while(pos)
		{
			CStringA& drive = rootdrives.GetNext(pos);
			
			files += "<tr>\r\n";
			files += 
				"<td><a href=\"[path]?path=" + UrlEncode(drive) + "\">" + drive + "</a></td>"
				"<td>Directory</td>"
				"<td>&nbsp</td>\r\n"
				"<td>&nbsp</td>";
			files += "</tr>\r\n";
		}

		path = "Root";
	}
	else
	{
		CString parent;

		if(path.GetLength() > 3)
		{
			CPath p(path + "..");
			p.Canonicalize();
			p.AddBackslash();
			parent = (LPCTSTR)p;
		}

		files += "<tr>\r\n";
		files += 
			"<td><a href=\"[path]?path=" + parent + "\">..</a></td>"
			"<td>Directory</td>"
			"<td>&nbsp</td>\r\n"
			"<td>&nbsp</td>";
		files += "</tr>\r\n";

		WIN32_FIND_DATA fd = {0};

		HANDLE hFind = FindFirstFile(path + "*.*", &fd);
		if(hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == '.')
					continue;

				CString fullpath = path + fd.cFileName;

				files += "<tr>\r\n";
				files += 
					"<td><a href=\"[path]?path=" + UrlEncode(UTF16To8(TToW(fullpath))) + "\">" + UTF16To8(TToW(fd.cFileName)) + "</a></td>"
					"<td>Directory</td>"
					"<td>&nbsp</td>\r\n"
					"<td><nobr>" + CStringA(CTime(fd.ftLastWriteTime).Format(_T("%Y.%m.%d %H:%M"))) + "</nobr></td>";
				files += "</tr>\r\n";
			}
			while(FindNextFile(hFind, &fd));
			
			FindClose(hFind);
		}

		hFind = FindFirstFile(path + "*.*", &fd);
		if(hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
					continue;

				CString fullpath = path + fd.cFileName;

				CStringA size;
				size.Format("%I64dK", ((UINT64)fd.nFileSizeHigh<<22)|(fd.nFileSizeLow>>10));

				CString type(_T("&nbsp"));
				LoadType(fullpath, type);

				files += "<tr>\r\n";
				files += 
					"<td><a href=\"[path]?path=" + UrlEncode(UTF16To8(TToW(fullpath))) + "\">" + UTF16To8(TToW(fd.cFileName)) + "</a></td>"
					"<td><nobr>" + UTF16To8(TToW(type)) + "</nobr></td>"
					"<td align=\"right\"><nobr>" + size + "</nobr></td>\r\n"
					"<td><nobr>" + CStringA(CTime(fd.ftLastWriteTime).Format(_T("%Y.%m.%d %H:%M"))) + "</nobr></td>";
				files += "</tr>\r\n";
			}
			while(FindNextFile(hFind, &fd));
			
			FindClose(hFind);
		}
	}

	LoadHtml(IDR_HTML_BROWSER, body);
	body.Replace("[charset]", "UTF-8"); // FIXME: win9x build...
	body.Replace("[currentdir]", TToA(path));
	body.Replace("[currentfiles]", files);

	return(true);
}

bool CWebServer::Handler404(CClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	LoadHtml(IDR_HTML_404, body);

	return(true);
}
