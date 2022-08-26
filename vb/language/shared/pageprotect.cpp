//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Page Heap management.  Ruthlessly stolen from the C# team.  Please notify Microsoft and Microsoft
//  about any changes to this file.  It is likely the change will need to be mirrored in the C#
//  implementation
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#include "stdafx.h"

#define PROTECT

#ifdef DEBUGPROTECT
struct ITEM
{

    void * a;
    size_t sz;
    void * np;
    size_t nsz;
    int ts;
    DWORD level;
    DWORD origOldLevel;
    DWORD oldLevel;
};

static ITEM items[1000];
int g_current = 0;
int g_large = 0;

void dumpItem(void * a, size_t sz, void * np, size_t nsz, DWORD level, DWORD origOld, DWORD oldLevel)
{
    items[g_current].a = a;
    items[g_current].sz = sz;
    items[g_current].np = np;
    items[g_current].nsz = nsz;
    items[g_current].level = level;
    items[g_current].origOldLevel = origOld;
    items[g_current].oldLevel = oldLevel;
    items[g_current].ts = g_large;
    g_current ++;
    g_large ++;
    if (g_current == 1000)
        g_current = 0;
}

#endif // DEBUGPROTECT

ProtectedEntityFlagsEnum const whatIsProtected = ProtectedEntityFlags::Nothing;

void ComputePagesFromAddress(void * addr, size_t size, size_t * pageFrom, size_t * pageTo)
{
    size_t SYSTEM_PAGE_SIZE = GetSystemPageSize();

    size_t lowerByFrom = ((size_t)addr) % SYSTEM_PAGE_SIZE;
    size_t lowerByTo = (((size_t)addr) + size) % SYSTEM_PAGE_SIZE;
    *pageFrom = (size_t) addr - lowerByFrom;
    *pageTo = ((size_t) addr + size) - lowerByTo;
}

DWORD PageProtect::ToggleWrite(ProtectedEntityFlagsEnum entity, void * p, size_t sz, DWORD level)
{
    if (!PageProtect::IsEntityProtected(entity))
        return PAGE_READWRITE;

    ProtectionToggleLock toggleLock;

    size_t SYSTEM_PAGE_SIZE = GetSystemPageSize();

    size_t lowerBy = ((size_t)p) % SYSTEM_PAGE_SIZE;
    size_t nsz = sz + lowerBy;
    nsz = nsz - (nsz % SYSTEM_PAGE_SIZE) + SYSTEM_PAGE_SIZE * ((nsz % SYSTEM_PAGE_SIZE) == 0 ? 0 : 1);
    DWORD oldFlags, origOldFlags;
    void * np = (void*)(((size_t)p) - lowerBy);
#ifdef PROTECT

    BOOL succeeded = VirtualProtect(np, nsz, level, &oldFlags);
    VSASSERT(succeeded, "Invalid");

#ifdef DEBUGPROTECT

    if (level == PAGE_READWRITE)
    {
        BYTE oldValue = *(BYTE*)p;
        *(BYTE*)p = 0;
        *(BYTE*)p = oldValue;
    }
#endif // DEBUGPROTECT

#else // PROTECT
    oldFlags = PAGE_READWRITE;
#endif // !PROTECT

    origOldFlags = oldFlags;
    if (oldFlags & (PAGE_NOACCESS | PAGE_EXECUTE))
    {
        oldFlags = PAGE_NOACCESS;
    }
    else if (oldFlags & (PAGE_READONLY | PAGE_EXECUTE_READ))
    {
        oldFlags = PAGE_READONLY;
    }
    else
    {
        VSASSERT(oldFlags & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY), "Invalid");
        oldFlags = PAGE_READWRITE;
    }
#ifdef DEBUGPROTECT
    dumpItem(p, sz, np, nsz, level, origOldFlags, oldFlags);
#endif

    return oldFlags;
}

CTinyLock memoryProtectionLock;

ProtectionToggleLock::ProtectionToggleLock()
{
    locked = memoryProtectionLock.Acquire();
}

ProtectionToggleLock::~ProtectionToggleLock()
{
    if (locked)
    {
        memoryProtectionLock.Release();
    }
}



