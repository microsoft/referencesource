//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class VBRuntimeException : std::exception
{
public:
    VBRuntimeException(const wchar_t * pErrorMessage) : m_pErrorMessage(pErrorMessage)
    {
    }
private:
    const wchar_t * m_pErrorMessage;
};


#if DEBUG
#define Assume(condition, description) VSASSERT(condition, description)
#else
#define Assume(condition, description) {if (!(condition)) { throw VBRuntimeException(description); }}
#endif // DEBUG

#define VBFatal(message) throw VBRuntimeException(message)
#define ThrowIfFalse(condition) Assume((condition), L"The condition should not be false! \nPLEASE OPEN A BUG. THIS WILL CAUSE A FATAL EXCEPTION IN RETAIL BUILD!")
#define ThrowIfFalse2(condition, msg) Assume((condition), WIDE(msg));
#define ThrowIfNull(condition) Assume((condition), L"The condition should not be null! \nPLEASE OPEN A BUG. THIS WILL CAUSE A FATAL EXCEPTION IN RETAIL BUILD!")
#define ThrowIfTrue(condition) Assume(!(condition), L"The condition should not be true! \nPLEASE OPEN A BUG. THIS WILL CAUSE A FATAL EXCEPTION IN RETAIL BUILD!")
