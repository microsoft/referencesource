//-------------------------------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// This header file declares FastStr Class. FastStr provides a fast case-insensitive comparison 
// of strings. 
//
//-------------------------------------------------------------------------------------------------
#pragma once

#include <wchar.h>

//-------------------------------------------------------------------------------------------------
//
// FastStr 
// - Provides fast case-insensitive comparisons ( ==, != ) between two strings
// - Does lexicographical ordering ( < )
// - Stores 
//      1) NULL terminating string 
//      2) string length 
//      3) hash value.   
// - Computes hash value of the string
// - Returns NULL terminating string
//
//-------------------------------------------------------------------------------------------------
class FastStr
{
public:

    // Constructor: Initializes class members with NULL value
    FastStr();

    // Constructor: Initializes class members with received string
    FastStr(_In_opt_z_ const wchar_t *);

    // Constructor: Initializes class members with received string and string length
    FastStr(
        _In_opt_z_count_(strLength) const wchar_t * wczStr, 
        size_t strLength); 

    // Copy Constructor: Initializes with the received object
    FastStr(_In_ const FastStr &); 

    // Destructor
    ~FastStr();

    // Assignment Operator: Initializes with received object
    FastStr& operator=(_In_ const FastStr &);

    // Assignment Operator: Initializes class members with received string
    FastStr& operator=(_In_opt_z_ const wchar_t *);    

    // Comparison Operator: Returns 'True' if two strings are same  
    bool operator==(_In_ const FastStr &rhsStr) const;

    // Comparison Operator: Returns 'True' if two strings are not same 
    bool operator!=(_In_ const FastStr &rhsStr) const;

    // Comparison Operator: Returns 'True' if first strings is smaller  
    bool operator<(_In_ const FastStr &rhsStr) const;

    // Comparison Operator: Returns 'True' if first strings is greater  
    bool operator>(_In_ const FastStr &rhsStr) const;

    // Release the object
    virtual void Release();
    
    // Cast Operator: Returns string
    operator const wchar_t *() const 
	{
		return m_wczStr;
	}

protected:

    // String 
    const wchar_t * m_wczStr;

    // Bytes occupied by the m_wczStr excluding terminating NULL
    size_t m_length;

    // Hash value
    unsigned long m_hashCode;

    //Initializes the class members
    virtual void Init(_In_opt_z_ const wchar_t * wczStr);

    // Returns Hash value of string
    static unsigned GetHashCode(
        _In_opt_z_count_(length) const wchar_t * wczStr, 
        size_t length);

    // Computes Hash of string
    static unsigned long ComputeStrHashValue(
        _In_opt_z_ unsigned long * pl, 
        size_t cl, 
        size_t r, 
        bool CaseSensitive);
};
