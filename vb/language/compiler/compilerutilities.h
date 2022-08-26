//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Compiler utility functions.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#define DOT_DELIMITER (WCHAR)'.'        // use to delimit strings like "foo.boo.main"

const BYTE UTF_8_BYTE_ORDER_MARKER[3] = {0xEF, 0xBB, 0xBF};
const ULONG UTF_8_BYTE_ORDER_MARKER_LENGTH = 3;

static const int CCH_CRLF   = 2;

//
// String & error formatting code
//

// Returns a concatenation of the namespace and name.
STRING * ConcatNameSpaceAndName(
    Compiler * pcompiler,
    LPCWSTR pwszNameSpace,
    LPCWSTR pwszName);

// Splits wszTypeName (a possibly fully-qualified typename) into
// its namespace and typename components.
void SplitTypeName(
    Compiler * pCompiler,
    _In_opt_z_ LPCWSTR wszTypeName,
    _Deref_out_z_ STRING ** ppstrNameSpace,
    _Deref_out_z_ STRING ** ppstrUnqualName);

extern int DaysToMonth365[], DaysToMonth366[];

// Converts a DateTime to a BSTR
BSTR BstrFromDateTime( __int64 DateValue, bool ShowCompleteDate);

#if !(IDE )
// Moved them from compiler.h so Compiler class is able to be used by both IDE
// with threading and other request such as command line compiler, expression
// evaluator.
inline void DebCheckInCompileThread(Compiler *pCompiler) {}
inline void DebCheckNoBackgroundThreads(Compiler *pCompiler) {}
inline void CheckInMainThread() {}

inline void FinishCompileStep(Compiler *pCompiler) {}
inline void OnProject (VBCOLLECTION eStatusP, CompilerProject * pProjectP) {}
inline void OnAssembly (VBCOLLECTION eStatusP, CompilerProject * pProjectP) {}

#else //IDE
// Moved them from compilerpackage.h - included in StdAfx.h
#if DEBUG

// Debug-only method to verify that we're being called from the compiler's thread.
inline
void DebCheckInCompileThread(Compiler * pCompiler)
{
    if (pCompiler)
    {
        pCompiler->DebCheckInThisThread();
    }
}

#else !DEBUG

inline void DebCheckInCompileThread(Compiler *pCompiler) {}

#endif !DEBUG

inline
void FinishCompileStep(Compiler * pCompiler)
{
    if(pCompiler)
    {
        pCompiler->FinishCompileStep();
    }
}

// pSementics: if we're aborting from within Semantics, otherwise NULL
bool CheckStop(_In_ Semantics *pSemantics);

inline
void OnProject(
    VBCOLLECTION eStatusP,
    CompilerProject * pProjectP)
{
    if (pProjectP && pProjectP->GetCompiler())
    {
        pProjectP->GetCompiler()->_OnProject(eStatusP,pProjectP);
    }
}
inline
void OnAssembly(
    VBCOLLECTION eStatusP,
    CompilerProject * pProjectP)
{
    if (pProjectP && pProjectP->GetCompiler())
    {
        pProjectP->GetCompiler()->_OnAssembly(eStatusP,pProjectP);
    }
}

#endif IDE

extern const tokens g_rgtokITypes[];
extern const unsigned short s_cbOfVtype[];

inline
unsigned CbOfVtype(Vtypes vtype)
{
    return (unsigned)s_cbOfVtype[vtype];
}

inline
bool IsCharType(Vtypes vtype)
{
    return vtype == t_char;
}

inline
bool IsBooleanType(Vtypes vtype)
{
    return vtype == t_bool;
}

inline
bool IsFloatingType(Vtypes vtype)
{
    return vtype == t_single || vtype == t_double;
}

inline
bool IsUnsignedLongType(Vtypes vtype)
{
    return vtype == t_ui8;
}

inline
bool IsArrayType(Vtypes vtype)
{
    return vtype == t_array;
}

extern const bool s_TypeIsNumeric[];

inline
bool IsNumericType(Vtypes vtype)
{
    return s_TypeIsNumeric[vtype];
}

extern const bool s_TypeIsIntegral[];

inline
bool IsIntegralType(Vtypes vtype)
{
    return s_TypeIsIntegral[vtype];
}

extern const bool s_TypeIsUnsigned[];

inline
bool IsUnsignedType(Vtypes vtype)
{
    return s_TypeIsUnsigned[vtype];
}

extern const Vtypes s_typeCharToVtype[];

inline
Vtypes VtypeOfTypechar(typeChars tp)
{
    return s_typeCharToVtype[tp];
}

// Associate a character with the TypeChar
extern const WCHAR *s_wszTypeChars[];

inline
const WCHAR * WszTypeChar(typeChars tch)
{
    return s_wszTypeChars[tch];
}

