//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//-------------------------------------------------------------------------------------------------
//
// After changing logic in XmlSymbols, make sure to verify the behavior of Xml features 
// in intellisense, in the immediate window, and in the compiler.  
// Also check behavior of DevDiv bugs 112707 and 112893.
//
void XmlSymbols::SetContext(
    CompilerProject *Project, 
    CompilerHost *CompilerHost)
{
    Initialize(Project, NULL, Project ? Project->GetCompiler() : NULL, CompilerHost);
}

void XmlSymbols::SetContext(
    Scope *Scope, 
    Compiler *Compiler, 
    CompilerHost *CompilerHost)
{
    Initialize(NULL, Scope, Compiler, CompilerHost);
}

void XmlSymbols::Initialize(
    CompilerProject *Project, 
    Scope *Scope, 
    Compiler *Compiler, 
    CompilerHost *CompilerHost)
{
    if (!Project && Scope)
    {
        Project = Scope->GetContainingProject();
    }

    if (Project && Compiler)
    {
        // Names must be looked up starting at the topmost scope, unless its the debugger scope, which uses a different mechanism
        BCSYM_Namespace *Namespace = Compiler->GetUnnamedNamespace(Project);
        if (Namespace)
        {
            Scope = Namespace->GetHash();
        }
    }

    if (Project && !CompilerHost)
    {
        CompilerHost = Project->GetCompilerHost();
    }

#if IDE
    // Dev10#815761: The project may remain the same, but we still have to reset our data
    // if it was decompiled since in that case the data we cached is trashed.
    if (Project)
    {
        ULONG projectBoundChangeIndex = Project->GetCurrentBoundChangeIndex();
        if (m_Project && m_projectBoundChangeIndex != projectBoundChangeIndex)
        {
            m_Project = NULL; // so that we go into the if below
        }
        m_projectBoundChangeIndex = projectBoundChangeIndex;
    }
#endif

    // Reuse any cached symbols if objects are identical
    if (m_Project != Project || m_Scope != Scope || m_Compiler != Compiler || m_CompilerHost != CompilerHost)
    {
        m_Project = Project;
        m_Scope = Scope;
        m_Compiler = Compiler;
        m_CompilerHost = CompilerHost;
        m_XContainer = NULL;
        m_XDocument = NULL;
        m_XElement = NULL;
        m_XAttribute = NULL;
        m_XNamespace = NULL;
        m_XProcessingInstruction = NULL;
        m_XComment = NULL;
        m_XCData = NULL;
        m_XNamespaceArray = NULL;
        m_XAttributeList = NULL;
        m_XmlExtensions = NULL;
        m_XmlHelper = NULL;
    }

    if (m_CompilerHost && !m_SymbolProvider)
    {
        m_SymbolProvider = m_CompilerHost->GetFXSymbolProvider();
    }
}

ClassOrRecordType * XmlSymbols::GetXmlHelper()
{
    if (!m_XmlHelper && m_Scope && m_Compiler && m_CompilerHost)
    {
        Type *ResultType = NULL;
        STRING *DefaultNamespace = NULL;

        // Qualify My.InternalXmlHelper with the default namespace, if one exists
        if (m_Project && m_Project->GetDefaultNamespace())
        {
            DefaultNamespace = m_Project->GetDefaultNamespace();
        }

        if (DefaultNamespace && StringPool::StringLength(DefaultNamespace) != 0)
        {
            StringBuffer ConcatBuffer;

            // Prepend the default namespace to the helper class name
            ConcatBuffer.AppendSTRING(DefaultNamespace);
            ConcatBuffer.AppendChar(L'.');
            ConcatBuffer.AppendSTRING(STRING_CONST(m_Compiler, ComMyXmlHelper));

            ResultType = GetType(m_Compiler->AddString(ConcatBuffer.GetString()));
        }
        else
        {
            ResultType = GetType(STRING_CONST(m_Compiler, ComMyXmlHelper));
        }

        if (ResultType && ResultType->IsClass())
        {
            m_XmlHelper = ResultType->PClass();
        }
    }
    return m_XmlHelper;
}

ArrayType *
XmlSymbols::EnsureLinqArrayClass(
    ArrayType *&ClassStorage,
    Symbols *symbolCreator, 
    ClassOrRecordType *type)
{
    if (!ClassStorage)
    {
        ClassStorage = symbolCreator->GetArrayType(1, type);
    }
    return ClassStorage;
}

BCSYM_GenericBinding *
XmlSymbols::EnsureLinqListClass(
    BCSYM_GenericBinding *&ClassStorage,
    Symbols *symbolCreator, 
    ClassOrRecordType *type)
{
    if (!ClassStorage)
    {
        BCSYM * typeArgs[] = {GetXAttribute()};
        ClassStorage = symbolCreator->GetGenericBinding(
            false, // IsBad
            m_SymbolProvider->GetType(FX::GenericListType),
            typeArgs, // Arguments
            1, // ArgumentCount
            NULL, // ParentBinding
            true // AllocAndCopyArgumentsToNewList
            );
    }
    return ClassStorage;
}

ClassOrRecordType *
XmlSymbols::EnsureLinqClass
(
    ClassOrRecordType *&ClassStorage,
    STRING_CONSTANTS ClassNameConst
)
{
    if (!ClassStorage && m_Scope && m_Compiler && m_CompilerHost)
    {
        StringBuffer ConcatBuffer;

        // Add the class name we're searching for after System.Xml.Linq
        ConcatBuffer.AppendSTRING(STRING_CONST(m_Compiler, SystemXmlLinq));
        ConcatBuffer.AppendChar(L'.');
        ConcatBuffer.AppendSTRING(m_Compiler->GetStringConstant(ClassNameConst));

        Type *ResultType = GetType(ConcatBuffer.GetString());

        // Check that the symbol exists and that it is good.  If System.Xml is missing
        // the XLinq class may be there but it is marked as bad. Return the bad symbol
        // as returning null might crash the compiler.
        if (ResultType && ResultType->IsClass())
        {
            ClassStorage = ResultType->PClass();
        }
    }
    return ClassStorage;
}

Procedure *
XmlSymbols::EnsureXmlHelperProc
(
    Procedure *&ProcStorage,
    STRING_CONSTANTS ProcNameConst
)
{
    if (!ProcStorage)
    {
        ClassOrRecordType * XmlHelper = GetXmlHelper();
        if (XmlHelper)
        {
            Type * ResultType = XmlHelper->GetHash()->SimpleBind(m_Compiler->GetStringConstant(ProcNameConst));
            if (ResultType && ResultType->IsProc())
            {
                ProcStorage = ResultType->PProc();
            }
        }
    }
    return ProcStorage;
}

Type *
XmlSymbols::GetType
(
    _In_z_ STRING *FullyQualifiedName           // This function only handles fully-qualified global names
)
{
    BCSYM * ResultType = NULL;
    VSASSERT(m_Scope && m_Compiler && m_CompilerHost, "Check these before calling GetType");


    // Otherwise, break up the string and find the symbol in the symbol table
    NorlsAllocator NameStorage(NORLSLOC);
    STRING **ParsedNames = NULL;
    ULONG NumberOfNames = 0;
    Location TextSpan;
    bool NameIsBad;

    IfFailThrow(BreakUpQualifiedName(m_Compiler, &ParsedNames, &NameStorage, &NumberOfNames, FullyQualifiedName, DOT_DELIMITER));

    CompilationCaches *pCompilationCaches = GetCompilerCompilationCaches();


    TextSpan.SetLocationToHidden();
    return Semantics::InterpretQualifiedName(
                ParsedNames,
                NumberOfNames,
                NULL,
                NULL,
                m_Scope,
                NameSearchTypeReferenceOnly | NameSearchIgnoreExtensionMethods | NameSearchGlobalName | NameSearchDoNotMergeNamespaceHashes,
                TextSpan,
                NULL,
                m_Compiler,
                m_CompilerHost,
                pCompilationCaches,
                NULL,
                false,
                NameIsBad);
}

XmlSemantics::XmlSemantics(XmlSemantics *Parent, Semantics *Analyzer, ExpressionFlags Flags ) :
    m_Parent(Parent),
    m_Analyzer(Analyzer),
    m_AttributeMgr(Analyzer->m_Compiler),
    m_XElementLocals(Analyzer),
    m_XDocumentLocals(Analyzer),
    m_ImportedNamespaceAttributes(NULL),
    m_Flags(Flags & ~ExprDontInferResultType), // Dev10#501659: for fragments embedded in the XML, we need to bind any lambdas they contain
    m_Prefixes(NULL),
    m_EmptyPrefixAlias(NULL),
    m_RootNamespaceAttributes(NULL),
    m_InheritImportedNamespaces(false)
{
    m_XmlNsPrefix.Name = STRING_CONST(Analyzer->m_Compiler, XmlNs);
    m_XmlNsPrefix.TypeCharacter = chType_NONE;
    m_XmlNsPrefix.IsBracketed = false;
    m_XmlNsPrefix.IsNullable = false;
    m_XmlNsPrefix.IsBad = false;

    m_EmptyNamespace = new(Analyzer->m_TreeStorage) ParseTree::StringLiteralExpression;
    m_EmptyNamespace->Opcode = ParseTree::Expression::StringLiteral;
    m_EmptyNamespace->Value = L"";
    m_EmptyNamespace->LengthInCharacters = 0;

    if (Parent && Parent->m_InheritImportedNamespaces)
    {
        m_RootNamespaceAttributes = Parent->m_RootNamespaceAttributes;
    }

}

XmlSemantics::~XmlSemantics()
{
    if (m_Parent && m_Parent->m_InheritImportedNamespaces)
    {
        VSASSERT(!m_Parent->m_RootNamespaceAttributes ||
            m_Parent->m_RootNamespaceAttributes == m_RootNamespaceAttributes,  
            "Something is wrong with the root namespace attributes");

        if (m_RootNamespaceAttributes)
        {
            m_Parent->m_RootNamespaceAttributes = m_RootNamespaceAttributes;
        }
    }
}

/*=============================================================================
FindImportedNamespaceAttribute

Searches the imported namespace attribute list for an attribute which defines
the prefix.

=============================================================================*/

ParseTree::XmlAttributeExpression *
XmlSemantics::FindImportedNamespaceAttribute(_In_z_ STRING *Prefix)
{
   for (ParseTree::ExpressionList *NamespaceAttributes = m_ImportedNamespaceAttributes;
        NamespaceAttributes != NULL;
        NamespaceAttributes = NamespaceAttributes->Next)
    {

        ParseTree::XmlAttributeExpression *NamespaceAttribute = NamespaceAttributes->Element->AsXmlAttribute();
        ParseTree::XmlNameExpression *AttributeName = NamespaceAttribute->Name->AsXmlName();

        if (StringPool::IsEqualCaseSensitive(Prefix, AttributeName->LocalName.Name) ||
            (StringPool::StringLength(Prefix) == 0 && StringPool::IsEqualCaseSensitive(AttributeName->LocalName.Name, STRING_CONST(m_Analyzer->m_Compiler, XmlNs))))
        {
            return NamespaceAttribute;
        }
    }

   return NULL;
}

/*=============================================================================
CreateImportedNamespaceAttribute

Create a namespace attribute parse tree expression that defines the prefix. One
of these attributes is created whenever an imported namespace prefix is used in
an Xml literal.

=============================================================================*/

ParseTree::XmlAttributeExpression *
XmlSemantics::CreateImportedNamespaceAttribute(_In_z_ STRING *Prefix, _In_z_ STRING *NamespaceURI)
{
    ParserHelper PH(&m_Analyzer->m_TreeStorage);

    ParseTree::Expression *NamespaceExpr = PH.CreateStringConst(NamespaceURI);

    // Create a namespace declaration for this prefix so it can be added to the literal
    ParseTree::XmlNameExpression *NamespaceName = new(m_Analyzer->m_TreeStorage) ParseTree::XmlNameExpression;
    NamespaceName->Opcode = ParseTree::Expression::XmlName;
    NamespaceName->Prefix = StringPool::StringLength(Prefix) == 0 ? PH.ParserHelper::CreateIdentifierDescriptor(Prefix): m_XmlNsPrefix;
    NamespaceName->LocalName = StringPool::StringLength(Prefix) == 0 ? m_XmlNsPrefix : PH.ParserHelper::CreateIdentifierDescriptor(Prefix);
    NamespaceName->IsElementName = false;

    // Create the namespace attribute
    ParseTree::XmlAttributeExpression *Attribute = new(m_Analyzer->m_TreeStorage) ParseTree::XmlAttributeExpression;
    Attribute->Opcode = ParseTree::Expression::XmlAttribute;
    Attribute->Delimiter = tkQuote;
    Attribute->Name = NamespaceName;
    Attribute->Value = NamespaceExpr;
    Attribute->PrecededByWhiteSpace = true;
    Attribute->IsImportedNamespace = true;

    // Add it to the current attribute list
    AddImportedNamespaceAttribute(Attribute);

    return Attribute;
}

void
XmlSemantics::AddImportedNamespaceAttribute(ParseTree::XmlAttributeExpression *Attribute)
{
    // Add it to the current attribute list
    ParseTree::ExpressionList *NamespaceAttributes = new (m_Analyzer->m_TreeStorage) ParseTree::ExpressionList;
    NamespaceAttributes->Element = Attribute;
    NamespaceAttributes->Next =m_ImportedNamespaceAttributes;
    m_ImportedNamespaceAttributes = NamespaceAttributes;
}

