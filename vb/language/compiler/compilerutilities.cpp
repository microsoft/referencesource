//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Compiler utility functions implementation.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

/*************************************************
Associate an Intrinsic type with a token
*************************************************/
const tokens g_rgtokITypes[] =
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  token,
    #include "Symbols\TypeTables.h"
    #undef DEF_TYPE
};

/*************************************************
Associate a character with the TypeChar
*************************************************/
const WCHAR *s_wszTypeChars[] =
{
    #define DEF_TYPECHAR(x,y,t)  WIDE(y),
    #include "Symbols\TypeTables.h"
    #undef DEF_TYPECHAR
};

/*************************************************
Table mapping Vtypes (t_varString) strings
*************************************************/
const WCHAR *s_wszVOfVtype[] =
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  WIDE(#type),
    #include "Symbols\TypeTables.h"
    #undef DEF_TYPE
};

const bool s_TypeIsNumeric[] =
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  isnumeric,
    #include "Symbols\TypeTables.h"
    #undef DEF_TYPE
};

const bool s_TypeIsIntegral[] =
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  isintegral,
    #include "Symbols\TypeTables.h"
    #undef DEF_TYPE
};

const bool s_TypeIsUnsigned[] =
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  isunsigned,
    #include "Symbols\TypeTables.h"
    #undef DEF_TYPE
};

/*************************************************
Table giving sizes (in bytes) of Vtypes (t_varString)
*************************************************/
const unsigned short s_cbOfVtype[] =
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  size,
    #include "Symbols\TypeTables.h"
    #undef DEF_TYPE
};

/*************************************************
Table maps typechar to Vtypes
*************************************************/
const Vtypes s_typeCharToVtype[] =
{
    #define DEF_TYPECHAR(x,y,t)  t,
    #include "Symbols\TypeTables.h"
    #undef DEF_TYPECHAR
};

const tokens s_OperatorTokenTypes[] =
{
    #define DEF_OPERATOR(op, clsname, token, isunary, isbinary, isconversion)  token,
    #include "Symbols\TypeTables.h"
    #undef DEF_OPERATOR
};

const bool s_OperatorIsUnary[] =
{
    #define DEF_OPERATOR(op, clsname, token, isunary, isbinary, isconversion)  isunary,
    #include "Symbols\TypeTables.h"
    #undef DEF_OPERATOR
};

const bool s_OperatorIsBinary[] =
{
    #define DEF_OPERATOR(op, clsname, token, isunary, isbinary, isconversion)  isbinary,
    #include "Symbols\TypeTables.h"
    #undef DEF_OPERATOR
};

const bool s_OperatorIsConversion[] =
{
    #define DEF_OPERATOR(op, clsname, token, isunary, isbinary, isconversion)  isconversion,
    #include "Symbols\TypeTables.h"
    #undef DEF_OPERATOR
};

// 
STRING * StringOfSymbol(
    Compiler * pCompiler,
    BCSYM * psym)
{
    STRING *pstr;
    WCHAR wsz[32];
    int offset = 0;

    wsz[0] = 0;

    if (!psym)
    {
        wcscpy_s(wsz, _countof(wsz), STRING_CONST(pCompiler, Lambda));
    }
    else
    {
        switch(psym->GetKind())
        {
        case SYM_DllDeclare:
            wcscpy_s(wsz, _countof(wsz), pCompiler->TokenToString(tkDECLARE));
            break;
        case SYM_Class:
            if ( psym->PClass()->IsDelegate())
            {
                wcscpy_s(wsz, _countof(wsz), WIDE("Delegate Class"));
            }
            else if ( psym->PClass()->IsEnum())
            {
                wcscpy_s(wsz, _countof(wsz), pCompiler->TokenToString(tkENUM));
            }
            else if ( psym->PClass()->IsStruct())
            {
                wcscpy_s(wsz, _countof(wsz), pCompiler->TokenToString(tkSTRUCTURE));
            }
            else if ( psym->PClass()->IsStdModule())
            {
                wcscpy_s(wsz, _countof(wsz), pCompiler->TokenToString(tkMODULE));
            }
            else
            {
                wcscpy_s(wsz, _countof(wsz), pCompiler->TokenToString(tkCLASS));
            }
            break;

        case SYM_Interface:
            wcscpy_s(wsz, _countof(wsz), pCompiler->TokenToString(tkINTERFACE));
            break;

        case SYM_Variable:
            if (psym->PVariable()->IsWithEvents())
            {
                STRING *WithEvents = pCompiler->TokenToString(tkWITHEVENTS);
                wcscpy_s(wsz, _countof(wsz), WithEvents);
                offset = (int)StringPool::StringLength(WithEvents);

    #pragma prefast(suppress: 26015 26017, "The length here is bounded")
                wsz[offset++] = L' ';
                wsz[offset] = 0;
            }
            __fallthrough; // fall through

        case SYM_VariableWithValue:
        case SYM_VariableWithArraySizes:
        {
    variable:
            HRESULT hr = ResLoadString( STRID_Variable, wsz + offset, DIM(wsz) - offset);
            VSASSERT( SUCCEEDED( hr ), "Failure getting Variable string");
            break;
        }

        case SYM_MethodDecl:
        case SYM_MethodImpl:
        case SYM_SyntheticMethod:
            wcscpy_s(wsz, _countof(wsz), pCompiler->TokenToString(psym->PProc()->GetType() == NULL ? tkSUB : tkFUNCTION));
            break;

        case SYM_UserDefinedOperator:
            wcscpy_s(wsz, _countof(wsz), pCompiler->TokenToString(tkOPERATOR));
            break;

        case SYM_Property:
            if (psym->PProperty()->CreatedByWithEventsDecl())
            {
                goto variable;
            }
            else
                wcscpy_s(wsz, _countof(wsz), pCompiler->TokenToString(tkPROPERTY));
            break;

        case SYM_EventDecl:
            wcscpy_s(wsz, _countof(wsz), pCompiler->TokenToString(tkEVENT));
            break;

        case SYM_Namespace:
            wcscpy_s(wsz, _countof(wsz), pCompiler->TokenToString(tkNAMESPACE));
            break;

        case SYM_GenericBinding:
        case SYM_GenericTypeBinding:
            return StringOfSymbol(pCompiler, psym->PGenericBinding()->GetGeneric());

        default:
            VSFAIL("Bad kind");
            wsz[0] = 0;
        }
    }

    // 

    wsz[_countof(wsz)-1]=0;  //make sure the loop bellow on wsz would not buffer overrun.
    WCHAR* twsz = wsz;

#pragma prefast(suppress: 26017, "There's an epxlicit introduction of a null terminating character.")
    while ((*++twsz) && *twsz!= L' ' && LowerCase(*twsz) == *twsz) // make sure same vscommon unicode is used
        ;

    if (*twsz == 0 || *twsz == L' ')
    {
        wsz[0] = LowerCase(wsz[0]);
    }

    pstr = pCompiler->AddString(wsz);

    return pstr;
}

STRING * StringOfAccess(
    Compiler * pCompiler,
    ACCESS access)
{
    switch (access)
    {
    case ACCESS_Public:
        return pCompiler->TokenToString(tkPUBLIC);

    case ACCESS_Protected:
        return pCompiler->TokenToString(tkPROTECTED);

    case ACCESS_Friend:
        return pCompiler->TokenToString(tkFRIEND);

    case ACCESS_Private:
        return pCompiler->TokenToString(tkPRIVATE);

    case ACCESS_ProtectedFriend:
        return pCompiler->AddString(WIDE("Protected Friend"));

    case ACCESS_CompilerControlled:
        return pCompiler->AddString(WIDE("Compiler"));

    default:
        VSFAIL("Bad access.");
        return NULL;
    }
}

/********************************************************************
// Wraps keywords and such in '[' and ']' so that it would safe to
// use these special names. If the name passed in is not one of those
// special names, or if some error occures, we just return the same
// string we were passed.
*********************************************************************/
STRING *MakeSafeName
(
    Compiler *pCompiler,  // Compiler
    _In_opt_z_ STRING *pstrName,     // Name to make safe
    bool fAfterDot        // Whether the name is after a dot
)
{
    HRESULT hr = NOERROR;

    // By default just return the string we were passed in
    STRING *pstrReturn = pstrName;

    // If we are passed in a NULL string to begin with, don't do a thing
    if (pstrName)
    {
        tokens IdKeyword = TokenOfString(pstrName);

        if (!fAfterDot && TokenIsReserved(IdKeyword))
        {
            // Wrap the string with '[' and ']'
            pstrReturn = pCompiler->ConcatStrings(WIDE("["), pstrName, WIDE("]"));
        }
    }

    // Everything is cool, just return the new string
    return pstrReturn;
}

/********************************************************************
// Wraps keywords and such in '[' and ']' so that it would safe to
// use these special names. This method takes any Dot qualified
// name, and adds the brackets to any component that needs it.
//
// Note: the final string is added to the string pool, and a pointer
// is returned.
*********************************************************************/
STRING * MakeSafeQualifiedName(
    Compiler * pCompiler,  // Compiler
    _In_z_ STRING * Name  /* Name to make safe */)
{
    if (!Name)
    {
        return NULL;
    }

    ULONG Items = 0;
    STRING **QualifiedName = NULL;
    NorlsAllocator NameStorage(NORLSLOC);
    StringBuffer SafeQualifiedNameBuffer;

    BreakUpQualifiedName(pCompiler, &QualifiedName, &NameStorage, &Items, Name, DOT_DELIMITER);

    for (ULONG i = 0; i < Items; ++i)
    {
        if (i > 0)
        {
            SafeQualifiedNameBuffer.AppendChar(DOT_DELIMITER);
        }

        SafeQualifiedNameBuffer.AppendSTRING(MakeSafeName(pCompiler, QualifiedName[i], false));
    }

    return pCompiler->AddString(&SafeQualifiedNameBuffer);
}

/********************************************************************
// Breaks up a qualified name "in the form of a.b.foo" into
// an array of STRINGs, each element being one component. The caller
// need not worry about freeing up the STRINGs allocated, since a NRA
// is used here.
*********************************************************************/
// 
HRESULT BreakUpQualifiedName(
    Compiler * pCompiler,  // [in] context to bind in.
    _Out_ _Deref_post_cap_(* ulDelimiterCount)STRING * * rgpstr[],  // [out] individual names
    NorlsAllocator * pnra,  // [in] used for allocating memory
    _Out_ ULONG * ulDelimiterCount,  // [out] the size of the rgpstr array
    _In_z_ WCHAR * wscStringToBreakUp,  // [in] the string to breakup
    WCHAR ch)              // [in] delimiter
{
    HRESULT hr = NOERROR;
    size_t cchLength = wcslen(wscStringToBreakUp);   // length of string to parse
    WCHAR *wcsTemp, *wcsRollBack;     // keeps track of the begining of the string
    ULONG ulsubDivisionCount = 0;       // number of components in the string
    WCHAR * wcsCopy = wscStringToBreakUp;           // a temp copy of the string

    *ulDelimiterCount = 0;

    wcsTemp = wcsRollBack = wscStringToBreakUp;

    // Microsoft 9/8/04:  I didn't add any overflow checks in this method, since we calculate string lengths ourselves and know that they'll fit into
    // reasonable numbers.

    // It is OK, to be called with a NULL string, just do nothing
    if (!cchLength)
    {
        return S_OK;
    }

    // skip over leading delimiters
    while (wscStringToBreakUp[0] && (ch == wscStringToBreakUp[0]))
    {
        ++wscStringToBreakUp;
    }

    // Count the number of components in the string
    while (wscStringToBreakUp[0])
    {
        if (ch == wscStringToBreakUp[0])
        {                   // if this is the delimiter, count it
            ++ulsubDivisionCount;
        }

        ++wscStringToBreakUp;
    }

    // If the last character is not the delimiter, add one to total count
#pragma prefast(suppress: 26001, "There is a length check at the beginning of the function. The string is at least 1 character long")
    if ((--wscStringToBreakUp)[0] != ch)
        ++ulsubDivisionCount;

    // Allocate space for that many array elements
    *rgpstr = (WCHAR**) pnra->Alloc((ulsubDivisionCount) * sizeof(STRING *));

    // Allocate one temp string to use for AddString() + the null character
    wcsTemp = wcsRollBack = (WCHAR*) pnra->Alloc((cchLength + 1) * sizeof(WCHAR));

    ulsubDivisionCount = 0;
    // skip over leading delimiters
    while (wcsCopy[0] && (ch == wcsCopy[0]))
    {
        ++wcsCopy;
    }

    // Loop yet another time, looking for delimiters, this time add components to array
    while (wcsCopy[0])
    {
        if (ch == wcsCopy[0])
        {                   // if this is the delimiter, terminate the temp string we are building
            wcsTemp[0] = (WCHAR)'\0';

            // remove strings that look like [abc]

            if (wcslen(wcsRollBack) != 0)
            {
                // Add it to the stringpool (if it is not already there)
                (*rgpstr)[ulsubDivisionCount] = pCompiler->AddString(wcsRollBack);

                // increament found count
                ++ulsubDivisionCount;
            }

            // reuse the same temp string
            wcsTemp = wcsRollBack;
        }
        else
        {                   // no delimiter, just keep coping
            wcsTemp[0] = wcsCopy[0];
            ++wcsTemp;
        }

        ++wcsCopy;
    }

    // If the last character is not the delimiter, add last string found
#pragma prefast(suppress: 26001, "There is a length check at the beginning of the function. The string is at least 1 character long")
    if ((--wcsCopy)[0] != ch)
    {
        wcsTemp[0] = '\0';

        // remove strings that look like [abc]
        if (wcslen(wcsRollBack) != 0)
        {
            // Add it to the stringpool (if it is not already there)
            (*rgpstr)[ulsubDivisionCount] = pCompiler->AddString(wcsRollBack);

            // increament found count
            ++ulsubDivisionCount;
        }
    }

    *ulDelimiterCount = ulsubDivisionCount;
    return hr;
}

