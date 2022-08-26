//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  The ParserHelper Class is designed to create parse tree elements during
//  the semantic phase of the compiler. Creating the Parse tree is much easier
//  than the Symbol tree and the final IL code is the same.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

ParseTree::CollectionInitializerExpression * 
ParserHelper::CreateCollectionInitializer
(
    ParseTree::NewExpression * pNewExpression, 
    ParseTree::PunctuatorLocation fromLocation, 
    ParseTree::BracedInitializerList * pInitializer
)
{
    ParseTree::CollectionInitializerExpression * pRet = new (*m_TreeStorage) ParseTree::CollectionInitializerExpression;
    
    pRet->Opcode = ParseTree::Expression::CollectionInitializer;
    pRet->NewExpression = pNewExpression;
    pRet->From = fromLocation;
    pRet->Initializer = pInitializer; 
    pRet->TextSpan = m_TextSpan;
    return pRet;
    
}

//-------------------------------------------------------------------------------------------------
//
//  Creates a constant integer literal
//
ParseTree::IntegralLiteralExpression * ParserHelper::CreateIntConst(
    int val) // The value
{
    ParseTree::IntegralLiteralExpression * IntConst = new(*m_TreeStorage) ParseTree::IntegralLiteralExpression;
    IntConst->TextSpan = m_TextSpan;
    IntConst->Opcode = ParseTree::Expression::IntegralLiteral;
    IntConst->Value = val;
    IntConst->Base = ParseTree::IntegralLiteralExpression::Decimal;
    IntConst->TypeCharacter = chType_NONE;

    return IntConst;
}

//-------------------------------------------------------------------------------------------------
//
//  Creates a constant boolean literal
//
ParseTree::BooleanLiteralExpression * ParserHelper::CreateBoolConst(
    bool val) // The value
{
    ParseTree::BooleanLiteralExpression * BoolConst = new(*m_TreeStorage) ParseTree::BooleanLiteralExpression;
    BoolConst->TextSpan = m_TextSpan;
    BoolConst->Opcode = ParseTree::Expression::BooleanLiteral;
    BoolConst->Value = val;

    return BoolConst;
}

//-------------------------------------------------------------------------------------------------
//
//  Creates a constant string literal
//
ParseTree::StringLiteralExpression * ParserHelper::CreateStringConst(
    _In_z_ STRING * Value) // The string value this represents
{
#pragma prefast(suppress: 26018, "StringPool::StringLength does indicate the buffer size")
    return CreateStringConst(Value, StringPool::StringLength(Value));
}

//-------------------------------------------------------------------------------------------------
//
//  Creates a constant string literal
//
ParseTree::StringLiteralExpression * ParserHelper::CreateStringConst(
    // The string value this represents
    // Caller is responsible for allocating this so that it lives as long as the expression itself
    _In_count_(Length + 1) _Pre_z_ WCHAR * Value,
    // The length of the string value
    size_t Length)
{
    VSASSERT(Value != NULL, "This string is invalid");
    if (Value == NULL)
        return NULL;

    ParseTree::StringLiteralExpression * StringConst = new (*m_TreeStorage) ParseTree::StringLiteralExpression;
    StringConst->TextSpan = m_TextSpan;
    StringConst->Opcode = ParseTree::Expression::StringLiteral;

    StringConst->Value = Value;
    StringConst->LengthInCharacters = Length;

    return StringConst;
}

//-------------------------------------------------------------------------------------------------
//
// Create a constant Nothing literal
//
ParseTree::Expression * ParserHelper::CreateNothingConst()
{
    ParseTree::Expression *NothingConst = new (*m_TreeStorage) ParseTree::Expression;
    NothingConst->TextSpan = m_TextSpan;
    NothingConst->Opcode = ParseTree::Expression::Nothing;
    return NothingConst;
}

//-------------------------------------------------------------------------------------------------
//
// Create a Me reference
//
ParseTree::Expression * ParserHelper::CreateMeReference()
{
    ParseTree::Expression *MeRef = new (*m_TreeStorage) ParseTree::Expression;
    MeRef->TextSpan = m_TextSpan;
    MeRef->Opcode = ParseTree::Expression::Me;
    return MeRef;
}

//-------------------------------------------------------------------------------------------------
//
// Create a MyBase reference
//
ParseTree::Expression * ParserHelper::CreateMyBaseReference()
{
    ParseTree::Expression *MyBaseRef = new (*m_TreeStorage) ParseTree::Expression;
    MyBaseRef->TextSpan = m_TextSpan;
    MyBaseRef->Opcode = ParseTree::Expression::MyBase;
    return MyBaseRef;
}

//---------------------------------------------------------------------------
//
//  Create a MyClass reference
//
ParseTree::Expression * ParserHelper::CreateMyClassReference()
{
    ParseTree::Expression *MyClassRef = new (*m_TreeStorage) ParseTree::Expression;
    MyClassRef->TextSpan = m_TextSpan;
    MyClassRef->Opcode = ParseTree::Expression::MyClass;
    return MyClassRef;
}

ParseTree::Expression * ParserHelper::CreateGroupReference(Location & TextSpan)
{
    ParseTree::Expression *GroupRef = new (*m_TreeStorage) ParseTree::Expression;
    GroupRef->TextSpan = TextSpan;
    GroupRef->Opcode = ParseTree::Expression::GroupRef;
    return GroupRef;
}

//---------------------------------------------------------------------------
//
//  Create a CType/DirectCast/TryCast operator
//
ParseTree::Expression * ParserHelper::CreateConversion(
      _In_opt_ ParseTree::Expression * value,
      _In_opt_ ParseTree::Type * targetType,
      ParseTree::Expression::Opcodes conversionKind)
{
    ASSERT(conversionKind == ParseTree::Expression::Conversion ||
           conversionKind == ParseTree::Expression::DirectCast ||
           conversionKind == ParseTree::Expression::TryCast,
           "Unexpected conversion kind");

    ParseTree::ConversionExpression *ctype= new (*m_TreeStorage) ParseTree::ConversionExpression;
    ctype->Value = value;
    ctype->TargetType = targetType;
    ctype->HasIs = false;
    ctype->Opcode = conversionKind;
    ctype->TextSpan = m_TextSpan;
    ctype->Comma.Column = m_TextSpan.m_lBegColumn;
    ctype->Comma.Line = m_TextSpan.m_lBegLine;
    return ctype;
}


//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateNewArray
//
//  Description:    Creates an Array Representation. This just starts the creation
//                      process, the elements of the array need to be added using th
//                      AddElementInitializer method.
//
//  Parameters:
//      [BaseType]  --  The base type of the array.
//
//  Returns [ParseTree::NewArrayInitializerExpression]:
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::NewArrayInitializerExpression *
ParserHelper::CreateNewArray
(
    _In_opt_ ParseTree::Type * BaseType
)
{
    ParseTree::ArrayType * ArrayType = new (*m_TreeStorage) ParseTree::ArrayType;
    ArrayType->TextSpan = m_TextSpan;
    ArrayType->Opcode = ParseTree::Type::ArrayWithoutSizes;
    ArrayType->Rank = 1;
    ArrayType->ElementType = BaseType;

    ParseTree::BracedInitializerList * InitialValues = new (*m_TreeStorage) ParseTree::BracedInitializerList;
    InitialValues->TextSpan = m_TextSpan;
    InitialValues->InitialValues = NULL;     //To be filled later
    InitialValues->RBracePresent=true;

    ParseTree::NewArrayInitializerExpression * TheObject = new (*m_TreeStorage) ParseTree::NewArrayInitializerExpression;
    TheObject->TextSpan = m_TextSpan;
    TheObject->Opcode = ParseTree::Expression::NewArrayInitializer;
    TheObject->ArrayType = ArrayType;
    TheObject->Elements = InitialValues;

    return TheObject;
}

static void
AddInitializer
(
    _Inout_opt_ ParseTree::BracedInitializerList * ElementsList,
    _In_ ParseTree::InitializerList * NewListElement
)
{
    //This should have been created by CreateArray:
    VSASSERT(ElementsList != NULL, "No initializer!");
    if (ElementsList == NULL)
        return;

    //Add the new element to the list
    if (ElementsList->InitialValues == NULL) {
        //If this list is emty add as first element
        ElementsList->InitialValues = NewListElement;
    }
    else {
        //Otherwise, add to the end of this list
        ParseTree::InitializerList * LastElement = ElementsList->InitialValues;
        while (LastElement->Next != NULL) {
            LastElement = LastElement->Next;
        }
        LastElement->Next = NewListElement;
    }
}


//+--------------------------------------------------------------------------
//  Method:     ParserHelper.AddElementInitializer
//
//  Description:    Adds elements to an array created with CreateArray.
//                      These elements must be derrived types of the base
//                      tpye specified in CreateArray.
//
//  Parameters:
//      [TheArray]  --  The base array
//      [NewValue]  --  A new value to add to the end of the list
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
void
ParserHelper::AddElementInitializer
(
    _In_opt_ ParseTree::ArrayInitializerExpression * TheArray,
    _In_opt_ ParseTree::Expression * NewValue
)
{

    //This should have been created by CreateArray:
    VSASSERT(TheArray != NULL, "The array needs to be created first");
    VSASSERT(NewValue != NULL, "The new value needs to be created first");
    if (TheArray == NULL || NewValue == NULL)
        return;

    //Create a new list element and assign the value to it
    ParseTree::InitializerList * NewListElement = new (*m_TreeStorage) ParseTree::InitializerList;
    NewListElement->TextSpan = m_TextSpan;
    NewListElement->Element = new (*m_TreeStorage) ParseTree::ExpressionInitializer;
    NewListElement->Element->Opcode = ParseTree::Initializer::Expression;
    NewListElement->Element->AsExpression()->Value = NewValue;

    AddInitializer( TheArray->Elements, NewListElement);
}


ParseTree::NewObjectInitializerExpression *
ParserHelper::CreateNewObjectInitializer
(
    _In_opt_ ParseTree::NewExpression * newExpression,
    ParseTree::BracedInitializerList *initialValues,
    _In_ const Location & textSpan
)
{
    ParseTree::NewObjectInitializerExpression * initializer = new (*m_TreeStorage) ParseTree::NewObjectInitializerExpression;
    initializer->Opcode = ParseTree::Expression::NewObjectInitializer;
    initializer->NewExpression = newExpression;
    initializer->InitialValues = initialValues;
    initializer->TextSpan = textSpan;

    return initializer;
}

ParseTree::BracedInitializerList *
ParserHelper::CreateBracedInitializerList
(
    _In_ ParseTree::InitializerList *initialValues,
    _In_ const Location & textSpan
)
{
    ParseTree::BracedInitializerList * initializer = new (*m_TreeStorage) ParseTree::BracedInitializerList;
    initializer->TextSpan = textSpan;
    initializer->InitialValues = initialValues;
    initializer->RBracePresent=true;

    return initializer;
}

