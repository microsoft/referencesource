//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Header file for the XML CodeModel generator
//
//-------------------------------------------------------------------------------------------------

#pragma once

#define INITIAL_DEPTH_LEVEL     0   // Top level indentation level
#define INITIAL_RANK_LEVEL      0   // Top level rank for array initializations
                                    // This is the value we subtract from the actual
                                    // rank to figure out what rank we are at.

#define NO_BRACKETS             0   // No need for brackets to escape names.
#define USE_BRACKETS            1   // Use brackets to escape reserved Keyword, if needed.

#define NO_SOURCE_NAME          0   // No need to process "sourcename" attribute.
#define PROCESS_SOURCE_NAME     1   // Process "sourcename" attribute.

#define FLAT_DEPTH_LEVEL        INITIAL_DEPTH_LEVEL + 1 // No nesting, used by XMLRunTypeDeclarations()

#define NO_ENTITY_REF_CHECK     0   // Don't check for entity references
#define CHECK_ENTITY_REF        1   // Check for entity references

#define NO_SIGNATURE            0   // Don't use the object's signature to generate the key
#define USE_SIGNATURE           1   // Use the object's signature to generate the key

#define NON_NESTED_EXPRESSION   0   // Normal expression
#define NESTED_EXPRESSION       1   // An expression within an expression

#define NO_METHODS              0   // No methods processing
#define PROCESS_METHODS         1   // Process mehtod modies

#define PROCESS_ALL_DECLS       0   // Process all the declrations (normal case)
#define PROCESS_TYPES_ONLY      1   // Process only types in a flat list (no nesting)

#define PROPERTY_GET            0   // property
#define PROPERTY_SET            1   // by reference arg

#if DEBUG
#define DEBUG_NEW_LINE L"\n"
#else
#define DEBUG_NEW_LINE
#endif DEBUG

typedef unsigned __int64 NameFlags;

enum IdKind                     {ID, ParamID, RefID};

//****************************************************************************
// This class can hold one component of a dot qualified name. It stores a copy
// of the name passed in with or with out brackets, depending on the state of
// the IsBracketed parameter.
//****************************************************************************

class BracketedDotQualifiedName : ZeroInit<BracketedDotQualifiedName>
{
public:
    BracketedDotQualifiedName(
        Compiler * pCompiler,
        _In_z_ STRING * NewName,
        bool IsBracketed)
    {
        if (!NewName)
        {
            return;
        }

        size_t LengthOfString = StringPool::StringLength(NewName);
        size_t SizeOfBuffer = LengthOfString;
        if (IsBracketed)
        {
            SizeOfBuffer += 3;
        }
        else
        {
            SizeOfBuffer += 1;
        }
        Name = new (zeromemory) WCHAR[SizeOfBuffer];

        WCHAR *PartialName = Name;

        if (IsBracketed)
        {
            PartialName[0] = CHAR_OpenBracket;
            PartialName++;
            SizeOfBuffer--;
        }

        wcsncpy_s(PartialName, SizeOfBuffer, NewName, LengthOfString);
        PartialName += LengthOfString;

        if (IsBracketed)
        {
            PartialName[0] = CHAR_CloseBracket;
            PartialName++;
            SizeOfBuffer--;
        }

        PartialName[0] = UCH_NULL;
        Length = PartialName - Name;
    }

    ~BracketedDotQualifiedName() { if (Name) delete [] Name; }

    WCHAR *GetName() { return Name; }
    size_t GetLength() { return Length; }

protected:

    WCHAR *Name;
    size_t Length;
};

//****************************************************************************
// A stack of BracketedDotQualifiedName nodes. This is used to build a Dot
// qualified name from the parsetrees. When ever we visit a new container,
// we push the name of that container onto this stack (the name may or may not
// be bracketed), and when we need to generate the fully-qualified name,
// we can do that from this stack.
//****************************************************************************

class BracketedDotQualifiedNameStack : ZeroInit<BracketedDotQualifiedNameStack>
{
public:

    BracketedDotQualifiedNameStack(Compiler * pCompiler) :
        m_pCompiler(pCompiler)
    {
    }