extern const tokens s_OperatorTokenTypes[];

inline
const tokens OperatorTokenTypes(UserDefinedOperators op)
{
    return s_OperatorTokenTypes[op];
}

extern const bool s_OperatorIsUnary[];

inline
bool IsUnaryOperator(UserDefinedOperators op)
{
    return s_OperatorIsUnary[op];
}

extern const bool s_OperatorIsBinary[];

inline
bool IsBinaryOperator(UserDefinedOperators op)
{
    return s_OperatorIsBinary[op];
}

extern const bool s_OperatorIsConversion[];

inline
bool IsConversionOperator(UserDefinedOperators op)
{
    return s_OperatorIsConversion[op];
}

STRING * StringOfSymbol(
    Compiler * pCompiler,
    BCSYM * psym);

STRING * StringOfAccess(
    Compiler * pCompiler,
    ACCESS access);

/****************************************************************/
// Wraps keywords and such in '[' and ']' so that it would safe to
// use these special names. If the name passed in is not one of those
// special names, or if some error occures, we just return the same
// string we were passed.
STRING * MakeSafeName(
    Compiler * pCompiler,
    _In_opt_z_ STRING * pstr,
    bool fParent);

/****************************************************************/
// Wraps keywords and such in '[' and ']' so that it would safe to
// use these special names. This method takes any Dot qualified
// name, and adds the brackets to any component that needs it.
//
// Note: the final string is added to the string pool, and a pointer
// is returned.
STRING * MakeSafeQualifiedName(
    Compiler * pCompiler,
    _In_z_ STRING * Name);

/****************************************************************/
// Breaks up a qualified name "in the form of a.b.foo" into
// an array of STRINGs, each element being one component. The caller
// need not worry about freeing up the BSTRs allcated, since a NRA
// is used here.
HRESULT BreakUpQualifiedName(
    Compiler * pCompiler,
    _Out_ _Deref_post_cap_(* ulDelimiterCount)STRING ** rgpstr[],
    NorlsAllocator * pnra,
    _Out_ ULONG * ulDelimiterCount,
    _In_z_ WCHAR * wscStringToBreakUp,
    WCHAR ch);

/****************************************************************/
// Grabs a string from a Name parsetree ,and returns it. Works
// for both simple and qualified names
STRING * StringFromParseTreeName(
    Compiler * pCompiler,
    ParseTree::Name * NameTree);

//============================================================================
// Determine if this is a Form class, in which case it could have a
// synthetic sub Main
//============================================================================
bool SeeIfWindowsFormClass(
    BCSYM_Class * ClassToVerify,
    Compiler * pCompiler);

/****************************************************************/
// Abstract: Determine the vtype of a given ParseTree::Type
Vtypes MapTypeToVType(ParseTree::Type::Opcodes intrinsicType);

/****************************************************************/
long CountLineBreaksInString(PCWSTR pstrText);

//============================================================================
// Converts the given Unicode string into a UTF8 string.
//============================================================================
void ConvertUnicodeToUTF8
(
    const WCHAR     *pwsz,      // IN:  Argument to encode
    ULONG            cch,       // IN:  Length of string (allows embedded NUL chars)
    NorlsAllocator  *pnra,      // IN:  Where to allocate UTF8 string from
    BYTE           **ppbUTF8,   // OUT: String encoded as UTF8.
    ULONG           *pcbUTF8    // OUT: Length of encoded UTF8 string in bytes
);


#if DEBUG
// Debug-only helper that returns a human-readable string representing the
// file's last-modified time.
STRING * DebStrFromFileTime(
    _In_z_ const WCHAR * pwszFile,
    Compiler * pCompiler);
#endif

#if DEBUG
// Debug-only helper that returns a human-readable string representing the
// given FILETIME structure.
STRING * DebStrFromFileTime(
    const FILETIME &filetime,
    Compiler * pCompiler);
#endif


#define DOUBLE_PRECISION 17
#define FLOAT_PRECISION 9

HRESULT R8ToBSTR(
    double doValue,
    BSTR * pbstrValue,
    bool fRound = true);

HRESULT R4ToBSTR(
    float flValue,
    BSTR * pbstrValue,
    bool fRound = true);

//============================================================================
// Return a strong assembly name in fusion format
// for example, "System, Version=x.x.xxxx.x, Culture=neutral, PublicKeyToken=xxxx"
//============================================================================
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
    DWORD dwFlags);

// Get the executable statements for a method
HRESULT ParseCodeBlock(
    Compiler * pCompiler,
    Text * ptext,
    SourceFile * pSourceFile,
    NorlsAllocator * pnra,
    ErrorTable * perrortable,
    BCSYM_Container * pConditionalCompilationConstants,
    const CodeBlockLocation * pCodeBlock,
    // passed only when method def is requested (need sig location for codegen)
    const CodeBlockLocation * pProcBlock,
    ParseTree::Statement::Opcodes MethodBodyKind,
    ParseTree::MethodBodyStatement ** pptree,
    MethodDeclKind methodDeclKind,
    bool alwaysParseXmlDocComments = false);