//========================================================================
// Returns a concatenation of the namespace and name.
//========================================================================
STRING * ConcatNameSpaceAndName(
    Compiler * pcompiler,
    LPCWSTR pwszNameSpace,
    LPCWSTR pwszName)
{
    // Name can be empty string only when namespace is empty string.  Neither can be NULL.
    VSASSERT(pwszNameSpace && pwszName && !(pwszNameSpace[0] && !pwszName[0]), "Bogus namespace or name.");

    // Ignore the namespace if it's empty.
    if (pwszNameSpace[0] == L'\0')
    {
        return pcompiler->AddString(pwszName);
    }
    else
    {
        return pcompiler->ConcatStrings(pwszNameSpace, L".", pwszName);
    }
}


//========================================================================
// Splits wszTypeName (a possibly qualified typename) into
// its namespace and unqualified typename components.
//
// Accepts NULL wszTypeName (returning NULL ppstrNameSpace and
// ppstrUnqualName).
//
// If non-NULL wszTypeName, can return empty (but not NULL) ppstrNameSpace.
//========================================================================
void SplitTypeName(
    Compiler * pcompiler,
    _In_opt_z_ LPCWSTR wszTypeName,  // IN:  Typename (qualified or unqualified)
    _Deref_out_z_ STRING * * ppstrNameSpace,  // OUT: Namespace
    _Deref_out_z_ STRING * * ppstrUnqualName  /*OUT: Unqualified typename */)
{
    // Default OUT params
    *ppstrNameSpace = STRING_CONST(pcompiler, EmptyString);
    *ppstrUnqualName = STRING_CONST(pcompiler, EmptyString);

    if (wszTypeName == NULL)
    {
        // Done
        return;
    }

    // Find trailing typename
    const WCHAR *pwchLastDot = wcsrchr(wszTypeName, L'.');

    // In case we're passed in a.b..ctor (where ".ctor" is the name we want)
    if (pwchLastDot && (pwchLastDot > wszTypeName) && *(pwchLastDot-1) == L'.')
    {
        pwchLastDot--;
        if (pwchLastDot == wszTypeName)
        {
            pwchLastDot = NULL;
        }
    }

    if (pwchLastDot)
    {
        // The namespace name is from the beginning of the input string through
        // the character preceding the last dot.
        *ppstrNameSpace = pcompiler->AddStringWithLen(wszTypeName, pwchLastDot - wszTypeName);

        // The type name is from the character following the last dot through
        // the end of the string.
        *ppstrUnqualName = pcompiler->AddString(pwchLastDot + 1);
    }
    else
    {
        // Name is unqualified.
        *ppstrUnqualName = pcompiler->AddString(wszTypeName);
        *ppstrNameSpace = STRING_CONST(pcompiler, EmptyString);
    }
}

