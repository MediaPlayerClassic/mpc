#pragma once

[uuid("01A5BBD3-FE71-487C-A2EC-F585918A8724")]
interface IKeyFrameInfo : public IUnknown
{
	STDMETHOD (GetKeyFrameCount) (UINT& nKFs) = 0; // returns S_FALSE when every frame is a keyframe
	STDMETHOD (GetKeyFrames) (const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs /* in, out*/) = 0;
};