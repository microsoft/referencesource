//-------------------------------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Seh guard
//
//-------------------------------------------------------------------------------------------------

#pragma once

class SehGuard
{
public:
    SehGuard();
    ~SehGuard();

private:
    _se_translator_function m_previous;
};

class SehException : public Exception
{
public:
    SehException(unsigned int code) : 
        Exception(HRESULT_FROM_WIN32(code)),
        m_code(code)
    {

    }

    unsigned int GetExceptionCode() const
    {
        return m_code;
    }

private:
    unsigned int m_code;
};
