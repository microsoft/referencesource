//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Misc templates that make exception guarantees.
//
//-------------------------------------------------------------------------------------------------

class GuardBase
{
protected:
    GuardBase() : m_enabled(true), m_hasRun(false)
    {
    }

    virtual void DoAction() = 0;

public:

    void RunAction()
    {
        if ( m_enabled && !m_hasRun )
        {
            m_hasRun = true;
            DoAction();
        }
    }

    bool IsEnabled() const
    {
        return m_enabled;
    }

    void SetIsEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

private:
    // Not copy safe
    GuardBase(const GuardBase&);
    GuardBase& operator=(const GuardBase&);

    bool m_enabled;
    bool m_hasRun;
};

template <typename T>
class ClearPointerGuard : public GuardBase
{
public:
    ClearPointerGuard(T*& ptr) : m_ptr(ptr)
    {
    }

    ~ClearPointerGuard()
    {
        RunAction();
    }

protected:
    virtual __override
    void DoAction()
    {
        m_ptr = NULL;
    }

private:
    T*& m_ptr;
};
