#include "stdafx.h"
#include "mplayerc.h"
#include "resource.h"
#include "MainFrm.h"
#include <atlbase.h>
#include <atlisapi.h>
#include "WebServerSocket.h"
#include "WebClientSocket.h"
#include "WebServer.h"
#include "..\..\zlib\zlib.h"

CAtlMap<CString, CWebServer::RequestHandler, CStringElementTraits<CString> > CWebServer::m_internalpages;
CAtlMap<CString, UINT, CStringElementTraits<CString> > CWebServer::m_downloads;
CAtlMap<CStringA, CStringA, CStringElementTraits<CStringA> > CWebServer::m_mimes;

CWebServer::CWebServer(CMainFrame* pMainFrame, int nPort)
	: m_pMainFrame(pMainFrame)
	, m_nPort(nPort)
{
	if(m_internalpages.IsEmpty())
	{
		m_internalpages[_T("/")] = &CWebClientSocket::OnIndex;
		m_internalpages[_T("/index.html")] = &CWebClientSocket::OnIndex;
		m_internalpages[_T("/browser.html")] = &CWebClientSocket::OnBrowser;
		m_internalpages[_T("/controls.html")] = &CWebClientSocket::OnControls;
		m_internalpages[_T("/command.html")] = &CWebClientSocket::OnCommand;
		m_internalpages[_T("/status.html")] = &CWebClientSocket::OnStatus;
		m_internalpages[_T("/player.html")] = &CWebClientSocket::OnPlayer;
		m_internalpages[_T("/snapshot.jpg")] = &CWebClientSocket::OnSnapShotJpeg;	
		m_internalpages[_T("/404.html")] = &CWebClientSocket::OnError404;
	}

	if(m_downloads.IsEmpty())
	{
		m_downloads[_T("/default.css")] = IDF_DEFAULT_CSS;
		m_downloads[_T("/vbg.gif")] = IDF_VBR_GIF;
		m_downloads[_T("/vbs.gif")] = IDF_VBS_GIF;
		m_downloads[_T("/sliderbar.gif")] = IDF_SLIDERBAR_GIF;
		m_downloads[_T("/slidergrip.gif")] = IDF_SLIDERGRIP_GIF;
		m_downloads[_T("/sliderback.gif")] = IDF_SLIDERBACK_GIF;
		m_downloads[_T("/1pix.gif")] = IDF_1PIX_GIF;
		m_downloads[_T("/headericon.png")] = IDF_HEADERICON_PNG;
		m_downloads[_T("/headerback.png")] = IDF_HEADERBACK_PNG;
		m_downloads[_T("/headerclose.png")] = IDF_HEADERCLOSE_PNG;
		m_downloads[_T("/leftside.png")] = IDF_LEFTSIDE_PNG;
		m_downloads[_T("/rightside.png")] = IDF_RIGHTSIDE_PNG;
		m_downloads[_T("/bottomside.png")] = IDF_BOTTOMSIDE_PNG;
		m_downloads[_T("/leftbottomside.png")] = IDF_LEFTBOTTOMSIDE_PNG;
		m_downloads[_T("/rightbottomside.png")] = IDF_RIGHTBOTTOMSIDE_PNG;
		m_downloads[_T("/seekbarleft.png")] = IDF_SEEKBARLEFT_PNG;
		m_downloads[_T("/seekbarmid.png")] = IDF_SEEKBARMID_PNG;
		m_downloads[_T("/seekbarright.png")] = IDF_SEEKBARRIGHT_PNG;
		m_downloads[_T("/seekbargrip.png")] = IDF_SEEKBARGRIP_PNG;
	}

	if(m_mimes.IsEmpty())
	{
		m_mimes[".html"] = "text/html";
		m_mimes[".txt"] = "text/plain";
		m_mimes[".css"] = "text/css";
		m_mimes[".gif"] = "image/gif";
		m_mimes[".jpeg"] = "image/jpeg";
		m_mimes[".jpg"] = "image/jpeg";
		m_mimes[".png"] = "image/png";
	}

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
	m_webroot = CPath(str);
	m_webroot.RemoveFileSpec();

	CString WebRoot = AfxGetAppSettings().WebRoot;
	WebRoot.Replace('/', '\\');
	CPath p(WebRoot);
	if(!p.IsDirectory()) m_webroot.Append(p);
	else m_webroot = p;
	m_webroot.Canonicalize();
	m_webroot.MakePretty();
	if(!m_webroot.IsDirectory()) m_webroot = CPath();

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

	CWebServerSocket s(this, m_nPort);

	MSG msg;
	while((int)GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

static void PutFileContents(LPCTSTR fn, const CStringA& data)
{
	if(FILE* f = _tfopen(fn, _T("wb")))
	{
		fwrite((LPCSTR)data, 1, data.GetLength(), f);
		fclose(f);
	}
}

void CWebServer::Deploy(CString dir)
{
	CStringA data;
	if(LoadResource(IDR_HTML_INDEX, data, RT_HTML)) PutFileContents(dir + _T("index.html"), data);
	if(LoadResource(IDR_HTML_BROWSER, data, RT_HTML)) PutFileContents(dir + _T("browser.html"), data);
	if(LoadResource(IDR_HTML_CONTROLS, data, RT_HTML)) PutFileContents(dir + _T("controls.html"), data);
	if(LoadResource(IDR_HTML_404, data, RT_HTML)) PutFileContents(dir + _T("404.html"), data);
	if(LoadResource(IDR_HTML_PLAYER, data, RT_HTML)) PutFileContents(dir + _T("player.html"), data);

	POSITION pos = m_downloads.GetStartPosition();
	while(pos)
	{
		CString fn;
		UINT id;
		m_downloads.GetNextAssoc(pos, fn, id);
		if(LoadResource(id, data, _T("FILE")))
			PutFileContents(dir + fn, data);
	}
}

bool CWebServer::LoadPage(UINT resid, CStringA& str, CString path)
{
	if(!path.IsEmpty())
	{
		path.Replace('/', '\\');
		if(path == _T("\\")) path = _T("index.html");
		path.TrimLeft('\\');

		CPath p;
		p.Combine(m_webroot, path);
		p.Canonicalize();
		if(p.m_strPath.GetLength() > m_webroot.m_strPath.GetLength() && p.FileExists())
		{
			if(FILE* f = _tfopen(p, _T("rb")))
			{
				fseek(f, 0, 2);
				char* buff = str.GetBufferSetLength(ftell(f));
				fseek(f, 0, 0);
				int len = fread(buff, 1, str.GetLength(), f);
				fclose(f);
				return len == str.GetLength();
			}
		}
	}

	return LoadResource(resid, str, RT_HTML);
}

void CWebServer::OnAccept(CWebServerSocket* pServer)
{
	CAutoPtr<CWebClientSocket> p(new CWebClientSocket(this, m_pMainFrame));
	if(pServer->Accept(*p))
		m_clients.AddTail(p);
}

void CWebServer::OnClose(CWebClientSocket* pClient)
{
	POSITION pos = m_clients.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		if(m_clients.GetNext(pos) == pClient)
		{
			m_clients.RemoveAt(cur);
			break;
		}
	}
}

void CWebServer::OnRequest(CWebClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	CPath p(pClient->m_path);
	CStringA ext = p.GetExtension().MakeLower();
	CStringA mime;
	m_mimes.Lookup(ext, mime);

	bool fHandled = false;

	hdr = "HTTP/1.0 200 OK\r\n";

	RequestHandler rh = NULL;
	if(!fHandled && m_internalpages.Lookup(pClient->m_path, rh) && (pClient->*rh)(hdr, body, mime))
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

	if(!fHandled && m_webroot.IsDirectory())
	{
		fHandled = LoadPage(0, body, pClient->m_path);
	}

	UINT resid;
	CStringA res;
	if(!fHandled && m_downloads.Lookup(pClient->m_path, resid) && LoadResource(resid, res, _T("FILE")))
	{
		if(mime.IsEmpty()) mime = "application/octet-stream";
		memcpy(body.GetBufferSetLength(res.GetLength()), res.GetBuffer(), res.GetLength());
		fHandled = true;
	}

	if(!fHandled)
	{
		hdr = mime == "text/html"
			? "HTTP/1.0 301 Moved Permanently\r\n" "Location: /404.html\r\n"
			: "HTTP/1.0 404 Not Found\r\n";
		return;
	}

	hdr += 
		"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
		"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
		"Pragma: no-cache\r\n";

	if(mime == "text/html")
	{

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
		if(!GetTempPathA(MAX_PATH, path) || !GetTempFileNameA(path, "mpc_gz", 0, fn))
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
