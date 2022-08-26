//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the page manager for our NorlsAllocator instances
//
//-------------------------------------------------------------------------------------------------

#pragma once

#ifdef DEBUG
#define DEBUGPROTECT
#endif // DEBUG

DECLARE_ENUM(ProtectedEntityFlags)
        Nothing = 0,
        Nametable = 1,
        ParseTree = 2,
        UnusedMemory = 4,
        Other = 8,
END_ENUM(ProtectedEntityFlags)
DECLARE_ENUM_OPERATORS(ProtectedEntityFlags)

// Microsoft this must be defined for each binary as desired.
extern ProtectedEntityFlagsEnum const whatIsProtected;

class PageProtect
{
public:

    static DWORD ForbidWrite(ProtectedEntityFlagsEnum entity, void * p, size_t sz)
    {
        return ToggleWrite(entity, p, sz, PAGE_READONLY);
    }
    static DWORD AllowWrite(ProtectedEntityFlagsEnum entity, void * p, size_t sz)
    {
        return ToggleWrite(entity, p, sz, PAGE_READWRITE);
    }

    static DWORD ToggleWrite(ProtectedEntityFlagsEnum entity, void * p, size_t sz, DWORD level);

    static DWORD ForbidAccess(ProtectedEntityFlagsEnum entity, void *p, size_t sz)
    {
        return ToggleWrite(entity, p, sz, PAGE_NOACCESS);
    }

    static bool IsEntityProtected(ProtectedEntityFlagsEnum entity)
    {
        return entity & StaticWhatIsProtected();
    }

    static ProtectedEntityFlagsEnum StaticWhatIsProtected()
    {
        return ::whatIsProtected;
    };
};


class ProtectionToggleLock
{
public:
    ProtectionToggleLock();
    ~ProtectionToggleLock();
private:
    bool locked;
};

void ComputePagesFromAddress(void * addr, size_t size, size_t * pageFrom, size_t * pageTo);

extern int g_current;

class WriteToggler
{
public:
    template <typename T>
    WriteToggler(ProtectedEntityFlagsEnum entity, T & item)
    {
        this->entity = entity;
        this->addr[0] = &item;
        this->size[0] = sizeof(item);
        onSeparatePages = false;
        this->wasReadOnly [0] = false;
#ifdef DEBUGPROTECT

        this->tsOfMRO[0] = -1;
        this->tsOfMRO[1] = -1;
        this->tsOfMW[0] = -1;
        this->tsOfMW[1] = -1;
#endif

        MakeWriteable();
    }
    template <typename T1, typename T2>
    WriteToggler(ProtectedEntityFlagsEnum entity, T1 & item1, T2 & item2)
    {
        this->entity = entity;
        this->addr[0] = &item1;
        this->size[0] = sizeof(T1);
        this->addr[1] = &item2;
        this->size[1] = sizeof(T2);
#ifdef DEBUGPROTECT

        this->tsOfMRO[0] = -1;
        this->tsOfMRO[1] = -1;
        this->tsOfMW[0] = -1;
        this->tsOfMW[1] = -1;
#endif

        this->wasReadOnly[0] = this->wasReadOnly[1] = false;
        onSeparatePages = OnSeparatePages();
        MakeWriteable();
    }
    ~WriteToggler()
    {
        MakeReadOnly();
    }
private:
    void MakeReadOnly(int index)
    {
        if (wasReadOnly[index])
        {
            PageProtect::ForbidWrite(entity, addr[index], size[index]);
#ifdef DEBUGPROTECT
            tsOfMRO[index] = g_current;
#endif

        }
    }
    void MakeWriteable(int index)
    {
        DWORD oldFlags = PageProtect::AllowWrite(entity, addr[index], size[index]);
#ifdef DEBUGPROTECT

        tsOfMW[index] = g_current;
#endif

        if (oldFlags == PAGE_NOACCESS)
        {
            VSFAIL("about to write to a non-access page.");
            PageProtect::ForbidAccess(entity, addr[index], size[index]);
        }
        else if (oldFlags == PAGE_READONLY)
        {
            wasReadOnly[index] = true;
        }
    }
    void MakeReadOnly()
    {
        MakeReadOnly(0);
        if (onSeparatePages)
        {
            MakeReadOnly(1);
        }
    }
    void MakeWriteable()
    {
        MakeWriteable(0);
        if (onSeparatePages)
        {
            MakeWriteable(1);
        }
    }
    bool OnSeparatePages()
    {
        size_t page1Start, page2Start, page1End, page2End;
        ComputePagesFromAddress(addr[0], size[0], &page1Start, &page1End);
        ComputePagesFromAddress(addr[1], size[1], &page2Start, &page2End);
        return (page1Start != page2Start || page1End != page2End);
    }

    ProtectionToggleLock toggleLock;
    bool wasReadOnly[2];
    bool onSeparatePages;
    void * addr[2];
    size_t size[2];
    ProtectedEntityFlagsEnum entity;
#ifdef DEBUGPROTECT

    int tsOfMW[2];
    int tsOfMRO[2];
#endif
};
