#include "stdafx.h"
#include "mplayerc.h"
#include "resource.h"
#include "MainFrm.h"
#include <atlbase.h>
#include <atlisapi.h>
#include "WebServer.h"
#include "..\..\zlib\zlib.h"

#define UTF8(str) UTF16To8(TToW(str))
#define UTF8Arg(str) UrlEncode(UTF8(str))

#define CMD_SETPOS "-1"
#define CMD_SETVOLUME "-2"

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

	struct cookie_attribs {CString path, expire, domain;};
	CMap<CString,CString,cookie_attribs,cookie_attribs&> m_cookieattribs;

public:
	CString m_sessid;
	CString m_cmd, m_path, m_ver;
	CMapStringToString m_hdrlines;
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

				int err;
				char c;

				int timeout = 1000;
				
				do
				{
					for(; len > 0 && (err = Receive(&c, 1)) > 0; len--)
					{
						if(c == '\r') continue;
						str += c;
						if(c == '\n' || len == 1)
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

					if(err == SOCKET_ERROR)
						Sleep(1);
				}
				while(err == SOCKET_ERROR && GetLastError() == WSAEWOULDBLOCK
					&& timeout-- > 0); // FIXME: this is just a dirty fix now

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

			m_pWebServer->OnRequest(this, reshdr, resbody);
		}
		else
		{
			reshdr = "HTTP/1.0 400 Bad Request\r\n";
		}

		if(!reshdr.IsEmpty())
		{
			// cookies
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

			reshdr += 
				"Server: MPC WebServer\r\n"
				"Connection: close\r\n"
				"\r\n";

			Send(reshdr, reshdr.GetLength());

			if(m_cmd != _T("HEAD") && reshdr.Find("HTTP/1.0 200 OK") == 0 && !resbody.IsEmpty()) 
			{
				Send(resbody, resbody.GetLength());
			}

			CString connection = _T("close");
			m_hdrlines.Lookup(_T("connection"), connection);

			Clear();

			// TODO
			// if(connection == _T("close"))
				OnClose(0);
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
	m_internalpages[_T("/")] = HandlerIndex;
	m_internalpages[_T("/index.html")] = HandlerIndex;
	m_internalpages[_T("/browser.html")] = HandlerBrowser;
	m_internalpages[_T("/controls.html")] = HandlerControls;
	m_internalpages[_T("/command.html")] = HandlerCommand;
	m_internalpages[_T("/404.html")] = Handler404;

	m_downloads[_T("/default.css")] = IDF_DEFAULT_CSS;
	m_downloads[_T("/vbg.gif")] = IDF_VBR_GIF;
	m_downloads[_T("/vbs.gif")] = IDF_VBS_GIF;
	m_downloads[_T("/sliderbar.gif")] = IDF_SLIDERBAR_GIF;
	m_downloads[_T("/slidergrip.gif")] = IDF_SLIDERGRIP_GIF;
	m_downloads[_T("/sliderback.gif")] = IDF_SLIDERBACK_GIF;
	m_downloads[_T("/1pix.gif")] = IDF_1PIX_GIF;

	m_mimes[".html"] = "text/html";
	m_mimes[".txt"] = "text/plain";
	m_mimes[".css"] = "text/css";
	m_mimes[".gif"] = "image/gif";

	CRegKey key;
	CString str(_T("MIME\\Database\\Content Type"));
	if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, str, KEY_READ))
	{
		TCHAR buff[256];
		DWORD len = countof(buff);
		for(int i = 0; ERROR_SUCCESS == key.EnumKey(i, buff, &len); i++, len = countof(buff))
		{
			CRegKey mime;
			TCHAR ext[64];
			ULONG len = countof(ext);
			if(ERROR_SUCCESS == mime.Open(HKEY_CLASSES_ROOT, str + _T("\\") + buff, KEY_READ)
			&& ERROR_SUCCESS == mime.QueryStringValue(_T("Extension"), ext, &len))
				m_mimes[CStringA(ext).MakeLower()] = CStringA(buff).MakeLower();
		}
	}

	GetModuleFileName(AfxGetInstanceHandle(), str.GetBuffer(MAX_PATH), MAX_PATH);
	str.ReleaseBuffer();
	m_wwwroot = CPath(str);
	m_wwwroot.RemoveFileSpec();
	m_wwwroot.Append(CPath(_T("wwwroot")));

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
	CPath p(pClient->m_path);
	CStringA ext = p.GetExtension().MakeLower();
	CStringA mime;
	m_mimes.Lookup(ext, mime);

	bool fHandled = false;

	hdr = "HTTP/1.0 200 OK\r\n";
