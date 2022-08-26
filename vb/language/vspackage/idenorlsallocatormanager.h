//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Manager for the NorlsAllocator instances 
//
//-------------------------------------------------------------------------------------------------

class IdeNorlsAllocatorManager : public NorlsAllocatorManager
{
public:

    virtual __override 
    void LockAllocator()
    {
        m_AllocatorCriticalSection.Enter();
    }

    virtual __override 
    void UnlockAllocator()
    {
        m_AllocatorCriticalSection.Leave();
    }

private:    
    CriticalSection m_AllocatorCriticalSection;

#if FV_TRACK_MEMORY

    virtual __override 
    void LockNorlsList()
    {
        m_NorlsCriticalSection.Enter();
    }

    virtual __override 
    void UnlockNorlsList()
    {
        m_NorlsCriticalSection.Leave();
    }

private:

    // Critical Section used to serialize access to the lists
    CriticalSection m_NorlsCriticalSection;

#endif


};

