//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Threading helpers.
//
//-------------------------------------------------------------------------------------------------

class CriticalSection
{
public:
                    CriticalSection();
                    ~CriticalSection();

    void            Enter();
    void            Leave();
    bool            TryEnter();
    void            Destroy();

private:
    // Do not generate
    CriticalSection(const CriticalSection&);
    CriticalSection& operator=(const CriticalSection);

    CRITICAL_SECTION    m_cs;
    bool m_fDestroyed;
};

inline CriticalSection::CriticalSection()
{
    m_fDestroyed = false;
    InitializeCriticalSection(&m_cs);
}

inline CriticalSection::~CriticalSection()
{
    Destroy();
}

inline void CriticalSection::Enter()
{
    EnterCriticalSection(&m_cs);
}

inline void CriticalSection::Leave()
{
    LeaveCriticalSection(&m_cs);
}

inline bool CriticalSection::TryEnter()
{
    return TryEnterCriticalSection(&m_cs);
}

inline void CriticalSection::Destroy()
{
    if (!m_fDestroyed)
    {
        DeleteCriticalSection(&m_cs);
        m_fDestroyed = true;
    }
}

#if DEBUG

//-------------------------------------------------------------------------------------------------
//
// The default CComAutoCriticalSection class incorrectly allows Copy constructors and 
// assignments.  This only leads to bugs so writing the following code will prevent accidental
// copies while compiling in DEBUG mode.  In retail this will default to CComAutoCriticalSection
//
//-------------------------------------------------------------------------------------------------
class SafeCriticalSection : public CComAutoCriticalSection
{
public:
    SafeCriticalSection(){}
private:
    SafeCriticalSection(const SafeCriticalSection&);
    SafeCriticalSection& operator=(const SafeCriticalSection&);
};

#else

typedef CComAutoCriticalSection SafeCriticalSection;

#endif

typedef CComCritSecLock<SafeCriticalSection> SafeCriticalSectionLock;