ParseTree::NewObjectInitializerExpression *
ParserHelper::CreateAnonymousTypeInitializer
(
    _In_ ParseTree::InitializerList *initialValues,
    _In_ const Location & textSpan,
    bool NoWithScope,
    bool QueryErrorMode
)
{
    ParseTree::NewObjectInitializerExpression * result = CreateNewObjectInitializer(NULL,CreateBracedInitializerList(initialValues, textSpan), textSpan);
    result->NoWithScope = NoWithScope;
    result->QueryErrorMode = QueryErrorMode;

    return result;
}

ParseTree::InitializerList *
ParserHelper::CreateInitializerListFromExpression
(
    _In_ ParseTree::Expression * NewValue,
    bool FieldIsKey
)
{
    //Create a new list element and assign the value to it
    ParseTree::InitializerList * NewListElement = new (*m_TreeStorage) ParseTree::InitializerList;

    ChangeToExpressionInitializer(NewListElement, NewValue, FieldIsKey);

    return NewListElement;
}

ParseTree::InitializerList *
ParserHelper::AddExpressionInitializer
(
    ParseTree::NewObjectInitializerExpression * objInitializer,
    _In_ ParseTree::Expression * NewValue,
    bool FieldIsKey
)
{
    //Create a new list element and assign the value to it
    ParseTree::InitializerList * NewListElement = CreateInitializerListFromExpression(NewValue, FieldIsKey);

    AddInitializer(objInitializer->InitialValues, NewListElement);

    return NewListElement;
}

ParseTree::InitializerList *
ParserHelper::AppendInitializerList
(
    ParseTree::NewObjectInitializerExpression * objInitializer,
    _In_ ParseTree::InitializerList * ListToAppend
)
{
    AddInitializer(objInitializer->InitialValues, ListToAppend);
    return ListToAppend;
}

void
ParserHelper::ChangeToExpressionInitializer
(
    ParseTree::InitializerList * NewListElement,
    _In_ ParseTree::Expression * NewValue,
    bool FieldIsKey
)
{
    NewListElement->TextSpan = NewValue->TextSpan;
    NewListElement->Element = new (*m_TreeStorage) ParseTree::ExpressionInitializer;
    NewListElement->Element->Opcode = ParseTree::Initializer::Expression;
    NewListElement->Element->AsExpression()->Value = NewValue;
    NewListElement->Element->FieldIsKey = FieldIsKey;
}


ParseTree::InitializerList *
ParserHelper::CreateInitializerListFromExpression
(
    ParseTree::IdentifierDescriptor * Name,
    ParseTree::Expression * NewValue,
    bool FieldIsKey
)
{
    return CreateInitializerListFromExpression(Name,NewValue,NewValue->TextSpan,FieldIsKey);
}

ParseTree::InitializerList *
ParserHelper::CreateInitializerListFromExpression
(
    ParseTree::IdentifierDescriptor * Name,
    ParseTree::Expression * NewValue,
    const Location & TextSpan,
    bool FieldIsKey
)
{
    //Create a new list element and assign the value to it
    ParseTree::InitializerList * NewListElement = new (*m_TreeStorage) ParseTree::InitializerList;
    NewListElement->TextSpan = TextSpan;

    ParseTree::AssignmentInitializer * AssignmentInitializer = new (*m_TreeStorage) ParseTree::AssignmentInitializer;
    NewListElement->Element = AssignmentInitializer;

    AssignmentInitializer->Opcode = ParseTree::Initializer::Assignment;
    AssignmentInitializer->TextSpan = TextSpan;
    AssignmentInitializer->Name = *Name;
    AssignmentInitializer->Initializer = new (*m_TreeStorage) ParseTree::ExpressionInitializer;
    AssignmentInitializer->Initializer->Opcode = ParseTree::Initializer::Expression;
    AssignmentInitializer->Initializer->AsExpression()->Value = NewValue;
    AssignmentInitializer->FieldIsKey = FieldIsKey;

    return NewListElement;
}


//+--------------------------------------------------------------------------
//  Method:     ParserHelper.AddAssignmentInitializer
//
//  Description:    Adds assignment initializers to an object created with
//                      CreateNewAggregateObject.
//
//  Parameters:
//      [TheObject]  --  The base object
//      [Name]       --  The name of the field being initializer
//      [NewValue]   --  The value of the field
//---------------------------------------------------------------------------
ParseTree::InitializerList *
ParserHelper::AddAssignmentInitializer
(
    _In_ ParseTree::ObjectInitializerExpression * TheObject,
    _In_ ParseTree::IdentifierDescriptor * Name,
    _In_ ParseTree::Expression * NewValue,
    _In_ const Location & TextSpan,
    bool FieldIsKey
)
{
    AssertIfNull(TheObject);
    AssertIfNull(Name);
    AssertIfNull(NewValue);

    //Create a new list element and assign the value to it
    ParseTree::InitializerList * NewListElement = CreateInitializerListFromExpression(Name,NewValue,TextSpan,FieldIsKey);

    AddInitializer(TheObject->InitialValues, NewListElement);

    return NewListElement;
}

ParseTree::InitializerList *
ParserHelper::AddAssignmentInitializer
(
    _In_ ParseTree::ObjectInitializerExpression * TheObject,
    _In_ ParseTree::IdentifierDescriptor * Name,
    _In_ ParseTree::Expression * NewValue,
    bool FieldIsKey
)
{
    return AddAssignmentInitializer(TheObject,Name,NewValue,m_TextSpan,FieldIsKey);
}