int DaysToMonth365[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
int DaysToMonth366[13] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

//============================================================================
// Converts the given DateTime into a BSTR.
//============================================================================
BSTR BstrFromDateTime(
    __int64 DateValue,  // binary value to translate into string
    bool ShowCompleteDate  /*boolean indicating we should show the entire date string */)
{
#define DATE_STRING_LEN 48

    int MonthValue, DayValue, YearValue, HourValue, MinuteValue, SecondValue;
    bool fPM = false;

    // n = number of days since 1/1/0001
    int n = (int)(DateValue / 864000000000);

    // y400 = number of whole 400-year periods since 1/1/0001
    int y400 = n / 146097;

    // n = day number within 400-year period
    n -= y400 * 146097;

    // y100 = number of whole 100-year periods within 400-year period
    int y100 = n / 36524;

    // Last 100-year period has an extra day, so decrement result if 4
    if (y100 == 4) y100 = 3;

    // n = day number within 100-year period
    n -= y100 * 36524;

    // y4 = number of whole 4-year periods within 100-year period
    int y4 = n / 1461;

    // n = day number within 4-year period
    n -= y4 * 1461;

    // y1 = number of whole years within 4-year period
    int y1 = n / 365;

    // Last year has an extra day, so decrement result if 4
    if (y1 == 4) y1 = 3;

    YearValue = y400 * 400 + y100 * 100 + y4 * 4 + y1 + 1;

    // n = day number within year
    n -= y1 * 365;

    // Leap year calculation looks different from IsLeapYear since y1, y4,
    // and y100 are relative to year 1, not year 0
    bool leapYear = y1 == 3 && (y4 != 24 || y100 == 3);

    int *days = leapYear? DaysToMonth366: DaysToMonth365;

    // All months have less than 32 days, so n >> 5 is a good conservative
    // estimate for the month
    int m = (n >> 5) + 1;

    // m = 1-based month number
    while (n >= days[m] && m < 12) m++;

    MonthValue =  m;

    // 1-based day-of-month
    DayValue = n - days[m - 1] + 1;

    HourValue = (int)((DateValue / 36000000000) % 24);
    MinuteValue = (int)((DateValue / 600000000) % 60);
    SecondValue = (int)((DateValue / 10000000) % 60);

    if (HourValue >= 12)
    {
        fPM = true;

        if (HourValue > 12)
            HourValue = HourValue - 12;
    }
    else if (HourValue == 0)
    {
        HourValue = 12;
    }

    BSTR ScratchDateString = SysAllocStringLen(NULL, DATE_STRING_LEN);

    IfNullThrow(ScratchDateString);
    *ScratchDateString = '\0';

    bool fShowDate = DayValue != 1 || MonthValue != 1 || YearValue != 1;
    bool fShowTime = HourValue != 12 || MinuteValue != 0 || SecondValue != 0 || fPM;

#pragma warning (push)
#pragma warning (disable:6387) // NULL check for ScratchDateString is above

    if (fShowDate && fShowTime || ShowCompleteDate)
    {
        StringCchPrintfW(ScratchDateString, DATE_STRING_LEN, L"%li/%li/%04li %li:%02li:%02li ",
            MonthValue, DayValue, YearValue,
            HourValue, MinuteValue, SecondValue);

        if (fPM)
        {
            StringCchCatW(ScratchDateString, DATE_STRING_LEN, L"PM");
        }
        else
        {
            StringCchCatW(ScratchDateString, DATE_STRING_LEN, L"AM");
        }
    }
    else if (fShowDate)
    {
        StringCchPrintfW(ScratchDateString, DATE_STRING_LEN, L"%li/%li/%04li",
            MonthValue, DayValue, YearValue);
    }
    else
    {
        StringCchPrintfW(ScratchDateString, DATE_STRING_LEN, L"%li:%02li:%02li ",
            HourValue, MinuteValue, SecondValue);

        if (fPM)
        {
            StringCchCatW(ScratchDateString, DATE_STRING_LEN, L"PM");
        }
        else
        {
            StringCchCatW(ScratchDateString, DATE_STRING_LEN, L"AM");
        }
    }

#pragma warning (pop)

    BSTR DateString = SysAllocString(ScratchDateString);
    IfNullThrow(DateString);
    SysFreeString(ScratchDateString);

    return DateString;
}

//============================================================================
// Converts the given Unicode string into a UTF8 string.
//============================================================================
void ConvertUnicodeToUTF8(
    const WCHAR * pwsz,  // IN:  Argument to encode
    ULONG cch,  // IN:  Length of string (allows embedded NUL chars)
    NorlsAllocator * pnra,  // IN:  Where to allocate UTF8 string from
    BYTE * * ppbUTF8,  // OUT: String encoded as UTF8.
    ULONG * pcbUTF8  /*OUT: Length of encoded UTF8 string in bytes */)
{
    int cchInt = cch;   // Convert because ---- COM+ API takes an int*

    // Get length that UTF8 string will be, alloc buffer, and convert
    *pcbUTF8 = UTF8LengthOfUnicode(pwsz, cchInt);

    if (*pcbUTF8 == 0)
    {
        // Need to handle empty string as special case, or pnra->Alloc(0) will barf
        *ppbUTF8 = NULL;
    }
    else
    {
        *ppbUTF8 = (BYTE *)pnra->Alloc(*pcbUTF8);
        UnicodeToUTF8(pwsz, &cchInt, (PSTR)*ppbUTF8, *pcbUTF8);
    }
}

/*****************************************************************************
 ;StringFromParseTreeName

 Abstract: Build a string from a simple name or qualified name
 Return: the string representing the parse tree for the name
*****************************************************************************/
STRING * StringFromParseTreeName(
                Compiler *pCompiler,
                ParseTree::Name *NameTree  // [in] the parse tree holding the name ( qualified or simple )
)
{
    STRING *BuiltString = pCompiler->AddString( L"" );
    while ( NameTree->IsQualified() )
    {
        BuiltString = pCompiler->ConcatStrings( NameTree->AsQualified()->Qualifier.Name, BuiltString );
        NameTree = NameTree->AsQualified()->Base;
        BuiltString = pCompiler->ConcatStrings( L".", BuiltString );
    }

    if ( NameTree->IsSimple() )
    {
        BuiltString = pCompiler->ConcatStrings( NameTree->AsSimple()->ID.Name, BuiltString );
    }
    else if ( NameTree->Opcode == ParseTree::Name::GlobalNameSpace )
    {
        BuiltString = pCompiler->ConcatStrings( pCompiler->TokenToString(tkGLOBAL), BuiltString );
    }

    return BuiltString;
}

//============================================================================
// ;SeeIfWindowsFormClass
// Determine if this is a Form class, in which case it could have a
// synthetic sub Main
//============================================================================
bool SeeIfWindowsFormClass(
    BCSYM_Class * ClassToVerify,  // [in] the class to verify
    Compiler * pCompiler)
{
    for (BCSYM *CurrentBase = ClassToVerify->GetBaseClass();
         CurrentBase && CurrentBase->IsClass() && !CurrentBase->IsObject();
         CurrentBase = CurrentBase->PClass()->GetBaseClass())
    {
        if (StringPool::IsEqual( CurrentBase->PClass()->GetName(), STRING_CONST(pCompiler, Form)) &&
            StringPool::IsEqual( CurrentBase->PClass()->GetNameSpace(), STRING_CONST(pCompiler, SystemWindowsForms)))
        {
            return true;
        }
    }

    return false;
}

/*****************************************************************************
 ;MapTypeToVType

 Abstract: Determine the vtype of a given ParseTree::Type
   Return: the vtype mapping for a type opcode
*****************************************************************************/
Vtypes MapTypeToVType(
    ParseTree::Type::Opcodes parseTreeIntrinsicType  /*[in] parse tree intrinsic type to map */)
{
    switch ( parseTreeIntrinsicType )
    {
        case ParseTree::Type::Boolean:
            return t_bool;
        case ParseTree::Type::SignedByte:
            return t_i1;
        case ParseTree::Type::Byte:
            return t_ui1;
        case ParseTree::Type::Short:
            return t_i2;
        case ParseTree::Type::UnsignedShort:
            return t_ui2;
        case ParseTree::Type::Integer:
            return t_i4;
        case ParseTree::Type::UnsignedInteger:
            return t_ui4;
        case ParseTree::Type::Long:
            return t_i8;
        case ParseTree::Type::UnsignedLong:
            return t_ui8;
        case ParseTree::Type::Decimal:
            return t_decimal;
        case ParseTree::Type::Single:
            return t_single;
        case ParseTree::Type::Double:
            return t_double;
        case ParseTree::Type::Date:
            return t_date;
        case ParseTree::Type::Char:
            return t_char;
        case ParseTree::Type::String:
            return t_string;
        case ParseTree::Type::Object:
            return t_ref;
        default:
            VSASSERT( false, "Unhandled ParseTree::Type::Opcode for mapping to vtype" );
            return t_UNDEF;
    }
}

/******************************************************************************
*******************************************************************************/
long CountLineBreaksInString(PCWSTR pstrText)
{
    //  Count the number of lines in the buffer to be inserted.
    //
    int cLines = 0;
    if ( pstrText != NULL )
    {
        PCWSTR p = pstrText;
        while ( p != NULL  && *p)
        {
            if (IsLineBreak(*p))
            {
                p = LineBreakAdvance(p);
                cLines++;
            }
            else
            {
                p++;
            }
        }
    }

    return cLines;
}


// Debug-only helper that returns a human-readable string representing the
// file's last-modified time.
#if DEBUG
STRING * DebStrFromFileTime(
    _In_z_ const WCHAR * pwszFile,
    Compiler * pCompiler)
{
    FILETIME filetime;

    if (GetFileModificationTime(pwszFile, &filetime))
    {
        return DebStrFromFileTime(filetime, pCompiler);
    }

    return pCompiler->AddString(L"<unknown-time>");
}
#endif


// Debug-only helper that returns a human-readable string representing the
// given FILETIME structure.
#if DEBUG
STRING *DebStrFromFileTime(const FILETIME &filetime, Compiler *pCompiler)
{
    NorlsAllocator nraTemp(NORLSLOC);
    WCHAR *pwszBuf;
    SYSTEMTIME systemtime;

    if (FileTimeToSystemTime(&filetime, &systemtime))
    {
        int cch = 100;
        pwszBuf = (WCHAR *)nraTemp.Alloc(cch * sizeof(WCHAR));
        // Only display "h:mm:ss.msec" -- ignore year, month, day, as they
        // won't be changing during a debug session.
        swprintf_s(pwszBuf, 100, L"%u:%02u:%02u.%04u",
            systemtime.wHour, systemtime.wMinute, systemtime.wSecond, systemtime.wMilliseconds);
    }
    else
    {
        pwszBuf = L"<unknown-time>";
    }

    return pCompiler->AddString(pwszBuf);
}
#endif

// RoundNumber is essentially stolen from COM+, albeit trimmed somewhat
// Rounds a mantissa in-place
void RoundNumber(
    _Inout_z_ char * chBuffer,  // The mantissa
    int pos,  // This defines the useful part of the mantissa
    int &nDecimal)     // This is directly related to the exponent
{
    unsigned i = 0;

    if (pos >= 0)
    {
        // Point to the first NULL in the useful part of the mantissa, or the character just after the useful part.
        while (i < (unsigned) pos && chBuffer[i] != 0)
        {
            i++;
        }

        // If it's the digit after the useful part, then is it greater than, or equal to, 5?
        if (i == pos && chBuffer[i] >= '5')
        {
            // Change all preceding contiguous 9's to zeros...
            while (i > 0 && chBuffer[i - 1] == '9')
            {
                chBuffer[i-1] = '0';
                i--;
            }

            // ...and then either add one to the character preceding the erstwhile nine 9's
            // (so, for example, 23499999999999995 -> 23500000000000000) ...
            if (i > 0)
            {
                chBuffer[i - 1]++;
            }
            // ... or, if none, then change the first digit to a 1 and increment the exponent
            // because this must have been 0.9999999....  So, for example, we might have
            // had a mantissa of 9999999999999995, and a dec of 0, and now we'll have
            //     a mantissa of 1000000000000000, and a dec of 1.
            else
            {
                nDecimal++;
                chBuffer[0] = '1';
                i = 1;
            }
        }
    }
}


HRESULT R8ToBSTR(
    double doValue,
    BSTR * pbstrValue,
    bool fRound /* = false */)
{
    int nDecimal = 0;       // Value is x.xxxxxxE(nDecimal-1)
    int nSign = 0;          // This holds the sign

    char chBuffer[256];  // This holds the mantissa

    int buflen = 0;
    int nPrecision = DOUBLE_PRECISION;
    int nRealPrecision = nPrecision - (fRound?2:0); // Try limiting the precision, and we'll see if we need to go higher.
    HRESULT hr = S_OK;

    if (0 != _ecvt_s(chBuffer, _countof(chBuffer), doValue, nRealPrecision, &nDecimal, &nSign))
    {
        return E_INVALIDARG;
    }

    buflen = nRealPrecision; // Start thinking about the useful digits.

    if (fRound)
    {
        RoundNumber(chBuffer, nRealPrecision , nDecimal);
    }

    if (nDecimal <= 0 || nDecimal > nRealPrecision)
    {
        // Everything except small positive exponent numbers should be stripped of all trailing zeroes.
        // For example, 2.0E-25, -0.0157, 3.0E+26 do not need trailing zeroes in their mantissa
        while (buflen > 0 && chBuffer[buflen-1] == L'0')
        {
            buflen--;
        }
    }
    else // For example, 3.67, -12000.0, 35.057 -- all greater than one, but not requiring an exponent.
    {
        // Strip off the correct number of zeros.  It would be
        // wrong, for instance, to reduce the mantissa 3000000000000000 to 3 if the number was 300.0.
        // Essentially, all trailing zeroes right of the decimal should be stripped.
        while (buflen > nDecimal && buflen >=1 && chBuffer[buflen-1] == L'0')
        {
            buflen--;
        }
    }

    char chPrintBuffer[64];  // More than adequate to hold a double, since largest possible string involves a mantissa of 17 with 15 leading
                             // zeroes, all preceded by a "-0." (scientific notation numbers will be even smaller).
    chPrintBuffer[0]=NULL;
    if (buflen==0) // The buffer must have been filled with zeros, which are all gone now.  Hence, this was the number 0.
    {
        strcat_s(chPrintBuffer, _countof(chPrintBuffer), "0.0");
    }
    else
    {
        // Set the sign, if any.
        if (nSign != 0)
        {
            strcat_s(chPrintBuffer, _countof(chPrintBuffer), "-");
        }

        if (nDecimal < -1 * nRealPrecision || nDecimal > nRealPrecision) // i.e., we'd want to print this as either x.xxxxE-yyy or x.xxxxE+yyy
        {
            // Print the character before the decimal point.
            strncat_s(chPrintBuffer, _countof(chPrintBuffer), chBuffer,1);
            // If there's anything left in the buffer, print it after a decimal point.
            if (buflen-1 > 0)
            {
                strcat_s(chPrintBuffer, _countof(chPrintBuffer), ".");
                strncat_s(chPrintBuffer, _countof(chPrintBuffer), chBuffer + 1, buflen - 1);
            }
            else
            {
                strcat_s(chPrintBuffer, _countof(chPrintBuffer), ".0"); // Otherwise, just print a .0 to dress it up, and be consistent with the fix for VS264074 below.
            }

            strcat_s(chPrintBuffer, _countof(chPrintBuffer), "E");  // Print the exponent notation.
            if (nDecimal-1 > 0)         // Print a plus sign if the exponent is positive (we'll get it for free below if it's negative)
            {
                strcat_s(chPrintBuffer, _countof(chPrintBuffer), "+");
            }

            // Double is limited to exponent of +/- 308, so eight characters should be more than sufficient.
            char ch[8] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
            sprintf_s(ch,  _countof(ch), "%d", nDecimal-1);      // Translate the exponent... (if negative, it'll have a minus sign for free)
            strcat_s(chPrintBuffer, _countof(chPrintBuffer), ch);           // and concatenate it to the number.
        }
        else if (nDecimal <= 0)                 // Simple negative exponent real number with fewer digits than precision allows for (0.1, 0.000035, etc.)
        {
            strcat_s(chPrintBuffer, _countof(chPrintBuffer), "0.");         // All of them must start with this, since it's a negative exponent
            if (nDecimal != 0)                  // If we need some zeroes before printing the mantissa portion...
            {
                for (int i=0;i<-1*nDecimal;i++) // ... then add 'em in (e.g., if exponent is -3, then need two (i.e., nDecimal) zeroes after the decimal point before mantissa)
                {
                    strcat_s(chPrintBuffer, _countof(chPrintBuffer), "0");
                }
            }
            strncat_s(chPrintBuffer, _countof(chPrintBuffer), chBuffer, buflen);
        }
        else // Simple positive exponent real number with fewer digits than precision allows for (e.g., 3.0, -12.067, etc.)
        {
            strncat_s(chPrintBuffer, _countof(chPrintBuffer),chBuffer,nDecimal);   // nDecimal is defined as the number of characters before the decimal point, so print out that many.
            if (buflen - nDecimal > 0)                  // Anything remaining in the usable buffer goes after the decimal point.
            {
                strcat_s(chPrintBuffer, _countof(chPrintBuffer), ".");
                strncat_s(chPrintBuffer, _countof(chPrintBuffer),chBuffer + nDecimal,buflen - nDecimal);
            }
            else // ... or, if nothing, then add a .0 so that's it's seen as a floating point number.  This addresses VS264074.
            {
                strcat_s(chPrintBuffer, _countof(chPrintBuffer), ".0");
            }
        }
    }

    if (fRound)
    {
        // OK, the number we got:  is it the same as the number we started with?
        double doNewValue = atof(chPrintBuffer);
        if (doNewValue != doValue)
        {
            // Guess we shouldn't have rounded -- those trailing 9's must have been legitimate. Try again...
            return R8ToBSTR(doValue, pbstrValue, false);
        }
    }

    WCHAR wszBuffer[256];
    swprintf_s(wszBuffer, _countof(wszBuffer), L"%S", chPrintBuffer);

    *pbstrValue = SysAllocString(wszBuffer);

    if (!pbstrValue)
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

// Microsoft:  Turning of optimizations is important, because otherwise the compiler will end up
// doing double-double comparisons in the cases where we want float-float. (VS272481)

// Update to the compiler:  Instead of disabling optimizations, just require that we use
// -fp:precise to not hork the (float)dbl == (float)dbl2 comparison
#pragma float_control(push)
#pragma float_control(precise, on)
HRESULT R4ToBSTR(
    float flValue,
    BSTR * pbstrValue,
    bool fRound /* = false */)
{
    int nDecimal = 0;       // Value is x.xxxxxxE(nDecimal-1)
    int nSign = 0;          // This holds the sign
    char chBuffer[256];  // This holds the mantissa

    int buflen = 0;
    int nPrecision = FLOAT_PRECISION;
    int nRealPrecision = nPrecision - (fRound?2:0); // Try limiting the precision, and we'll see if we need to go higher.

    double doOldValue = (double)flValue;
    HRESULT hr = S_OK;

    if (0 != _ecvt_s(chBuffer, _countof(chBuffer), doOldValue, nRealPrecision, &nDecimal, &nSign))
    {
        return E_INVALIDARG;
    }

    buflen = nRealPrecision; // Start thinking about the useful digits.

    if (fRound)
    {
        RoundNumber(chBuffer, nRealPrecision, nDecimal);
    }

    if (nDecimal <= 0 || nDecimal > nRealPrecision)
    {
        // Everything except small positive exponent numbers should be stripped of all trailing zeroes.
        // For example, 2.0E-25, -0.0157, 3.0E+26 do not need trailing zeroes in their mantissa
        while (buflen > 0 && chBuffer[buflen-1] == L'0') // Get rid of extraneous zeros.
        {
            buflen--;
        }
    }
    else // For example, 3.67, -12000.0, 35.057 -- all greater than one, but not requiring an exponent.
    {
        // Strip off the correct number of zeros.  It would be
        // wrong, for instance, to reduce the mantissa 3000000000000000 to 3 if the number was 300.0.
        // Essentially, all trailing zeroes right of the decimal should be stripped.
        while (buflen > nDecimal && buflen >= 1 && chBuffer[buflen-1] == L'0')
        {
            buflen--;
        }
    }

    char chPrintBuffer[32];  // More than adequate to hold a double, since largest possible string involves a mantissa of 9 with 7 leading
                             // zeroes, all preceded by a "-0." (scientific notation numbers will be even smaller).
    chPrintBuffer[0]=NULL;
    if (buflen==0) // The buffer must have been filled with zeros, which are all gone now.  Hence, this was the number 0.
    {
        strcat_s(chPrintBuffer, _countof(chPrintBuffer), "0.0");
    }
    else
    {
        // Set the sign, if any.
        if (nSign != 0)
        {
            strcat_s(chPrintBuffer, _countof(chPrintBuffer), "-");
        }

        if (nDecimal < -1 * nRealPrecision || nDecimal > nRealPrecision) // i.e., we'd want to print this as either x.xxxxE-yyy or x.xxxxE+yyy
        {
            // Print the character before the decimal point.
            strncat_s(chPrintBuffer, _countof(chPrintBuffer),chBuffer,1);
            // If there's anything left in the buffer, print it after a decimal point.
            if (buflen-1 > 0)
            {
                strcat_s(chPrintBuffer, _countof(chPrintBuffer), ".");
                strncat_s(chPrintBuffer, _countof(chPrintBuffer),chBuffer + 1,buflen - 1);
            }
            else
            {
                strcat_s(chPrintBuffer, _countof(chPrintBuffer), ".0");  // Otherwise, just print a .0 to dress it up, and be consistent with the fix for VS264074 below.
            }

            strcat_s(chPrintBuffer, _countof(chPrintBuffer), "E");      // Print the exponent notation.
            if (nDecimal-1 > 0)             // Print a plus sign if the exponent is positive (we'll get it for free below if it's negative)
            {
                strcat_s(chPrintBuffer, _countof(chPrintBuffer), "+");
            }

            // Float is limited to exponent of +/- 38, so eight characters should be more than sufficient.
            char ch[8] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
            sprintf_s(ch, _countof(ch), "%d", nDecimal-1);      // Translate the exponent... (if negative, it'll have a minus sign for free)
            strcat_s(chPrintBuffer, _countof(chPrintBuffer), ch);           // and concatenate it to the number.
        }
        else if (nDecimal <= 0)                 // Simple negative exponent real number with fewer digits than precision allows for (0.1, 0.000035, etc.)
        {
            strcat_s(chPrintBuffer, _countof(chPrintBuffer), "0.");         // All of them must start with this, since it's a negative exponent
            if (nDecimal != 0)                  // If we need some zeroes before printing the mantissa portion...
            {
                for (int i=0;i<-1*nDecimal;i++) // ... then add 'em in (e.g., if exponent is -3, then need two (i.e., nDecimal) zeroes after the decimal point before mantissa)
                {
                    strcat_s(chPrintBuffer, _countof(chPrintBuffer), "0");
                }
            }
            strncat_s(chPrintBuffer, _countof(chPrintBuffer),chBuffer,buflen);
        }
        else // Simple positive exponent real number with fewer digits than precision allows for (e.g., 3.0, -12.067, etc.)
        {
            strncat_s(chPrintBuffer, _countof(chPrintBuffer),chBuffer,nDecimal);   // nDecimal is defined as the number of characters before the decimal point, so print out that many.
            if (buflen - nDecimal > 0)                  // Anything remaining in the usable buffer goes after the decimal point.
            {
                strcat_s(chPrintBuffer, _countof(chPrintBuffer), ".");
                strncat_s(chPrintBuffer, _countof(chPrintBuffer),chBuffer + nDecimal,buflen - nDecimal);
            }
            else // ... or, if nothing, then add a .0 so that's it's seen as a floating point number.  This addresses VS264074.
            {
                strcat_s(chPrintBuffer, _countof(chPrintBuffer), ".0");
            }
        }
    }

    if (fRound)
    {
        // OK, the number we got:  is it the same as the number we started with?
        float flNewValue = (float)atof(chPrintBuffer);
        if (flNewValue != flValue)
        {
            // Guess we shouldn't have rounded -- those trailing 9's must have been legitimate. Try again...
            return R4ToBSTR(flValue, pbstrValue, false);
        }
    }

    WCHAR wszBuffer[256];
    swprintf_s(wszBuffer, _countof(wszBuffer), L"%S", chPrintBuffer);

    *pbstrValue = SysAllocString(wszBuffer);

    if (!pbstrValue)
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}
// Microsoft: End of optimization issues.
#pragma float_control(pop)

static
HRESULT SetVersion(
    IAssemblyName * pAssemblyName,
    DWORD dwPropID,
    USHORT usVerNum)
{
    return pAssemblyName->SetProperty(dwPropID, (LPVOID*) &usVerNum, sizeof(USHORT));
}

static
HRESULT SetOriginator(
    IAssemblyName * pAssemblyName,
    BYTE * rgbOriginator,
    DWORD cbOriginator)
{
    // Converts the 160-byte binary originator to a format that IAssemblyName
    // can understand.
    HRESULT hr = S_FALSE;
    if (rgbOriginator != NULL && cbOriginator != 0)
    {
        BYTE* pbStrongNameToken = NULL;
        ULONG cbStrongNameToken = 0;
        if (SUCCEEDED(hr = LegacyActivationShim::StrongNameTokenFromPublicKey_HRESULT(rgbOriginator, cbOriginator, &pbStrongNameToken, &cbStrongNameToken)))
        {
            hr = pAssemblyName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, (LPVOID*) pbStrongNameToken, cbStrongNameToken);
        }

        if (pbStrongNameToken)
        {
            LegacyActivationShim::StrongNameFreeBuffer(pbStrongNameToken);
        }
    }
    return hr;
}

typedef HRESULT (STDAPICALLTYPE *PFNCREATEASSEMBLYNAMEOBJECT)(
    LPASSEMBLYNAME *ppAssemblyNameObj, 
    LPCWSTR szAssemblyName, 
    DWORD dwFlags, 
    LPVOID pvReserved);

//If any error happens, we just return pstrAssemblyName as pFusionName
STRING * MakeStrongAssemblyName(
    Compiler * pCompiler,
    _In_z_ STRING * pstrAssemblyName,
    _In_z_ STRING * pstrLocale,
    BYTE * pvPublicKey,
    ULONG cbPublicKey,
    BYTE * pvAssemblyHash,
    ULONG cbAssemblyHash,
    USHORT usMajorVersion,
    USHORT usMinorVersion,
    USHORT usBuildNumber,
    USHORT usRevisionNumber,
    DWORD dwFlags)
{
    HRESULT hr = NOERROR;
    STRING * pFusionName = pstrAssemblyName;
    PFNCREATEASSEMBLYNAMEOBJECT pfnCreateAssemblyNameObject = NULL;
    CComPtr<IAssemblyName> srpAssemblyName;

    LPWSTR wszDisplayName = NULL;
    WCHAR wszTemp[100];
    DWORD dwReqCchCount = 0;
    ULONG cbLocale = (pstrLocale ? (ULONG)StringPool::StringLength(pstrLocale) : 0);

    IfFalseGo(pCompiler && pstrAssemblyName && pstrAssemblyName[0],E_FAIL);
#ifndef FEATURE_CORESYSTEM
    HMODULE hFusion = NULL;
    IfFailGo(LegacyActivationShim::LoadLibraryShim(L"fusion.dll",NULL,NULL,&hFusion));
    pfnCreateAssemblyNameObject = (PFNCREATEASSEMBLYNAMEOBJECT) GetProcAddress(hFusion, "CreateAssemblyNameObject");
#else
    pfnCreateAssemblyNameObject = (PFNCREATEASSEMBLYNAMEOBJECT) &CreateAssemblyNameObject;
#endif
    IfFalseGo(pfnCreateAssemblyNameObject,E_FAIL);
    IfFailGo(pfnCreateAssemblyNameObject(&srpAssemblyName, NULL, 0, NULL));
    // Set name information
    // Microsoft 9/8/2004: For this to overflow, the assembly name would have to exceed MAXPATH by a rather
    // substantial amount -- that's not going to happen.  Nevertheless, I'll be paranoid here...
    size_t nNameLen = StringPool::StringLength(pstrAssemblyName);
    IfFalseGo(nNameLen + 1 > nNameLen && 
        VBMath::TryMultiply((nNameLen + 1), sizeof(WCHAR)), 
        E_FAIL);
    IfFalseGo(NOERROR == srpAssemblyName->SetProperty(ASM_NAME_NAME, (LPVOID*)pstrAssemblyName,(DWORD)(StringPool::StringLength(pstrAssemblyName) + 1) * sizeof(STRING)),E_FAIL);
    // Set version information.  Note:  currently fusion reverses the revision and build numbers.
    IfFalseGo(NOERROR == SetVersion(srpAssemblyName, ASM_NAME_MAJOR_VERSION, usMajorVersion),E_FAIL);
    IfFalseGo(NOERROR == SetVersion(srpAssemblyName, ASM_NAME_MINOR_VERSION, usMinorVersion),E_FAIL);
    IfFalseGo(NOERROR == SetVersion(srpAssemblyName, ASM_NAME_BUILD_NUMBER, usBuildNumber),E_FAIL);
    IfFalseGo(NOERROR == SetVersion(srpAssemblyName, ASM_NAME_REVISION_NUMBER, usRevisionNumber),E_FAIL);
    // Set culture information
    if (cbLocale == 0)
    {
        IfFalseGo(NOERROR == srpAssemblyName->SetProperty(ASM_NAME_CULTURE,(LPVOID*) L"\0",(DWORD)sizeof(WCHAR)),E_FAIL);
    }
    else
    {
        IfFalseGo(NOERROR == srpAssemblyName->SetProperty(ASM_NAME_CULTURE,(LPVOID*)pstrLocale,(DWORD)(cbLocale + 1) * sizeof(WCHAR)),E_FAIL);
    }
    // Set public key token information
    if (IsAfPublicKey(dwFlags))
    {
        IfFalseGo(NOERROR == SetOriginator(srpAssemblyName,pvPublicKey,cbPublicKey),E_FAIL);
    }
    else if (pvPublicKey != NULL && cbPublicKey != 0)
    {
        IfFalseGo(NOERROR == srpAssemblyName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, (LPVOID*)pvPublicKey,cbPublicKey),E_FAIL);
    }
    // Set hash value information
    if (pvAssemblyHash != NULL && cbAssemblyHash != 0)
    {
        IfFalseGo(NOERROR == srpAssemblyName->SetProperty(ASM_NAME_HASH_VALUE, (LPVOID*)pvAssemblyHash,cbAssemblyHash),E_FAIL);
    }
    // Call GetDisplayName
    wszTemp[0] = L'\0';
    wszDisplayName = wszTemp;
    hr = srpAssemblyName->GetDisplayName(wszDisplayName,&dwReqCchCount,0);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        // Microsoft 9/8/2004: This one we'll check, since the input is external.
        RawStringBuffer nameBuffer;
    
        nameBuffer.AllocateNonNullCharacterCount(dwReqCchCount);
        VBMath::Convert(&dwReqCchCount, nameBuffer.GetCharacterCount());
        hr = srpAssemblyName->GetDisplayName(wszDisplayName,&dwReqCchCount,0);
        if (hr == NOERROR)
        {
            pFusionName = pCompiler->AddString(nameBuffer.GetData());
        }
    }
    else if (hr == NOERROR)
    {
        pFusionName = pCompiler->AddString(wszDisplayName);
    }

