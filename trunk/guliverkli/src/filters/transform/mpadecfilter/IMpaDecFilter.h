/* 
 *	Copyright (C) 2003-2004 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

enum SampleFormat {SF_PCM16, SF_PCM24, SF_PCM32, SF_FLOAT32};

[uuid("2067C60F-752F-4EBD-B0B1-4CBC5E00741C")]
interface IMpaDecFilter : public IUnknown
{
	STDMETHOD(SetSampleFormat(SampleFormat sf)) = 0;
	STDMETHOD_(SampleFormat, GetSampleFormat()) = 0;
	STDMETHOD(SetSpeakerConfig(int sc)) = 0; // sign of sc tells if spdif is active
	STDMETHOD_(int, GetSpeakerConfig()) = 0;
	STDMETHOD(SetDynamicRangeControl(bool fDRC)) = 0;
	STDMETHOD_(bool, GetDynamicRangeControl()) = 0;
};
