#pragma once

typedef enum {DIAuto, DIWeave, DIBlend} ditype;

[uuid("A6C61113-B27A-4D69-BDE7-EC71E96866F4")]
interface IMpeg2DecFilter : public IUnknown
{
	STDMETHOD(SetDeinterlaceMethod(ditype di)) = 0;
	STDMETHOD_(ditype, GetDeinterlaceMethod()) = 0;

	// Brightness: -255.0 to 255.0, default 0.0
	// Contrast: 0.0 to 10.0, default 1.0
	// Hue: -180.0 to +180.0, default 0.0
	// Saturation: 0.0 to 10.0, default 1.0

	STDMETHOD(SetBrightness(double bright)) = 0;
	STDMETHOD(SetContrast(double cont)) = 0;
	STDMETHOD(SetHue(double hue)) = 0;
	STDMETHOD(SetSaturation(double sat)) = 0;
	STDMETHOD_(double, GetBrightness()) = 0;
	STDMETHOD_(double, GetContrast()) = 0;
	STDMETHOD_(double, GetHue()) = 0;
	STDMETHOD_(double, GetSaturation()) = 0;

	STDMETHOD(EnableForcedSubtitles(bool fEnable)) = 0;
	STDMETHOD_(bool, IsForcedSubtitlesEnabled()) = 0;

	STDMETHOD(EnablePlanarYUV(bool fEnable)) = 0;
	STDMETHOD_(bool, IsPlanarYUVEnabled()) = 0;
};