Error:;

#ifndef FEATURE_CORESYSTEM        
    if (hFusion)
    {
        FreeLibrary(hFusion);
    }
#endif
    return pFusionName;
}

//-------------------------------------------------------------------------------------------------
//
// We need to make sure the project-level conditional compilation symbols are created
// before we parse.
//
// ***DANGER*** 
//
// There are code paths in both the BG and IDE thread which access this function.  In particular 
// GetDeclTrees can be called from either thread.  However if we actually hit the inner block in 
// this thread for the BG thread we will crash because we will attempt to take a BG lock from a
// BG thread.  This doesn't appear to happen though because the BG thread will force project
// conditional compilation symbols to be parsed before this point
//
//-------------------------------------------------------------------------------------------------
void EnsureProjectLevelCondCompSymbolsExist(SourceFile * pSourceFile)
{
#if IDE
    if(pSourceFile && 
        pSourceFile->GetProject()->GetProjectLevelCondCompScope() == NULL &&
        pSourceFile->GetProject()->GetCompState() == CS_NoState)
    {
        BackgroundCompilerLock compilerLock(pSourceFile->GetProject());
        if (pSourceFile->GetProject()->GetProjectLevelCondCompScope() == NULL)
        {
            // We don't clear the project level compilation symbols because rather than keep the
            // background compiler locked until finished parsing, we are willing to use
            // stale project level compilation symbols.  If the project level compilation symbols change
            // while we are at NoState, it's ok because we'll eventually start using the correct ones
            // when we get to Declared.
            pSourceFile->GetProject()->CreateProjectLevelCondCompSymbols(NULL);
        }
    }
#endif
}

// Get the executable statements for a method
HRESULT ParseCodeBlock(
    Compiler * pCompiler,
    Text * ptext,
    SourceFile * pSourceFile,
    NorlsAllocator * pnra,
    ErrorTable * perrortable,
    BCSYM_Container * pConditionalCompilationConstants,
    const CodeBlockLocation * pCodeBlock,
    const CodeBlockLocation * pProcBlock,  // passed only when method def is requested (need sig location for codegen)
    ParseTree::Statement::Opcodes MethodBodyKind,
    ParseTree::MethodBodyStatement * * pptree,
    MethodDeclKind methodDeclKind,
    bool alwaysParseXmlDocComments)
{
    HRESULT hr = NOERROR;

    EnsureProjectLevelCondCompSymbolsExist(pSourceFile);

    const WCHAR *wszText = NULL;
    size_t cchText = 0;
    IfFalseGo(pCompiler && ptext, E_UNEXPECTED);

    ptext->GetTextOfRange(pCodeBlock->m_oBegin,pCodeBlock->m_oEnd,&wszText,&cchText);

    // wszText will be NULL if the file has disappeared since we last opened it.
    // Do nothing.
    //
    if (wszText)
    {
        Scanner tsBody(pCompiler,
            wszText,
            cchText,
            0,
            pCodeBlock->m_lBegLine,
            pCodeBlock->m_lBegColumn,
            methodDeclKind);
        
        bool IsXMLDocOn = alwaysParseXmlDocComments;
        CompilerHost *pCompilerHost = NULL;

        if (pSourceFile && pSourceFile->GetProject())
        {
            IsXMLDocOn |= pSourceFile->GetProject()->IsXMLDocCommentsOn();
            pCompilerHost = pSourceFile->GetCompilerHost();
        }

        if (!pCompilerHost)
        {
            pCompilerHost = pCompiler->GetDefaultCompilerHost();
        }

        Parser methodBodyParser(
            pnra,
            pCompiler,
            pCompilerHost,
            IsXMLDocOn,
            pSourceFile ? pSourceFile->GetProject()->GetCompilingLanguageVersion() : LANGUAGE_CURRENT
            );

        BCSYM_Container *pProjectLevelCondCompScope = pSourceFile ? pSourceFile->GetProject()->GetProjectLevelCondCompScope() : NULL;

        IfFailGo(methodBodyParser.ParseMethodBody(&tsBody,
            perrortable,
            pProjectLevelCondCompScope,
            pConditionalCompilationConstants,
            MethodBodyKind,
            pptree));

        // if the method definition is necesary for method signature location, reparse it.
        if (pProcBlock && pptree && *pptree)
        {
            wszText = NULL;
            cchText = 0;
            ptext->GetTextOfRange(pProcBlock->m_oBegin,pProcBlock->m_oEnd,&wszText,&cchText);

            if (wszText && cchText > 0)
            {
                Scanner tsProc(pCompiler,
                    wszText,
                    cchText,
                    0,
                    pProcBlock->m_lBegLine,
                    pProcBlock->m_lBegColumn,
                    methodDeclKind);

                Parser parse(
                    pnra,
                    pCompiler,
                    pCompilerHost,
                    IsXMLDocOn,
                    pSourceFile ? pSourceFile->GetProject()->GetCompilingLanguageVersion() : LANGUAGE_CURRENT
                    );
                ParseTree::StatementList *pStatementList = NULL;
                ParseTree::BlockStatement *context = NULL;
                switch (MethodBodyKind)
                {
                case ParseTree::Statement::ProcedureBody:
                case ParseTree::Statement::FunctionBody:
                case ParseTree::Statement::OperatorBody:
                    context = new(*pnra) ParseTree::TypeStatement;
                    context->Opcode = ParseTree::Statement::Class;

                    parse.ParseOneLine(&tsProc, NULL, context, &pStatementList, 
                                pProjectLevelCondCompScope, pConditionalCompilationConstants); // Dev10 #789970 Use proper conditional complation scope.

                    if ( !pStatementList || !pStatementList->Element || !pStatementList->Element->IsMethodDefinition())
                    {
                        // wrong definition, discard it and do not report a method definition back.
                        // This can be the result of an out of sync buffer due to a file change. See bug 550393 and the linked bug.
                        // Consider to add a much more elaborate check here, i.e names should match. Or, make sure the
                        // file change is detected and decompilation triggered before we reach this code.

                        pStatementList = 0;
                    }
                    break;

                case ParseTree::Statement::PropertyGetBody:
                case ParseTree::Statement::PropertySetBody:
                    context = new(*pnra) ParseTree::PropertyStatement;
                    context->Opcode = ParseTree::Statement::Property;
                    parse.ParseOnePropertyOrEventProcedureDefinition(&tsProc, NULL, context, &pStatementList,
                                pProjectLevelCondCompScope, pConditionalCompilationConstants); // Dev10 #789970 Use proper conditional complation scope.
                    break;

                case ParseTree::Statement::AddHandlerBody:
                case ParseTree::Statement::RemoveHandlerBody:
                case ParseTree::Statement::RaiseEventBody:
                    context = new(*pnra) ParseTree::BlockEventDeclarationStatement;
                    context->Opcode = ParseTree::Statement::BlockEventDeclaration;
                    parse.ParseOnePropertyOrEventProcedureDefinition(&tsProc, NULL, context, &pStatementList,
                                pProjectLevelCondCompScope, pConditionalCompilationConstants); // Dev10 #789970 Use proper conditional complation scope.
                    break;
                }

                if (pStatementList && pStatementList->Element)
                {
                    VSASSERT((*pptree)->Definition == 0, "Unexpected MethodDefintion in MethodBodyStatement");
                    if (!pStatementList->Element->HasSyntaxError)
                    {
                        (*pptree)->Definition = pStatementList->Element->AsMethodDefinition();
                    }
                }

            }

        }
    }
Error:
    return hr;
}