    ~BracketedDotQualifiedNameStack()
    {
        VSASSERT(m_NamesStack.Empty(), L"BracketedDotQualifiedNameStack should be empty!");

        while (!m_NamesStack.Empty())
        {
            Pop();
        }
    }

    void Push(
        _In_z_ STRING * NewName,
        bool IsBracketed)
    {
        if (NewName)
        {
            BracketedDotQualifiedName *NewBracketedDotQualifiedNameNode = 
                new (zeromemory) BracketedDotQualifiedName(m_pCompiler, NewName, IsBracketed);

            m_NamesStack.Push(NewBracketedDotQualifiedNameNode);
        }
    }

    void Pop()
    {
        BracketedDotQualifiedName *PoppedNode = m_NamesStack.Top();

        VSASSERT((m_NamesStack.Empty() && !PoppedNode) || (!m_NamesStack.Empty() && PoppedNode),
            L"Inconsistent stack state!");

        if (PoppedNode)
        {
            m_NamesStack.Pop();
            delete PoppedNode;
        }
    }

    void GetBracketedDotQualifiedName(StringBuffer * pstrBuffer)
    {
        if (!pstrBuffer)
        {
            return;
        }

        for (ULONG i = 0; i < m_NamesStack.Count(); ++i)
        {
            BracketedDotQualifiedName *NthName = m_NamesStack.Element(i);

            VSASSERT(NthName, L"Inconsistent stack state!");

            if (NthName->GetLength() > 0)
            {
                pstrBuffer->AppendWithLength(NthName->GetName(), NthName->GetLength());

                if (i < m_NamesStack.Count() - 1)
                {
                    pstrBuffer->AppendChar(CHAR_Period);
                }
            }
        }
    }

protected:

    Stack<BracketedDotQualifiedName *> m_NamesStack;
    Compiler *m_pCompiler;
};

//****************************************************************************
// This class encapsulates the BracketedDotQualifiedNameStack class in that
// it provides a way to do a single push and a single pop when this object
// goes out of scope. It's a good idea to use this object instead of
// BracketedDotQualifiedNameStack because this object will remove whatever
// was pushed onto the stack when the function exits, without an explicit
// call to the stack Pop function.
//****************************************************************************

class SmartPushPopObject : ZeroInit<SmartPushPopObject>
{
public :
    SmartPushPopObject(BracketedDotQualifiedNameStack * pStackToUse) :
        m_pStackToUse(pStackToUse), 
        m_IsPushed(false)
    {
        VSASSERT(pStackToUse, L"NULL stack passed into SmartPushPopObject");
    }

    ~SmartPushPopObject()
    {
        if (m_IsPushed)
        {
            Pop();
        }
    }

    void Push(
        _In_z_ STRING * NewName,
        bool IsBracketed)
    {
        if (m_IsPushed)
        {
            VSFAIL(L"This is a single Push/Pop object, you can not use it for multiple Pushs/Pops");
        }
        else
        {
            if (m_pStackToUse)
            {
                m_pStackToUse->Push(NewName, IsBracketed);
                m_IsPushed = true;
            }
        }
    }

    void Pop()
    {
        if (m_IsPushed)
        {
            if (m_pStackToUse)
            {
                m_pStackToUse->Pop();
                m_IsPushed = false;
            }
        }
        else
        {
            VSFAIL(L"Pop without a Push!");
        }
    }

protected :
    BracketedDotQualifiedNameStack *m_pStackToUse;
    bool m_IsPushed;
};

//****************************************************************************
// Implements the XML generator for VB CodeModel
//****************************************************************************

class XMLGen : ZeroInit<XMLGen>
{
friend class CodeFile;

public:
    // Process declarations into XML
    static
    HRESULT XMLRunDeclarations(
        Compiler * pCompiler,
        SourceFile * pSourceFile,
        StringBuffer * pstrBuffer);

    // Process method bodies into XML
    static
    HRESULT XMLRunBodies(
        Compiler * pCompiler,
        SourceFile * pSourceFile,
        StringBuffer * pstrBuffer,
        BCSYM_Proc * pproc);

