#pragma once

class CJpegEncoder
{
	static const int ColorComponents = 3;

	FILE* m_f;
	int m_w, m_h;
	BYTE* m_p;

	unsigned int m_bbuff, m_bwidth;
	bool PutByte(BYTE b);
	bool PutBit(int b, int n);
	void Flush();
	int GetBitWidth(short q);

	void WriteSOI();
	void WriteDQT();
	void WriteSOF0();
	void WriteDHT();
	void WriteSOS();
	void WriteEOI();

public:
	CJpegEncoder(LPCTSTR fn, const BYTE* dib);
};
