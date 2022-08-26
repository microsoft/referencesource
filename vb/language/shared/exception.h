//-------------------------------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Basic Exception class
//
//-------------------------------------------------------------------------------------------------

class Exception : std::exception
{
public:

    HRESULT GetHResult() const
    {
        return m_code;
    }

    static 
    Exception FromHResult(HRESULT hr)
    {
        return Exception(hr);
    }

    static
    Exception FromWin32(unsigned int code)
    {
        return Exception(HRESULT_FROM_WIN32(code));
    }

protected:
    Exception(HRESULT code)
        :m_code(code)
    {

    }

private:
    Exception();

    HRESULT m_code;
};