//  Get the Decl Trees for this file.
HRESULT GetDeclTrees(
    Compiler * pCompiler,
    Text * ptext,
    SourceFile * pSourceFile,
    NorlsAllocator * pnra,
    NorlsAllocator * pnraConditionalCompilaton,
    ErrorTable * pErrorTable,
    BCSYM_Container * * ppcontinerConditionalCompilaton,
    LineMarkerTable * LineMarkerTableForConditionals,
    ParseTree::FileBlockStatement * * ppbiltree,
    long startLine,
    bool alwaysParseXmlDocComments)
{
    HRESULT hr = NOERROR;
    VSASSERT( pnra && pnraConditionalCompilaton, "Expecting allocators for parse tree storage" );

    const WCHAR *wszText = NULL;
    const WCHAR *wszTextStart = NULL;
    size_t cchText = 0;
    long initialAbsOffset = 0;
    IfFalseGo(pCompiler && ptext, E_UNEXPECTED);

    EnsureProjectLevelCondCompSymbolsExist(pSourceFile);

    // Get the text.
    if (startLine != 0)
    {
        // First, get the pointer to the start of the text:
        ptext->GetText(&wszTextStart, &cchText);

        // Get the pointer to the desired line:
        ptext->GetText(startLine, &wszText, &cchText);

        // Calculate character position offset:
        if (wszText != NULL && wszTextStart != NULL)
        {
            initialAbsOffset = static_cast<long>(wszText - wszTextStart);
        }
    }
    else
    {
        ptext->GetText(&wszText, &cchText);
    }

    // The text will be NULL if we were unable to open the file.  Do not
    // produce a tree for this case.
    //
    if (wszText)
    {
        Scanner fileTokens(
            pCompiler, 
            wszText, 
            cchText, 
            initialAbsOffset, 
            startLine, 
            0,
            NormalKind); // don't assume await/yield are keywords

        
        bool IsXMLDocOn = alwaysParseXmlDocComments;
        CompilerHost *pCompilerHost = NULL;

        if (pSourceFile && pSourceFile->GetProject())
        {
            IsXMLDocOn |= pSourceFile->GetProject()->IsXMLDocCommentsOn();
            pCompilerHost = pSourceFile->GetCompilerHost();
        }

        if (!pCompilerHost)
        {
            pCompilerHost = pCompiler->GetDefaultCompilerHost();
        }

#if DEBUG
        if (pCompilerHost && ppcontinerConditionalCompilaton && pSourceFile)
        {
#if IDE 
            // Workaround:  Temporarily disable this assert for aspx files until we have a
            // way to parse decl trees without conditional compilation statements
            if (!pSourceFile->HasSourceFileView() || !pSourceFile->GetSourceFileView()->IsAspxHiddenFile())
#endif
            {
                if (cchText > 0)   // the hostable editor can be fast and request parsetree for coloring
                {
                    VSASSERT(pCompilerHost->GetComPlusProject()->GetCompState() >= CS_Declared, "Why isn't mscorlib Declared?");
                }
            }
        }
#endif

        Parser parse(
            pnra,
            pCompiler,
            pCompilerHost,
            IsXMLDocOn,
            pSourceFile ? pSourceFile->GetProject()->GetCompilingLanguageVersion() : LANGUAGE_CURRENT
            );

        IfFailGo(parse.ParseDecls(&fileTokens,
                                  pErrorTable,
                                  pSourceFile,
                                  ppbiltree,
                                  pnraConditionalCompilaton,
                                  pSourceFile ? pSourceFile->GetProject()->GetProjectLevelCondCompScope() : NULL,
                                  ppcontinerConditionalCompilaton,
                                  LineMarkerTableForConditionals));

#if IDE
		// Bug 843894: we need to notify our package that it needs to start XML Schemas service. Otherwise, Xml literals
		// support will not be available until Xml intellisense is invoked twice
        if(parse.GetSeenXmlLiterals())
        {
            if(pSourceFile && !pSourceFile->GetProject()->IsIntellisenseSchemasOpened())
            {
                GetCompilerSharedState()->GetMessages()->FlagXmlSchemas(pSourceFile->GetProject());
            }
        }
#endif
        //
        // Debug stuff.
        //

#if DEBUG

        if (VSFSWITCH(fDumpDeclTrees))
        {
            DebPrintf("//============================================================================\n");
            if (pSourceFile)
            {
                DebPrintf("// Decl trees for %S.\n", pSourceFile->GetFileName());
            }
            else
            {
                DebPrintf("// Decl trees for this SourceFileView object.\n");
            }
            DebPrintf("//============================================================================\n");

            ParseTree::DumpStatement(*ppbiltree, pCompiler);
        }

#endif DEBUG
    }
Error:
    return hr;
}

//============================================================================
// Get the span of the statement including comments
//============================================================================
void GetStatementSpanIncludingComments(
    ParseTree::Statement * pStatement,
    Location * plocStatement)
{
    *plocStatement = pStatement->TextSpan;

    for (ParseTree::CommentList *pComments = pStatement->Comments; pComments; pComments = pComments->Next)
    {
        ParseTree::Comment *pComment = pComments->Element;

        if (pComment && pComment->TextSpan.EndsAfter(plocStatement))
        {
            plocStatement->m_lEndLine = pComment->TextSpan.m_lEndLine;
            plocStatement->m_lEndColumn = pComment->TextSpan.m_lEndColumn;
        }
    }
}

bool HasPunctuator(const ParseTree::PunctuatorLocation &loc)
{
    // There can never be punctuators at offset (0, 0), so a punctuator with
    // that value means that there is no punctuator
    return loc.Line != 0 || loc.Column != 0;
}

bool StartsAfterPunctuator(
    const Location &loc,
    const Location &locBase,
    const ParseTree::PunctuatorLocation &punctuator)
{
    if (HasPunctuator(punctuator))
    {
        Location locPunctuator;
        locPunctuator.SetLocation(locBase.m_lBegLine + punctuator.Line, punctuator.Column);
        return loc.StartsAfter(&locPunctuator);
    }

    return false;
}

bool HasParens(ParseTree::MethodSignatureStatement * pMethodStmt)
{
    return HasPunctuator(pMethodStmt->LeftParen) && HasPunctuator(pMethodStmt->RightParen);
}

bool HasParens(ParseTree::PropertyStatement * pPropertyStmt)
{
    return HasPunctuator(pPropertyStmt->LeftParen) && HasPunctuator(pPropertyStmt->RightParen);
}


//=============================================================================
// Lookup a symbol given a its name, starting hash, and context
//=============================================================================
BCSYM_NamedRoot * LookupSymbol(
    _In_z_ const WCHAR * wszSymbolName,  // [in] Name to lookup, may be fully qualified, cannot be NULL
    CompilerHost * pCompilerHost,  // [in] Cannot be NULL.
    BCSYM_Container * pLookup,  // [in] Hash where to start looking , cannot be NULL.
    BCSYM_Container * pContext,  // [in] Context of the lookup.
    NameFlags LookUpFlags,  // [in] Flags to perform the lookup.
    int iArity,
    bool AllowBadSymbol,
    bool * pfHasTrailingTokens,  // [in] Whether wszSymbolName is allowed to have tokens after the valid name (default = true).
    bool * pfHasParseError)
{
    AssertIfNull(wszSymbolName);
    AssertIfNull(pCompilerHost);

    // pLookup can be NULL if the project doesn't have any source files. The workaround would be
    // to search in the unnamednamespace of mscorlib.dll, and do accesibility checks.
    if (pLookup != NULL && wszSymbolName != NULL)
    {
        Compiler *pCompiler = pLookup->GetCompiler();
        NorlsAllocator nraTemp(NORLSLOC);
        CompilerFile *pFile = pLookup->GetCompilerFile();

        Parser parser(
            &nraTemp,
            pCompiler,
            pCompilerHost,
            false,
            pFile ? pFile->GetProject()->GetCompilingLanguageVersion() : LANGUAGE_CURRENT
            );
        bool HasParseError = false;
        bool HasTrailingTokens = false;

        // Parse the name using the parser.
        ParseTree::Name *pName = parser.ParseName(
            HasParseError,
            wszSymbolName,
            false /* AllowGlobalNameSpace*/,
            true /* Allow generics */,
            &HasTrailingTokens);

        if (pfHasParseError)
        {
            *pfHasParseError = HasParseError;
        }

        if (pfHasTrailingTokens)
        {
            *pfHasTrailingTokens = HasTrailingTokens;
        }

        if (HasParseError || pName == NULL)
        {
            return NULL;
        }
        CompilationCaches *pCompilationCaches = GetCompilerCompilationCaches();


        // Use Semantics to interpret the name
        bool IsBadName = false;
        Location locDummy;
        locDummy.Invalidate();
        BCSYM_NamedRoot *pNamedRoot =
            Semantics::EnsureNamedRoot
            (
                Semantics::InterpretName
                (
                    pName,
                    pLookup->GetHash(),
                    NULL,
                    LookUpFlags | NameSearchIgnoreExtensionMethods ,
                    NULL,
                    NULL,
                    pCompiler,
                    pCompilerHost,
                    pCompilationCaches,
                    NULL,
                    IsBadName,
                    NULL,
                    NULL,
                    iArity
                )
            );
        // Return any symbol from InterpretQualifiedName that is not bad
        // (ignore IsBadName flags, which for example is set for inaccessible members)
        if (pNamedRoot && (AllowBadSymbol || !pNamedRoot->IsBad()))
        {
            return pNamedRoot;
        }
    }

    return NULL;
}

#if IDE 
// 





void WaitUntilDesiredState(
    Compiler * pCompiler,
    CompilationState cs,
    WaitStatePumpingEnum ShouldPump, 
    WaitStateCancelEnum CanCancel)
{
    if (pCompiler)
    {
        AllProjectsIterator projIterator(pCompiler);
        CompilerProject* pCompilerProject = NULL;

        while (pCompilerProject = projIterator.Next())
        {
            while (true)
            {
                if (pCompilerProject->GetCompState() >= cs)
                {
                    break;
                }
                if (CS_Declared == cs)
                {
                    pCompilerProject->WaitUntilDeclaredState(ShouldPump, CanCancel);
                }
                else if (CS_Bound == cs)
                {
                    pCompilerProject->WaitUntilBindableState(ShouldPump, CanCancel);
                }
                else
                {
                    pCompilerProject->WaitUntilCompiledState(ShouldPump, CanCancel);
                }
            }
        }
    }
}

void GetProjectName(
    IVsHierarchy * pProjectHierarchy,
    VSITEMID itemid,
    StringBuffer * psb)
{
    if (pProjectHierarchy && psb)
    {
        VARIANT var;
        VariantInit(&var);
        psb->Clear();
        if (SUCCEEDED(pProjectHierarchy->GetProperty(itemid,VSHPROPID_Name,&var)) && V_BSTR(&var))
        {
            psb->AppendString(V_BSTR(&var));
        }
        VariantClear(&var);
    }
}

bool IsLinkedFile(
    IVsHierarchy * pProjectHierarchy,
    VSITEMID itemid)
{
    bool LinkFile = false;
    if (pProjectHierarchy && itemid && itemid != VSITEMID_NIL)
    {
        VARIANT var;
        VariantInit(&var);
        if (SUCCEEDED(pProjectHierarchy->GetProperty(itemid,VSHPROPID_IsLinkFile,&var)) && V_VT(&var) == VT_BOOL)
        {
            LinkFile = V_BOOL(&var) == VARIANT_TRUE ? true : false;
        }
        VariantClear(&var);
    }
    return LinkFile;
}