//  Get the Decl Trees for this file.
HRESULT GetDeclTrees(
    _In_ Compiler * pCompiler,
    _In_ Text * ptext,
    _In_ SourceFile * pSourceFile,
    _In_ NorlsAllocator * pnra,
    _In_ NorlsAllocator * pnraConditionalCompilaton,
    _In_ ErrorTable * pErrorTable,
    _In_ BCSYM_Container ** ppcontinerConditionalCompilaton,
    _In_ LineMarkerTable * LineMarkerTableForConditionals,
    _Out_ ParseTree::FileBlockStatement ** ppbiltree,
    _In_opt_ long startLine = 0,
    _In_opt_ bool alwaysParseXmlDocComments = false
    );

void GetStatementSpanIncludingComments(
    ParseTree::Statement * pStatement,
    Location * plocStatement);

bool HasPunctuator(const ParseTree::PunctuatorLocation &Loc);

bool StartsAfterPunctuator(
    const Location &loc,
    const Location &locBase,
    const ParseTree::PunctuatorLocation &punctuator);

bool HasParens(ParseTree::MethodSignatureStatement * pMethodStmt);

bool HasParens(ParseTree::PropertyStatement *pPropertyStmt);

BCSYM_NamedRoot * LookupSymbol(
    _In_z_ const WCHAR * wszSymbolName,
    CompilerHost * pCompilerHost,
    BCSYM_Container * pLookup,
    BCSYM_Container * pContext,
    NameFlags LookUpFlags,
    int iArity = - 1,
    bool AllowBadSymbol = false,
    bool * pfHasTrailingTokens = NULL,
    bool * pfHasParseError = NULL);

#if IDE 
void WaitUntilDesiredState(
    Compiler * pCompiler,
    CompilationState cs,
    WaitStatePumpingEnum ShouldPump, 
    WaitStateCancelEnum CanCancel);

void GetProjectName(
    IVsHierarchy * pProjectHierarchy,
    VSITEMID itemid,
    StringBuffer * psb);

bool IsLinkedFile(
    IVsHierarchy * pProjectHierarchy,
    VSITEMID itemid);

STRING * GetProjectNameForTestHooks(CompilerProject * pProject);

HRESULT GetProjectAndFile(
    const WCHAR * wszProject,
    const WCHAR * wszFile,
    CompilerProject ** ppCompilerProject,
    SourceFile ** ppSourceFile);

HRESULT GetProject(
    _In_ const WCHAR * wszProject,
    _Deref_out_ CompilerProject ** ppCompilerProject);

SourceFile *FindSourceFile(Compiler *pCompiler, _In_z_ STRING *pstrFileName);
// Returns false if pstrFileName is NULL
bool LookupFile(
    Compiler * pCompiler,
    IVsHierarchy * pIVsHierarchy,
    _In_opt_z_ STRING * pstrFileName,
    CompilerProject ** ppCompilerProject,
    SourceFile ** ppSourceFile,
    SourceFileView ** ppSourceFileView);

void GetTextDimensions(
    HWND hwnd,
    HFONT hfont,
    _In_z_ WCHAR * wszText,
    long iWidth,
    long * plTextHeight,
    long * plTextWidth);

long GetTextHeight(
    HWND hwnd,
    HFONT hfont,
    _In_z_ WCHAR * wszText,
    long lWidth);

long GetTextWidth(
    HWND hwnd,
    HFONT hfont,
    _In_z_ WCHAR * wszText,
    long lWidth);

HRESULT MakeBSTRArray(
    DynamicArray<STRING *> * pdaStrings,
    BSTR ** prgbstr);

void DeallocateBSTRArray(
    int iSize,
    BSTR * pbstr);

bool IsLineBlank(
    _In_opt_count_(cchText)const WCHAR * wszText,
    size_t cchText);

bool IsLineBlank(
    IVsTextLayer * pIVsTextLayer,
    long lLine);

const WCHAR * SkipWhiteSpace(_In_z_ const WCHAR * wszText);

const WCHAR * SkipNonWhiteSpace(
    _In_z_ const WCHAR * wszText,
    bool fSkipLineBreaks);

// Removes extra white spaces from a string. This is done is place, so no new memory is allocated.
void RemoveExtraWhiteSpacesFromString(_Inout_opt_z_ WCHAR * pTextWithCRAndLF);

STRING * RemoveBrackets(_In_z_ WCHAR * wszIdentifier);

