#pragma once

#include <afxinet.h>
#include "Playlist.h"

#define ISDb_PROTOCOL_VERSION 1

struct isdb_subtitle
{
	int id, discs, disc_no;
	CStringA name, format, language, iso639_2, nick, email;
	struct isdb_subtitle() {reset();}
	void reset() {id = discs = disc_no = 0; format = language = nick = email = "";}
};

struct isdb_movie
{
	CList<CStringA> titles;
	CList<isdb_subtitle> subs;
	void reset() {titles.RemoveAll(); subs.RemoveAll();}
	void operator = (const struct isdb_movie& m)
	{
		titles.RemoveAll(); 
		titles.AddTail((CList<CStringA>*)&m.titles);
		subs.RemoveAll(); 
		subs.AddTail((CList<isdb_subtitle>*)&m.subs);
	}
};

struct filehash {CString name; UINT64 size, hash;};

extern bool hash(LPCTSTR fn, filehash& fh);
extern void hash(CPlaylist& pl, CList<filehash>& fhs);
extern CStringA makeargs(CPlaylist& pl);
extern bool OpenUrl(CInternetSession& is, CString url, CStringA& str);

