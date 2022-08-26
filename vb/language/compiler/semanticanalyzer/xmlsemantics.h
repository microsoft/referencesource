//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Interface to VB xml semantic analysis.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class XmlAttributeManager
{
private:
    struct XmlQName
    {
        STRING *Namespace;
        STRING *LocalPart;
    };

public:
    // Construction.
    XmlAttributeManager(Compiler *pCompiler)
    {

    }

    // Makes sure that this goes away.
    ~XmlAttributeManager()
    {
    }

    void Add(_In_z_ STRING *Namespace, _In_z_ STRING *LocalPart);

    bool Find(_In_z_ STRING *Namespace, _In_z_ STRING *LocalPart);

    ULONG GetMark() {return m_Names.Count();}

    void ReleaseToMark(ULONG Mark)
    {
        VSASSERT(Mark <= m_Names.Count(), "Out of range");
        if (Mark < m_Names.Count())
            m_Names.Shrink(m_Names.Count() - Mark);
    }

private:
    DynamicArray<XmlQName> m_Names;
};

typedef unsigned __int64 ExpressionFlags;

class XmlNamesCache : ZeroInit<XmlNamesCache>
{
    Symbols *m_SymbolCreator;   // Symbol factory from which Xml symbols will be created
    Scope *m_XmlNamespaces;     // Hash of all cached namespaces
    TransientSymbolStore *m_SymbolsCreatedDuringInterpretation;

public:
    XmlNamesCache()
    {
    }

    void Initialize(_In_ Symbols *SymbolCreator, _In_ TransientSymbolStore *SymbolsCreatedDuringInterpretation)
    {
        m_SymbolCreator = SymbolCreator;
        m_SymbolsCreatedDuringInterpretation = SymbolsCreatedDuringInterpretation;
        m_XmlNamespaces = m_SymbolsCreatedDuringInterpretation ? m_SymbolsCreatedDuringInterpretation->GetXmlNamespaces() : NULL;
    }

    Scope * GetXmlNamespaces()
    {
        return m_XmlNamespaces;
    }

    BCSYM_XmlNamespace * GetXmlNamespace(_In_z_ STRING *Namespace, CompilerFile *OwningFile, CompilerProject *Project)
    {
        if (!m_XmlNamespaces)
        {
            m_XmlNamespaces = m_SymbolCreator->GetHashTable(NULL, NULL, true, 1, NULL);

            if (m_SymbolsCreatedDuringInterpretation)
            {
                // If Xml names were created during interpretation, then store them in transient symbol store for next time InterpretXmlExpression is called
                m_SymbolsCreatedDuringInterpretation->SetXmlNamespaces(m_XmlNamespaces);
            }
        }

        BCSYM_XmlNamespace * NamespaceSymbol = m_XmlNamespaces->SimpleBind(Namespace)->PXmlNamespace();
        if (!NamespaceSymbol)
        {
            NamespaceSymbol = m_SymbolCreator->GetXmlNamespace(Namespace, OwningFile, Project);
            Symbols::AddSymbolToHash(m_XmlNamespaces, NamespaceSymbol, true, false, false);
        }

        return NamespaceSymbol;
    }

#if IDE 
    BCSYM_XmlName * GetXmlName(BCSYM_XmlNamespace *NamespaceSymbol, _In_z_ STRING *LocalName)
    {
        return m_SymbolCreator->GetXmlName(NamespaceSymbol, LocalName);
    }

    BCSYM_XmlName * GetXmlName(_In_z_ STRING *Namespace, _In_z_ STRING *LocalName, CompilerFile *OwningFile, CompilerProject *Project)
    {
        return GetXmlName(GetXmlNamespace(Namespace, OwningFile, Project), LocalName);
    }
#endif
};

class XmlLocalSymbolMgr
{
public:
    XmlLocalSymbolMgr(Semantics *Analyzer);

    ILTree::SymbolReferenceExpression * AllocateLocal(Type *pType);

    void FreeLocal()
    {
        m_NextFreeLocal--;
    }
private:
    Semantics *m_Analyzer;
    DynamicArray<ILTree::SymbolReferenceExpression *> m_Locals;
    ULONG  m_NextFreeLocal;
};

class XmlSymbols : ZeroInit<XmlSymbols>
{
public:
    XmlSymbols()
    {
    }