/*=============================================================================
EnsureImportedNamespaceAttribute

Search the imported namespace attribute list and create a namespace attribute
if one doesn't already exist.  If this xml literal is contained within an
embedded expression and static analysis has determined that the literal is added
to the outer literal, it also checks whether the namespace attribute has been
defined there.
=============================================================================*/

bool
XmlSemantics::EnsureImportedNamespaceAttribute(BCSYM_Alias *PrefixAlias)
{
    if (PrefixAlias)
    {
        STRING *PrefixString = PrefixAlias->GetName();
        BCSYM_XmlNamespaceDeclaration *XmlNamespaceDeclaration = PrefixAlias->PAlias()->GetSymbol()->PXmlNamespaceDeclaration();
        STRING *NamespaceString = XmlNamespaceDeclaration->GetName();

        // Don't create an imported namespace declaration for the empty namespace.
        // i.e. xmlns=""

        if (StringPool::StringLength(NamespaceString) != 0)
        {

            // Add xmlns attributes for imported namespace but
            // don't create definitions for xml and xmlns prefixes

            if (XmlNamespaceDeclaration->IsImported() &&
                !StringPool::IsEqualCaseSensitive(PrefixString, STRING_CONST(m_Analyzer->m_Compiler, XmlNs)) &&
                !StringPool::IsEqualCaseSensitive(PrefixString, STRING_CONST(m_Analyzer->m_Compiler, Xml))
                )
            {

                // Ensure that a namespace attribute is created for this imported namespace definition

                return EnsureImportedNamespaceAttribute(PrefixString, NamespaceString);
            }
        }
    }

    return false;
}

/*=============================================================================
EnsureImportedNamespaceAttribute

Search the imported namespace attribute list and create a namespace attribute
if one doesn't already exist.  If this xml literal is contained within an
embedded expression and static analysis has determined that the literal is added
to the outer literal, it also checks whether the namespace attribute has been
defined there.
=============================================================================*/

bool
XmlSemantics::EnsureImportedNamespaceAttribute(_In_z_ STRING *PrefixString, _In_z_ STRING *NamespaceString)
{
    Declaration *PrefixAlias = Semantics::InterpretXmlPrefix(m_Prefixes, PrefixString, NameSearchDonotResolveImportsAlias, m_Analyzer->m_SourceFile);

    if (PrefixAlias)
    {
        BCSYM_XmlNamespaceDeclaration *pXmlNamespaceDeclaration = PrefixAlias->PAlias()->GetSymbol()->PXmlNamespaceDeclaration();

        if (!pXmlNamespaceDeclaration->IsImported())
        {
            // Imported namespace is declared explicitly in the xml literal

            return StringPool::IsEqualCaseSensitive(NamespaceString, pXmlNamespaceDeclaration->GetName());
        }
        else if (FindImportedNamespaceAttribute(PrefixString))
        {
            return true;
        }
    }

    if (m_Parent && m_Parent->m_InheritImportedNamespaces)
    {
        if (m_Parent->EnsureImportedNamespaceAttribute(PrefixString, NamespaceString))
        {
            return true;
        }
    }

    CreateImportedNamespaceAttribute(PrefixString, NamespaceString);

    return true;
}

/*=============================================================================
AreImportedNamespacesDefined

Returns true if  imported namespace are defined in the module.  Do not include
the built-in namespaces for xml, xmlns and blank.
=============================================================================*/

bool
XmlSemantics::AreImportedNamespacesDefined()
{
    // Now look for imported xml prefixes
    XmlNamespaceImportsIterator Iterator(m_Analyzer->m_SourceFile);

    while (Iterator.MoveNext())
    {
        BCSYM_Alias *PrefixAlias = Iterator.Current()->m_pAlias;
        if (StringPool::StringLength(PrefixAlias->PAlias()->GetSymbol()->PXmlNamespaceDeclaration()->GetName()) != 0 &&
            !StringPool::IsEqualCaseSensitive(PrefixAlias->GetName(), STRING_CONST(m_Analyzer->m_Compiler, Xml)) &&
            !StringPool::IsEqualCaseSensitive(PrefixAlias->GetName(), STRING_CONST(m_Analyzer->m_Compiler, XmlNs)))
            return true;
    }

    return false;
}

/*=============================================================================
IsImportedNamespaceUsed

Returns true if the particular imported namespace has been used in the current
scope.  Note, the prefix alias must reference and imported namespace declaration
=============================================================================*/
bool
XmlSemantics::IsImportedNamespaceUsed(BCSYM_Alias *PrefixAlias)
{
    BCSYM_XmlNamespaceDeclaration *XmlNamespaceDeclaration = PrefixAlias->PAlias()->GetSymbol()->PXmlNamespaceDeclaration();

    VSASSERT(XmlNamespaceDeclaration->IsImported(), "Prefix must reference an imported namespace");

    return IsImportedNamespaceUsed(PrefixAlias->GetName(), XmlNamespaceDeclaration->GetName());
}

/*=============================================================================
IsImportedNamespaceUsed

Returns true if the particular imported namespace has been used in the current
scope.  Note, the prefix alias must reference an imported namespace declaration
Recursively check the outer xml literal if this xml literal is inside and embedded
expression and static analysis shows that it is contained within the outer literal.
=============================================================================*/
bool
XmlSemantics::IsImportedNamespaceUsed(_In_z_ STRING *PrefixString, _In_opt_z_ STRING *NamespaceString)
{
    Declaration *PrefixAlias = Semantics::InterpretXmlPrefix(m_Prefixes, PrefixString, NameSearchDonotResolveImportsAlias, m_Analyzer->m_SourceFile);

    if (PrefixAlias)
    {
        BCSYM_XmlNamespaceDeclaration *pXmlNamespaceDeclaration = PrefixAlias->PAlias()->GetSymbol()->PXmlNamespaceDeclaration();

        if (!pXmlNamespaceDeclaration->IsImported())
        {
            // Imported namespace is declared explicitly in the xml literal

            return StringPool::IsEqualCaseSensitive(NamespaceString, pXmlNamespaceDeclaration->GetName());
        }
        else if (FindImportedNamespaceAttribute(PrefixString))
        {
            // Imported namespace is already used

            return true;
        }
    }

    if (m_Parent && m_Parent->m_InheritImportedNamespaces)
    {
         return m_Parent->IsImportedNamespaceUsed(PrefixString, NamespaceString);
    }

    return false;
}

ILTree::Expression *
XmlSemantics::CreateConstructedInstance(Type *ElementType, const Location &loc, ParseTree::ArgumentList *ArgList)
{
    ILTree::Expression *Result;

    //The constructor
    if (!ElementType)
    {
        Result = m_Analyzer->AllocateBadExpression(loc);
    }
    else
    {
        Result = m_Analyzer->CreateConstructedInstance(ElementType,
            loc,
            loc,
            ArgList,
            m_Flags);
    }

    return Result;
}

/*=============================================================================
TraceLinqExpression

Walks expression and tries to unwrap it to find the expression that is returned

( Expr ) returns   Expr
Function (...) Expr returns Expr
From ... Where ... Select Expr returns Expr

This function is used for static analysis of embedded expressions to determine
if the embedded expression returns xml.
=============================================================================*/

ParseTree::Expression *
XmlSemantics::TraceLinqExpression(ParseTree::Expression * pExpr)
{
    ParseTree::Expression * pNextExpr = pExpr;

    // Trace through linq expressions that filter, sort, or group, as these operations do not change the
    // "Element" type of the expression (i.e. if input is IEnumerable(Of XElement), output might be
    // IEnumerable(Of IEnumerable(Of XElement)) or XElement, but not IEnumerable(Of Integer)).

    // Non-recursively trace the expression, peeling away uninteresting wrapper layers
    while (pNextExpr)
    {
        pExpr = pNextExpr;
        pNextExpr = NULL;

        switch (pExpr->Opcode)
        {
        case ParseTree::Expression::CallOrIndex:
            pNextExpr = pExpr->AsCallOrIndex()->Target;
            break;

        case ParseTree::Expression::Lambda:
            if (!pExpr->AsLambda()->IsStatementLambda)
            {
                pNextExpr = pExpr->AsLambda()->GetSingleLineLambdaExpression();
            }
            break;

        case ParseTree::Expression::Parenthesized:
            pNextExpr = pExpr->AsParenthesized()->Operand;
            break;

        case ParseTree::Expression::From:
        case ParseTree::Expression::Let:
            break;

        case ParseTree::Expression::Where:
        case ParseTree::Expression::Distinct:
        case ParseTree::Expression::OrderBy:
        case ParseTree::Expression::TakeWhile:
        case ParseTree::Expression::SkipWhile:
        case ParseTree::Expression::Take:
        case ParseTree::Expression::Skip:
            // Bind to source expression
            pNextExpr = pExpr->AsLinqOperator()->Source;
            break;

        case ParseTree::Expression::Select:
            // Bind to single item projection
            pNextExpr = TraceSingleInitializer(pExpr->AsSelect()->Projection);
            break;
        }
    }

    return pExpr;
}

ParseTree::Expression *
XmlSemantics::TraceSingleInitializer(ParseTree::InitializerList * pInitList)
{
    // If list consists of a single item, then try to bind to that item
    if (pInitList && !pInitList->Next)
    {
        return TraceInitializer(pInitList->Element);
    }

    return NULL;
}

ParseTree::Expression *
XmlSemantics::TraceInitializer(ParseTree::Initializer * pInit)
{
    while (pInit)
    {
        switch (pInit->Opcode)
        {
            case ParseTree::Initializer::Deferred:
                pInit = pInit->AsDeferred()->Value;
                break;

            case ParseTree::Initializer::Assignment:
                pInit = pInit->AsAssignment()->Initializer;
                break;

            case ParseTree::Initializer::Expression:
                return pInit->AsExpression()->Value;
        }
    }

    return NULL;
}


XmlLocalSymbolMgr::XmlLocalSymbolMgr(Semantics *Analyzer) 
: m_Analyzer(Analyzer)
, m_NextFreeLocal(0)
{
}


ILTree::SymbolReferenceExpression *
XmlLocalSymbolMgr::AllocateLocal(Type *pType)
{
    ILTree::SymbolReferenceExpression *VariableReference;

    if (m_NextFreeLocal < m_Locals.Count())
    {
        VariableReference = m_Locals.Element(m_NextFreeLocal);
    }
    else
    {
        Location loc;
        loc.SetLocationToHidden();

        Variable *TempVar = m_Analyzer->AllocateShortLivedTemporary(pType);

        VariableReference =
            m_Analyzer->AllocateSymbolReference(
            TempVar,
            pType,
            NULL,   // No base reference for local
            loc);

        m_Locals.AddElement(VariableReference);
    }
    m_NextFreeLocal++;

    VSASSERT(pType == VariableReference->AsSymbolReferenceExpression().ResultType, "Wrong type passed to local symbol allocator");

    return VariableReference;
}


void
XmlAttributeManager::Add(_In_z_ STRING *Namespace, _In_z_ STRING *LocalPart)
{
    XmlQName &Name = m_Names.Grow();
    Name.Namespace = Namespace;
    Name.LocalPart = LocalPart;
}

bool
XmlAttributeManager::Find(_In_z_ STRING *Namespace, _In_z_ STRING *LocalPart)
{
    for (ULONG i = m_Names.Count(); i > 0; i--)
    {
        XmlQName &Name = m_Names.Element(i - 1);
        if (StringPool::IsEqualCaseSensitive(Name.Namespace, Namespace) && StringPool::IsEqualCaseSensitive(Name.LocalPart, LocalPart))
            return true;
    }


    return false;
}

bool
Semantics::CheckXmlFeaturesAllowed
(
    const Location & TextSpan,
    ExpressionFlags Flags
)
{
#if IDE 
    if (m_SourceFile && m_SourceFile->IsAspx())
    {
        ReportSemanticError(
            ERRID_XmlFeaturesNotSupported,
            TextSpan);

        return false;
    }
#endif

    if (HasFlag(Flags, ExprMustBeConstant))
    {
        ReportSemanticError(
            ERRID_RequiredConstExpr,
            TextSpan);

        return false;
    }

    // Now that Xml features are being used, ensure that the Xml helper class will be generated as part of the user's assembly
    // If XObject is not available, something has gone wrong, and we cannot continue
    if (!EnsureXmlHelperClass() || !m_XmlSymbols.GetXObject() || m_XmlSymbols.GetXObject()->IsBad())
    {
        ReportSemanticError(ERRID_XmlFeaturesNotAvailable, TextSpan);
        return false;
    }

    return true;
}

