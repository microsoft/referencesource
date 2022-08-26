//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Macros and functions for operating on HRESULTs (error codes)
//
//-------------------------------------------------------------------------------------------------

#pragma once

class HResultInfo
{
public:
    NEW_CTOR_SAFE()

    HResultInfo() 
    {
        Clear();
    }

    HResultInfo(HRESULT hr) 
    {
        Clear();
        Load(hr);
    }

    ~HResultInfo()
    {
        Destroy();
    }

    void Destroy()
    {
        if ( m_hasData )
        {
            ReleaseHrInfo(&m_info);
            Clear();
        }
    }

    void Load(HRESULT hr)
    {
        Destroy();
        GetHrInfo(hr, &m_info);
        m_hasData = true;
    }

    HRINFO& GetHRINFO() 
    {
        return m_info;
    }

private:
    // Do not generate
    HResultInfo(const HResultInfo&);
    HResultInfo& operator=(const HResultInfo&);

    void Clear()
    {
        m_hasData = false;
        ZeroMemory(&m_info, sizeof(m_info));
    }

    bool m_hasData;
    HRINFO m_info;
};
