//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation file for the XML CodeModel generator
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

// I use this to break in some places that match the given name. This allows me
// to debug a certain subtree at some Unknown depth level very quickly
// Set this to 0 to turn off all debugging, or 1 to turn on debugging
#if DEBUG
#if 0
PCWCH BreakAtName = NULL;               // Runtime Breakpoints
// PCWCH BreakAtName = L"#2#3";
#define AssertConditionSet 1
#else
PCWCH BreakAtName = NULL;               // Runtime Breakpoints
#define AssertConditionSet 0            // Overrides assertions
#endif

// A way for us to force the dumping of the keys, even if we are in test mode
// #define KEY_DUMP_OVERRIDE

#endif DEBUG

extern const WCHAR *g_wszFullCLRNameOfVtype[];

//****************************************************************************
// Constructor for calling helper methods. We are not doing any XML work here.
// The only purpose of this constructor is to create an XMLGen object, and
// then this object can be used later on to call any non-static method.
//
// All input args must be valid. We are not checking for validation of anything
// here.
//****************************************************************************
XMLGen::XMLGen(
    Compiler * pCompiler,
    SourceFile * pSourceFile,
    CompilerProject * pCompilerProject,
    HRESULT * phr) :
    m_NameStack(pCompiler)
{
    m_pCompiler = pCompiler;
    m_pCompilerProject = pCompilerProject;
    m_pSourceFile = pSourceFile;

#if IDE
    if (pCompilerProject->GetCompState() < CS_Bound) // #296747 - if XML for declarations includes information about initializers, then the entire project must be Bound to produce XML.
    {
        *phr = m_pCompilerProject->WaitUntilBindableState(WaitStatePumping::PumpingAllowed, WaitStateCancel::CancelAllowed); // Make sure we are in Bound

        if (FAILED(*phr))
        {
            return;
        }

        if (pSourceFile->GetCompState() < CS_Bound)
        {
            VSFAIL(L"How come the file is not in Bound state?");
            *phr = E_FAIL;
        }
    }
#endif IDE
}

//****************************************************************************
// Creates the XML CodeModel generator. This constrcutor processes only
// declaration. While processing the declarations, we may encounter methods,
// and we may decide to process these as well, but the entry point is always
// a declaration.
//
// All input args must be valid. We are not checking for validation of anything
// here.
//****************************************************************************
XMLGen::XMLGen(
    Compiler * pCompiler,
    SourceFile * pSourceFile,
    ParseTree::BlockStatement * ptree,
    CompilerProject * pCompilerProject,
    StringBuffer * psbBuffer,
    bool fProcessMethods,
    bool fTypesOnly,
    HRESULT * phr) :
    m_NameStack(pCompiler)
{
    m_pCompiler = pCompiler;
    m_pCompilerProject = pCompilerProject;
    m_pSourceFile = pSourceFile;
    m_fProcessMethods = fProcessMethods;
    m_fProcessTypesOnly = fTypesOnly;

#if IDE
    if (pCompilerProject->GetCompState() < CS_Bound) // #296747 - if XML for declarations includes information about initializers, then the entire project must be Bound to produce XML.
    {
        *phr = m_pCompilerProject->WaitUntilBindableState(WaitStatePumping::PumpingAllowed, WaitStateCancel::CancelAllowed); // Make sure we are in Bound

        if (FAILED(*phr))
        {
            return;
        }

        if (pSourceFile->GetCompState() < CS_Bound)
        {
            VSFAIL(L"How come the file is not in Bound state?");
            *phr = E_FAIL;
            return;
        }
    }
#endif IDE

    AppendString(psbBuffer, XML_VERSION, INITIAL_DEPTH_LEVEL, NO_ENTITY_REF_CHECK);
    *phr = ProcessDeclarations(psbBuffer, ptree, INITIAL_DEPTH_LEVEL);
}

//****************************************************************************
// Creates the XML CodeModel generator. This constrcutor processes only
// declaration. While processing the declarations, we may encounter methods,
// and we may decide to process thesr as well, but the entry point is always
// a declaration.
//
// All input args must be valid. We are not checking for validation of anything
// here.
//****************************************************************************
XMLGen::XMLGen(
    Compiler * pCompiler,
    SourceFile * pSourceFile,
    ParseTree::StatementList * ptree,
    CompilerProject * pCompilerProject,
    StringBuffer * psbBuffer,
    bool fProcessMethods,
    HRESULT * phr) :
    m_NameStack(pCompiler)
{
    m_pCompiler = pCompiler;
    m_pCompilerProject = pCompilerProject;
    m_pSourceFile = pSourceFile;
    m_fProcessMethods = fProcessMethods;

#if IDE
    // If processing methods, the entire project must be in Bound state.
    // Otherwise, it is sufficient for just this file to be in Bound state.
    if (fProcessMethods ?
            pCompilerProject->GetCompState() < CS_Bound :
            pSourceFile->GetCompState() < CS_Bound)
    {
        *phr = m_pCompilerProject->WaitUntilBindableState(WaitStatePumping::PumpingAllowed, WaitStateCancel::CancelAllowed); // Make sure we are in Bound

        if (FAILED(*phr))
        {
            return;
        }

        if (pSourceFile->GetCompState() < CS_Bound)
        {
            VSFAIL(L"How come the file is not in Bound state?");
            *phr = E_FAIL;
            return;
        }
    }
#endif IDE

    AppendString(psbBuffer, XML_VERSION, INITIAL_DEPTH_LEVEL, NO_ENTITY_REF_CHECK);
    *phr = ProcessStatementList(psbBuffer, ptree, INITIAL_DEPTH_LEVEL);
}

//****************************************************************************
// Creates the XML CodeModel generator. This constuctor processes a method
// body.
//
// All input args must be valid. We are not checking for validation of anything
// here.
//****************************************************************************
XMLGen::XMLGen(
    Compiler * pCompiler,
    SourceFile * pSourceFile,
    ILTree::ILNode * PILNode,
    CompilerProject * pCompilerProject,
    StringBuffer * psbBuffer,
    BCSYM_Proc * pproc,
    HRESULT * phr) :
    m_NameStack(pCompiler)
{
    m_pCompiler = pCompiler;
    m_pCompilerProject = pCompilerProject;
    m_pSourceFile = pSourceFile;
    m_pproc = pproc;
    m_fProcessMethods = true;       // We are processing method bodies

#if IDE
    // If processing methods, the entire project must be in Bound state.
    if (pCompilerProject->GetCompState() < CS_Bound)
    {
        *phr = m_pCompilerProject->WaitUntilBindableState(WaitStatePumping::PumpingAllowed, WaitStateCancel::CancelAllowed); // Make sure we are in Bound

        if (FAILED(*phr))
        {
            return;
        }

        if (pSourceFile->GetCompState() < CS_Bound)
        {
            VSFAIL(L"How come the file is not in Bound state?");
            *phr = E_FAIL;
            return;
        }
    }
#endif IDE

    AppendString(psbBuffer, XML_VERSION, INITIAL_DEPTH_LEVEL, NO_ENTITY_REF_CHECK);
    *phr = ProcessMethodBody(psbBuffer, PILNode, INITIAL_DEPTH_LEVEL);
}

//****************************************************************************
// Deletes the XML CodeModel generator.
//****************************************************************************
XMLGen::~XMLGen()
{
    // If we have used this text buffer, we go ahead and delete it
    if (m_pTextFile)
    {
        delete m_pTextFile;
    }

#if DEBUG && AssertConditionSet
    VSASSERT(m_BN_WITH == 0 , L"We never finished a \"With\" block!");
    VSASSERT(!m_fShouldQuote, "Hay, somebody was trying to quote a block of code but didn't get a chance to!");
#endif
}

//****************************************************************************
// Entry point for getting the XML text for declarations inside a file.
// We return the XML text in psbBuffer.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::XMLRunDeclarations(
    Compiler * pCompiler,
    SourceFile * pSourceFile,
    StringBuffer * psbBuffer)
{
    if (!pSourceFile)
    {
        return NOERROR;
    }

    // If we are passed a bad buffer, we can't do a thing
    if (!psbBuffer)
    {
        return E_POINTER;
    }

    NorlsAllocator nraDeclTrees(NORLSLOC);
    NorlsAllocator nraConsTrees(NORLSLOC);

    ParseTree::FileBlockStatement *ptree;

#if IDE
    // Get decl trees for that one source file
    HRESULT hr = NOERROR;

    pSourceFile->GetParseTreeService()->GetDeclTrees(NULL, // Countainer for CC's
                                                     &ptree);
#else
    // Get decl trees for that one source file
    HRESULT hr = pSourceFile->GetDeclTrees(&nraDeclTrees, &nraConsTrees, NULL,
                                            NULL, pSourceFile->GetLineMarkerTable(), &ptree);
#endif

    // GetDeclTrees can fail if the file has disappeared, check for this case.
    if (FAILED(hr) || !ptree)
    {
        return E_FAIL;
    }

    // The whole thing is being done in the constructor. Once it returns, the XML string should be
    // in the out param. Run the XML generator on the decl tree
    XMLGen XMLGenerator(pCompiler, pSourceFile, ptree, pSourceFile->GetProject(),
                        psbBuffer, NO_METHODS, PROCESS_ALL_DECLS, &hr);

    return hr;
}

//****************************************************************************
// Entry point for getting the XML text for method bodies inside a file.
// We return the XML text in psbBuffer.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::XMLRunBodies(Compiler *pCompiler, SourceFile *pSourceFile,
                             StringBuffer *psbBuffer, BCSYM_Proc *pproc)
{
    if (!pSourceFile)
    {
        return NOERROR;
    }

    // If we are passed a bad buffer, we can't do a thing
    if (!psbBuffer)
    {
        return E_POINTER;
    }

    if (!pproc || pproc->GetKind() != SYM_MethodImpl)
    {
        VSASSERT(!AssertConditionSet, "Can only generate XML code for method bodies of type SYM_MethodImpl");
        return E_INVALIDARG;
    }

    ILTree::PILNode pBilMethodBodyTree = NULL;
    NorlsAllocator nrabodyTrees(NORLSLOC);
    ErrorTable errTable(pCompiler, pSourceFile->GetProject(),  NULL);
    CompilationCaches Caches;

    // Dummy cycles. Without it we crash in GetBoundMethodBodyTrees()
    Cycles cycles(pCompiler, &nrabodyTrees, &errTable, ERRID_SubNewCycle1, ERRID_SubNewCycle2);
    HRESULT hr = pSourceFile->GetBoundMethodBodyTrees(pproc,
                                                      &nrabodyTrees,
                                                      &errTable,
                                                      &cycles,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      &Caches,
                                                      gmbIDEBodies | gmbMergeAnonymousTypes | gmbIncludeBadExpressions | gmbLowerTree,
                                                      &pBilMethodBodyTree,
                                                      NULL,
                                                      NULL,
                                                      NULL);

    // GetbodyTrees can fail if the file has disappeared, check for this case.
    if (FAILED(hr) || !pBilMethodBodyTree)
    {
        return E_FAIL;
    }

    // The whole thing is being done in the constructor. Once it returns, the XML string should be
    // in the out param.
    // Run the XML generator on the method body tree
    XMLGen XMLGenerator(pCompiler, pSourceFile, pBilMethodBodyTree, pSourceFile->GetProject(),
                        psbBuffer, pproc, &hr);

    return hr;
}

//****************************************************************************
// Generates an ID attribute for any given BillTree. The tree is expected to
// have a .Loc member that we c----e to generate the IDs.
// This method uses the new parse tree.
//
// Return: Success or catstrophic failure code..
//****************************************************************************
HRESULT XMLGen::GenerateStatementID(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, Location *pLoc)
{
    HRESULT hr = NOERROR;
    WCHAR wszId[cbIdSize * 2];  // Upto 4 location info can be stored here.
    WCHAR wszExtraId[cbIdSize];
    wszExtraId[0] = UCH_NULL;

    CodeFile::MakeStatementId(pstatement, wszId, _countof(wszId));  // Use the CodeFile ID generator service

    // If an extra location information is provided, we go ahead and use it. This allows us to have up
    // to 3 location information in IDs.
    if (pLoc && pLoc->IsValid())
    {
        CodeFile::MakeStatementId(*pLoc, wszExtraId, _countof(wszExtraId));

        size_t IdLength = wcslen(wszId);

        if (IdLength + 1 < _countof(wszId))
        {
            wszId[IdLength] = L'~';
            wszId[IdLength + 1] = UCH_NULL;

            wcscat_s(wszId, _countof(wszId), wszExtraId);
        }
    }

    IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Id, wszId, CHECK_ENTITY_REF, NO_BRACKETS));

#if DEBUG
    // Its very helpful to undo the ID info so that we can find out what the line numbers
    // we are incoding are, for debugging purposes.
    Location startLocation;
    Location endLocation;
    Location extraLocation;

    CodeFile::DecodeId(wszId, _countof(wszId), &startLocation, &endLocation, &extraLocation);
#endif  // DEBUG

Error:
    return hr;
}

HRESULT XMLGen::GenerateStatementID(StringBuffer *psbBuffer, const Location &loc, IdKind KindOfIDToGenerate)
{
    HRESULT hr = NOERROR;
    WCHAR wszId[cbIdSize];

    CodeFile::MakeStatementId(loc, wszId, _countof(wszId));  // Use the CodeFile ID generator service

    switch (KindOfIDToGenerate)
    {
    case ID :
        IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Id, wszId, CHECK_ENTITY_REF, NO_BRACKETS));
        break;

    case ParamID :
        IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Param_ID, wszId, CHECK_ENTITY_REF, NO_BRACKETS));
        break;

    case RefID :
        IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Reference_ID, wszId, CHECK_ENTITY_REF, NO_BRACKETS));
        break;

    default :
        VSFAIL("Unknown ID Type");
    }


#if DEBUG
    // Its very helpful to undo the ID info so that we can find out what the line numbers
    // we are incoding are, for debugging purposes.

    Location startLocation;
    Location endLocation;

    CodeFile::DecodeId(wszId, _countof(wszId), &startLocation, &endLocation, NULL);
#endif  // DEBUG

Error:
    return hr;
}

//****************************************************************************
// Retrun the hash for the passed in BCSYM, or NULL if none exist.
//****************************************************************************
BCSYM_Hash *XMLGen::ContextOf(BCSYM_NamedRoot *pContext)
{
    if (!pContext || !pContext->IsContainer())
    {
        return NULL;
    }

    return pContext->PContainer()->GetHash();
}

//****************************************************************************
// Returns true if the string passed in needs to be bracketed for it to be a
// valid identifier name in VB (e.g. [integer])
//****************************************************************************
bool XMLGen::NeedsBrackets(BCSYM_NamedRoot *NameInQuestion)
{
    // For method bodies, we FX doesn't want any brackets, so we make sure we don't add them in. VS540676
    if (NameInQuestion && NameInQuestion->IsMember() && !m_fProcessMethods)
    {
        return (NameInQuestion->PMember()->IsBracketed() == true);
    }

    return false;
}

//****************************************************************************
// Does nothing is we are not in debug mode, or asserts if we are on debug mode.
//****************************************************************************
HRESULT XMLGen::DebAssertHelper(_In_z_ PCWCH VSASSERTString)
{
#if DEBUG
    if (AssertConditionSet)
    {
        VSFAIL(VSASSERTString);
    }
#endif

    m_fShouldQuote = true;
    return NOERROR;
}

#if DEBUG
//****************************************************************************
// Removes '[' and ']' form things like System.[Integer]
//****************************************************************************
void XMLGen::RemoveBrackets(_Inout_z_ WCHAR *pText)
{
    WCHAR *pText0, *pText1;
    pText0 = pText1 = pText;

    while (pText0[0])
    {
        while (pText0[0] == L'[' || pText0[0] == L']')
        {
            ++pText0;
        }

        pText1[0] = pText0[0];

        if (!pText0[0])
        {
            break;
        }

        ++pText0, ++pText1;
    }

    pText1[0] = UCH_NULL;
}
#endif DEUBG

//****************************************************************************
// Generates an ref ID attribute for any given BillTree. The tree is expected to
// have a .Loc member that we c----e to generate the IDs.
//
// Return: Success or catstrophic failure code..
//****************************************************************************
void XMLGen::GenerateIDRef(StringBuffer *psbBuffer, ILTree::PILNode PILNode)
{
    WCHAR wszId[cbIdSize];

    CodeFile::MakeId(PILNode, wszId, _countof(wszId));         // Use the CodeFile ID generator service
    GenerateGenericElement(psbBuffer, ELEMENT_IdReference, wszId, CHECK_ENTITY_REF, NO_BRACKETS);
}

//****************************************************************************
// Returns the type of a member.
//****************************************************************************
BCSYM *XMLGen::TypeOf(BCSYM_NamedRoot *psym)
{
    if (psym && psym->IsMember())
    {
        return psym->PMember()->GetType();
    }

    return NULL;
}

//****************************************************************************
// ;GetEndLocationForImplementsAndInherits
//
//  Returns the span for Implements and inherits statements on a Class/Module.
//****************************************************************************
void XMLGen::GetEndLocationForImplementsAndInherits(ParseTree::StatementList *pClassChildren, Location *pLoc)
{
    Location endLoc;

    endLoc.Invalidate();    // The end of the span

    VSASSERT(pClassChildren, L"Must be passed a children list");      // make sure it is good

    ParseTree::Statement *pElement = pClassChildren->Element;

    // Iterate over all inherits and implements statements and figure out where
    // the overall span is
    while (pElement && (pElement->Opcode == ParseTree::Statement::Inherits ||
           pElement->Opcode == ParseTree::Statement::Implements))
    {
        if (pElement->TextSpan.StartsAfter(&endLoc))
        {
            memcpy(&endLoc, &(pElement->TextSpan), sizeof(Location));  // Copy all fields
        }

        pClassChildren = pClassChildren->NextInBlock;
        pClassChildren ? pElement = pClassChildren->Element : pElement = NULL;
    }

    if (endLoc.IsValid())
    {
        memcpy(pLoc, &(endLoc), sizeof(Location));  // Copy all fields
    }
}

//****************************************************************************
// ;GetEndLocationForImplementsAndInherits
//
// Returns the span for Implements and handles statements on a Method.
//****************************************************************************
void XMLGen::GetEndLocationForImplementsAndHandles(ParseTree::MethodDeclarationStatement *pMethod,
                                                   Location *pLoc)
{
    // If there is either a handles or an implements
    if (pMethod->Handles || pMethod->Implements)
    {
        // Set the location of the initial keyword
        pLoc->m_lBegLine = pMethod->TextSpan.m_lBegLine + pMethod->HandlesOrImplements.Line;
        pLoc->m_lBegColumn = pMethod->TextSpan.m_lBegColumn;

        // If there is a handles, set the end location.
        if (pMethod->Handles)
        {
            pLoc->m_lEndLine = pMethod->Handles->TextSpan.m_lEndLine;
            pLoc->m_lEndColumn = pMethod->Handles->TextSpan.m_lEndColumn;
        }

        // If there is an implements, and it ends after the current location,
        // set the new end location.
        if (pMethod->Implements && pMethod->Implements->TextSpan.EndsAfter(pLoc))
        {
            pLoc->m_lEndLine = pMethod->Implements->TextSpan.m_lEndLine;
            pLoc->m_lEndColumn = pMethod->Implements->TextSpan.m_lEndColumn;
        }
    }
}

//****************************************************************************
// Generates a fully qualified name using the BCSYM node.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::GenerateFullName(StringBuffer *psbBuffer, BCSYM_NamedRoot *pBCSYM, bool fProcessSourceName)
{
    HRESULT hr = NOERROR;

    if (pBCSYM)
    {

        STRING *QualifiedName = pBCSYM->GetQualifiedName(false);

        IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Full_Name, QualifiedName, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (fProcessSourceName)
        {
            StringBuffer SourceNameBuffer;
            m_NameStack.GetBracketedDotQualifiedName(&SourceNameBuffer);

            if (wcscmp(QualifiedName, SourceNameBuffer.GetString()))
            {
#if DEBUG
                BSTR UnbracketedSourceName = SysAllocStringLen(SourceNameBuffer.GetString(), SourceNameBuffer.GetStringLength());

                if (UnbracketedSourceName)
                {
                    RemoveBrackets((WCHAR*)UnbracketedSourceName);

                    VSASSERT(CompareNoCase(QualifiedName, (WCHAR*)UnbracketedSourceName) == 0, L"sourcename and fullname attributes differ by more than brackets!");
                    SysFreeString(UnbracketedSourceName);
                }
                else
                {
                    VSFAIL(L"SysAllocStringLen failed");
                }
#endif DEBUG

                IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Source_Name, SourceNameBuffer.GetString(), NO_ENTITY_REF_CHECK, NO_BRACKETS));
            }
        }
    }

Error:
    return hr;
}

//****************************************************************************
// Generates a line number attribute for designer's use.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::GenerateLineNumber(StringBuffer *psbBuffer, unsigned LineNumber)
{
    // and just convert the hash value to string so that we can dump it
    WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};      // 64-bit int can only take a maximum of 20 chars (will work for 64-bit ints!)

    _itow_s(LineNumber, wszIntValue, _countof(wszIntValue), 10);

    // And dump it as a "line" attribute
    return GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Line, wszIntValue, NO_ENTITY_REF_CHECK, NO_BRACKETS);
}

//****************************************************************************
// Generates a unique key for this named BCSYM. The key is based on the fully
// qualified name of that symbol, so if the symbol moves around, the Id will
// change, but the key should stay the same.
//
// We only need to use with the symbol (second param) or the string (last param)
// to generate the key. We use the sting if it is there, or hte symbol if it is not.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::GenerateKey(StringBuffer *psbBuffer, BCSYM_NamedRoot *pnamed, _In_opt_z_ WCHAR *inString)
{
    HRESULT hr = NOERROR;
    STRING_INFO *hashValue = NULL;
    STRING *pQualifiedName = NULL;

    if (!inString && !pnamed)
    {
        DebAssertHelper(L"Can't generate a key w/o having a BCSYM_NamedRoot");
        m_fShouldQuote = false;     // We don't want to quote this thing just because we can generate a key!

        return hr;
    }

    if (pnamed)
    {
        // Get the qualified name for this NamedRoot if it is passed in.
        pQualifiedName = pnamed->GetQualifiedName();
    }

    // If we are passed in a string, we append the qualified name to that string and use that to generate the key
    if (inString)
    {
        STRING *pstrLongString = NULL;

        if (pQualifiedName)
        {
            pstrLongString = m_pCompiler->ConcatStrings(WIDE("<"), pQualifiedName, WIDE(">"), inString);
        }
        else
        {
            pstrLongString = m_pCompiler->AddString(inString);
        }

        // and use its stringpool hash value to generate a unique key
        hashValue = StringPool::Pstrinfo(pstrLongString);
    }
    else
    {
        if (!pQualifiedName)
        {
            VSFAIL(L"Qualified name for a BCSYM_NamedRoot is NULL!");
            m_fShouldQuote = false;     // We don't want to quote this thing just because we can generate a key!
            return hr;
        }

        // and use its stringpool hash value to generate a unique key
        hashValue = StringPool::Pstrinfo(pQualifiedName);
    }

    // and just convert the hash value to string so that we can dump it
    WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};      // 64-bit int can only take a maximum of 20 chars (will work for 64-bit ints!)
    _i64tow_s((__int64)hashValue, wszIntValue, _countof(wszIntValue), 10);

    // And dump it as a "key" attribute
    IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Key, wszIntValue, NO_ENTITY_REF_CHECK, NO_BRACKETS));

Error:
    return hr;
}

//****************************************************************************
// Processes a qualified or a simple name given a ParseTree::Name
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessName(StringBuffer *psbBuffer, ParseTree::Name *pname,
                            ULONG depthLevel, bool UseBrackets)
{
    HRESULT hr = NOERROR;

    if (!pname)
    {
        m_fShouldQuote = true;
        return NOERROR;
    }

    switch (pname->Opcode)
    {
    case ParseTree::Name::Simple :
        {
            if (UseBrackets && pname->AsSimple()->ID.IsBracketed)
            {
                AppendChar(psbBuffer, CHAR_OpenBracket, NO_INDENTATION, NO_ENTITY_REF_CHECK);
            }

            AppendString(psbBuffer, pname->AsSimple()->ID.Name, NO_INDENTATION, NO_ENTITY_REF_CHECK);

            if (UseBrackets && pname->AsSimple()->ID.IsBracketed)
            {
                AppendChar(psbBuffer, CHAR_CloseBracket, NO_INDENTATION, NO_ENTITY_REF_CHECK);
            }

            break;
        }

    case ParseTree::Name::Qualified :
        {
            IfFailGo(ProcessName(psbBuffer, pname->AsQualified()->Base, depthLevel, UseBrackets));
            IfTrueRet(m_fShouldQuote, hr);

            AppendChar(psbBuffer, '.', NO_INDENTATION, NO_ENTITY_REF_CHECK);

            if (UseBrackets && pname->AsQualified()->Qualifier.IsBracketed)
            {
                AppendChar(psbBuffer, CHAR_OpenBracket, NO_INDENTATION, NO_ENTITY_REF_CHECK);
            }

            AppendString(psbBuffer, pname->AsQualified()->Qualifier.Name, NO_INDENTATION, NO_ENTITY_REF_CHECK);

            if (UseBrackets && pname->AsQualified()->Qualifier.IsBracketed)
            {
                AppendChar(psbBuffer, CHAR_CloseBracket, NO_INDENTATION, NO_ENTITY_REF_CHECK);
            }

            break;
        }
    case ParseTree::Name::SimpleWithArguments :
    {
        //just avoid the assert and return....
        m_fShouldQuote = true;
        return hr;
    }
    default :
        VSFAIL(L"Unknown ParseTree::Name Opcode");
        m_fShouldQuote = true;
        return hr;
    }

Error:
    return hr;
}

//****************************************************************************
// Tries to find the BCSYM associated with a parsetree name or a WCHAR string.
// Note: the last argument is used to append an optional string to the name
// before trying to resovle that name. This appending will only work if we are
// given the ParseTree::Name, not the wszInString string. Primarly, this us
// used for attribute names "BobAttribute".
//
// Return: BCSY if found, or NULL if not.
//****************************************************************************
HRESULT XMLGen::GetSymbolFromName(ParseTree::Name *pname, _In_opt_z_ WCHAR *wszInString,
                                  BCSYM **ppbcsym, NameFlags Flags, _In_opt_z_ const WCHAR *pStringToAppend)
{
    HRESULT hr = NOERROR;

    BCSYM_NamedRoot *psymName = NULL;     // BCSYM to return
    bool fBadName = false;

    StringBuffer tempBuffer;
    *ppbcsym = NULL;

    // If the name is good, use it
    if (pname)
    {
        IfFailGo(ProcessName(&tempBuffer, pname, NO_INDENTATION, NO_BRACKETS));
        IfTrueRet(m_fShouldQuote, hr);

        if (pStringToAppend)
        {
            tempBuffer.AppendString(pStringToAppend);
        }
    }

    // Use either the ParseTree::Name or the string passed in
    if (m_pCompilerProject && (tempBuffer.GetStringLength() > 0 || wszInString))
    {
        ULONG ulItems = 0;
        STRING **rgpstrQualifiedName = NULL;
        BCSYM_NamedRoot *pSymObject = NULL;
        NorlsAllocator nraSymbols(NORLSLOC);
        Symbols symbols(m_pCompiler, &nraSymbols, NULL);

        // Breakup the qualified name
        if (pname)
        {
            IfFailGo(BreakUpQualifiedName(m_pCompiler, &rgpstrQualifiedName, &nraSymbols, &ulItems,
                     tempBuffer.GetString(), DOT_DELIMITER));
        }
        else
        {
            IfFailGo(BreakUpQualifiedName(m_pCompiler, &rgpstrQualifiedName, &nraSymbols, &ulItems,
                     wszInString, DOT_DELIMITER));
        }

        // the array of names is empty
        if (ulItems <= 0 || !rgpstrQualifiedName)
        {
            return hr;
        }

        BCSYM_Hash *pScope = ContextOf(m_pCurrentContext);

        if (!pScope)
        {
            pScope = m_pCompiler->GetUnnamedNamespace(m_pSourceFile)->GetHash();
        }

        Location Loc;           // Dummy location.
        Loc.Invalidate();

        // Do the actual binding
        CompilationCaches *pCompilationCaches = GetCompilerCompilationCaches();

        psymName = Semantics::EnsureNamedRoot(
            Semantics::InterpretQualifiedName(
                rgpstrQualifiedName,
                ulItems,
                NULL,
                NULL,
                pScope,
                Flags | NameSearchIgnoreExtensionMethods,
                Loc,
                NULL,
                m_pCompiler,
                m_pCompilerProject->GetCompilerHost(),
                pCompilationCaches,
                m_pSourceFile,
                true,          // perform Obsolete checks
                fBadName));
    }

Error:

    if (fBadName == false)
    {
        *ppbcsym = psymName;
    }

    return hr;
}

