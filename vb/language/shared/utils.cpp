#include "StdAfx.h"


HRESULT
GetLastHResultError()
{
    long lErr = GetLastError();
    return lErr ? HRESULT_FROM_WIN32(lErr) : E_OUTOFMEMORY;
}