STRING * GetProjectNameForTestHooks(CompilerProject * pProject)
{
    AssertIfNull(pProject);

    if (pProject->IsMetaData())
    {
        WCHAR wszSplitFileName[_MAX_FNAME];
        WCHAR wszSplitExtension[_MAX_EXT];

        _wsplitpath_s(pProject->GetFileName(),
                      NULL, 0,
                      NULL, 0,
                      wszSplitFileName, _countof(wszSplitFileName),
                      wszSplitExtension, _countof(wszSplitExtension));

        return pProject->GetCompiler()->ConcatStrings(wszSplitFileName, wszSplitExtension);
    }
    else
    {
        return pProject->GetFileName();
    }
}

HRESULT GetProjectAndFileUsingDTE(
    const WCHAR * wszFile,
    CompilerProject * * ppCompilerProject,
    SourceFile * * ppSourceFile)
{
    VB_ENTRY();
    VerifyInPtr(wszFile);

    VerifyOutPtr(ppCompilerProject);
    VerifyOutPtr(ppSourceFile);

    *ppCompilerProject  = NULL;
    *ppSourceFile    = NULL;

    // Using DTE Object, find the project that contains the file. 
    CComPtr<_DTE> spDTEObject = NULL;
    CComPtr<_Solution> spSolution = NULL;
    CComPtr<ProjectItem> spProjItem = NULL;
    CComPtr<CFileCodeModel> spFileCodeModel = NULL;

    // Get DTE object
    IfFailGo(IDEHelpers::GetApplicationObject((DTE **)&spDTEObject));
    IfNullGo(spDTEObject);

    // Get Solution
    IfFailGo(spDTEObject->get_Solution((Solution**)&spSolution));
    IfNullGo(spSolution);

    // Get project containing file
    IfFailGo(spSolution->FindProjectItem((BSTR)wszFile, &spProjItem));
    IfNullGo(spProjItem);

    // Get SourceFile and CompilerProject using File Code Model 
    IfFailGo(spProjItem->get_FileCodeModel((FileCodeModel **)&spFileCodeModel));
    IfNullGo(spFileCodeModel);

    *ppSourceFile = spFileCodeModel->GetSourceFile();
    if (*ppSourceFile)
    {
        *ppCompilerProject = (*ppSourceFile)->GetProject();
    }

    if (*ppSourceFile && *ppCompilerProject)
    {
        hr = S_OK; 
    }
    else
    {
        hr = E_FAIL;
        *ppCompilerProject  = NULL;
        *ppSourceFile    = NULL;
    }

Error:;
    VB_EXIT();
}