/*
	if(m_wwwroot.IsDirectory())
	{
		CString localpath = pClient->m_path;
		localpath.Replace(_T("/"), _T("\\"));
		localpath.TrimLeft('\\');

		CPath path;
		path.Combine(m_wwwroot, CPath(localpath));
		path.Canonicalize();
		if(path.m_strPath.GetLength() > m_wwwroot.m_strPath.GetLength()
		&& path.FileExists())
		{
			if(FILE* f = _tfopen(path, _T("rb")))
			{
				fseek(f, 0, 2);
				char* buff = body.GetBufferSetLength(ftell(f));
				fseek(f, 0, 0);
				int len = fread(buff, 1, body.GetLength(), f);
				fHandled = len == body.GetLength();
				fclose(f);
			}
		}
	}
*/
	RequestHandler rh = NULL;
	if(!fHandled 
	&& m_internalpages.Lookup(pClient->m_path, rh)
	&& (this->*rh)(pClient, hdr, body))
	{
		if(mime.IsEmpty()) mime = "text/html";

		CString redir;
		if(pClient->m_get.Lookup(_T("redir"), redir)
		|| pClient->m_post.Lookup(_T("redir"), redir))
		{
			if(redir.IsEmpty()) redir = '/';

			hdr = 
				"HTTP/1.0 302 Found\r\n"
				"Location: " + CStringA(redir) + "\r\n";
			return;
		}

		fHandled = true;
	}

	UINT resid;
	CStringA res;
	if(!fHandled 
	&& m_downloads.Lookup(pClient->m_path, resid)
	&& LoadResource(resid, res, _T("FILE")))
	{
		if(mime.IsEmpty()) mime = "application/octet-stream";
		memcpy(body.GetBufferSetLength(res.GetLength()), res.GetBuffer(), res.GetLength());
		fHandled = true;
	}

	if(!fHandled)
	{
		if(mime == "text/html")
		{
			hdr = 
				"HTTP/1.0 301 Moved Permanently\r\n"
				"Location: /404.html\r\n";
		}
		else
		{
			hdr = 
				"HTTP/1.0 404 Not Found\r\n";
		}

		return;
	}

	if(mime == "text/html")
	{
		hdr += 
			"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
			"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
			"Pragma: no-cache\r\n";

		CStringA debug;
		if(AfxGetAppSettings().fWebServerPrintDebugInfo)
		{
			debug += "<hr>\r\n";
			CString key, value;
			POSITION pos;
			pos = pClient->m_hdrlines.GetStartPosition();
			while(pos) {pClient->m_hdrlines.GetNextAssoc(pos, key, value); debug += "HEADER[" + key + "] = " + value + "<br>\r\n";}
			debug += "cmd: " + pClient->m_cmd + "<br>\r\n";
			debug += "path: " + pClient->m_path + "<br>\r\n";
			debug += "ver: " + pClient->m_ver + "<br>\r\n";
			pos = pClient->m_get.GetStartPosition();
			while(pos) {pClient->m_get.GetNextAssoc(pos, key, value); debug += "GET[" + key + "] = " + value + "<br>\r\n";}
			pos = pClient->m_post.GetStartPosition();
			while(pos) {pClient->m_post.GetNextAssoc(pos, key, value); debug += "POST[" + key + "] = " + value + "<br>\r\n";}
			pos = pClient->m_cookie.GetStartPosition();
			while(pos) {pClient->m_cookie.GetNextAssoc(pos, key, value); debug += "COOKIE[" + key + "] = " + value + "<br>\r\n";}
			pos = pClient->m_request.GetStartPosition();
			while(pos) {pClient->m_request.GetNextAssoc(pos, key, value); debug += "REQUEST[" + key + "] = " + value + "<br>\r\n";}
		}

		body.Replace("[path]", CStringA(pClient->m_path));
		body.Replace("[indexpath]", "/index.html");
		body.Replace("[commandpath]", "/command.html");
		body.Replace("[browserpath]", "/browser.html");
		body.Replace("[controlspath]", "/controls.html");
		body.Replace("[wmcname]", "wm_command");
		body.Replace("[setposcommand]", CMD_SETPOS);
		body.Replace("[setvolumecommand]", CMD_SETVOLUME);
		body.Replace("[debug]", debug);
		// TODO: add more general tags to replace
	}

	// gzip
	if(AfxGetAppSettings().fWebServerUseCompression)
	do
	{
		CString accept_encoding;
		pClient->m_hdrlines.Lookup(_T("accept-encoding"), accept_encoding);
		accept_encoding.MakeLower();
		CList<CString> sl;
		ExplodeMin(accept_encoding, sl, ',');
		if(!sl.Find(_T("gzip"))) break;;

		CHAR path[MAX_PATH], fn[MAX_PATH];
		if(!GetTempPathA(MAX_PATH, path) || !GetTempFileNameA(path, "gz", 0, fn))
			break;

		gzFile gf = gzopen(fn, "wb9");
		if(!gf || gzwrite(gf, (LPVOID)(LPCSTR)body, body.GetLength()) != body.GetLength())
		{
			if(gf) gzclose(gf);
			DeleteFileA(fn);
			break;
		}
		gzclose(gf);

		FILE* f = fopen(fn, "rb");
		if(!f) {DeleteFileA(fn); break;}
		fseek(f, 0, 2);
		CHAR* s = body.GetBufferSetLength(ftell(f));
		fseek(f, 0, 0);
		int len = fread(s, 1, body.GetLength(), f);
		ASSERT(len == body.GetLength());
		fclose(f);
		DeleteFileA(fn);

		hdr += "Content-Encoding: gzip\r\n";
	}
	while(0);

	CStringA content;
	content.Format(
		"Content-Type: %s\r\n"
		"Content-Length: %d\r\n", 
		mime, body.GetLength());
	hdr += content;
}

