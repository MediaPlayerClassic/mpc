#pragma once

#include <afxsock.h>
#include <atlcoll.h>

class CServerSocket;
class CClientSocket;
class CMainFrame;

class CWebServer
{
	CMainFrame* m_pMainFrm;
	int m_nPort;

	DWORD ThreadProc();
    static DWORD WINAPI StaticThreadProc(LPVOID lpParam);
    DWORD m_ThreadId;
    HANDLE m_hThread;

	CAutoPtrList<CClientSocket> m_clients;

public:
	CWebServer(CMainFrame* pMainFrm, int nPort = 13579);
	virtual ~CWebServer();

	void OnAccept(CServerSocket* pServer);
	void OnClose(CClientSocket* pClient);
	void OnRequest(CClientSocket* pClient, CStringA& reshdr, CStringA& resbody);
};