HRESULT GetProjectAndFile(
    const WCHAR * wszProject,
    const WCHAR * wszFile,
    CompilerProject * * ppCompilerProject,
    SourceFile * * ppSourceFile)
{

    VerifyInPtr(wszProject);
    VerifyInPtr(wszFile);

    VerifyOutPtr(ppCompilerProject);
    VerifyOutPtr(ppSourceFile);

    *ppCompilerProject  = NULL;
    *ppSourceFile    = NULL;

    HRESULT hr = E_FAIL;

    Compiler *pCompiler = (Compiler *)GetCompilerPackage();
    if (!pCompiler)
    {
        hr = E_UNEXPECTED;
        goto RetryUsingDTE;
    }

    STRING *pstrProject = pCompiler->LookupStringWithLen(wszProject, wcslen(wszProject), false);
    if (!pstrProject)
    {
        hr = E_INVALIDARG;
        goto RetryUsingDTE;
    }

    {

        CompilerProject *pProject = NULL;
        AllProjectsIterator iter(pCompiler);

        while (pProject = iter.Next())
        {
            if (!pProject->IsMetaData() && 
                StringPool::IsEqual(GetProjectNameForTestHooks(pProject), pstrProject))
            {
                break;
            }
        }

        if (!pProject)
        {
            hr = E_INVALIDARG;
            goto RetryUsingDTE;
        }

        STRING *pstrFileName = pCompiler->LookupStringWithLen(wszFile, wcslen(wszFile), false);
        if (!pstrFileName)
        {
            hr = E_INVALIDARG;
            goto RetryUsingDTE;
        }

        SourceFile *pSourceFile = pProject->FindSourceFileByName(pstrFileName);
        if (!pSourceFile)
        {
            iter.Reset();

            CompilerProject *pTempProject = NULL;

            // For a Venus aspx page, the project name will come from the hierarcy
            while (pTempProject = iter.Next())
            {
                CComBSTR bstrProjectName;

                if (SUCCEEDED(COBLibraryHelpers::GetProjectName(pTempProject, true, &bstrProjectName)) &&
                    CompareNoCase(bstrProjectName, pstrProject) == 0)
                {
                    // There can be multiple projects with the same name
                    SourceFile *pTempSourceFile = pTempProject->FindSourceFileByName(pstrFileName);

                    // We are only looking for aspx pages here
                    if (pTempSourceFile &&
                        pTempSourceFile->HasSourceFileView() &&
                        pTempSourceFile->GetSourceFileView()->IsAspxHiddenFile())
                    {
                        pSourceFile = pTempSourceFile;
                        pProject = pTempProject;
                        break;
                    }
                }
            }

            if (!pSourceFile)
            {
                hr = E_INVALIDARG;
                goto RetryUsingDTE;
            }
        }

        // Set Values
        *ppCompilerProject = pProject;
        *ppSourceFile      = pSourceFile;

        hr = S_OK;
    }
RetryUsingDTE:

    // Since Web Site projects don't have specific project name so let's try to get 
    // the compiler project and source file using DTE object and file name
    if (hr != S_OK &&
        SUCCEEDED(GetProjectAndFileUsingDTE(wszFile, ppCompilerProject, ppSourceFile)))
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT GetProject(
    _In_ const WCHAR * wszProject,
    _Deref_out_ CompilerProject * * ppCompilerProject)
{

    VerifyInPtr(wszProject);
    VerifyOutPtr(ppCompilerProject);

    *ppCompilerProject  = NULL;

    HRESULT hr = E_FAIL;

    Compiler *pCompiler = (Compiler *)GetCompilerPackage();
    if (pCompiler)
    {
        STRING *pstrProject = pCompiler->LookupStringWithLen(wszProject, wcslen(wszProject), false);
        if (pstrProject)
        {
            CompilerProject *pProject = NULL;
            AllProjectsIterator iter(pCompiler);

            while (pProject = iter.Next())
            {
                if (!pProject->IsMetaData() && 
                    StringPool::IsEqual(GetProjectNameForTestHooks(pProject), pstrProject))
                {
                    *ppCompilerProject = pProject;
                    hr = S_OK;
                    break;
                }
            }
        }
    }

    return hr;
}


/// <summary>
/// Finds source file by name. Return NULL if not found
/// </summary>
/// <param name="pCompiler">Compiler instance to use.</param>
/// <param name="pstrFileName"></param>
SourceFile *FindSourceFile(Compiler *pCompiler, _In_z_ STRING *pstrFileName)
{
    IfNullThrow(pCompiler);
    IfNullThrow(pstrFileName);

    WCHAR wszSplitFileName[_MAX_FNAME];
    _wsplitpath_s(pstrFileName, NULL, 0, NULL, 0, wszSplitFileName, _countof(wszSplitFileName), NULL, 0);

    AllSourceFilesIterator iter(pCompiler);
    SourceFile *pSourceFile = NULL;
    while (pSourceFile = iter.Next())
    {
        // See if the source file ends with the name we are looking for
        WCHAR wszSplitFileName2[_MAX_FNAME];
        _wsplitpath_s(pSourceFile->GetFileName(), NULL, 0, NULL, 0, wszSplitFileName2, _countof(wszSplitFileName2), NULL, 0);
        if (!CompareFilenames(wszSplitFileName, wszSplitFileName2))
        {
            break;
        }
    }
    return pSourceFile;
}



//=============================================================================
// Given a hierarchy and file name, finds the SourceFile, SourceFileView, and
// the containing project
//=============================================================================
bool
LookupFile
(
    Compiler *pCompiler,                        // [in]  cannot be NULL
    IVsHierarchy *pIVsHierarchy,                // [in]  Hierarchy containing the file, can be NULL
    _In_opt_z_ STRING *pstrFileName,            // [in]  File name, can be NULL
    CompilerProject **ppCompilerProject,        // [out] The project containing the file, can be NULL
    SourceFile **ppSourceFile,                  // [out] SourceFile of the given file, can be NULL
    SourceFileView **ppSourceFileView           // [out] SourceFileView of the given file, can be NULL
)
{
    VSASSERT(pCompiler, "Argument pCompiler cannot be NULL!");

    if (!pstrFileName)
    {
        return false;
    }

    CompilerProject *pCompilerProject = NULL;
    SourceFileView *pSourceFileView = NULL;
    SourceFile *pSourceFile = NULL;
    bool FoundFile = false;

    if (pIVsHierarchy)
    {
        CComPtr<IVsHierarchy> spHierarchy(pIVsHierarchy);

        AllProjectsIterator projectIterator(pCompiler);

        CompilerProject* pTempProject = NULL;
        //There is no backward pointer to previous compiler project

        while (pTempProject = projectIterator.Next())
        {
            // In the aspx case, it is possible for multiple CompilerProjects (one per aspx file) to
            // have the same hierarchy, so we look in each CompilerProject that has the given hierarchy.
            if (spHierarchy.IsEqualObject(pTempProject->GetHierarchy()))
            {
                SourceFileIterator fileIterator(pTempProject);
                SourceFile *pTempSourceFile = NULL;

                while (pTempSourceFile = fileIterator.Next())
                {
                    if (StringPool::IsEqual(pstrFileName, pTempSourceFile->GetFileName()))
                    {
                        pSourceFile = pTempSourceFile;
                        pSourceFileView = pTempSourceFile->GetSourceFileView();
                        FoundFile = true;
                        break;
                    }
                }

                if (FoundFile)
                {
                    pCompilerProject = pTempProject;
                    break;
                }
            }
        }
    }

    if (!FoundFile && !pCompilerProject && GetCompilerPackage())
    {
        // This is the vbnotepad case (no project system)
        DynamicArray<SourceFileView *> daSourceFileView;
        GetCompilerPackage()->GetSourceFileViews(&daSourceFileView);
        for (ULONG i = 0; i < daSourceFileView.Count(); i++)
        {
            if (StringPool::IsEqual(pstrFileName, daSourceFileView.Element(i)->GetFileName()))
            {
                pSourceFileView = daSourceFileView.Element(i);
                FoundFile = true;
                break;
            }
        }
    }

    if (FoundFile)
    {
        if (ppCompilerProject)
        {
            *ppCompilerProject = pCompilerProject;
        }

        if (ppSourceFile)
        {
            *ppSourceFile = pSourceFile;
        }

        if (ppSourceFileView)
        {
            *ppSourceFileView = pSourceFileView;
        }
    }

    return FoundFile;
}

//=============================================================================
// Get the width and height needed to contain the text
//=============================================================================
void
GetTextDimensions
(
    HWND hwnd,              // [in]  Window handle, cannot be NULL
    HFONT hfont,            // [in]  Text font, cannot be NULL
    _In_z_ WCHAR *wszText,         // [in]  The text, cannot be NULL
    long lWidth,            // [in]  Default width
    long *plTextHeight,     // [out] The text height, can be NULL
    long *plTextWidth       // [out] The text width, can be NULL
)
{
    VSASSERT(hwnd, "Argument hwnd cannot be NULL!");
    VSASSERT(hfont, "Argument hfont cannot be NULL!");
    VSASSERT(wszText, "Argument wszText cannot be NULL!");

    HDC hdc = GetDC(hwnd);

    __try
    {
        SelectObject(hdc, hfont);
        RECT rect;
        rect.top = 0;
        rect.bottom = 0;
        rect.left = 0;
        rect.right = lWidth;

        DrawText(hdc, wszText, -1, &rect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);

        if (plTextHeight)
        {
            *plTextHeight = rect.bottom;
        }

        if (plTextWidth)
        {
            *plTextWidth = rect.right;
        }
    }
    __finally
    {
        if (hdc && hwnd)
        {
            ReleaseDC(hwnd, hdc);
        }
    }
}

//=============================================================================
// Get the height needed to contain the text
//=============================================================================
long
GetTextHeight
(
    HWND hwnd,              // [in] Window handle, cannot be NULL
    HFONT hfont,            // [in] Text font, cannot be NULL
    _In_z_ WCHAR *wszText,         // [in] The text, cannot be NULL
    long lWidth             // [in] Default width
)
{
    long lTextHeight = 0;
    GetTextDimensions(hwnd, hfont, wszText, lWidth, &lTextHeight, NULL);
    return lTextHeight;
}

//=============================================================================
// Get the width needed to contain the text
//=============================================================================
long
GetTextWidth
(
    HWND hwnd,              // [in] Window handle, cannot be NULL
    HFONT hfont,            // [in] Text font, cannot be NULL
    _In_z_ WCHAR *wszText,         // [in] The text, cannot be NULL
    long lWidth             // [in] Default width
)
{
    long lTextWidth = 0;
    GetTextDimensions(hwnd, hfont, wszText, lWidth, NULL, &lTextWidth);
    return lTextWidth;
}

//=============================================================================
// Make an array of BSTRs from a given dynamic array of strings
//=============================================================================
HRESULT
MakeBSTRArray
(
    DynamicArray<STRING *> *pdaStrings,   // [in]  Dynamic array of strings, cannot be NULL
    BSTR **prgbstr                        // [out] Receives the allocated array, cannot be NULL
)
{
    HRESULT hr = S_OK;
    BSTR *rgbstrTemp = NULL;

    IfFalseGo(pdaStrings && prgbstr, E_INVALIDARG);

    unsigned cElements = pdaStrings->Count();
    size_t cBufferSize = cElements * sizeof(BSTR); 

    IfFalseGo(VBMath::TryMultiply(cElements, sizeof(BSTR)), E_FAIL);

    rgbstrTemp = reinterpret_cast<BSTR *>(::CoTaskMemAlloc(cBufferSize));
    IfNullGo(rgbstrTemp);
    ::ZeroMemory(rgbstrTemp, cBufferSize);

    for (ULONG i = 0; i < cElements; i++)
    {
        rgbstrTemp[i] = ::SysAllocString(pdaStrings->Element(i));
        IfNullGo(rgbstrTemp[i]);
    }

    *prgbstr = rgbstrTemp;

Error:

    if (FAILED(hr) && rgbstrTemp)
    {
        DeallocateBSTRArray(cElements, rgbstrTemp);
    }

    return hr;
}

//=============================================================================
// Deallocates an array of BSTRs
//=============================================================================
void
DeallocateBSTRArray
(
    int iSize,              // [in] Size of the array to deallocate
    BSTR *rgbstr            // [in] The array to deallocate
)
{
    if (rgbstr)
    {
        for (int i = 0; i < iSize; i++)
        {
            if (rgbstr[i])
            {
                ::SysFreeString(rgbstr[i]);
            }
        }

        ::CoTaskMemFree(rgbstr);
    }
}

bool IsLineBlank(
    _In_opt_count_(cchText)const WCHAR * wszText,
    size_t cchText)
{
    if (wszText && cchText > 0)
    {
        for (size_t i = 0; i < cchText; i++)
        {
            if (!IsWhitespace(wszText[i]))
            {
                return false;
            }
        }
    }

    return true;
}

//============================================================================
// Does the given line in the buffer only contain white space characters?
//============================================================================
bool
IsLineBlank
(
    IVsTextLayer *pIVsTextLayer,        // [in] Text layer, cannot be NULL
    long lLine                          // [in] Line number to check
)
{
    VSASSERT(pIVsTextLayer, "Argument pIVsTextLayer cannot be NULL!");

    LINEDATAEX ld;
    ZeroMemory(&ld, sizeof(LINEDATAEX));
    IfFailThrow(pIVsTextLayer->GetLineDataEx(gldeDefault, lLine, 0, 0, &ld, NULL));

    bool IsBlank = IsLineBlank(ld.pszText, ld.iLength);
    pIVsTextLayer->ReleaseLineDataEx(&ld);
    return IsBlank;
}


//============================================================================
// Skips leading whitespace in the given string
//============================================================================
const WCHAR *
SkipWhiteSpace
(
    _In_z_ const WCHAR *wszText              // [in] The text, cannot be NULL
)
{
    VSASSERT(wszText, "Argument wszText cannot be NULL!");

    while (wszText[0] != UCH_NULL && IsBlank(wszText[0]))
    {
        wszText++;
    }

    return wszText;
}

//============================================================================
// Skips leading non whitespace characters in the given string
//============================================================================
const WCHAR *
SkipNonWhiteSpace
(
    _In_z_ const WCHAR *wszText,             // [in] The text, cannot be NULL
    bool fSkipLineBreaks
)
{
    VSASSERT(wszText, "Argument wszText cannot be NULL!");

    while (wszText[0] != UCH_NULL && !IsBlank(wszText[0]) && (fSkipLineBreaks || !IsLineBreak(wszText[0])))
    {
        wszText++;
    }

    return wszText;
}

//============================================================================
// Removes extra white spaces from the stiring. This is done is place, so
// no new memory is allocated. This is done so if we have this comment:
//
// '@<param name='x'>Hello
// '@there</param>
//
// The the tooltip will not look like this "Hellothere", instead, "Hello there".
//============================================================================
void RemoveExtraWhiteSpacesFromString(_Inout_opt_z_ WCHAR * pTextWithCRAndLF)
{
    if (!pTextWithCRAndLF)
    {
        return;
    }

    long CopyFrom   = 0;
    long CopyTo     = 0;

    bool NeedsASpace = false;
    bool LeadingSpace = true;

#pragma prefast(suppress: 26018, "IsWhiteSpace returns false for the Null character");
    while (pTextWithCRAndLF[CopyFrom] != UCH_NULL)
    {
#pragma prefast(suppress: 26018, "IsWhiteSpace returns false for the Null character");
        while (IsWhitespace(pTextWithCRAndLF[CopyFrom]))
        {
            if (!LeadingSpace)
            {
                NeedsASpace = true;
            }

            CopyFrom++;
        }

        if (pTextWithCRAndLF[CopyFrom] != UCH_NULL)
        {
            LeadingSpace = false;

            if (NeedsASpace)
            {
                pTextWithCRAndLF[CopyTo++] = UCH_SPACE;
                NeedsASpace = false;
            }
            else
            {
                pTextWithCRAndLF[CopyTo++] = pTextWithCRAndLF[CopyFrom++];
            }
        }
    }

    pTextWithCRAndLF[CopyTo] = UCH_NULL;
}

//============================================================================
// Strips brackets from a bracketed identifer
//============================================================================
STRING * RemoveBrackets(_In_z_ WCHAR * wszIdentifier)
{
    VSASSERT(wszIdentifier, "Argument wszIdentifier cannot be NULL!");

    size_t cchIdentifier = wcslen(wszIdentifier);

    if (cchIdentifier >= 2)
    {
        if (wszIdentifier[0] == L'[' && wszIdentifier[cchIdentifier - 1] == L']')
        {
            wszIdentifier++;
            cchIdentifier -= 2;
        }
    }

    return GetCompilerPackage()->AddStringWithLen(wszIdentifier, cchIdentifier);
}

//=============================================================================
// Insert indent, accounting for tabs
//=============================================================================
void
InsertIndent
(
    long lDepth,                    // [in]  Indent depth, in column
    bool ShouldInsertTabs,          // [in]  Whether we should use tabss
    long lTabSize,                  // [in]  The tab size
    StringBuffer *psbText           // [out] Receives the indent, cannot be NULL
)
{
    VSASSERT(psbText, "Argument psbText cannot be NULL!");

    if (ShouldInsertTabs)
    {
        psbText->AppendMultiCopiesOfAWChar(UCH_TAB, lDepth / lTabSize);
        lDepth %= lTabSize;
    }

    psbText->AppendMultiCopiesOfAWChar(L' ', lDepth);
}

//=============================================================================
// Get the list containing the first statement on the logical line
//=============================================================================
ParseTree::StatementList * GetFirstStatementOnLogicalLine(
    ParseTree::StatementList * pStatementList)
{
    while (pStatementList &&
           pStatementList->Element &&
           !pStatementList->Element->IsFirstOnLine)
    {
        pStatementList = pStatementList->PreviousLexical;
    }

    return pStatementList;
}

//=============================================================================
// Get the list containing the last statement on the logical line
//=============================================================================
ParseTree::StatementList * GetLastStatementOnLogicalLine(
    ParseTree::StatementList * pStatementList)
{
    while (pStatementList &&
           pStatementList->NextLexical &&
           pStatementList->NextLexical->Element &&
           !pStatementList->NextLexical->Element->IsFirstOnLine)
    {
        pStatementList = pStatementList->NextLexical;
    }

    return pStatementList;
}

//=============================================================================
// Create a copy of the given parse tree name
//=============================================================================
ParseTree::Name *
CloneParseTreeName
(
    ParseTree::Name *pName,                 // [in]  Name to copy, cannot be NULL
    NorlsAllocator *pAllocator,             // [in]  Allocator to use, cannot be NULL
    _In_opt_z_ STRING *pstrLeftMostReplacement         // [in]  Replacement for the leftmost identifier, can be NULL
)
{
    VSASSERT(pName, "Argument pName cannot be NULL!");
    VSASSERT(pAllocator, "Argument pAllocator cannot be NULL!");

    ParseTree::Name *pRightMostName = NULL;
    ParseTree::Name *pPreviousName = NULL;

    while (pName->IsQualified())
    {
        // Clone the qualified name

        ParseTree::QualifiedName *pQualifiedName;

        if (pName->Opcode == ParseTree::Name::QualifiedWithArguments)
        {
            ParseTree::QualifiedWithArgumentsName *pQualifiedWithArgumentsName = new (*pAllocator) ParseTree::QualifiedWithArgumentsName;
            IfNullThrow(pQualifiedWithArgumentsName);
            *pQualifiedWithArgumentsName = *(pName->AsQualifiedWithArguments());

            VSFAIL("Cloning of generic arguments in parse trees not yet implemented.");

            pQualifiedName = pQualifiedWithArgumentsName;
        }
        else
        {
            pQualifiedName = new (*pAllocator) ParseTree::QualifiedName;
            IfNullThrow(pQualifiedName);
            *pQualifiedName = *(pName->AsQualified());
        }

        // Set the rightmost portion of the name if not set
        if (!pRightMostName)
        {
            pRightMostName = pQualifiedName;
        }

        // Link the newly allocated portion of the name
        if (pPreviousName)
        {
            pPreviousName->AsQualified()->Base = pQualifiedName;
        }

        pPreviousName = pQualifiedName;
        pName = pName->AsQualified()->Base;
    }

    ParseTree::Name *pLeftMostName = NULL;
    if (pName->IsSimple())
    {
        // Clone the simple name

        ParseTree::SimpleName *pSimpleName;

        if (pName->Opcode == ParseTree::Name::SimpleWithArguments)
        {
            ParseTree::SimpleWithArgumentsName *pSimpleWithArgumentsName = new (*pAllocator) ParseTree::SimpleWithArgumentsName;
            IfNullThrow(pSimpleWithArgumentsName);
            *pSimpleWithArgumentsName = *(pName->AsSimpleWithArguments());

            VSFAIL("Cloning of generic arguments in parse trees not yet implemented.");

            pSimpleName = pSimpleWithArgumentsName;
        }
        else
        {
            pSimpleName = new (*pAllocator) ParseTree::SimpleName;
            IfNullThrow(pSimpleName);
            *pSimpleName = *(pName->AsSimple());
        }

        // Replace the name if a replacement was given
        if (pstrLeftMostReplacement)
        {
            pSimpleName->ID.Name = pstrLeftMostReplacement;
        }

        pLeftMostName = pSimpleName;
    }
    else
    {
        pLeftMostName = new (*pAllocator) ParseTree::Name;
        IfNullThrow(pLeftMostName);
        *pLeftMostName = *pName;
    }

    // Set the rightmost portion of the name if not set
    if (!pRightMostName)
    {
        pRightMostName = pLeftMostName;
    }

    // Link the newly allocated portion of the name
    if (pPreviousName)
    {
        pPreviousName->AsQualified()->Base = pLeftMostName;
    }

    return pRightMostName;
}

CompilerProject * FindProjectByFileName(_In_z_ const WCHAR * wszProjectFileName)
{
    AssertIfNull(GetCompilerPackage());
    AssertIfNull(wszProjectFileName);

    CompilerProject *pProject = NULL;

    AllProjectsIterator iter(GetCompilerPackage());
    while (pProject = iter.Next())
    {
        if (CompareFilenames(wszProjectFileName, pProject->GetFileName()) == 0)
        {
            break;
        }
    }

    return pProject;
}

UINT64 ComputeCRC64
(
    const BYTE *rgpbToBeHashed[],
    DWORD rgcbToBeHashed[],
    DWORD nNumElements
)
{
    CRC64 crc64;

    for (DWORD i = 0; i < nNumElements; i++)
    {
        crc64.Update(rgpbToBeHashed[i], rgcbToBeHashed[i]);
    }

    return crc64;
}

#endif IDE

//============================================================================
// StripTrailingBlanksAndLineBreaks
// Purpose:
// String any trailing blanks, and line breaks from the buffer
//============================================================================
void StripTrailingBlanksAndLineBreaks(
    _Inout_opt_cap_(Length)WCHAR * Buffer,
    size_t Length)
{
    // Check to see if there's any trailing blanks
    while (Length > 0 &&
            (IsBlank(Buffer[Length - 1])  ||
             IsLineBreak(Buffer[Length - 1])))
    {
        Length--;
        Buffer[Length] = UCH_NULL;
    }
}

STRING * GetEmittedTypeNameFromActualTypeName(
    _In_z_ STRING * ActualTypeName,
    BCSYM_GenericParam * ListOfGenericParamsForThisType,
    Compiler * CompilerInstance)
{
    VSASSERT(ActualTypeName, "NULL actual name unexpected!!!");

    int GenericParamCount = 0;

    for(BCSYM_GenericParam *GenericParam = ListOfGenericParamsForThisType;
        GenericParam;
        GenericParam = GenericParam->GetNextParam())
    {
        GenericParamCount++;
    }

    if (GenericParamCount == 0)
    {
        return ActualTypeName;
    }

    // Generate a name corresponding to the following :
    //      <ActualName>!<Number of Type Parameters>

    StringBuffer EmittedTypeName;

    EmittedTypeName.AppendSTRING(ActualTypeName);
    EmittedTypeName.AppendChar(GenericTypeNameManglingChar);

    // Max required for 64 bit integer
    //
    WCHAR Buffer[MaxStringLengthForParamSize + 1];

    _itow_s(GenericParamCount, Buffer, _countof(Buffer), DecimalRadix);

    // Just in case... for safety
    //
    Buffer[MaxStringLengthForParamSize] = 0;

    EmittedTypeName.AppendString(Buffer);

    return CompilerInstance->AddString(&EmittedTypeName);
}

unsigned VB_wtoi(
    _In_z_ WCHAR * InputStr,
    bool &NonNumericCharFound)
{
    NonNumericCharFound = false;

    unsigned Result = 0;

    if (InputStr)
    {
		int i;
        for(i = 0; InputStr[i] != '\0'; i++)
        {
            WCHAR ch = InputStr[i];

            if (ch >= L'0' && ch <= L'9')
            {
                Result = (Result * 10) + ch - L'0';
            }
            else
            {
                NonNumericCharFound = true;
                break;
            }
        }

        // Empty string
        //
        if (i == 0)
        {
            NonNumericCharFound = true;
        }
    }
    else
    {
        NonNumericCharFound = true;
    }

    return Result;
}

STRING * GetActualTypeNameFromEmittedTypeName(
    _In_z_ STRING * EmittedTypeName,
    int GenericParamCount,  // -1 indicates match against any arity.
    Compiler * CompilerInstance,
    unsigned * ActualGenericParamCount)
{
    VSASSERT(EmittedTypeName, "NULL actual name unexpected!!!");

    if (ActualGenericParamCount)
    {
        *ActualGenericParamCount = 0;
    }

    if (GenericParamCount == 0)
    {
        return EmittedTypeName;
    }

    size_t EmittedTypeNameLength = StringPool::StringLength(EmittedTypeName);

    VSASSERT(
        EmittedTypeName[EmittedTypeNameLength] == 0,
        "Unexpected end for interned string!!!");

#pragma prefast(suppress: 26018, "EmittedTypeNameLength contains the size of the string");
    IfFalseThrow(EmittedTypeName[EmittedTypeNameLength] == 0);

    size_t IndexOfManglingChar;
    for (IndexOfManglingChar = EmittedTypeNameLength;
         IndexOfManglingChar;
         IndexOfManglingChar--)
    {
        if (EmittedTypeName[IndexOfManglingChar - 1] == GenericTypeNameManglingChar ||
            EmittedTypeName[IndexOfManglingChar - 1] == OldGenericTypeNameManglingChar) // 
                                                                                    // But get rid of this support before shipping, preferably much sooner.
        {
            break;
        }
    }

    if (!IndexOfManglingChar ||
        !(EmittedTypeNameLength - IndexOfManglingChar) ||
        (EmittedTypeNameLength - IndexOfManglingChar > MaxStringLengthForParamSize))
    {
        return EmittedTypeName;
    }

    WCHAR *StringRepresentingParamcount = &EmittedTypeName[IndexOfManglingChar];

    // Given a name corresponding to <ActualName>!<Number Of Type Parameters>,
    // unmanagle the name to be <ActualName>

    bool NonNumericCharFound = false;
    unsigned GenericParamCountInName = VB_wtoi(StringRepresentingParamcount, NonNumericCharFound);

    if (NonNumericCharFound)
    {
        return EmittedTypeName;
    }

    if (ActualGenericParamCount)
    {
        *ActualGenericParamCount = GenericParamCountInName;
    }

    if (GenericParamCount >= 0 &&          // -1 indicates match against any arity.
        GenericParamCount != GenericParamCountInName)
    {
        return EmittedTypeName;
    }

    return
        CompilerInstance->AddStringWithLen(EmittedTypeName, (size_t)(IndexOfManglingChar - 1));
}

HRESULT CoCreateXMLDocument(
    REFIID riid,
    void * * ppXMLDocument)
{
    return CoCreateDOMDocument(riid, ppXMLDocument);
}

//
// Similar functionality to wcstok, but we don't use a static varible (so we
// can call it multiple times without trashing the static nexttoken buffer), and
// we ignore delimiters in quotes.
//
// MatchingStartChar and MatchEndChar can be used for balancing sets of chars like
// (, ) etc. When these are specified, delimiter chars are not treated as the delimiter
// when the matching chars are not balanced. Current there are used for generic Imports
// at the command line.
// They are expected to be different from the delimiter, " and different from
// each other.
//
WCHAR * Split(
    _In_opt_z_ WCHAR * String,
    WCHAR Delimiter,
    _Inout_ _Deref_prepost_z_ WCHAR * * ppNextToken,
    WCHAR MatchingStartChar,
    WCHAR MatchingEndChar)
{
    WCHAR *Token;
    bool InQuote = false;

    // If string==NULL, continue with previous string
    if (!String)
        String = *ppNextToken;

    // Find beginning of token (skip over leading delimiters). Note that
    // there is no token iff this loop sets string to point to the terminal
    // null (*string == '\0')
    while(*String && *String == Delimiter)
    {
        String++;
    }

    Token = String;

    int Nesting = 0;

    VSASSERT(!(!!MatchingStartChar ^ !!MatchingEndChar) &&
             MatchingStartChar != '\"' &&
             MatchingEndChar != '\"' &&
             MatchingEndChar != Delimiter,
                "Unexpected matching chars!!!");

    VSASSERT(MatchingStartChar == 0 ||
             MatchingStartChar != MatchingEndChar,
                "Unexpected matching character set!!!");

    // Find the end of the token. If it is not the end of the string,
    // put a null there.
#pragma prefast(suppress: 26018, "The code correctly checks for the null terminating character")
    while (*String)
    {
        if (*String == MatchingStartChar)
        {
            Nesting++;
        }
        else if (*String == MatchingEndChar)
        {
            Nesting--;
        }

        ULONG cSlashes = 0;

#pragma prefast(suppress: 26018, "The code correctly checks for the null terminating character")
        while (*String  && *String == L'\\')
        {
            cSlashes++;
            String++;
        }

        if (*String == L'\0')
        {
            break;
        }

        if (*String == L'\"' && (cSlashes & 1) == 0)
            InQuote = !InQuote;

        if (*String == Delimiter && !InQuote && Nesting == 0)
        {
            *String++ = L'\0';
            break;
        }

        String++;
    }

    // Update NextToken
    *ppNextToken = String;

    // Determine if a token has been found.
    if (Token == String)
        return NULL;
    else
        return Token;
}


// Impose an ordering on the given GUIDs
int CompareGUIDs(
    const GUID * pGUID1,
    const GUID * pGUID2)
{
    AssertIfNull(pGUID1);
    AssertIfNull(pGUID2);

    int iCompare = CompareValues(pGUID1->Data1, pGUID2->Data1);

    if (iCompare == 0)
    {
        iCompare = CompareValues(pGUID1->Data2, pGUID2->Data2);

        if (iCompare == 0)
        {
            iCompare = CompareValues(pGUID1->Data3, pGUID2->Data3);

            if (iCompare == 0)
            {
                iCompare = memcmp(pGUID1->Data4, pGUID2->Data4, DIM(pGUID1->Data4));
            }
        }
    }

    return iCompare;
}

DWORD ComputeCRC32(
    const BYTE * rgpbToBeHashed[],
    size_t rgcbToBeHashed[],
    ULONG nNumElements)
{
    CRC32 crc32;

    for (ULONG i = 0; i < nNumElements; i++)
    {
        crc32.Update(rgpbToBeHashed[i], rgcbToBeHashed[i]);
    }

    return crc32;
}

BCSYM * GetParamType(
    BCSYM_Param * pParam,
    BCSYM_GenericBinding * pGenericBinding,
    Symbols * pSymbols)
{
    ThrowIfNull(pSymbols);

    if (pParam &&
        pParam->GetType() &&
        !pParam->GetType()->IsBad())
    {
        BCSYM *pParamType = GetDataType(pParam);

        if (TypeHelpers::IsPointerType(pParamType))
        {
            pParamType = TypeHelpers::GetReferencedType(pParamType->PPointerType());
        }

        return ReplaceGenericParametersWithArguments(
            pParamType,
            pGenericBinding,
            *pSymbols);
    }

    return NULL;
}

// If the type is a generic instantiation of Expression(of T), return T
BCSYM * DigThroughGenericExpressionType(BCSYM * pType)
{
    if (pType &&
        pType->IsGenericTypeBinding())
    {
        BCSYM_NamedRoot *pGenericType = pType->PGenericTypeBinding()->GetGenericType();
        CompilerHost *pCompilerHost = pGenericType->GetContainingProject()->GetCompilerHost();

        if (pCompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericExpressionType) &&
            TypeHelpers::EquivalentTypes(pType->PGenericTypeBinding()->GetGenericType(),
                            pCompilerHost->GetFXSymbolProvider()->GetType(FX::GenericExpressionType)))
        {
            return pType->PGenericTypeBinding()->GetArgument(0);
        }
    }

    return pType;
}

