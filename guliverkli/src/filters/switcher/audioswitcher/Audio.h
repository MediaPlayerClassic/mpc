#pragma once

#include <windows.h>
#include <MMREG.H>
#include <mmsystem.h>
#include <msacm.h>

typedef long (*AudioPointSampler)(void *, void *, long, long, long);
typedef long (*AudioDownSampler)(void *, void *, long *, int, long, long, long);

class AudioStreamResampler 
{
private:
	AudioPointSampler ptsampleRout;
	AudioDownSampler dnsampleRout;
	long samp_frac;
	long accum;
	int holdover;
	long *filter_bank;
	int filter_width;
	bool fHighQuality;

	enum { BUFFER_SIZE=512 };
	BYTE cbuffer[4*BUFFER_SIZE];
	int bps;

public:
	AudioStreamResampler(int bps, long org_rate, long new_rate, bool fHighQuality);
	~AudioStreamResampler();

	long Downsample(void* input, long samplesin, void* output, long samplesout);
};