//////////////

bool CWebServer::HandlerCommand(CClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	CString arg;
	if(pClient->m_request.Lookup(_T("wm_command"), arg))
	{
		int id = _tcstol(arg, NULL, 10);

		if(id > 0)
		{
			m_pMainFrm->SendMessage(WM_COMMAND, id);
		}
		else
		{
			if(arg == CMD_SETPOS && pClient->m_request.Lookup(_T("position"), arg))
			{
				int h, m, s, ms = 0;
				TCHAR c;
				if(_stscanf(arg, _T("%d%c%d%c%d%c%d"), &h, &c, &m, &c, &s, &c, &ms) >= 5)
				{
					REFERENCE_TIME rtPos = 10000i64*(((h*60+m)*60+s)*1000+ms);
					m_pMainFrm->SeekTo(rtPos);
					for(int retries = 20; retries-- > 0; Sleep(50))
					{
						if(abs((int)((rtPos - m_pMainFrm->GetPos())/10000)) < 100)
							break;
					}
				}
			}
			else if(arg == CMD_SETVOLUME && pClient->m_request.Lookup(_T("volume"), arg))
			{
				int volume = _tcstol(arg, NULL, 10);
				m_pMainFrm->m_wndToolBar.Volume = min(max(volume, 1), 100);
				m_pMainFrm->OnPlayVolume(0);
			}
		}
	}

	CString ref;
	if(!pClient->m_hdrlines.Lookup(_T("referer"), ref))
		return(true);

	hdr = 
		"HTTP/1.0 302 Found\r\n"
		"Location: " + CStringA(ref) + "\r\n";

	return(true);
}

