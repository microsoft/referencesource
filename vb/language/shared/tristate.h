//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

template <class T>
class TriState
{
public:
    TriState() : 
      m_hasValue(false) 
    {
    }

    TriState(T value) : 
        m_hasValue(true), 
        m_value(value) 
    {
    }

    ~TriState() 
    {
    }

    bool HasValue() const 
    {
         return m_hasValue; 
    }

    const T& GetValue() const
    {
        ThrowIfFalse(HasValue());
        return m_value;
    }

    T& GetValue() 
    {
        ThrowIfFalse(HasValue());
        return m_value;
    }

    T GetValueOrDefault( _In_ const T& defaultValue) const
    {
        if ( !HasValue() )
        {
            return defaultValue;
        }
        return m_value;
    }

    void SetValue(T value)
    {
        m_value = value;
        m_hasValue = true;
    }

    void ClearValue()
    {
        ClearValue(T());
    }

    void ClearValue(const T& defaultValue)
    {
        m_hasValue = false;
        m_value = defaultValue;
    }

private:
    bool m_hasValue;
    T m_value;
};