//****************************************************************************
// Generate an XML attribute. This is generic enough to work with any attribute
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::GenerateGenericAttributeFromName(StringBuffer *psbBuffer,
                                                 _In_opt_z_ PCWCH wszAttribName,
                                                 ParseTree::Name *pname,
                                                 bool UseBrackets)
{
    HRESULT hr = NOERROR;

    // Nothing to do if we don't have an attribute name!
    if (!wszAttribName || !pname)
    {
        return hr;
    }

    // Dump the name of the attribute first
    AppendChar(psbBuffer, L' ', NO_INDENTATION, NO_ENTITY_REF_CHECK);
    AppendString(psbBuffer, wszAttribName, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    AppendString(psbBuffer, L"=\"", NO_INDENTATION, NO_ENTITY_REF_CHECK);

    IfFailGo(ProcessName(psbBuffer, pname, NO_INDENTATION, UseBrackets))
    AppendChar(psbBuffer, L'"', NO_INDENTATION, NO_ENTITY_REF_CHECK);

Error:
    return hr;
}

//****************************************************************************
// Generate an XML attribute. This is generic enough to work with any attribute
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::GenerateGenericAttribute(StringBuffer *psbBuffer, _In_opt_z_ PCWCH wszAttribName,
                                         _In_opt_z_ PCWCH wszAttribValue, bool fEntityRefCheck,
                                         bool IsBracketed)
{
    // Nothing to do if we don't have an attribute name!
    if (!wszAttribName || !wszAttribValue)
    {
        return E_FAIL;
    }

    // Dump the name of the attribute first
    AppendChar(psbBuffer, ' ', NO_INDENTATION, NO_ENTITY_REF_CHECK);
    AppendString(psbBuffer, wszAttribName, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    AppendString(psbBuffer, L"=\"", NO_INDENTATION, NO_ENTITY_REF_CHECK);

    if (IsBracketed)
    {
        AppendChar(psbBuffer, CHAR_OpenBracket, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }

    // Then dump the value of the attribute
    AppendString(psbBuffer, wszAttribValue, NO_INDENTATION, fEntityRefCheck);

    if (IsBracketed)
    {
        AppendChar(psbBuffer, CHAR_CloseBracket, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }

    AppendChar(psbBuffer, '"', NO_INDENTATION, NO_ENTITY_REF_CHECK);

    return NOERROR;
}

//****************************************************************************
// Generates a generic XML element with an arbitrary depth level. Depth level
// refers to the number of tabs we use to indent. If the element name or the
// element value is NULL, we only generate the the one that is not NULL.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
void XMLGen::GenerateGenericElement(StringBuffer *psbBuffer, _In_opt_z_ PCWCH wszElementName,
                                       _In_opt_z_ PCWCH wszElementValue, bool fEntityRefCheck,
                                       bool IsBracketed)
{
    // We generate a <ThisReference> element instead of the usual element for "Me"
    if (wszElementName && !_wcsicmp(wszElementName, ELEMENT_This_Reference))
    {
        AppendString(psbBuffer, L"<ThisReference />", NO_INDENTATION, NO_ENTITY_REF_CHECK);
        return;
    }

    // We generate a <BaseReference> element instead of the usual element for "MyBase"
    if (wszElementName && !_wcsicmp(wszElementName, ELEMENT_Base_Reference))
    {
        AppendString(psbBuffer, L"<BaseReference />", NO_INDENTATION, NO_ENTITY_REF_CHECK);
        return;
    }

    // Then we generate the <elementname> tag
    if (wszElementName)
    {
        AppendChar(psbBuffer, CHAR_XMLOpenTag,  NO_INDENTATION, NO_ENTITY_REF_CHECK);
        AppendString(psbBuffer, wszElementName, NO_INDENTATION, NO_ENTITY_REF_CHECK);
        AppendChar(psbBuffer, CHAR_XMLCloseTag, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }

    // Generate the element value
    if (wszElementValue)
    {
        if (IsBracketed)
        {
            AppendChar(psbBuffer, CHAR_OpenBracket, NO_INDENTATION, NO_ENTITY_REF_CHECK);
        }

        AppendString(psbBuffer, wszElementValue, NO_INDENTATION, fEntityRefCheck);

        if (IsBracketed)
        {
            AppendChar(psbBuffer, CHAR_CloseBracket, NO_INDENTATION, NO_ENTITY_REF_CHECK);
        }
    }

    // Then we generate the </elementname> tag
    if (wszElementName)
    {
        AppendString(psbBuffer, L"</", NO_INDENTATION, NO_ENTITY_REF_CHECK);
        AppendString(psbBuffer, wszElementName, NO_INDENTATION, NO_ENTITY_REF_CHECK);
        AppendChar(psbBuffer, '>', NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }
}

//****************************************************************************
// Return true if this a "Me" reference, or false if it is not
//****************************************************************************
bool XMLGen::IsThisReference(BCSYM_NamedRoot *pnamed)
{
    if (pnamed && pnamed->IsVariable() && pnamed->PVariable()->IsMe())
    {
        return true;
    }

    return false;
}

//****************************************************************************
// Generates a generic XML literal element with an arbitrary depth level.
// Depth level refers to the number of tabs we use to indent.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::GenerateLiteral(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    AppendString(psbBuffer, L"<Literal>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        switch (PILNode->bilop)
        {
        case SX_CNS_STR :
            GenerateGenericElement(psbBuffer, ELEMENT_String, PILNode->AsStringConstant().Spelling, CHECK_ENTITY_REF, NO_BRACKETS);
            break;

        case SX_CNS_INT :
            {
                if (PILNode->AsIntegralConstantExpression().vtype == t_char)
                {
                    AppendString(psbBuffer, L"<Char>", depthLevel + 1, NO_ENTITY_REF_CHECK);
                    AppendChar(psbBuffer, (WCHAR)PILNode->AsIntegralConstantExpression().Value, NO_INDENTATION);
                    AppendString(psbBuffer, L"</Char>" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
                }
                else if (PILNode->AsIntegralConstantExpression().vtype == t_bool)
                {
                    if (PILNode->AsIntegralConstantExpression().Value == 0)
                    {
                        GenerateGenericElement(psbBuffer, ATTRIB_VALUE_Boolean, ATTRIB_VALUE_False, CHECK_ENTITY_REF, NO_BRACKETS);
                    }
                    else
                    {
                        GenerateGenericElement(psbBuffer, ATTRIB_VALUE_Boolean, ATTRIB_VALUE_True, CHECK_ENTITY_REF, NO_BRACKETS);
                    }
                }
                else
                {
                    IfFailGo(GenerateNumberLiteral(psbBuffer, PILNode, depthLevel + 1));
                }

                break;
            }

        case SX_CNS_DEC :
        case SX_CNS_FLT :
            IfFailGo(GenerateNumberLiteral(psbBuffer, PILNode, depthLevel + 1));
            break;

        case SX_NOTHING :
            GenerateGenericElement(psbBuffer, ELEMENT_Null, NULL, CHECK_ENTITY_REF, NO_BRACKETS);
            break;

        default :
            VSFAIL(L"Unknown literal type");
            m_fShouldQuote = true;
            return hr;
        }
    }
    AppendString(psbBuffer, L"</Literal>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    return hr;
}

//****************************************************************************
// Generates a generic XML number element with an arbitrary depth level.
// Depth level refers to the number of tabs we use to indent.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::GenerateNumberLiteral(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    AppendString(psbBuffer, L"<Number", depthLevel, NO_ENTITY_REF_CHECK);
    {
        if (PILNode->uFlags & SXF_ICON_HEX)
        {
            IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Base, ATTRIB_VALUE_16, NO_ENTITY_REF_CHECK, NO_BRACKETS));    // Number is in Hex
        }
        else
        {
            if (PILNode->uFlags & SXF_ICON_OCT)
            {
                IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Base, ATTRIB_VALUE_8, NO_ENTITY_REF_CHECK, NO_BRACKETS));     // Number is in Oct
            }
            else
            {
                IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Base, ATTRIB_VALUE_10, NO_ENTITY_REF_CHECK, NO_BRACKETS));    // Number is in Dec
            }
        }

        // We are marking floats as having scientific notation
        if (PILNode->bilop == SX_CNS_FLT)
        {
            IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Scientificnotation, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Type, g_wszFullCLRNameOfVtype[PILNode->AsExpression().vtype], NO_ENTITY_REF_CHECK, NO_BRACKETS));
        AppendChar(psbBuffer, '>', NO_INDENTATION, NO_ENTITY_REF_CHECK);

        switch (PILNode->bilop)
        {
        case SX_CNS_INT :
            {
                WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};      // 64-bit int can only take a maximum of 20 chars + sign + '\0'
                if(PILNode->AsExpression().vtype == t_ui8)
                {
                    _ui64tow_s(PILNode->AsIntegralConstantExpression().Value, wszIntValue, _countof(wszIntValue), 10);
                }
                else
                {
                    _i64tow_s(PILNode->AsIntegralConstantExpression().Value, wszIntValue, _countof(wszIntValue), 10);
                }

                AppendString(psbBuffer, wszIntValue, NO_INDENTATION);
                break;
            }

        case SX_CNS_DEC :
            {
                BSTR bstrValue = NULL;

                IfFailThrow(VarBstrFromDec(&PILNode->AsDecimalConstantExpression().Value,
                    LCID_US_ENGLISH,
                    LOCALE_NOUSEROVERRIDE,
                    &bstrValue));

                AppendString(psbBuffer, bstrValue, NO_INDENTATION);
                SysFreeString(bstrValue);
                break;
            }

        case SX_CNS_FLT :
            {
                BSTR bstrValue = NULL;

                IfFailGo(R8ToBSTR(PILNode->AsFloatConstantExpression().Value, &bstrValue));
                AppendString(psbBuffer, bstrValue, NO_INDENTATION);
                SysFreeString(bstrValue);
                break;
            }

        default :
            VSFAIL(L"Unknown literal kind");
            m_fShouldQuote = true;
            return hr;
        }
    }
    AppendString(psbBuffer, L"</Number>" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

Error:
    return hr;
}

//****************************************************************************
// Generates an access attribute. The access is gathered from the BCSYM that
// corresponds to the bilTree we ae looking at, and we don;t find one, we
// just dump "Unknown"
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::GenerateAccess(StringBuffer *psbBuffer, BCSYM_NamedRoot *pnamed)
{
    HRESULT hr = NOERROR;

    if (!pnamed)
    {
        // If we can't find it, we just go with "Unknown"
        return GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Access, ATTRIB_VALUE_Unknown, NO_ENTITY_REF_CHECK, NO_BRACKETS);
    }

    // Get the access bits and generate the corresponding access strings
    switch (pnamed->PNamedRoot()->GetAccess())
    {
    case ACCESS_Public :
        IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Access, ATTRIB_VALUE_Public, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        break;

    case ACCESS_Private :
        IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Access, ATTRIB_VALUE_Private, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        break;

    case ACCESS_Friend :
        IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Access, ATTRIB_VALUE_Internal, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        break;

    case ACCESS_Protected :
        IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Access, ATTRIB_VALUE_Protected, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        break;

    case ACCESS_ProtectedFriend :
        IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Access, ATTRIB_VALUE_ProtectedFriend, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        break;

    default :
        IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Access, ATTRIB_VALUE_Unknown, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

Error:
    return hr;
}

//****************************************************************************
// Checks for '<', '>', '&', '\'' and '"' and replaces them with
//             &lt, &gt, &amp, &apos, &quot respectively
// Note: We are allocating a WCHAR string here and we are not freeing it.
// It is up to the caller to free this string, since the caller is the consumer
// of that string.
//****************************************************************************
void XMLGen::CheckForEntityReferences(_In_z_ PCWCH wszInString,
                                         _Deref_out_z_ WCHAR **pwszOutString,
                                         bool fProcessMethods)
{
    StringBuffer tempBuffer;

    // Attempt to get a big-enough buffer before we begin appending
    tempBuffer.AllocateSize(VBMath::Multiply(
        VBMath::Add(wcslen(wszInString), 32), 
        sizeof(WCHAR))); 

    for (WCHAR *pchCurrent = (WCHAR *)wszInString; *pchCurrent; ++pchCurrent)
    {
        switch (*pchCurrent)
        {
        case 0x0022 : // L"\""
            tempBuffer.AppendString(L"&quot;");
            break;

        case 0x0026 : // L"&"
            tempBuffer.AppendString(L"&amp;");
            break;

        case 0x0027 : // L"\'"
            tempBuffer.AppendString(L"&apos;");
            break;

        case 0x003c : // L"<"
            tempBuffer.AppendString(L"&lt;");
            break;

        case 0x003e : // L">"
            tempBuffer.AppendString(L"&gt;");
            break;

        default :
            {
                // Skip invalid XML characters. Valid XML characters are:
                // Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]
                // any Unicode character, excluding the surrogate blocks, FFFE, and FFFF.

                // We are using the Private-User-Area (0xE000 - 0xE01F) for characters from 0x0000 to 0x001F

                if (*pchCurrent < FirstPrintableChar)
                {
                    tempBuffer.AppendChar(*pchCurrent | START_OF_ENCODING_PRIVATE_USER_AREA);
                    continue;
                }

                if (*pchCurrent > LastValidCharacter)
                {
                    continue;
                }

                // VSW#139228 - For chinese we need to allow surrogate pairs.
                // This is a deviation from the XML spec.
                // if (*pchCurrent > 0xD7FF && *pchCurrent < START_OF_ENCODING_PRIVATE_USER_AREA)
                // {
                //     continue;
                // }

                // Designers use their own parser to parse the XML we generate, so we don't want to encode spaces
                // for them, however, all other clients use msxml.dll, which eats up the spaces.

                if (fProcessMethods == NO_METHODS && *pchCurrent == FirstPrintableChar)
                {
                    tempBuffer.AppendChar(*pchCurrent | START_OF_ENCODING_PRIVATE_USER_AREA);
                    continue;
                }

                tempBuffer.AppendChar(*pchCurrent);
                break;
            }
        }
    }

    size_t outStringLength = tempBuffer.GetStringLength();
    *pwszOutString = new (zeromemory) WCHAR[outStringLength + 1];

    memcpy(*pwszOutString, tempBuffer.GetString(), outStringLength * sizeof(WCHAR));
    (*pwszOutString)[outStringLength] = UCH_NULL;
}

//****************************************************************************
// A general append a string at the end of a StringBuffer
//
// Return: Success or catstrophic failure code.
//****************************************************************************
void XMLGen::AppendString(StringBuffer *psbBuffer, _In_opt_z_ PCWCH wszBuffer, ULONG depthLevel, bool fEntityRefCheck)
{
    // Sometimes we get called with really big number for indentation, so check for this case here.
    // VSASSERT(depthLevel < 1000, "Looks like we have an infinit loop");

    // Save our time here
    if (!wszBuffer || wszBuffer[0] == UCH_NULL)
    {
        return;
    }

    // Then, append the string
    // If we need to check for entity references, we can do it right now.
    if (fEntityRefCheck == CHECK_ENTITY_REF)
    {
        WCHAR *entityString = NULL;

        CheckForEntityReferences(wszBuffer, &entityString, m_fProcessMethods);
        psbBuffer->AppendString(entityString);

#if DEBUG
        // We want to break at this point, do it now.
        if (BreakAtName &&
            entityString &&
            CompareNoCase(BreakAtName, entityString) == 0)
        {
            // Allocate enough space for the text message + the string we are appending to it
            WCHAR *tempString;
            size_t cchLength = wcslen(entityString) + 30;

            tempString = new (zeromemory) WCHAR[cchLength];
            swprintf_s(tempString, cchLength, L"Element Name matches: \"%s\"", entityString);
            VSFAIL(tempString);

            delete[] tempString;
        }
#endif

        delete[] entityString;
    }
    else
    {
        psbBuffer->AppendString(wszBuffer);

#if DEBUG
        // We want to break at this point, do it now.
        if (BreakAtName &&
            wszBuffer &&
            CompareNoCase(BreakAtName, wszBuffer) == 0)
        {
            // Allocate enough space for the text message + the string we are appending to it
            WCHAR *tempString;
            size_t cchLength = wcslen(wszBuffer) + 30;

            tempString = new (zeromemory) WCHAR[cchLength];
            swprintf_s(tempString, cchLength, L"Element Name matches: \"%s\"", wszBuffer);
            VSFAIL(tempString);

            delete[] tempString;
        }
#endif
    }
}

//****************************************************************************
// A general append a character at the end of a StringBuffer
//
// Return: Success or catstrophic failure code.
//****************************************************************************
void XMLGen::AppendChar(StringBuffer *psbBuffer, WCHAR wch, ULONG depthLevel, bool fEntityRefCheck)
{
    WCHAR CharString[2];

    // Sometimes we get called with really big number for indentation, so check for this case here.
    VSASSERT(depthLevel < 10000, "Indenting more than 10,000 tabs! Is this OK?");

    // Save our time here
    if (wch == UCH_NULL)
    {
        return;
    }

    CharString[0] = wch;
    CharString[1] = UCH_NULL;

    // Then, append the character
    // If we need to check for entity references, we can do it right now.
    if (fEntityRefCheck == CHECK_ENTITY_REF)
    {
        WCHAR *entityString = NULL;

        CheckForEntityReferences(CharString, &entityString, m_fProcessMethods);
        psbBuffer->AppendString(entityString);

#if DEBUG
        if (BreakAtName)
        {
            // Check to see if debug name matches
            VSASSERT(CompareNoCase(BreakAtName, entityString) != 0, "Character matches!");
        }
#endif

        delete[] entityString;
    }
    else
    {
        psbBuffer->AppendString(CharString);

#if DEBUG
        if (BreakAtName)
        {
            // Check to see if debug name matches
            VSASSERT(CompareNoCase(BreakAtName, CharString) != 0, "Character matches!");
        }
#endif
    }
}

//============================================================================
// Processes a linked list of statements
//
// Return: Success or catstrophic failure code.
//============================================================================
HRESULT XMLGen::ProcessStatementList(StringBuffer *psbBuffer, ParseTree::StatementList *ptreeList, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Loop over all children, processing one at a time.
    for ( ; ptreeList; ptreeList = ptreeList->NextInBlock)
    {
        if (ptreeList->Element)
        {
            IfFailGo(MetaProcessParseTree(psbBuffer, ptreeList, depthLevel));
            IfTrueRet(m_fShouldQuote, hr);
        }
    }

Error:
    return hr;
}

//============================================================================
// Processes a linked list of types
//
// Return: Success or catstrophic failure code.
//============================================================================
HRESULT XMLGen::ProcessTypeList(StringBuffer *psbBuffer, ParseTree::TypeList *ptreeList, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Loop over all children, processing one at a time.
    for ( ; ptreeList; ptreeList = ptreeList->Next)
    {
        IfFailGo(ProcessTypeStatement(psbBuffer, ptreeList->Element, NULL, depthLevel));
        IfTrueRet(m_fShouldQuote, hr);
    }

Error:
    return hr;
}

//============================================================================
// Processes a linked list Named Params for an attribute
//
// Return: Success or catstrophic failure code.
//============================================================================
HRESULT XMLGen::ProcessAttributeArgumentList(StringBuffer *psbBuffer, ParseTree::ArgumentList *ptreeList, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Loop over all children, processing one at a time.
    for ( ; ptreeList; ptreeList = ptreeList->Next)
    {
        IfFailGo(ProcessAttributeProperty(psbBuffer, ptreeList->Element, depthLevel));
        IfTrueRet(m_fShouldQuote, hr);
    }

Error:
    return hr;
}