bool
Semantics::ExtractXmlNamespace
(
    ParseTree::Expression * Expr,
    ParseTree::IdentifierDescriptor & Prefix,
    ILTree::StringConstantExpression * & NamespaceExpr
)
{
    Prefix = ParserHelper::CreateIdentifierDescriptor(STRING_CONST(m_Compiler, EmptyString));
    NamespaceExpr = NULL;

    if (Expr->Opcode == ParseTree::Expression::XmlAttribute)
    {
        ParseTree::XmlAttributeExpression *Attribute = Expr->AsXmlAttribute();

        if (Attribute->Name->Opcode == ParseTree::Expression::XmlName)
        {
            ParseTree::XmlNameExpression *Name = Attribute->Name->AsXmlName();

            if (StringPool::IsEqualCaseSensitive(Name->Prefix.Name, STRING_CONST(m_Compiler, XmlNs)))
                Prefix = Name->LocalName;

            if (StringPool::StringLength(Prefix.Name) != 0 || StringPool::IsEqualCaseSensitive(Name->LocalName.Name, STRING_CONST(m_Compiler, XmlNs)))
            {
                // Verify expression is not an embedded expression - Namespaces must be constants.
                if (Attribute->Value->Opcode == ParseTree::Expression::XmlEmbedded)
                {
                    ReportSemanticError(ERRID_EmbeddedExpression, Attribute->Value->TextSpan);

                    Location Loc = Attribute->Value->TextSpan;
                    Loc.m_lEndColumn = Loc.m_lBegColumn;

                    NamespaceExpr = &ProduceStringConstantExpression(
                        L"",
                        0,
                        Loc
                        IDE_ARG(0))->AsStringConstant();
                }
                else
                {
                    ILTree::Expression *ValueExpr = Semantics::InterpretXmlExpression(Attribute->Value, ParseTree::Expression::XmlAttribute);

                    if (ValueExpr->bilop == SX_CNS_STR)
                    {
                        NamespaceExpr = &ValueExpr->AsStringConstant();
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

ILTree::Expression *
Semantics::GetDeferredTempForXNamespace(_In_ STRING *Namespace)
{
    ILTree::Expression *deferredTemp = NULL;

    if (!m_XmlNameVars)
    {
        m_XmlNameVars = new (m_TreeStorage) DynamicHashTable<STRING *,ILTree::Expression *,NorlsAllocWrapper>(&m_TreeStorage);
    }

    if (!m_XmlNameVars->GetValue(Namespace, &deferredTemp))
    {
        ParserHelper PH(&m_TreeStorage);

        ParseTree::Expression *value = PH.CreateBoundExpression(CreateXNamespaceExpression(Namespace, Location::GetHiddenLocation()));
        deferredTemp = AllocateDeferredTemp(value, m_XmlSymbols.GetXNamespace(), ExprForceRValue, Location::GetHiddenLocation());

        m_XmlNameVars->SetValue(Namespace, deferredTemp);
    }

    return deferredTemp;
}

ILTree::Expression *
Semantics::CreateXNamespaceExpression(_In_ STRING *NamespaceString, const Location &loc)
{
    ILTree::Expression *BoundArg =
        AllocateExpression(
            SX_ARG,
            TypeHelpers::GetVoidType(),
            ProduceStringConstantExpression(
                NamespaceString,
                StringPool::StringLength(NamespaceString),
                loc
                IDE_ARG(0)
                ),
        loc);

    ExpressionList *BoundList = AllocateExpression(
        SX_LIST,
        TypeHelpers::GetVoidType(),
        BoundArg,
        NULL,
        loc);

    ILTree::Expression *Target = ReferToProcByName(
        loc,
        m_XmlSymbols.GetXNamespace(),
        STRING_CONST(m_Compiler, XmlGetMethod),
        NULL,
        ExprIsExplicitCallTarget);

    return InterpretCallExpressionWithNoCopyout(
        loc,
        Target,
        chType_NONE,
        BoundList,
        false,
        ExprNoFlags,
        // OvrldNoFlags,
        NULL);
}

ILTree::Expression *
Semantics::GetXNamespaceReference(_In_ STRING *NamespaceString, const Location &loc)
{
    ILTree::Expression *XNamespaceRef;

    if ((m_SourceFile && m_SourceFile->GetProject()->GenerateENCableCode()) ||
        m_IsGeneratingXML)
    {
        // For edit and continue do not cache the xnamespace in a var.
        // Just generate the simple call to XNamespace.Get

        XNamespaceRef = CreateXNamespaceExpression(NamespaceString, loc);
    }
    else
    {
        XNamespaceRef = GetDeferredTempForXNamespace(NamespaceString);
    }

    return XNamespaceRef;
}


ILTree::Expression *
Semantics::GetDeferredTempForXName(_In_ STRING *LocalName, _In_ STRING *Namespace)
{
    ILTree::Expression *deferredTemp = NULL;

    if (!m_XmlNameVars)
    {
        m_XmlNameVars = new (m_TreeStorage) DynamicHashTable<STRING *,ILTree::Expression *,NorlsAllocWrapper>(&m_TreeStorage);
    }

    StringBuffer ConcatBuffer;

    // Create a fully qualified xml name
    ConcatBuffer.AppendChar(L'{');
    ConcatBuffer.AppendSTRING(Namespace);
    ConcatBuffer.AppendChar(L'}');
    ConcatBuffer.AppendSTRING(LocalName);

    STRING *FullName = m_Compiler->AddString(ConcatBuffer.GetString());

    if (!m_XmlNameVars->GetValue(FullName, &deferredTemp))
    {
        ParserHelper PH(&m_TreeStorage);

        ParseTree::Expression *value = PH.CreateBoundExpression(CreateXNameExpression(LocalName, Namespace, Location::GetHiddenLocation()));
        deferredTemp = AllocateDeferredTemp(value, m_XmlSymbols.GetXName(), ExprForceRValue, Location::GetHiddenLocation());

        m_XmlNameVars->SetValue(FullName, deferredTemp);
    }

    return deferredTemp;
}

ILTree::Expression *
Semantics::CreateXNameExpression(_In_ STRING *LocalName, _In_ STRING *NameSpace, const Location &loc)
{
    ILTree::Expression *BoundArg =
       AllocateExpression(
            SX_ARG,
            TypeHelpers::GetVoidType(),
            ProduceStringConstantExpression(
                LocalName,
                StringPool::StringLength(LocalName),
                loc
                IDE_ARG(0)
                ),
            loc);

    ExpressionList *BoundList = AllocateExpression(
        SX_LIST,
        TypeHelpers::GetVoidType(),
        BoundArg,
        NULL,
        loc);

    ILTree::Expression *Target = ReferToProcByName(
        loc,
        m_XmlSymbols.GetXNamespace(),
        STRING_CONST(m_Compiler, XmlGetNameMethod),
        GetXNamespaceReference(NameSpace, loc),
        m_XmlSemantics->m_Flags | ExprIsExplicitCallTarget);

    ILTree::Expression *XNameRef = InterpretCallExpressionWithNoCopyout(
        loc,
        Target,
        chType_NONE,
        BoundList,
        false,
        m_XmlSemantics->m_Flags,
        // OvrldNoFlags,
        NULL);

    return XNameRef;
}


ILTree::Expression *
Semantics::GetXNameReference(_In_ STRING *LocalName, _In_ STRING *Namespace, const Location &loc)
{
    ILTree::Expression *XNameRef;

    if ((m_SourceFile && m_SourceFile->GetProject()->GenerateENCableCode()) ||
        m_IsGeneratingXML)
    {
        // For edit and continue do not cache the xnamespace in a var.
        // Just generate the simple call to XName.Get

        XNameRef = CreateXNameExpression(LocalName, Namespace, loc);
    }
    else
    {
       XNameRef = GetDeferredTempForXName(LocalName, Namespace);
    }

    return XNameRef;
}

ULONG
Semantics::AddInScopeXmlNamespaces
(
    ParseTree::ExpressionList *Attributes
)
{
    ULONG Count = 0;
    Scope * Hash = NULL;

    while (Attributes)
    {
        ParseTree::IdentifierDescriptor Prefix;
        ILTree::StringConstantExpression *NamespaceExpr = NULL;
        bool CreatedImportedNamespaceAttribute = false;

        if (ExtractXmlNamespace(Attributes->Element, Prefix, NamespaceExpr))
        {
            ValidateXmlNamespace(Prefix, NamespaceExpr, m_Errors, m_Compiler);
        }

        if (NamespaceExpr)
        {
            if (!Hash)
            {
                Hash = m_SymbolCreator.GetHashTable(NULL, m_XmlSemantics->m_Prefixes, true, 1, NULL);
                m_XmlSemantics->m_Prefixes = Hash;
            }

            BCSYM_XmlNamespaceDeclaration * NamespaceSymbol = NULL;
            STRING * NamespaceSTRING = NULL;

            NamespaceSTRING = m_Compiler->AddStringWithLen(NamespaceExpr->Spelling, NamespaceExpr->Length);

            if (!m_IsGeneratingXML)
            {
                Declaration *PrefixAlias = InterpretXmlPrefix(m_XmlSemantics->m_Prefixes, Prefix.Name, NameSearchDonotResolveImportsAlias, m_SourceFile);

                if (PrefixAlias)
                {
                    BCSYM_XmlNamespaceDeclaration *XmlNamespaceDeclaration = PrefixAlias->PAlias()->GetSymbol()->PXmlNamespaceDeclaration();

                    // If this attribute is functioning the same as an imported namespace, add it to the imported namespace list.

                    if (XmlNamespaceDeclaration->IsImported() &&
                        StringPool::IsEqualCaseSensitive(NamespaceSTRING, XmlNamespaceDeclaration->GetName()))
                    {
                        CreatedImportedNamespaceAttribute = m_XmlSemantics->EnsureImportedNamespaceAttribute(PrefixAlias->PAlias());
                    }
                }
            }

            if (!CreatedImportedNamespaceAttribute)
            {
                NamespaceSymbol = m_SymbolCreator.GetXmlNamespaceDeclaration(NamespaceSTRING, m_SourceFile,  m_Project, &Attributes->Element->TextSpan, false);

                BCSYM_Alias *PrefixAlias =  m_SymbolCreator.GetAlias(
                    &Prefix.TextSpan,
                    Prefix.Name,
                    0,
                    NamespaceSymbol,
                    NULL);

                Symbols::AddSymbolToHash(Hash, PrefixAlias, true, false, false);
                Count++;
            }
        }

        Attributes = Attributes->Next;
    }

    return Count;
}

ULONG
Semantics::AddInScopeXmlNamespaces
(
    ParseTree::XmlElementExpression * Expr
)
{
    ULONG Count = 0;

    if (Expr && Expr->Parent)
    {
        Stack<ParseTree::XmlElementExpression*> Path;

        while (Expr->Parent)
        {
            Expr = Expr->Parent;
            Path.Push(Expr);
        }

        while (Path.Count() > 0)
        {
            Expr = Path.Top();
            Path.Pop();
            Count += AddInScopeXmlNamespaces(Expr->Attributes);
        }

        Path.Destroy();
    }
    return Count;
}


class StringComparer
{
public:
    StringComparer()
    {
    }

    static int
    Compare
    (
        _In_z_ STRING* left,
        _In_z_ STRING* right
    )
    {
        return left == right ? 0 : 1;
    }
};


void 
Semantics::AddInScopePrefixAndNamespace
(
     BCSYM_Alias *prefixAlias,
     ArrayList<STRING *, NorlsAllocWrapper> *inScopePrefixes, 
     ArrayList<STRING*, NorlsAllocWrapper> *inScopeNamespaces
 )
{
    // Add all in scope imported namespaces
    STRING *name = prefixAlias->GetName();
    StringComparer comparer;

    if (StringPool::StringLength(name) == 0)
    {
        name = STRING_CONST(m_Compiler, XmlNs);
    }

    if (!inScopePrefixes->LinearSearch(name, NULL, comparer))
    {
        inScopePrefixes->Add(name);
        inScopeNamespaces->Add(prefixAlias->GetSymbol()->PXmlNamespaceDeclaration()->GetName());
    }
}


void
Semantics::GetInScopePrefixesAndNamespaces(ILTree::Expression **prefixesExpr, ILTree::Expression **namespacesExpr)
{
    ArrayList<STRING *, NorlsAllocWrapper> inScopePrefixes(&m_TreeStorage);
    ArrayList<STRING*, NorlsAllocWrapper> inScopeNamespaces(&m_TreeStorage);

    for (XmlSemantics *xmlSemantics = m_XmlSemantics;
        xmlSemantics != NULL;
        xmlSemantics = xmlSemantics->m_Parent)
    {
        BCITER_HASH_CHILD PrefixIter(xmlSemantics->m_Prefixes);

        for (Declaration *next = PrefixIter.GetNext();
            next;
            next = PrefixIter.GetNext())
        {
            AddInScopePrefixAndNamespace(next->PAlias(), &inScopePrefixes, &inScopeNamespaces);
        }
    }

    for (XmlNamespaceImportsIterator Iterator(m_SourceFile); Iterator.MoveNext();)
    {
        BCSYM_Alias *prefixAlias = Iterator.Current()->m_pAlias;
        if (!StringPool::IsEqualCaseSensitive(prefixAlias->GetName(), STRING_CONST(m_Compiler, Xml)) &&
            !StringPool::IsEqualCaseSensitive(prefixAlias->GetName(), STRING_CONST(m_Compiler, XmlNs)) &&
             m_XmlSemantics->IsImportedNamespaceUsed(prefixAlias))
        {
            // Add all in scope imported namespaces
            AddInScopePrefixAndNamespace(prefixAlias, &inScopePrefixes, &inScopeNamespaces);
        }
    }

    Location loc = Location::GetHiddenLocation();
    ParserHelper PH(&m_TreeStorage, loc);

    ParseTree::Name * XNamespaceTypeName = PH.CreateName(
            PH.CreateGlobalNameSpaceName(),
            4,
            STRING_CONST(m_Compiler, ComDomain),
            STRING_CONST(m_Compiler, ComXmlDomain),
            STRING_CONST(m_Compiler, ComLinqDomain),
            STRING_CONST(m_Compiler, ComXmlNamespace));

    ParseTree::Name * StringTypeName = PH.CreateName(
        PH.CreateGlobalNameSpaceName(),
        2,
        STRING_CONST(m_Compiler, ComDomain),
        STRING_CONST(m_Compiler, ComString));

    ParseTree::NewArrayInitializerExpression * PrefixArray = NULL;
    ParseTree::NewArrayInitializerExpression * NamespaceArray = NULL;

    // Now look for imported xml prefix and namespaces
    ParseTree::Expression *InitialValue;
    ParseTree::Expression *Value = NULL;

    for (unsigned long i = 0; i < inScopePrefixes.Count(); i++)
    {
        // Add all in scope imported prefixes and namespaces
        ILTree::Expression *XNamespaceRef =  GetXNamespaceReference(inScopeNamespaces[i], loc);

        InitialValue = PH.CreateBoundExpression(XNamespaceRef);

        if (!NamespaceArray)
            NamespaceArray = PH.CreateNewArray(PH.CreateType(XNamespaceTypeName));

        PH.AddElementInitializer(NamespaceArray, InitialValue); 

        ILTree::Expression *prefix = ProduceStringConstantExpression(
            inScopePrefixes[i],
            StringPool::StringLength(inScopePrefixes[i]),
            loc
            IDE_ARG(0));

        InitialValue = PH.CreateBoundExpression(prefix);

        if (!PrefixArray)
            PrefixArray = PH.CreateNewArray(PH.CreateType(StringTypeName));

        PH.AddElementInitializer(PrefixArray, InitialValue); 
    }

    if (PrefixArray)
    {
        Value = PrefixArray;
    }
    else
    {
        // return Nothing
        Value = PH.CreateBoundExpression(AllocateExpression(SX_NOTHING, m_SymbolCreator.GetArrayType(1, GetFXSymbolProvider()->GetStringType()), loc));
    }

    *prefixesExpr = AllocateDeferredTemp(Value, m_SymbolCreator.GetArrayType(1, GetFXSymbolProvider()->GetStringType()), ExprForceRValue, loc);

    if (NamespaceArray)
    {
        Value = NamespaceArray;
    }
    else
    {
        // return Nothing
        Value = PH.CreateBoundExpression(AllocateExpression(SX_NOTHING, m_XmlSymbols.GetXNamespaceArray(&m_SymbolCreator), loc));
    }

    *namespacesExpr = AllocateDeferredTemp(Value, m_XmlSymbols.GetXNamespaceArray(&m_SymbolCreator), ExprForceRValue, loc);
}


ILTree::Expression*
Semantics::AllocateRootNamspaceAttributes()
{
    Location loc = Location::GetHiddenLocation();
    ParserHelper PH(&m_TreeStorage, loc);

    BCSYM_GenericBinding *pSymbol =  m_XmlSymbols.GetXAttributeList(&m_SymbolCreator);

    ParseTree::Expression *Value = PH.CreateBoundExpression(CreateConstructedInstance(pSymbol,
        loc,
        loc,
        NULL,
        0));

    ILTree::DeferredTempExpression *temp = AllocateDeferredTemp(Value,  pSymbol, ExprForceRValue, loc);

    return temp;
}

//+--------------------------------------------------------------------------
//  Method:     Semantics::InterpretXmlExpression
//
//  Description:
//                      Creates an expression to execute XmlExpression,
//
//
//  Parameters:
//      [Xml]   --  Expression evaluating to an XLinq object.
//
//  History:    06-27-2005  Microsoft    Added
//---------------------------------------------------------------------------
ILTree::Expression *
Semantics::InterpretXmlExpression
(
    ParseTree::Expression * Expr,
    ExpressionFlags Flags
)
{
    XmlSemantics XmlAnalyzer(m_XmlSemantics, this, Flags);
    ILTree::Expression *Result;

    if (!CheckXmlFeaturesAllowed(Expr->TextSpan, Flags))
    {
        return AllocateBadExpression(Expr->TextSpan);
    }

    if (!m_XmlSemantics)
    {
        m_XmlSemantics = &XmlAnalyzer;

        if (Expr->Opcode == ParseTree::Expression::XmlElement)
        {
            AddInScopeXmlNamespaces(Expr->AsXmlElement());
        }
        m_XmlSemantics = NULL;
    }

    if (HasFlag(Flags, ExprTypeInferenceOnly))
    {
        if (Expr->Opcode == ParseTree::Expression::XmlDocument ||
            Expr->Opcode == ParseTree::Expression::XmlElement)
        {
            // null out content to short circuit evaluation
            ParseTree::ExpressionList *Content = Expr->AsXml()->Content;
            Expr->AsXml()->Content = NULL;
            Result = InterpretXmlExpression(&XmlAnalyzer, Expr);
            Expr->AsXml()->Content = Content;
            return Result;
        }
    }

    Result = InterpretXmlExpression(&XmlAnalyzer, Expr);

    return Result;
}


//+--------------------------------------------------------------------------
//  Method:     Semantics::InterpretXmlExpression
//
//  Description:
//                      Creates an expression to execute XmlExpression,
//
//
//  Parameters:
//      [Xml]   --  Expression evaluating to an XLinq object.
//
//  History:    06-27-2005  Microsoft    Added
//---------------------------------------------------------------------------
ILTree::Expression *
Semantics::InterpretXmlExpression
(
    XmlSemantics *XmlAnalyzer,
    ParseTree::Expression * Expr
)
{
    XmlSemantics *OldXmlSemantics = m_XmlSemantics;
    m_XmlSemantics = XmlAnalyzer;

    // Initialize XmlNamesCache helper
    m_XmlSemantics->m_XmlNames.Initialize(&m_TransientSymbolCreator, m_SymbolsCreatedDuringInterpretation);

    if (!m_XmlSemantics->m_Prefixes)
    {
        m_XmlSemantics->m_Prefixes = m_SymbolCreator.GetHashTable(NULL, NULL, true, 1, NULL);

        m_XmlSemantics->m_EmptyPrefixAlias =
            m_SymbolCreator.GetAlias(
                STRING_CONST(m_Compiler, EmptyString),
                0,
                m_SymbolCreator.GetXmlNamespaceDeclaration(STRING_CONST(m_Compiler, EmptyString), m_SourceFile, m_Project, NULL, false),
                NULL);
    }

    ILTree::Expression *Result = InterpretXmlExpression(Expr, ParseTree::Expression::XmlElement);

    m_XmlSemantics = OldXmlSemantics;

    return Result;
}


ILTree::Expression*
Semantics::InterpretXmlExpression
(
    ParseTree::Expression * Xml,
    ParseTree::Expression::Opcodes Opcode
)
{
    ILTree::Expression * BoundExpr = NULL;
    ParseTree::ExpressionList *Content;

    switch (Xml->Opcode)
    {
        case ParseTree::Expression::XmlDocument:
            BoundExpr = InterpretXmlDocument(Xml->AsXmlDocument());
            break;

        case ParseTree::Expression::XmlElement:
            BoundExpr = InterpretXmlElement(Xml->AsXmlElement());
            break;

        case ParseTree::Expression::XmlAttribute:
            BoundExpr = InterpretXmlAttribute(Xml->AsXmlAttribute());
            break;

        case ParseTree::Expression::XmlName:
            BoundExpr = InterpretXmlName(Xml->AsXmlName(), NULL);
            break;

        case ParseTree::Expression::XmlPI:
            BoundExpr = InterpretXmlPI(Xml->AsXmlPI());
            break;

        case ParseTree::Expression::XmlComment:
            BoundExpr = InterpretXmlComment(Xml->AsXml());
            break;

        case ParseTree::Expression::XmlCharData:
            BoundExpr = InterpretXmlCharData(Xml->AsXmlCharData(), Opcode);
            break;

        case ParseTree::Expression::XmlCData:
            BoundExpr = InterpretXmlCData(Xml->AsXml());
            break;

        case ParseTree::Expression::XmlAttributeValueList:
            Content = Xml->AsXml()->Content;
            BoundExpr = ConcatenateAdjacentText(Content, ParseTree::Expression::XmlAttribute);
            break;

        case ParseTree::Expression::XmlEmbedded:
            if (!Xml->AsXmlEmbedded()->AllowEmdedded)
                ReportSemanticError(ERRID_EmbeddedExpression, Xml->TextSpan);
            BoundExpr = InterpretExpression(Xml->AsXmlEmbedded()->Operand, m_XmlSemantics->m_Flags);
            break;

         case ParseTree::Expression::XmlReference:
             ValidateEntityReference(Xml->AsXmlReference());

             BoundExpr = InterpretXmlCharData(Xml->AsXmlCharData(), ParseTree::Expression::XmlElement);
            break;

         case ParseTree::Expression::StringLiteral:
             // Allow stringliteral because value of created imported namespace attribute is a stringliteral
             BoundExpr = InterpretExpression(Xml, m_XmlSemantics->m_Flags);
             break;
    }

    if (BoundExpr == NULL)
    {
        BoundExpr = AllocateBadExpression(Xml->TextSpan);
    }

    return BoundExpr;
}


//+--------------------------------------------------------------------------
//  Method:     Semantics.InterpretXmlDocument
//
//  Description:
//                      Creates an expression to execute XmlDocument
//
//
//  Parameters:
//      [Xml]   --  Expression evaluating to an XLinq object.
//
//  History:    09-08-2005  Microsoft    Added
//---------------------------------------------------------------------------

ILTree::Expression *
Semantics::InterpretXmlDocument(ParseTree::XmlDocumentExpression *XmlDocument)
{
    Location loc =  XmlDocument->TextSpan;
    ParserHelper PH(&m_TreeStorage, loc);
    LocalSymbol DocumentReference(&m_XmlSemantics->m_XDocumentLocals, m_XmlSymbols.GetXDocument());

    ParseTree::ArgumentList *ArgList = PH.CreateArgList(2, InterpretXmlDeclaration(XmlDocument), PH.CreateNothingConst());

    ILTree::Expression *Constructor =
        m_XmlSemantics->CreateConstructedInstance(m_XmlSymbols.GetXDocument(), loc, ArgList);

    ILTree::Expression *Assignment = GenerateAssignment(
        loc,
        DocumentReference,
        Constructor,
        false);

    ILTree::Expression *Content = InterpretXmlContent(XmlDocument, DocumentReference, NULL);

    ILTree::Expression *Result =
        AllocateExpression(
        SX_SEQ_OP2,
        m_XmlSymbols.GetXDocument(),
        Assignment,
        Content,
        loc);

   SetFlag32(Result, SXF_SEQ_OP2_XML);

   return Result;
}


//+--------------------------------------------------------------------------
//  Method:     Semantics.InterpretXmlDeclaration
//
//  Description:
//                      Creates an expression to execute XmlDocument
//
//
//  Parameters:
//      [Xml]   --  Expression evaluating to an XLinq object.
//
//  History:    09-08-2005  Microsoft    Added
//---------------------------------------------------------------------------

ParseTree::Expression *
Semantics::InterpretXmlDeclaration(ParseTree::XmlDocumentExpression *XmlDocument)
{
    Location loc =  XmlDocument->TextSpan;
    ParserHelper PH(&m_TreeStorage, loc);

    ParseTree::Name * TypeName = PH.CreateName(
            PH.CreateGlobalNameSpaceName(),
            4,
            STRING_CONST(m_Compiler, ComDomain),
            STRING_CONST(m_Compiler, ComXmlDomain),
            STRING_CONST(m_Compiler, ComLinqDomain),
            STRING_CONST(m_Compiler, ComXmlDeclaration));

    ParseTree::Type *DeclarationType = PH.CreateType(TypeName);

    ParseTree::XmlAttributeExpression *Version = NULL;
    ParseTree::XmlAttributeExpression *Encoding = NULL;
    ParseTree::XmlAttributeExpression *Standalone = NULL;

    ULONG AttributeMark = m_XmlSemantics->m_AttributeMgr.GetMark();
    ParseTree::ExpressionList *Attributes = XmlDocument->Attributes;

    while (Attributes)
    {
        ParseTree::Expression *Element = Attributes->Element;
        if (Element->Opcode == ParseTree::Expression::XmlEmbedded)
        {
            ReportSemanticError(ERRID_EmbeddedExpression, Element->TextSpan);
        }
        else if (Element->Opcode == ParseTree::Expression::XmlAttribute)
        {
            ParseTree::XmlAttributeExpression *Attribute = Element->AsXmlAttribute();

            if (!Attribute->PrecededByWhiteSpace)
            {
                ReportSemanticError(ERRID_ExpectedXmlWhiteSpace, Attribute->Name->TextSpan);
            }

            if (Attribute->Value && Attribute->Value->Opcode == ParseTree::Expression::XmlEmbedded)
            {
                ReportSemanticError(ERRID_EmbeddedExpression, Attribute->Value->TextSpan);
            }

            if (Attribute->Name)
            {
                if (Attribute->Name->Opcode == ParseTree::Expression::XmlEmbedded)
                {
                    ReportSemanticError(ERRID_EmbeddedExpression, Attribute->Name->TextSpan);
                }
                else if (Attribute->Name->Opcode == ParseTree::Expression::XmlName)
                {
                    unsigned ErrorId = 0;
                    ParseTree::XmlNameExpression *QName = Attribute->Name->AsXmlName();

                    // Check for duplicates

                    STRING *Prefix = QName->Prefix.Name;
                    STRING *LocalPart = QName->LocalName.Name;

                    if (m_XmlSemantics->m_AttributeMgr.Find(Prefix, LocalPart))
                        ReportSemanticError(ERRID_DuplicateXmlAttribute, QName->TextSpan, Prefix, StringPool::StringLength(Prefix) != 0 ? L":" : (WCHAR *) NULL, LocalPart);
                    else
                        m_XmlSemantics->m_AttributeMgr.Add(Prefix, LocalPart);

                    if (StringPool::StringLength(Prefix) == 0)
                    {
                        if (StringPool::IsEqualCaseSensitive(LocalPart, STRING_CONST(m_Compiler, XmlDeclVersion)))
                        {
                            Version = Attribute;
                            if (Encoding || Standalone)
                                ReportSemanticError(ERRID_VersionMustBeFirstInXmlDecl, QName->TextSpan);

                            ValidateXmlAttributeValue(Version->Value, STRING_CONST(m_Compiler, V10));
                        }
                        else if (StringPool::IsEqualCaseSensitive(LocalPart, STRING_CONST(m_Compiler, XmlDeclEncoding)))
                        {
                            Encoding = Attribute;
                            if (Standalone)
                                ReportSemanticError(ERRID_AttributeOrder, QName->TextSpan, STRING_CONST(m_Compiler, XmlDeclEncoding), STRING_CONST(m_Compiler, XmlDeclStandalone));

                        }
                        else if (StringPool::IsEqualCaseSensitive(LocalPart, STRING_CONST(m_Compiler, XmlDeclStandalone)))
                        {
                            Standalone = Attribute;
                            ValidateXmlAttributeValue2(Standalone->Value, STRING_CONST(m_Compiler, Yes), STRING_CONST(m_Compiler, No));
                        }
                        else
                            ErrorId = ERRID_IllegalAttributeInXmlDecl;
                    }
                    else
                        ErrorId = ERRID_IllegalAttributeInXmlDecl;

                    if (ErrorId)
                        ReportSemanticError(ErrorId, QName->TextSpan, Prefix, StringPool::StringLength(Prefix) != 0 ? L":" : (WCHAR *) NULL, LocalPart);
                }
             }
        }

        Attributes = Attributes->Next;
    }

    m_XmlSemantics->m_AttributeMgr.ReleaseToMark(AttributeMark);


    if (!Version)
        ReportSemanticError(ERRID_MissingVersionInXmlDecl, XmlDocument->DeclTextSpan);

    ParseTree::ArgumentList *ArgList = PH.CreateArgList(3,
        Version ? Version->Value : NULL,
        Encoding ? Encoding->Value : NULL,
        Standalone ? Standalone->Value : NULL);

    return PH.CreateNewObject(DeclarationType, ArgList);
}


//+--------------------------------------------------------------------------
//  Method:     Semantics.InterpretXmlContent
//
//  Description:
//                      Creates an expression to execute XmlExpression,
//
//
//  Parameters:
//      [Xml]   --  Expression evaluating to an XLinq object.
//
//  History:    06-27-2005  Microsoft    Added
//---------------------------------------------------------------------------

ILTree::Expression *
Semantics::InterpretXmlContent(ParseTree::XmlExpression *ParentElement, ILTree::SymbolReferenceExpression *Parent, ILTree::Expression **LastExpression)
{
    Location loc;
    loc.SetLocationToHidden();

    ILTree::Expression *PrevExpression = NULL;
    ILTree::Expression *NextExpression = NULL;
    ILTree::Expression *Result = NULL;
    ILTree::Expression *BoundExpr = NULL;
    ParseTree::ExpressionList * Content = ParentElement->Content;
    ILTree::Expression *prefixesExpr = NULL;
    ILTree::Expression *namespacesExpr = NULL;

    while (Content)
    {
        ParseTree::Expression *Expr = Content->Element;
#if IDE    
        if (CheckStop(this)) // if the foreground thread wants to stop the bkd, then let's not recur so IDE is more responsive
        {
            ReportSemanticError(ERRID_EvaluationAborted, Expr->TextSpan);
            return AllocateBadExpression(Expr->TextSpan);
        }
#endif IDE


        switch (Expr->Opcode)
        {
        case ParseTree::Expression::XmlElement:
            BoundExpr = InterpretXmlElement(Expr->AsXmlElement());
            break;

        case ParseTree::Expression::XmlCharData:
        case ParseTree::Expression::XmlReference:
            BoundExpr = ConcatenateAdjacentText(Content, Expr->Opcode);
            break;

        case ParseTree::Expression::XmlComment:
            BoundExpr = InterpretXmlComment(Expr->AsXml());
            break;

        case ParseTree::Expression::XmlPI:
            BoundExpr = InterpretXmlPI(Expr->AsXmlPI());
            break;

        case ParseTree::Expression::XmlCData:
            BoundExpr = InterpretXmlCData(Expr->AsXml());
            break;

        case ParseTree::Expression::XmlEmbedded:
            {
                Expr = Expr->AsXmlEmbedded()->Operand;
                ParseTree::Expression *LinqResult = m_XmlSemantics->TraceLinqExpression(Expr);

                // If static analysis shows that the expression is an xmlelement then let the element inherit in scope namespaces
                // to avoid adding and removing extra namespace declarations.

                BackupValue<bool> backup_m_InheritImportedNamespaces(&m_XmlSemantics->m_InheritImportedNamespaces);

                if (LinqResult->Opcode == ParseTree::Expression::XmlElement && ParentElement->Opcode == ParseTree::Expression::XmlElement)
                {
                    m_XmlSemantics->m_InheritImportedNamespaces = true;
                }

                BoundExpr = InterpretExpression(Expr, m_XmlSemantics->m_Flags);

                if (LinqResult->Opcode != ParseTree::Expression::XmlElement &&
                    ParentElement->Opcode == ParseTree::Expression::XmlElement &&
                    BoundExpr &&
                    !IsBad(BoundExpr))
                {
                    ParserHelper PH(&m_TreeStorage);

                    // Only insert the InternalXmlHelper.Add to remove imported namespaces if the expression is not intrinsic or enum
                    // and there are imported xml namespaces are defined in the module. Because namespaces are removed from children
                    // and also bubble up, RemoveXmlNamespaces must be called even when no namespaces are currently in scope.

                    if (TypeHelpers::IsVoidType(BoundExpr->ResultType))
                    {
                        ReportSemanticError(ERRID_VoidValue, Expr->TextSpan);
                        BoundExpr = AllocateExpression(SX_NOTHING, GetFXSymbolProvider()->GetObjectType(), loc);
                    }
                    else if (m_ProcedureTree && 
                        m_XmlSemantics->AreImportedNamespacesDefined() && 
                        !TypeHelpers::IsIntrinsicOrEnumType(BoundExpr->ResultType) && BoundExpr->bilop != SX_NOTHING)
                    {

                        if (m_XmlSemantics->m_RootNamespaceAttributes == NULL)
                        {
                            m_XmlSemantics->m_RootNamespaceAttributes = AllocateRootNamspaceAttributes();
                        }
    
                        if (!prefixesExpr)
                        {
                            GetInScopePrefixesAndNamespaces(&prefixesExpr, &namespacesExpr);
                        }

                        ParseTree::ArgumentList *ArgList = PH.CreateArgList(4, 
                            PH.CreateBoundExpression(prefixesExpr),
                            PH.CreateBoundExpression(namespacesExpr),
                            PH.CreateBoundExpression(m_XmlSemantics->m_RootNamespaceAttributes),
                            PH.CreateBoundExpression(BoundExpr));

                        ILTree::Expression *Target = ReferToSymbol(
                            loc,
                            GetXmlHelperMethod(STRING_CONST(m_Compiler, XmlRemoveNamespacesMethod)),
                            chType_NONE,
                            NULL,
                            NULL,
                            m_XmlSemantics->m_Flags | ExprIsExplicitCallTarget);

                        BoundExpr = BindArgsAndInterpretCallExpressionWithNoCopyOut(loc, Target, chType_NONE, ArgList, m_XmlSemantics->m_Flags, OvrldNoFlags, NULL);
                    }
                }
            }
            break;

        default:
            BoundExpr = NULL;
            break;
        }

        if (BoundExpr && !IsBad(BoundExpr))
        {
            ParseTree::ArgumentList *ArgList = new(m_TreeStorage) ParseTree::ArgumentList;
            ArgList->Element = new(m_TreeStorage) ParseTree::Argument;
            ArgList->Element->Value = Expr;

            ILTree::Expression *Target =
                ReferToProcByName(
                loc,
                m_XmlSymbols.GetXContainer(),
                STRING_CONST(m_Compiler, Add),
                Parent,
                m_XmlSemantics->m_Flags | ExprIsExplicitCallTarget);

            ILTree::Expression *BoundArg =
                AllocateExpression(
                SX_ARG,
                TypeHelpers::GetVoidType(),
                BoundExpr,
                BoundExpr->Loc);

            ExpressionList *BoundList = AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                BoundArg,
                NULL,
                loc);

            BoundExpr = InterpretCallExpressionWithNoCopyout(
                loc,
                Target,
                chType_NONE,
                BoundList,
                false,
                m_XmlSemantics->m_Flags,
                // OvrldNoFlags,
                NULL);

            NextExpression =
                AllocateExpression(
                SX_SEQ_OP2,
                Parent->ResultType,
                BoundExpr,
                NULL,
                loc);

            if (PrevExpression)
            {
                PrevExpression->AsExpressionWithChildren().Right = NextExpression;
            }
            else
            {
                Result = NextExpression;
            }

            PrevExpression = NextExpression;
        }

        Content = Content->Next;
    }

    if (PrevExpression)
    {
        PrevExpression->AsExpressionWithChildren().Right = Parent;
    }
    else if (ParentElement->Opcode == ParseTree::Expression::XmlElement &&
        !ParentElement->AsXmlElement()->IsEmpty)
    {
        ILTree::Expression *EmptyString = ProduceStringConstantExpression(
            L"",
            0,
            loc
            IDE_ARG(0));

        ILTree::Expression *BoundArg =
                AllocateExpression(
                    SX_ARG,
                    TypeHelpers::GetVoidType(),
                    EmptyString,
                    EmptyString->Loc);

        ExpressionList *BoundList = AllocateExpression(
            SX_LIST,
            TypeHelpers::GetVoidType(),
            BoundArg,
            NULL,
            loc);

        ILTree::Expression *Target =
            ReferToProcByName(
            loc,
            m_XmlSymbols.GetXContainer(),
            STRING_CONST(m_Compiler, Add),
            Parent,
            m_XmlSemantics->m_Flags | ExprIsExplicitCallTarget);

        ILTree::Expression *CallAdd = InterpretCallExpressionWithNoCopyout(
            loc,
            Target,
            chType_NONE,
            BoundList,
            false,
            m_XmlSemantics->m_Flags,
            // OvrldNoFlags,
            NULL);

        Result = PrevExpression = AllocateExpression(
            SX_SEQ_OP2,
            Parent->ResultType,
            CallAdd,
            Parent,
            loc);
    }
    else
    {
        Result = Parent;
        PrevExpression = NULL;
    }

    if (LastExpression)
    {
        *LastExpression = PrevExpression;
    }

    return Result;
}

ILTree::Expression *
Semantics::InterpretXmlComment(ParseTree::XmlExpression *Comment)
{
    return InterpretXmlOther(Comment, m_XmlSymbols.GetXComment(), NULL);
}


ILTree::Expression *
Semantics::InterpretXmlCData(ParseTree::XmlExpression *CData)
{
    return InterpretXmlOther(CData, m_XmlSymbols.GetXCData(), NULL);
}

ILTree::Expression *
Semantics::InterpretXmlPI(ParseTree::XmlPIExpression *PI)
{
    Location loc = PI->TextSpan;
    ParserHelper PH(&m_TreeStorage, loc);
    ParseTree::ArgumentList *ArgList  = NULL;
    ParseTree::Expression *PITarget = NULL;

    // PI Name must be a constant string

    if (PI->Name && PI->Name->Opcode == ParseTree::Expression::XmlName)
    {
        ParseTree::XmlNameExpression *Name = PI->Name->AsXmlName();
        if (StringPool::StringLength(Name->Prefix.Name) != 0)
        {
            // PI name can't be qualified
            ReportSemanticError(ERRID_QualifiedNameNotAllowed, Name->TextSpan, Name->LocalName.Name);
        }
        else if (_wcsicmp(Name->LocalName.Name, L"xml") == 0)
        {
            // Xml is not allowed as a PI Name
            ReportSemanticError(ERRID_IllegalProcessingInstructionName, Name->TextSpan, Name->LocalName.Name);
        }

        ValidateXmlName(Name->LocalName.Name, Name->LocalName.TextSpan, m_Errors);
        PITarget =  PH.CreateStringConst(Name->LocalName.Name);

        if (PI->MissingWhiteSpace)
        {
            Location TextSpan = PI->Name->TextSpan;
            TextSpan.m_lEndColumn += 1;
            TextSpan.m_lBegColumn = TextSpan.m_lEndColumn;

            ReportSemanticError(ERRID_ExpectedXmlWhiteSpace, TextSpan);
        }
    }
    else
    {
        ReportSemanticError(ERRID_ExpectedXmlName, PI->TextSpan);
    }


    ArgList = new(m_TreeStorage) ParseTree::ArgumentList;

    ParseTree::Argument *Arg = new(m_TreeStorage) ParseTree::Argument;

    ArgList->Element = Arg;

    Arg->Value =  PITarget;

    return InterpretXmlOther(PI, m_XmlSymbols.GetXProcessingInstruction(), ArgList);
}

ILTree::Expression *
Semantics::InterpretXmlOther(ParseTree::XmlExpression *Expr, Type *XObjectType, ParseTree::ArgumentList *ArgList)
{
    Location loc =  Expr->TextSpan;
    ParserHelper PH(&m_TreeStorage, loc);

    // ConcatenateAdjacentText updates Content so assign to local before calling.
    // todo - Microsoft - Make a version of ConcatenateAdjacentText that is not destructive!

    ParseTree::ExpressionList *Content = Expr->Content;

    ILTree::Expression *BoundExpr = ConcatenateAdjacentText(Content, Expr->Opcode);

    if (!BoundExpr)
    {
        BoundExpr =
            ProduceStringConstantExpression(
            L"",
            0,
            loc
            IDE_ARG(0));
    }

    ParseTree::ArgumentList *DataList  = new(m_TreeStorage) ParseTree::ArgumentList;

    ParseTree::Argument *Arg = new(m_TreeStorage) ParseTree::Argument;

    DataList->Element = Arg;

    Arg->Value = PH.CreateBoundExpression(BoundExpr);

    if (ArgList)
    {
        ArgList->Next = DataList;
    }
    else
    {
        ArgList = DataList;
    }

    return  m_XmlSemantics->CreateConstructedInstance(XObjectType, loc, ArgList);
}

//+--------------------------------------------------------------------------
//  Method:     Semantics.ConcatenateAdjacentText
//
//  Description:
//              Walk the content and concatenate the text.  Normalize atttribute value and strip
//              whitepsace depending on layout rules and whether the whitespace is significant or not.
//
//
//  Parameters:
//      [Xml]   --  Expression evaluating to an XLinq object.  This may be NULL if content is insignificant whitespace.
//
//  History:    06-27-2005  Microsoft    Added
//---------------------------------------------------------------------------

ILTree::Expression *
Semantics::ConcatenateAdjacentText(ParseTree::ExpressionList *&Content, ParseTree::Expression::Opcodes Opcode)
{
     ParseTree::XmlCharDataExpression *XmlCharData;
     ILTree::Expression *StringValue = NULL;
     StringBuffer Buffer;
     ParseTree::ExpressionList *Last=Content;

     VSASSERT(!Content ||
         (Content->Element &&
         (Content->Element->Opcode == ParseTree::Expression::XmlCharData ||
         Content->Element->Opcode == ParseTree::Expression::XmlReference ||
         Content->Element->Opcode == ParseTree::Expression::SyntaxError)), "Bad Xml Content List");

     for (ParseTree::ExpressionList *Current = Content; Current; Current = Current->Next)
     {
         // If this is an attribute value then normalize new lines to whitespace

         ParseTree::Expression *Expr = Current->Element;
         bool AppendData = true;

         switch (Expr->Opcode)
         {
         case ParseTree::Expression::XmlCharData:
             XmlCharData = Expr->AsXmlCharData();
             if (XmlCharData->IsSignificant)
             {
                 switch (Opcode)
                 {
                 case ParseTree::Expression::XmlAttribute:
                     // Normalize attribute whitespace
                     NormalizeAttributeValue(XmlCharData, Buffer);
                     AppendData = false;
                     break;

                 case ParseTree::Expression::XmlComment:
                     if (XmlCharData->LengthInCharacters == 2 &&
                         XmlCharData->Value[0] == '-' &&
                         XmlCharData->Value[1] == '-')
                     {
                         ReportSemanticError(ERRID_IllegalXmlCommentChar, Expr->TextSpan);
                         break;
                     }
                     break;
                 }

                 if (AppendData)
                    Buffer.AppendWithLength(XmlCharData->Value, XmlCharData->LengthInCharacters);
             }
             break;

         case ParseTree::Expression::XmlReference:
             // Do not normalize entity references.  Per spec, character entity references are exempt.
             // Because general entity references are not supported by VB, there is no need to do any normalization.
             {
                 ParseTree::XmlReferenceExpression *Reference= Expr->AsXmlReference();

                 ValidateEntityReference(Reference);

                 Buffer.AppendWithLength(Reference->Value, Reference->LengthInCharacters);
             }
             break;

         default:
             goto Done;
             break;
         }

         Last = Current;
     }

Done:
     Content = Last;
     if (Buffer.GetStringLength())
     {
         long Length = Buffer.GetStringLength();
         WCHAR *Value = new(m_TreeStorage) WCHAR[Length + 1];

         memcpy(
             Value,
             Buffer.GetString(),
             Length * sizeof(WCHAR));

         Value[Length] = L'\0';

         StringValue = ProduceStringConstantExpression(
            Value,
            Length,
            Content->TextSpan
            IDE_ARG(0));
     }

     return StringValue;
}


ILTree::Expression *
Semantics::InterpretXmlCharData(ParseTree::XmlCharDataExpression *XmlCharData, ParseTree::Expression::Opcodes Opcode)
{
    size_t Length;
    WCHAR *Value;

    if (Opcode == ParseTree::Expression::XmlAttribute)
    {
        StringBuffer Buffer;

        NormalizeAttributeValue(XmlCharData, Buffer);
        Length = Buffer.GetStringLength();

        Value = new(m_TreeStorage) WCHAR[Length + 1];
        memcpy(
            Value,
            Buffer.GetString(),
            Length * sizeof(WCHAR));
    }
    else
    {
        Length = XmlCharData->LengthInCharacters;

        Value = new(m_TreeStorage) WCHAR[Length + 1];
        memcpy(
            Value,
            XmlCharData->Value,
            Length * sizeof(WCHAR));
    }

    Value[Length] = L'\0';

    ILTree::Expression *Result =
        ProduceStringConstantExpression(
        Value,
        Length,
        XmlCharData->TextSpan
        IDE_ARG(0));

    return Result;
}


//+--------------------------------------------------------------------------
//    Method:     Semantics.InterpretXmlName
//
//    Description:
//                      Creates an expression to execute XmlExpression,
//
//
//    Parameters:
//        [Xml]     --    Expression evaluating to an XLinq object.
//
//    History:    06-27-2005     Microsoft     Added
//---------------------------------------------------------------------------

ILTree::Expression *
Semantics::InterpretXmlName(ParseTree::Expression * Expr, BCSYM_Alias **ResolvedPrefix)
{
    Location loc = Expr->TextSpan;
    Location hidden = Location::GetHiddenLocation();
    ParserHelper PH(&m_TreeStorage, loc);

    if (Expr->Opcode == ParseTree::Expression::XmlName)
    {
        ParseTree::XmlNameExpression *Name = Expr->AsXmlName();
        ParseTree::Expression *NamespaceExpr = NULL;
        ParseTree::Expression *LocalPartExpr = NULL;
        Declaration *PrefixAlias = NULL;
        Declaration *LocalAlias = NULL;

        ValidateXmlName(Name->Prefix.Name, Name->Prefix.TextSpan, m_Errors);
        ValidateXmlName(Name->LocalName.Name, Name->LocalName.TextSpan, m_Errors);

        // Lookup prefix in current scope
        if (Name->IsElementName)
        {
            // Element names can use the default namespace
            if (StringPool::IsEqualCaseSensitive(Name->Prefix.Name, STRING_CONST(m_Compiler, XmlNs)))
            {
                // DD Bug #26898: Literal elements are not allowed to xmlns as a prefix
                ReportSemanticError(ERRID_IllegalXmlnsPrefix, Name->Prefix.TextSpan);
            }

            PrefixAlias = Semantics::InterpretXmlPrefix(m_XmlSemantics->m_Prefixes, Name->Prefix.Name, NameSearchDonotResolveImportsAlias, m_SourceFile);
        }
        else if (StringPool::StringLength(Name->Prefix.Name) != 0)
        {
            // attribute names do not use the default namespace
            PrefixAlias = Semantics::InterpretXmlPrefix(m_XmlSemantics->m_Prefixes, Name->Prefix.Name, NameSearchDonotResolveImportsAlias, m_SourceFile);
        }
        else
        {
            // Use default empty namespace for attributes without a prefix.

            PrefixAlias = m_XmlSemantics->m_EmptyPrefixAlias;
        }

        if (!PrefixAlias)
        {
            ReportSemanticError(ERRID_UndefinedXmlPrefix, Name->Prefix.TextSpan, Name->Prefix.Name);
            PrefixAlias = m_XmlSemantics->m_EmptyPrefixAlias;
        }

        if (ResolvedPrefix)
        {
            *ResolvedPrefix = PrefixAlias->PAlias();
        }

        BCSYM_XmlNamespaceDeclaration *pXmlNamespaceDeclaration = PrefixAlias->PAlias()->GetSymbol()->PXmlNamespaceDeclaration();

        STRING * NamespaceString = pXmlNamespaceDeclaration->GetName();

        if (StringPool::StringLength(NamespaceString) == 0)
        {
            NamespaceExpr = m_XmlSemantics->m_EmptyNamespace;
        }
        else
        {
            NamespaceExpr = PH.CreateStringConst(NamespaceString);
        }

        // Hide everything by default.
        ParserHelper PHHidden(&m_TreeStorage, hidden);

#if IDE 
        // If m_isGeneratingXML is true, generate symbols which the symbol locator can find
        if (m_IsGeneratingXML && PrefixAlias)
        {
            // Don't create symbol for xmlns namespace
            if (!StringPool::IsEqualCaseSensitive(PrefixAlias->GetName(), STRING_CONST(m_Compiler, XmlNs)))
            {
                NamespaceExpr = PH.CreateBoundExpression(
                   AllocateSymbolReference(
                        PrefixAlias,
                        GetFXSymbolProvider()->GetStringType(),
                        NULL,
                        loc));
            }

            pXmlNamespaceDeclaration = PrefixAlias->PAlias()->GetAliasedSymbol()->PXmlNamespaceDeclaration();

            // Get XmlNamespace for this XmlNamespace declaration
            BCSYM_XmlNamespace *pXmlNamespace = m_XmlSemantics->m_XmlNames.GetXmlNamespace(pXmlNamespaceDeclaration->GetName(), m_SourceFile, m_Project);

            // Lookup symbol for the local name
            Declaration *LocalName = NULL;

            if (StringPool::IsEqualCaseSensitive(PrefixAlias->GetName(), STRING_CONST(m_Compiler, XmlNs)))
            {
                // Name is an xmlns:prefix definition, so create a prefix (alias) symbol out of the local name
                LocalName = Semantics::InterpretXmlPrefix(m_XmlSemantics->m_Prefixes, Name->LocalName.Name, NameSearchDonotResolveImportsAlias, m_SourceFile);
            }
            else
            {
                // Don't create symbol if attribute name = xmlns
                if (Name->IsElementName || StringPool::StringLength(PrefixAlias->GetName()) || !StringPool::IsEqualCaseSensitive(Name->LocalName.Name, STRING_CONST(m_Compiler, XmlNs)))
                {
                    LocalName = m_XmlSemantics->m_XmlNames.GetXmlName(pXmlNamespace, Name->LocalName.Name);
                }
            }

            if (LocalName)
            {
                LocalPartExpr = PH.CreateBoundExpression(
                    AllocateSymbolReference(
                    LocalName,
                    GetFXSymbolProvider()->GetStringType(),
                    NULL,
                    loc));
            }
        }
#endif

        // If no local expression has been created yet, create a simple string constant
        if (!LocalPartExpr)
        {
            LocalPartExpr = PHHidden.CreateStringConst(Name->LocalName.Name);
        }

        ILTree::Expression *Target;
        ParseTree::ArgumentList *ArgList;

        if ((m_SourceFile && m_SourceFile->GetProject()->GenerateENCableCode()) ||
            m_IsGeneratingXML)
        {
            // For edit and continue or debugger scope do not cache the xnamespace in a var.
            // Just generate the simple call to XName.Get

            ArgList = PHHidden.CreateArgList(2,
                LocalPartExpr,
                NamespaceExpr);

            Target = ReferToProcByName(
                loc,
                m_XmlSymbols.GetXName(),
                STRING_CONST(m_Compiler, XmlGetMethod),
                NULL,
                m_XmlSemantics->m_Flags | ExprIsExplicitCallTarget);

            return BindArgsAndInterpretCallExpressionWithNoCopyOut(loc, Target, chType_NONE, ArgList, m_XmlSemantics->m_Flags, OvrldNoFlags, NULL);
        }
        else
        {
            return GetXNameReference(Name->LocalName.Name, NamespaceString, loc);
        }

    }

    ILTree::Expression *NameExpr = InterpretExpression(Expr, m_XmlSemantics->m_Flags);

    if (NameExpr->ResultType == GetFXSymbolProvider()->GetObjectType() && !m_UsingOptionTypeStrict)
    {
        NameExpr = AllocateExpression(
                            SX_DIRECTCAST,
                            m_XmlSymbols.GetXName(),
                            NameExpr,
                            NameExpr->Loc);
    }

    return NameExpr;
}


ILTree::Expression *
Semantics::InterpretXmlAttribute(ParseTree::XmlAttributeExpression * Attribute)
{
    ParseTree::Expression *Expr = Attribute->Name;
    Location loc =  Attribute->TextSpan;
    ParserHelper PH(&m_TreeStorage, loc);
    ILTree::Expression *Result;
    BCSYM_Alias *ResolvedPrefix = NULL;
    Declaration *PrefixDeclaration;
    BCSYM_XmlNamespaceDeclaration *XmlNamespaceDeclaration;

    if (!Attribute->PrecededByWhiteSpace)
    {
        ReportSemanticError(ERRID_ExpectedXmlWhiteSpace, Attribute->Name->TextSpan);
    }

    if (Attribute->Delimiter == tkXmlWhiteSpace)
         ReportSemanticError(ERRID_ExpectedQuote, Attribute->Value->TextSpan);

    ILTree::Expression *NameExpr = InterpretXmlName(Attribute->Name, &ResolvedPrefix);

    if (Attribute->Name->Opcode == ParseTree::Expression::XmlName)
    {
        ParseTree::XmlNameExpression *Name = Attribute->Name->AsXmlName();
        if (StringPool::IsEqualCaseSensitive(Name->Prefix.Name, STRING_CONST(m_Compiler, Xml)) &&
            StringPool::IsEqualCaseSensitive(Name->LocalName.Name, STRING_CONST(m_Compiler, Space)))
        {
            ValidateXmlAttributeValue2(Attribute->Value,
                STRING_CONST(m_Compiler, Default),
                STRING_CONST(m_Compiler, Preserve));
        }

        if (ResolvedPrefix)
        {
            XmlNamespaceDeclaration = ResolvedPrefix->GetSymbol()->PXmlNamespaceDeclaration();

            STRING *Namespace = XmlNamespaceDeclaration->GetName();

            m_XmlSemantics->EnsureImportedNamespaceAttribute(ResolvedPrefix);

            // Check for duplicate attributes

            if (m_XmlSemantics->m_AttributeMgr.Find(Namespace, Name->LocalName.Name))
            {
                ReportSemanticError(ERRID_DuplicateXmlAttribute, Name->TextSpan, Name->Prefix.Name, StringPool::StringLength(Name->Prefix.Name) != 0 ? L":" : L"", Name->LocalName.Name);
            }
            else
            {
                m_XmlSemantics->m_AttributeMgr.Add(Namespace, Name->LocalName.Name);
            }
        }

        if (Attribute->IsImportedNamespace)
        {
            // When IsImportedNamespace is true the value is STRING from a Namespace symbol.

            STRING *NamespaceString = Attribute->Value->AsStringLiteral()->Value;

            ILTree::Expression *XNamespaceRef =  GetXNamespaceReference(NamespaceString, Location::GetHiddenLocation());

            ParseTree::ArgumentList *ArgList = PH.CreateArgList(2,
                PH.CreateBoundExpression(NameExpr),
                PH.CreateBoundExpression(XNamespaceRef));

            ILTree::Expression *Target = ReferToSymbol(
                loc,
                GetXmlHelperMethod(STRING_CONST(m_Compiler, XmlCreateNamespaceMethod)),
                chType_NONE,
                NULL,
                NULL,
                m_XmlSemantics->m_Flags | ExprIsExplicitCallTarget);

            return BindArgsAndInterpretCallExpressionWithNoCopyOut(loc, Target, chType_NONE, ArgList, m_XmlSemantics->m_Flags, OvrldNoFlags, NULL);
        }
        else
        {
            STRING *Prefix;
            STRING *LocalName;

            if (StringPool::StringLength(Name->Prefix.Name) != 0)
            {
                Prefix = Name->Prefix.Name;
                LocalName = Name->LocalName.Name;
            }
            else
            {
                Prefix = Name->LocalName.Name;
                LocalName = STRING_CONST(m_Compiler, EmptyString);
            }

            if (StringPool::IsEqualCaseSensitive(Prefix, STRING_CONST(m_Compiler, XmlNs)))
            {
                PrefixDeclaration = Semantics::InterpretXmlPrefix(m_XmlSemantics->m_Prefixes, LocalName, NameSearchDonotResolveImportsAlias, m_SourceFile);
                if (PrefixDeclaration)
                {
                    XmlNamespaceDeclaration = PrefixDeclaration->PAlias()->GetSymbol()->PXmlNamespaceDeclaration();

                    if (XmlNamespaceDeclaration->IsImported())
                    {
                        // This is a namespace in the literal that exactly matches an imported namespace, i.e. both its prefix and namespace match an
                        // Import <xmlns:prefix = "namespace"> declaration.  This attribute will be added when the imported namespace attribute list is
                        // processed so skip adding it here.

                        return NULL;
                    }
                }
            }
        }
    }

    ParseTree::ArgumentList *ArgList = new(m_TreeStorage) ParseTree::ArgumentList;
    ParseTree::Argument *Arg;

    Arg = new(m_TreeStorage) ParseTree::Argument;
    Arg->Value = PH.CreateBoundExpression(NameExpr);

    ArgList->Element = Arg;
    ArgList->Next = NULL;

    ParseTree::Expression *Value = PH.CreateBoundExpression(InterpretXmlExpression(Attribute->Value, ParseTree::Expression::XmlAttribute));

    if (Value)
    {
        ParseTree::ArgumentList *Content = new(m_TreeStorage) ParseTree::ArgumentList;
        Arg = new(m_TreeStorage) ParseTree::Argument;
        Content->Element = Arg;
        Content->Next = NULL;
        Arg->Value = Value;
        ArgList->Next = Content;
    }

    VSASSERT(!ArgList || ArgList->Element && ArgList->Element->Value, "Bad Xml Attribute Value List");

    if (Attribute->Value->Opcode == ParseTree::Expression::XmlEmbedded)
    {
        ILTree::Expression *Target = ReferToSymbol(
                                loc,
                                GetXmlHelperMethod(STRING_CONST(m_Compiler, XmlCreateAttributeMethod)),
                                chType_NONE,
                                NULL,
                                NULL,
                                m_XmlSemantics->m_Flags | ExprIsExplicitCallTarget);

        Result = BindArgsAndInterpretCallExpressionWithNoCopyOut(loc, Target, chType_NONE, ArgList, m_XmlSemantics->m_Flags, OvrldNoFlags, NULL);
    }
    else
    {
        //The constructor
        Result = m_XmlSemantics->CreateConstructedInstance(m_XmlSymbols.GetXAttribute(),
                loc,
                ArgList);
    }

    return Result;
}


ILTree::Expression *
Semantics::InterpretXmlAttributes(ParseTree::ExpressionList * Attributes, ILTree::SymbolReferenceExpression *Parent,
        ILTree::Expression **LastExpression)

{
    ULONG AttributeMark = m_XmlSemantics->m_AttributeMgr.GetMark();

    Location HiddenLoc = Location::GetHiddenLocation();

    ILTree::Expression *PrevExpression = NULL;
    ILTree::Expression *NextExpression = NULL;
    ILTree::Expression *Result = NULL;
    ILTree::Expression *AttributeExpr = NULL;
    ParseTree::ExpressionList *PrevAttributes = NULL;

    for (; Attributes; PrevAttributes = Attributes, Attributes = Attributes->Next)
    {
        ParseTree::Expression * Expr = Attributes->Element;
        ILTree::Expression *XNamespaceRef = NULL;
        bool IsAttrEmbeddedExpr = false;

        switch (Expr->Opcode)
        {
        case ParseTree::Expression::XmlAttribute:
            {
                ParseTree::XmlAttributeExpression *XmlAttribute = Expr->AsXmlAttribute();

                AttributeExpr  = InterpretXmlAttribute(XmlAttribute);
            }
            break;

        default:
            AttributeExpr = InterpretExpression(Expr, m_XmlSemantics->m_Flags);
            IsAttrEmbeddedExpr = true;
        }

        if (AttributeExpr && !IsBad(AttributeExpr))
        {
            ILTree::Expression *BoundArg =
                AllocateExpression(
                SX_ARG,
                TypeHelpers::GetVoidType(),
                AttributeExpr,
                HiddenLoc);

            ExpressionList *BoundList = AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                BoundArg,
                NULL,
                HiddenLoc);

            ILTree::Expression *Target =
                ReferToProcByName(
                HiddenLoc,
                m_XmlSymbols.GetXContainer(),
                STRING_CONST(m_Compiler, Add),
                Parent,
                m_XmlSemantics->m_Flags | ExprIsExplicitCallTarget);

            ILTree::Expression *CallAdd = InterpretCallExpressionWithNoCopyout(
                HiddenLoc,
                Target,
                chType_NONE,
                BoundList,
                false,
                m_XmlSemantics->m_Flags,
                // OvrldNoFlags,
                NULL);

            NextExpression =
                AllocateExpression(
                SX_SEQ_OP2,
                m_XmlSymbols.GetXElement(),
                CallAdd,
                NULL,
                HiddenLoc);

            if (PrevExpression)
            {
                PrevExpression->AsExpressionWithChildren().Right = NextExpression;
            }
            else
            {
                Result = NextExpression;
            }

            PrevExpression = NextExpression;
        }
    }

    if (LastExpression)
    {
        *LastExpression = PrevExpression;
    }

    m_XmlSemantics->m_AttributeMgr.ReleaseToMark(AttributeMark);

    return Result;
}


void
Semantics::ValidateEntityReference(ParseTree::XmlReferenceExpression *Reference)
{
    Location Loc = Reference->TextSpan;
    Loc.m_lBegColumn++;
    if (Reference->HasSemiColon)
    {
        Loc.m_lEndColumn--;
    }
    else
    {
        Location TextSpan = Reference->TextSpan;
        TextSpan.m_lEndColumn++;
        TextSpan.m_lBegColumn = TextSpan.m_lEndColumn;
        ReportSemanticError(ERRID_ExpectedSColon, TextSpan);
    }

    if (Reference->Name)
    {
        ValidateXmlName(Reference->Name, Loc, m_Errors);
    }

    if (!Reference->IsDefined)
    {
        ReportSemanticError(ERRID_XmlEntityReference, Reference->TextSpan);
    }
}


void
Semantics::ValidateXmlAttributeValue(ParseTree::Expression *Value, _In_z_ STRING *Text)
{
    if (!Value ||
        Value->Opcode == ParseTree::Expression::XmlCharData)
    {
        ParseTree::XmlCharDataExpression *CharData = Value->AsXmlCharData();

        size_t Length = StringPool::StringLength(Text);

        if (CharData->LengthInCharacters != Length ||
            wcsncmp(CharData->Value, Text, Length) != 0)
        {
            ReportSemanticError(ERRID_InvalidAttributeValue1, CharData->TextSpan, Text);
        }
    }
}

void
Semantics::ValidateXmlAttributeValue2(ParseTree::Expression *Value, _In_z_ STRING *Text1, _In_z_ STRING *Text2)
{
    if (!Value ||
        Value->Opcode == ParseTree::Expression::XmlCharData)
    {
        ParseTree::XmlCharDataExpression *CharData = Value->AsXmlCharData();

        size_t Length1 = StringPool::StringLength(Text1);
        size_t Length2 = StringPool::StringLength(Text2);


        if ((CharData->LengthInCharacters != Length1 || wcsncmp(CharData->Value, Text1, Length1) != 0) &&
            (CharData->LengthInCharacters != Length2 || wcsncmp(CharData->Value, Text2, Length2) != 0))
        {
            ReportSemanticError(ERRID_InvalidAttributeValue2, CharData->TextSpan,
                Text1,
                Text2);
        }
    }
}


//+--------------------------------------------------------------------------
//  Method:     Semantics.InterpretXmlElementExpression
//
//  Description:
//                      Creates an expression to execute XmlExpression,
//
//
//  Parameters:
//      [Xml]   --  Expression evaluating to an XLinq object.
//
//  History:    06-27-2005  Microsoft    Added
//---------------------------------------------------------------------------
ILTree::Expression *
Semantics::InterpretXmlElement
(
    ParseTree::XmlElementExpression * XmlElement
)
{
#if IDE    
    if (CheckStop(this)) // if the foreground thread wants to stop the bkd, then let's not recur so IDE is more responsive
    {
        ReportSemanticError(ERRID_EvaluationAborted, XmlElement->TextSpan);
        return AllocateBadExpression(XmlElement->TextSpan);
    }
#endif IDE

    Location loc = XmlElement->TextSpan;
    ParserHelper PH(&m_TreeStorage, loc);

    LocalSymbol ElementReference(&m_XmlSemantics->m_XElementLocals, m_XmlSymbols.GetXElement());

    ULONG NamespacesAdded = AddInScopeXmlNamespaces(XmlElement->Attributes);

    if (!XmlElement->IsEmpty && !XmlElement->NamesMatch)
    {
        ReportSemanticError(ERRID_MissingXmlEndTag, XmlElement->Name->TextSpan);
        if (XmlElement->HasEndName)
        {
            if (XmlElement->Name->Opcode == ParseTree::Expression::XmlName)
            {
                ParseTree::XmlNameExpression *Name = XmlElement->Name->AsXmlName();
                ReportSemanticError(ERRID_MismatchedXmlEndTag, XmlElement->EndNameTextSpan,
                    Name->Prefix.Name,
                    StringPool::StringLength(Name->Prefix.Name) != 0 ? L":" : L"",
                    Name->LocalName.Name);
            }
            else
            {
                ReportSemanticError(ERRID_MismatchedXmlEndTag, XmlElement->EndNameTextSpan,
                    L"",
                    L"",
                    L"");
            }
        }
    }

    Location TextSpan;

    if (XmlElement->HasWSBeforeName)
    {
        TextSpan = XmlElement->TextSpan;
        TextSpan.m_lEndLine = TextSpan.m_lBegLine;
        TextSpan.m_lBegColumn++;
        TextSpan.m_lEndColumn = TextSpan.m_lBegColumn;
        ReportSemanticError(ERRID_IllegalXmlWhiteSpace, TextSpan);
    }

    if (XmlElement->HasWSBeforeEndName)
    {
        TextSpan = XmlElement->TextSpan;
        TextSpan.m_lBegLine += XmlElement->BeginEndElementLoc.Line;
        TextSpan.m_lEndLine = TextSpan.m_lBegLine;
        TextSpan.m_lBegColumn = XmlElement->BeginEndElementLoc.Column + 2;
        TextSpan.m_lEndColumn = TextSpan.m_lBegColumn;
        ReportSemanticError(ERRID_IllegalXmlWhiteSpace, TextSpan);
    }

    ParseTree::ArgumentList *ArgList = new(m_TreeStorage) ParseTree::ArgumentList;

    ParseTree::Argument *Arg = new(m_TreeStorage) ParseTree::Argument;

    BCSYM_Alias *ResolvedPrefix = NULL;

    ILTree::Expression *NameExpr = InterpretXmlName(XmlElement->Name, &ResolvedPrefix);

    m_XmlSemantics->EnsureImportedNamespaceAttribute(ResolvedPrefix);

    if (ResolvedPrefix)
    {
        // If the resolved prefix is an imported default namespace, i.e. xmlns:p="" then
        // ensure that there is a xmlns="" namespace created.

        STRING *Prefix = ResolvedPrefix->GetName();
        BCSYM_XmlNamespaceDeclaration *XmlNamespaceDeclaration = ResolvedPrefix->GetSymbol()->PXmlNamespaceDeclaration();

        if (XmlNamespaceDeclaration->IsImported())
        {
            STRING *Namespace = XmlNamespaceDeclaration->GetName();

            if (StringPool::StringLength(Namespace) == 0 && StringPool::StringLength(Prefix) != 0)
            {
                m_XmlSemantics->EnsureImportedNamespaceAttribute(Namespace, Namespace);
            }
        }
    }

    Arg->Value = PH.CreateBoundExpression(NameExpr);

    ArgList->Element = Arg;

    ArgList->Next = NULL;

    ILTree::Expression *Constructor = m_XmlSemantics->CreateConstructedInstance(m_XmlSymbols.GetXElement(),
        loc,
        ArgList);

    ILTree::Expression *Assignment = GenerateAssignment(
        loc,
        ElementReference,
        Constructor,
        false);

    ILTree::Expression *LastAttribute = NULL;
    ILTree::Expression *LastNamespace = NULL;
    ILTree::Expression *LastContent = NULL;

    ILTree::Expression *Attributes = InterpretXmlAttributes(XmlElement->Attributes, ElementReference, &LastAttribute);

    ILTree::Expression *Content = InterpretXmlContent(XmlElement, ElementReference, &LastContent);

    // Put Imported namespace declarations on the literal's root element
    if (m_XmlSemantics->m_ImportedNamespaceAttributes && XmlElement->Parent == NULL)
    {
        // Todo: Microsoft - For now put imported namespaces after attributes.  Consider putting them first.
        ILTree::Expression *Namespaces = InterpretXmlAttributes(m_XmlSemantics->m_ImportedNamespaceAttributes, ElementReference, &LastNamespace);

        if (LastAttribute)
        {
            LastAttribute->AsExpressionWithChildren().Right = Namespaces;
        }
        else
        {
            Attributes = Namespaces;
        }

        if (LastNamespace)
        {
            LastAttribute = LastNamespace;
        }
    }

    if (LastAttribute)
    {
        LastAttribute->AsExpressionWithChildren().Right = Content;
        Content = Attributes;
    }

    if (!LastContent)
    {
        LastContent = LastAttribute;
    }

    // Put runtime created namespace declarations on the root

    if (XmlElement->Parent == NULL && m_XmlSemantics->m_RootNamespaceAttributes)
    {
        // Check whether outer Xml exists, i.e. this xml is inside of an embedded expression
        // that is statically determined to be inside of the outer xml.  If so then don't add the
        // attributes here but let them bubble up.

        if (!m_XmlSemantics->m_Parent ||
            !m_XmlSemantics->m_Parent->m_InheritImportedNamespaces)
        {
            ILTree::Expression *Target =
                ReferToProcByName(
                loc,
                m_XmlSymbols.GetXContainer(),
                STRING_CONST(m_Compiler, Add),
                ElementReference,
                m_XmlSemantics->m_Flags | ExprIsExplicitCallTarget);

            ILTree::Expression *BoundArg =
                AllocateExpression(
                SX_ARG,
                TypeHelpers::GetVoidType(),
                m_XmlSemantics->m_RootNamespaceAttributes,
                m_XmlSemantics->m_RootNamespaceAttributes->Loc);

            ExpressionList *BoundList = AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                BoundArg,
                NULL,
                loc);

            ILTree::Expression *BoundExpr = InterpretCallExpressionWithNoCopyout(
                loc,
                Target,
                chType_NONE,
                BoundList,
                false,
                m_XmlSemantics->m_Flags,
                // OvrldNoFlags,
                NULL);

            ILTree::Expression *RuntimeImportedNamespaces = 
                AllocateExpression(
                SX_SEQ_OP2,
                m_XmlSymbols.GetXElement(),
                BoundExpr,
                ElementReference,
                loc);

            if (LastContent)
            {
                LastContent->AsExpressionWithChildren().Right = RuntimeImportedNamespaces;
            }
            else
            {
                Content = RuntimeImportedNamespaces;
            }
        }
    }

    ILTree::Expression *Result =
        AllocateExpression(
        SX_SEQ_OP2,
        m_XmlSymbols.GetXElement(),
        Assignment,
        Content,
        loc);

    SetFlag32(Result, SXF_SEQ_OP2_XML);

    if (NamespacesAdded)
        m_XmlSemantics->m_Prefixes = m_XmlSemantics->m_Prefixes->GetImmediateParent()->PHash();

    //The actual Call
    return Result;
}


ClassOrRecordType *
Semantics::EnsureXmlHelperClass
(
)
{
    ClassOrRecordType *XmlClass = NULL;

    if (m_SymbolsCreatedDuringInterpretation)
    {
        XmlClass = m_SymbolsCreatedDuringInterpretation->GetXmlHelperClass();
    }

    if (XmlClass == NULL)
    {
        XmlClass = m_XmlSymbols.GetXmlHelper();

        // If the Xml helper class is generated only as needed, then ensure it's generated as a transient symbol
        if (m_SymbolsCreatedDuringInterpretation && XmlClass && XmlClass->GetCompilerFile()->CompileAsNeeded())
        {
            m_SymbolsCreatedDuringInterpretation->SetXmlHelperClass(XmlClass);

            // Insert nested Closure class as transient symbol if any exist.
            BCSYM_Container *NestedTypes = XmlClass->GetNestedTypes();
            if (NestedTypes)
            {
                m_SymbolsCreatedDuringInterpretation->Insert(XmlClass->GetNestedTypes(), NULL);
            }
        }
    }

    return XmlClass;
}


Procedure *
Semantics::GetXmlHelperMethod
(
    _In_z_ STRING *MethodName
)
{
    ClassOrRecordType *XmlClass = EnsureXmlHelperClass();
    VSASSERT(XmlClass, "GetXmlHelperMethod should not be called unless it's guaranteed that XmlHelper has been generated.");
    return XmlClass->GetHash()->SimpleBind(MethodName)->PProc();
}


void
NormalizeAttributeValue(ParseTree::XmlCharDataExpression *XmlCharData, StringBuffer &Buffer)
{
    // Normalize attribute whitespace
    if (XmlCharData->LengthInCharacters == 1 && XmlCharData->Value[0] == '\n')
    {
        Buffer.AppendChar(L' ');
    }
    else
    {
        for (size_t i = 0; i < XmlCharData->LengthInCharacters; i++)
        {
            WCHAR c = XmlCharData->Value[i];
            if (c == '\t')
                Buffer.AppendChar(L' ');
            else
                Buffer.AppendChar(c);
        }
    }
}


bool
ValidateXmlName
(
    _In_z_ STRING *Name,
    const Location & TextSpan,
    ErrorTable *Errors
)
{
    RESID ErrId = 0;

    WCHAR *p = Name;
    if (!*p)
        return false;

    if (!XmlCharacter::isStartNameChar(*p))
    {
        ErrId = ERRID_IllegalXmlStartNameChar;
    }
    else
    {
        p++;
        while (*p)
        {
            if (!XmlCharacter::isNameChar(*p))
            {
                ErrId = ERRID_IllegalXmlNameChar;
                break;
            }
            else
            {
                p++;
            }
        }
    }

    if (ErrId)
    {
        if (Errors)
        {
            WCHAR BadHexValue[8];
            WCHAR BadChar[2];

            BadChar[0] = *p;
            BadChar[1] = '\0';

            // NULL Terminate the string if it overflows.
            if (_snwprintf_s(BadHexValue,
                             sizeof(BadHexValue) / sizeof(WCHAR) - 1,
                            _TRUNCATE, L"0x%x",
                            *p) < 0)
            {
                BadHexValue[sizeof(BadHexValue) / sizeof(WCHAR) - 1] = 0; 
            }

            Errors->CreateError(ErrId, (Location*)&TextSpan, BadChar, BadHexValue);
        }

        return false;
    }

    return true;
}

bool
ValidateXmlNamespace
(
    ParseTree::IdentifierDescriptor & Prefix,
    ILTree::StringConstantExpression *NamespaceExpr,
    ErrorTable * Errors,
    Compiler * pCompiler,
    bool AsImport
)
{
    return ValidateXmlNamespace(
        Prefix,
        NamespaceExpr->Spelling,
        NamespaceExpr->Length,
        NamespaceExpr->Loc,
        Errors,
        pCompiler,
        AsImport);
}

bool
ValidateXmlNamespace
(
    ParseTree::IdentifierDescriptor & Prefix,
    _In_opt_count_(LengthInCharacters) const WCHAR *Namespace,
    size_t LengthInCharacters,
    const Location &TextSpan,
    ErrorTable * Errors,
    Compiler * pCompiler,
    bool AsImport
)
{
    if (StringPool::IsEqualCaseSensitive(Prefix.Name, STRING_CONST(pCompiler, XmlNs)) ||
       (StringPool::IsEqualCaseSensitive(Prefix.Name, STRING_CONST(pCompiler, Xml)) &&
        wcscmp(Namespace, STRING_CONST(pCompiler, XmlNamespaceURI)) != 0))
    {
        if (Errors)
        {
            Errors->CreateError(ERRID_ReservedXmlPrefix, &Prefix.TextSpan, Prefix.Name);
        }
    }
    else if (StringPool::StringLength(Prefix.Name) != 0 && LengthInCharacters == 0)
    {
        if (AsImport)
        {
            return true;
        }

        if (Errors)
        {
            Errors->CreateError(ERRID_IllegalDefaultNamespace, &Prefix.TextSpan);
        }
    }
    else if (!StringPool::IsEqualCaseSensitive(Prefix.Name, STRING_CONST(pCompiler, Xml)) &&
             wcscmp(Namespace, STRING_CONST(pCompiler, XmlNamespaceURI)) == 0)
    {
        if (Errors)
        {
            Errors->CreateError(ERRID_ReservedXmlNamespace, const_cast<Location *>(&TextSpan), Prefix.Name, STRING_CONST(pCompiler, Xml));
        }
    }
    else if (!StringPool::IsEqualCaseSensitive(Prefix.Name, STRING_CONST(pCompiler, XmlNs)) &&
             wcscmp(Namespace, STRING_CONST(pCompiler, XmlNsNamespaceURI)) == 0)
    {
        if (Errors)
        {
            Errors->CreateError(ERRID_ReservedXmlNamespace, const_cast<Location *>(&TextSpan), Prefix.Name, STRING_CONST(pCompiler, XmlNs));
        }
    }
    else if (LengthInCharacters > MaxIdentifierLength)
    {
        if (Errors)
        {
            Errors->CreateError(ERRID_IdTooLong, const_cast<Location *>(&TextSpan));
        }
    }
    else
    {
        return true;
    }

    return false;
}