const WCHAR *PlatformAgnostic = L"anycpu";
const WCHAR *PlatformX86      = L"x86";
const WCHAR *PlatformIA64     = L"Itanium";
const WCHAR *PlatformAMD64    = L"x64";
const WCHAR *PlatformARM      = L"arm";
const WCHAR *PlatformAnycpu32bitpreferred = L"Anycpu32bitpreferred";

PlatformKinds MapStringToPlatformKind(LPCWSTR wszPlatformKind)
{
    if (!wszPlatformKind || CompareNoCase(wszPlatformKind, L"") == 0)
    {
        // default to agnostic
        return Platform_Agnostic;
    }
    else if (CompareNoCase(wszPlatformKind, PlatformAgnostic) == 0)
    {
        return Platform_Agnostic;
    }
    else if (CompareNoCase(wszPlatformKind, PlatformX86) == 0)
    {
        return Platform_X86;
    }
    else if (CompareNoCase(wszPlatformKind, PlatformIA64) == 0)
    {
        return Platform_IA64;
    }
    else if (CompareNoCase(wszPlatformKind, PlatformAMD64) == 0)
    {
        return Platform_AMD64;
    }
    else if (CompareNoCase(wszPlatformKind, PlatformARM) == 0)
    {
        return Platform_ARM;
    }
    else if (CompareNoCase(wszPlatformKind, PlatformAnycpu32bitpreferred) == 0)
    {
        return Platform_Anycpu32bitpreferred;
    }


    return Platform_Invalid;
}

bool IsValidPlatformKind(unsigned Type)
{
    return Type <= Platform_Max;
}

bool IsValidPlatformKind(LPCWSTR wszType)
{
    return IsValidPlatformKind(MapStringToPlatformKind(wszType));
}

#if IDE

// see if long running compiler operations should be aborted
// if Semantics is passed, in check to see if it's abortable (needed for ui thread only)
bool CheckStop(_In_ Semantics *pSemantics )
{
    bool fAbortCompile = false;

    // bgd thread stop mechanism
    if (GetCompilerSharedState()->IsInBackgroundThread())
    {
        if ( GetCompilerSharedState()->GetThreadStopsUnsafe() > 0 )
        {
            // It is somewhat safe to use this unsafe method in this context.  The UI thread is in control of this
            // variable and use it to signal the background thread to stop processing.  This method is responsible for
            // aborting a background compilation and hence the use fits the model.  The race conditions which can
            // occur here are are the following
            //
            //   1. GetThreadStopsUnsafe returns 0 but the UI thread wants to abort:  This is slower than we would
            //      like but the UI thread will properly wait for a break and eventually we will complete the compile 
            //      or hit another CheckStop which correctly aborts
            //   2. GetThreadStopsUnsafe returns > 0 but the UI thread wants us to run.  This shouldn't ever occur because
            //      whenever the UI thread increments the stop count it will wait for the background thread to get into
            //      a known stopped state.  It will not re-increment the value until this occurs and hence it shouldn't
            //      be possible to get into this state
            //
            // Note: Previous versions of this code also tested GetCompilerSharedState()->IsBackgroundCompilationStopping to
            // determine if we should abort here.  There is no need to check this value as the time for which the value of 
            // GetThreadStopsUnsafe will be > 0 is a superset of IsBackgroundCompilationStopping.  Previously this was the 
            // only place where the result of IsBackgroundCompilationStopping was shared between threads so it simplifies
            // the model greatly to only share one value and hith the same scenarios
            fAbortCompile = true;
        }

        // IVbCompilerThreadTestHook
        if (GetCompilerSharedState()->GetCheckStopTarget()  && GetCompilerSharedState()->GetCheckStopCount() >= GetCompilerSharedState()->GetCheckStopTarget() )
        {
            fAbortCompile = true;
        }
        
        if (!fAbortCompile)
        {
            GetCompilerSharedState()->IncrementCheckStopCount();
        }
    }
    if (fAbortCompile)
    {
#if LOGTHREADEVENTS
        const WCHAR *projname = L"";
        int compstate = 0; 
        if ( GetCompilerSharedState()->GetProjectBeingCompiled())
        {
            compstate = GetCompilerSharedState()->GetProjectBeingCompiled()->GetCompState();
            projname = GetCompilerSharedState()->GetProjectBeingCompiled()->GetFileName();
        }
        DebThreadLogEvent(("CheckStop True cs=%d %S\n", compstate, projname));
#endif LOGTHREADEVENTS  
    }
    return fAbortCompile;
}
#endif IDE

