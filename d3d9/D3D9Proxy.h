
#pragma warning (disable: 4996)

#include <Windows.h>
#include "minilib.h"

#include <d3d9.h>

#include "detourxs.h"

IDirect3D9* CreateBaseInterface(UINT SDKVersion);
HRESULT CreateBaseExInterface(UINT SDKVersion, IDirect3D9Ex** ppD3D9);
void InitD3D9Proxy();