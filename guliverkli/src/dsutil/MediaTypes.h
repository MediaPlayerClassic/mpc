#pragma once

#pragma pack(1)
struct VIH
{
	VIDEOINFOHEADER vih;
	UINT mask[3];
	int size;
	const GUID* subtype;
};
struct VIH2
{
	VIDEOINFOHEADER2 vih;
	UINT mask[3];
	int size;
	const GUID* subtype;
};
#pragma pack()

extern VIH vihs[];
extern VIH2 vih2s[];

extern int VIHSIZE;

extern CString VIH2String(int i), Subtype2String(const GUID& subtype);
extern void CorrectMediaType(AM_MEDIA_TYPE* pmt);