    void SetContext(CompilerProject *Project, CompilerHost *CompilerHost);
    void SetContext(Scope *Scope, Compiler *Compiler, CompilerHost *CompilerHost);

    ClassOrRecordType *GetXObject()
    {
        return EnsureLinqClass(m_XObject, STRING_CONST_ComXmlObject);
    }
    ClassOrRecordType *GetXContainer()
    {
        return EnsureLinqClass(m_XContainer, STRING_CONST_ComXmlContainer);
    }
    ClassOrRecordType *GetXDocument()
    {
        return EnsureLinqClass(m_XDocument, STRING_CONST_ComXmlDocument);
    }
    ClassOrRecordType *GetXElement()
    {
        return EnsureLinqClass(m_XElement, STRING_CONST_ComXmlElement);
    }
    ClassOrRecordType *GetXAttribute()
    {
        return EnsureLinqClass(m_XAttribute, STRING_CONST_ComXmlAttribute);
    }
    ClassOrRecordType *GetXNamespace()
    {
        return EnsureLinqClass(m_XNamespace, STRING_CONST_ComXmlNamespace);
    }
    ClassOrRecordType *GetXName()
    {
        return EnsureLinqClass(m_XName, STRING_CONST_ComXmlName);
    }
    ClassOrRecordType *GetXProcessingInstruction()
    {
        return EnsureLinqClass(m_XProcessingInstruction, STRING_CONST_ComXmlProcessingInstruction);
    }
    ClassOrRecordType *GetXComment()
    {
        return EnsureLinqClass(m_XComment, STRING_CONST_ComXmlComment);
    }
    ClassOrRecordType *GetXCData()
    {
        return EnsureLinqClass(m_XCData, STRING_CONST_ComXmlCData);
    }
    ArrayType *GetXNamespaceArray(Symbols *symbolCreator)
    {
        return EnsureLinqArrayClass(m_XNamespaceArray, symbolCreator, GetXNamespace());
    }
    BCSYM_GenericBinding *GetXAttributeList(Symbols *symbolCreator)
    {
        return EnsureLinqListClass(m_XAttributeList, symbolCreator, GetXAttribute());
    }
    ClassOrRecordType *GetXmlExtensions()
    {
        return EnsureLinqClass(m_XmlExtensions, STRING_CONST_Extensions);
    }
    ClassOrRecordType *GetXmlHelper();
    Procedure *GetValueProc()
    {
        return EnsureXmlHelperProc(m_Value, STRING_CONST_Value);
    }

private:
    ArrayType *EnsureLinqArrayClass(ArrayType *&ClassStorage, Symbols *symbolCreator, ClassOrRecordType *Type);
    BCSYM_GenericBinding *EnsureLinqListClass(BCSYM_GenericBinding *&ClassStorage, Symbols *symbolCreator, ClassOrRecordType *Type);
    ClassOrRecordType *EnsureLinqClass(ClassOrRecordType *&ClassStorage, STRING_CONSTANTS ClassNameConst);
    Procedure *EnsureXmlHelperProc(Procedure *&ProcStorage, STRING_CONSTANTS ProcNameConst);
    Type *GetType(_In_z_ STRING *FullyQualifiedName);
    void Initialize(CompilerProject *Project, Scope *Scope, Compiler *Compiler, CompilerHost *CompilerHost);

#if IDE
    ULONG m_projectBoundChangeIndex;
#endif

    CompilerProject   *m_Project;
    Scope               *m_Scope;
    Compiler            *m_Compiler;
    CompilerHost        *m_CompilerHost;
    FX::FXSymbolProvider*m_SymbolProvider;
    ClassOrRecordType   *m_XObject;
    ClassOrRecordType   *m_XContainer;
    ClassOrRecordType   *m_XDocument;
    ClassOrRecordType   *m_XElement;
    ClassOrRecordType   *m_XAttribute;
    ClassOrRecordType   *m_XNamespace;
    ClassOrRecordType   *m_XName;
    ClassOrRecordType   *m_XProcessingInstruction;
    ClassOrRecordType   *m_XComment;
    ClassOrRecordType   *m_XCData;
    BCSYM_GenericBinding *m_XAttributeList;
    ArrayType           *m_XNamespaceArray;
    ClassOrRecordType   *m_XmlExtensions;
    ClassOrRecordType   *m_XmlHelper;
    Procedure           *m_Value;
};

