#pragma once

typedef enum ChapterType {
    AtomicChapter = 0, // only contain one element
    SubChapter    = 1, // contain a list of elements
};

#pragma pack(push, 1)
struct ChapterElement {
    WORD Size;				// size of this structure
    BYTE Type;				// see ChapterType
    UINT ChapterId;			// unique identifier for this element
    REFERENCE_TIME rtStart;	// REFERENCE_TIME in 100ns
    REFERENCE_TIME rtStop;	// REFERENCE_TIME in 100ns
};
#pragma pack(pop)

[uuid("8E128709-3DC8-4e49-B632-380FCF496B6D")]
interface IChapterInfo : public IUnknown
{
	#define CHAPTER_BAD_ID   0xFFFFFFFF
	#define CHAPTER_ROOT_ID   0

	//	\param aChapterID is 0 for the top level one
	STDMETHOD_(UINT, GetChapterCount) (UINT aChapterID) = 0;

	//	\param aIndex start from 1 to GetChapterCount( aParentChapterId )
	STDMETHOD_(UINT, GetChapterId) (UINT aParentChapterId, UINT aIndex) = 0;

	STDMETHOD_(UINT, GetChapterCurrentId) () = 0;

	STDMETHOD_(BOOL, GetChapterInfo) (UINT aChapterID, struct ChapterElement* pStructureToFill) = 0;

	//	\param PreferredLanguage Language code as in ISO-639-2 (3 chars)
	//	\param CountryCode       Country code as in internet domains
	STDMETHOD_(BSTR, GetChapterStringInfo) (UINT aChapterID, const CHAR PreferredLanguage[3], const CHAR CountryCode[2]) = 0;
};