bool CWebServer::HandlerIndex(CClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	CStringA wmcoptions;

	// generate page

	AppSettings& s = AfxGetAppSettings();
	POSITION pos = s.wmcmds.GetHeadPosition();
	while(pos)
	{
		wmcmd& wc = s.wmcmds.GetNext(pos);
		CStringA str;
		str.Format("%d", wc.cmd);
		wmcoptions += "<option value=\"" + str + "\">" 
			+ CStringA(wc.name) + "\r\n";
	}

	LoadHtml(IDR_HTML_INDEX, body);
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

		if(CFileGetStatus(path, fs) && !(fs.m_attribute&CFile::directory))
		{
			// TODO: make a new message for just opening files, this is a bit overkill now...

			CList<CString> cmdln;

			cmdln.AddTail(path);

			CString focus;
			if(pClient->m_get.Lookup(_T("focus"), focus) && !focus.CompareNoCase(_T("no")))
				cmdln.AddTail(_T("/nofocus"));

			int len = 0;
			
			POSITION pos = cmdln.GetHeadPosition();
			while(pos)
			{
				CString& str = cmdln.GetNext(pos);
				len += (str.GetLength()+1)*sizeof(TCHAR);
			}

			CAutoVectorPtr<BYTE> buff;
			if(buff.Allocate(4+len))
			{
				BYTE* p = buff;
				*(DWORD*)p = cmdln.GetCount(); 
				p += sizeof(DWORD);

				POSITION pos = cmdln.GetHeadPosition();
				while(pos)
				{
					CString& str = cmdln.GetNext(pos);
					len = (str.GetLength()+1)*sizeof(TCHAR);
					memcpy(p, (LPCTSTR)str, len);
					p += len;
				}

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

		if(CFileGetStatus(path, fs) && !(fs.m_attribute&CFile::directory))
		{
			CPath p(path);
			p.RemoveFileSpec();
			path = (LPCTSTR)p;
		}
	}

	if(path.Find(_T("://")) >= 0)
		path.Empty();

	if(CFileGetStatus(path, fs) && (fs.m_attribute&CFile::directory)
	|| path.Find(_T("\\")) == 0) // FIXME
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
					"<td><a href=\"[path]?path=" + UTF8Arg(fullpath) + "\">" + UTF8(fd.cFileName) + "</a></td>"
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
					"<td><a href=\"[path]?path=" + UTF8Arg(fullpath) + "\">" + UTF8(fd.cFileName) + "</a></td>"
					"<td><nobr>" + UTF8(type) + "</nobr></td>"
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
	body.Replace("[currentdir]", UTF8(path));
	body.Replace("[currentfiles]", files);

	return(true);
}

bool CWebServer::HandlerControls(CClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	CString path = m_pMainFrm->m_wndPlaylistBar.GetCur();
	CString dir;

	if(!path.IsEmpty())
	{
		CPath p(path);
		p.RemoveFileSpec();
		dir = (LPCTSTR)p;
	}

	OAFilterState fs = m_pMainFrm->GetMediaState();
	CString state;
	state.Format(_T("%d"), fs);
	CString statestring;
	switch(fs)
	{
	case State_Stopped: statestring = _T("Stopped"); break;
	case State_Paused: statestring = _T("Paused"); break;
	case State_Running: statestring = _T("Playing"); break;
	default: statestring = _T("n/a"); break;
	}

	int pos = (int)(m_pMainFrm->GetPos()/10000);
	int dur = (int)(m_pMainFrm->GetDur()/10000);

	CString position, duration;
	position.Format(_T("%d"), pos);
	duration.Format(_T("%d"), dur);

	CString positionstring, durationstring, playbackrate;
//	positionstring.Format(_T("%02d:%02d:%02d.%03d"), (pos/3600000), (pos/60000)%60, (pos/1000)%60, pos%1000);
//	durationstring.Format(_T("%02d:%02d:%02d.%03d"), (dur/3600000), (dur/60000)%60, (dur/1000)%60, dur%1000);
	positionstring.Format(_T("%02d:%02d:%02d"), (pos/3600000), (pos/60000)%60, (pos/1000)%60);
	durationstring.Format(_T("%02d:%02d:%02d"), (dur/3600000), (dur/60000)%60, (dur/1000)%60);
	playbackrate = _T("1"); // TODO

	CString volumelevel, muted;
	volumelevel.Format(_T("%d"), m_pMainFrm->m_wndToolBar.m_volctrl.GetPos());
	muted.Format(_T("%d"), m_pMainFrm->m_wndToolBar.Volume == -10000 ? 1 : 0);

	CString reloadtime(_T("0")); // TODO

	LoadHtml(IDR_HTML_CONTROLS, body);
	body.Replace("[charset]", "UTF-8"); // FIXME: win9x build...
	body.Replace("[filepatharg]", UTF8Arg(path));
	body.Replace("[filepath]", UTF8(path));
	body.Replace("[filedirarg]", UTF8Arg(dir));
	body.Replace("[filedir]", UTF8(dir));
	body.Replace("[state]", UTF8(state));
	body.Replace("[statestring]", UTF8(statestring));
	body.Replace("[position]", UTF8(position));
	body.Replace("[positionstring]", UTF8(positionstring));
	body.Replace("[duration]", UTF8(duration));
	body.Replace("[durationstring]", UTF8(durationstring));
	body.Replace("[volumelevel]", UTF8(volumelevel));
	body.Replace("[muted]", UTF8(muted));
	body.Replace("[playbackrate]", UTF8(playbackrate));
	body.Replace("[reloadtime]", UTF8(reloadtime));

	return(true);
}

bool CWebServer::Handler404(CClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	LoadHtml(IDR_HTML_404, body);

	return(true);
}
