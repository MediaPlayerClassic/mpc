#pragma once

#include <afxcoll.h>

class CPlaylistItem
{
	static UINT m_globalid;

public:
	UINT m_id;
	CStringList m_fns;
	CStringList m_subs;
	REFERENCE_TIME m_duration;
	bool m_fInvalid;

public:
	CPlaylistItem();
	virtual ~CPlaylistItem();

	CPlaylistItem(const CPlaylistItem& pli);
	CPlaylistItem& operator = (const CPlaylistItem& pli);

	POSITION FindFile(CString path);
};

class CPlaylist : public CList<CPlaylistItem>
{
protected:
	POSITION m_pos;

public:
	CPlaylist();
	virtual ~CPlaylist();

	void RemoveAll();
	bool RemoveAt(POSITION pos);

	void SortById(), SortByName(), SortByPath(), Randomize();

	POSITION GetPos();
	void SetPos(POSITION pos);
	CPlaylistItem& GetNextWrap(POSITION& pos);
	CPlaylistItem& GetPrevWrap(POSITION& pos);
};