class XmlSemantics
{
public:
    XmlSemantics(XmlSemantics *Parent, Semantics *Analyzer, ExpressionFlags Flags );
    ~XmlSemantics();
    ILTree::Expression *CreateConstructedInstance(Type *ElementType, const Location &loc, ParseTree::ArgumentList *ArgList);
    bool EnsureImportedNamespaceAttribute(BCSYM_Alias *PrefixAlias);
    bool EnsureImportedNamespaceAttribute(_In_z_ STRING *PrefixString, _In_z_ STRING *NamespaceString);
    ParseTree::XmlAttributeExpression *FindImportedNamespaceAttribute(_In_z_ STRING *Prefix);
    ParseTree::XmlAttributeExpression *CreateImportedNamespaceAttribute(_In_z_ STRING *PrefixString, _In_z_ STRING *NamespaceString);
    void AddImportedNamespaceAttribute(ParseTree::XmlAttributeExpression *Attribute);

    bool AreImportedNamespacesDefined();
    bool IsImportedNamespaceUsed(BCSYM_Alias *PrefixAlias);
    bool IsImportedNamespaceUsed(_In_z_ STRING *PrefixString, _In_opt_z_ STRING *NamespaceString);

    ParseTree::Expression *TraceLinqExpression(ParseTree::Expression * pExpr);
    ParseTree::Expression *TraceSingleInitializer(ParseTree::InitializerList * pInitList);
    ParseTree::Expression *TraceInitializer(ParseTree::Initializer * pInit);

    XmlSemantics *m_Parent;
    Semantics *m_Analyzer;
    XmlAttributeManager m_AttributeMgr;
    ParseTree::ExpressionList *m_ImportedNamespaceAttributes;
    ParseTree::IdentifierDescriptor m_XmlNsPrefix;
    ExpressionFlags m_Flags;
    Scope * m_Prefixes;             // In-scope prefixes
    XmlNamesCache m_XmlNames;       // Cache of all Xml namespaces, names, prefixes created during Xml compilation
    ILTree::Expression *m_RootNamespaceAttributes;
    BCSYM_NamedRoot *m_EmptyPrefixAlias;
    ParseTree::StringLiteralExpression *m_EmptyNamespace;
    XmlLocalSymbolMgr m_XElementLocals;
    XmlLocalSymbolMgr m_XDocumentLocals;
    bool m_InheritImportedNamespaces;
};

class LocalSymbol
{
public:
    LocalSymbol(_In_ XmlLocalSymbolMgr *pLocalSymbolMgr, Type *pType) : m_pLocalSymbolMgr(pLocalSymbolMgr)
    {
        m_pVariableReference = m_pLocalSymbolMgr->AllocateLocal(pType);
    }

    ~LocalSymbol()
    {
        m_pLocalSymbolMgr->FreeLocal();
    }

    operator ILTree::SymbolReferenceExpression *() const
    {
        return m_pVariableReference;
    }

    XmlLocalSymbolMgr *m_pLocalSymbolMgr;
    ILTree::SymbolReferenceExpression *m_pVariableReference;
};


void
NormalizeAttributeValue
(
    ParseTree::XmlCharDataExpression *XmlCharData,
    StringBuffer &Buffer
);


bool
ValidateXmlName
(
    _In_z_ STRING *Name,
    const Location & TextSpan,
    ErrorTable *Errors
);

bool
ValidateXmlNamespace
(
    ParseTree::IdentifierDescriptor & Prefix,
    ILTree::StringConstantExpression *NamespaceExpr,
    ErrorTable * Errors,
    Compiler * pCompiler,
    bool AsImport = false
);

bool
ValidateXmlNamespace
(
    ParseTree::IdentifierDescriptor & Prefix,
    _In_opt_count_(LengthInCharacters) const WCHAR *Namespace,
    size_t LengthInCharacters,
    const Location & Textspan,
    ErrorTable * Errors,
    Compiler * pCompiler,
    bool AsImport = false
);

inline bool
IsXmlNamespace
(
    Symbol * SymbolToTest
)
{
    return SymbolToTest->IsXmlNamespace();
}
