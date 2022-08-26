//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Manager for the NorlsAllocator instances 
//
//-------------------------------------------------------------------------------------------------

extern class NorlsAllocatorManager* g_pvbNorlsManager;



class NorlsAllocatorManager
{
public:
    NorlsAllocatorManager();
    virtual ~NorlsAllocatorManager(); 

    PageHeap& GetPageHeap()
    {
        return m_heap;
    }

    void CleanupPageHeap()
    {
        m_heap.DecommitUnusedPages();
    }

    virtual 
    void LockAllocator()
    {
    }

    virtual 
    void UnlockAllocator()
    {
    }

private:
    PageHeap m_heap;
};



