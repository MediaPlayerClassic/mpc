#pragma once

//
// IQTVideoSurface
//

[uuid("A6AE36F7-A6F2-4157-AF54-6599857E4E20")]
interface IQTVideoSurface : public IUnknown
{
	STDMETHOD (BeginBlt) (const BITMAP& bm) PURE;
	STDMETHOD (DoBlt) (const BITMAP& bm) PURE;
};
