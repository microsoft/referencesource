//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Wrapper around HCORENUM
//
//-------------------------------------------------------------------------------------------------

#pragma once

class CorEnum
{
public:
    CorEnum( _In_ IMetaDataImport* pImport) :
      m_spImport(pImport),
      m_pHandle(NULL)
    {
        ThrowIfNull(pImport);
    }

    ~CorEnum()
    {
        Close();
    }

    void Close() throw()
    {
        if ( m_pHandle )
        {
            m_spImport->CloseEnum(m_pHandle);
            m_pHandle = NULL;
        }
    }

    HCORENUM* operator&() throw()
    {
        return &m_pHandle;
    }

    operator HCORENUM() const throw()
    {
        return m_pHandle;
    }

private:
    // Do not autogenerate
    CorEnum();
    CorEnum(const CorEnum&);
    CorEnum& operator=(const CorEnum&);

    CComPtr<IMetaDataImport> m_spImport;
    HCORENUM m_pHandle;
};