//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateTypeList
//
//  Description:    This creates a TypeList out of a list of types.
//
//  Parameters:
//      [TypeCount]     --  The number of types listed. Must be > 0
//      [...]           --  A list of Types to be added
//
//  Returns [ParseTree::TypeList]:
//
//---------------------------------------------------------------------------
ParseTree::TypeList *
ParserHelper::CreateTypeList
(
    unsigned TypeCount,
    ...
)
{
    va_list TypeList;
    va_start(TypeList, TypeCount);

    ParseTree::TypeList * FirstType = CreateTypeList(TypeCount, TypeList);

    va_end(TypeList);

    return FirstType;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateTypeList
//
//  Description:    This creates a TypeList out of a list of types.
//
//  Parameters:
//      [TypeCount]     --  The number of types listed. Must be > 0
//      [...]           --  A list of Types to be added
//
//  Returns [ParseTree::TypeList]:
//
//---------------------------------------------------------------------------
ParseTree::TypeList *
ParserHelper::CreateTypeList
(
    unsigned TypeCount,
    va_list TypeList
)
{
    ParseTree::TypeList * FirstType = NULL;
    ParseTree::TypeList * LastType = NULL;

    ParseTree::TypeList * TypeListElement = NULL;
    for (unsigned i = 0; i < TypeCount; i++)
    {
        ParseTree::Type * TypeValue = va_arg(TypeList, ParseTree::Type *);

        TypeListElement = CreateTypeList(TypeValue);

        if (FirstType == NULL) {
            FirstType = TypeListElement;
        }

        if (LastType != NULL) {
            LastType->Next = TypeListElement;
        }

        LastType = TypeListElement;
    }

    return FirstType;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateOneTypeList
//
//  Description:    This creates a TypeList Element for one type
//
//  Parameters:
//      [Type]          --  The type list to be wrapped
//
//  Returns [ParseTree::TypeList]:
//
//---------------------------------------------------------------------------
ParseTree::TypeList *
ParserHelper::CreateTypeList
(
    ParseTree::Type *Type
)
{
    ParseTree::TypeList * TypeListElement = new (*m_TreeStorage) ParseTree::TypeList;
    TypeListElement->TextSpan = m_TextSpan;
    TypeListElement->Element = Type;

    return TypeListElement;
}

ParseTree::ArgumentList *
ParserHelper::CreateBoundArgList    
(
    ILTree::ExpressionWithChildren * pArguments
)
{
    ParseTree::ArgumentList * pRet = 
        CreateArgList
        (
            CreateBoundExpression
            (
                pArguments->Left
            )    
        );

    ParseTree::ArgumentList * pCurrent = pRet;

    while (pArguments->Right && pArguments->Right->bilop == SX_LIST)
    {
        pArguments = &pArguments->Right->AsExpressionWithChildren();
        
        pCurrent = 
            AddArgument
            (
                pCurrent, 
                CreateBoundExpression
                (
                    pArguments->Left
                )
            );
    }

    return pRet;
}


//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateArgList
//
//  Description:    This creates an ArgumentList out of a list of expressions.
//
//  Parameters:
//      [TextSpan]  --  Source location of this expression
//      [ArgCount]  --  The number of arguments listed. Must be > 0
//           [...]  --  A list of Expressions to be added, NULL values are converted
//                      to Nothing literals
//
//  Returns [ParseTree::ArgumentList]:
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::ArgumentList *
ParserHelper::CreateArgList
(
    const Location & TextSpan,
    unsigned ArgCount,
    ...
)
{
    va_list ArgExpressionList;
    va_start(ArgExpressionList, ArgCount);
    return CreateArgList(TextSpan, ArgCount, ArgExpressionList);
}

ParseTree::ArgumentList *
ParserHelper::CreateArgList
(
    unsigned ArgCount,
    ...
)
{
    va_list ArgExpressionList;
    va_start(ArgExpressionList, ArgCount);
    return CreateArgList(m_TextSpan, ArgCount, ArgExpressionList);
}

ParseTree::ArgumentList *
ParserHelper::CreateArgList
(
    const Location & TextSpan,
    unsigned ArgCount,
    va_list ArgExpressionList
)
{
    ParseTree::ArgumentList * FirstArg = NULL;
    ParseTree::ArgumentList * ArgListElement = NULL;

    for (unsigned i = 0; i < ArgCount; i++)
    {
        ParseTree::Expression * ArgExpression = va_arg(ArgExpressionList, ParseTree::Expression *);
        if (ArgExpression == NULL)
            ArgExpression = CreateNothingConst();

        ArgListElement = AddArgument( ArgListElement, ArgExpression, TextSpan );

        if (FirstArg == NULL) {
            FirstArg = ArgListElement;
        }
    }

    va_end(ArgExpressionList);

    return FirstArg;
}

//+--------------------------------------------------------------------------
// This override allows you to create a single arg list and then add to it.
//---------------------------------------------------------------------------

ParseTree::ArgumentList *
ParserHelper::CreateArgList( ParseTree::Expression * Argument )
{
    ThrowIfNull( Argument );

    return AddArgument(NULL, Argument, m_TextSpan);
}

// Returns the new element added.

ParseTree::ArgumentList *
ParserHelper::AddArgument(
    ParseTree::ArgumentList * List,
    ParseTree::Expression * Argument,
    const Location & TextSpan
    )
{
    ThrowIfNull( Argument );

    ParseTree::ArgumentList * Element = new (*m_TreeStorage) ParseTree::ArgumentList;
    Element->TextSpan = TextSpan;
    Element->Element = new (*m_TreeStorage) ParseTree::Argument;
    Element->Element->TextSpan = TextSpan;
    Element->Element->Value = Argument;

#if IDE 
    if (Argument->Opcode == ParseTree::Expression::Name &&
        Argument->AsName()->Name.Name)
    {
        Element->Element->ValueWidth = StrLen(Argument->AsName()->Name.Name);
    }
#endif

    if( List != NULL )
    {
        List->Next = Element;
    }
    return( Element );
}

// Returns the new element added.

ParseTree::ArgumentList *
ParserHelper::AddArgument( ParseTree::ArgumentList * List, ParseTree::Expression * Argument )
{
    ThrowIfNull( Argument );
    return( AddArgument( List, Argument, m_TextSpan ) );
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateName
//
//  Description:    Creates a Name which can be Qualified based on a list of strings.
//                      Unlike a NameExpression, this is used to reference a non-value type
//                      like a class name.
//                      Example: System.Data.DataTable or DataTable
//
//  Parameters:
//      [NameCount]     --  1 if this is a simple name (like "DataTable")
//                                          >1 if this is a qualified name (like "System.Data.DataTable")
//          [...]   --  A list of STRING * for each part of the name
//
//  Returns [ParseTree::Name]:
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::Name *
ParserHelper::CreateName
(
    unsigned NameCount,
    ...
)
{
    va_list NameList;
    va_start(NameList, NameCount);

    ParseTree::Name * Result = CreateNameEx(NameCount, NameList);

    va_end(NameList);
    return Result;
}

ParseTree::Name *
ParserHelper::CreateName
(
    ParseTree::Name * BaseName,
    unsigned NameCount,
    ...
)
{
    va_list NameList;
    va_start(NameList, NameCount);

    ParseTree::Name * Result = CreateNameEx(NameCount, NameList, BaseName);

    va_end(NameList);
    return Result;
}

ParseTree::Name *
ParserHelper::CreateGlobalNameSpaceName()
{
    ParseTree::Name *Result = new (*m_TreeStorage) ParseTree::Name;
    Result->Opcode = ParseTree::Name::GlobalNameSpace;
    Result->TextSpan = m_TextSpan;

    return Result;
}

ParseTree::Name *
ParserHelper::CreateNameWithTypeArguments
(
    ParseTree::TypeList *TypeArgs,
    unsigned NameCount,
    ...
)
{
    va_list NameList;
    va_start(NameList, NameCount);

    ParseTree::Name * Result = CreateNameEx(NameCount, NameList, NULL, TypeArgs);

    va_end(NameList);
    return Result;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateNameEx
//
//  Description:    Creates a Name which can be Qualified based on a list of strings.
//                      Unlike a NameExpression, this is used to reference a non-value type
//                      like a class name.
//                      Example: System.Data.DataTable or DataTable
//
//  Parameters:
//      [NameCount] --  1 if this is a simple name (like "DataTable")
//                      >1 if this is a qualified name (like "System.Data.DataTable")
//      [NameList]  --  A list of STRING * for each part of the name (in list format)
//      [BaseName]  --  The base name of the expression (like the namespace)
//      [TypeArgs]  --  The type arguments for the name.
//
//  Returns [ParseTree::Name]:
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::Name *
ParserHelper::CreateNameEx
(
    unsigned NameCount,
    va_list NameList,
    _In_opt_ ParseTree::Name * BaseName,
    ParseTree::TypeList * TypeArgs
)
{
    VSASSERT(NameCount > 0, "Why no name?");
    VSASSERT(!BaseName || (NameCount > 1) || !TypeArgs, "Can't add type args to an existing name!");
    if (NameCount < 1)
        return NULL;

    int StartArg = 0;   //Only parse first element specially in BaseName == NULL

    if (BaseName == NULL) {
        STRING * BaseNameString = va_arg(NameList, STRING *);

        // The first part
        if (NameCount > 1 || !TypeArgs)
        {
            BaseName = new (*m_TreeStorage) ParseTree::SimpleName;
            BaseName->TextSpan = m_TextSpan;
            BaseName->Opcode = ParseTree::Name::Simple;
            BaseName->AsSimple()->ID.Name = BaseNameString;
        }
        else
        {
            BaseName = new (*m_TreeStorage) ParseTree::SimpleWithArgumentsName;
            BaseName->TextSpan = m_TextSpan;
            BaseName->Opcode = ParseTree::Name::SimpleWithArguments;
            BaseName->AsSimpleWithArguments()->ID.Name = BaseNameString;
            BaseName->AsSimpleWithArguments()->ID.TextSpan = m_TextSpan;
            BaseName->AsSimpleWithArguments()->Arguments.Opcode = ParseTree::GenericArguments::WithTypes;
            BaseName->AsSimpleWithArguments()->Arguments.Arguments = TypeArgs;
        }

        StartArg = 1;
    }

    ParseTree::Name * LastName = BaseName;
    for (unsigned i = StartArg; i < NameCount; i++)
    {
        STRING * NamePartString = va_arg(NameList, STRING *);

        if (i < (NameCount - 1) || !TypeArgs)
        {
            ParseTree::QualifiedName * NamePart = new (*m_TreeStorage) ParseTree::QualifiedName;
            NamePart->TextSpan = m_TextSpan;
            NamePart->Opcode = ParseTree::Name::Qualified;
            NamePart->Qualifier.Name = NamePartString;
            NamePart->Base = LastName;

            LastName = NamePart;
        }
        else
        {
            ParseTree::QualifiedWithArgumentsName * NamePart = new (*m_TreeStorage) ParseTree::QualifiedWithArgumentsName;
            NamePart->TextSpan = m_TextSpan;
            NamePart->Opcode = ParseTree::Name::QualifiedWithArguments;
            NamePart->Qualifier.Name = NamePartString;
            NamePart->Base = LastName;
            NamePart->Arguments.Opcode = ParseTree::GenericArguments::WithTypes;
            NamePart->Arguments.Arguments = TypeArgs;

            LastName = NamePart;
        }
    }

    return LastName;
}

ParseTree::Expression *
ParserHelper::CreateGlobalNameSpaceExpression()
{
    ParseTree::Expression * GlobalNs = new (*m_TreeStorage) ParseTree::Expression;
    GlobalNs->Opcode = ParseTree::Expression::GlobalNameSpace;
    GlobalNs->TextSpan = m_TextSpan;

    return GlobalNs;
}


//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateExpressionTreeNameExpression
//
//    Description:    Creates a Name Expression in the Expression Tree namespace
//
//  Parameters:
//      [Compiler]  --  The compiler, for string constants
//      [NameCount]     --  The number of args
//      [...]   --  A list of STRING * for each part
//
//  Returns [ParseTree::Expression]:
//---------------------------------------------------------------------------
ParseTree::Expression *
ParserHelper::CreateExpressionTreeNameExpression
(
    Compiler *Compiler,
    _In_z_ STRING *Name
)
{
    return CreateQualifiedNameExpression(
            CreateGlobalNameSpaceExpression(),
            5,
            STRING_CONST(Compiler, ComDomain),
            STRING_CONST(Compiler, ComLinqDomain),
            STRING_CONST(Compiler, ComExpressionsDomain),
            STRING_CONST(Compiler, ComExpression),
            Name);
}

ParseTree::NameExpression *
ParserHelper::CreateNameExpression
(
    _In_z_ STRING * Name,
    _In_ const Location & TextSpan
)
{
        ParseTree::NameExpression * BaseName = new (*m_TreeStorage) ParseTree::NameExpression;
        BaseName->TextSpan = TextSpan;
        BaseName->Opcode = ParseTree::Expression::Name;
        BaseName->Name.Name = Name;
        BaseName->Name.IsBracketed = true;
        BaseName->Name.IsNullable = false;
        BaseName->Name.TextSpan = TextSpan;

        return BaseName;
}

ParseTree::NameExpression *
ParserHelper::CreateNameExpression
(
    _In_z_ STRING * Name
)
{
    return CreateNameExpression(Name, m_TextSpan);
}

ParseTree::NameExpression *
ParserHelper::CreateNameExpression
(
    _In_ ParseTree::IdentifierDescriptor & Identifier
)
{
        ParseTree::NameExpression * BaseName = new (*m_TreeStorage) ParseTree::NameExpression;
        BaseName->TextSpan = Identifier.TextSpan;
        BaseName->Opcode = ParseTree::Expression::Name;
        BaseName->Name = Identifier;

        return BaseName;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateNameExpression
//
//  Description:    Creates a NameExpression which can be dot qualified.
//                      Unlike a NameExpression, this is used to reference a value like
//                      a variable or property.
//                      All quallified names will be Dot Qualified
//
//  Parameters:
//      [NameCount]     --  Number of string parts
//      [...]   --  A list of STRING * for each part
//
//  Returns [ParseTree::Expression]:
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::Expression *
ParserHelper::CreateNameExpression
(
    unsigned NameCount,
    ...
)
{
    va_list NameList;
    va_start(NameList, NameCount);

    ParseTree::Expression * Result = CreateNameExpressionEx(NameCount, NameList);

    va_end(NameList);
    return Result;
}


ParseTree::Expression *
ParserHelper::CreateQualifiedNameExpression
(
    ParseTree::Expression *Base,
    unsigned NameCount,
    ...
)
{
    va_list NameList;
    va_start(NameList, NameCount);

    ParseTree::Expression * Result = CreateNameExpressionEx(NameCount, NameList, Base);

    va_end(NameList);
    return Result;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateNameExpressionEx
//
//  Description:    Creates a NameExpression which can be dot qualified.
//                      Unlike a NameExpression, this is used to reference a value like
//                      a variable or property.
//                      All quallified names will be Dot Qualified
//
//  Parameters:
//      [NameCount]     --  The number of args
//      [NameList]  --  Number of string parts (in list format)
//      [BaseName]  --  The base name of the expression (like the namespace)
//
//  Returns [ParseTree::Expression]:
//
//  History:    2-18-2005   Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::Expression *
ParserHelper::CreateNameExpressionEx
(
    _In_ const Location &TextSpan,
    unsigned NameCount,
    va_list NameList,
    _In_opt_ ParseTree::Expression * BaseName
)
{
    VSASSERT(NameCount > 0, "Why no name?");
    if (NameCount < 1)
        return NULL;

    int StartArg = 0;   //Only parse first element specially in BaseName == NULL

    if (BaseName == NULL) {
        //The first part
        STRING * BaseNameString = va_arg(NameList, STRING *);
        BaseName = new (*m_TreeStorage) ParseTree::NameExpression;
        BaseName->TextSpan = TextSpan;
        BaseName->Opcode = ParseTree::Expression::Name;
        BaseName->AsName()->Name.Name = BaseNameString;
        BaseName->AsName()->Name.IsBracketed = true;
        BaseName->AsName()->Name.IsNullable = false;

        StartArg = 1;
    }

    ParseTree::Expression * LastName = BaseName;
    for (unsigned i = StartArg; i < NameCount; i++)
    {
        STRING * NamePartString = va_arg(NameList, STRING *);
        ParseTree::QualifiedExpression * NamePart = new (*m_TreeStorage) ParseTree::QualifiedExpression;
        NamePart->TextSpan = TextSpan;
        NamePart->Opcode = ParseTree::Expression::DotQualified;
        NamePart->Name = CreateNameExpression(1, NamePartString);
        NamePart->Base = LastName;

        LastName = NamePart;
    }

    return LastName;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateNameExpressionEx
//
//  Description:    Creates a NameExpression which can be dot qualified.
//                      Unlike a NameExpression, this is used to reference a value like
//                      a variable or property.
//                      All quallified names will be Dot Qualified
//
//  Parameters:
//      [NameCount]     --  The number of args
//      [NameList]  --  Number of string parts (in list format)
//      [BaseName]  --  The base name of the expression (like the namespace)
//
//  Returns [ParseTree::Expression]:
//
//  History:    2-18-2005   Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::Expression *
ParserHelper::CreateNameExpressionEx
(
    unsigned NameCount,
    va_list NameList,
    _In_opt_ ParseTree::Expression * BaseName
)
{
    return CreateNameExpressionEx(m_TextSpan, NameCount, NameList, BaseName);
}


ParseTree::GenericQualifiedExpression *
ParserHelper::CreateGenericQualifiedExpression
(
    _In_opt_ ParseTree::Expression *Base,
    ParseTree::TypeList *Args
)
{
    ParseTree::GenericQualifiedExpression *NewExpr = new (*m_TreeStorage) ParseTree::GenericQualifiedExpression();
    NewExpr->TextSpan = m_TextSpan;
    NewExpr->Opcode = ParseTree::Expression::GenericQualified;
    NewExpr->Base = Base;
    NewExpr->Arguments.Opcode = ParseTree::GenericArguments::WithTypes;
    NewExpr->Arguments.Arguments = Args;

    return NewExpr;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateBoundExpression
//
//  Description:    This is a wrapper for an already bound expression. This
//                      is used if we need to add an expression that has already been
//                      bound into the parse tree. This is very usefull for TableReference
//                      values in SQL which are evaluated before the SQL expression.
//
//  Parameters:
//      [BoundExpression]   --  A Symbol Tree Expression
//
//  Returns [ParseTree::AlreadyBoundExpression]:    A Parse Tree Expression
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::AlreadyBoundExpression *
ParserHelper::CreateBoundExpression
(
    ILTree::Expression * BoundExpression
)
{
    VSASSERT(BoundExpression != NULL, "This shouldn't be null");

    ParseTree::AlreadyBoundExpression * WrapperExpr = new (*m_TreeStorage) ParseTree::AlreadyBoundExpression;
    WrapperExpr->TextSpan = BoundExpression->Loc;
    WrapperExpr->Opcode = ParseTree::Expression::AlreadyBound;
    WrapperExpr->BoundExpression = BoundExpression;

    return WrapperExpr;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateBoundSymbol
//
//  Description:    Creates a new Bound Symbol Expression
//
//  Parameters:
//      [Symbol]  -- the expression returning the reference to the expression.
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  NOTE:
//      The CALLER is resposible for ensuring that the symbol to file references
//      are maintained for the decompilation dependencies
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::Expression *
ParserHelper::CreateBoundSymbol
(
    Symbol * Symbol
)
{
    // This function should be used for plain unqualified symbols, e.g. local variable references

    VSASSERT(Symbol != NULL, "This shouldn't be null");

    ParseTree::AlreadyBoundSymbolExpression * WrapperExpr = new (*m_TreeStorage) ParseTree::AlreadyBoundSymbolExpression;
    WrapperExpr->TextSpan = m_TextSpan;
    WrapperExpr->Opcode = ParseTree::Expression::AlreadyBoundSymbol;
    WrapperExpr->Symbol = Symbol;
	WrapperExpr->BaseReference = NULL;
	WrapperExpr->UseBaseReferenceTypeAsSymbolContainerType = false;
    return WrapperExpr;
}


ParseTree::Expression *
ParserHelper::CreateBoundMemberSymbol
(
    ParseTree::Expression * BaseReference,
    Symbol * Symbol
)
{
    // WARNING! This should only be used if "Symbol" is a member of "BaseReference" itself.
    // It does not work if Symbol is (say) present only on BaseReference's ancestor.
    //
    // EXPLANATION: When InterpretExpression tries to bind "BaseReference.Symbol", it ultimately
    // has to emit these things:
    //    PUSH BaseReference
    //    ACCESS BaseReferenceType.Symbol
    // This function creates an "AlreadyBoundSymbolExpression", configured in such a way that
    // InterpretExpression will CHANGE "CurrentSymbolType.Symbol" into "BaseReferenceType.Symbol".
    // For instance, even if Symbol had been looked-up without any generic type binding, the
    // result would pick up the generic type binding of BaseReference.
    //
    // This function only works if "Symbol" is a member of BaseReferenceType already.
    // If not, then it might or might not produce unverifiable code. See Dev11#194805 for example.

    VSASSERT(BaseReference != NULL, "This shouldn't be null");
    ParseTree::AlreadyBoundSymbolExpression * WrapperExpr = CreateBoundSymbol(Symbol)->AsAlreadyBoundSymbol();
    WrapperExpr->BaseReference = BaseReference;
    WrapperExpr->UseBaseReferenceTypeAsSymbolContainerType = true;
    return WrapperExpr;
}


ParseTree::Expression *
ParserHelper::CreateBoundSymbolOnNewInstance
(
    ParseTree::Expression * BaseReference,
    Symbol * Symbol
)
{
    // WARNING! This function should only be used if "Symbol" already has suitable binding.
    //
    // EXPLANATION: When InterpretExpression tries to bind "BaseReference.Symbol", it ultimately
    // has to emit these things:
    //    PUSH BaseReference
    //    ACCESS SubstitutedSymbolType.Symbol
    // This function produces an "AlreadyBoundSymbolExpression", configured in such a way that
    // InterpretExpression will SUBSTITUTE "OriginalSymbolType.Symbol" into "SubstitutedSymbolType.Symbol".
    // For instance, if Symbol had been bound to an ancestor member without any generic type binding,
    // the result would also lack any generic type binding.

    VSASSERT(BaseReference != NULL, "This shouldn't be null");
    ParseTree::AlreadyBoundSymbolExpression * WrapperExpr = CreateBoundSymbol(Symbol)->AsAlreadyBoundSymbol();
    WrapperExpr->BaseReference = BaseReference;
    WrapperExpr->UseBaseReferenceTypeAsSymbolContainerType = false;
    return WrapperExpr;
}


//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateUnaryExpression
//
//  Description:    Creates a new unary expression
//
//  Parameters:
//      [UnaryType]     --  The expression type.
//      [Operand]   --  The unbound operand
//
//  History:    2-23-2005   Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::UnaryExpression *
ParserHelper::CreateUnaryExpression
(
    ParseTree::Expression::Opcodes UnaryType,
    _In_ ParseTree::Expression * Operand
)
{
    ParseTree::UnaryExpression * NewExpr = new (*m_TreeStorage) ParseTree::UnaryExpression();
    NewExpr->TextSpan = m_TextSpan;
    NewExpr->Opcode = UnaryType;
    NewExpr->Operand = Operand;

    return NewExpr;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateBinaryExpression
//
//  Description:    Creates a new unary expression
//
//  Parameters:
//      [UnaryType]     --  The expression type.
//      [Left]   --  The left operand
//      [Right]   --  The right operand
//
//  History:    09-22-2006   Microsoft    Created
//---------------------------------------------------------------------------
ParseTree::BinaryExpression *
ParserHelper::CreateBinaryExpression
(
    ParseTree::Expression::Opcodes UnaryType,
    _In_ ParseTree::Expression * Left,
    _In_ ParseTree::Expression * Right
)
{
    ParseTree::BinaryExpression * NewExpr = new (*m_TreeStorage) ParseTree::BinaryExpression();
    NewExpr->TextSpan = m_TextSpan;
    NewExpr->Opcode = UnaryType;
    NewExpr->Left = Left;
    NewExpr->Right = Right;

    return NewExpr;
}

ParseTree::LambdaExpression *
ParserHelper::CreateSingleLineLambdaExpression
(
    ParseTree::ParameterList *Parameters,
    _In_opt_ ParseTree::Expression *Value,
    bool functionLambda
)
{
    return CreateSingleLineLambdaExpression(Parameters, Value, m_TextSpan, functionLambda);
}

ParseTree::LambdaExpression *
ParserHelper::CreateSingleLineLambdaExpression
(
    ParseTree::ParameterList *Parameters,
    _In_opt_ ParseTree::Expression *Value,
    _In_ const Location & TextSpan,
    bool functionLambda
)
{
    ParseTree::LambdaExpression * NewExpr = new (*m_TreeStorage) ParseTree::LambdaExpression();
    NewExpr->TextSpan = TextSpan;
    NewExpr->Opcode = ParseTree::Expression::Lambda;
    NewExpr->Parameters = Parameters;
    NewExpr->SetSingleLineLambdaExpression(Value);
    NewExpr->AllowRelaxationSemantics = false;
    NewExpr->MethodFlags = functionLambda ? DECLF_Function : 0;
    NewExpr->IsStatementLambda = false;

    return NewExpr;
}

ParseTree::Parameter *
ParserHelper::CreateParameter
(
    ParseTree::IdentifierDescriptor Name,
    ParseTree::Type * Type,
    bool IsQueryIterationVariable,
    bool IsQueryRecord
)
{
    return CreateParameter(Name, Type, m_TextSpan, IsQueryIterationVariable, IsQueryRecord);
}

// Port SP1 CL 2929782 to VS10

ParseTree::Parameter *
ParserHelper::CreateParameter
(
    ParseTree::IdentifierDescriptor Name,
    ParseTree::Type * Type,
    const Location & TextSpan,
    bool IsQueryIterationVariable,
    bool IsQueryRecord,
    ParseTree::ParameterSpecifierList *Specifiers
)
{
    ParseTree::Declarator *Declarator = new (*m_TreeStorage) ParseTree::Declarator;
    ParseTree::Parameter *Parameter = new (*m_TreeStorage) ParseTree::Parameter;

    Declarator->Name = Name;
    Parameter->TextSpan = TextSpan;
    Parameter->Name = Declarator;
    Parameter->Type = Type;
    Parameter->Specifiers = Specifiers;

    AssertIfTrue(!IsQueryIterationVariable && IsQueryRecord);
    Parameter->IsQueryIterationVariable = IsQueryIterationVariable;

    if(IsQueryIterationVariable)
    {
        Parameter->IsQueryRecord = IsQueryRecord;
    }

    return Parameter;
}

ParseTree::ParameterSpecifierList *
ParserHelper::CreateParameterSpecifierList
(
    unsigned SpecifierCount,
    ...
)
{
    va_list SpecifierList;
    va_start(SpecifierList, SpecifierCount);
    return CreateParameterSpecifierList(m_TextSpan, SpecifierCount, SpecifierList);
}


ParseTree::ParameterSpecifierList *
ParserHelper::CreateParameterSpecifierList
(
    const Location & TextSpan,
    unsigned SpecifierCount,
    ...
)
{
    va_list SpecifierList;
    va_start(SpecifierList, SpecifierCount);
    return CreateParameterSpecifierList(TextSpan, SpecifierCount, SpecifierList);
}


ParseTree::ParameterSpecifierList *
ParserHelper::CreateParameterSpecifierList
(
    const Location & TextSpan,
    unsigned SpecifierCount,
    va_list SpecifierList
)
{
    ParseTree::ParameterSpecifierList * FirstSpecifier = NULL;
    ParseTree::ParameterSpecifierList * LastSpecifier  = NULL;

    for (unsigned i = 0; i < SpecifierCount; i++)
    {
        ParseTree::ParameterSpecifierList * Specifier = new(*m_TreeStorage) ParseTree::ParameterSpecifierList;
        Specifier->TextSpan = TextSpan;
        Specifier->Next = NULL;
        Specifier->Element = va_arg(SpecifierList, ParseTree::ParameterSpecifier *);

        if (FirstSpecifier == NULL) {
            FirstSpecifier = Specifier;
        }

        if (LastSpecifier != NULL) {
            LastSpecifier->Next = Specifier;
        }

        LastSpecifier = Specifier;
    }

    va_end(SpecifierList);

    return FirstSpecifier;
}

ParseTree::ParameterSpecifierList *
ParserHelper::AddParameterSpecifier
(
    ParseTree::ParameterSpecifierList * List,
    ParseTree::ParameterSpecifier * Specifier,
    const Location & TextSpan
)
{
    ParseTree::ParameterSpecifierList *item = new(*m_TreeStorage) ParseTree::ParameterSpecifierList;
    item->TextSpan = TextSpan;
    item->Element = Specifier;

    if( List != NULL )
    {
        VSASSERT(List->Next == NULL, L"Can only add parameter specifier to the end of the list");
        List->Next = item;
    }
    return( item );
}

    
ParseTree::ParameterSpecifier *
ParserHelper::CreateParameterSpecifier
(
    ParseTree::ParameterSpecifier::Specifiers SpecifierOpcode,
    _In_ const Location & TextSpan
)
{
    ParseTree::ParameterSpecifier *Specifier = new(*m_TreeStorage) ParseTree::ParameterSpecifier;
    Specifier->TextSpan = TextSpan;
    Specifier->Opcode = SpecifierOpcode;
    return Specifier;
}




ParseTree::ParameterList *
ParserHelper::CreateParameterList
(
    unsigned ParameterCount,
    ...
)
{
    va_list ParameterExpressionList;
    va_start(ParameterExpressionList, ParameterCount);
    return CreateParameterList(m_TextSpan, ParameterCount, ParameterExpressionList);
}

ParseTree::ParameterList *
ParserHelper::CreateParameterList
(
    const Location & TextSpan,
    unsigned ParameterCount,
    ...
)
{
    va_list ParameterExpressionList;
    va_start(ParameterExpressionList, ParameterCount);
    return CreateParameterList(TextSpan, ParameterCount, ParameterExpressionList);
}


ParseTree::ParameterList *
ParserHelper::CreateParameterList
(
    const Location & TextSpan,
    unsigned ParameterCount,
    va_list ParameterExpressionList
)
{
    ParseTree::ParameterList * FirstParameter = NULL;
    ParseTree::ParameterList * LastParameter = NULL;

    ParseTree::ParameterList * ParameterListElement = NULL;

    for (unsigned i = 0; i < ParameterCount; i++)
    {
        ParseTree::Parameter * Parameter = va_arg(ParameterExpressionList, ParseTree::Parameter *);

        ParameterListElement = new (*m_TreeStorage) ParseTree::ParameterList;
        ParameterListElement->TextSpan = TextSpan;
        ParameterListElement->Element = Parameter;

        if (FirstParameter == NULL) {
            FirstParameter = ParameterListElement;
        }

        if (LastParameter != NULL) {
            LastParameter->Next = ParameterListElement;
        }

        LastParameter = ParameterListElement;
    }

    va_end(ParameterExpressionList);

    return FirstParameter;
}

ParseTree::ParameterList *
ParserHelper::AddParameter
(
    ParseTree::ParameterList * List,
    ParseTree::Parameter * Parameter,
    const Location & TextSpan
)
{
    ThrowIfNull( Parameter );

    ParseTree::ParameterList * Element = new (*m_TreeStorage) ParseTree::ParameterList;
    Element->TextSpan = TextSpan;
    Element->Element = Parameter;
    Element->Element->TextSpan = TextSpan;

    if( List != NULL )
    {
        List->Next = Element;
    }
    return( Element );
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateType
//
//  Description:    Creates a "Type" from a Name description. Use CreateName
//                      to make the type name.
//
//  Parameters:
//      [TypeName]  --  The Type name.
//
//  Returns [ParseTree::Type]:
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::Type *
ParserHelper::CreateType
(
    ParseTree::Name * TypeName
)
{
    ParseTree::NamedType * NewType = new (*m_TreeStorage) ParseTree::NamedType();
    NewType->TextSpan = m_TextSpan;
    NewType->Opcode = ParseTree::Type::Named;
    NewType->TypeName = TypeName;

    return NewType;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateBoundType
//
//  Description:    This is a wrapper for an already bound type. This
//                      is used if we need to add a type that has already been
//                      bound into the parse tree.
 //
//  Parameters:
//      [BoundType]     --  A bound BCSYM Type
//
//  Returns [ParseTree::AlreadyBoundType]:
//
//  History:    3-23-2005   Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::AlreadyBoundType *
ParserHelper::CreateBoundType
(
    Type * BoundType
)
{
    return CreateBoundType(BoundType, m_TextSpan);
}


ParseTree::AlreadyBoundType *
ParserHelper::CreateBoundType
(
    Type * BoundType,
    const Location & TextSpan
)
{
    VSASSERT(BoundType != NULL, "This shouldn't be null");
    AssertIfTrue(TypeHelpers::IsBadType(BoundType));

    ParseTree::AlreadyBoundType * WrapperType = new (*m_TreeStorage) ParseTree::AlreadyBoundType;
    WrapperType->TextSpan = TextSpan;
    WrapperType->Opcode = ParseTree::Type::AlreadyBound;
    WrapperType->BoundType = BoundType;

    return WrapperType;
}

ParseTree::NullableType *
ParserHelper::CreateNullableType
(
    _In_ ParseTree::Type * ElementType,
    _In_ const Location & TextSpan
)
{
    AssertIfNull(ElementType);

    ParseTree::NullableType *NullableType = new (*m_TreeStorage) ParseTree::NullableType;
    NullableType->Opcode = ParseTree::Type::Nullable;
    NullableType->ElementType = ElementType;
    NullableType->TextSpan = TextSpan;

    return NullableType;
}


ParseTree::ImplicitConversionExpression *
ParserHelper::CreateImplicitConversionExpression
(
    _In_ ParseTree::Expression *Value,
    Type *TargetType
)
{
    AssertIfNull(Value);
    AssertIfNull(TargetType);

    ParseTree::ImplicitConversionExpression * conversion = new (*m_TreeStorage) ParseTree::ImplicitConversionExpression;
    conversion->TextSpan = Value->TextSpan;
    conversion->Opcode = ParseTree::Expression::ImplicitConversion;
    conversion->Value = Value;
    conversion->TargetType = TargetType;

    return conversion;
}


//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateBoundDelayCalculatedType
//
//  Description:    This is a wrapper for an already bound type. This
//                  is used if we need to add a type that has already been
//                  bound into the parse tree, but we don't know the exact type yet
//                  until this parsetree gets interpreted.
//
//  Parameters:
//      [DelayedCalculation]    --  a function pointer used to caclulated the bound type when invoked
//      [Parameter]   --  The parameter to pass to the delayed calculation function.
//
//  Returns [ParseTree::AlreadyBoundDelayCalculatedType]:
//
//  History:    11-8-2006    Microsoft     Created
//---------------------------------------------------------------------------
ParseTree::AlreadyBoundDelayCalculatedType *
ParserHelper::CreateBoundDelayCalculatedType
(
    BCSYM* (*DelayedCalculation)(void*),
    void* Parameter
)
{
    VSASSERT(DelayedCalculation != NULL, "This shouldn't be null");

    ParseTree::AlreadyBoundDelayCalculatedType * WrapperType = new (*m_TreeStorage) ParseTree::AlreadyBoundDelayCalculatedType;
    WrapperType->TextSpan = m_TextSpan;
    WrapperType->Opcode = ParseTree::Type::AlreadyBoundDelayCalculated;
    WrapperType->Parameter = Parameter;
    WrapperType->DelayedCalculation = DelayedCalculation;

    return WrapperType;

}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateNewObject
//
//  Description:    Creates a new instance of an object.
//
//  Parameters:
//      [ObjectType]    --  The type of the new object. Use CreateType.
//      [ConstructorArgs]   --  Optional - A list of Constructor arguments. Default NULL.
//
//  Returns [ParseTree::NewExpression]:
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::NewExpression *
ParserHelper::CreateNewObject
(
    _In_opt_ ParseTree::Type * ObjectType,
    ParseTree::ArgumentList * ConstructorArgs
)
{
    ParseTree::NewExpression *New = new (*m_TreeStorage) ParseTree::NewExpression;
    New->TextSpan = m_TextSpan;
    New->Opcode = ParseTree::Expression::New;

    New->InstanceType = ObjectType;
    New->Arguments.Values = ConstructorArgs;
    New->Arguments.ClosingParenthesisPresent = true;

    return New;
}


ParseTree::QueryOperatorCallExpression *
ParserHelper::CreateQueryOperatorCall
(
    ParseTree::CallOrIndexExpression *MethodCall
)
{
    ParseTree::QueryOperatorCallExpression * TheOperator = new (*m_TreeStorage) ParseTree::QueryOperatorCallExpression;
    TheOperator->TextSpan = MethodCall->TextSpan;
    TheOperator->Opcode = ParseTree::Expression::QueryOperatorCall;
    TheOperator->operatorCall = MethodCall;

    return TheOperator;
}

ParseTree::CrossJoinExpression *
ParserHelper::CreateCrossJoinExpression
(
    ParseTree::LinqExpression * Source,
    _In_opt_ ParseTree::LinqExpression * JoinTo,
    _In_ Location & TextSpan
)
{
    ParseTree::CrossJoinExpression *  crossJoin =  new (*m_TreeStorage) ParseTree::CrossJoinExpression;
    crossJoin->Opcode = ParseTree::Expression::CrossJoin;
    crossJoin->Source = Source;
    crossJoin->JoinTo = JoinTo;

    crossJoin->TextSpan = TextSpan;

    return crossJoin;
}

ParseTree::FromItem *
ParserHelper::CreateLetFromItem
(
    _In_ ParseTree::VariableDeclaration *VariableDeclaration,
    _In_ ParseTree::Expression * Value,
    _In_ Location &TextSpan
)
{
        ParseTree::FromItem *Result = new (*m_TreeStorage) ParseTree::FromItem;
        Result->ControlVariableDeclaration = VariableDeclaration;
        Result->IsLetControlVar = true;
        Result->Source = Value;
        Result->TextSpan = TextSpan;

        return Result;
}

ParseTree::FromExpression *
ParserHelper::CreateFromExpression
(
    _In_ ParseTree::FromItem * FromItem
)
{
    ParseTree::FromExpression *  From =  new (*m_TreeStorage) ParseTree::FromExpression;
    From->Opcode = FromItem->IsLetControlVar ? ParseTree::Expression::Let : ParseTree::Expression::From;

    ParseTree::FromList *ResultFromList = new (*m_TreeStorage) ParseTree::FromList;
    ResultFromList->Element = FromItem;
    ResultFromList->TextSpan = FromItem->TextSpan;

    From->FromItems = ResultFromList;

    From->TextSpan = FromItem->TextSpan;

    return From;
}

ParseTree::SelectExpression *
ParserHelper::CreateSelectExpression
(
    ParseTree::LinqExpression * Source,
    _In_opt_ ParseTree::InitializerList * Projection,
    _In_ Location & TextSpan
)
{
    ParseTree::SelectExpression *Select = new (*m_TreeStorage) ParseTree::SelectExpression;
    Select->Opcode = ParseTree::Expression::Select;
    Select->Source = Source;
    Select->Projection = Projection;
    Select->TextSpan = TextSpan;

    return Select;
}

ParseTree::QueryAggregateGroupExpression *
ParserHelper::CreateQueryAggregateGroupExpression
(
    _In_ ParseTree::LinqExpression * Group
)
{
    ParseTree::QueryAggregateGroupExpression *AggGroup = new (*m_TreeStorage) ParseTree::QueryAggregateGroupExpression;
    AggGroup->Opcode = ParseTree::Expression::QueryAggregateGroup;
    AggGroup->Group = Group;
    AggGroup->TextSpan = Group->TextSpan;

    return AggGroup;
}


//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateMethodCall
//
//  Description:    Creates a MethodCall.
//
//  Parameters:
//      [MethodName]    --  The name of the method. Use CreateNameExpression.
//      [Args]  --  Optional - A list of parameters to pass. Default NULL.
//
//  Returns [ParseTree::CallOrIndexExpression]:
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::CallOrIndexExpression *
ParserHelper::CreateMethodCall
(
    _In_ ParseTree::Expression *MethodName,
    ParseTree::ArgumentList * Args
)
{
    return CreateMethodCall(MethodName, Args, m_TextSpan);
}

ParseTree::CallOrIndexExpression *
ParserHelper::CreateMethodCall
(
    _In_ ParseTree::Expression *MethodName,
    _In_ ParseTree::ArgumentList * Args,
    _In_ const Location &TextSpan
)
{
    ParseTree::CallOrIndexExpression * TheCall = new (*m_TreeStorage) ParseTree::CallOrIndexExpression;
    TheCall->TextSpan = TextSpan;
    TheCall->Opcode = ParseTree::Expression::CallOrIndex;
    TheCall->Arguments.Values = Args;
    TheCall->Arguments.ClosingParenthesisPresent = true;
    TheCall->Target = MethodName;
    TheCall->AlreadyResolvedTarget = false;

    return TheCall;
}

ParseTree::CallOrIndexExpression *
ParserHelper::CreateMethodCallOnAlreadyResolvedTarget
(
    _In_ ParseTree::Expression *MethodName,
    _In_ ParseTree::ArgumentList * Args,
    _In_ const Location &TextSpan
)
{
    ParseTree::CallOrIndexExpression * TheCall = CreateMethodCall(MethodName, Args, TextSpan);
    TheCall->AlreadyResolvedTarget = true;
    return TheCall;
}


// Converts an AggregationExpression to a Call expression
// This is consumed by intellisense, so locations are also
// copied/calculated
ParseTree::CallOrIndexExpression *
ParserHelper::CreateMethodCallFromAggregation
(
    _In_opt_ ParseTree::Expression *BaseOfTarget,
    _In_ ParseTree::AggregationExpression *Aggregation
)
{
#pragma warning (push)
#pragma warning (disable:6001) // False warning as the code path above will initialize nShiftSize

    ParseTree::Expression *Target = CreateNameExpression(Aggregation->AggFunc);

    if (BaseOfTarget)
    {
        Target = CreateQualifiedExpression(BaseOfTarget, Target, Aggregation->TextSpan);
    }

    Location locCallExpression = Aggregation->TextSpan;

    ParseTree::ArgumentList *ArgumentList = NULL;

    if (Aggregation->Argument)
    {
        ArgumentList = CreateArgList(Aggregation->Argument->TextSpan, 1, Aggregation->Argument);
    }

    Location locLParen;
    locLParen.SetLocation(Aggregation->TextSpan.m_lBegLine, &Aggregation->FirstPunctuator, 1);

    Location locRParen;

    if (Aggregation->HaveRParen)
    {
        locRParen.SetLocation(Aggregation->TextSpan.m_lBegLine, &Aggregation->RParenLocation, 1);
    }
    else
    {
        locRParen.Invalidate();
    }

    if (Aggregation->HaveRParen)
    {
        locCallExpression.m_lEndLine = locRParen.m_lEndLine;
        locCallExpression.m_lEndColumn = locRParen.m_lEndColumn;
    }
    else if (ArgumentList)
    {
        locCallExpression.m_lEndLine = ArgumentList->TextSpan.m_lEndLine;
        locCallExpression.m_lEndColumn = ArgumentList->TextSpan.m_lEndColumn;
    }
    else
    {
        locCallExpression.m_lEndLine = locLParen.m_lEndLine;
        locCallExpression.m_lEndColumn = locLParen.m_lEndColumn;
    }

    ParseTree::CallOrIndexExpression *Call = CreateMethodCall(
        Target,
        ArgumentList,
        locCallExpression);

    Call->FirstPunctuator = Aggregation->FirstPunctuator;
    Call->Arguments.ClosingParenthesisPresent = Aggregation->HaveRParen;
    Call->Arguments.TextSpan.SetLocation(
        locLParen.m_lBegLine,
        locLParen.m_lBegColumn,
        Aggregation->HaveRParen ? locRParen.m_lEndLine : locCallExpression.m_lEndLine,
        Aggregation->HaveRParen ? locRParen.m_lEndColumn-1 : locCallExpression.m_lEndColumn);

    return Call;

#pragma warning (push)
}

// Cloned from Parser::ExpressionSyntaxError()
ParseTree::Expression *
ParserHelper::CreateSyntaxErrorExpression
(
        const Location & TextSpan
)
{
    ParseTree::Expression *Error = new (*m_TreeStorage) ParseTree::Expression;
    Error->TextSpan = TextSpan;
    Error->Opcode = ParseTree::Expression::SyntaxError;

    return Error;
}


ParseTree::Expression *
ParserHelper::CreateQualifiedExpression
(
    _In_opt_ ParseTree::Expression *Base,
    _In_ ParseTree::Expression *Name,
    ParseTree::Expression::Opcodes Opcode
)
{
    return CreateQualifiedExpression(Base,Name,m_TextSpan, Opcode);
}

ParseTree::Expression *
ParserHelper::CreateQualifiedExpression
(
    _In_ ParseTree::Expression *Base,
    _In_ ParseTree::Expression *Name,
    _In_ const Location & TextSpan,
    ParseTree::Expression::Opcodes Opcode
)
{
    ParseTree::QualifiedExpression *Qualified = new (*m_TreeStorage) ParseTree::QualifiedExpression;
    Qualified->TextSpan = TextSpan;
    Qualified->Opcode = Opcode;
    Qualified->Base = Base;
    Qualified->Name = Name;

    return Qualified;
}


ParseTree::VariableDeclaration *
ParserHelper::CreateVariableDeclaration
(
    _In_ ParseTree::IdentifierDescriptor &Name
)
{
    ParseTree::Declarator *Declarator = new (*m_TreeStorage) ParseTree::Declarator;
    ParseTree::DeclaratorList *Declarators = new (*m_TreeStorage) ParseTree::DeclaratorList;
    ParseTree::VariableDeclaration *VariableDecl = new (*m_TreeStorage) ParseTree::VariableDeclaration;

    Declarator->Name = Name;
    Declarator->TextSpan = Name.TextSpan;
    Declarators->Element = Declarator;
    Declarators->TextSpan = Name.TextSpan;
    VariableDecl->TextSpan = Name.TextSpan;
    VariableDecl->Opcode = ParseTree::VariableDeclaration::NoInitializer;
    VariableDecl->Variables = Declarators;
    VariableDecl->HasSyntaxError = false;

    return VariableDecl;
}

ParseTree::VariableDeclarationStatement *
ParserHelper::CreateVariableDeclarationStatement
(
    _In_z_ STRING * VariableName,
    _In_ ParseTree::Type * VariableType,
    _In_ ParseTree::Expression * Value
)
{
    //The Variable name wrapped in a list
    ParseTree::DeclaratorList * Variables = new (*m_TreeStorage) ParseTree::DeclaratorList;
    Variables->TextSpan = m_TextSpan;
    Variables->Element = new (*m_TreeStorage) ParseTree::Declarator;
    Variables->Element->TextSpan = m_TextSpan;
    Variables->Element->Name.Name = VariableName;

    // Build the defered expression
    ParseTree::DeferredExpression * DeferredValue = new (*m_TreeStorage) ParseTree::DeferredExpression;
    DeferredValue->TextSpan = m_TextSpan;
    DeferredValue->Opcode = ParseTree::Expression::Deferred;
    DeferredValue->Value = Value;

    // Build the initialization code.
    ParseTree::DeferredInitializer *InitialValue = new (*m_TreeStorage) ParseTree::DeferredInitializer;
    InitialValue->Opcode = ParseTree::Initializer::Deferred;
    InitialValue->Value = new (*m_TreeStorage) ParseTree::ExpressionInitializer;
    InitialValue->Value->Opcode = ParseTree::Initializer::Expression;
    InitialValue->Value->AsExpression()->Value = DeferredValue;

    //The declaration
    ParseTree::VariableDeclarationList * Declarations = new (*m_TreeStorage) ParseTree::VariableDeclarationList;
    Declarations->TextSpan = m_TextSpan;
    Declarations->Element = new (*m_TreeStorage) ParseTree::InitializerVariableDeclaration;
    Declarations->Element->TextSpan = m_TextSpan;
    Declarations->Element->Opcode = ParseTree::VariableDeclaration::WithInitializer;
    Declarations->Element->AsInitializer()->InitialValue = InitialValue;
    Declarations->Element->Variables = Variables;
    Declarations->Element->Type = VariableType;
    Declarations->Element->HasSyntaxError = false;

    // The specifier (dim)
    //The specifier (dim)wrapped in a list.
    ParseTree::SpecifierList * Specifiers = new (*m_TreeStorage) ParseTree::SpecifierList;
    Specifiers->TextSpan = m_TextSpan;
    Specifiers->Element = new (*m_TreeStorage) ParseTree::Specifier;
    Specifiers->Element->TextSpan = m_TextSpan;
    Specifiers->Element->Opcode = ParseTree::Specifier::Dim;

    // Combine the specifier and the declaration.
    ParseTree::VariableDeclarationStatement *Result = new (*m_TreeStorage) ParseTree::VariableDeclarationStatement;
    Result->TextSpan = m_TextSpan;
    Result->Opcode = ParseTree::Statement::VariableDeclaration;
    Result->Specifiers = Specifiers;
    Result->Attributes = NULL;
    Result->Declarations = Declarations;

    return Result;
}

ParseTree::IdentifierDescriptor
ParserHelper::CreateIdentifierDescriptor
(
    _In_z_ STRING * Name
)
{
    return CreateIdentifierDescriptor(Name, Location::GetHiddenLocation());
}

ParseTree::IdentifierDescriptor
ParserHelper::CreateIdentifierDescriptor
(
    _In_z_ STRING * Name,
    _In_ const Location & TextSpan
)
{
    ParseTree::IdentifierDescriptor Result;

    Result.Name = Name;
    Result.TypeCharacter = chType_NONE;
    Result.IsBracketed = false;
    Result.IsNullable = false;
    Result.IsBad = false;

    Result.TextSpan = TextSpan;

    return Result;
}
//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateAddressOf
//
//  Description:    Creates a new AddressOf Expression
//
//  Parameters:
//      [MethodReference]  -- the expression returning the method referece
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::AddressOfExpression *
ParserHelper::CreateAddressOf
(
    ParseTree::Expression * MethodReference,
    bool                    UseLocationOfTargetMethodForStrict /*=false*/
)
{
    VSASSERT(MethodReference != NULL, "Needs a method reference");

    ParseTree::AddressOfExpression *Result = new (*m_TreeStorage) ParseTree::AddressOfExpression;
    Result->TextSpan = m_TextSpan;
    Result->Opcode = ParseTree::Expression::AddressOf;
    Result->Operand = MethodReference;
    Result->UseLocationOfTargetMethodForStrict = UseLocationOfTargetMethodForStrict;

    return Result;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.AppendToStatementList
//
//  Description:    Appends teh passed in statement to the passed in statement list
//
//  Parameters:
//      [LatestStatement]  -- the statementlist to append the statement to.
//                            null is ok signifying the only one to append.
//      [CurrentStatement]  -- the statement to append to the statementlist
//
//  History:    21-10-2006    Microsoft     Created
//---------------------------------------------------------------------------
ParseTree::StatementList *
ParserHelper::AppendToStatementList
(
    _Inout_ ParseTree::StatementList * LatestStatement,
    _In_opt_ ParseTree::Statement * CurrentStatement
)
{
    //"Needs a current statement to put in the list."
    ThrowIfFalse(CurrentStatement != NULL);

    ParseTree::StatementList *StatementList = new (*m_TreeStorage) ParseTree::StatementList;

    StatementList->Element = CurrentStatement;

    StatementList->PreviousInBlock = LatestStatement;
    StatementList->PreviousLexical = LatestStatement;

    StatementList->NextInBlock = NULL;
    StatementList->NextLexical = NULL;

    if (LatestStatement)
    {
        ThrowIfFalse(LatestStatement->NextInBlock == NULL);
        ThrowIfFalse(LatestStatement->NextLexical == NULL);

        LatestStatement->NextInBlock = StatementList;
        LatestStatement->NextLexical = StatementList;
    }

    return StatementList;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateStatementList
//
//  Description:    Creates a new StatementList
//
//  Parameters:
//      [CurrentStatement]  -- the statement to wrap into a statementlist
//
//  History:    21-10-2006    Microsoft     Created
//---------------------------------------------------------------------------
ParseTree::StatementList *
ParserHelper::CreateStatementList
(
    _In_opt_ ParseTree::Statement * CurrentStatement
)
{
    //"Needs a current statement to put in the list."
    ThrowIfFalse(CurrentStatement != NULL);

    return AppendToStatementList(NULL, CurrentStatement);
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.JoinStatementList
//
//  Description:    This joins a list of statementlists into a single statementlists
//
//  Parameters:
//      [StatementCount]     --  The number of statementslists listed. Must be > 0
//      [...]           --  A list of statementlists to be joined
//
//  Returns void:
//
//---------------------------------------------------------------------------
void
ParserHelper::JoinStatementList
(
    unsigned StatementCount,
    ...
)
{
    va_list StatementList;
    va_start(StatementList, StatementCount);

    JoinStatementList(StatementCount, StatementList);

    va_end(StatementList);
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.JoinStatementList
//
//  Description:    This joins a list of statementlists into a single statementlists
//
//  Parameters:
//      [StatementCount]     --  The number of statementslists listed. Must be > 0
//      [StatementList]      --  A list of statementlists to be joined
//
//  Returns void:
//
//---------------------------------------------------------------------------
void
ParserHelper::JoinStatementList
(
    unsigned StatementCount,
    va_list StatementList
)
{
    ThrowIfFalse(StatementCount > 0);

    ParseTree::StatementList * LastStatement = NULL;

    for (unsigned i = 0; i < StatementCount; i++)
    {
        ParseTree::StatementList * StatementValue = va_arg(StatementList, ParseTree::StatementList *);

        LastStatement = JoinStatementList(LastStatement, StatementValue);
    }
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.JoinStatementList
//
//  Description:    Joins two statement lists
//
//  Parameters:
//      [Left]      --  Head of left statement list
//      [Right]     --  Head of right statement list
//
//  Returns [ParseTree::StatementList]: The last statement list of Right.
//
//---------------------------------------------------------------------------
ParseTree::StatementList *
ParserHelper::JoinStatementList
(
    _Inout_opt_ ParseTree::StatementList * Left,
    _Inout_opt_ ParseTree::StatementList * Right,
    ParseTree::BlockStatement * Parent /* = NULL*/
)
{
    ThrowIfFalse(Right != NULL);
    ThrowIfFalse(Right->PreviousLexical == NULL);
    ThrowIfFalse(Right->PreviousInBlock == NULL);

    if (Left != NULL)
    {
        ThrowIfFalse(Left->NextLexical == NULL);
        ThrowIfFalse(Left->NextInBlock == NULL);

        Left->NextInBlock = Right;
        Left->NextLexical = Right;
#if DEBUG
        if (Left->Element != NULL)
        {
            VSASSERT(Parent == Left->Element->GetParent(), "Left must already be parented.");
        }
#endif
    }

    ParseTree::StatementList * LastRight = FindLastStatementAndPlaceInBlock(Right, Parent);

    Right->PreviousInBlock = Left;
    Right->PreviousLexical = Left;

    ThrowIfFalse(LastRight->NextLexical == NULL);
    ThrowIfFalse(LastRight->NextInBlock == NULL);

    return LastRight;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateIfStatement
//
//  Description:    Creates a parsetree as SatementList for an If expression
//
//  Parameters:
//      [Condition]  --  The condition of the if statement
//      [Then]  --  A statemetnlist with the code for the if block.
//      [Else]  -- A statemetnlist with the else code. Can be null.
//
//  History:    2-7-2005    Microsoft     Created
//---------------------------------------------------------------------------
ParseTree::StatementList *
ParserHelper::CreateIfStatement
(
    ParseTree::Expression * Condition,
    _Inout_opt_ ParseTree::StatementList * Then,
    ParseTree::StatementList * Else
)
{
    ThrowIfNull(Condition);
    ThrowIfNull(Then);

    VSASSERT(!Else, "Else is not supported yet, please uncomment the else parts to get support");

    ParseTree::IfStatement *IfStatement = new (*m_TreeStorage) ParseTree::IfStatement;
    IfStatement->TextSpan = m_TextSpan;
    IfStatement->Opcode = ParseTree::Statement::BlockIf;
    IfStatement->Operand = Condition;
    IfStatement->Children = Then;

    // Find last statement in Then code and place the statements in the if block.
    ParseTree::StatementList *ThenLastStatement = FindLastStatementAndPlaceInBlock(Then, IfStatement);

    ParseTree::ElseStatement *ElseStatement = NULL;
    ParseTree::StatementList *ElseLastStatement = NULL;

    ParseTree::Statement *EndIfStatement = new (*m_TreeStorage) ParseTree::Statement;
    EndIfStatement->TextSpan = m_TextSpan;
    EndIfStatement->Opcode = ParseTree::Statement::EndIf;

    // End the If block.
    IfStatement->TerminatingConstruct = EndIfStatement;

    /* Else Part Not Supported yet, so not shipping
    if (Else)
    {
        ElseStatement = new (*m_TreeStorage) ParseTree::ElseStatement;
        ElseStatement->TextSpan = m_TextSpan;
        IfStatement->Opcode = ParseTree::Statement::BlockElse;
        ElseStatement->ContainingIf = IfStatement;
        ElseStatement->Children = Else;

        // Find last statement in Else code and place the statements in the else block.
        ElseLastStatement = FindLastStatementAndPlaceInBlock(Else, ElseStatement);
    }
    */

    // Convert the statements to statement lists
    ParseTree::StatementList *IfStatementList = new (*m_TreeStorage) ParseTree::StatementList;
    IfStatementList->Element = IfStatement;
    ParseTree::StatementList *ElseStatementList = new (*m_TreeStorage) ParseTree::StatementList;
    ElseStatementList->Element = ElseStatement;
    ParseTree::StatementList *EndIfStatementList = new (*m_TreeStorage) ParseTree::StatementList;
    EndIfStatementList->Element = EndIfStatement;

    // Link the statement lists lexically
    // IfStatement->PreviousLexical stays unchanged
    IfStatementList->NextLexical = Then;
    Then->PreviousLexical = IfStatementList;

    /* Else Part Not Supported yet, so not shipping
    if (Else)
    {
        ThenLastStatement->NextLexical = ElseStatementList;
        ElseStatementList->PreviousLexical = ThenLastStatement;
        ElseStatementList->NextLexical = Else;
        Else->PreviousLexical = ElseStatementList;
        ElseLastStatement->NextLexical = EndIfStatementList;
        EndIfStatementList->PreviousLexical = ElseLastStatement;
    }
    else
    */
    {
        ThenLastStatement->NextLexical = EndIfStatementList;
        EndIfStatementList->PreviousLexical = ThenLastStatement;
    }
    EndIfStatementList->NextLexical = NULL;

    // Link the statements at the block level

    // IfStatement->PreviousInBlock stays unchanged
    /* Else Part Not Supported yet, so not shipping
    if (Else)
    {
        IfStatementList->NextInBlock = ElseStatementList;
        ElseStatementList->PreviousInBlock = IfStatementList;
        ElseStatementList->NextInBlock = EndIfStatementList;
        EndIfStatementList->PreviousInBlock = ElseStatementList;
    }
    else
    */
    {
        IfStatementList->NextInBlock = EndIfStatementList;
        EndIfStatementList->PreviousInBlock = IfStatementList;
    }
    EndIfStatementList->NextInBlock = NULL;

    return IfStatementList;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateAddHandler
//
//  Description:    Creates a parsetree for AddHandler
//
//  Parameters:
//      [EventReference]  --  The base array
//      [DelegateReference]  --  A reference expression like AddressOf or Lambda
//
//  History:    2-7-2005    Microsoft     Created
//---------------------------------------------------------------------------
ParseTree::HandlerStatement *
ParserHelper::CreateAddHandler
(
    _In_ ParseTree::Expression * EventReference,
    _In_opt_ ParseTree::Expression * DelegateReference
)
{
    return CreateHandlerStatement(
        ParseTree::Statement::AddHandler,
        EventReference,
        DelegateReference);
}


//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateAddHandler
//
//  Description:    Creates a parsetree for AddHandler
//
//  Parameters:
//      [EventReference]  --  The base array
//      [DelegateReference]  --  A reference expression like AddressOf or Lambda
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::HandlerStatement *
ParserHelper::CreateRemoveHandler
(
    _In_ ParseTree::Expression * EventReference,
    _In_opt_ ParseTree::Expression * DelegateReference
)
{
    return CreateHandlerStatement(
        ParseTree::Statement::RemoveHandler,
        EventReference,
        DelegateReference);
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateAssignment
//
//  Description:    Creates an assignment statement and assigns the source
//                  expression to the target expressons symbol.
//
//  Parameters:
//      [Target]  --  Target = Source
//      [Source]  --  Target = Source
//
//  History:    21-9-2006    Microsoft     Added
//---------------------------------------------------------------------------
ParseTree::AssignmentStatement *
ParserHelper::CreateAssignment
(
    _In_opt_ ParseTree::Expression * Target,
    _In_opt_ ParseTree::Expression * Source
)
{
    ThrowIfFalse(Target != NULL);
    ThrowIfFalse(Source != NULL);

    ParseTree::AssignmentStatement *Result = new (*m_TreeStorage) ParseTree::AssignmentStatement;
    Result->TextSpan = m_TextSpan;
    Result->Opcode = ParseTree::Statement::Assign;

    Result->Target = Target;
    Result->Source = Source;

    return Result;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.FindLastStatementAndPlaceInBlock
//
//  Description:    Helper to find the last statement in the statement list
//                  by walking the NextInBlock.
//                  If the statemetn contains an element it will update that
//                  element to point to the parent block.
//
//  Parameters:
//      [StartOfList]  --  The completely seperate unattached statementlist
//      [ParentBlock]  --  The block to make the parent of these statements
//
//  History:    21-9-2006    Microsoft     Added
//---------------------------------------------------------------------------
ParseTree::StatementList *
ParserHelper::FindLastStatementAndPlaceInBlock
(
    _In_opt_ ParseTree::StatementList * StartOfList,
    ParseTree::BlockStatement * ParentBlock
)
{
    ThrowIfFalse(StartOfList != NULL);
    ThrowIfFalse(StartOfList->PreviousLexical == NULL);
    ThrowIfFalse(StartOfList->PreviousInBlock == NULL);

    ParseTree::StatementList * LastStatement = StartOfList;

    if (LastStatement->Element)
    {
        LastStatement->Element->SetParent(ParentBlock);
    }

    while (LastStatement &&
           LastStatement->NextInBlock &&
           LastStatement->NextInBlock->Element) // Don't know exact scenario, but other loops terminate on empty Elements as well.
    {
        LastStatement = LastStatement->NextInBlock;
        if (LastStatement->Element)
        {
            LastStatement->Element->SetParent(ParentBlock);
        }
    }

    ThrowIfFalse(LastStatement->NextLexical == NULL);
    ThrowIfFalse(LastStatement->NextInBlock == NULL);

    return LastStatement;
}

//+--------------------------------------------------------------------------
//  Method:     ParserHelper.CreateHandlerStatement
//
//  Description:    Creates a parsetree for Add or Remove Handler
//
//  Parameters:
//      [OpCode]             --  The opcode to say add or remove handler.
//      [EventReference]     --  The base array
//      [DelegateReference]  --  A reference expression like AddressOf or Lambda
//
//  History:    2-7-2005    Microsoft     Comment Added
//---------------------------------------------------------------------------
ParseTree::HandlerStatement *
ParserHelper::CreateHandlerStatement
(
    ParseTree::Statement::Opcodes OpCode,
    _In_ ParseTree::Expression * EventReference,
    _In_opt_ ParseTree::Expression * DelegateReference
)
{
    VSASSERT(EventReference != NULL, "Needs a method reference");
    VSASSERT(DelegateReference != NULL, "Needs a method reference");
    VSASSERT(
        DelegateReference->Opcode == ParseTree::Expression::Lambda ||
        DelegateReference->Opcode == ParseTree::Expression::AddressOf ||
        DelegateReference->Opcode == ParseTree::Expression::Name,
        "Method reference should be Lambda or AddressOf ");

    ParseTree::HandlerStatement * NewExpr = new (*m_TreeStorage) ParseTree::HandlerStatement();
    NewExpr->TextSpan = m_TextSpan;
    NewExpr->Opcode = OpCode;
    NewExpr->Event = EventReference;
    NewExpr->Delegate = DelegateReference;

    return NewExpr;
}

////////////////////////////////////
//
// TryGetLambdaBodyFromParseTreeNode
//
// Run through pParseTreeNode looking for any multiline lambdas within the statement or expr.
//
// Returns the LambdaBodyStatement for the lambda that contains the location of pTextSpan.
// If pTextSpan is NULL, returns the last lambda (lexically) that is in the node.
// If calling from IDE, look at ParseTreeHelpers::GetLambdaBodyInStatement, which doesn't visit every node


//    static 
ParseTree::LambdaBodyStatement *
ParserHelper::TryGetLambdaBodyFromParseTreeNode
(
    _In_ ParseTree::ParseTreeNode *pParseTreeNode, 
    _In_ bool fIsStatement, // kind of pParseTreeNode: either ParseTree::Statement or ParseTree::Expression
    _In_opt_ Location *pTextSpan )
{
    // Try to find a lambda 
    ParseTree::LambdaBodyStatement *pLambdaBodyStatement = NULL;
    if (pParseTreeNode)
    {
        ParseTree::Expression *pResult = NULL;
        if (fIsStatement)
        {
            pResult = ParseTreeSearcher::FindFirstExpressionWithArgument((ParseTree::Statement *) pParseTreeNode, pTextSpan, ParseTree::LambdaExpression::GetIsStatementLambda);
        }
        else
        {
            pResult = ParseTreeSearcher::FindFirstExpressionWithArgument((ParseTree::Expression *)pParseTreeNode, pTextSpan, ParseTree::LambdaExpression::GetIsStatementLambda);
        }
        
        if (pResult)
        {
            pLambdaBodyStatement= pResult->AsLambda()->GetStatementLambdaBody();
        }
    }

    return pLambdaBodyStatement;
    
}


// GetLambdaBodyInStatement
//
// Run through pStatement looking for any multiline lambdas within the statement.
//
// Returns the LambdaBodyStatement for the lambda that contains the location of pTextSpan.
// If there are nested lambdas, returns the Outermost 
//
//static
ParseTree::LambdaBodyStatement *
ParserHelper::GetLambdaBodyInStatement
(
    _In_ ParseTree::Statement *pStatement, 
    _In_ Location *pTextSpan, 
    _In_opt_ ParseTreeService *pParseTreeService)
{
    ParseTree::LambdaBodyStatement *pLambdaBodyStatement = NULL;
    if (!pStatement->ContainsLambdaExpression)
    {
        return pLambdaBodyStatement ;
    }
#if IDE
    static bool fRecursionGuard = false;
    VSASSERT(fRecursionGuard == false, "Didn't i get rid of recursion here?");
    if (!fRecursionGuard && pParseTreeService)
    {
        // from tree root direct path down tree: descends to leaf via only branches that contain pTextSpan
        BackupValue<bool> backupRecursionGuard(&fRecursionGuard);
        fRecursionGuard  = true;
        DynamicArray<ParseTreeContextElement *> daContextElements;

        // this will find ALL lambdabody stmts within a stmt. 
        pParseTreeService->LocateAllContexts(pStatement, PTCT_Statement,  PTCT_LambdaBodyStatement, &daContextElements);
        
        Location *plocOutermost = 0;
        
        for (ULONG i = 0 ; i < daContextElements.Count() ; i++)
        {
            ParseTreeContextElement *pElement = daContextElements.Element(i) ;
            ParseTree::LambdaBodyStatement *pLambdaBodyTemp = pElement->m_pLambdaBodyStatement;
            
            if (plocOutermost == NULL  || pLambdaBodyTemp->BodyTextSpan.ContainsInclusive(*plocOutermost)) 
            {
                ParseTree::LambdaExpression *pLambdaExpr = pLambdaBodyTemp->pOwningLambdaExpression;    // the owner has the complete textspan, including header
                // if the one we found is in the right span
                if (pLambdaExpr->TextSpan.ContainsInclusive(*pTextSpan))
                {
                    plocOutermost = &pLambdaExpr->TextSpan; 
                    pLambdaBodyStatement = pLambdaBodyTemp; 
                }
            }
            
        }

    }
    else
#endif IDE
    {
        // visits every node in tree
        pLambdaBodyStatement = ParserHelper::TryGetLambdaBodyFromParseTreeNode( pStatement,true, pTextSpan);
    }

    return pLambdaBodyStatement;
}




//============================================================================
// Given an Initializer finds the nested Initializer with expression and
// returns the expresion, otherwise returns NULL.
//============================================================================
ParseTree::Expression*
ParserHelper::DigToInitializerValue(ParseTree::Initializer *pInitializer)
{
    while (pInitializer && pInitializer->Opcode != ParseTree::Initializer::Expression)
    {
        if (pInitializer->Opcode == ParseTree::Initializer::Deferred)
            pInitializer = pInitializer->AsDeferred()->Value;
        else
            pInitializer = pInitializer->AsAssignment()->Initializer;
    }
    return pInitializer ? pInitializer->AsExpression()->Value : (ParseTree::Expression*)NULL;
}