    // Checks for '<', '>', '&', '\'' and '"' and replaces them as necessary
    static
    void CheckForEntityReferences(
        _In_z_ PCWCH wszInString,
        _Deref_out_z_ WCHAR ** pwszOutString,
        bool fProcessMethods);


protected:

    // Constructor for calling helper methods
    XMLGen(
        Compiler * pCompiler,
        SourceFile * pSourceFile,
        CompilerProject * pCompilerProject,
        HRESULT * phr);

    // Constructor for declarations (statementblocks)
    XMLGen(
        Compiler * pCompiler,
        SourceFile * pSourceFile,
        ParseTree::BlockStatement * ptree,
        CompilerProject * pCompilerProject,
        StringBuffer * pstrBuffer,
        bool fProcessMethods,
        bool fTypesOnly,
        HRESULT * hr);

    // Constructor for statementlists
    XMLGen(
        Compiler * pCompiler,
        SourceFile * pSourceFile,
        ParseTree::StatementList * ptree,
        CompilerProject * pCompilerProject,
        StringBuffer * pstrBuffer,
        bool fProcessMethods,
        HRESULT * phr);

    // Constructor for a method body
    XMLGen(
        Compiler * pCompiler,
        SourceFile * pSourceFile,
        ILTree::ILNode * PILNode,
        CompilerProject * pCompilerProject,
        StringBuffer * pstrBuffer,
        BCSYM_Proc * pproc,
        HRESULT * hr);

    ~XMLGen();

    // Does nothing is we are not in debug mode, or asserts if we are on debug mode.
    HRESULT DebAssertHelper(_In_z_ PCWCH debAssertString);

#if DEBUG
    void RemoveBrackets(_Inout_z_ WCHAR *pText);
#endif DEBUG

    // Uses the new parse trees
    HRESULT GenerateStatementID(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        Location * pLoc = NULL);

    HRESULT GenerateStatementID(
        StringBuffer * psbBuffer,
        const Location &loc,
        IdKind KindOfIDToGenerate);

    // Retrun the hash for the passed in BCSYM, or NULL if none exist.
    BCSYM_Hash * ContextOf(BCSYM_NamedRoot * pContext);

    bool NeedsBrackets(BCSYM_NamedRoot * NameIsQuestion);

    // Dumps a reference ID for a given bound tree.
    void GenerateIDRef(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode);

    // Process the root namespace for the project
    HRESULT ProcessRootNameSpace(
        StringBuffer * pstrBuffer,
        _In_opt_z_ STRING * pRootNameSpaceName,
        ParseTree::StatementList * pStatementList,
        ULONG depthLevel);

    // Processes a qualified or a simple name given a ParseTree::Name
    HRESULT ProcessName(
        StringBuffer * pstrBuffer,
        ParseTree::Name * pname,
        ULONG depthLevel,
        bool UseBrackets);

    // Tries to find the BCSYM associated with a parsetree name or a WCHAR string
    HRESULT GetSymbolFromName(
        ParseTree::Name * pname,
        _In_opt_z_ WCHAR * wszInString,
        BCSYM ** ppbcsym,
        NameFlags Flags,
        _In_opt_z_ const WCHAR * pStringToAppend = NULL);

    // Dumps an attribute given that we have a ParseTree::Name
    HRESULT GenerateGenericAttributeFromName(
        StringBuffer * pstrBuffer,
        _In_opt_z_ PCWCH wszAttribName,
        ParseTree::Name * pname,
        bool UseBrackets);

    // Dumps an attribute
    HRESULT GenerateGenericAttribute(
        StringBuffer * pstrBuffer,
        _In_opt_z_ PCWCH wszAttribName,
        _In_opt_z_ PCWCH wszAttribValue,
        bool fEntityRefCheck,
        bool IsBracketed);

