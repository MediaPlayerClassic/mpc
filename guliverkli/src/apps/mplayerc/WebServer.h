#pragma once

#include <afxsock.h>
#include <atlcoll.h>

class CServerSocket;
class CClientSocket;

class CWebServer
{
	CFrameWnd* m_pMainFrm;

    static DWORD WINAPI ThreadProc(LPVOID lpParam);
    DWORD m_ThreadId;
    HANDLE m_hThread;

	CAutoPtrList<CClientSocket> m_clients;

public:
	CWebServer(CFrameWnd* pMainFrm);
	virtual ~CWebServer();

	void OnAccept(CServerSocket* pServer);
	void OnClose(CClientSocket* pClient);
	void OnRequest(CClientSocket* pClient, CStringA& reshdr, CStringA& resbody);
};