void InsertIndent(
    long lDepth,
    bool ShouldInsertTabs,
    long lTabSize,
    StringBuffer * psbText);

ParseTree::StatementList * GetFirstStatementOnLogicalLine(
    ParseTree::StatementList * pStatementList);

ParseTree::StatementList * GetLastStatementOnLogicalLine(
    ParseTree::StatementList * pStatementList);

ParseTree::Name * CloneParseTreeName(
    ParseTree::Name * pName,
    NorlsAllocator * pAllocator,
    _In_opt_z_ STRING * pstrLeftMostReplacement = NULL);

CompilerProject * FindProjectByFileName(_In_z_ const WCHAR * wszProjectName);

UINT64 ComputeCRC64(
    const BYTE * rgpbToBeHashed[],
    DWORD rgcbToBeHashed[],
    DWORD nNumElements);

#endif IDE

DWORD ComputeCRC32(
    const BYTE * rgpbToBeHashed[],
    size_t rgcbToBeHashed[],
    ULONG nNumElements);

void StripTrailingBlanksAndLineBreaks(
    _Inout_opt_cap_(Length)WCHAR * Buffer,
    size_t Length);

//============================================================================
// Helper to dump the "word" form of the compilation state.
//============================================================================
inline
void DumpCompilationState(CompilationState state)
{
    const char *pState = NULL;

    switch (state)
    {
    case CS_NoState:
        pState = "CS_NoState ";
        break;
    case CS_Declared:
        pState = "CS_Declared";
        break;
    case CS_Bound:
        pState = "CS_Bound";
        break;
    case CS_Compiled:
        pState = "CS_Compiled";
        break;
    case CS_TypesEmitted:
        pState = "CS_TypesEmitted";
        break;
    default:
        VSFAIL("Invalid state");
        break;

    }

    if (VBFTESTSWITCH(fDumpDecompilesForSuites))
    {
        DebTShellPrintf(pState);
    }
    else
    {
        DebPrintf(pState);
    }
}

// Max required for 64 bit integer
//
const int MaxStringLengthForParamSize = 22;
const int MaxStringLengthForIntToStringConversion = 22;
const int DecimalRadix = 10;
const WCHAR GenericTypeNameManglingChar = L'`';
const WCHAR OldGenericTypeNameManglingChar = L'!';

STRING * GetEmittedTypeNameFromActualTypeName(
    _In_z_ STRING * ActualTypeName,
    BCSYM_GenericParam * ListOfGenericParamsForThisType,
    Compiler * CompilerInstance);

STRING * GetActualTypeNameFromEmittedTypeName(
    _In_z_ STRING * EmittedTypeName,
    int GenericParamCount, // -1 indicates match against any arity.
    Compiler * CompilerInstance,
    unsigned * ActualGenericParamCount = NULL);

//============================================================================
// Helper for creating a V4, 3, or 2 XMLDocument (Depending on which one is installed on the machine).
//============================================================================
HRESULT CoCreateXMLDocument(
    REFIID riid,
    void ** ppXMLDocument);

//============================================================================
// Helper for splitting a delimited string.
//============================================================================
WCHAR * Split(
    _In_opt_z_ WCHAR * String,
    WCHAR Delimiter,
    _Inout_ _Deref_prepost_z_ WCHAR ** ppNextToken,
    WCHAR MatchingStartChar = 0,
    WCHAR MatchingEndChar = 0);

//============================================================================
// Helper to impose an ordering among two GUIDs
//============================================================================
int CompareGUIDs(
    const GUID * pGUID1,
    const GUID * pGUID2);

//============================================================================
// Error reporting: true - for a real error or a warning repoprted as error
//============================================================================
inline
bool ReportedAsError(
    DWORD errid,
    CompilerProject * pProject)
{
    return errid > 0 &&     // comments have an errid == 0
         ( errid < WRNID_First ||
             (pProject && pProject->GetWarningLevel(errid) == WARN_AsError));
}

inline
Declaration * GetInvokeFromDelegate(
    Type * delegateType,
    Compiler * compiler) /*NULL ok, if in IDE */
{
#if IDE 
    if (compiler == NULL)
    {
        return delegateType->PClass()->GetHash()->SimpleBind(IDE_STRING_CONST(DelegateInvoke));
    }
    else
#endif
    {
        VSASSERT(compiler != NULL, "Must pass in compiler, only in IDE you are allowed to pass in NULL since strings are global there");
        return delegateType->PClass()->GetHash()->SimpleBind(STRING_CONST(compiler, DelegateInvoke));
    }
}

BCSYM * GetParamType(
    BCSYM_Param * pParam,
    BCSYM_GenericBinding * pGenericBinding,
    Symbols * pSymbols);

// If the type is a generic instantiation of Expression(of T), return T
BCSYM * DigThroughGenericExpressionType(BCSYM * pType);