//============================================================================
// Processes a linked list of attributes
//
// Return: Success or catstrophic failure code.
//============================================================================
HRESULT XMLGen::ProcessAttributeList(StringBuffer *psbBuffer, ParseTree::AttributeSpecifierList *pAttributes, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (pAttributes == NULL)
    {
        return hr;
    }

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Attributes>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

    for(; pAttributes; pAttributes = pAttributes->Next)
    {
        ParseTree::AttributeSpecifier *pAttributeBlock = pAttributes->Element;

        ParseTree::AttributeList *ptreeList = pAttributeBlock->Values;

        {
            AppendString(&tempBuffer, L"<AttributeSection", depthLevel + 1, NO_ENTITY_REF_CHECK);
            {
                IfFailGo(GenerateStatementID(&tempBuffer, pAttributes->TextSpan, ID));

                // End Attributes
                AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

                // Loop over all children, processing one at a time.
                for (; ptreeList; ptreeList = ptreeList->Next)
                {
                    IfFailGo(ProcessAttribute(&tempBuffer, ptreeList->Element, depthLevel + 2));
                    IfTrueRet(m_fShouldQuote, hr);
                }
            }
            AppendString(&tempBuffer, L"</AttributeSection>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        }
    }
    AppendString(&tempBuffer, L"</Attributes>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//============================================================================
// Processes a linked list of variables
//
// Return: Success or catstrophic failure code.
//============================================================================
HRESULT XMLGen::ProcessVariableDeclaratorList(StringBuffer *psbBuffer,
                                              ParseTree::VariableDeclaration *pDeclaration,
                                              ParseTree::DeclaratorList *ptreeList,
                                              ParseTree::Initializer *pinitializer,
                                              ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Loop over all children, processing one at a time.
    for ( ; ptreeList; ptreeList = ptreeList->Next)
    {
        IfFailGo(ProcessVariableDeclaration(psbBuffer, pDeclaration, ptreeList->Element, pinitializer, depthLevel));
        IfTrueRet(m_fShouldQuote, hr);
    }

Error:
    return hr;
}

//============================================================================
// Processes a linked list of locals
//
// Return: Success or catstrophic failure code.
//============================================================================
HRESULT XMLGen::ProcessLocalDeclarationList(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    for (ILTree::PILNode ptreeListElement = PILNode->AsStatementWithExpression().ptreeOp1; ptreeListElement;
         ptreeListElement = ptreeListElement->AsExpressionWithChildren().Right)
    {
        IfFailGo(ProcessLocalDimStmt(psbBuffer, ptreeListElement, depthLevel));
        IfTrueRet(m_fShouldQuote, hr);
    }

Error:
    return hr;
}

//============================================================================
// Processes a linked list of parameters. We walk both the list of parameter
// symbols as well as parsetree params, and we pass this over to
// ProcessParameterDeclaration() which wil process each parameter.
//
// Return: Success or catstrophic failure code.
//============================================================================
HRESULT XMLGen::ProcessParameterList(StringBuffer *psbBuffer, ParseTree::ParameterList *ptreeList,
                                     BCSYM_Proc *pProcSymbol, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (!pProcSymbol)
    {
        // Nothing to do!
        return hr;
    }

    if (!pProcSymbol->IsProc())
    {
        VSFAIL(L"Bad Proc symbol");
        return hr;
    }

    BCSYM_Param *pParameterSymbol = pProcSymbol->GetFirstParam();

    // Loop over all children, processing one at a time.
    for ( ; pParameterSymbol; pParameterSymbol = pParameterSymbol->GetNext())
    {
#if DEBUG
        if (ptreeList)
        {
            VSASSERT(!memcmp(pParameterSymbol->GetLocation(), &(ptreeList->Element->Name->Name.TextSpan), sizeof(Location)),
                     L"ParameterList is out of order!!!");
        }
#endif DEBUG

        IfFailGo(ProcessParameterDeclaration(psbBuffer, ptreeList ? ptreeList->Element : NULL, pParameterSymbol, depthLevel));
        IfTrueRet(m_fShouldQuote, hr);

        if (ptreeList)
        {
            ptreeList = ptreeList->Next;
        }
    }

    VSASSERT(!ptreeList, "Looks like the symbollist of parameters is actually shorter than the parsetree list of parameters");

Error:
    return hr;
}

//============================================================================
// Processes a linked list of Initializers
//
// Return: Success or catstrophic failure code.
//============================================================================
HRESULT XMLGen::ProcessInitializerList(StringBuffer *psbBuffer, ParseTree::InitializerList *ptreeList,
                                       BCSYM *ptype,
                                       ULONG rankDepth,
                                       ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (!ptreeList)
    {
        return hr;
    }

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    // We are using tempBuffer2 to skip change the order in which we dump the elements. Basicly,
    // we need to be able to dump the "Bound" expression first, before the rest of the initializations
    StringBuffer tempBuffer2;
    tempBuffer2.SetAllocationSize(StringBuffer::AllocSize::Large);

    AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        AppendString(&tempBuffer, L"<NewArray>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        {
            // Process the type of the array
            IfFailGo(ProcessBCSYMArrayType(&tempBuffer, ptype, NULL, rankDepth, NULL, depthLevel + 2));

            ULONG bound = 0;

            // Loop over all children, processing one at a time.
            for ( ; ptreeList; ptreeList = ptreeList->Next)
            {
                ++bound;

                IfFailGo(ProcessInitializer(&tempBuffer2, ptreeList->Element, ptype, rankDepth + 1, depthLevel + 2));
                IfTrueRet(m_fShouldQuote, hr);
            }

            // We are generating the bound for the array, so we use a string to make the conversion
            WCHAR wszBound[MaxStringLengthForIntToStringConversion] = {0};
            _ultow_s(bound, wszBound, _countof(wszBound), 10);

            GenerateGenericElement(&tempBuffer, ELEMENT_Bound, wszBound, CHECK_ENTITY_REF, NO_BRACKETS);

            // the we just copy the initializations back where it belongs
            AppendString(&tempBuffer, tempBuffer2.GetString(), NO_INDENTATION, NO_ENTITY_REF_CHECK);
        }
        AppendString(&tempBuffer, L"</NewArray>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
    }
    AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//============================================================================
// Toplevel function for generating the declarations in a file
//
// Return: Success or catstrophic failure code.
//============================================================================
HRESULT XMLGen::ProcessDeclarations(StringBuffer *psbBuffer,
                                    ParseTree::BlockStatement *pnamespaceTree,
                                    ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (!pnamespaceTree)
    {
        return hr;
    }

    AppendString(psbBuffer, L"<Declarations>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        STRING *pDefaultNameSpace = m_pCompilerProject->GetDefaultNamespace();

        if (pDefaultNameSpace)
        {
            IfFailGo(ProcessRootNameSpace(psbBuffer, pDefaultNameSpace, pnamespaceTree->Children, depthLevel + 1));
        }
        else
        {
            // Go over children one at a time, and process them
            IfFailGo(ProcessStatementList(psbBuffer, pnamespaceTree->Children, depthLevel + 1));
        }
    }
    AppendString(psbBuffer, L"</Declarations>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    return hr;
}

//****************************************************************************
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::MetaProcessParseTree(StringBuffer *psbBuffer, ParseTree::StatementList *pstatementList, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    ParseTree::Statement *pstatement = pstatementList->Element;

    // We just let the case statments handle whatever we are diming
    IfFailGo(ProcessDeclStatementParseTree(psbBuffer, pstatement, depthLevel));

    // Let's see if we need to quote this
    if (m_fShouldQuote == true)
    {
        // Quote it here
        IfFailGo(ProcessQuote(psbBuffer, NULL, pstatementList, depthLevel));
        m_fShouldQuote = false;
    }

    // Process the comments attached to the statement.

    if (m_fProcessTypesOnly == PROCESS_ALL_DECLS)
    {
        for (ParseTree::CommentList *pComments = pstatement->Comments; pComments; pComments = pComments->Next)
        {
            IfFailGo(ProcessComment(psbBuffer, pComments->Element, depthLevel));
        }
    }

Error:
    return hr;
}

//****************************************************************************
// Main switch statment for calling up the right function that is responsible
// for processing a sub tree. Greate for recursion
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessDeclStatementParseTree(StringBuffer *psbBuffer,
                                              ParseTree::Statement *pstatement,
                                              ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // By putting this here, we don't have to check for NULL trees all over the place, just
    // in one spot
    if (!pstatement)
    {
        return hr;
    }

    // If we see an error at eny level, we go ahead and quote it.
    if (pstatement->HasSyntaxError)
    {
        m_fShouldQuote = true;
        return hr;
    }

    // If we are processing types only, we only need to look at the types,
    // otherwise, we look at everything.
    if (m_fProcessTypesOnly == PROCESS_TYPES_ONLY)
    {
        switch(pstatement->Opcode)
        {
        case ParseTree::Statement::Namespace :
            IfFailGo(ProcessNameSpace(psbBuffer, pstatement, FLAT_DEPTH_LEVEL));
            break;

        case ParseTree::Statement::Structure :
            IfFailGo(ProcessStructure(psbBuffer, pstatement, FLAT_DEPTH_LEVEL));
            break;

        case ParseTree::Statement::Interface :
            IfFailGo(ProcessInterface(psbBuffer, pstatement, FLAT_DEPTH_LEVEL));
            break;

        case ParseTree::Statement::Enum :
            IfFailGo(ProcessEnumDeclaration(psbBuffer, pstatement, FLAT_DEPTH_LEVEL));
            break;

        case ParseTree::Statement::Class :
        case ParseTree::Statement::Module :
            IfFailGo(ProcessClass(psbBuffer, pstatement, FLAT_DEPTH_LEVEL));
            break;

        case ParseTree::Statement::Enumerator :
        case ParseTree::Statement::EnumeratorWithValue :
            IfFailGo(ProcessEnumVariable(psbBuffer, pstatement, FLAT_DEPTH_LEVEL));
            break;
        }
    }
    else
    {
        switch(pstatement->Opcode)
        {
        case ParseTree::Statement::Namespace :
            IfFailGo(ProcessNameSpace(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::Structure :
            IfFailGo(ProcessStructure(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::Interface :
            IfFailGo(ProcessInterface(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::Enum :
            IfFailGo(ProcessEnumDeclaration(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::Class :
        case ParseTree::Statement::Module :
            IfFailGo(ProcessClass(psbBuffer, pstatement, depthLevel));
            break;

            // We already handle implements list within the class / struct / method
        case ParseTree::Statement::Implements :
            break;

        case ParseTree::Statement::Inherits :
            IfFailGo(ProcessInheritsStmt(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::Imports :
            IfFailGo(ProcessImportsStmt(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::EventDeclaration :
            IfFailGo(ProcessEventDeclaration(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::Enumerator :
        case ParseTree::Statement::EnumeratorWithValue :
            IfFailGo(ProcessEnumVariable(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::VariableDeclaration :
            IfFailGo(ProcessFieldDeclaration(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::DelegateProcedureDeclaration :
        case ParseTree::Statement::DelegateFunctionDeclaration :
            IfFailGo(ProcessDelegateDeclaration(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::ProcedureDeclaration :
        case ParseTree::Statement::FunctionDeclaration :
        case ParseTree::Statement::OperatorDeclaration :
            IfFailGo(ProcessProcedureDeclaration(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::ForeignProcedureDeclaration :
        case ParseTree::Statement::ForeignFunctionDeclaration :
        case ParseTree::Statement::ForeignFunctionNone :
            IfFailGo(ProcessExternalProcedureDeclaration(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::ConstructorDeclaration :
            IfFailGo(ProcessConstructorDeclaration(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::Property :
            IfFailGo(ProcessPropertyDeclaration(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::PropertyGet :
            IfFailGo(ProcessPropertyGet(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::PropertySet :
            IfFailGo(ProcessPropertySet(psbBuffer, pstatement, depthLevel));
            break;

        case ParseTree::Statement::BlockEventDeclaration:
        case ParseTree::Statement::AddHandlerDeclaration:
        case ParseTree::Statement::RemoveHandlerDeclaration:
        case ParseTree::Statement::RaiseEventDeclaration:
            // 
            break;

        case ParseTree::Statement::Region :
            {
                ParseTree::StatementList *pRegionStatementList = new (zeromemory) ParseTree::StatementList;

                // Fake a Statementlist node and use it to process the quote
                if (pRegionStatementList)
                {
                    pRegionStatementList->Element = pstatement;
                    ProcessQuote(psbBuffer, NULL, pRegionStatementList, depthLevel);
                    delete pRegionStatementList;
                }
            }
            break;

        case ParseTree::Statement::EndRegion :
            {
                ParseTree::StatementList *pRegionStatementList = new (zeromemory) ParseTree::StatementList;

                // Fake a Statementlist node and use it to process the quote
                if (pRegionStatementList)
                {
                    pRegionStatementList->Element = pstatement;
                    ProcessQuote(psbBuffer, NULL, pRegionStatementList, depthLevel);
                    delete pRegionStatementList;
                }
            }
            break;

        case ParseTree::Statement::Attribute :
            IfFailGo(ProcessAttributeList(psbBuffer, pstatement->AsAttribute()->Attributes, depthLevel + 1));
            break;

        case ParseTree::Statement::EndIf :
        case ParseTree::Statement::EndUsing :
        case ParseTree::Statement::EndWith :
        case ParseTree::Statement::EndSelect :
        case ParseTree::Statement::EndStructure :
        case ParseTree::Statement::EndEnum :
        case ParseTree::Statement::EndInterface :
        case ParseTree::Statement::EndClass :
        case ParseTree::Statement::EndModule :
        case ParseTree::Statement::EndNamespace :
        case ParseTree::Statement::EndSub :
        case ParseTree::Statement::EndFunction :
        case ParseTree::Statement::EndOperator :
        case ParseTree::Statement::EndProperty :
        case ParseTree::Statement::EndGet :
        case ParseTree::Statement::EndSet :
        case ParseTree::Statement::EndEvent:
        case ParseTree::Statement::EndAddHandler:
        case ParseTree::Statement::EndRemoveHandler:
        case ParseTree::Statement::EndRaiseEvent:
        case ParseTree::Statement::EndNext :
        case ParseTree::Statement::EndWhile:
        case ParseTree::Statement::EndLoop :
        case ParseTree::Statement::EndLoopWhile :
        case ParseTree::Statement::EndLoopUntil :
        case ParseTree::Statement::EndTry :
        case ParseTree::Statement::EndUnknown :
        case ParseTree::Statement::EndInvalid :
        case ParseTree::Statement::Empty :
            break;

        case ParseTree::Statement::CommentBlock :
            // CommentBlock are nothing but a list of comments, so just process these comments.
            IfFailGo(ProcessStatementList(psbBuffer, pstatement->AsCommentBlock()->Children, depthLevel));
            break;

        default :
            m_fShouldQuote = true;
            break;
        }
    }

Error:
    return hr;
}

//****************************************************************************
// Main switch statment for calling up the right function that is responsible
// for processing a blocks.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessMethodBodyStatement(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // By putting this here, we don't have to check for NULL trees all over the place, just
    // in one spot
    if (!PILNode)
    {
        return hr;
    }

    // If we see an error at eny level, we go ahead and quote it.
    if (PILNode->uFlags & SF_ERROR)
    {
        m_fShouldQuote = true;
        return hr;
    }

    switch(PILNode->bilop)
    {
    case SB_IF_BLOCK :
        if (PILNode->uFlags & SBF_IF_BLOCK_NO_XML)
        {
            // Skip this block and the single If contained within it, and process
            // the single statement within the If. (This condition arises only
            // for conditional guards synthesized around RaiseEvent calls.)
            ProcessMethodBodyStatement(psbBuffer, PILNode->AsIfGroup().Child->AsIfBlock().Child, depthLevel);
        }
        else
        {
            IfFailGo(ProcessIfBlock(psbBuffer, PILNode, depthLevel));
        }
        break;
    case SB_HIDDEN_CODE_BLOCK:
        IfFailGo(ProcessBlock(psbBuffer, PILNode, depthLevel));
        break;

    case SB_IF :
    case SB_ELSE_IF :
    case SB_ELSE :
        IfFailGo(ProcessIf_Else(psbBuffer, PILNode, depthLevel));
        break;

    case SB_FOR :
        IfFailGo(ProcessForLoop(psbBuffer, PILNode, depthLevel));
        break;

    case SB_FOR_EACH :
        IfFailGo(ProcessForEachLoop(psbBuffer, PILNode, depthLevel));
        break;

    case SB_LOOP :
        if (PILNode->uFlags & SBF_LOOP_WHILE)
        {
            IfFailGo(ProcessDoLoop(psbBuffer, PILNode, depthLevel));
        }
        else
        {
            IfFailGo(ProcessWhileLoop(psbBuffer, PILNode, depthLevel));
        }
        break;

    case SB_SELECT :
        IfFailGo(ProcessSelectBlock(psbBuffer, PILNode, depthLevel));
        break;

    case SB_WITH :
        IfFailGo(ProcessWithBlock(psbBuffer, PILNode, depthLevel));
        break;

    case SB_TRY :
        IfFailGo(ProcessTryBlock(psbBuffer, PILNode, depthLevel));
        break;

    case SB_CATCH :
    case SB_FINALLY :
        break;

    case SL_ERROR :
        IfFailGo(ProcessErrorStatement(psbBuffer, PILNode, depthLevel));
        break;

    case SL_ON_ERR :
        IfFailGo(ProcessOnError(psbBuffer, PILNode, depthLevel));
        break;

    case SL_RESUME :
        IfFailGo(ProcessResumeStatment(psbBuffer, PILNode, depthLevel));
        break;

    case SL_STMT :
        IfFailGo(ProcessExpressionStmt(psbBuffer, PILNode, depthLevel));
        break;

    case SL_GOTO :
        IfFailGo(ProcessGotoStmt(psbBuffer, PILNode, depthLevel));
        break;

    case SL_LABEL :
        IfFailGo(ProcessLabelStmt(psbBuffer, PILNode, depthLevel));
        break;

    case SL_CONTINUE:
        IfFailGo(ProcessContinueStatement(psbBuffer, PILNode, depthLevel));
        break;

    case SL_EXIT :
        IfFailGo(ProcessExitStatment(psbBuffer, PILNode, depthLevel));
        break;

    case SL_RETURN :
        IfFailGo(ProcessReturnStatment(psbBuffer, PILNode, depthLevel));
        break;

    case SL_STOP :
        IfFailGo(ProcessStopStatment(psbBuffer, PILNode, depthLevel));
        break;

    case SL_END :
        IfFailGo(ProcessEndStatment(psbBuffer, PILNode, depthLevel));
        break;

    case SL_VAR_DECL :
        IfFailGo(ProcessLocalDeclarationList(psbBuffer, PILNode, depthLevel));
        break;

    case SL_DEBUGLOCNOP:
        break;

    default :
        VSFAIL(L"Unknown Bound Tree Node Type");
        m_fShouldQuote = true;
        break;
    }

Error:
    return hr;
}

//****************************************************************************
// Generate a quote. This is pretty much any this we don't understand, so
// it is the default for the case statment above.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessQuote(StringBuffer *psbBuffer, ILTree::PILNode PILNode,
                             ParseTree::StatementList *pstatementList, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    long lBegLine, lEndLine, lBegColumn, lEndColumn;
    BSTR bstrText = NULL;       // We use this to get a range of text lines from the source file
    BSTR bstrTextCopy = NULL;   // A copy of the BSTR so se can free it later.

    ParseTree::Statement *pstatement = NULL;

    if (pstatementList)
    {
        pstatement = pstatementList->Element;
    }

    // Make sure we have something to work with first
    if (!PILNode && !pstatement)
    {
        VSFAIL(L"Bad args to ProcessQuote()");
        return hr;
    }

    // If we are not a statment level, we just set the quoting flag and return.
    // This is because we want to quote the largest possible chucnk of code. This
    // makes it possible to quote statement that have errors in them.

    if (PILNode && !PILNode->IsStmtNode() && m_fShouldQuote == false)
    {
        m_fShouldQuote = true;
        return hr;
    }

    // We want to be able to bail out if we have an empty quote.
    AppendStringBuffer tempBuffer(psbBuffer);

    size_t tempBufferLength = 0;

    AppendString(&tempBuffer, L"<Quote", depthLevel, NO_ENTITY_REF_CHECK);
    {
        if (PILNode)
        {
            // Get position info from BilTree
            lBegLine = PILNode->Loc.m_lBegLine;
            lEndLine = PILNode->Loc.m_lEndLine;
            lBegColumn = PILNode->Loc.m_lBegColumn;
            lEndColumn = PILNode->Loc.m_lEndColumn;
            IfFailGo(GenerateStatementID(&tempBuffer, PILNode->Loc, ID));
        }
        else
        {
            // Get position info from pstatement
            lBegLine = pstatement->TextSpan.m_lBegLine;
            lEndLine = pstatement->TextSpan.m_lEndLine;
            lBegColumn = pstatement->TextSpan.m_lBegColumn;
            lEndColumn = pstatement->TextSpan.m_lEndColumn;

            // For block statements, the  full span of a block is from the start of the
            // block statement through the end of the statement following the block (the end construct).
            if (pstatement->IsBlock() && pstatementList->NextInBlock)
            {
                ParseTree::Statement *pendConstruct = pstatementList->NextInBlock->Element;

                // We also want to quote the EndConstruct here
                lEndLine = pendConstruct->TextSpan.m_lEndLine;
                lEndColumn = pendConstruct->TextSpan.m_lEndColumn;
            }
            IfFailGo(GenerateStatementID(&tempBuffer, pstatement));
        }
        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        long line = 0;              // Line number for scanning the quoted text
        long column = 0;            // Column number for the same purpose
        Text *pTextFile = NULL;

        if (lEndLine < lBegLine && lEndLine > 0)
        {
            VSFAIL(L"Hey, Ending line number is < Begining line number!");
            m_fShouldQuote = false; // We want to recover from this, and we are already trying quoting this whole thing
                                    // we don't want to get into some nasty enless loop here.
#pragma prefast(suppress:25013, "Explicitly wanted here")
            return hr;  // We're explicitly not keeping the contents of the string buffer. Let the
                        // AppendStringBuffer implicitly rollback.
        }

        // Get a chunck of text from the source file
        IfFailGo(GetTextFile(&pTextFile));
        pTextFile->GetTextLineRange(lBegLine, lEndLine, &bstrText);

        IfNullGo(bstrText);

        bstrTextCopy = bstrText;
        bstrText += lBegColumn; // We start at the begining column

        // If we only need to append one character, then do it right here, no need to do more
        // work than needed.
        if (lBegLine == lEndLine && lBegColumn == lEndColumn)
        {
            AppendChar(&tempBuffer, bstrText[0], NO_INDENTATION, CHECK_ENTITY_REF);
            tempBufferLength = 1;
        }
        else
        {
            // Go over all quoted lines except for the last one, we process that one down below
            for (line = lBegLine; line < lEndLine; ++line)
            {
                bool fCR;

                // Goto the end of the line
                for (column = 0; !IsLineBreak(bstrText[column]); ++column);

                fCR = (bstrText[column] == UCH_CR);

                // Replace the return with a '\0'. this way we can print only this line
                bstrText[column] = UCH_NULL;
                // print that one line
                tempBufferLength += wcslen(bstrText);  // We only count useful stuff
                AppendString(&tempBuffer, bstrText, depthLevel + 1);
                AppendChar(&tempBuffer, UCH_LF, NO_INDENTATION, NO_ENTITY_REF_CHECK);

                // Move on to the next line
                bstrText += ++column;

                // And just skip over CRLF pairs
                if (fCR && bstrText[0] == UCH_LF)
                {
                    ++bstrText;
                }
            }

            // If we eatup the whole thing, we don't need to to any more work
            if (bstrText[0] != UCH_NULL)
            {
                if (lBegLine >= lEndLine)
                {
                    if (lEndColumn == lBegColumn)
                    {
                        // This is the case where we only have one line, so the for loop above doesn't get executed at all.
                        bstrText[min((lEndColumn - lBegColumn), (long)wcslen(bstrText))] = UCH_NULL;
                    }
                    else
                    {
                        bstrText[min((lEndColumn - lBegColumn) + 1, (long)wcslen(bstrText))] = UCH_NULL;
                    }
                }
                else
                {
                    // This is the last line of the quoted chunck of text. We need to only go as far as the last
                    // column, but not any thing beyond that, or as far as the end of the string if that is smaller.
                    bstrText[min(lEndColumn + 1, (long)wcslen(bstrText))] = UCH_NULL;
                }

                // Get rid of trailing returns
                for (column = (long)wcslen(bstrText) - 1; column >= 0 && IsLineBreak(bstrText[column]) ; --column)
                {
                    bstrText[column] = UCH_NULL;
                }

                // Now, we can print the last line. We first check to see of there is somthing to dump
                if (wcslen(bstrText) > 0)
                {
                    tempBufferLength += wcslen(bstrText);  // We only count useful stuff
                    AppendString(&tempBuffer, bstrText, depthLevel + 1);
                    AppendChar(&tempBuffer, UCH_LF, NO_INDENTATION, NO_ENTITY_REF_CHECK);
                }
            }
        }
    }
    AppendString(&tempBuffer, L"</Quote>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR && tempBufferLength > 0)
    {
        tempBuffer.Commit();
    }

    if (bstrTextCopy)
    {
        SysFreeString(bstrTextCopy);        // Freeup the BSTR we used
    }

    return hr;
}

//****************************************************************************
// Processes a comment tree
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessComment(StringBuffer *psbBuffer, ParseTree::Comment *pcomment, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // We want to be able to bail out if we have an empty quote.
    AppendStringBuffer tempBuffer(psbBuffer);
    size_t tempBufferLength = 0;

    // No Comment, no dump! this value however includes Rem or '
    if (pcomment->LengthInCharacters < 1)
    {
        return hr;
    }

    WCHAR *wszComment = NULL;       // Comment text
    size_t uLen = 0;                // Comment text length

    AppendString(&tempBuffer, L"<Comment", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Set up the comment string and generate the ID
        IfFailGo(GenerateStatementID(&tempBuffer, pcomment->TextSpan, ID));

        wszComment = pcomment->Spelling;
        uLen = pcomment->LengthInCharacters;

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        tempBufferLength += wcslen(wszComment);  // We only count useful stuff
        AppendString(&tempBuffer, wszComment, depthLevel + 1);
        AppendChar(&tempBuffer, '\n', NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }
    AppendString(&tempBuffer, L"</Comment>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR && tempBufferLength > 0)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Prcesses an Error statment
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessErrorStatement(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::StatementWithExpression bilErrorTree = PILNode->AsStatementWithExpression();

    AppendString(&tempBuffer, L"<Error>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // This is the expression for the error number
        IfFailGo(ProcessExpression(&tempBuffer, bilErrorTree.ptreeOp1, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Error>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Prcesses an OnError statment
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessOnError(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::OnErrorStatement bilOnErrorTree = PILNode->AsOnErrorStatement();

    AppendString(&tempBuffer, L"<OnError", depthLevel, NO_ENTITY_REF_CHECK);
    {
        if (PILNode->uFlags & SLF_ONERR_NEXT)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, L"resumenext", ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }
        if (PILNode->uFlags & SLF_ONERR_ZERO)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, L"zero", ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }
        if (PILNode->uFlags & SLF_ONERR_MINUS1)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, L"minusone", ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        if (bilOnErrorTree.Label && bilOnErrorTree.Label->Name)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, bilOnErrorTree.Label->Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }
    AppendString(&tempBuffer, L"</OnError>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Prcesses a Resume statment
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessResumeStatment(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    LABEL_ENTRY_BIL *pLabel = PILNode->AsOnErrorStatement().Label;

    if (PILNode->uFlags & SLF_ONERR_NEXT)
    {
        AppendString(&tempBuffer, L"<ResumeNext", depthLevel, NO_ENTITY_REF_CHECK);
    }
    else
    {
        AppendString(&tempBuffer, L"<Resume", depthLevel, NO_ENTITY_REF_CHECK);
    }
    {
        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        VSASSERT(!pLabel || !(PILNode->uFlags & SLF_ONERR_NEXT), "We can't have a label in a ResumeNext statement");

        if (pLabel)
        {
            // Generate the actual label we are resuming to
            GenerateGenericElement(&tempBuffer, NULL, pLabel->Name, CHECK_ENTITY_REF, NO_BRACKETS);
        }
    }
    if (PILNode->uFlags & SLF_ONERR_NEXT)
    {
        AppendString(&tempBuffer, L"</ResumeNext>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }
    else
    {
        AppendString(&tempBuffer, L"</Resume>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }

    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process the initialization expression. We handle the three types of
// initializers here: Expression, Aggregate, and Deferred.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessInitializer(StringBuffer *psbBuffer,
                                   ParseTree::Initializer *pinitializer,
                                   BCSYM *ptype,
                                   ULONG rankDepth,
                                   ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (!pinitializer)
    {
        return hr;
    }

    NorlsAllocator nraExpression(NORLSLOC);

    if (ptype && ptype->IsAlias())
    {
        ptype = ptype->DigThroughAlias();
    }

    if (ptype && ptype->IsPointerType())
    {
        ptype = ptype->PPointerType()->GetRoot();
    }

    // Here, we use the semantic analyzer to get us a bil expression tree
    Semantics SemanticAnalyzer(&nraExpression, NULL, m_pCompiler, m_pCompilerProject->GetCompilerHost(), m_pSourceFile, NULL, true);

    ILTree::ILNode *PILNode = SemanticAnalyzer.InterpretInitializer(pinitializer,
        m_pCurrentContext ? ContextOf(m_pCurrentContext) : ContextOf(m_pCompiler->GetUnnamedNamespace(m_pSourceFile)),
        NULL,
        ptype,
        true); // Merge anonymous types

    IfFailGo(ProcessExpression(psbBuffer, PILNode, depthLevel));

    // If Processing the Expression failed, then go ahead and quote the expression. We shouldn't need
    // to quote more than just the expression since we want to process as much XML as we can.
    if (m_fShouldQuote)
    {
        hr = NOERROR;
        IfFailGo(ProcessQuote(psbBuffer, PILNode, NULL, depthLevel));
        m_fShouldQuote = false;
    }

Error:
    return hr;
}

//****************************************************************************
// Process variable declaration and its initialization
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessVariableDeclaration(StringBuffer *psbBuffer, ParseTree::VariableDeclaration *pDeclaration,
                                           ParseTree::Declarator *pVarDecl,
                                           ParseTree::Initializer *pinitializer,
                                           ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    SmartPushPopObject LocalStackPush(&m_NameStack);

    AppendString(&tempBuffer, L"<Variable", depthLevel, NO_ENTITY_REF_CHECK);
    {
        BCSYM_NamedRoot *pVarSymbol = NULL;

        // Get the symbol if we can
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&pVarDecl->Name.TextSpan, &pVarSymbol));

        // We can't get anything from a bad symbol, so punt right away
        if (!pVarSymbol || !pVarSymbol->IsMember())
        {
            m_fShouldQuote = true;
            return hr;
        }

        // If the proc is bad because one or more of it's types (parameters or return type) are bad, then we can still
        // generate XML for that proc, but if it is bad because of any other reason, then we don't know what exactly is
        // wrong, and we give up.
        if (pVarSymbol->IsBad() && !pVarSymbol->PVariable()->IsBadVariableType())
        {
            m_fShouldQuote = true;
            return hr;
        }

        IfFailGo(GenerateStatementID(&tempBuffer, pVarDecl->TextSpan, ID));

        // 

        if (!pinitializer)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Implicit_Value, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // The name of the variable here
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pVarDecl->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pVarDecl->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        LocalStackPush.Push(pVarDecl->Name.Name, pVarDecl->Name.IsBracketed);

        // Fullname of the variable
        IfFailGo(GenerateFullName(&tempBuffer, pVarSymbol, PROCESS_SOURCE_NAME));

        // Is it a special name? (hidden for example?)
        if (pVarSymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        if (pVarDecl->ArrayInfo)
        {
            // Array type case
            IfFailGo(ProcessArrayType(&tempBuffer, pVarDecl->ArrayInfo, pDeclaration->Type, TypeOf(pVarSymbol), pDeclaration->Type ? &(pDeclaration->Type->TextSpan) : NULL, depthLevel + 1));
        }
        else
        {
            // Non-Array type case
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, TypeOf(pVarSymbol), pDeclaration->Type, pDeclaration->Type ? &(pDeclaration->Type->TextSpan) : NULL, depthLevel + 1));
        }

        IfTrueRet(m_fShouldQuote, hr);

        // And also process the initialization expression, if one exists
        IfFailGo(ProcessInitializer(&tempBuffer, pinitializer, TypeOf(pVarSymbol), INITIAL_RANK_LEVEL, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Variable>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process variable declaration
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessEnumVariable(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    SmartPushPopObject LocalStackPush(&m_NameStack);

    AppendString(&tempBuffer, L"<Variable", depthLevel, NO_ENTITY_REF_CHECK);
    {
        ParseTree::EnumeratorStatement *penum = pstatement->AsEnumerator();
        BCSYM_NamedRoot *pEnumSymbol = NULL;

        // Get the symbol if we can
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&penum->Name.TextSpan, &pEnumSymbol));

        // We can't do anything with bad symbols, so punt right away
        if (!pEnumSymbol || pEnumSymbol->IsBad())
        {
            m_fShouldQuote = true;
            return hr;
        }

        VSASSERT(pEnumSymbol->IsVariable(), L"Bad enum value symbol!");

        IfFailGo(GenerateStatementID(&tempBuffer, penum->TextSpan, ID));

        // Implicit value defined
        if (pstatement->Opcode == ParseTree::Statement::EnumeratorWithValue)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Implicit_Value, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, penum->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (penum->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Fullname of the variable
        LocalStackPush.Push(penum->Name.Name, penum->Name.IsBracketed);
        IfFailGo(GenerateFullName(&tempBuffer, pEnumSymbol, PROCESS_SOURCE_NAME));

        // Is it a special name? (hidden for example?)
        if (pEnumSymbol && pEnumSymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Value given as well
        if (pstatement->Opcode == ParseTree::Statement::EnumeratorWithValue)
        {
            IfFailGo(ProcessParseExpressionTree(&tempBuffer, penum->AsEnumeratorWithValue()->Value, TypeOf(pEnumSymbol), depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);
        }

        // And process any attributes used with this enum variable as well
        IfFailGo(ProcessAttributeList(&tempBuffer, penum->Attributes, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</Variable>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process any known type by simply using the name from the ParseTree::Type.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessUnresolvedType(StringBuffer *psbBuffer, ParseTree::Type *ptype,
                                      Location *PhysicalTypeReferenceLocation, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (!ptype)
    {
        return hr;
    }

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Type", depthLevel, NO_ENTITY_REF_CHECK);
    {
        if (PhysicalTypeReferenceLocation)
        {
            IfFailGo(GenerateStatementID(&tempBuffer, *PhysicalTypeReferenceLocation, RefID));
        }

        // Any container type is implicit.
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Implicit, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // We really don't know what this type is, so just indicate that we don't have any clue what the NamedTyps is.
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Unresolved, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // End Attributes
        AppendString(&tempBuffer, L">", NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // This is the actual name of the type.
        IfFailGo(ProcessName(&tempBuffer, ptype->AsNamed()->TypeName, NO_INDENTATION, NO_BRACKETS));
    }
    AppendString(&tempBuffer, L"</Type>" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a specific type. This is the core of the type switch statement,
// and it handles all type given a ParseTree::Type.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessTypeStatement(StringBuffer *psbBuffer, ParseTree::Type *ptype,
                                     ParseTree::Type *pElementArrayType, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    NorlsAllocator nraSymbols(NORLSLOC);
    Location *PhysicalTypeReferenceLocation = NULL;

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    if (!ptype)
    {
        return hr;
    }

    BCSYM_NamedRoot *ptypeSym = NULL;

    switch(ptype->Opcode)
    {
    case ParseTree::Type::SyntaxError :
        m_fShouldQuote = true;
        return hr;

        // All the intrinsic types here
    case ParseTree::Type::Short:
    case ParseTree::Type::UnsignedShort:
    case ParseTree::Type::Integer:
    case ParseTree::Type::UnsignedInteger:
    case ParseTree::Type::Long:
    case ParseTree::Type::UnsignedLong:
    case ParseTree::Type::Decimal:
    case ParseTree::Type::Single:
    case ParseTree::Type::Double:
    case ParseTree::Type::SignedByte:
    case ParseTree::Type::Byte:
    case ParseTree::Type::Boolean:
    case ParseTree::Type::Char:
    case ParseTree::Type::Date:
    case ParseTree::Type::String:
        PhysicalTypeReferenceLocation = &(ptype->TextSpan);
        ptypeSym = m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->GetType(MapTypeToVType(ptype->Opcode));
        break;

        // The case of Named type first
    case ParseTree::Type::Named :
        {
            PhysicalTypeReferenceLocation = &(ptype->AsNamed()->TypeName->TextSpan);
            m_pSourceFile->GetSymbolOfLocation(PhysicalTypeReferenceLocation, &ptypeSym);

            // If GetSymbolOfLocation() fails, we try to find the BCSYM by using Semantic----isys...
            if (!ptypeSym)
            {
                // This is the type that the implemented method belongs to
                BCSYM *BoundNamedType = NULL;

                IfFailGo(GetSymbolFromName(ptype->AsNamed()->TypeName, NULL, &BoundNamedType, NameSearchTypeReferenceOnly));
                IfTrueRet(m_fShouldQuote, hr);

                // Check for bad symbols
                if (!BoundNamedType || BoundNamedType->IsBad())
                {
                    if (m_fProcessMethods)
                    {
                        m_fShouldQuote = true;
                        return hr;
                    }

                    break;
                }

                if (BoundNamedType->DigThroughAlias()->IsNamedRoot())
                {
                    ptypeSym = BoundNamedType->DigThroughAlias()->PNamedRoot();          // Symbol found is good, use it
                }
            }
        }
        break;

    case ParseTree::Type::Object:
        ptypeSym = m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ObjectType);
        break;

    case ParseTree::Type::ArrayWithoutSizes :
    case ParseTree::Type::ArrayWithSizes :
        PhysicalTypeReferenceLocation = ptype->AsArray()->ElementType ? &(ptype->AsArray()->ElementType->TextSpan) : NULL;

        // Pass the ArrayElementType through so we can finally generate the type of the array.
        IfFailGo(ProcessArrayType(&tempBuffer, ptype->AsArray(), pElementArrayType, NULL, pElementArrayType ? &(pElementArrayType->TextSpan) : NULL, depthLevel + 1));
        goto Error;

    default :
        VSFAIL(L"Bad Opcode for a Type");
        m_fShouldQuote = true;
        return hr;
    }

    // Process the type
    if (ptypeSym)
    {
        IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, ptypeSym, ptype, PhysicalTypeReferenceLocation, depthLevel));
        IfTrueRet(m_fShouldQuote, hr);
    }
    else
    {
        // If we can't get the BCSYM, we just use the name of the type
        // in the parsetree to generate the type
        if (!ptypeSym && ptype->Opcode == ParseTree::Type::Named)
        {
            hr = ProcessUnresolvedType(&tempBuffer, ptype, PhysicalTypeReferenceLocation, depthLevel);
        }
        else
        {
            VSFAIL(L"What else can the type be?");
            m_fShouldQuote = true;
            return hr;
        }
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a type char
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessTypeChar(StringBuffer *psbBuffer, typeChars typeCharacter, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Type", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // 


        if (typeCharacter == chType_sI8)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_TypeChar, WIDE("&amp;"), NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }
        else
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_TypeChar, WszTypeChar(typeCharacter), NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">", NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }
    AppendString(&tempBuffer, L"</Type>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Prcesses the array type
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessArrayType(StringBuffer *psbBuffer, ParseTree::ArrayType *pArrayInfo,
                                 ParseTree::Type *pArrayElementType, BCSYM *psymType,
                                 Location *PhysicalTypeReferenceLocation, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (!pArrayInfo)
    {
        return hr;
    }

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<ArrayType", depthLevel, NO_ENTITY_REF_CHECK);
    {
        WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};
        _itow_s(pArrayInfo->Rank, wszIntValue, _countof(wszIntValue), 10);

        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Rank, wszIntValue, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Try to generate the size of the different ranks:
        if (pArrayInfo->Opcode==ParseTree::Type::ArrayWithSizes)
        {
#if DEBUG
            int nRankCount = 0;
#endif
            ParseTree::ArrayWithSizesType *pawstInfo = pArrayInfo->AsArrayWithSizes();
            AppendString(&tempBuffer, L"<Ranks>" DEBUG_NEW_LINE, depthLevel+1, NO_ENTITY_REF_CHECK);
            ParseTree::ArrayDimList *pexplist = pawstInfo->Dims;

            for ( ; pexplist; pexplist = pexplist->Next)
            {
#if DEBUG
                nRankCount++;
#endif

                bool IsResultBad = false;
                NorlsAllocator nraExpression(NORLSLOC);

                // Here, we use semantics to get us a bound constant expression
                Semantics SemanticAnalyzer(&nraExpression, NULL, m_pCompiler, m_pCompilerProject->GetCompilerHost(), m_pSourceFile, NULL, true);

                ConstantValue RankValue = SemanticAnalyzer.InterpretConstantExpression(pexplist->Element->upperBound,
                    m_pCurrentContext ? ContextOf(m_pCurrentContext) : ContextOf(m_pCompiler->GetUnnamedNamespace(m_pSourceFile)),
                    NULL, false, &IsResultBad);

                if (!IsResultBad)
                {
                    ULONG Rank = 0;

                    switch (RankValue.TypeCode)
                    {
                    case t_i1 :
                    case t_ui1 :
                    case t_i2 :
                    case t_ui2 :
                    case t_i4 :
                    case t_ui4 :
                    case t_i8 :
                    case t_ui8 :
                        {
#if DEBUG
                            // If the user is creating an array with more than 4 billion elements,
                            // then he/she has major problems. We will truncate the size for them.
                            if (RankValue.TypeCode == t_i8)
                            {
                                VSASSERT((RankValue.Integral & 0xFFFFFFFF00000000) == 0,
                                         L"Wow! Someone is creating an array of more than 2^32 elements!");
                            }
#endif

                            Rank = (ULONG) RankValue.Integral;
                            break;
                        }

                    case t_single :
                        {
                            IfFailGo(VarUintFromR4(RankValue.Single, &Rank));
                            break;
                        }

                    case t_double :
                        {
                            IfFailGo(VarUintFromR8(RankValue.Double, &Rank));
                            break;
                        }

                    case t_decimal :
                        {
                            IfFailThrow(VarUintFromDec(&RankValue.Decimal, &Rank));
                            break;
                        }

                    default :
                        VSFAIL(L"Bad rank entry value!");
                        break;
                    }

                    _itow_s(Rank + 1, wszIntValue, _countof(wszIntValue), 10);
                    AppendString(&tempBuffer, L"<RankEntry", depthLevel + 2, NO_ENTITY_REF_CHECK);
                    IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Count, wszIntValue, NO_ENTITY_REF_CHECK, NO_BRACKETS));
                    AppendString(&tempBuffer, L"/>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
                }
            }

            VSASSERT(nRankCount == pArrayInfo->Rank, L"Number of ranks does not match number of entries!");
            AppendString(&tempBuffer, L"</Ranks>" DEBUG_NEW_LINE, depthLevel+1, NO_ENTITY_REF_CHECK);
        }

        // The type of the array (can be an array type)
        if (psymType && psymType->IsArrayType())
        {
            VSASSERT(psymType->IsArrayType(), L"Symbol must be an ArrayType Symbol");
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, psymType->PArrayType()->GetRoot(), pArrayElementType, PhysicalTypeReferenceLocation, depthLevel + 1));
        }
        else
        {
            IfFailGo(ProcessTypeStatement(&tempBuffer, pArrayInfo->ElementType ? pArrayInfo->ElementType : pArrayElementType, pArrayElementType, depthLevel + 1));
        }
    }
    AppendString(&tempBuffer, L"</ArrayType>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Prcesses the array declarations of all types given a BCSYM_ArrayType
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessBCSYMArrayType(StringBuffer *psbBuffer, BCSYM *parrayType, ParseTree::Type *ParseTreeType,
                                      ULONG rankDepth, Location *PhysicalTypeReferenceLocation, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (!parrayType)
    {
        return hr;
    }

    VSASSERT(parrayType->IsArrayType(), L"BCSYM must be a BCSYM_ArrayType inside ProcessBCSYMArrayType()");

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<ArrayType", depthLevel, NO_ENTITY_REF_CHECK);
    {
        unsigned rank = parrayType->PArrayType()->GetRank() - rankDepth;
        WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};
        _itow_s(rank, wszIntValue, _countof(wszIntValue), 10);

        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Rank, wszIntValue, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        if (ParseTreeType)
        {
            switch (ParseTreeType->Opcode)
            {
            case ParseTree::Type::ArrayWithSizes :
                // The type of the array
                IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, parrayType->PArrayType()->GetRoot(),
                         ParseTreeType->AsArrayWithSizes()->ElementType, PhysicalTypeReferenceLocation, depthLevel + 1));
                break;

            case ParseTree::Type::ArrayWithoutSizes :
                // The type of the array
                IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, parrayType->PArrayType()->GetRoot(),
                         ParseTreeType->AsArray()->ElementType, PhysicalTypeReferenceLocation, depthLevel + 1));
                break;

            default:
                // The type of the array
                IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, parrayType->PArrayType()->GetRoot(), ParseTreeType, PhysicalTypeReferenceLocation, depthLevel + 1));
            }
        }
        else
        {
            // The type of the array
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, parrayType->PArrayType()->GetRoot(), NULL, PhysicalTypeReferenceLocation, depthLevel + 1));
        }
    }
    AppendString(&tempBuffer, L"</ArrayType>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a BCSYM Container type.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessBCSYMContainerType(StringBuffer *psbBuffer, BCSYM_Container *pcontainerType,  BCSYM_GenericBinding *pGenericTypeBinding,
                                          Location *PhysicalTypeReferenceLocation, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (!pcontainerType)
    {
        return hr;
    }

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Type", depthLevel, NO_ENTITY_REF_CHECK);
    {

        if (PhysicalTypeReferenceLocation)
        {
            IfFailGo(GenerateStatementID(&tempBuffer, *PhysicalTypeReferenceLocation, RefID));
        }

        // Any container type is implicit.
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Implicit, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // End Attributes
        AppendString(&tempBuffer, L">", NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // This is the actual name of the type.
        if (m_fUseTrueQualifiedName)
        {
            BCSYM_NamedRoot *pType = pcontainerType;

            if (pGenericTypeBinding)
            {
                pType = pGenericTypeBinding;
            }

            AssertIfNull(pType);

#if IDE
            STRING *QualifiedName = pType->GetCompiler()->GetCLRTypeNameEncoderDecoderIDE()->TypeToCLRName(pType, NULL, NULL);
#else
            STRING *QualifiedName = pType->GetCompiler()->GetCLRTypeNameEncoderDecoder()->TypeToCLRName(pType, NULL, NULL);
#endif
            GenerateGenericElement(&tempBuffer, NULL, QualifiedName, CHECK_ENTITY_REF, NO_BRACKETS);
        }
        else
        {
            GenerateGenericElement(&tempBuffer, NULL, pcontainerType->GetQualifiedName(), CHECK_ENTITY_REF, NO_BRACKETS);
        }
    }
    AppendString(&tempBuffer, L"</Type>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process any type, given a BCSYM for the thingy declared of that type.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessTypeFromBCSYM(StringBuffer *psbBuffer, BCSYM *psymType, ParseTree::Type *ParseTreeType,
                                     Location *PhysicalTypeReferenceLocation, ULONG depthLevel,
                                     BCSYM_GenericBinding *pGenericBinding)
{
    HRESULT hr = NOERROR;

    if (!psymType)
    {
        return hr;
    }

    switch (psymType->GetKind())
    {
    case SYM_GenericBadNamedRoot :
    case SYM_TypeForwarder :
        VSASSERT(psymType->PNamedRoot()->IsBad(), L"should be bad!");

        if (m_fProcessMethods || !ParseTreeType)
        {
            m_fShouldQuote = true;
            break;
        }

        IfFailGo(ProcessTypeStatement(psbBuffer, ParseTreeType, NULL, depthLevel));
        IfTrueRet(m_fShouldQuote, hr);
        break;

    case SYM_VoidType :
        {
            AppendString(psbBuffer, L"<Type", depthLevel, NO_ENTITY_REF_CHECK);
            {
                if (PhysicalTypeReferenceLocation)
                {
                    IfFailGo(GenerateStatementID(psbBuffer, *PhysicalTypeReferenceLocation, RefID));
                }

                // End Attributes
                AppendString(psbBuffer, L">", NO_INDENTATION, NO_ENTITY_REF_CHECK);

                // This is the actual name of the type.
                GenerateGenericElement(psbBuffer, NULL, ELEMENT_Void, NO_ENTITY_REF_CHECK, NO_BRACKETS);
            }
            AppendString(psbBuffer, L"</Type>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
            break;
        }

    case SYM_PointerType :
        hr = ProcessTypeFromBCSYM(psbBuffer, psymType->PPointerType()->GetRoot(), ParseTreeType, PhysicalTypeReferenceLocation, depthLevel);
        break;

    case SYM_NamedType :
        hr = ProcessTypeFromBCSYM(psbBuffer, psymType->PNamedType()->GetSymbol(), ParseTreeType, PhysicalTypeReferenceLocation, depthLevel);
        break;

    case SYM_Alias :
        hr = ProcessTypeFromBCSYM(psbBuffer, psymType->DigThroughAlias(), ParseTreeType, PhysicalTypeReferenceLocation, depthLevel);
        break;

    case SYM_Class :
    case SYM_Interface :
        hr = ProcessBCSYMContainerType(psbBuffer, psymType->PContainer(), pGenericBinding, PhysicalTypeReferenceLocation, depthLevel);
        break;

    case SYM_GenericTypeBinding:
        hr = ProcessBCSYMContainerType(psbBuffer, psymType->PContainer(), psymType->PGenericTypeBinding(), PhysicalTypeReferenceLocation, depthLevel);
        break;
        
    case SYM_ArrayLiteralType :    
    case SYM_ArrayType :
        hr = ProcessBCSYMArrayType(psbBuffer, psymType, ParseTreeType, 0, PhysicalTypeReferenceLocation, depthLevel);
        break;

    default :
        if (m_fProcessMethods || !ParseTreeType)
        {
            m_fShouldQuote = true;
            return hr;
        }

        // At this point, we give up on trying to find out what this type is, and instead, process it
        // as an unresolved type
        if (ParseTreeType && ParseTreeType->Opcode == ParseTree::Type::Named)
        {
            hr = ProcessUnresolvedType(psbBuffer, ParseTreeType, PhysicalTypeReferenceLocation, depthLevel);
            IfTrueRet(m_fShouldQuote, hr);
        }
        else
        {
            VSFAIL(L"What else can the type be?");
            m_fShouldQuote = true;
        }
    }

Error:
    return hr;
}

//****************************************************************************
// Process the declarations of method parameters. Note: this code accumes that
// we have a Param symbol.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessParameterDeclaration(StringBuffer *psbBuffer, ParseTree::Parameter *pparameter,
                                            BCSYM_Param *pPrameterSymbol, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (pPrameterSymbol->IsBadParam())
    {
        m_fShouldQuote = true;
        return hr;
    }

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Parameter", depthLevel, NO_ENTITY_REF_CHECK);
    {
        if (pPrameterSymbol->HasLocation())
        {
            IfFailGo(GenerateStatementID(&tempBuffer, *(pPrameterSymbol->GetLocation()), ID));
        }

        // CodeModel needs a location span for the whole parameter (ByVal x as long, not just the long), so
        // we generate a ParamID attribute for that purpose.
        if (pparameter)
        {
            IfFailGo(GenerateStatementID(&tempBuffer, pparameter->TextSpan, ParamID));
        }

        // declared as an optional param
        if (pPrameterSymbol->IsOptional())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Optional, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // declared byref?
        if (pPrameterSymbol->IsByRefKeywordUsed())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_ByRef, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // declared as paramarray?
        if (pPrameterSymbol->IsParamArray())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_ParamArray, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Parameter name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pPrameterSymbol->GetName(), NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pparameter && pparameter->Name->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, pPrameterSymbol->GetType(), pparameter ? pparameter->Type : NULL,
                (pparameter && pparameter->Type) ?  &(pparameter->Type->TextSpan) : NULL, depthLevel + 1));

        IfTrueRet(m_fShouldQuote, hr);

        if (pparameter)
        {
            // And process any attributes used with this param as well
            IfFailGo(ProcessAttributeList(&tempBuffer, pparameter->Attributes, depthLevel + 1));
        }

        // Optional params have default value
        // For Enums, we want "Red" not "3", so we need to process the Init expression from the parsetrees
        if (pparameter && pparameter->Specifiers->HasSpecifier(ParseTree::ParameterSpecifier::Optional))
        {
            IfFailGo(ProcessParseExpressionTree(&tempBuffer, pparameter->AsOptional()->DefaultValue,
                     pPrameterSymbol->GetCompilerType(), depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);
        }
    }
    AppendString(&tempBuffer, L"</Parameter>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Either quotes or processes a method body
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ExecuteMethodBody(StringBuffer *psbBuffer, BCSYM_Proc *pmethod, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    ILTree::ILNode *pBilMethodBodyTree = NULL;
    BCSYM *pBCSYM = pmethod;

    if (m_fProcessMethods == NO_METHODS)
    {
        return hr;
    }

    NorlsAllocator nraMethodBodyTree(NORLSLOC);         // NRA on the stack
    ErrorTable errTable(m_pCompiler, m_pSourceFile->GetProject(), NULL);
    Cycles cycles(m_pCompiler, &nraMethodBodyTree, &errTable, ERRID_SubNewCycle1, ERRID_SubNewCycle2);

    if (!pBCSYM)
    {
        return hr;
    }

    if (!pBCSYM->IsProc())
    {
        return DebAssertHelper(L"A BCSYM for a method body can't be a non-Proc!");
    }

    // If this not a method impel, we just punt
    if (pBCSYM->GetKind() != SYM_MethodImpl)
    {
        return hr;
    }

    Text *pTextFile = NULL;

    IfFailGo(GetTextFile(&pTextFile));

    // Get the bound method tree for this particular method
    IfFailGo(m_pSourceFile->GetBoundMethodBodyTrees(
        pBCSYM->PProc(),
        &nraMethodBodyTree,
        &errTable,
        &cycles,
        NULL,
        NULL,
        pTextFile,
        NULL, //&m_CompilationCaches,
        gmbIDEBodies | gmbMergeAnonymousTypes | gmbLowerTree,
        &pBilMethodBodyTree));

    // If we see any errors in the method, we just go ahead and quote it.
    if (errTable.GetErrorCount() == 0)
    {
        // Process it
        IfFailGo(ProcessMethodBody(psbBuffer, pBilMethodBodyTree, depthLevel));
    }
    else
    {
        // Quote it
        IfFailGo(ProcessQuote(psbBuffer, pBilMethodBodyTree, NULL, depthLevel));
    }

Error:
    return hr;
}

//****************************************************************************
// Process implements logic for a method
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessMethodImplements(StringBuffer *psbBuffer, ParseTree::NameList *ptreeList, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    if (!ptreeList)
    {
        return hr;
    }

    // Loop over all names, processing one at a time.
    for ( ; ptreeList; ptreeList = ptreeList->Next)
    {
        AppendString(&tempBuffer, L"<ImplementedMethod>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
        {
            // This is the type that the implemented method belongs to
            BCSYM *psym = NULL;

            IfFailGo(GetSymbolFromName(ptreeList->Element, NULL, &psym, 0));
            IfTrueRet(m_fShouldQuote, hr);

            if (psym)
            {
                IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, psym->IsNamedRoot() ? psym->PNamedRoot()->GetParent() : NULL, NULL, NULL, depthLevel + 1));
            }

            AppendString(&tempBuffer, L"<MethodName>", depthLevel + 1, NO_ENTITY_REF_CHECK);
            {
                // If we already have the BCSYM for the method, we use it so that we emit the fully qualified name
                if (psym && psym->IsNamedRoot())
                {
                    AppendString(&tempBuffer, psym->PNamedRoot()->GetQualifiedName(false), NO_INDENTATION);
                }
                else
                {
                    IfFailGo(ProcessName(&tempBuffer, ptreeList->Element, NO_INDENTATION, USE_BRACKETS));
                    IfTrueRet(m_fShouldQuote, hr);
                }
            }
            AppendString(&tempBuffer, L"</MethodName>" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
        }
        AppendString(&tempBuffer, L"</ImplementedMethod>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process implements logic for an event
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessEventImplements(StringBuffer *psbBuffer, ParseTree::NameList *ptreeList, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    if (!ptreeList)
    {
        return hr;
    }

    // Loop over all names, processing one at a time.
    for ( ; ptreeList; ptreeList = ptreeList->Next)
    {
        AppendString(&tempBuffer, L"<ImplementedMethod>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
        {
            // This is the type that the implemented event belongs to
            BCSYM *psym = NULL;

            IfFailGo(GetSymbolFromName(ptreeList->Element, NULL, &psym, NameNoFlags));
            IfTrueRet(m_fShouldQuote, hr);

            if (psym)
            {
                IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, psym->IsNamedRoot() ? psym->PNamedRoot()->GetParent() : NULL, NULL, NULL , depthLevel + 1));
            }

            AppendString(&tempBuffer, L"<MethodName>", depthLevel + 1, NO_ENTITY_REF_CHECK);
            {
                // If we already have the BCSYM for the method, we use it so that we emit the fully qualified name
                if (psym && psym->IsNamedRoot())
                {
                    AppendString(&tempBuffer, psym->PNamedRoot()->GetQualifiedName(false), NO_INDENTATION);
                }
                else
                {
                    IfFailGo(ProcessName(&tempBuffer, ptreeList->Element, NO_INDENTATION, USE_BRACKETS));
                    IfTrueRet(m_fShouldQuote, hr);
                }
            }
            AppendString(&tempBuffer, L"</MethodName>" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
        }
        AppendString(&tempBuffer, L"</ImplementedMethod>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process handles logic using a BCSYM_MethodImpl
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessMethodHandlesFromBCYM
(
    StringBuffer *psbBuffer,
    BCSYM_NamedRoot *pnamed,
    _In_z_ STRING * pstrMethodName,
    ULONG depthLevel
)
{
    HRESULT hr = NOERROR;

    // Nothing to do if this is not a good method impl
    if (!pnamed || !pnamed->IsMethodImpl())
    {
        return hr;
    }

    BCSYM_EventDecl *phandledProc = pnamed->PMethodImpl()->GetHandledEvent();

    // The source file whare where the thingy we are handling lives in
    if (!phandledProc)
    {
        return hr;
    }

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    STRING *pstrEventName = phandledProc->GetName();

    if (pstrEventName)
    {
        size_t eventNameLength  = StringPool::StringLength(pstrEventName);
        size_t methodNameLength = StringPool::StringLength(pstrMethodName);

        if (eventNameLength >= methodNameLength)
        {
            VSFAIL(L"Bad Magic event hookup name");
        }
        else
        {
            // We use a copy in order to be able to modify it
            WCHAR *methodNameCopy;
            IfNullGo(methodNameCopy = new (zeromemory) WCHAR[methodNameLength + 1]);
#pragma prefast(suppress: 26018, "StringPool::StringLength does return the string size")
            memcpy(methodNameCopy, pstrMethodName, methodNameLength * sizeof(WCHAR));
            methodNameCopy[methodNameLength] = UCH_NULL;

            if (methodNameCopy[methodNameLength - eventNameLength - 1] == CHAR_UnderScore)
            {
                methodNameCopy[methodNameLength - eventNameLength - 1] = CHAR_Period;

                AppendString(&tempBuffer, L"<Handles", depthLevel, NO_ENTITY_REF_CHECK);
                {
                    // Event name
                    IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Event_Name, methodNameCopy, NO_ENTITY_REF_CHECK, NO_BRACKETS));

                    // End attributes
                    AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

                    // The source file that the symbol lives in
                    SourceFile *psourceFile = phandledProc->GetContainingSourceFileIfBasic();

                    if (psourceFile)
                    {
                        GenerateGenericElement(&tempBuffer, ELEMENT_Source_Name, psourceFile->GetFileName(), CHECK_ENTITY_REF, NO_BRACKETS);
                    }
                }
                AppendString(&tempBuffer, L"</Handles>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

                delete[] methodNameCopy;
                goto Error;

            }
            else
            {
                VSFAIL(L"Bad Magic event hookup name");
                delete[] methodNameCopy;
            }
        }
    }

    AppendString(&tempBuffer, L"<Handles", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Event name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Event_Name, phandledProc->GetName(), NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // End attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // The source file that the symbol lives in
        SourceFile *psourceFile = phandledProc->GetContainingSourceFileIfBasic();

        if (psourceFile)
        {
            GenerateGenericElement(&tempBuffer, ELEMENT_Source_Name, psourceFile->GetFileName(), CHECK_ENTITY_REF, NO_BRACKETS);
        }
    }
    AppendString(&tempBuffer, L"</Handles>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process handles logic
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessMethodHandles(StringBuffer *psbBuffer, ParseTree::NameList *ptreeList, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (!ptreeList)
    {
        return hr;
    }

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    // Loop over all names, processing one at a time.
    for ( ; ptreeList; ptreeList = ptreeList->Next)
    {
        AppendString(&tempBuffer, L"<Handles", depthLevel, NO_ENTITY_REF_CHECK);
        {
            // Event name
            IfFailGo(GenerateGenericAttributeFromName(&tempBuffer, ATTRIB_VALUE_Event_Name, ptreeList->Element, USE_BRACKETS));

            // End attributes
            AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

            BCSYM *psym = NULL;

            IfFailGo(GetSymbolFromName(ptreeList->Element, NULL, &psym, 0));
            IfTrueRet(m_fShouldQuote, hr);

            // The source file whare where the thingy we are handling lives in
            if (psym && psym->IsNamedRoot())
            {
                // The source file that the symbol lives in
                SourceFile *psourceFile = psym->PNamedRoot()->GetContainingSourceFileIfBasic();

                if (psourceFile)
                {
                    GenerateGenericElement(&tempBuffer, ELEMENT_Source_Name, psourceFile->GetFileName(), NO_ENTITY_REF_CHECK, NO_BRACKETS);
                }
            }
        }
        AppendString(&tempBuffer, L"</Handles>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a signature
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessSignature(StringBuffer *psbBuffer, ParseTree::MethodSignatureStatement *pmethodSig,
                                 BCSYM_Proc *pProcSymbol, ULONG depthLevel, BCSYM *ptype)
{
    HRESULT hr = NOERROR;

    // No need to dump the <signature> tag if we don't have parametersa or a return type.
    if (!pProcSymbol->GetFirstParam() && !ptype)
    {
        return hr;
    }

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Signature>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, ptype, pmethodSig->ReturnType, pmethodSig->ReturnType ? &(pmethodSig->ReturnType->TextSpan) : NULL, depthLevel + 1));

        // And the params
        IfFailGo(ProcessParameterList(&tempBuffer, pmethodSig->Parameters, pProcSymbol, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Signature>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process method declarations. We process the name, signature, ImplementedMethods
// within method declarations
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessProcedureDeclaration(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    SmartPushPopObject LocalStackPush(&m_NameStack);

    // 

    AppendString(&tempBuffer, L"<Method", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Get the symbol if we can
        BCSYM_NamedRoot *pProcSymbol = NULL;

        ParseTree::MethodDefinitionStatement *pmethod = pstatement->AsMethodDefinition();
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&pmethod->Name.TextSpan, &pProcSymbol));

        // We can't get anything from a NULL symbol, so punt right away
        if (!pProcSymbol)
        {
            m_fShouldQuote = true;
            return hr;
        }

        VSASSERT(pProcSymbol->IsProc(), L"Bad proc symbol!");

        // If the proc is bad because one or more of it's types (parameters or return type) are bad, then we can still
        // generate XML for that proc, but if it is bad because of any other reason, then we don't know what exactly is
        // wrong, and we give up.
        if (pProcSymbol->IsBad() && !pProcSymbol->PProc()->IsBadParamTypeOrReturnType())
        {
            m_fShouldQuote = true;
            return hr;
        }

        // 


        if (pProcSymbol->PProc()->IsUserDefinedOperatorMethod())
        {
            pProcSymbol = pProcSymbol->PProc()->GetAssociatedOperatorDef();
        }

        // Find the End Location for the inherits and implements list.
        Location ImpementsAndHandlesSpan;
        ImpementsAndHandlesSpan.Invalidate();

        // Find the span for Implements and Handles statements
        GetEndLocationForImplementsAndHandles(pmethod, &ImpementsAndHandlesSpan);

        IfFailGo(GenerateStatementID(&tempBuffer, pstatement, &ImpementsAndHandlesSpan));

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, pProcSymbol));

        // 

        // Declared with shadows
        if (pmethod->Specifiers->HasSpecifier(ParseTree::Specifier::Shadows))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shadows, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Static
        if (pmethod->Specifiers->HasSpecifier(ParseTree::Specifier::Static))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Static, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Shared
        if (pmethod->Specifiers->HasSpecifier(ParseTree::Specifier::Shared))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shared, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Virtual (overridable)
        if (pmethod->Specifiers->HasSpecifier(ParseTree::Specifier::Overridable))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Virtual, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }
        else if (pmethod->Specifiers->HasSpecifier(ParseTree::Specifier::NotOverridable))
        {
            // Declared as NotOverridable
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Not_Over_Ridable, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Overrides
        if (pmethod->Specifiers->HasSpecifier(ParseTree::Specifier::Overrides))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Over_Rides, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Overloads
        if (pmethod->Specifiers->HasSpecifier(ParseTree::Specifier::Overloads))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Over_Loads, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as MustOverrides
        if (pProcSymbol->PProc()->IsMustOverrideKeywordUsed())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Abstract, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // If this a function instead of a Sub, we generate the "function" flag
        if (pstatement->Opcode == ParseTree::Statement::FunctionDeclaration)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Function, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // 

        // Method name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pmethod->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pmethod->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Full name for the method
        LocalStackPush.Push(pmethod->Name.Name, pmethod->Name.IsBracketed);
        IfFailGo(GenerateFullName(&tempBuffer, pProcSymbol, PROCESS_SOURCE_NAME));

        // Is it a special name? (hidden for example?)
        if (pProcSymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Process the signature
        IfFailGo(ProcessSignature(&tempBuffer, pmethod->AsMethodSignature(),
                                  pProcSymbol && pProcSymbol->PProc() ? pProcSymbol->PProc() : NULL,
                                  depthLevel + 1, TypeOf(pProcSymbol)));

        IfTrueRet(m_fShouldQuote, hr);

        // And process implements and handles statments (if any)
        IfFailGo(ProcessMethodImplements(&tempBuffer, pmethod->Implements, depthLevel + 1));

        // With handles, we have two options: either we get it from the parsetree, or from the BCSYM_MethodImpl
        // we use the latter when the "handles" keyword is missing from the method declaration.
        if (pmethod->Handles)
        {
            IfFailGo(ProcessMethodHandles(&tempBuffer, pmethod->Handles, depthLevel + 1));
        }
        else
        {
            IfFailGo(ProcessMethodHandlesFromBCYM(&tempBuffer, pProcSymbol, pmethod->Name.Name, depthLevel + 1));
        }

        // And process any attributes used with this method as well
        IfFailGo(ProcessAttributeList(&tempBuffer, pmethod->Attributes, depthLevel + 1));

        // And process the body of the method (if any)
        IfFailGo(ExecuteMethodBody(&tempBuffer, pProcSymbol->PProc(), depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Method>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Prcesses an external method Declaration statement (dll declared)
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessExternalProcedureDeclaration(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    SmartPushPopObject LocalStackPush(&m_NameStack);

    AppendString(&tempBuffer, L"<Method", depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

        ParseTree::ForeignMethodDeclarationStatement *pexternalMethod = pstatement->AsForeignMethodDeclaration();
        BCSYM_NamedRoot *pProcSymbol = NULL;

        // Get the symbol if we can
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&pexternalMethod->Name.TextSpan, &pProcSymbol));

        // We can't get anything from a NULL or bad symbol, so punt right away
        if (!pProcSymbol || pProcSymbol->IsBad() || !pProcSymbol->IsProc())
        {
            m_fShouldQuote = true;
            return hr;
        }

        VSASSERT(pProcSymbol->IsDllDeclare(), L"Bad external proc symbol!");

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, pProcSymbol));

        // Declared with shadows
        if (pexternalMethod->Specifiers->HasSpecifier(ParseTree::Specifier::Shadows))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shadows, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Static
        if (pexternalMethod->Specifiers->HasSpecifier(ParseTree::Specifier::Static))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Static, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Shared
        if (pexternalMethod->Specifiers->HasSpecifier(ParseTree::Specifier::Shared))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shared, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Virtual (overridable)
        if (pexternalMethod->Specifiers->HasSpecifier(ParseTree::Specifier::Overridable))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Virtual, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }
        else if (pexternalMethod->Specifiers->HasSpecifier(ParseTree::Specifier::NotOverridable))
        {
            // Declared as NotOverridable
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Not_Over_Ridable, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Overrides
        if (pexternalMethod->Specifiers->HasSpecifier(ParseTree::Specifier::Overrides))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Over_Rides, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Overloads
        if (pexternalMethod->Specifiers->HasSpecifier(ParseTree::Specifier::Overloads))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Over_Loads, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as MustOverrides
        if (pProcSymbol->PProc()->IsMustOverrideKeywordUsed())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Abstract, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // If this a function instead of a Sub, we generate the "function" flag
        if (pstatement->Opcode == ParseTree::Statement::ForeignFunctionDeclaration ||
            pstatement->Opcode == ParseTree::Statement::ForeignFunctionNone)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Function, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Externally declared
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Extern, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Method name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pexternalMethod->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pexternalMethod->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Full name for the method
        LocalStackPush.Push(pexternalMethod->Name.Name, pexternalMethod->Name.IsBracketed);
        IfFailGo(GenerateFullName(&tempBuffer, pProcSymbol, PROCESS_SOURCE_NAME));

        // Is it a special name? (hidden for example?)
        if (pProcSymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Process the signature
        IfFailGo(ProcessSignature(&tempBuffer, pexternalMethod->AsMethodSignature(),
                                  pProcSymbol && pProcSymbol->IsProc() ? pProcSymbol->PProc() : NULL,
                                  depthLevel + 1, TypeOf(pProcSymbol)));

        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Method>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a constructor declaration
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessConstructorDeclaration(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    SmartPushPopObject LocalStackPush(&m_NameStack);

    AppendString(&tempBuffer, L"<Constructor", depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

        ParseTree::MethodDefinitionStatement *pconstructor = pstatement->AsMethodDefinition();
        BCSYM_NamedRoot *pProcSymbol = NULL;

        // Get the symbol if we can
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&pconstructor->Name.TextSpan, &pProcSymbol));

        // We can't get anything from a NULL or bad symbol, so punt right away
        if (!pProcSymbol || pProcSymbol->IsBad())
        {
            m_fShouldQuote = true;
            return hr;
        }

        VSASSERT(pProcSymbol->IsProc(), L"Bad constructor symbol!");

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, pProcSymbol));

        // Declared as Static
        if (pconstructor->Specifiers->HasSpecifier(ParseTree::Specifier::Static))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Static, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Shared
        if (pconstructor->Specifiers->HasSpecifier(ParseTree::Specifier::Shared))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shared, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Constructor's name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pconstructor->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pconstructor->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Full name for the constructor
        LocalStackPush.Push(pconstructor->Name.Name, pconstructor->Name.IsBracketed);
        IfFailGo(GenerateFullName(&tempBuffer, pProcSymbol, PROCESS_SOURCE_NAME));

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Process the signature (no return type)
        IfFailGo(ProcessSignature(&tempBuffer, pconstructor->AsMethodSignature(),
                 pProcSymbol->PProc() ? pProcSymbol->PProc() : NULL, depthLevel + 1, NULL));

        IfTrueRet(m_fShouldQuote, hr);

        // And process any attributes used with this constructor as well
        IfFailGo(ProcessAttributeList(&tempBuffer, pconstructor->Attributes, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        // And process the body of the constructor (if any)
        IfFailGo(ExecuteMethodBody(&tempBuffer, pProcSymbol->PProc(), depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Constructor>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a propertyaccessor signature
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessPropertySignature(StringBuffer *psbBuffer, ULONG depthLevel, bool fKind)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    VSASSERT(m_pPropertySymbol && m_pPropertySymbol->IsProc(), L"Bad property symbol");

    AppendString(&tempBuffer, L"<Signature>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Process the return type if any. We only process the return type if we are in a Get
        // property, but not a Set property
        if (fKind == PROPERTY_GET)
        {
            VSASSERT(TypeOf(m_pPropertySymbol) || m_pPropertyType, L"Property Get must return a type!");

            if (TypeOf(m_pPropertySymbol))
            {
                IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, TypeOf(m_pPropertySymbol), m_pPropertyType,
                         m_pPropertyType ? &(m_pPropertyType->TextSpan) : NULL, depthLevel + 1));
            }
            else
            {
                IfFailGo(ProcessTypeStatement(&tempBuffer, m_pPropertyType, NULL, depthLevel + 1));
            }
        }

        // And the params
        IfFailGo(ProcessParameterList(&tempBuffer, m_pPropertyParams,
            fKind == PROPERTY_GET ? m_pPropertySymbol->PProperty()->GetProperty() : m_pPropertySymbol->PProperty()->SetProperty(),
            depthLevel + 1));

        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Signature>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process attributes that are common among Properties and it's Getter and
// Setter.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessCommonPropertyAttributes(StringBuffer *psbBuffer)
{
    HRESULT hr = NOERROR;

    AppendStringBuffer tempBuffer(psbBuffer);

    // Declared with shadows
    if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::Shadows))
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shadows, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

    // Declared as Static
    if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::Static))
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Static, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

    // Declared as Shared
    if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::Shared))
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shared, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

    // Declared as Virtual (overridable)
    if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::Overridable))
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Virtual, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }
    else if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::NotOverridable))
    {
        // Declared as NotOverridable
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Not_Over_Ridable, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

    // Declared as Overrides
    if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::Overrides))
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Over_Rides, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

    // Declared as Overloads
    if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::Overloads))
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Over_Loads, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

    // Declared as MustOverrides
    if (m_pPropertySymbol->PProc()->IsMustOverrideKeywordUsed())
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Abstract, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process property accessors Get
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessPropertyGet(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    SmartPushPopObject LocalStackPush(&m_NameStack);

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    ParseTree::MethodDefinitionStatement *pmethod = pstatement->AsMethodDefinition();

    AppendString(&tempBuffer, L"<Method", depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

        if (!m_pPropertySymbol || !m_pPropertySymbol->IsProperty())
        {
            return DebAssertHelper(L"Bad Property BCSYM");
        }

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, m_pPropertySymbol));

        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Acessor, ATTRIB_VALUE_Get, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Propegate attributes from the property to the Get method
        IfFailGo(ProcessCommonPropertyAttributes(&tempBuffer));

        // Property Gets are always functions, mark them as such.
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Function, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Method name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, ELEMENT_Get, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Full name for the property
        StringBuffer nameBuffer;
        nameBuffer.AppendSTRING(m_pPropertySymbol->GetQualifiedName());
        nameBuffer.AppendChar(L'.');
        nameBuffer.AppendString(ELEMENT_Get);
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Full_Name, nameBuffer.GetString(), NO_ENTITY_REF_CHECK, NO_BRACKETS));

        STRING *GetName = m_pCompiler->AddString(ELEMENT_Get);
        LocalStackPush.Push(GetName, NO_BRACKETS);

        StringBuffer SourceNameBuffer;
        m_NameStack.GetBracketedDotQualifiedName(&SourceNameBuffer);

        if (wcscmp(nameBuffer.GetString(), SourceNameBuffer.GetString()))
        {
#if DEBUG
            BSTR UnbracketedSourceName = SysAllocStringLen(SourceNameBuffer.GetString(), SourceNameBuffer.GetStringLength());

            if (UnbracketedSourceName)
            {
                RemoveBrackets((WCHAR*)UnbracketedSourceName);

                VSASSERT(CompareNoCase(nameBuffer.GetString(), (WCHAR*)UnbracketedSourceName) == 0, L"sourcename and fullname attributes differ by more than brackets!");
                SysFreeString(UnbracketedSourceName);
            }
            else
            {
                VSFAIL(L"SysAllocStringLen failed");
            }
#endif DEBUG

            IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Source_Name, SourceNameBuffer.GetString(), NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Generate the key for property Get

#ifndef KEY_DUMP_OVERRIDE
        if (!VSFSWITCH(fDumpXML))
#endif // KEY_DUMP_OVERRIDE
        {
            IfFailGo(GenerateKey(&tempBuffer, NULL, nameBuffer.GetString()));
        }

        // Is it a special name? (hidden for example?)
        if (m_pPropertySymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Process the signature
        IfFailGo(ProcessPropertySignature(&tempBuffer, depthLevel + 1, PROPERTY_GET));

        // And process implements statments (if any)
        IfFailGo(ProcessMethodImplements(&tempBuffer, m_pPropertyImplements, depthLevel + 1));

        // And process any attributes used with this method as well
        IfFailGo(ProcessAttributeList(&tempBuffer, pmethod->Attributes, depthLevel + 1));

        // And process the body of the method (if any)
        IfFailGo(ExecuteMethodBody(&tempBuffer, m_pPropertySymbol->PProperty()->GetProperty(), depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Method>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process property accessors Set.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessPropertySet(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    SmartPushPopObject LocalStackPush(&m_NameStack);

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    ParseTree::MethodDefinitionStatement *pmethod = pstatement->AsMethodDefinition();

    AppendString(&tempBuffer, L"<Method", depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

        if (!m_pPropertySymbol || !m_pPropertySymbol->IsProperty())
        {
            return DebAssertHelper(L"Bad Property BCSYM");
        }

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, m_pPropertySymbol));

        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Acessor, ATTRIB_VALUE_Set, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Propegate attributes from the property to the Set method
        IfFailGo(ProcessCommonPropertyAttributes(&tempBuffer));

        // Method name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, ELEMENT_Set, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Full name for the property
        StringBuffer nameBuffer;
        nameBuffer.AppendSTRING(m_pPropertySymbol->GetQualifiedName());
        nameBuffer.AppendChar(L'.');
        nameBuffer.AppendString(ELEMENT_Set);
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Full_Name, nameBuffer.GetString(), NO_ENTITY_REF_CHECK, NO_BRACKETS));

        STRING *SetName = m_pCompiler->AddString(ELEMENT_Set);
        LocalStackPush.Push(SetName, NO_BRACKETS);

        StringBuffer SourceNameBuffer;
        m_NameStack.GetBracketedDotQualifiedName(&SourceNameBuffer);

        if (wcscmp(nameBuffer.GetString(), SourceNameBuffer.GetString()))
        {
#if DEBUG
            BSTR UnbracketedSourceName = SysAllocStringLen(SourceNameBuffer.GetString(), SourceNameBuffer.GetStringLength());

            if (UnbracketedSourceName)
            {
                RemoveBrackets((WCHAR*)UnbracketedSourceName);

                VSASSERT(CompareNoCase(nameBuffer.GetString(), (WCHAR*)UnbracketedSourceName) == 0, L"sourcename and fullname attributes differ by more than brackets!");
                SysFreeString(UnbracketedSourceName);
            }
            else
            {
                VSFAIL(L"SysAllocStringLen failed");
            }
#endif DEBUG

            IfFailGo(GenerateGenericAttribute(psbBuffer, ATTRIB_VALUE_Source_Name, SourceNameBuffer.GetString(), NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

#ifndef KEY_DUMP_OVERRIDE
        if (!VSFSWITCH(fDumpXML))
#endif // KEY_DUMP_OVERRIDE
        {
            // Generate the key for property Get
            IfFailGo(GenerateKey(&tempBuffer, NULL, nameBuffer.GetString()));
        }

        // Is it a special name? (hidden for example?)
        if (m_pPropertySymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Process the signature
        IfFailGo(ProcessPropertySignature(&tempBuffer, depthLevel + 1, PROPERTY_SET));
        IfTrueRet(m_fShouldQuote, hr);

        // And process implements statments (if any)
        IfFailGo(ProcessMethodImplements(&tempBuffer, m_pPropertyImplements, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        // And process any attributes used with this method as well
        IfFailGo(ProcessAttributeList(&tempBuffer, pmethod->Attributes, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        // And process the body of the method (if any)
        IfFailGo(ExecuteMethodBody(&tempBuffer, m_pPropertySymbol->PProperty()->SetProperty(), depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Method>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Makeup a property Get for properties declared inside an interface.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::FakePropertyGet(StringBuffer *psbBuffer, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Method", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, m_pPropertySymbol));

        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Acessor, ATTRIB_VALUE_Get, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Propegate attributes from the property to the Get method
        IfFailGo(ProcessCommonPropertyAttributes(&tempBuffer));

        // Property Gts all always functions, mark them as such.
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Function, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Method name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, ELEMENT_Get, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Full name for the property
        StringBuffer nameBuffer;
        nameBuffer.AppendSTRING(m_pPropertySymbol->GetQualifiedName());
        nameBuffer.AppendChar(L'.');
        nameBuffer.AppendString(ELEMENT_Get);
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Full_Name, nameBuffer.GetString(), NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Generate the key for property Get

#ifndef KEY_DUMP_OVERRIDE
        if (!VSFSWITCH(fDumpXML))
#endif // KEY_DUMP_OVERRIDE
        {
            IfFailGo(GenerateKey(&tempBuffer, NULL, nameBuffer.GetString()));
        }

        // Is it a special name? (hidden for example?)
        if (m_pPropertySymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Process the signature
        IfFailGo(ProcessPropertySignature(&tempBuffer, depthLevel + 1, PROPERTY_GET));

        // And process implements statments (if any)
        IfFailGo(ProcessMethodImplements(&tempBuffer, m_pPropertyImplements, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</Method>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Makeup a property Set for properties declared inside an interface.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::FakePropertySet(StringBuffer *psbBuffer, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Method", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, m_pPropertySymbol));

        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Acessor, ATTRIB_VALUE_Set, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Propegate attributes from the property to the Set method
        IfFailGo(ProcessCommonPropertyAttributes(&tempBuffer));

        // Method name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, ELEMENT_Set, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Full name for the property
        StringBuffer nameBuffer;
        nameBuffer.AppendSTRING(m_pPropertySymbol->GetQualifiedName());
        nameBuffer.AppendChar(L'.');
        nameBuffer.AppendString(ELEMENT_Set);
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Full_Name, nameBuffer.GetString(), NO_ENTITY_REF_CHECK, NO_BRACKETS));

#ifndef KEY_DUMP_OVERRIDE
        if (!VSFSWITCH(fDumpXML))
#endif // KEY_DUMP_OVERRIDE
        {
            // Generate the key for property Get
            IfFailGo(GenerateKey(&tempBuffer, NULL, nameBuffer.GetString()));
        }

        // Is it a special name? (hidden for example?)
        if (m_pPropertySymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Process the signature
        IfFailGo(ProcessPropertySignature(&tempBuffer, depthLevel + 1, PROPERTY_SET));

        // And process implements statments (if any)
        IfFailGo(ProcessMethodImplements(&tempBuffer, m_pPropertyImplements, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</Method>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process property declarations. We process the name, signature,
// ImplementedMethods within method declarations
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessPropertyDeclaration(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    SmartPushPopObject LocalStackPush(&m_NameStack);

    AppendString(&tempBuffer, L"<Property", depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

        m_pPropertyStatement = pstatement->AsProperty();

        // Get the symbol if we can
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&m_pPropertyStatement->Name.TextSpan, &m_pPropertySymbol));

        // We can't get anything from a NULL or bad symbol, so punt right away
        if (!m_pPropertySymbol)
        {
            m_fShouldQuote = true;
            return hr;
        }

        VSASSERT(m_pPropertySymbol->IsProperty(), L"Bad property symbol!");

        // If the proc is bad because one or more of it's types (parameters or return type) are bad, then we can still
        // generate XML for that proc, but if it is bad because of any other reason, then we don't know what exactly is
        // wrong, and we give up.
        if (m_pPropertySymbol->IsBad() && !m_pPropertySymbol->PProc()->IsBadParamTypeOrReturnType())
        {
            m_fShouldQuote = true;
            return hr;
        }

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, m_pPropertySymbol));

        // Dump the Read/Write/ReadWrite propertyType
        if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::ReadOnly))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Property_Type, ATTRIB_VALUE_Prop_ReadOnly, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }
        else if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::WriteOnly))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Property_Type, ATTRIB_VALUE_Prop_WriteOnly, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }
        else
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Property_Type, ATTRIB_VALUE_Prop_ReadWrite, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as default
        if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::Default))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Default, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        IfFailGo(ProcessCommonPropertyAttributes(&tempBuffer));

        // Declared with params?
        if (m_pPropertyStatement->Parameters)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Indexer, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Property name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, m_pPropertyStatement->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (m_pPropertyStatement->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Full name for the property
        LocalStackPush.Push(m_pPropertyStatement->Name.Name, m_pPropertyStatement->Name.IsBracketed);
        IfFailGo(GenerateFullName(&tempBuffer, m_pPropertySymbol, PROCESS_SOURCE_NAME));

        // Is it a special name? (hidden for example?)
        if (m_pPropertySymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // We need to do this ugly global stuff so that we can access the return type and the params
        // from within the property accessors.
        m_pPropertyType         = m_pPropertyStatement->PropertyType;
        m_pPropertyParams       = m_pPropertyStatement->Parameters;
        m_pPropertyImplements   = m_pPropertyStatement->Implements;

        // Process the signature (if any)
        if (m_pPropertyParams || m_pPropertyType || TypeOf(m_pPropertySymbol))
        {
            AppendString(&tempBuffer, L"<Signature>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
            {
                if (TypeOf(m_pPropertySymbol))
                {
                    IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, TypeOf(m_pPropertySymbol), m_pPropertyType,
                             m_pPropertyType ? &(m_pPropertyType->TextSpan) : NULL, depthLevel + 2));
                }
                else
                {
                    IfFailGo(ProcessTypeStatement(&tempBuffer, m_pPropertyType, NULL, depthLevel + 2));
                }

                IfFailGo(ProcessParameterList(&tempBuffer, m_pPropertyParams,
                         m_pPropertySymbol && m_pPropertySymbol->PProc() ? m_pPropertySymbol->PProc() : NULL,
                         depthLevel + 2));

                IfTrueRet(m_fShouldQuote, hr);
            }
            AppendString(&tempBuffer, L"</Signature>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        }

        // And process implements statments (if any)
        IfFailGo(ProcessMethodImplements(&tempBuffer, m_pPropertyStatement->Implements, depthLevel + 1));

        // Go over children one at a time, and process them too (Get and Set)
        // If there we have a children list, we just go ahead and process it, otherwise, we don't have
        // a Get or a Set property, which mean that we need to fake them so that VS CodeModel can hand out
        // CodeFunction objects for the Get and the Set, even though they are not there.
        if (m_pPropertyStatement->Children)
        {
            IfFailGo(ProcessStatementList(&tempBuffer, m_pPropertyStatement->Children, depthLevel + 1));
        }
        else
        {
            if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::ReadOnly))
            {
                IfFailGo(FakePropertyGet(&tempBuffer, depthLevel + 1));
            }
            else if (m_pPropertyStatement->Specifiers->HasSpecifier(ParseTree::Specifier::WriteOnly))
            {
                IfFailGo(FakePropertySet(&tempBuffer, depthLevel + 1));
            }
            else
            {
                IfFailGo(FakePropertyGet(&tempBuffer, depthLevel + 1));
                IfFailGo(FakePropertySet(&tempBuffer, depthLevel + 1));
            }
        }

        // And process any attributes used with this property as well
        IfFailGo(ProcessAttributeList(&tempBuffer, m_pPropertyStatement->Attributes, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</Property>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:

    // Reset the property info
    m_pPropertySymbol       = NULL;
    m_pPropertyType         = NULL;
    m_pPropertyParams       = NULL;
    m_pPropertyImplements   = NULL;
    m_pPropertyStatement    = NULL;

    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a AttributeParam for an attribute
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessAttributeProperty(StringBuffer *psbBuffer, ParseTree::Argument *pArgument, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    // If there is a name, then we are using a named arg notation, and hence we use the AttributeProperty
    // tag, otherwise, its a normal argument, which we already know how to handle
    if (pArgument->Name.Name)
    {
        AppendString(&tempBuffer, L"<AttributeProperty", depthLevel, NO_ENTITY_REF_CHECK);
        {
            IfFailGo(GenerateStatementID(&tempBuffer, pArgument->TextSpan, ID));
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pArgument->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

            if (pArgument->Name.IsBracketed)
            {
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            }

            // End Attributes
            AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

            IfFailGo(ProcessParseExpressionTree(&tempBuffer, pArgument->Value, NULL, depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);
        }
        AppendString(&tempBuffer, L"</AttributeProperty>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }
    else
    {
        AppendString(&tempBuffer, L"<Argument> DEBUG_NEW_LINE", depthLevel, NO_ENTITY_REF_CHECK);
        {
            IfFailGo(ProcessParseExpressionTree(&tempBuffer, pArgument->Value, NULL, depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);
        }
        AppendString(&tempBuffer, L"</Argument>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process attribute section
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessAttribute(StringBuffer *psbBuffer, ParseTree::Attribute *pAttribute, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Attribute", depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(GenerateStatementID(&tempBuffer, pAttribute->TextSpan, ID));

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // This is the type of the attribute
        BCSYM *ptype = NULL;

        IfFailGo(GetSymbolFromName(pAttribute->Name, NULL, &ptype, NameSearchTypeReferenceOnly));
        IfTrueRet(m_fShouldQuote, hr);

        // If we can't find it, try the name with "Attribute" appended to the end...
        if (!ptype)
        {
            IfFailGo(GetSymbolFromName(pAttribute->Name, NULL, &ptype, NameSearchTypeReferenceOnly, ELEMENT_Attribute));
        }

        if (ptype)
        {
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, ptype, NULL, NULL, depthLevel + 1));
        }
        else
        {
            // If we fail pretty bad, we have not choice but to use the name from the parsetree
            AppendString(&tempBuffer, L"<Type", depthLevel + 1, NO_ENTITY_REF_CHECK);
            {
                IfFailGo(GenerateStatementID(&tempBuffer, pAttribute->Name->TextSpan, ID));

                // End Attributes
                AppendString(&tempBuffer, L">", NO_INDENTATION, NO_ENTITY_REF_CHECK);

                // This is the actual name of the type of the attribute.
                IfFailGo(ProcessName(&tempBuffer, pAttribute->Name, NO_INDENTATION, NO_BRACKETS))
            }
            AppendString(&tempBuffer, L"</Type>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        }

        // Porcess the AttributeParams and/or expressions
        IfFailGo(ProcessAttributeArgumentList(&tempBuffer, pAttribute->Arguments.Values, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Attribute>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a local Dim statement.
//
// The VAR_DECL statements are AsStatementWithExpression, with a list of declared
// items appearing as the only operand. The list structure mirrors
// the syntax of variable declarations, which means that it is a
// list of lists (of SX_SYM nodes). For example:
//
//   Dim X,Y As Integer, Z As Double
//
// comes through as a list of lists of SX_SYM nodes.
// The first list has two elements, one that refers to X and one
// that refers to Y. The second list has one element that refers to Z.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessLocalDimStmt(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    ILTree::Expression *InitializationExpression = NULL;

    // Check the validity of the tree first
    if (!PILNode->AsExpressionWithChildren().Left || !PILNode->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left)
    {
        m_fShouldQuote = true;
        return hr;
    }

    BCSYM_Variable *VariableBCSYM = NULL;
    ILTree::SymbolReferenceExpression VariableSymbol;

    AppendString(&tempBuffer, L"<Local", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Generate the line number so that designers can genrate proper error messages.
        IfFailGo(GenerateLineNumber(&tempBuffer, PILNode->AsExpressionWithChildren().Left->Loc.m_lBegLine));

        if (PILNode->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->bilop == SX_SYM)
        {
            VariableSymbol = PILNode->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->AsSymbolReferenceExpression();
        }
        else if (PILNode->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->bilop == SX_ASG)
        {
            VariableSymbol = PILNode->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->AsBinaryExpression().Left->AsSymbolReferenceExpression();
        }
        else if (PILNode->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->bilop == SX_BAD)
        {
            m_fShouldQuote = true;
            return hr;
        }
        else
        {
            VSFAIL(L"Encountered a bad ILTree node while processing locals");
            return E_FAIL;
        }

        if (VariableSymbol.pnamed && VariableSymbol.pnamed->IsVariable())
        {
            VariableBCSYM = VariableSymbol.pnamed->PVariable();
        }
        else
        {
            VSFAIL(L"BAD named variable");
            return E_FAIL;
        }

        // declared as static
        if (VariableBCSYM->IsStatic())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Static, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        if (VariableBCSYM->IsMe())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Instance, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        if (VariableBCSYM->IsImplicitDecl())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Implicit, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        if (VariableBCSYM->IsConstant())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Constant, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
        IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, TypeOf(VariableBCSYM), NULL, NULL, depthLevel + 1));


        for (ILTree::PILNode LocalVariable = PILNode->AsExpressionWithChildren().Left; LocalVariable; LocalVariable = LocalVariable->AsExpressionWithChildren().Right)
        {
            if (LocalVariable->AsExpressionWithChildren().Left->bilop == SX_SYM)
            {
                VariableSymbol = LocalVariable->AsExpressionWithChildren().Left->AsSymbolReferenceExpression();
                InitializationExpression = NULL;
            }
            else if (LocalVariable->AsExpressionWithChildren().Left->bilop == SX_ASG)
            {
                VariableSymbol = LocalVariable->AsExpressionWithChildren().Left->AsBinaryExpression().Left->AsSymbolReferenceExpression();
                InitializationExpression = LocalVariable->AsExpressionWithChildren().Left->AsBinaryExpression().Right;
            }
            else
            {
                VSFAIL(L"Encountered a bad ILTree node while processing locals");
                return E_FAIL;
            }

            if (VariableSymbol.pnamed && VariableSymbol.pnamed->IsVariable())
            {
                VariableBCSYM = VariableSymbol.pnamed->PVariable();
                GenerateGenericElement(&tempBuffer, ELEMENT_Name, VariableBCSYM->GetName(), CHECK_ENTITY_REF, NeedsBrackets(VariableBCSYM));

                if (InitializationExpression)
                {
                    IfFailGo(ProcessExpression(&tempBuffer, InitializationExpression, depthLevel + 1));
                }
            }
        }
    }
    AppendString(&tempBuffer, L"</Local>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process DIM statments inside some container. This should only be used
// if we are processing DIMs inside containers, not global stuff.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessFieldDeclaration(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    ParseTree::VariableDeclarationStatement *pVarDecl = pstatement->AsVariableDeclaration();

    if (!pVarDecl->Declarations)
    {
        VSFAIL(L"Bad variable declaration tree");
        m_fShouldQuote = true;
        return hr;
    }

    ParseTree::VariableDeclaration *pDeclaration = pVarDecl->Declarations->Element;

    if (!pDeclaration)
    {
        VSFAIL(L"Bad variable declaration tree");
        m_fShouldQuote = true;
        return hr;
    }

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    if (pVarDecl->Specifiers->HasSpecifier(ParseTree::Specifier::Const))
    {
        AppendString(&tempBuffer, L"<Constant", depthLevel, NO_ENTITY_REF_CHECK);
    }
    else
    {
        AppendString(&tempBuffer, L"<Field", depthLevel, NO_ENTITY_REF_CHECK);
    }

    IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

    BCSYM_NamedRoot *pVariableSymbol = NULL;

    // We use the first variable to find the access for and all others have the same access.
    if (pDeclaration->Variables)
    {
        ParseTree::Declarator *pfirstVariable = pDeclaration->Variables->Element;
        // Get the symbol if we can
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&pfirstVariable->Name.TextSpan, &pVariableSymbol));
    }
    else
    {
        // If we can't find the first variable, we are horked, but we generate an access="unkown" anyway.
        // Get the symbol if we can
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&pDeclaration->TextSpan, &pVariableSymbol));
    }

    // We can't get anything from a bad symbol, so punt right away
    if (!pVariableSymbol)
    {
        m_fShouldQuote = true;
        return hr;
    }

    // If the proc is bad because one or more of it's types (parameters or return type) are bad, then we can still
    // generate XML for that proc, but if it is bad because of any other reason, then we don't know what exactly is
    // wrong, and we give up.
    if (pVariableSymbol->IsBad() && !pVariableSymbol->PVariable()->IsBadVariableType())
    {
        m_fShouldQuote = true;
        return hr;
    }

    VSASSERT(pVariableSymbol->IsVariable(), L"Bad variable symbol!");

    // Dump the access attributes (public, private, protected, ...)
    IfFailGo(GenerateAccess(&tempBuffer, pVariableSymbol));

    if (pVarDecl->Specifiers->HasSpecifier(ParseTree::Specifier::Const) ||
        pVarDecl->Specifiers->HasSpecifier(ParseTree::Specifier::ReadOnly))
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_ReadOnly, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }
    else
    {
        // This attribute is only for non-constants
        // Declared as Static
        if (pVarDecl->Specifiers->HasSpecifier(ParseTree::Specifier::Static))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Static, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Shared
        if (pVarDecl->Specifiers->HasSpecifier(ParseTree::Specifier::Shared))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shared, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }
    }

    // declared with shadows
    if (pVarDecl->Specifiers->HasSpecifier(ParseTree::Specifier::Shadows))
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shadows, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

    // declared with withevents
    if (pVarDecl->Specifiers->HasSpecifier(ParseTree::Specifier::WithEvents))
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_WithEvents, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

    // End Attributes
    AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

    // And process any attributes used with this variable as well
    IfFailGo(ProcessAttributeList(&tempBuffer, pVarDecl->Attributes, depthLevel + 1));

    for (ParseTree::VariableDeclarationList *pDeclarations = pVarDecl->Declarations; pDeclarations; pDeclarations = pDeclarations->Next)
    {
        ParseTree::VariableDeclaration *pDeclaration2 = pDeclarations->Element;

        // Now process the list of variables declared of this type. We also pass the initializer to this
        // variable processing list, so that it can be dumped along with the variable.
        // If there are more than one decls, things get pretty ugly. The parse tree would not show any errors,
        // even though this is not allowed. We deal with this by simply dumping the initializer with every
        // variable declaration.
        IfFailGo(ProcessVariableDeclaratorList(&tempBuffer, pDeclaration2, pDeclaration2->Variables,
            pDeclaration2->Opcode == ParseTree::VariableDeclaration::WithInitializer ?
            pDeclaration2->AsInitializer()->InitialValue : NULL, depthLevel + 1));

        IfTrueRet(m_fShouldQuote, hr);
    }

    if (pVarDecl->Specifiers->HasSpecifier(ParseTree::Specifier::Const))
    {
        AppendString(&tempBuffer, L"</Constant>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }
    else
    {
        AppendString(&tempBuffer, L"</Field>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }

    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

Error:
    return hr;
}

//****************************************************************************
// Process imports statments. We are using a cool syntax "Using" instead of
// the VB "imports" keyword.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessImportsStmt(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Using", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Walk over the names defined off this statement and create an
        // ImportedNamespace on the importednamespaces list.
        //
        IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Get the list of imported namespaces
        ParseTree::ImportDirectiveList *----ortsList = pstatement->AsImports()->Imports;

        // Loop over the list of imported namespaces.
        for ( ; ----ortsList; ----ortsList = ----ortsList->Next)
        {
            ParseTree::ImportDirective *----ortedDirective = ----ortsList->Element;

            // 
            if (----ortedDirective->Mode != ParseTree::ImportDirective::XmlNamespace)
            {
                AppendString(&tempBuffer, L"<ImportedNameSpace", depthLevel + 1, NO_ENTITY_REF_CHECK);
                {
                    if (----ortedDirective->Mode == ParseTree::ImportDirective::Alias)
                    {
                        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Alias, ----ortedDirective->AsAlias()->Alias.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));
                    }

                    AppendChar(&tempBuffer, '>', NO_INDENTATION, NO_ENTITY_REF_CHECK);

                    IfFailGo(ProcessName(&tempBuffer, ----ortedDirective->AsNamespace()->ImportedName, NO_INDENTATION, NO_BRACKETS))
                    IfTrueRet(m_fShouldQuote, hr);
                }
                AppendString(&tempBuffer, L"</ImportedNameSpace>" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
            }
        }
    }
    AppendString(&tempBuffer, L"</Using>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process an inherits statment.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessInheritsStmt(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // m_pCurrentContext is the class/interface that we are processing, and therefore, it can't be NULL.
    // if the context is anything else, then we have a bad inherits statement, and we need to quote it.
    if (!m_pCurrentContext || !(m_pCurrentContext->IsClass() || m_pCurrentContext->IsInterface()))
    {
        m_fShouldQuote = true;
        return hr;
    }

    VSASSERT(pstatement->AsTypeList()->Types, L"Bad type parsetree!");

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Base", depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Porcess the types we are inheriting from
        if (m_pCurrentContext->IsClass())
        {
            // For Classes, we only have single inheritance, so there is no need to call ProcessTypeList() since
            // we can get the base class symbol directly from the current class symbol
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, m_pCurrentContext->PClass()->GetCompilerBaseClass(), pstatement->AsTypeList()->Types->Element,
                    &(pstatement->AsTypeList()->Types->TextSpan), depthLevel + 1));
        }
        else
        {
            // Interface case here.
            IfFailGo(ProcessTypeList(&tempBuffer, pstatement->AsTypeList()->Types, depthLevel + 1));
        }

        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Base>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes an implements statments for Classes
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessImplementsList(StringBuffer *psbBuffer, ParseTree::StatementList *plist,
                                      BCSYM_Implements *----lementsList, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (!----lementsList)
    {
        return hr;
    }

    // We also walk the list of children so that we wan walk it and find all the implements
    // and generate IDs based on the location info
    ParseTree::Statement *pstatement = plist->Element;

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    bool fFirstImplements = true;       // The first implements is differnet.

    AppendString(&tempBuffer, L"<Implements", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Walk the BCSYM list
        while (----lementsList)
        {
            // And also walk the ParseTree children
            while (pstatement && (pstatement->Opcode != ParseTree::Statement::Implements))
            {
                plist = plist->NextInBlock;

                if (plist)
                {
                    pstatement = plist->Element;
                }
                else
                {
                    pstatement = NULL;
                }
            }

            BCSYM *pnamed = ----lementsList->GetRoot();

            if (!pnamed || pnamed->IsBad())
            {
                return DebAssertHelper(L"Implements BCSYM is bad");
            }

            if (fFirstImplements)
            {
                fFirstImplements = false;

                if (pstatement)
                {
                    IfFailGo(GenerateStatementID(&tempBuffer, pstatement));
                }
                else
                {
                    DebAssertHelper(L"We don't have a ParseTree::Statement::Implements to use for generation of an ID");
                    m_fShouldQuote = false;     // We don't want to quote this thing just because we can generate a key!
                }

                // End Attributes
                AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
            }

            // Porcess the types we are implementing.
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, pnamed, NULL, pstatement ? &(pstatement->TextSpan) : NULL, depthLevel + 1));

            // Get next
            ----lementsList = ----lementsList->GetNext();

            if (plist)
            {
                plist = plist->NextInBlock;

                if (plist)
                {
                    pstatement = plist->Element;
                }
                else
                {
                    pstatement = NULL;
                }
            }
        }
    }
    AppendString(&tempBuffer, L"</Implements>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Prcesses a Class and everything inside of it
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessClass(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    BCSYM_NamedRoot *pOldContext = NULL;       // Save the old context

    // We use this second buffer in order to put everything nested in a seperate buffer
    // and then later we can append this buffer to the original ine. This will flatten out
    // the nesting for us.
    StringBuffer tempBuffer2;
    tempBuffer2.SetAllocationSize(StringBuffer::AllocSize::Large);

    SmartPushPopObject LocalStackPush(&m_NameStack);

    AppendString(&tempBuffer, L"<Class", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Get the symbol if we can
        BCSYM_NamedRoot *pClassSymbol = NULL;
        ParseTree::TypeStatement *pClass = pstatement->AsType();
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&pClass->Name.TextSpan, &pClassSymbol));

#if DEBUG
        if (!pClassSymbol)
        {
            VSFAIL(L"NULL Class Symbol");
#if IDE
            m_pSourceFile->DumpSymbolsHashTable();
#endif IDE
        }
#endif DEBUG

        // We can't do anything with bad symbols, so punt right away
        if (!pClassSymbol || pClassSymbol->IsBad())
        {
            m_fShouldQuote = true;
            return hr;
        }

        VSASSERT(pClassSymbol->IsClass(), L"Bad class symbol!");

        // Find the End Location for the inherits and implements list.
        Location ImpementsAndInheritsSpan;
        ImpementsAndInheritsSpan.Invalidate();

        // Also, get the span for the Implements and Inherits span
        if (pClass->Children)
        {
            GetEndLocationForImplementsAndInherits(pClass->Children, &ImpementsAndInheritsSpan);
        }

        IfFailGo(GenerateStatementID(&tempBuffer, pstatement, &ImpementsAndInheritsSpan));

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, pClassSymbol));

        // Process Shadows
        if (pClass->Specifiers->HasSpecifier(ParseTree::Specifier::Shadows))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shadows, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Process Module
        if (pstatement->Opcode == ParseTree::Statement::Module || pClass->Specifiers->HasSpecifier(ParseTree::Specifier::Static))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Module, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Process NotInheritable
        if (pClass->Specifiers->HasSpecifier(ParseTree::Specifier::NotInheritable))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Sealed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Process Abstract
        if (pClass->Specifiers->HasSpecifier(ParseTree::Specifier::MustInherit))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Abstract, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Class name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pClass->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pClass->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Full name for the class
        LocalStackPush.Push(pClass->Name.Name, pClass->Name.IsBracketed);
        IfFailGo(GenerateFullName(&tempBuffer, pClassSymbol, PROCESS_SOURCE_NAME));

        pOldContext = m_pCurrentContext;       // Save the old context
        m_pCurrentContext = pClassSymbol;      // and make the current context = the thingy we are at

        // Is it a special name? (hidden for example?)
        if (pClassSymbol && pClassSymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // We now process the implements list seperatly
        if (pClassSymbol)
        {
            IfFailGo(ProcessImplementsList(&tempBuffer, pClass->Children, pClassSymbol->PClass()->GetFirstImplements(), depthLevel + 1));
        }

        if (m_fProcessTypesOnly == PROCESS_TYPES_ONLY)
        {
            // Go over children one at a time, and process them too
            IfFailGo(ProcessStatementList(&tempBuffer2, pClass->Children, FLAT_DEPTH_LEVEL));
            IfTrueRet(m_fShouldQuote, hr);
        }
        else
        {
            // Go over children one at a time, and process them too
            IfFailGo(ProcessStatementList(&tempBuffer, pClass->Children, depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);
        }

        // And process any attributes used with this class as well
        IfFailGo(ProcessAttributeList(&tempBuffer, pClass->Attributes, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Class>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();

        if (m_fProcessTypesOnly == PROCESS_TYPES_ONLY)
        {
            AppendString(psbBuffer, tempBuffer2.GetString(), NO_INDENTATION, NO_ENTITY_REF_CHECK);
        }
    }

    // Revert back to the outer context...
    m_pCurrentContext = pOldContext;

    return hr;
}

//****************************************************************************
// Process root namespace
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessRootNameSpace(StringBuffer *psbBuffer, _In_opt_z_ STRING* pRootNameSpaceName,
                                     ParseTree::StatementList* pStatementList, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    if (m_fProcessTypesOnly == PROCESS_TYPES_ONLY)
    {
        // Go over children one at a time, and process them too
        IfFailGo(ProcessStatementList(&tempBuffer, pStatementList, FLAT_DEPTH_LEVEL));
    }
    else
    {
        // The number of qualified names is needed here because the </NameSpace> tag is closed here
        ULONG ulItems = 0;      // Number of unqualified names
        STRING **rgpstrQualifiedName = NULL;        // The broken name
        NorlsAllocator nraQualifiedName(NORLSLOC);

        // Breakup the qualified name
        if (pRootNameSpaceName)
        {
            IfFailGo(BreakUpQualifiedName(m_pCompiler, &rgpstrQualifiedName, &nraQualifiedName, &ulItems, pRootNameSpaceName, DOT_DELIMITER));
        }

        if (ulItems == 0)
        {
            AppendString(&tempBuffer, L"<NameSpace", NO_INDENTATION, NO_ENTITY_REF_CHECK);
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pRootNameSpaceName, NO_ENTITY_REF_CHECK, NO_BRACKETS));

            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Full_Name, pRootNameSpaceName, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Root, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));

            // If the root namespace is "", we still hash it and generate the key based on that ""
            ULONG hashValue = StringPool::HashValOfString(STRING_CONST(m_pCompiler, EmptyString));
            // and just convert the hash value to string so that we can dump it
            WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};      // 64-bit int can only take a maximum of 20 chars (will work for 64-bit ints!)

            _ultow_s(hashValue, wszIntValue, _countof(wszIntValue), 10);

            // And dump it as a "key" attribute
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Key, wszIntValue, NO_ENTITY_REF_CHECK, NO_BRACKETS));

            // End Attributes
            AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
        }
        else
        {
            StringBuffer SourceNameBuffer;

            for (ULONG i = 0; i < ulItems; ++i )
            {
                AppendString(&tempBuffer, L"<NameSpace", NO_INDENTATION, NO_ENTITY_REF_CHECK);
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, rgpstrQualifiedName[i], NO_ENTITY_REF_CHECK, NO_BRACKETS));

                m_NameStack.Push(rgpstrQualifiedName[i], NO_BRACKETS);
                m_NameStack.GetBracketedDotQualifiedName(&SourceNameBuffer);

                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Full_Name, SourceNameBuffer.GetString(), NO_ENTITY_REF_CHECK, NO_BRACKETS));
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Root, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));

                // generate the Key from the fullname
                ULONG hashValue = StringPool::HashValOfString(m_pCompiler->AddString(SourceNameBuffer.GetString()));
                // and just convert the hash value to string so that we can dump it
                WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};      // 64-bit int can only take a maximum of 20 chars (will work for 64-bit ints!)
                _ultow_s(hashValue, wszIntValue, _countof(wszIntValue), 10);

                // And dump it as a "key" attribute
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Key, wszIntValue, NO_ENTITY_REF_CHECK, NO_BRACKETS));

                SourceNameBuffer.Clear();

                // End Attributes
                AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
            }
        }

        NorlsAllocator nraSymbols(NORLSLOC);
        Symbols symbols(m_pCompiler, &nraSymbols, NULL);

        m_pCurrentContext = m_pCompiler->GetUnnamedNamespace(m_pSourceFile);

        if (ulItems > 0)
        {
            // Use semantics to bind to the root namespace, which could be in the form "aa.bb.cc". This is why
            // we can't use SimpleBind to do the binding, if we did, a DOT qualified NameSpace binding would
            // break. See VS290673.
            Location Loc;           // Dummy location.
            Loc.Invalidate();
            bool fBadName = false;

            if (m_pCurrentContext && m_pCurrentContext->IsContainer())
            {
                CompilationCaches *pCompilationCaches = GetCompilerCompilationCaches();
                
                // Do the actual binding
                m_pCurrentContext = Semantics::EnsureNamedRoot(
                    Semantics::InterpretQualifiedName(
                        rgpstrQualifiedName,
                        ulItems,
                        NULL,
                        NULL,
                        m_pCurrentContext->PContainer()->GetHash(),
                        NameSearchIgnoreImports | NameSearchIgnoreExtensionMethods,
                        Loc,
                        NULL,
                        m_pCompiler,
                        m_pCompilerProject->GetCompilerHost(),
                        pCompilationCaches,
                        m_pSourceFile,
                        true,          // perform Obsolete checks
                        fBadName));
            }

            if (!m_pCurrentContext || fBadName)
            {
                // If we can't bind to that root namespace, just use the UnnamedNameSpace
                m_pCurrentContext = m_pCompiler->GetUnnamedNamespace(m_pSourceFile);
            }
        }

        // Go over children one at a time, and process them too
        IfFailGo(ProcessStatementList(&tempBuffer, pStatementList, depthLevel + ulItems));

        // Close all <NameSpace> which were opened in ProcessQualifiedNameSpace()
        if (ulItems == 0)
        {
            // The case of no root namespace
            AppendString(&tempBuffer, L"</NameSpace>" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
        }
        else
        {
            for (ULONG i = 0; i < ulItems; ++i )
            {
                AppendString(&tempBuffer, L"</NameSpace>" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
                m_NameStack.Pop();
            }
        }
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a qualified namespace. A qualified namespace looks like the following:
// Namespace A.B.C
// For this type of namespace, we build up several nested namespaces:
//
// <Namespace name = "A">
//     <Namespace name = "B">
//         <Namespace name = "C">
//               ...
//               ...
//               ...
//         </Namespace>
//     </Namespace>
// </Namespace>
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessQualifiedNameSpace(StringBuffer *psbBuffer,
                                          ParseTree::Statement *pstatement,
                                          ParseTree::Name *pname,
                                          ULONG *numberOfNestedNamespaces)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ParseTree::NamespaceStatement *pnamespace = pstatement->AsNamespace();

    // If the namespace is qualified, we need to fake the nesting of
    // the individual componenets of the namespace.
    if (pname && pname->IsQualified())
    {
        ProcessQualifiedNameSpace(psbBuffer, pstatement, pname->AsQualified()->Base, numberOfNestedNamespaces);
    }

    AppendString(&tempBuffer, L"<NameSpace", *numberOfNestedNamespaces, NO_ENTITY_REF_CHECK);
    IfFailGo(GenerateStatementID(&tempBuffer, pstatement)); // Each one of the components (a.b.c) will have the same ID
    ++(*numberOfNestedNamespaces);          // Increament the count of nested NameSpaces

    // NameSpace name
    if (pname && pname->IsQualified())
    {
        m_NameStack.Push(pname->AsQualified()->Qualifier.Name, pname->AsQualified()->Qualifier.IsBracketed);
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pname->AsQualified()->Qualifier.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pname->AsQualified()->Qualifier.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }
    }
    else
    {
        m_NameStack.Push(pname->AsSimple()->ID.Name, pname->AsSimple()->ID.IsBracketed);
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pname->AsSimple()->ID.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pname->AsSimple()->ID.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }
    }

    BCSYM_NamedRoot *pNameSpaceNamedRoot = NULL;     // NamedRoot for NameSpace symbol
    BCSYM *pNameSpaceSymbol = NULL;                  // BCSYM for NameSpace symbol

    IfFailGo(GetSymbolFromName(pname, NULL, &pNameSpaceSymbol, NameSearchIgnoreImports));

    if (pNameSpaceSymbol && pNameSpaceSymbol->IsNamedRoot())
    {
        pNameSpaceNamedRoot = pNameSpaceSymbol->PNamedRoot();
    }

    m_pCurrentContext = pNameSpaceNamedRoot;  // current context = the thingy we are at

    if (pNameSpaceNamedRoot)
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Full_Name,
            pNameSpaceNamedRoot->GetQualifiedName(false), NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }
    else
    {
        StringBuffer PartialQualifiedName;
        m_NameStack.GetBracketedDotQualifiedName(&PartialQualifiedName);

        // the fullname
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Full_Name, PartialQualifiedName.GetString(), NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // generate the Key from the fullname
        ULONG hashValue = StringPool::HashValOfString(m_pCompiler->AddString(PartialQualifiedName.GetString()));
        // and just convert the hash value to string so that we can dump it
        WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};      // 64-bit int can only take a maximum of 20 chars (will work for 64-bit ints!)

        _ultow_s(hashValue, wszIntValue, _countof(wszIntValue), 10);

        // And dump it as a "key" attribute
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Key, wszIntValue, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

    // Is it a special name? (hidden for example?)
    if (pNameSpaceNamedRoot && pNameSpaceNamedRoot->IsSpecialName())
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

    // End Attributes
    AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);


Error:

    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a namespace
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessNameSpace(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    BCSYM_NamedRoot *pOldContext = m_pCurrentContext;                        // Save the old context
    SmartPushPopObject LocalStackPush(&m_NameStack);

    ParseTree::NamespaceStatement *pnamespace = pstatement->AsNamespace();

    if (m_fProcessTypesOnly == PROCESS_TYPES_ONLY)
    {
        // Go over children one at a time, and process them too
        IfFailGo(ProcessStatementList(&tempBuffer, pnamespace->Children, FLAT_DEPTH_LEVEL));
    }
    else
    {
        // If the namespace is qualified, we need to fake the nesting of
        // the individual componenets of the namespace.
        if (pnamespace && pnamespace->Name && pnamespace->Name->IsQualified())
        {
            // The number of qualified names is needed here because the </NameSpace> tag is closed here
            ULONG numberOfNestedNamespaces = 0;
            IfFailGo(ProcessQualifiedNameSpace(&tempBuffer, pstatement, pnamespace->Name, &numberOfNestedNamespaces));

            // Go over children one at a time, and process them too
            IfFailGo(ProcessStatementList(&tempBuffer, pnamespace->Children, depthLevel + numberOfNestedNamespaces));

            // Close all <NameSpace> which were opened in ProcessQualifiedNameSpace()
            for ( ; numberOfNestedNamespaces > 0 ; --numberOfNestedNamespaces)
            {
                AppendString(&tempBuffer, L"</NameSpace>" DEBUG_NEW_LINE, depthLevel + numberOfNestedNamespaces - 1, NO_ENTITY_REF_CHECK);
                m_NameStack.Pop();
            }

            if (m_fShouldQuote)
            {
                goto Error;
            }
        }
        else
        {
            AppendString(&tempBuffer, L"<NameSpace", depthLevel, NO_ENTITY_REF_CHECK);
            {
                IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

                // NameSpace name
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, StringFromParseTreeName(m_pCompiler, pnamespace->Name), NO_ENTITY_REF_CHECK, NO_BRACKETS));

                BCSYM_NamedRoot *pNameSpaceNamedRoot = NULL;     // NamedRoot for NameSpace symbol
                BCSYM *pNameSpaceSymbol = NULL;                  // BCSYM for NameSpace symbol

                IfFailGo(GetSymbolFromName(pnamespace->Name, NULL, &pNameSpaceSymbol, NameSearchIgnoreImports));

                if (pNameSpaceSymbol && pNameSpaceSymbol->IsNamedRoot())
                {
                    pNameSpaceNamedRoot = pNameSpaceSymbol->PNamedRoot();
                }

                m_pCurrentContext = pNameSpaceNamedRoot;                // and make the current context = the thingy we are at

                if (pnamespace->Name->AsSimple()->ID.IsBracketed)
                {
                    IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
                }

                LocalStackPush.Push(pnamespace->Name->AsSimple()->ID.Name, pnamespace->Name->AsSimple()->ID.IsBracketed);

                if (pNameSpaceNamedRoot)
                {
                    IfFailGo(GenerateFullName(&tempBuffer, pNameSpaceNamedRoot, PROCESS_SOURCE_NAME));
                }
                else
                {
                    // If there's a conflict (say a module and a namespace named aaa then we will hit this)
                    IfFailGo(GenerateGenericAttributeFromName(&tempBuffer, ATTRIB_VALUE_Full_Name, pnamespace->Name, NO_BRACKETS));
                    IfFailGo(GenerateGenericAttributeFromName(&tempBuffer, ATTRIB_VALUE_Source_Name, pnamespace->Name, USE_BRACKETS));
                }

                // Is it a special name? (hidden for example?)
                if (pNameSpaceNamedRoot && pNameSpaceNamedRoot->IsSpecialName())
                {
                    IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
                }

                // End Attributes
                AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

                // Go over children one at a time, and process them too
                IfFailGo(ProcessStatementList(&tempBuffer, pnamespace->Children, depthLevel + 1));
                IfTrueRet(m_fShouldQuote, hr);
            }
            AppendString(&tempBuffer, L"</NameSpace>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
        }
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    // Revert back to the outer context...
    m_pCurrentContext = pOldContext;

    return hr;
}

//****************************************************************************
// Process a struct
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessStructure(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    BCSYM_NamedRoot *pOldContext = NULL;       // Save the old context

    // We use this second buffer in order to put everything nested in a seperate buffer
    // and then later we can append this buffer to the original ine. This will flatten out
    // the nesting for us.
    StringBuffer tempBuffer2;
    tempBuffer2.SetAllocationSize(StringBuffer::AllocSize::Large);
    SmartPushPopObject LocalStackPush(&m_NameStack);

    AppendString(&tempBuffer, L"<Class", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Get the symbol if we can
        BCSYM_NamedRoot *pStructureSymbol = NULL;
        ParseTree::TypeStatement *pStruct = pstatement->AsType();
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&pStruct->Name.TextSpan, &pStructureSymbol));

#if DEBUG
        if (!pStructureSymbol)
        {
            VSFAIL(L"NULL Structure Symbol");

#if IDE
            m_pSourceFile->DumpSymbolsHashTable();
#endif IDE
        }
#endif DEBUG

        // We can't do anything with bad symbols, so punt right away
        if (!pStructureSymbol || pStructureSymbol->IsBad())
        {
            m_fShouldQuote = true;
            return hr;
        }

        VSASSERT(pStructureSymbol->IsClass(), L"Bad struct symbol!");

        // Find the End Location for the inherits and implements list.
        Location ImpementsAndInheritsSpan;
        ImpementsAndInheritsSpan.Invalidate();

        // Also, get the span for the Implements and Inherits span
        if (pStruct->Children)
        {
            GetEndLocationForImplementsAndInherits(pStruct->Children, &ImpementsAndInheritsSpan);
        }

        IfFailGo(GenerateStatementID(&tempBuffer, pstatement, &ImpementsAndInheritsSpan));

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, pStructureSymbol));

        // This is a property of a the struct
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Value, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // Struct name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pStruct->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pStruct->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Full name for the struct
        LocalStackPush.Push(pStruct->Name.Name, pStruct->Name.IsBracketed);
        IfFailGo(GenerateFullName(&tempBuffer, pStructureSymbol, PROCESS_SOURCE_NAME));

        pOldContext = m_pCurrentContext;       // Save the old context
        m_pCurrentContext = pStructureSymbol;                             // and make the current context = the thingy we are at

        // Is it a special name? (hidden for example?)
        if (pStructureSymbol && pStructureSymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // We now process the implements list seperately
        if (pStructureSymbol)
        {
            // 



            IfFailGo(ProcessImplementsList(&tempBuffer, pStruct->Children, pStructureSymbol->PClass()->GetFirstImplements(), depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);
        }

        if (m_fProcessTypesOnly == PROCESS_TYPES_ONLY)
        {
            // Go over children one at a time, and process them too
            IfFailGo(ProcessStatementList(&tempBuffer2, pStruct->Children, FLAT_DEPTH_LEVEL));
            IfTrueRet(m_fShouldQuote, hr);
        }
        else
        {
            // Go over children one at a time, and process them too
            IfFailGo(ProcessStatementList(&tempBuffer, pStruct->Children, depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);
        }

        // And process any attributes used with this struct as well
        IfFailGo(ProcessAttributeList(&tempBuffer, pStruct->Attributes, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Class>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();

        if (m_fProcessTypesOnly == PROCESS_TYPES_ONLY)
        {
            AppendString(psbBuffer, tempBuffer2.GetString(), NO_INDENTATION, NO_ENTITY_REF_CHECK);
        }
    }

    // Revert back to the outer context...
    m_pCurrentContext = pOldContext;

    return hr;
}

//****************************************************************************
// Process an interface
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessInterface(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    BCSYM_NamedRoot *pOldContext = NULL;       // Save the old context

    // We use this second buffer in order to put everything nested in a seperate buffer
    // and then later we can append this buffer to the original ine. This will flatten out
    // the nesting for us.
    StringBuffer tempBuffer2;
    tempBuffer2.SetAllocationSize(StringBuffer::AllocSize::Large);
    SmartPushPopObject LocalStackPush(&m_NameStack);

    AppendString(&tempBuffer, L"<Interface", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Get the symbol if we can
        ParseTree::TypeStatement *pInterface = pstatement->AsType();
        BCSYM_NamedRoot *pInterfaceSymbol = NULL;
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&pInterface->Name.TextSpan, &pInterfaceSymbol));

#if DEBUG
        if (!pInterfaceSymbol)
        {
            VSFAIL(L"NULL Interface Symbol");

#if IDE
            m_pSourceFile->DumpSymbolsHashTable();
#endif IDE
        }
#endif DEBUG

        // We can't do anything with bad symbols, so punt right away
        if (!pInterfaceSymbol || pInterfaceSymbol->IsBad())
        {
            m_fShouldQuote = true;
            return hr;
        }

        VSASSERT(pInterfaceSymbol->IsInterface(), L"Bad interface symbol!");

        // Find the End Location for the inherits and implements list.
        Location ImpementsAndInheritsSpan;
        ImpementsAndInheritsSpan.Invalidate();

        // Also, get the span for the Implements and Inherits span
        if (pInterface->Children)
        {
            GetEndLocationForImplementsAndInherits(pInterface->Children, &ImpementsAndInheritsSpan);
        }

        IfFailGo(GenerateStatementID(&tempBuffer, pstatement, &ImpementsAndInheritsSpan));

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, pInterfaceSymbol));

        // Interface name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pInterface->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pInterface->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Full name for the interface
        LocalStackPush.Push(pInterface->Name.Name, pInterface->Name.IsBracketed);
        IfFailGo(GenerateFullName(&tempBuffer, pInterfaceSymbol, PROCESS_SOURCE_NAME));

        pOldContext = m_pCurrentContext;       // Save the old context
        m_pCurrentContext = pInterfaceSymbol;                             // and make the current context = the thingy we are at

        // Is it a special name? (hidden for example?)
        if (pInterfaceSymbol && pInterfaceSymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        if (m_fProcessTypesOnly == PROCESS_TYPES_ONLY)
        {
            // Go over children one at a time, and process them too
            IfFailGo(ProcessStatementList(&tempBuffer2, pInterface->Children, FLAT_DEPTH_LEVEL));
            IfTrueRet(m_fShouldQuote, hr);
        }
        else
        {
            // Go over children one at a time, and process them too
            IfFailGo(ProcessStatementList(&tempBuffer, pInterface->Children, depthLevel + 1));
        }

        // And process any attributes used with this interface as well
        IfFailGo(ProcessAttributeList(&tempBuffer, pInterface->Attributes, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Interface>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();

        if (m_fProcessTypesOnly == PROCESS_TYPES_ONLY)
        {
            AppendString(psbBuffer, tempBuffer2.GetString(), NO_INDENTATION, NO_ENTITY_REF_CHECK);
        }
    }

    // Revert back to the outer context...
    m_pCurrentContext = pOldContext;

    return hr;
}

//****************************************************************************
// Process an event declaration
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessEventDeclaration(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    SmartPushPopObject LocalStackPush(&m_NameStack);

    AppendString(&tempBuffer, L"<Event", depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

        ParseTree::MethodDeclarationStatement *pevent = pstatement->AsMethodDeclaration();
        BCSYM_NamedRoot *pEventSymbol = NULL;

        // Get the symbol if we can
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&pevent->Name.TextSpan, &pEventSymbol));

        // We can't get anything from a NULL symbol, so punt right away
        if (!pEventSymbol)
        {
            m_fShouldQuote = true;
            return hr;
        }

        VSASSERT(pEventSymbol->IsEventDecl(), L"Bad event symbol!");

        // If the proc is bad because one or more of it's types (parameters or return type) are bad, then we can still
        // generate XML for that proc, but if it is bad because of any other reason, then we don't know what exactly is
        // wrong, and we give up.
        if (pEventSymbol->IsBad() && !pEventSymbol->PProc()->IsBadParamTypeOrReturnType())
        {
            m_fShouldQuote = true;
            return hr;
        }

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, pEventSymbol));

        // Declared with shadows
        if (pevent->Specifiers->HasSpecifier(ParseTree::Specifier::Shadows))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shadows, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Static
        if (pevent->Specifiers->HasSpecifier(ParseTree::Specifier::Static))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Static, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Declared as Shared
        if (pevent->Specifiers->HasSpecifier(ParseTree::Specifier::Shared))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shared, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Event name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pevent->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pevent->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Full name for the Event
        LocalStackPush.Push(pevent->Name.Name, pevent->Name.IsBracketed);
        IfFailGo(GenerateFullName(&tempBuffer, pEventSymbol, PROCESS_SOURCE_NAME));

        // Is it a special name? (hidden for example?)
        if (pEventSymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Process the signature
        IfFailGo(ProcessSignature(&tempBuffer, pevent->AsMethodSignature(),
                                  pEventSymbol->PProc() ? pEventSymbol->PProc() : NULL,
                                  depthLevel + 1, TypeOf(pEventSymbol)));

        IfTrueRet(m_fShouldQuote, hr);

        // And process implements and handles statments (if any)
        IfFailGo(ProcessEventImplements(&tempBuffer, pevent->Implements, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        if (pEventSymbol->IsEventDecl())
        {
            // We now want to generate Type or Delegate that this Event declares
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, pEventSymbol->PEventDecl()->GetRawDelegate(), NULL, NULL, depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);
        }
    }
    AppendString(&tempBuffer, L"</Event>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process an enum declaration
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessEnumDeclaration(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    SmartPushPopObject LocalStackPush(&m_NameStack);

    BCSYM_NamedRoot *pOldContext = NULL;       // Save the old context

    AppendString(&tempBuffer, L"<Enumeration", depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

        ParseTree::TypeStatement *penum = pstatement->AsType();
        BCSYM_NamedRoot *pEnumSymbol = NULL;

        // Get the symbol if we can
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&penum->Name.TextSpan, &pEnumSymbol));

#if DEBUG
        if (!pEnumSymbol)
        {
            VSFAIL(L"NULL Enum Symbol");

#if IDE
            m_pSourceFile->DumpSymbolsHashTable();
#endif IDE
        }
#endif DEBUG

        // We can't do anything with bad symbols, so punt right away
        if (!pEnumSymbol || pEnumSymbol->IsBad())
        {
            m_fShouldQuote = true;
            return hr;
        }

        VSASSERT(pEnumSymbol->IsEnum(), L"Bad enum symbol!");

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, pEnumSymbol));

        // Enum name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, penum->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (penum->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Full name for the Enum
        LocalStackPush.Push(penum->Name.Name, penum->Name.IsBracketed);
        IfFailGo(GenerateFullName(&tempBuffer, pEnumSymbol, PROCESS_SOURCE_NAME));

        pOldContext = m_pCurrentContext;       // Save the old context
        m_pCurrentContext = pEnumSymbol;                             // and make the current context = the thingy we are at

        // Is it a special name? (hidden for example?)
        if (pEnumSymbol && pEnumSymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        ParseTree::EnumTypeStatement *penumtype = pstatement->AsEnumType();
        ParseTree::Type *pUnderlyingRepresentation = penumtype->UnderlyingRepresentation;

        IfFailGo(ProcessTypeStatement(&tempBuffer,pUnderlyingRepresentation, NULL, depthLevel + 1));

        // We don't want to process any of the members of this Enum if we are only processing types
        if (m_fProcessTypesOnly == PROCESS_ALL_DECLS)
        {
            // Go over children one at a time, and process them too
            IfFailGo(ProcessStatementList(&tempBuffer, penum->Children, depthLevel + 1));
        }

        // And process any attributes used with this enum as well
        IfFailGo(ProcessAttributeList(&tempBuffer, penum->Attributes, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Enumeration>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    // Revert back to the outer context...
    m_pCurrentContext = pOldContext;

    return hr;
}

//****************************************************************************
// Process a delegate declaration.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessDelegateDeclaration(StringBuffer *psbBuffer, ParseTree::Statement *pstatement, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    bool fIsFunction = false;
    SmartPushPopObject LocalStackPush(&m_NameStack);

    BCSYM_NamedRoot *pOldContext = NULL;       // Save the old context

    AppendString(&tempBuffer, L"<Delegate", depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(GenerateStatementID(&tempBuffer, pstatement));

        ParseTree::MethodDeclarationStatement *pdelegate = pstatement->AsMethodDeclaration();
        BCSYM_NamedRoot *pDelegateSymbol = NULL;

        // Get the symbol if we can
        IfFailGo(m_pSourceFile->GetSymbolOfLocation(&pdelegate->Name.TextSpan, &pDelegateSymbol));

        // We can't get anything from a NULL or bad symbol, so punt right away
        if (!pDelegateSymbol || pDelegateSymbol->IsBad())
        {
            m_fShouldQuote = true;
            return hr;
        }

        VSASSERT(pDelegateSymbol->IsDelegate(), L"Bad delegate symbol!");

        // Dump the access attributes (public, private, protected, ...)
        IfFailGo(GenerateAccess(&tempBuffer, pDelegateSymbol));

        // If this a delegate function instead of a delegate Sub, we generate the "function" flag
        if (pstatement->Opcode == ParseTree::Statement::DelegateFunctionDeclaration)
        {
            fIsFunction = true;
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Function, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // declared with shadows
        if (pdelegate->Specifiers->HasSpecifier(ParseTree::Specifier::Shadows))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Shadows, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Delegate name
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pdelegate->Name.Name, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        if (pdelegate->Name.IsBracketed)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Bracketed, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Full name for the Delegate
        LocalStackPush.Push(pdelegate->Name.Name, pdelegate->Name.IsBracketed);
        IfFailGo(GenerateFullName(&tempBuffer, pDelegateSymbol, PROCESS_SOURCE_NAME));

        pOldContext = m_pCurrentContext;       // Save the old context
        m_pCurrentContext = pDelegateSymbol;                             // and make the current context = the thingy we are at

        // Is it a special name? (hidden for example?)
        if (pDelegateSymbol->IsSpecialName())
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Special_Name, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        BCSYM_NamedRoot *pInvokeProc = NULL;

        // The Invoke proc is needed for processing the parameters of the delegate
        pInvokeProc = GetInvokeFromDelegate(pDelegateSymbol, m_pCompiler);

        // Process the signature
        IfFailGo(ProcessSignature(&tempBuffer, pdelegate->AsMethodSignature(),
                                  pInvokeProc && pInvokeProc->IsProc() ? pInvokeProc->PProc() : NULL,
                                  depthLevel + 1, TypeOf(pInvokeProc)));

        IfTrueRet(m_fShouldQuote, hr);

        // And process any attributes used with this delegate as well
        IfFailGo(ProcessAttributeList(&tempBuffer, pdelegate->AsMethodSignature()->Attributes, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Delegate>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    // Revert back to the outer context...
    m_pCurrentContext = pOldContext;

    return hr;
}

//****************************************************************************
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessBlock(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // If this is not block node, we don't have anything to do
    if (!PILNode || !PILNode->IsExecutableBlockNode())
    {
        return hr;
    }

    AppendString(psbBuffer, L"<Block>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Temp string to work with so that we can bail out when we see errors.
        AppendStringBuffer tempBuffer(psbBuffer);

        // Process all children. Note that if we do see a statment node, we get the next one so that we
        // process the actual thing, not the statment node.
        for (ILTree::PILNode pBilTreeStmt = PILNode->AsExecutableBlock().ptreeChild ; pBilTreeStmt ; pBilTreeStmt = pBilTreeStmt->AsStatement().Next)
        {
            VSASSERT(pBilTreeStmt->bilop != SL_STMT || pBilTreeStmt->AsStatementWithExpression().ptreeOp1,
                     "empty statement unexpected. : #11/12/2003#");

            if (!pBilTreeStmt)
            {
                break;
            }

            // We just let the case statments handle whatever typeof block
            IfFailGo(ProcessMethodBodyStatement(&tempBuffer, pBilTreeStmt, depthLevel + 1));

            // Let's see if we need to quote this block
            if (m_fShouldQuote == true)
            {
                // If we are the statment level, we can quote it here
                if (pBilTreeStmt->IsStmtNode() || depthLevel == INITIAL_DEPTH_LEVEL)
                {
                    IfFailGo(ProcessQuote(psbBuffer, pBilTreeStmt, NULL, depthLevel + 1));
                    m_fShouldQuote = false;
                }
            }
            else
            {
                tempBuffer.Commit();
            }
        }
    }
    AppendString(psbBuffer, L"</Block>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    return hr;
}


//============================================================================
// Toplevel function for generating the method bodies in a file
//
// Return: Success or catstrophic failure code.
//============================================================================
HRESULT XMLGen::ProcessMethodBody(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Process the whole method body block
    m_fUseTrueQualifiedName = true;
    IfFailGo(ProcessBlock(psbBuffer, PILNode, depthLevel));
    m_fUseTrueQualifiedName = false;

Error:
    return hr;
}

//****************************************************************************
// Process an Goto statement
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessGotoStmt(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);


    AppendString(&tempBuffer, L"<Goto>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        if (PILNode->AsGotoStatement().Label != NULL)
        {
            // Generate the actual label we are going to
            GenerateGenericElement(&tempBuffer, NULL, PILNode->AsGotoStatement().Label->Name, CHECK_ENTITY_REF, NO_BRACKETS);
        }
    }
    AppendString(&tempBuffer, L"</Goto>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a Label statement
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessLabelStmt(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Label>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Generate the actual label
        GenerateGenericElement(&tempBuffer, NULL, PILNode->AsLabelStatement().Label->Name, CHECK_ENTITY_REF, NO_BRACKETS);
    }
    AppendString(&tempBuffer, L"</Label>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a While ... Wend block. This is the case where we have the condition
// at the top of the loop.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessWhileLoop(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::LoopBlock bilLoopTree = PILNode->AsLoopBlock();

    AppendString(&tempBuffer, L"<While>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // If the termination expression is there, goahead and use it, other wise we spit out a TRUE
        if ((PILNode->AsLoopBlock()).Conditional)
        {
            IfFailGo(ProcessExpression(&tempBuffer, (PILNode->AsLoopBlock()).Conditional, depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);
        }
        else
        {
            AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
            {
                AppendString(&tempBuffer, L"<UnaryOperation", depthLevel + 2, NO_ENTITY_REF_CHECK);
                {
                    IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Unary_Operator, ATTRIB_VALUE_True, NO_ENTITY_REF_CHECK, NO_BRACKETS));

                    // End Attributes
                    AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
                }
                AppendString(&tempBuffer, L"</UnaryOperation>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
            }
            AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        }

        // Process the body of the while loop
        IfFailGo(ProcessBlock(&tempBuffer, PILNode, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</While>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a do ... {while | until} block. This is the case where we have the
// condition at the bottom of the loop.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessDoLoop(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::LoopBlock bilLoopTree = PILNode->AsLoopBlock();

    AppendString(&tempBuffer, L"<Do>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // The termination expression
        IfFailGo(ProcessExpression(&tempBuffer, (PILNode->AsLoopBlock()).Conditional, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        // Process the body of the do loop
        IfFailGo(ProcessBlock(&tempBuffer, PILNode, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</Do>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a For Each ... Next block
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessForEachLoop(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::ForEachBlock bilEachTree = PILNode->AsForEachBlock();

    AppendString(&tempBuffer, L"<ForEach>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // This is for the FieldAccess/LocalAccess/ArrayElementAccess
        IfFailGo(ProcessExpression(&tempBuffer, bilEachTree.ControlVariableReference, depthLevel + 1, NESTED_EXPRESSION));
        IfTrueRet(m_fShouldQuote, hr);

        // This is whatever we are iterating over
        IfFailGo(ProcessExpression(&tempBuffer, bilEachTree.Collection, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        // Process the body of the for loop
        IfFailGo(ProcessBlock(&tempBuffer, PILNode, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</ForEach>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a For ... Next block
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessForLoop(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::ForBlock bilForTree = PILNode->AsForBlock();

    AppendString(&tempBuffer, L"<For>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // The initialization statements
        AppendString(&tempBuffer, L"<Block>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        {
            AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
            {
                AppendString(&tempBuffer, L"<Assignment", depthLevel + 3, NO_ENTITY_REF_CHECK);
                {
                    IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Equals, NO_ENTITY_REF_CHECK, NO_BRACKETS));

                    // End Attributes
                    AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

                    // Process left side of assignment
                    IfFailGo(ProcessExpression(&tempBuffer, bilForTree.ControlVariableReference, depthLevel + 4));
                    IfTrueRet(m_fShouldQuote, hr);

                    // Process right side of assignment
                    IfFailGo(ProcessExpression(&tempBuffer, bilForTree.Initial, depthLevel + 4));
                    IfTrueRet(m_fShouldQuote, hr);
                }
                AppendString(&tempBuffer, L"</Assignment>" DEBUG_NEW_LINE, depthLevel + 3, NO_ENTITY_REF_CHECK);
            }
            AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
        }
        AppendString(&tempBuffer, L"</Block>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);

        // The termination expression
        IfFailGo(ProcessExpression(&tempBuffer, bilForTree.Limit, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        // The increment statements
        AppendString(&tempBuffer, L"<Block>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        {
            IfFailGo(ProcessExpression(&tempBuffer, bilForTree.Step, depthLevel + 2));
            IfTrueRet(m_fShouldQuote, hr);
        }
        AppendString(&tempBuffer, L"</Block>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);

        // Process the body of the for loop
        IfFailGo(ProcessBlock(&tempBuffer, PILNode, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</For>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process an If block. This method will process all ElseIf and Else in the
// if block.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessIfBlock(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<If>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Process all children. Here, we loop over all ElseIfs and the last Else
        for (ILTree::PILNode pBilTreeStmt = PILNode->AsIfGroup().ptreeChild ; pBilTreeStmt ; pBilTreeStmt = pBilTreeStmt->AsStatement().Next)
        {
            // We just let the case statments handle the ElseIf/Else blocks
            IfFailGo(ProcessMethodBodyStatement(&tempBuffer, pBilTreeStmt, depthLevel + 1));
        }
    }
    AppendString(&tempBuffer, L"</If>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process an If statement
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessIf_Else(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Process the conditional expression
    IfFailGo(ProcessExpression(psbBuffer, PILNode->AsIfBlock().Conditional, depthLevel));
    IfTrueRet(m_fShouldQuote, hr);

    // Process the If block
    IfFailGo(ProcessBlock(psbBuffer, PILNode, depthLevel));

Error:
    return hr;
}

//****************************************************************************
// Process a statment inside a method body
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessExpressionStmt(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    if (!PILNode->AsStatementWithExpression().ptreeOp1)
    {
        return hr;
    }

    AppendString(&tempBuffer, L"<ExpressionStatement", depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Generate the line number so that designers can genrate proper error messages.
        IfFailGo(GenerateLineNumber(&tempBuffer, PILNode->AsStatementWithExpression().ptreeOp1->Loc.m_lBegLine));

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsStatementWithExpression().ptreeOp1, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</ExpressionStatement>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a GetType construct
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessGetType(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (PILNode->AsBinaryExpression().Left->bilop == SX_BAD || IsBad(PILNode->AsBinaryExpression().Left))
    {
        m_fShouldQuote = true;
        return hr;
    }

    if (PILNode->AsExpressionWithChildren().Left->bilop == SX_NOTHING)
    {
        return ProcessTypeFromBCSYM(psbBuffer, PILNode->AsBinaryExpression().Left->ResultType, NULL, NULL, depthLevel);
    }
    else
    {
        VSASSERT(PILNode->AsExpressionWithChildren().Left->bilop == SX_SYM, "Unexpected!");
        return ProcessNameRef(psbBuffer, PILNode->AsExpressionWithChildren().Left, depthLevel + 1);
    }
}

//****************************************************************************
// Process a late bound statement
//
// Return: Success or catstrophic failure code.
//****************************************************************************
void XMLGen::ProcessLateStmt(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    if (m_BN_WITH > 0)
    {
        AppendString(psbBuffer, L"<WithAccess>", depthLevel, NO_ENTITY_REF_CHECK);
    }
    else
    {
        return;
    }
    {
        ProcessExpression(psbBuffer, PILNode->AsLateBoundExpression().LateIdentifier, NO_INDENTATION);
    }
    if (m_BN_WITH > 0)
    {
        AppendString(psbBuffer, L"</WithAccess>" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }
}


//****************************************************************************
// Process Continue statement
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessContinueStatement(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Continue>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
    }
    AppendString(&tempBuffer, L"</Continue>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}


//****************************************************************************
// Process Break statement
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessExitStatment(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    // Type of statement to exit.
    EXITKIND exitkind = PILNode->AsExitStatement().exitkind;

    AppendString(&tempBuffer, L"<Break", depthLevel, NO_ENTITY_REF_CHECK);
    {
        switch (exitkind)
        {
        case EXIT_DO :                      // This is for While loops
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_BreakKind, ATTRIBUTE_VALUE_Do, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case EXIT_FOR :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_BreakKind, ATTRIBUTE_VALUE_For, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case EXIT_TRY :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_BreakKind, ATTRIBUTE_VALUE_Try, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case EXIT_SELECT :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_BreakKind, ATTRIBUTE_VALUE_Select, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        default :
            VSFAIL(L"Unknown Exit Kind!");
            m_fShouldQuote = true;
            return hr;
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }
    AppendString(&tempBuffer, L"</Break>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}


//****************************************************************************
// Process Break statement
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessReturnStatment(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);


    AppendString(&tempBuffer, L"<Return>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        if (PILNode->AsReturnStatement().ReturnExpression)
        {
            IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsReturnStatement().ReturnExpression, depthLevel + 1));
        }
    }
    AppendString(&tempBuffer, L"</Return>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}


//****************************************************************************
// Process an End statement
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessEndStatment(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<End>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    AppendString(&tempBuffer, L"</End>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a Stop statement
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessStopStatment(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Stop>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    AppendString(&tempBuffer, L"</Stop>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a With block
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessWithBlock(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::WithBlock bilWithTree = PILNode->AsWithBlock();

    AppendString(&tempBuffer, L"<With>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // This is normally the expression for the with statement
        if (bilWithTree.ptreeWith)
        {
            IfFailGo(ProcessExpression(&tempBuffer, bilWithTree.ptreeWith->AsBinaryExpression().Right, depthLevel + 1));
        }
        // VS 535938: Sometimes the expression is in the other field.
        else if (bilWithTree.RecordReference)
        {
            IfFailGo(ProcessExpression(&tempBuffer, bilWithTree.RecordReference, depthLevel + 1));
        }
        else
        {
            VSFAIL("No information for with block");
        }
        IfTrueRet(m_fShouldQuote, hr);

        // Process the body of the with block
        ++m_BN_WITH;
        IfFailGo(ProcessBlock(&tempBuffer, PILNode, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</With>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    --m_BN_WITH;
    return hr;
}

//****************************************************************************
// Process a Try block
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessTryBlock(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::TryBlock bilTryTree = PILNode->AsTryBlock();

    AppendString(&tempBuffer, L"<Try>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // The code to under the try section
        IfFailGo(ProcessBlock(&tempBuffer, PILNode, depthLevel + 1));

        ILTree::PILNode pBilTreeNext = PILNode->AsStatement().Next;

        // We go ahead and process all catches here
        while (pBilTreeNext && pBilTreeNext->bilop == SB_CATCH)
        {
            IfFailGo(ProcessCatchBlock(&tempBuffer, pBilTreeNext, depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);

            pBilTreeNext = pBilTreeNext->AsStatement().Next;
        }

        // And also the finally as well. There should only be one, but you never know
        while (pBilTreeNext && pBilTreeNext->bilop == SB_FINALLY)
        {
            IfFailGo(ProcessBlock(&tempBuffer, pBilTreeNext, depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);

            pBilTreeNext = pBilTreeNext->AsStatement().Next;
        }
    }
    AppendString(&tempBuffer, L"</Try>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a Catch block
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessCatchBlock(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::CatchBlock bilCatchTree = PILNode->AsCatchBlock();

    AppendString(&tempBuffer, L"<Catch>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // Porcess the catch variable, if any
        if (bilCatchTree.CatchVariableTree)
        {
            IfFailGo(ProcessNameRef(&tempBuffer, bilCatchTree.CatchVariableTree, depthLevel + 1));
        }

        // Process the when expression
        IfFailGo(ProcessExpression(&tempBuffer, bilCatchTree.WhenExpression, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        // Process the catch block
        IfFailGo(ProcessBlock(&tempBuffer, PILNode, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</Catch>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process an Select block
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessSelectBlock(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::SelectBlock BilSelTree = PILNode->AsSelectBlock();

    AppendString(&tempBuffer, L"<Switch>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // this is the expression to switch on
        IfFailGo(ProcessExpression(&tempBuffer, ((PILNode->AsSelectBlock()).SelectorCapture->AsBinaryExpression()).Right, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        // process all case statments that may follow
        for (ILTree::PILNode bilCaseTree = PILNode->AsSelectBlock().ptreeChild ; bilCaseTree ; bilCaseTree = bilCaseTree->AsStatement().Next)
        {
            IfFailGo(ProcessCaseStatement(&tempBuffer, bilCaseTree, depthLevel + 1));
        }
    }
    AppendString(&tempBuffer, L"</Switch>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process an Case statement
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessCaseStatement(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::CASELIST *pcaseList = PILNode->AsCaseBlock().BoundCaseList;

    AppendString(&tempBuffer, L"<Case>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // If caseList is non-Null, then there is an expression in the case statement, other wise
        // there isn't one (The Else part)
        if (pcaseList)
        {
            AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
            {
                switch (pcaseList->RelationalOpcode)
                {
                case SX_EQ :
                    AppendString(&tempBuffer, L"<BinaryOperation binaryoperator=\"equals\">" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
                    break;

                case SX_NE :
                    AppendString(&tempBuffer, L"<BinaryOperation binaryoperator=\"notequals\">" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
                    break;

                case SX_LE :
                    AppendString(&tempBuffer, L"<BinaryOperation binaryoperator=\"lessthanorequals\">" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
                    break;

                case SX_GE :
                    AppendString(&tempBuffer, L"<BinaryOperation binaryoperator=\"greaterthanorequals\">" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
                    break;

                case SX_LT :
                    AppendString(&tempBuffer, L"<BinaryOperation binaryoperator=\"lessthan\">" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
                    break;

                case SX_GT :
                    AppendString(&tempBuffer, L"<BinaryOperation binaryoperator=\"greaterthan\">" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
                    break;

                default:
                    // Processes a range of valid values
                    AppendString(&tempBuffer, L"<BinaryOperation binaryoperator=\"to\">" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
                    // Process lower range
                    IfFailGo(ProcessExpression(&tempBuffer, pcaseList->LowBound, depthLevel + 3));
                    IfTrueRet(m_fShouldQuote, hr);

                    // Process upper range
                    IfFailGo(ProcessExpression(&tempBuffer, pcaseList->HighBound, depthLevel + 3));
                    IfTrueRet(m_fShouldQuote, hr);
                    break;
                }

                if (pcaseList->RelationalOpcode)
                {
                    // The expression to check for True or False. This is always uses the "special" tag
                    ProcessSpecial(&tempBuffer, PILNode, depthLevel + 3);
                    IfFailGo(ProcessExpression(&tempBuffer, pcaseList->LowBound, depthLevel + 3));
                    IfTrueRet(m_fShouldQuote, hr);
                }
                AppendString(&tempBuffer, L"</BinaryOperation>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
            }
            AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        }

        IfFailGo(ProcessBlock(&tempBuffer, PILNode, depthLevel + 1));

        // Now, process the block that is executed if the case expression matches...
        // IfFailGo(ProcessBlock(&tempBuffer, pBilTree->AsCaseBlock().ptreeChild, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</Case>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}


//****************************************************************************
// Process a Special case. this used to Model VB's select/case statments.
//
// Return: Success or catstrophic failure code.
//****************************************************************************
void XMLGen::ProcessSpecial(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    AppendString(psbBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        AppendString(psbBuffer, L"<Special>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        {
            GenerateIDRef(psbBuffer, PILNode->AsCaseBlock().Parent);
        }
        AppendString(psbBuffer, L"</Special>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
    }
    AppendString(psbBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
}

//****************************************************************************
// Process a NameRef kind (local, field, property, method, or Unknown)
// This one takes a BCSYM_NamedRoot *
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessNameRefKind(StringBuffer *psbBuffer, BCSYM_NamedRoot *pnamed)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    // We really don't know what is going on here, we just don't want to crash
    if (!pnamed)
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Unknown, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        goto Error;
    }

    switch (pnamed->GetKind())
    {
    case SYM_Variable :
    case SYM_VariableWithValue :
    case SYM_VariableWithArraySizes :
        {
            if (pnamed->PVariable()->GetVarkind() == VAR_Local ||
                pnamed->PVariable()->GetVarkind() == VAR_FunctionResult ||
                pnamed->PVariable()->GetVarkind() == VAR_Param)
            {
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Local, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            }
            else if (pnamed->PVariable()->GetVarkind() == VAR_Const)
            {
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Field, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            }
            else if (pnamed->PVariable()->GetVarkind() == VAR_Member || pnamed->PVariable()->GetVarkind() == VAR_WithEvents)
            {
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Field, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            }
            else
            {
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Unknown, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            }
        }
        break;

    case SYM_MethodImpl :
    case SYM_MethodDecl :
    case SYM_SyntheticMethod :
    case SYM_UserDefinedOperator :
        {
            if (!pnamed->PProc()->IsProcedure())
            {
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Property, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            }
            else
            {
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Method, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            }
        }
        break;

        // We don't know what kind of var this is!
    default :
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Unknown, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        break;
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a NameRef kind (local, field, property, method, or Unknown)
// This one takes a PBILTREE
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessNameRefKind(StringBuffer *psbBuffer, ILTree::PILNode PILNode)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    // We really don't know what is going on here, we just don't want to crash
    if (!PILNode)
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Unknown, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        goto Error;
    }

    if (PILNode->bilop == SX_SYM)
    {
        IfFailGo(ProcessNameRefKind(&tempBuffer, PILNode->AsSymbolReferenceExpression().pnamed));
    }
    else
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Unknown, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::GenerateDottedNameRef(
    StringBuffer * psbBuffer,
    _In_count_x_(index + 1) STRING ** rgpstrQualifiedName,
    long index,
    ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    if (index >= 0)
    {
        AppendString(&tempBuffer, L"<NameRef", depthLevel, NO_ENTITY_REF_CHECK);
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Unknown, NO_ENTITY_REF_CHECK, NO_BRACKETS));

            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, rgpstrQualifiedName[index], NO_ENTITY_REF_CHECK, NO_BRACKETS));

            // End Attributes
            AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

            // If we have more names in the array, generate the <Expression> tag first and then
            // call ourselves
            if ((index - 1) >= 0)
            {
                AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);

                // Do the recusrive call here.
                IfFailGo(GenerateDottedNameRef(&tempBuffer, rgpstrQualifiedName, index - 1, depthLevel + 2));
                AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
            }

        }
        AppendString(&tempBuffer, L"</NameRef>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a name reference using a BCSYM_NamedRoot
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessNameRefFromBCSYM(StringBuffer *psbBuffer, BCSYM_NamedRoot *pNameRefSymbol, BCSYM_GenericBinding *pGenericBinding, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    StringBuffer tempBuffer;   // NOTE: not using AppendStringBuffer on purpose!

    if (pNameRefSymbol->IsMember())
    {
        AppendString(&tempBuffer, L"<NameRef", depthLevel, NO_ENTITY_REF_CHECK);
        {
            STRING *EntityName = pNameRefSymbol->IsProc() && pNameRefSymbol->PProc()->IsAnyConstructor() ? STRING_CONST( m_pCompiler, New ) : pNameRefSymbol->GetName();
            IfFailGo(ProcessNameRefKind(&tempBuffer, pNameRefSymbol));
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, EntityName, NO_ENTITY_REF_CHECK, NeedsBrackets(pNameRefSymbol)));
            IfFailGo(GenerateFullName(&tempBuffer, pNameRefSymbol, NO_SOURCE_NAME));

            // End Attributes
            AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
            AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
            {
                AppendString(&tempBuffer, L"<Literal>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
                {
                    IfFailGo(ProcessTypeFromBCSYM(&tempBuffer,
                                                  pNameRefSymbol->GetContainer(),
                                                  NULL,
                                                  NULL,
                                                  depthLevel + 3,
                                                  pGenericBinding));
                }
                AppendString(&tempBuffer, L"</Literal>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
            }
            AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        }
        AppendString(&tempBuffer, L"</NameRef>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }
    else
    {
        // First, get the fully qualified name of the symbol we are trying to generate the fullname for
        STRING *nameRefFullName = pNameRefSymbol->PNamedRoot()->GetQualifiedName(false);

        ULONG ulItems = 0;      // Number of unqualified names
        STRING **rgpstrQualifiedName = NULL;        // The broken name
        NorlsAllocator nraSymbols(NORLSLOC);

        // Breakup the qualified name
        if (nameRefFullName)
        {
            IfFailGo(BreakUpQualifiedName(m_pCompiler, &rgpstrQualifiedName, &nraSymbols, &ulItems, nameRefFullName, DOT_DELIMITER));
        }

        // Now, just go over each name and generate the corresponding NameRef tag
        IfFalseGo(ulItems > 0x7fffffff,E_FAIL);
        IfFailGo(GenerateDottedNameRef(&tempBuffer, rgpstrQualifiedName, (long) ulItems - 1, depthLevel));
    }

Error:
    if (hr == NOERROR)
    {
        AppendString(psbBuffer, tempBuffer.GetString(), NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }

    return hr;
}

//****************************************************************************
// Process a name reference using a biltree
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessNameRef(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Temp string to work with so that we can bail out when we see errors.
    StringBuffer tempBuffer; // NOTE: not using AppendStringBuffer on purpose!

    if (PILNode->bilop == SX_ADR)
    {
        return ProcessExpression(psbBuffer, PILNode->AsBinaryExpression().Left, depthLevel, NESTED_EXPRESSION);
    }

    AppendString(&tempBuffer, L"<NameRef", depthLevel, NO_ENTITY_REF_CHECK);
    {
        switch (PILNode->bilop)
        {
        case SX_SYM :
            {
                // If there is no base reference, but we have a symbol, we use the symbol to generate the NameRef
                // info. This applies to things like System.foo
                if (PILNode->AsSymbolReferenceExpression().pnamed && !PILNode->AsSymbolReferenceExpression().ptreeQual)
                {

                    // Is the this a "Me" or a "Base" reference?
                    if (PILNode->AsSymbolReferenceExpression().uFlags & SXF_SYM_MYBASE)
                    {
                        // If we have a MyBase reference, we ignore the <NameRef> tag generated above
                        GenerateGenericElement(psbBuffer, ELEMENT_Base_Reference, NULL, CHECK_ENTITY_REF, NO_BRACKETS);
                        return hr;
                    }
                    else if (IsThisReference(PILNode->AsSymbolReferenceExpression().pnamed))
                    {
                        // If we have a Me reference, we ignore the <NameRef> tag generated above
                        GenerateGenericElement(psbBuffer, ELEMENT_This_Reference, NULL, CHECK_ENTITY_REF, NO_BRACKETS);
                        return hr;
                    }

                    // This temp string is used to just ignore the <NameRef> tag that was generated above,
                    // since we generate another one in ProcessNameRefFromBCSYM() below...
                    StringBuffer nameRefBuffer;

                    hr = ProcessNameRefFromBCSYM(&nameRefBuffer,
                                                 PILNode->AsSymbolReferenceExpression().pnamed,
                                                 PILNode->AsSymbolReferenceExpression().GenericBindingContext,
                                                 depthLevel);

                    // We just ignore the tempBuffer, and use tempBuffer2 instead
                    if (hr == NOERROR)
                    {
                        AppendString(psbBuffer, nameRefBuffer.GetString(), NO_INDENTATION, NO_ENTITY_REF_CHECK);
                    }

                    return hr;
                }

                IfFailGo(ProcessNameRefKind(&tempBuffer, PILNode->AsSymbolReferenceExpression().pnamed));

                STRING *EntityName = PILNode->AsSymbolReferenceExpression().pnamed->IsProc() && PILNode->AsSymbolReferenceExpression().pnamed->PProc()->IsAnyConstructor() ?
                                     STRING_CONST( m_pCompiler, New ) : PILNode->AsSymbolReferenceExpression().pnamed->GetName();

                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, EntityName, NO_ENTITY_REF_CHECK, NeedsBrackets(PILNode->AsSymbolReferenceExpression().pnamed)));
                IfFailGo(GenerateFullName(&tempBuffer, PILNode->AsSymbolReferenceExpression().pnamed, NO_SOURCE_NAME));

                // End Attributes
                AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

                IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsSymbolReferenceExpression().ptreeQual, depthLevel + 1));
                IfTrueRet(m_fShouldQuote, hr);
                break;
            }

        case SX_CALL :
            {
                IfFailGo(ProcessNameRefKind(&tempBuffer, PILNode->AsCallExpression().Left));

                VSASSERT(PILNode->AsCallExpression().Left->bilop == SX_SYM, "Unexpected!");

                // If there is no basereference, that means that we are at a dotted-name, not a dotted-type.
                // We then use the BCSYM to generate the nameref. This also means that we don't have types
                // for each of the names.
                if (!PILNode->AsCallExpression().ptreeThis && PILNode->AsCallExpression().Left)
                {
                    // This temp string is used to just ignore the <NameRef> tag that was generated above,
                    // since we generate another one in ProcessNameRefFromBCSYM() below...
                    StringBuffer nameRefBuffer;

                    hr = ProcessNameRefFromBCSYM(&nameRefBuffer,
                                                 PILNode->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed,
                                                 PILNode->AsCallExpression().Left->AsSymbolReferenceExpression().GenericBindingContext,
                                                 depthLevel);

                    // We just ignore the tempBuffer, and use tempBuffer2 instead
                    if (hr == NOERROR)
                    {
                        AppendString(psbBuffer, nameRefBuffer.GetString(), NO_INDENTATION, NO_ENTITY_REF_CHECK);
                    }

                    return hr;
                }

                BCSYM_NamedRoot *Symbol = PILNode->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed;
                STRING *EntityName = Symbol->IsProc() && Symbol->PProc()->IsAnyConstructor() ? STRING_CONST(m_pCompiler, New) : Symbol->GetName();
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, EntityName, NO_ENTITY_REF_CHECK, NeedsBrackets(Symbol)));

                // End Attributes
                AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

                IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsCallExpression().ptreeThis, depthLevel + 1));
                IfTrueRet(m_fShouldQuote, hr);

            }
            break;

        case SX_NAME :
            VSFAIL(L"unexpected SX_NAME");
            break;

        default :
            VSFAIL(L"Unknown NameRef node kind");
            m_fShouldQuote = true;
            return hr;
        }
    }
    AppendString(&tempBuffer, L"</NameRef>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        AppendString(psbBuffer, tempBuffer.GetString(), NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }

    return hr;
}

//****************************************************************************
// Process a raise event statment
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessRaiseEvent(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<RaiseEvent>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(ProcessNameRef(&tempBuffer, PILNode, depthLevel + 1));

        // Now, we generate the args.
        IfFailGo(ProcessMethodArguments(&tempBuffer, PILNode->AsCallExpression().Right, depthLevel + 1));
    }
    AppendString(&tempBuffer, L"</RaiseEvent>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes a PropertySet as if it was an assignement. This makes it
// consistent with C#.
// Defers to ProcessPropertySetIndexedAccess is the property has indexers
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessPropertySetAccess(StringBuffer *psbBuffer, ILTree::PILNode PILNode, bool IsWithEventsVariable, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);
    bool bHasIndexers = (PILNode->AsCallExpression().Right && PILNode->AsCallExpression().Right->AsExpressionWithChildren().Right) ? true : false;

    // This code needed to be changed to match C#'s
    AppendString(&tempBuffer, L"<Assignment", depthLevel, NO_ENTITY_REF_CHECK);
    IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Equals, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);


    if (bHasIndexers)
    {
        /* Here's an example of the expected XML this should generate
            <Assignment>
                <Expression>
                    <PropertyAccess>
                        <NameRef>...</NameRef>
                        <Argument>...</Argument> '1 or more
                    </PropertyAccess>
                </Expression>
                <Expression>...</Expression> 'The RHS of the assignment
            </Assignment>
        */
        IfFailGo(ProcessPropertySetIndexedAccess(&tempBuffer, PILNode, IsWithEventsVariable, depthLevel + 1));
    }
    else
    {
        /* Here's an example of the expected XML this should generate
            <Assignment>
                <Expression>
                    <NameRef>...</NameRef>
                </Expression>
                <Expression>...</Expression> 'The RHS of the assignment
            </Assignment>
        */
        AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel+1, NO_ENTITY_REF_CHECK);
        AppendString(&tempBuffer, L"<NameRef", depthLevel + 2, NO_ENTITY_REF_CHECK);

        if (IsWithEventsVariable)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Field, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }
        else
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Property, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        if (PILNode->AsCallExpression().Left && PILNode->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed)
        {
            BCSYM_NamedRoot *pPropertySymbol = PILNode->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed->PProc()->GetAssociatedPropertyDef();

            if (!pPropertySymbol)
            {
                VSFAIL(L"NULL Property Set symbol!");
                m_fShouldQuote = true;
                return hr;
            }

            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pPropertySymbol->GetName(),
                        NO_ENTITY_REF_CHECK, NeedsBrackets(pPropertySymbol)));

            IfFailGo(GenerateFullName(&tempBuffer, pPropertySymbol, NO_SOURCE_NAME));
        }

        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
        IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsCallExpression().ptreeThis, depthLevel + 3));
        AppendString(&tempBuffer, L"</NameRef>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);

        AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);

        ILTree::ILNode *pRHS = PILNode->AsCallExpression().Right;
        // Process RHS
        IfFailGo(ProcessExpression(&tempBuffer, pRHS->AsExpressionWithChildren().Left, depthLevel + 2));

    }


    AppendString(&tempBuffer, L"</Assignment>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);


Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;

}


//****************************************************************************
// Processes a PropertySet with indexers as a PropertyAccess
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessPropertySetIndexedAccess(StringBuffer *psbBuffer, ILTree::PILNode PILNode, bool IsWithEventsVariable, ULONG depthLevel)
{

    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    AppendString(&tempBuffer, L"<PropertyAccess", depthLevel + 1, NO_ENTITY_REF_CHECK);
    IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Acessor, ATTRIB_VALUE_Set, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    AppendString(&tempBuffer, L"<NameRef", depthLevel + 2, NO_ENTITY_REF_CHECK);

    if (IsWithEventsVariable)
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Field, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }
    else
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Property, NO_ENTITY_REF_CHECK, NO_BRACKETS));
    }

    if (PILNode->AsCallExpression().Left && PILNode->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed)
    {
        BCSYM_NamedRoot *pPropertySymbol = PILNode->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed->PProc()->GetAssociatedPropertyDef();

        if (!pPropertySymbol)
        {
            VSFAIL(L"NULL Property Set symbol!");
            m_fShouldQuote = true;
            return hr;
        }

        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pPropertySymbol->GetName(),
                    NO_ENTITY_REF_CHECK, NeedsBrackets(pPropertySymbol)));

        IfFailGo(GenerateFullName(&tempBuffer, pPropertySymbol, NO_SOURCE_NAME));
    }

    AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsCallExpression().ptreeThis, depthLevel + 3));

    AppendString(&tempBuffer, L"</NameRef>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);


    ILTree::ILNode *pArgList = PILNode->AsCallExpression().Right;
    // This is the list of bounds for the new array
    for (; pArgList ; pArgList = pArgList->AsExpressionWithChildren().Right)
    {
        //If not the last element in the list these must be property arguments
        if (pArgList->AsExpressionWithChildren().Right )
        {
            AppendString(&tempBuffer, L"<Argument>", depthLevel + 2, NO_ENTITY_REF_CHECK);
            IfFailGo(ProcessExpression(&tempBuffer, pArgList->AsExpressionWithChildren().Left, depthLevel + 3));
            AppendString(&tempBuffer, L"</Argument>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
        }
        else
        {
            AppendString(&tempBuffer, L"</PropertyAccess>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
            AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
            //Last argument is actually property set RHS
            IfFailGo(ProcessExpression(&tempBuffer, pArgList->AsExpressionWithChildren().Left, depthLevel));
        }
    }


Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;

}


//****************************************************************************
// Processes a PropertyGet as a nameref. This makes it match C#
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessPropertyGetAccess(StringBuffer *psbBuffer, ILTree::PILNode PILNode, bool IsWithEventsVariable, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    // This code needed to be changed to match C#'s
    if (PILNode->AsCallExpression().Left && PILNode->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed)
    {
        BCSYM_NamedRoot *pPropertySymbol = PILNode->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed->PProc()->GetAssociatedPropertyDef();

        if (!pPropertySymbol)
        {
            VSFAIL(L"NULL Property Get symbol!");
            m_fShouldQuote = true;
            return hr;
        }

        AppendString(&tempBuffer, L"<NameRef", depthLevel, NO_ENTITY_REF_CHECK);
        {
            if (pPropertySymbol->GetKind() == SYM_Property)
            {
                if (IsWithEventsVariable)
                {
                    IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Field, NO_ENTITY_REF_CHECK, NO_BRACKETS));
                }
                else
                {
                    IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Property, NO_ENTITY_REF_CHECK, NO_BRACKETS));
                }
            }
            else
            {
                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_VariableKind, ATTRIB_VALUE_Field, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            }

            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, pPropertySymbol->GetName(), NO_ENTITY_REF_CHECK, NeedsBrackets(pPropertySymbol)));

            IfFailGo(GenerateFullName(&tempBuffer, pPropertySymbol, NO_SOURCE_NAME));
            AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

            if (PILNode->AsCallExpression().ptreeThis)
            {
                IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsCallExpression().ptreeThis, depthLevel + 1));
            }
            else
            {
                AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
                {
                    AppendString(&tempBuffer, L"<Literal>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
                    {
                        IfFailGo(ProcessTypeFromBCSYM(&tempBuffer,
                                                      pPropertySymbol->GetContainer(),
                                                      NULL,
                                                      NULL,
                                                      depthLevel + 3,
                                                      PILNode->AsCallExpression().Left->AsSymbolReferenceExpression().GenericBindingContext));
                    }
                    AppendString(&tempBuffer, L"</Literal>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
                }
                AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
            }
        }
        AppendString(&tempBuffer, L"</NameRef>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process a call statment (constructor calls included here)
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessCallBlock(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    BCSYM_NamedRoot *pnamedProc = NULL;
    bool fIsConstructorCall = false;

    ILTree::CallExpression bilCallTree = PILNode->AsCallExpression();

    if (bilCallTree.Left->bilop == SX_BAD || IsBad(bilCallTree.Left))
    {
        m_fShouldQuote = true;
        return hr;
    }

    if (bilCallTree.Left->bilop == SX_SYM)
    {
        pnamedProc = bilCallTree.Left->AsSymbolReferenceExpression().pnamed;

        // If this is an event call, we don't handle it here.
        if (bilCallTree.uFlags & SXF_CALL_RAISEEVENT)
        {
            IfFailGo(ProcessRaiseEvent(psbBuffer, PILNode, depthLevel));
            goto Error;
        }

        // Set the invoke kind
        if (pnamedProc && pnamedProc->IsProc())
        {
            if (pnamedProc->PProc()->IsAnyConstructor())
            {
                fIsConstructorCall = true;
            }
        }
    }

    if (fIsConstructorCall)
    {
        AppendString(&tempBuffer, L"<ConstructorCall>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }
    else if (pnamedProc && pnamedProc->IsProc() && pnamedProc->PProc()->IsPropertySet())
    {
        IfFailGo(ProcessPropertySetAccess(psbBuffer, PILNode, IsWithEventsVariableAccessor(pnamedProc->PProc()), depthLevel));
        goto Error;
    }
    else if (pnamedProc && pnamedProc->IsProc() && pnamedProc->PProc()->IsPropertyGet())
    {
        IfFailGo(ProcessPropertyGetAccess(psbBuffer, PILNode, IsWithEventsVariableAccessor(pnamedProc->PProc()), depthLevel));
        goto Error;
    }
    else
    {
        AppendString(&tempBuffer, L"<MethodCall>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }
    {
        AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        {
            IfFailGo(ProcessNameRef(&tempBuffer, PILNode, depthLevel + 2));
        }
        AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);

        // We now process the type which that method belongs to
        if (bilCallTree.ptreeThis && bilCallTree.ptreeThis->bilop == SX_SYM && bilCallTree.Left->AsSymbolReferenceExpression().pnamed)
        {
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer,
                                          bilCallTree.Left->AsSymbolReferenceExpression().pnamed->GetContainer(),
                                          NULL,
                                          NULL,
                                          depthLevel + 1,
                                          bilCallTree.Left->AsSymbolReferenceExpression().GenericBindingContext));
        }

        // Now, we generate the args.
        IfFailGo(ProcessMethodArguments(&tempBuffer, bilCallTree.Right, depthLevel + 1));
    }
    if (fIsConstructorCall)
    {
        AppendString(&tempBuffer, L"</ConstructorCall>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }
    else
    {
        AppendString(&tempBuffer, L"</MethodCall>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process an assignment statment
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessAssignment(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<Assignment", depthLevel, NO_ENTITY_REF_CHECK);
    {
        IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Equals, NO_ENTITY_REF_CHECK, NO_BRACKETS));

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // Process left side of assignment
        IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsBinaryExpression().Left, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        // Process right side of assignment
        IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsBinaryExpression().Right, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Assignment>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Process an Expression from a ParseTree::Expression
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessParseExpressionTree(StringBuffer *psbBuffer, ParseTree::Expression *pexpression,
                                           BCSYM *ptype, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if (!pexpression)
    {
        return hr;
    }

    NorlsAllocator nraExpression(NORLSLOC);

    if (ptype && ptype->IsAlias())
    {
        ptype = ptype->DigThroughAlias();
    }

    if (ptype && ptype->IsPointerType())
    {
        ptype = ptype->PPointerType()->GetRoot();
    }

    // Here, we use the semantic analyzer to get us a bil expression tree
    Semantics SemanticAnalyzer(&nraExpression, NULL, m_pCompiler, m_pCompilerProject->GetCompilerHost(), m_pSourceFile, NULL, true);

    ILTree::ILNode *PILNode = SemanticAnalyzer.InterpretExpression(pexpression,
        m_pCurrentContext ? ContextOf(m_pCurrentContext) : ContextOf(m_pCompiler->GetUnnamedNamespace(m_pSourceFile)),
        NULL, ExprNoFlags, ptype);

    IfFailGo(ProcessExpression(psbBuffer, PILNode, depthLevel));

    // If Processing the Expression failed, then go ahead and quote the expression. We shouldn't need
    // to quote more than just the expression since we want to process as much XML as we can.
    if (m_fShouldQuote)
    {
        hr = NOERROR;
        IfFailGo(ProcessQuote(psbBuffer, PILNode, NULL, depthLevel));
        m_fShouldQuote = false;
    }

Error:
    return hr;
}

//****************************************************************************
// Process an Expression from a bilTree
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessExpression(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel, bool fSubExpression)
{
    HRESULT hr = NOERROR;

    if (!PILNode)
    {
        return hr;
    }

    // If there are any errrors, we punt
    if (PILNode->bilop == SX_BAD || IsBad(PILNode))
    {
        m_fShouldQuote = true;
        return hr;
    }

    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    if (fSubExpression == NON_NESTED_EXPRESSION)
    {
        if (PILNode->uFlags & SXF_PAREN_EXPR)
        {
            AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
            AppendString(&tempBuffer, L"<Parentheses>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
        }

        AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }
    {
        // BINOP & UNOP are not differentiated
        switch (PILNode->bilop)
        {
        case SX_ADD :
        case SX_SUB :
        case SX_MUL :
        case SX_DIV :
        case SX_MOD :
        case SX_POW :
        case SX_IDIV :
        case SX_SHIFT_LEFT :
        case SX_SHIFT_RIGHT :
        case SX_ORELSE :
        case SX_ANDALSO :
        case SX_CONC :
        case SX_LIKE :
        case SX_EQ :
        case SX_NE :
        case SX_LE :
        case SX_GE :
        case SX_LT :
        case SX_GT :
        case SX_OR :
        case SX_AND :
        case SX_XOR :
        case SX_IS :
        case SX_ISNOT:
        case SX_ISTYPE :
            IfFailGo(ProcessBinaryOperation(&tempBuffer, PILNode, depthLevel + 1));
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, PILNode->AsBinaryExpression().ResultType, NULL, NULL, depthLevel + 1));
            break;

        case SX_ASG :
        case SX_ASG_RESADR :
            if (PILNode->AsBinaryExpression().Right->bilop == SX_NEW_ARRAY)
            {
                IfFailGo(ProcessNewArray(&tempBuffer, PILNode, depthLevel + 1));
            }
            else
            {
                IfFailGo(ProcessAssignment(&tempBuffer, PILNode, depthLevel + 1));
            }
            break;

        case SX_CTYPE :
        case SX_DIRECTCAST :
        case SX_TRYCAST :
            IfFailGo(ProcessCast(&tempBuffer, PILNode, depthLevel + 1));
            break;

        case SX_WIDE_COERCE:
            IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsBinaryExpression().Left, depthLevel, NESTED_EXPRESSION));
            break;

        case SX_NOT :
        case SX_NEG :
        case SX_PLUS :
            IfFailGo(ProcessUnaryOperation(&tempBuffer, PILNode, depthLevel + 1));
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, PILNode->AsBinaryExpression().ResultType, NULL, NULL, depthLevel + 1));
            break;

        case SX_IIF :
        case SX_IIFCoalesce:
            //
            VSFAIL("xml IIF not implemented");
            break;

        case SX_CTYPEOP :
            //
            VSFAIL("xml CTYPEOP not implemented");
            break;
        case SX_CALL :
            if (PILNode->uFlags & SXF_CALL_WAS_QUERY_OPERATOR)
            {
                m_fShouldQuote = true;
                return hr;
            }

            IfFailGo(ProcessCallBlock(&tempBuffer, PILNode, depthLevel + 1));
            break;

        case SX_CNS_INT :
        case SX_CNS_DEC :
        case SX_CNS_FLT :
        case SX_CNS_STR :
        case SX_NOTHING :
            IfFailGo(GenerateLiteral(&tempBuffer, PILNode, depthLevel + 1));
            break;

        case SX_NAME :
            VSFAIL(L"unexpected SX_NAME");
            break;

        case SX_SYM :
            IfFailGo(ProcessNameRef(&tempBuffer, PILNode, depthLevel + 1));
            break;

        case SX_NEW :
            IfFailGo(ProcessNew(&tempBuffer, PILNode, depthLevel + 1));
            break;

        case SX_NEW_ARRAY :
            IfFailGo(ProcessNewArray(&tempBuffer, PILNode, depthLevel + 1));
            break;

        case SX_CREATE_ARRAY :
            IfFailGo(ProcessCreateArray(&tempBuffer, PILNode->AsExpressionWithChildren().Right, PILNode->AsExpression().ResultType, depthLevel + 1));
            break;

        case SX_ARRAYLITERAL :
            IfFailGo(ProcessArrayLiteral(&tempBuffer, PILNode, depthLevel + 1));
            break;

        case SX_INDEX :
            IfFailGo(ProcessIndexOperator(&tempBuffer, PILNode, depthLevel + 1));
            break;

        case SX_ADR :
            IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsBinaryExpression().Left, depthLevel, NESTED_EXPRESSION));
            break;

        case SX_LATE :
            ProcessLateStmt(&tempBuffer, PILNode, depthLevel + 1);
            break;

        case SX_METATYPE :
            IfFailGo(ProcessGetType(&tempBuffer, PILNode, depthLevel + 1));
            break;

        case SX_SEQ_OP1 :
        case SX_SEQ_OP2 :
            m_fShouldQuote = true;
            return hr;

        case SX_LAMBDA:
        case SX_UNBOUND_LAMBDA:
            m_fShouldQuote = true;
            return hr;

        default :
            VSFAIL(L"Unknown Operator");
            m_fShouldQuote = true;
            return hr;
        }
    }
    if (fSubExpression == NON_NESTED_EXPRESSION)
    {
        AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

        if (PILNode->uFlags & SXF_PAREN_EXPR)
        {
            AppendString(&tempBuffer, L"</Parentheses>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
            AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
        }
    }

    IfTrueRet(m_fShouldQuote, hr);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes array index operator
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessIndexOperator(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<ArrayElementAccess>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        ILTree::IndexExpression bilIndexTree = PILNode->AsIndexExpression();

        // this is the array we are trying to index
        IfFailGo(ProcessExpression(&tempBuffer, bilIndexTree.Left, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        ILTree::PILNode indexList = PILNode->AsIndexExpression().Right;
        IfTrueRet(m_fShouldQuote, hr);

        for (long i = 0; i < bilIndexTree.DimensionCount; ++i)
        {
            // this is the index we are using
            IfFailGo(ProcessExpression(&tempBuffer, (indexList->AsExpressionWithChildren()).Left, depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);

            if (indexList->AsExpressionWithChildren().Right)
            {
                indexList = indexList->AsExpressionWithChildren().Right;
            }
        }
    }
    AppendString(&tempBuffer, L"</ArrayElementAccess>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes a VB binary operator
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessBinaryOperation(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<BinaryOperation", depthLevel, NO_ENTITY_REF_CHECK);
    {
        switch (PILNode->bilop)
        {
        case SX_EQ :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Equals, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_NE :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Notequals, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_LT :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Lessthan, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_LE :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Lessthanorequals, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_GT :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Greaterthan, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_GE :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Greaterthanorequals, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_ADD :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Plus, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_SUB :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Minus, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_MUL :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Times, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_DIV :
        case SX_IDIV :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Divide, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_SHIFT_LEFT:
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Shiftleft, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_SHIFT_RIGHT:
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Shiftright, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_POW :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Power, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_MOD :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Remainder, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_ORELSE :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Logicalor, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_ANDALSO :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Logicaland, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_OR :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Bitor, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_AND :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Bitand, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_XOR :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Bitxor, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_CONC :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Concatenate, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_IS :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Refequals, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_ISNOT:
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Refnotequals, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_ISTYPE :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Istype, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_LIKE :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Binary_Operator, ATTRIBUTE_VALUE_Like, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        default :
            VSFAIL(L"Unknown binary operator");
            m_fShouldQuote = true;
            return hr;
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }

    // Process left-hand operand
    IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsBinaryExpression().Left, depthLevel + 1));
    IfTrueRet(m_fShouldQuote, hr);

    // Process right-hand operand
    IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsBinaryExpression().Right, depthLevel + 1));

    IfTrueRet(m_fShouldQuote, hr);
    AppendString(&tempBuffer, L"</BinaryOperation>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes New delegate
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessNewDelegate(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::NewExpression bilNewDelegateTree = PILNode->AsNewExpression();
    ILTree::DelegateConstructorCallExpression bilDelegateCtor = bilNewDelegateTree.ptreeNew->AsDelegateConstructorCallExpression();

    AppendString(&tempBuffer, L"<NewDelegate", depthLevel, NO_ENTITY_REF_CHECK);
    {
        switch ((bilNewDelegateTree.ptreeNew->AsDelegateConstructorCallExpression()).Method->bilop)
        {
        case SX_NAME :
            VSFAIL(L"unexpected SX_NAME");
            break;

        case SX_SYM :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Name, ((bilNewDelegateTree.ptreeNew->AsDelegateConstructorCallExpression()).Method->AsSymbolReferenceExpression()).pnamed->GetName(),
                     NO_ENTITY_REF_CHECK, NeedsBrackets(((bilNewDelegateTree.ptreeNew->AsDelegateConstructorCallExpression()).Method->AsSymbolReferenceExpression()).pnamed)));
            break;

        default :
            VSFAIL(L"Unknown Name node type");
            m_fShouldQuote = true;
            return hr;
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        // The delegate to create
        BCSYM *pDelegatetype = bilNewDelegateTree.pmod;

        IfFailGo(ProcessBCSYMContainerType(
            &tempBuffer,
            pDelegatetype->PClass(),
            pDelegatetype->IsGenericTypeBinding() ? pDelegatetype->PGenericTypeBinding() : NULL,
            NULL,
            depthLevel + 1));

        IfTrueRet(m_fShouldQuote, hr);

        // Expression that defines the object that contains the method to point to.
        IfFailGo(ProcessExpression(&tempBuffer, bilDelegateCtor.ObjectArgument, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);

        // The type that the method belongs to.
        if ((bilNewDelegateTree.ptreeNew->AsDelegateConstructorCallExpression()).Method->bilop == SX_SYM)
        {
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer,
                                          ((bilNewDelegateTree.ptreeNew->AsDelegateConstructorCallExpression()).Method->AsSymbolReferenceExpression()).pnamed->GetContainer(),
                                          NULL,
                                          NULL,
                                          depthLevel + 1,
                                          (bilNewDelegateTree.ptreeNew->AsDelegateConstructorCallExpression()).Method->AsSymbolReferenceExpression().GenericBindingContext));

            IfTrueRet(m_fShouldQuote, hr);
        }
    }
    AppendString(&tempBuffer, L"</NewDelegate>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes New Class
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessNewClass(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::NewExpression bilNewClassTree = PILNode->AsNewExpression();

    AppendString(&tempBuffer, L"<NewClass>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        BCSYM *pClasstype = bilNewClassTree.pmod;

        IfFailGo(ProcessBCSYMContainerType(
            &tempBuffer,
            pClasstype->PClass(),
            pClasstype->IsGenericTypeBinding() ? pClasstype->PGenericTypeBinding() : NULL,
            NULL,
            depthLevel + 1));

        IfTrueRet(m_fShouldQuote, hr);

        // Now, we generate the args.
        if ((PILNode->AsNewExpression()).ptreeNew)
        {
            IfFailGo(ProcessMethodArguments(&tempBuffer, (PILNode->AsNewExpression()).ptreeNew->AsCallExpression().Right, depthLevel + 1));
            IfTrueRet(m_fShouldQuote, hr);
        }
    }
    AppendString(&tempBuffer, L"</NewClass>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes New Array
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessNewArray(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    ILTree::ExpressionWithChildren bilNewArrayTree = PILNode->AsExpressionWithChildren();

    if (bilNewArrayTree.Left->bilop == SX_BAD || IsBad(bilNewArrayTree.Left))
    {
        m_fShouldQuote = true;
        return hr;
    }

    AppendString(&tempBuffer, L"<NewArray>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        // This is the array type
        IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, bilNewArrayTree.AsExpression().ResultType, NULL, NULL, depthLevel + 1));

        ILTree::ILNode *pboundList = bilNewArrayTree.Left;

        // This is the list of bounds for the new array
        for ( ; pboundList ; pboundList = pboundList->AsExpressionWithChildren().Right)
        {
            AppendString(&tempBuffer, L"<Bound>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
            {
                IfFailGo(ProcessExpression(&tempBuffer, pboundList->AsExpressionWithChildren().Left, depthLevel + 2));
            }
            AppendString(&tempBuffer, L"</Bound>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        }
    }
    AppendString(&tempBuffer, L"</NewArray>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes an array creation and initialization list. This is applies to the
// following VB code:
// Me.Controls.AddRange(New System.Windows.Forms.Control() {Me.button1})
//
// This code will only work if the array is 1-D
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessCreateArray(StringBuffer *psbBuffer, ILTree::PILNode NewArrayRootNode,
                                   BCSYM* ArrayType, ULONG depthLevel)
{
    HRESULT hr = NOERROR;    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    if (NewArrayRootNode->bilop == SX_BAD || IsBad(NewArrayRootNode))
    {
        m_fShouldQuote = true;
        return hr;
    }

    AppendString(&tempBuffer, L"<NewArray>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    {
        ULONG Rank = 0;

        // To find the array type, we first need to find an element node, which can be found by walking the left child (ptreeOp1)
        // all the way until we don't see an SX_LIST anymore. The count of the SX_LIST list gives us the rank of this array.
        ILTree::ILNode *ListNodeForArrayType;
        for (ListNodeForArrayType = NewArrayRootNode; ListNodeForArrayType && ListNodeForArrayType->bilop == SX_LIST ; ListNodeForArrayType = ListNodeForArrayType->AsExpressionWithChildren().Left)
        {
            ++Rank;
        }

        if (ListNodeForArrayType)
        {
            AppendString(&tempBuffer, L"<ArrayType", depthLevel, NO_ENTITY_REF_CHECK);
            {
                WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};
                _ultow_s(Rank, wszIntValue, _countof(wszIntValue), 10);

                IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Rank, wszIntValue, NO_ENTITY_REF_CHECK, NO_BRACKETS));

                // End Attributes
                AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

                // The type of the array
                IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, ListNodeForArrayType->AsExpression().ResultType, NULL, NULL, depthLevel + 1));
            }
            AppendString(&tempBuffer, L"</ArrayType>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
        }
        else
        {
            IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, ArrayType, NULL, NULL, depthLevel + 1));
        }

        AppendString(&tempBuffer, L"<Bound>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        {
            AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
            {
                AppendString(&tempBuffer, L"<Literal>" DEBUG_NEW_LINE, depthLevel + 3, NO_ENTITY_REF_CHECK);
                {
                    AppendString(&tempBuffer, L"<Number>", depthLevel + 4, NO_ENTITY_REF_CHECK);
                    {
                        ULONG Bound = 0;

                        // The number of elements in the array is equal to the length of the ptreeOp2 list of children.
                        for (ILTree::ILNode *ListNode = NewArrayRootNode; ListNode ; ListNode = ListNode->AsExpressionWithChildren().Right)
                        {
                            VSASSERT(ListNode->bilop == SX_LIST, L"Bad Create Array Tree");
                            //Make sure there is actually an element in the array
                            if (ListNode->AsExpressionWithChildren().Left)
                            {
                                ++Bound;
                            }
                        }

                        WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};      // 64-bit int can only take a maximum of 20 chars + sign + '\0'

                        _ultow_s(Bound, wszIntValue, _countof(wszIntValue), 10);
                        AppendString(&tempBuffer, wszIntValue, NO_INDENTATION);
                    }
                    AppendString(&tempBuffer, L"</Number>" DEBUG_NEW_LINE, depthLevel + 4, NO_ENTITY_REF_CHECK);
                }
                AppendString(&tempBuffer, L"</Literal>" DEBUG_NEW_LINE, depthLevel + 3, NO_ENTITY_REF_CHECK);
            }
            AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
        }
        AppendString(&tempBuffer, L"</Bound>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);

        // If the Left child is a list, this means that we have a list of a list (more than 1-D array),
        // so we call ourselves on the left tree
        if (NewArrayRootNode->AsExpressionWithChildren().Left && NewArrayRootNode->AsExpressionWithChildren().Left->bilop == SX_LIST)
        {
            for (ILTree::ILNode *ListNode = NewArrayRootNode ; ListNode ; ListNode = ListNode->AsExpressionWithChildren().Right)
            {
                AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
                {
                    IfFailGo(ProcessCreateArray(&tempBuffer, ListNode->AsExpressionWithChildren().Left,
                                                ArrayType && ArrayType->IsArrayType() ? ArrayType->PArrayType()->GetRoot() : ArrayType,
                                                depthLevel + 2));

                    IfTrueRet(m_fShouldQuote, hr);
                }
                AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
            }
        }
        else
        {
            // A list of initialization expression
            for (ILTree::ILNode *ListNode = NewArrayRootNode ; ListNode ; ListNode = ListNode->AsExpressionWithChildren().Right)
            {
                IfFailGo(ProcessExpression(&tempBuffer, ListNode->AsExpressionWithChildren().Left, depthLevel + 4));
                IfTrueRet(m_fShouldQuote, hr);
            }
        }
    }
    AppendString(&tempBuffer, L"</NewArray>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes an array literal. This is applied to the following VB code:
// Me.Controls.AddRange({ Me.button1 })
//
// Return: Success or catastrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessArrayLiteral(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    ILTree::ArrayLiteralExpression & ArrayLiteralNode = PILNode->AsArrayLiteralExpression();
    if (!ArrayLiteralNode.ResultType || !ArrayLiteralNode.ResultType->IsArrayType())
    {
        VSFAIL(L"BCSYM must be a BCSYM_ArrayType inside ProcessArrayLiteral()");
        m_fShouldQuote = true;
        return NOERROR;
    }

    return ProcessArrayLiteral(psbBuffer, ArrayLiteralNode, 0, ArrayLiteralNode.ElementList, depthLevel);
}

//****************************************************************************
// Processes a single rank of an array literal.
//
// Return: Success or catastrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessArrayLiteral(StringBuffer *psbBuffer, ILTree::ArrayLiteralExpression & ArrayLiteralNode,
                                    unsigned currentRank, ILTree::ExpressionWithChildren * Elements, ULONG depthLevel)
{
    HRESULT hr = NOERROR;    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    {
        unsigned rankFromEnd = ArrayLiteralNode.Rank - currentRank;
        // Criteria for needsArrayType were determined via trial-and-error to match backwards compatibility
        bool needsArrayType = (ArrayLiteralNode.Dims[currentRank] > 0 || ArrayLiteralNode.Rank == 1);

        AppendString(&tempBuffer, L"<NewArray>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
        if (needsArrayType)
        {
            AppendString(&tempBuffer, L"<ArrayType", depthLevel, NO_ENTITY_REF_CHECK);

            WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};
            _ultow_s(rankFromEnd, wszIntValue, _countof(wszIntValue), 10);
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Rank, wszIntValue, NO_ENTITY_REF_CHECK, NO_BRACKETS));

            // End Attributes
            AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
        }

        // The type of the array element
        IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, ArrayLiteralNode.ResultType->PArrayType()->GetRoot(), NULL, NULL, depthLevel + 1));

        if (needsArrayType)
        {
            AppendString(&tempBuffer, L"</ArrayType>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
        }

        AppendString(&tempBuffer, L"<Bound>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
        {
            AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
            {
                AppendString(&tempBuffer, L"<Literal>" DEBUG_NEW_LINE, depthLevel + 3, NO_ENTITY_REF_CHECK);
                {
                    AppendString(&tempBuffer, L"<Number>", depthLevel + 4, NO_ENTITY_REF_CHECK);
                    {
                        WCHAR wszIntValue[MaxStringLengthForIntToStringConversion] = {0};      // 64-bit int can only take a maximum of 20 chars + sign + '\0'

                        _ultow_s(ArrayLiteralNode.Dims[currentRank], wszIntValue, _countof(wszIntValue), 10);
                        AppendString(&tempBuffer, wszIntValue, NO_INDENTATION);
                    }
                    AppendString(&tempBuffer, L"</Number>" DEBUG_NEW_LINE, depthLevel + 4, NO_ENTITY_REF_CHECK);
                }
                AppendString(&tempBuffer, L"</Literal>" DEBUG_NEW_LINE, depthLevel + 3, NO_ENTITY_REF_CHECK);
            }
            AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 2, NO_ENTITY_REF_CHECK);
        }
        AppendString(&tempBuffer, L"</Bound>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);

#if DEBUG
        unsigned Index = 0;
#endif DEBUG

        for (ILTree::ILNode *ListNode = Elements; ListNode; ListNode = ListNode->AsExpressionWithChildren().Right)
        {
            ILTree::ILNode *DataElement = ListNode->AsExpressionWithChildren().Left;
            if (rankFromEnd > 1)
            {
                VSASSERT(DataElement->bilop == SX_NESTEDARRAYLITERAL, "Nested array literal has unexpected structure");

                AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
                {
                    IfFailGo(ProcessArrayLiteral(&tempBuffer, ArrayLiteralNode, currentRank + 1,
                                                 DataElement->AsNestedArrayLiteralExpression().ElementList,
                                                 depthLevel + 2));
                    IfTrueRet(m_fShouldQuote, hr);
                }
                AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
            }
            else
            {
                IfFailGo(ProcessExpression(&tempBuffer, DataElement, depthLevel + 4));
                IfTrueRet(m_fShouldQuote, hr);
            }
#if DEBUG
            Index++;
#endif DEBUG
        }

        VSASSERT(Index == ArrayLiteralNode.Dims[currentRank], "Array shape inconsistency detected!!!");

        AppendString(&tempBuffer, L"</NewArray>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes New operator
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessNew(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    // Let's find out what type of new it is
    BCSYM_Class *pType = PILNode->AsNewExpression().pmod->PClass();

    if (!pType)
    {
        return DebAssertHelper(L"No BCSYM_Type node was found for a new operator");
    }

    if (pType->IsClass())
    {
        if (pType->PClass()->IsDelegate())
        {
            IfFailGo(ProcessNewDelegate(psbBuffer, PILNode, depthLevel));
        }
        else
        {
            IfFailGo(ProcessNewClass(psbBuffer, PILNode, depthLevel));
        }
    }

Error:
    return hr;
}

//****************************************************************************
// Not sure what this is exactly yet!
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessCoerceArg(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;

    if ((PILNode->AsBinaryExpression().Left)->bilop == SX_SYM)
    {
        IfFailGo(ProcessExpression(psbBuffer, PILNode->AsBinaryExpression().Right, depthLevel));
        IfTrueRet(m_fShouldQuote, hr);
    }
    else
    {
        IfFailGo(ProcessExpression(psbBuffer, PILNode->AsBinaryExpression().Left->AsBinaryExpression().Right, depthLevel));
        IfTrueRet(m_fShouldQuote, hr);
    }

Error:
    return hr;
}

//****************************************************************************
// Processes a cast operator
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessCast(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    //{
    //    bool RequiresNarrowingConversion = false;
    //    bool NarrowingFromNumericLiteral = false;
    //    bool SuppressMethodNameInErrorMessages= false;
    //    DelegateRelaxationLevel DelegateRelaxationLevel = DelegateRelaxationLevelNone;
    //    bool RequiresUnwrappingNullable = false;
    //Semantics SemanticAnalyzer(&nraExpression, NULL, m_pCompiler, m_pCompilerProject->GetCompilerHost(), NULL, m_pSourceFile, NULL, true);

    //    Expression ResultExpression = SemanticAnalyzer.ConvertWithErrorChecking(
    //        InputExpression,
    //        TargeType,
    //        ExprNoFlags,
    //        NULL, //CopyBackConversionParam
    //        RequiresNarrowingConversion,
    //        NarrowingFromNumericLiteral,
    //        false, // SuppressMethodNameInErrorMessages
    //        DelegateRelaxationLevel,
    //        RequiresUnwrappingNullable);

    //}

    Type* TargeType = PILNode->AsExpression().ResultType;
    ILTree::PILNode InputExpression = PILNode->AsBinaryExpression().Left;


    AppendString(&tempBuffer, L"<Cast", depthLevel, NO_ENTITY_REF_CHECK);
    {
        if (!(PILNode->uFlags & SXF_COERCE_EXPLICIT))
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Implicit, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // Directcast needs to be marked so that if we go back to VB code, we can generate the right cast or directcast
        if (PILNode->bilop == SX_DIRECTCAST)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_DirectCast, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        if (PILNode->bilop == SX_TRYCAST)
        {
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_TryCast, ATTRIB_VALUE_Yes, NO_ENTITY_REF_CHECK, NO_BRACKETS));
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);

        IfFailGo(ProcessTypeFromBCSYM(&tempBuffer, TargeType, NULL, NULL, depthLevel + 1));
        IfFailGo(ProcessExpression(&tempBuffer, InputExpression, depthLevel + 1));
        IfTrueRet(m_fShouldQuote, hr);
    }
    AppendString(&tempBuffer, L"</Cast>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes a VB unary operator
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessUnaryOperation(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    AppendString(&tempBuffer, L"<UnaryOperation", depthLevel, NO_ENTITY_REF_CHECK);
    {
        switch (PILNode->bilop)
        {
        case SX_PLUS :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Unary_Operator, ATTRIBUTE_VALUE_Plus, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_NEG :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Unary_Operator, ATTRIBUTE_VALUE_Minus, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        case SX_NOT :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Unary_Operator, ATTRIBUTE_VALUE_Not, NO_ENTITY_REF_CHECK, NO_BRACKETS));
            break;

        // 

        /* case SX_BNOT :
            IfFailGo(GenerateGenericAttribute(&tempBuffer, ATTRIB_VALUE_Unary_Operator, ATTRIBUTE_VALUE_Bitnot, NO_ENTITY_REF_
*/

        default :
            VSFAIL(L"Unknown unary operator");
            m_fShouldQuote = true;
            return hr;
        }

        // End Attributes
        AppendString(&tempBuffer, L">" DEBUG_NEW_LINE, NO_INDENTATION, NO_ENTITY_REF_CHECK);
    }

    // Process one and only operand
    IfFailGo(ProcessExpression(&tempBuffer, PILNode->AsBinaryExpression().Left, depthLevel + 1));
    IfTrueRet(m_fShouldQuote, hr);

    AppendString(&tempBuffer, L"</UnaryOperation>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Processes args to procs
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::ProcessMethodArguments(StringBuffer *psbBuffer, ILTree::PILNode PILNode, ULONG depthLevel)
{
    HRESULT hr = NOERROR;
    // Transaction buffer that can rollback when we see errors.
    AppendStringBuffer tempBuffer(psbBuffer);

    if (!PILNode)
    {
        return hr;
    }

    ILTree::ExpressionWithChildren *pArgsList = &(PILNode->AsExpressionWithChildren());

    for ( ; pArgsList ; pArgsList = &(pArgsList->Right->AsExpressionWithChildren()))
    {
        AppendString(&tempBuffer, L"<Argument>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);
        {
            if (pArgsList->Left)
            {
                switch (pArgsList->Left->bilop)
                {
                case SX_ADR :
                    // We have a name for the argument, we just dump that
                    IfFailGo(ProcessExpression(&tempBuffer, pArgsList->Left->AsBinaryExpression().Left, depthLevel + 1));
                    IfTrueRet(m_fShouldQuote, hr);
                    break;

                case SX_ASG_RESADR :
                    // We have an expression for the argument, we dump that
                    IfFailGo(ProcessCoerceArg(&tempBuffer, pArgsList->Left, depthLevel + 1));
                    IfTrueRet(m_fShouldQuote, hr);
                    break;

                case SX_ARG :
                    IfFailGo(ProcessExpression(&tempBuffer, pArgsList->Left->AsArgumentExpression().Left, depthLevel + 1));
                    IfTrueRet(m_fShouldQuote, hr);
                    break;

                case SX_CALL :
                    AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
                    {
                        IfFailGo(ProcessCallBlock(&tempBuffer, pArgsList->Left, depthLevel + 2));
                    }
                    AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);

                    IfTrueRet(m_fShouldQuote, hr);
                    break;

                case SX_SYM :
                    AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
                    {
                        IfFailGo(ProcessNameRef(&tempBuffer, pArgsList->Left, depthLevel + 2));
                    }
                    AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);

                    IfTrueRet(m_fShouldQuote, hr);
                    break;

                case SX_CREATE_ARRAY :
                    AppendString(&tempBuffer, L"<Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);
                    {
                        IfFailGo(ProcessCreateArray(&tempBuffer, pArgsList->Left->AsExpressionWithChildren().Right, pArgsList->Left->AsExpression().ResultType, depthLevel + 2));
                    }
                    AppendString(&tempBuffer, L"</Expression>" DEBUG_NEW_LINE, depthLevel + 1, NO_ENTITY_REF_CHECK);

                    IfTrueRet(m_fShouldQuote, hr);
                    break;

                    // Anything else we process as an expression
                default :
                    IfFailGo(ProcessExpression(&tempBuffer, pArgsList->Left, depthLevel + 1));
                    IfTrueRet(m_fShouldQuote, hr);
                    break;
                }
            }
        }
        AppendString(&tempBuffer, L"</Argument>" DEBUG_NEW_LINE, depthLevel, NO_ENTITY_REF_CHECK);

        if (!pArgsList->Right)
        {
            break;
        }
    }

Error:
    if (hr == NOERROR)
    {
        tempBuffer.Commit();
    }

    return hr;
}

//****************************************************************************
// Gets the text of the file
//
// Return: Success or catstrophic failure code.
//****************************************************************************
HRESULT XMLGen::GetTextFile(Text **ppTextFile)
{
    HRESULT hr = NOERROR;

    // since the Text::init function is pretty expensive, we only want to do it
    // at most once and only when we need to do it.
    if (!m_pTextFile)
    {
        m_pTextFile = new Text();
        IfFailGo(m_pTextFile->Init(m_pSourceFile));
    }

    *ppTextFile = m_pTextFile;
Error:
    return(hr);
}

//****************************************************************************
// Determines if the specified proc symbol is associated with WithEvents
// variable declaration
// Return: True, if the method is accessor if WithEvents variable
//****************************************************************************
bool XMLGen::IsWithEventsVariableAccessor(_In_ BCSYM_Proc* pnamedProc)
{
    ThrowIfNull(pnamedProc);

    if (pnamedProc->IsPropertyGet() ||pnamedProc->IsPropertySet())
    {
        return pnamedProc->GetAssociatedPropertyDef() && pnamedProc->GetAssociatedPropertyDef()->CreatedByWithEventsDecl();
    }

    return false;
}