    // Generates a generic XML literal element with an arbitrary depth level.
    HRESULT GenerateLiteral(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Dumps a generic number
    HRESULT GenerateNumberLiteral(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Dumps an element
    void GenerateGenericElement(
        StringBuffer * pstrBuffer,
        _In_opt_z_ PCWCH wszElementName,
        _In_opt_z_ PCWCH wszElementValue,
        bool fEntityRefCheck,
        bool IsBracketed);

    // Return true if this a "Me" reference, or false if it is not
    bool IsThisReference(BCSYM_NamedRoot * pnamed);

    // Dumps an access string (public, private, ...)
    HRESULT GenerateAccess(
        StringBuffer * pstrBuffer,
        BCSYM_NamedRoot * pnamed);

    // A general way of dumps XML code with any level of indentation. Used all over.
    void AppendString(
        StringBuffer * pstrBuffer,
        _In_opt_z_ PCWCH wszBuffer,
        ULONG level,
        bool fEntityRefCheck = CHECK_ENTITY_REF);

    // A general way of dumps one character with any level of indentation.
    void AppendChar(
        StringBuffer * pstrBuffer,
        WCHAR wch,
        ULONG depthLevel,
        bool fEntityRefCheck = CHECK_ENTITY_REF);

    // Returns the type of a member.
    BCSYM * TypeOf(BCSYM_NamedRoot * psym);

    // Returns the span for Implements and inherits statements on a Class/Module
    void GetEndLocationForImplementsAndInherits(
        ParseTree::StatementList * pClassChildren,
        Location * pLoc);

    // Returns the span for Implements and handles statements on Method
    void GetEndLocationForImplementsAndHandles(
        ParseTree::MethodDeclarationStatement * pMethod,
        Location * pLoc);

    // Generates a fully qualified name using the BCSYM node. If we can't find the BCSYM, we revert
    // to dumping the shortname instead of the fullname (vivian's idea).
    HRESULT GenerateFullName(
        StringBuffer * pstrBuffer,
        BCSYM_NamedRoot * pBCSYM,
        bool fProcessSourceName);

    // Generates a line number attribute for designer's use.
    HRESULT GenerateLineNumber(
        StringBuffer * psbBuffer,
        unsigned LineNumber);

    // Generates a unique key for this named BCSYM. The key is based on the fully
    // qualified name of that symbol, so if the symbol moves around, the Id will
    // change, but the key should stay the same.
    HRESULT GenerateKey(
        StringBuffer * pstrBuffer,
        BCSYM_NamedRoot * pnamed,
        _In_opt_z_ WCHAR * inString = NULL);

    // Does some setup work before calling Generatekey
    HRESULT MetaGenerateKey(
        StringBuffer * pstrBuffer,
        BCSYM_NamedRoot * pnamed,
        bool fuseSig = NO_SIGNATURE);

    // Processes a linked list of statements
    HRESULT ProcessStatementList(
        StringBuffer * pstrBuffer,
        ParseTree::StatementList * ptreeList,
        ULONG depthLevel);

    // Processes a linked list of variables
    HRESULT ProcessVariableDeclaratorList(
        StringBuffer * pstrBuffer,
        ParseTree::VariableDeclaration * pDeclaration,
        ParseTree::DeclaratorList * ptreeList,
        ParseTree::Initializer * pinitializer,
        ULONG depthLevel);

    // Processes a linked list of locals.
    HRESULT XMLGen::ProcessLocalDeclarationList(
        StringBuffer * psbBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes a linked list of parameters
    HRESULT ProcessParameterList(
        StringBuffer * pstrBuffer,
        ParseTree::ParameterList * ptreeList,
        BCSYM_Proc * pProcSymbol,
        ULONG depthLevel);

    // Processes a linked list of Initializers
    HRESULT ProcessInitializerList(
        StringBuffer * pstrBuffer,
        ParseTree::InitializerList * ptreeList,
        BCSYM * ptype,
        ULONG rankDepth,
        ULONG depthLevel);

    // Processes a linked list of types
    HRESULT ProcessTypeList(
        StringBuffer * pstrBuffer,
        ParseTree::TypeList * ptreeList,
        ULONG depthLevel);

    // Processes a linked list Named Params for an attribute
    HRESULT ProcessAttributeArgumentList(
        StringBuffer * pstrBuffer,
        ParseTree::ArgumentList * ptreeList,
        ULONG depthLevel);

    // Processes a linked list of attributes
    HRESULT ProcessAttributeList(
        StringBuffer * pstrBuffer,
        ParseTree::AttributeSpecifierList * ptreeList,
        ULONG depthLevel);

    // Process declarations. Top level functions for running through all declarations in a tree
    HRESULT ProcessDeclarations(
        StringBuffer * pstrBuffer,
        ParseTree::BlockStatement * pnamespaceTree,
        ULONG depthLevel);

    // Takes care of handeling returns and quoting the correct block when there is an error
    HRESULT ProcessBlock(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Main switch statment for calling up the right function that is responsible
    // for processing a blocks.
    HRESULT ProcessMethodBodyStatement(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process implements logic for methods
    HRESULT ProcessMethodImplements(
        StringBuffer * pstrBuffer,
        ParseTree::NameList * ----lements,
        ULONG depthLevel);

    // Process implements logic for events
    HRESULT ProcessEventImplements(
        StringBuffer * pstrBuffer,
        ParseTree::NameList * ----lements,
        ULONG depthLevel);

    // Process handles logic using a BCSYM_MethodImpl
    HRESULT ProcessMethodHandlesFromBCYM(
        StringBuffer * pstrBuffer,
        BCSYM_NamedRoot * pnamed,
        _In_z_ STRING * methodName,
        ULONG depthLevel);

    // Process handles logic using a ParseTree
    HRESULT ProcessMethodHandles(
        StringBuffer * pstrBuffer,
        ParseTree::NameList * phandles,
        ULONG depthLevel);

    // Sets the Method/Class/Struct/Interface nesting counters as needed
    void SetBlockNesting(long value);

    // Process method bodies. Top level function for processing the body of a function
    HRESULT ProcessMethodBody(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Wraps the invocation of ProcessDeclStatementParseTree() so that we can handel errors in a consistent way.
    HRESULT MetaProcessParseTree(
        StringBuffer * pstrBuffer,
        ParseTree::StatementList * pstatementList,
        ULONG depthLevel);

    // Process a BilTree. Top level for processing diffenrent nodes
    HRESULT ProcessDeclStatementParseTree(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process anything we don't understand, or anything that has a parse error in it.
    HRESULT ProcessQuote(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ParseTree::StatementList * pstatementList,
        ULONG depthLevel);

    // Process Comments from BilTree or a ParseTree::Statement
    HRESULT ProcessComment(
        StringBuffer * pstrBuffer,
        ParseTree::Comment * pcomment,
        ULONG depthLevel);

    // Process inherits statments
    HRESULT ProcessInheritsStmt(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process Implements statments
    HRESULT ProcessImplementsList(
        StringBuffer * pstrBuffer,
        ParseTree::StatementList * plist,
        BCSYM_Implements * ----lementsList,
        ULONG depthLevel);

    // Process Error statment
    HRESULT ProcessErrorStatement(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process OnError statment
    HRESULT ProcessOnError(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Prcesses an external method Declaration statement (dll declared)
    HRESULT ProcessExternalProcedureDeclaration(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process a Resume statment
    HRESULT ProcessResumeStatment(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a local Dim statement
    HRESULT ProcessLocalDimStmt(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a dim statement
    HRESULT ProcessFieldDeclaration(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process the initialization expression. We handle the three types of initializers here: Expression, Aggregate, and Deferred.
    HRESULT ProcessInitializer(
        StringBuffer * pstrBuffer,
        ParseTree::Initializer * pinitializer,
        BCSYM * ptype,
        ULONG rankDepth,
        ULONG depthLevel);

    // Process variable declaration statment
    HRESULT ProcessVariableDeclaration(
        StringBuffer * pstrBuffer,
        ParseTree::VariableDeclaration * pDeclaration,
        ParseTree::Declarator * pvarDecl,
        ParseTree::Initializer * pinitializer,
        ULONG depthLevel);

    // Process any unkown type from a ParseTree::Type
    HRESULT ProcessUnresolvedType(
        StringBuffer * psbBuffer,
        ParseTree::Type * ptype,
        Location * PhysicalTypeReferenceLocation,
        ULONG depthLevel);

    // Process a specific type. This is the core of the type switch statement, and it handles all types
    HRESULT ProcessTypeStatement(
        StringBuffer * pstrBuffer,
        ParseTree::Type * ptype,
        ParseTree::Type * pElementArrayType,
        ULONG depthLevel);

    // Process a type char
    HRESULT ProcessTypeChar(
        StringBuffer * pstrBuffer,
        typeChars typeCharacter,
        ULONG depthLevel);

    // Prcesses the array declarations of all types given a BCSYM_ArrayType
    HRESULT ProcessBCSYMArrayType(
        StringBuffer * pstrBuffer,
        BCSYM * parrayType,
        ParseTree::Type * ParseTreeType,
        ULONG rankDepth,
        Location * PhysicalTypeReferenceLocation,
        ULONG depthLevel);

    // Prcesses an array type
    HRESULT ProcessArrayType(
        StringBuffer * psbBuffer,
        ParseTree::ArrayType * pArrayInfo,
        ParseTree::Type * pArrayElementType,
        BCSYM * psymType,
        Location * PhysicalTypeReferenceLocation,
        ULONG depthLevel);

    // Process a BCSYM Container type
    HRESULT ProcessBCSYMContainerType(
        StringBuffer * pstrBuffer,
        BCSYM_Container * pcontainerType,
        BCSYM_GenericBinding * pGenericTypeBinding,
        Location * PhysicalTypeReferenceLocation,
        ULONG depthLevel);

    // Process any type, given a BCSYM for the thingy declared of that type.
    HRESULT ProcessTypeFromBCSYM(
        StringBuffer * pstrBuffer,
        BCSYM * psymType,
        ParseTree::Type * ParseTreeType,
        Location * PhysicalTypeReferenceLocation,
        ULONG depthLevel,
        BCSYM_GenericBinding * pGenericBinding = NULL);

    // Process Parameters
    HRESULT ProcessParameterDeclaration(
        StringBuffer * pstrBuffer,
        ParseTree::Parameter * pparameter,
        BCSYM_Param * pParameterSymbol,
        ULONG depthLevel);

    // Either quotes or processes a method body
    HRESULT ExecuteMethodBody(
        StringBuffer * pstrBuffer,
        BCSYM_Proc * pmethod,
        ULONG depthLevel);

    // Process a signature
    HRESULT ProcessSignature(
        StringBuffer * pstrBuffer,
        ParseTree::MethodSignatureStatement * pmethodSig,
        BCSYM_Proc * pProcSymbol,
        ULONG depthLevel,
        BCSYM * ptype);

    // Process Procedures
    HRESULT ProcessProcedureDeclaration(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process a constructor declaration
    HRESULT ProcessConstructorDeclaration(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process the signature of a property. This only works for properties since we have have to mock around
    // with them.
    HRESULT ProcessPropertySignature(
        StringBuffer * pstrBuffer,
        ULONG depthLevel,
        bool fKind);

    // Process attributes that are common among Properties and it's Getter and Setter.
    HRESULT ProcessCommonPropertyAttributes(StringBuffer * psbBuffer);

    // Process property accessors Get
    HRESULT ProcessPropertyGet(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process property accessors Set
    HRESULT ProcessPropertySet(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Makeup a property Get for properties declared inside an interface.
    HRESULT FakePropertyGet(
        StringBuffer * psbBuffer,
        ULONG depthLevel);

    // Makeup a property Set for properties declared inside an interface.
    HRESULT FakePropertySet(
        StringBuffer * psbBuffer,
        ULONG depthLevel);

    // Process property declarations.
    HRESULT ProcessPropertyDeclaration(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process a the attributeparam and its expression
    HRESULT ProcessAttributeProperty(
        StringBuffer * pstrBuffer,
        ParseTree::Argument * pargument,
        ULONG depthLevel);

    // Process attribute section
    HRESULT ProcessAttribute(
        StringBuffer * pstrBuffer,
        ParseTree::Attribute * pattribute,
        ULONG depthLevel);

    // Process Imports
    HRESULT ProcessImportsStmt(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process Classes
    HRESULT ProcessClass(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process a qualified namespace like a.b.c. We end up building several nested namespaces, one for a, one for b, and one for c.
    HRESULT ProcessQualifiedNameSpace(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ParseTree::Name * pname,
        ULONG * numberOfNestedNamespaces);

    // Process a namespace
    HRESULT ProcessNameSpace(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process Structures
    HRESULT ProcessStructure(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process Interfaces
    HRESULT ProcessInterface(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process an enum declaration
    HRESULT ProcessEnumDeclaration(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process an event declaration
    HRESULT ProcessEventDeclaration(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process an enum number
    HRESULT ProcessEnumVariable(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process delegate declarations.
    HRESULT ProcessDelegateDeclaration(
        StringBuffer * pstrBuffer,
        ParseTree::Statement * pstatement,
        ULONG depthLevel);

    // Process a With block
    HRESULT ProcessWithBlock(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a Try block
    HRESULT ProcessTryBlock(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a Catch block
    HRESULT ProcessCatchBlock(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process an Select block
    HRESULT ProcessSelectBlock(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process an Case statement
    HRESULT ProcessCaseStatement(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a Special case. this used to Model VB's select/case statments.
    void ProcessSpecial(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a statment inside a method body
    HRESULT ProcessExpressionStmt(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process an Goto statement
    HRESULT ProcessGotoStmt(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a Label statement
    HRESULT ProcessLabelStmt(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a GetType construct
    HRESULT ProcessGetType(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a late bound statement
    void ProcessLateStmt(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a Continue Statement
    HRESULT ProcessContinueStatement(
        StringBuffer * psbBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a Break statement
    HRESULT ProcessExitStatment(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a Exit Sub / Return statement
    HRESULT ProcessReturnStatment(
        StringBuffer * psbBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process an End statement
    HRESULT ProcessEndStatment(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a Stop statement
    HRESULT ProcessStopStatment(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a For Each ... Next block
    HRESULT ProcessForEachLoop(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a For ... Next block
    HRESULT ProcessForLoop(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a While ... Wend block
    HRESULT ProcessWhileLoop(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a do ... {While | Until} block
    HRESULT ProcessDoLoop(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes an If block. This method calls the If/ElseIf/Else method below to
    HRESULT ProcessIfBlock(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process an If statement
    HRESULT ProcessIf_Else(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a NameRef kind (local, field, property, method, or unkown). This one takes a BCSYM_NamedRoot *
    HRESULT ProcessNameRefKind(
        StringBuffer * pstrBuffer,
        BCSYM_NamedRoot * panmed);

    // Process a NameRef kind (local, field, property, method, or unkown. This one takes a PBILTREE
    HRESULT ProcessNameRefKind(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode);

    HRESULT GenerateDottedNameRef(
        StringBuffer * pstrBuffer,
        _In_count_x_(index + 1) STRING ** rgpstrQualifiedName,
        long index,
        ULONG depthLevel);

    // Process a name reference from a method BCSYM
    HRESULT ProcessNameRefFromBCSYM(
        StringBuffer * pstrBuffer,
        BCSYM_NamedRoot * pNameRefSymbol,
        BCSYM_GenericBinding * pGenericBinding,
        ULONG depthLevel);

    // Process a name reference from a biltree
    HRESULT ProcessNameRef(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process a raise event statment
    HRESULT ProcessRaiseEvent(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes a PropertySet (with no indexers) as if it was an assignement.
    HRESULT ProcessPropertySetAccess(
        StringBuffer * psbBuffer,
        ILTree::PILNode PILNode,
        bool IsWithEventsVariable,
        ULONG depthLevel);

    // Processes a PropertySet with indexers as a PropertyAccess .
    HRESULT ProcessPropertySetIndexedAccess(
        StringBuffer * psbBuffer,
        ILTree::PILNode PILNode,
        bool IsWithEventsVariable,
        ULONG depthLevel);

    // Processes a PropertyGet as if it was an assignement.
    HRESULT ProcessPropertyGetAccess(
        StringBuffer * psbBuffer,
        ILTree::PILNode PILNode,
        bool IsWithEventsVariable,
        ULONG depthLevel);

    // Process a call statment (constructor calls included here)
    HRESULT ProcessCallBlock(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process an assignment statment
    HRESULT ProcessAssignment(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Process an Expression from a ParseTree::Expression
    HRESULT ProcessParseExpressionTree(
        StringBuffer * pstrBuffer,
        ParseTree::Expression * pexpression,
        BCSYM * ptype,
        ULONG depthLevel);

    // Process an Expression
    HRESULT ProcessExpression(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel,
        bool fSubExpression = NON_NESTED_EXPRESSION);

    // Processes array index operator
    HRESULT ProcessIndexOperator(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes an VB binary operator
    HRESULT ProcessBinaryOperation(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes New operator
    HRESULT ProcessNew(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes New operator
    HRESULT ProcessNewClass(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes New Array
    HRESULT ProcessNewArray(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes an array creation and initialization list. This is applies to the following VB
    // code: Me.Controls.AddRange(New System.Windows.Forms.Control() {Me.button1})
    HRESULT ProcessCreateArray(
        StringBuffer * pstrBuffer,
        ILTree::PILNode NewArrayRootNode,
        BCSYM * ArrayType,
        ULONG depthLevel);

    // Processes an array creation and initialization list. This applies to the following VB
    // code: Me.Controls.AddRange({Me.button1})
    HRESULT ProcessArrayLiteral(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes an array literal
    HRESULT ProcessArrayLiteral(
        StringBuffer * pstrBuffer,
        ILTree::ArrayLiteralExpression & ArrayLiteral,
		unsigned currentRank,
		ILTree::ExpressionWithChildren * Elements,
        ULONG depthLevel);

    // Processes New delegate
    HRESULT ProcessNewDelegate(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Not sure what this is exactly yet!
    HRESULT ProcessCoerceArg(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes a cast operator
    HRESULT ProcessCast(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes an VB unary operator
    HRESULT ProcessUnaryOperation(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Processes args to procs
    HRESULT ProcessMethodArguments(
        StringBuffer * pstrBuffer,
        ILTree::PILNode PILNode,
        ULONG depthLevel);

    // Get the text for the file. Creates it if necessary
    HRESULT GetTextFile(Text ** ppTextFile);

    // Helper method to detect WithEvents variables
    static bool IsWithEventsVariableAccessor(_In_ BCSYM_Proc* pnamedProc);

    // Data members
    SourceFile *m_pSourceFile;              // Actual sourcefile we are using
    BCSYM_Proc *m_pproc;                    // Proc to generate XML for (if any)
    CompilerProject *m_pCompilerProject;    // Compiler project
    Compiler *m_pCompiler;                  // compiler


    // DO NOT USE THIS DIRECTLY -- Go through GetTextFile()
    Text *m_pTextFile;                      // We use this to get text from the sourcefile

    BCSYM_NamedRoot *m_pPropertySymbol;     // BCSYM for at most one property at a time

    ParseTree::PropertyStatement *m_pPropertyStatement; // The ParseTree statement for the property
    ParseTree::Type *m_pPropertyType;                   // Return type for the property
    ParseTree::ParameterList *m_pPropertyParams;        // Param list for the prop
    ParseTree::NameList *m_pPropertyImplements;         // List of implements for the property

    BCSYM_NamedRoot *m_pCurrentContext;     // BCSYM for the current context we are processing

    // The differnet types of block nestings.
    unsigned m_BN_WITH;

    bool m_fShouldQuote;                    // Indicates if we found a tree that should be quoted
    bool m_fProcessMethods;                 // Indicates if we found a tree that should be quoted
    bool m_fProcessTypesOnly;               // Indicates if we are fired up from XMLRunTypeDeclarations or not
    bool m_fUseTrueQualifiedName;           // Indicates if we are should use Integer or System.Int32 (same thing for other types)

    BracketedDotQualifiedNameStack m_NameStack; // A stack of bracketed Dot qualified names to use for generating "sourcename" attribute
};
