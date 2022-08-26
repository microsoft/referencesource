//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Definition of abstract syntax trees produced by the parser.
//
//  There are several root classes in the parse tree class hierarchy. These are:
//
//      Statement
//      Expression
//      Name
//      Type
//      Initializer
//
//  Every parse tree has a field named Opcode that identifies the specific tree
//  construct. There is a separate set of Opcode values for each root class.
//
//  Every root class has a set of accessor methods (all named "AsXXX")
//  to safely cast to specific classes. For example, to interpret an Expression
//  tree T as a BinaryExpression, use "T->AsBinary()". (These are declared with
//  XXXAccessor and SpecificXXXAccessor macros, where XXX is the name of the
//  root class.)
//
//-------------------------------------------------------------------------------------------------

#pragma once

namespace ILTree
{
    struct Expression;
}
namespace ParseTree
{
    typedef STRING Identifier;

    struct Initializer;
    struct BracedInitializerList;
    struct Expression;
    struct DeferredExpression;
    struct Statement;
    struct StatementList;
    struct ExpressionList;
    struct ArgumentList;
    struct Argument;
    struct Type;
    struct ArrayType;
    struct Name;
    struct NameList;
    struct TypeList;
    struct GenericParameterList;
    struct CommentList;
    struct ArrayDimList;

    // ;IdentifierDescriptor
    struct IdentifierDescriptor
    {
        Identifier *Name;
        typeChars TypeCharacter;
        bool IsBracketed;
        bool IsBad;
        bool IsNullable;

        Location TextSpan;
    };

    struct PunctuatorLocation
    {
        // Line is the delta from the start line of the construct
        // where a punctuator within the construct starts. Punctuators
        // are syntactic elements that do not appear in a typical abstract syntax
        // tree (e.g. infix or prefix operators, separator tokens, or parentheses).
        // Their locations are noted in the trees to enable accurate mapping of
        // locations to syntactic elements, and to enable pretty listing to respect
        // line continuations.
        long Line;

        // Column is the column where the first punctuator of the
        // construct (if any) starts.
        long Column;
    };

    // ;List. Note this is a lightweight template so that each instantiation will have minimal code generation
    template <class ElementType, class ListType> struct List : ParseTreeNode
    {
        ElementType *Element;

        ListType *Next;

        Location TextSpan; // From the start of the list element through the end of the list.

        // Most list punctuators are commas, but some are not.
        PunctuatorLocation Punctuator;
    };


    template <class ElementType, class ListType> int
    CountElements
    (
        ParseTree::List<ElementType, ListType> *List
    )
    {

         for (int i = 0; List != NULL; List = List->Next) {
            i++;
         }
         return i;
    }

    template <class ElementType, class ListType> 
    inline List<ElementType, ListType> * GetLastListNode
    (
        _In_opt_ ParseTree::List<ElementType, ListType> *List
    )
    {
        if (List == NULL)
        {
            return NULL;
        }

        while (List->Next)
        {
            List = List->Next;
        }

        return List;
    }

    


    template <class ElementType, class ListType> inline ElementType *
    GetLastListItem
    (
        _In_opt_ ParseTree::List<ElementType, ListType> *List
    )
    {
        if (List == NULL)
        {
            return NULL;
        }

        while (List->Next)
        {
            List = List->Next;
        }

        return List->Element;
    }


    struct DeferredText
    {
        size_t TextLengthInCharacters;
        WCHAR Text[1];
    };

    // ********** Begin Statements **********

    struct AttributeStatement;
    struct NamedValueStatement;
    struct EnumeratorStatement;
    struct EnumeratorWithValueStatement;
    struct ExpressionStatement;
    struct BlockStatement;
    struct EndBlockStatement;
    struct HiddenBlockStatement;
    struct ExecutableBlockStatement;
    struct MethodBodyStatement;
    struct LambdaBodyStatement;
    struct CommentBlockStatement;
    struct TypeStatement;
    struct EnumTypeStatement;
    struct NamespaceStatement;
    struct MethodSignatureStatement;
    struct MethodDeclarationStatement;
    struct MethodDefinitionStatement;
    struct PropertyStatement;
    struct AutoPropertyStatement;
    struct OperatorDefinitionStatement;
    struct EventDeclarationStatement;
    struct BlockEventDeclarationStatement;
    struct ForeignMethodDeclarationStatement;
    struct VariableDeclarationStatement;
    struct TypeListStatement;
    struct ImportsStatement;
    struct FileBlockStatement;
    struct ExpressionBlockStatement;
    struct IfStatement;
    struct ElseIfStatement;
    struct ElseStatement;
    struct SelectStatement;
    struct TopTestDoStatement;
    struct BottomTestLoopStatement;
    struct CatchStatement;
    struct FinallyStatement;
    struct ForStatement;
    struct ForFromToStatement;
    struct ForEachInStatement;
    struct EndNextStatement;
    struct LabelReferenceStatement;
    struct OnErrorStatement;
    struct ResumeStatement;
    struct CallStatement;
    struct RaiseEventStatement;
    struct AssignmentStatement;
    struct EraseStatement;
    struct AssignMidStatement;
    struct RedimStatement;
    struct CaseStatement;
    struct HandlerStatement;
    struct RegionStatement;
    struct OptionStatement;
    struct CCConstStatement;
    struct CCIfStatement;
    struct CCElseStatement;
    struct CCBranchStatement;
    struct CCEndStatement;
    struct DelegateDeclarationStatement;
    struct UsingStatement;
    struct LambdaExpression;
    struct CallOrIndexExpression;
    struct AttributeSpecifierList;

    struct ParseTreeNode
    {
        ParseTreeNode()
        {
        }
    };

    // ;Statement
    struct Statement : ParseTreeNode
    {
        
        enum Opcodes
        {
            #define DEF_STATEMENT(OpCode, Type, CastFunc) OpCode,
            #define DEF_STATEMENT_ENUM_VALUE(Name, Value) Name = Value,
            #include "StatementTable.h"
            #undef DEF_STATEMENT_ENUM_VALUE
            #undef DEF_STATEMENT
        };

        bool
        IsBlock
        (
        );

        bool
        IsContainerBlock
        (
        );

        bool
        IsExecutableBlock
        (
        );

        bool
        IsMethodDefinition
        (
        );

        bool
        IsMethodBody
        (
        );

        bool
        IsEndConstruct
        (
        );

        bool
        Statement::IsPlacebo
        (
        );

        bool
        IsConditionalCompilationBranch
        (
        );

        bool 
        IsOptionStatement
        (
        );

        Opcodes Opcode;
        Location TextSpan;

        StatementList *ContainingList;

        CommentList *Comments;
        ExpressionList *XmlRoots;
        unsigned short ResumeIndex;

        bool HasSyntaxError : 1;
        bool IsFirstOnLine : 1;
        bool ContainsAnonymousTypeInitialization : 1;
        bool ContainsQueryExpression : 1;
        bool ContainsLambdaExpression : 1;

        Statement * AsStatement()
        {
            return this;
        }
        
        StatementAccessor(Attribute)
        StatementAccessor(NamedValue)
        StatementAccessor(Enumerator)
        StatementAccessor(EnumeratorWithValue)
        StatementAccessor(Expression)
        StatementAccessor(Block)
        StatementAccessor(EndBlock)
        StatementAccessor(HiddenBlock)
        StatementAccessor(ExecutableBlock)
        StatementAccessor(MethodBody)
        StatementAccessor(LambdaBody)
        StatementAccessor(CommentBlock)
        StatementAccessor(Type)
        StatementAccessor(EnumType)
        StatementAccessor(Namespace)
        StatementAccessor(MethodDefinition)
        StatementAccessor(Property)
        StatementAccessor(AutoProperty)
        StatementAccessor(OperatorDefinition)
        StatementAccessor(EventDeclaration)
        StatementAccessor(BlockEventDeclaration)
        StatementAccessor(MethodDeclaration)
        StatementAccessor(DelegateDeclaration)
        StatementAccessor(MethodSignature)
        StatementAccessor(ForeignMethodDeclaration)
        StatementAccessor(VariableDeclaration)
        StatementAccessor(TypeList)
        StatementAccessor(Imports)
        StatementAccessor(FileBlock)
        StatementAccessor(ExpressionBlock)
        StatementAccessor(If)
        StatementAccessor(ElseIf)
        StatementAccessor(Else)
        StatementAccessor(Select)
        StatementAccessor(TopTestDo)
        StatementAccessor(BottomTestLoop)
        StatementAccessor(Catch)
        StatementAccessor(Finally)
        StatementAccessor(For)
        StatementAccessor(ForFromTo)
        StatementAccessor(ForEachIn)
        StatementAccessor(EndNext)
        StatementAccessor(LabelReference)
        StatementAccessor(Call)
        StatementAccessor(RaiseEvent)
        StatementAccessor(Assignment)
        StatementAccessor(Erase)
        StatementAccessor(AssignMid)
        StatementAccessor(Redim)
        StatementAccessor(Case)
        StatementAccessor(OnError)
        StatementAccessor(Resume)
        StatementAccessor(Handler)
        StatementAccessor(Region)
        StatementAccessor(Option)
        StatementAccessor(CCConst)
        StatementAccessor(CCIf)
        StatementAccessor(CCElse)
        StatementAccessor(CCBranch)
        StatementAccessor(CCEnd)
        StatementAccessor(Using)

    #if DEBUG
        static void DumpOpcode(Opcodes Opcode);
    #endif

        BlockStatement *GetParent();
        BlockStatement *GetRawParent();
        BlockStatement **GetRawParentAddress();
        void SetParent(BlockStatement *pValue);

    private:
        BlockStatement *Parent;
    };

    // ;StatementList
    struct StatementList
    {
        Statement *Element;

        // All statements for a given parent block are collected
        // in lexical order on a list.

        StatementList *PreviousInBlock;
        StatementList *NextInBlock;

        // All statements for a parsed unit, irrespective of block structure,
        // are collected in lexical order.

        StatementList *PreviousLexical;
        StatementList *NextLexical;

        // If the element statement is not the last on a line, and is therefore
        // separated from the NextLexical statement by a colon, this is the
        // punctuator for the colon, relative to the start location of the
        // element statement.

        PunctuatorLocation Colon;
    };

    // ;BlockStatement
    struct BlockStatement : Statement
    {
        StatementList *Children;

        // If the block has an end construct, TerminatingConstruct is it.
        // An If is terminated by an EndIf, not by an Else or ElseIf.
        // A Try is terminated by an EndTry, not by a Catch or Finally.
        Statement *TerminatingConstruct;
        bool HasProperTermination;
        // When a block statement gets established by the Parser as a context,
        // this flag specifies when that context is or is contained by a Try,
        // Catch, Finally, Using, SyncLock, etc.
        bool IsOrContainedByExceptionContext;

        // The location of the body does not include the block statement
        // or the end construct of the block. The end construct of a block
        // is always the statement immediately following the block in a
        // statement list, so the full span of a block is from the start of
        // the block statement through the end of the statement following
        // the block.
        Location BodyTextSpan;

        SpecificStatementAccessor(Block)
    };

    // ;EndBlockStatement
    struct EndBlockStatement : Statement
    {
        // The location of the punctuator (block keyword) in the End 
        // block statement
        PunctuatorLocation Punctuator;

        SpecificStatementAccessor(EndBlock)
    };

    // ;HiddenBlockStatement
    // HiddenBlockStatements are the only BlockStatements that do not 
    // require a End Construct. They are also not included into a StatementList.
    // Microsoft: Consider, this seems poorly named.  There are assumptions in the
    // parser that only a multi-line lambda can use these (e.g. LinkStatement())
    // Therefore this should probably be called MultiLineLambdaBlockStatement...
    struct HiddenBlockStatement : BlockStatement
    {
        SpecificStatementAccessor(HiddenBlock)
    };

    struct ExecutableBlockStatement : BlockStatement
    {
        unsigned LocalsCount;

        SpecificStatementAccessor(ExecutableBlock)
    };

    struct MethodBodyStatement : ExecutableBlockStatement
    {
        unsigned DefinedLabelCount;
        unsigned OnErrorHandlerCount;
        unsigned OnErrorResumeCount;
        unsigned ResumeTargetCount;
        StatementList *DefinedLabels;
        MethodDefinitionStatement *Definition;
        bool IsEmpty;
        bool ProcedureContainsTry;
        bool ProcedureContainsOnError;
        bool ProcedureContainsResume;

        SpecificStatementAccessor(MethodBody)
    };

    struct LambdaBodyStatement : ExecutableBlockStatement
    {
        unsigned definedLabelCount;
        StatementList *pDefinedLabels;
        StatementList *pLastLabelLinked;
        ParseTree::Statement *pOwningStatement;
        ParseTree::LambdaExpression *pOwningLambdaExpression;
        ParseTree::LambdaBodyStatement *pOuterMostLambdaBody;
        bool linkedIntoStatementList;

        // Members used for Function statement lambdas.
        Type *pReturnType;
        AttributeSpecifierList *pReturnTypeAttributes;
        PunctuatorLocation As;

        SpecificStatementAccessor(LambdaBody)
    };

    // A CommentBlockStatement is a block which contains a list of comments.
    // All comments in that list must either be XMLDoc comments or normal
    // comments, but not a mix of both.
    // XMLDoc comments interpretation can be switched on or off by a flag
    // passed into the parser. If the switch is off, XMLDoc comments are treated
    // like normal comment. Comment blocks cannot appear inside methods.
    struct CommentBlockStatement : BlockStatement
    {
        bool IsXMLDocComment;

        SpecificStatementAccessor(CommentBlock)
    };

    // An ExternalSourceDirective represents a #ExternalSource directive.
    // FirstLine and LastLine represent the lines between the start and end
    // constructs of the directive.
    //
    // The fields declared here as signed longs are logically unsigned, but
    // are declared to match the (also logically unsigned) signed long fields
    // in Location.
    struct ExternalSourceDirective
    {
        long FirstLine;
        long LastLine;
        const WCHAR *ExternalSourceFileName;
        long ExternalSourceFileStartLine;
        ExternalSourceDirective *Next;
        bool hasErrors;
    };

    // ;FileBlockStatement
    struct FileBlockStatement : BlockStatement
    {
        ExternalSourceDirective *SourceDirectives;

        SpecificStatementAccessor(FileBlock)
    };

    // ;OptionStatement
    struct OptionStatement : Statement
    {
        PunctuatorLocation FirstPunctuator;

        SpecificStatementAccessor(Option)
    };

    struct ParenthesizedArgumentList : ParseTreeNode
    {
        ParenthesizedArgumentList()
        {
            Values = NULL;
            ClosingParenthesisPresent = false;
            TextSpan.SetLocation(0,0,0,0);            
        }
            
        ArgumentList *Values;

        // Intellisense cares if the argument list is complete.
        bool ClosingParenthesisPresent;

        Location TextSpan;
    };

    // ********** Begin Declaration Statements **********

    struct Attribute : ParseTreeNode
    {
        Name *Name;
        ParenthesizedArgumentList Arguments;

        // Can be NULL if error parsing attribute
        DeferredExpression *DeferredRepresentationAsCall;

        bool IsAssembly;
        bool IsModule;

        PunctuatorLocation Colon;

        Location TextSpan;
    };

    struct AttributeList : List<Attribute, AttributeList>
    {
    };

    struct AttributeSpecifier
    {
        AttributeList *Values;
        Location TextSpan;
    };

    struct AttributeSpecifierList: List<AttributeSpecifier, AttributeSpecifierList>
    {
    };

    // ;Declarator
    struct Declarator : ParseTreeNode
    {
        IdentifierDescriptor Name;
        ArrayType *ArrayInfo;

        Location TextSpan;
        unsigned short ResumeIndex;
        unsigned char IsForControlVarDeclaration:1;
    };


    // ;Specifier
    struct Specifier
    {
        enum Specifiers
        {
            Private = 0x1,
            Protected = 0x2,
            Friend = 0x4,
            ProtectedFriend = 0x6,
            Public = 0x8,
            Accesses = 0xf,

            NotOverridable = 0x10,
            Overridable = 0x20,
            MustOverride = 0x40,
            OverrideModifiers = 0x70,

            Dim = 0x80,
            Const = 0x100,
            DataModifiers = 0x180,

            MustInherit = 0x200,
            NotInheritable = 0x400,
            Overloads = 0x800,
            Overrides = 0x1000,
            Default = 0x2000,
            Static = 0x4000,
            Shared = 0x8000,
            Shadows = 0x10000,

            ReadOnly = 0x20000,
            WriteOnly = 0x40000,
            WriteabilityModifiers = 0x60000,

            WithEvents = 0x80000,

            Partial = 0x100000,

            Widening = 0x200000,
            Narrowing = 0x400000,
            ConversionModifiers = 0x600000,

            // Should only show up in error scenarios
            Custom = 0x800000,

            Async = 0x1000000,
            Iterator = 0x2000000
        };

        Specifiers Opcode;
        Location TextSpan;
    };

    // ;SpecifierList
    struct SpecifierList : List<Specifier, SpecifierList>
    {
        SpecifierList *
        HasSpecifier
        (
            // The target specifiers can be a bit mask of multiple specifiers.
            unsigned TargetSpecifiers
        );
    };

    // ;TypeStatement - this is for Class, Interface, Structure, Module.
    struct TypeStatement : BlockStatement
    {
        SpecifierList *Specifiers;
        IdentifierDescriptor Name;
        GenericParameterList *GenericParameters;

        AttributeSpecifierList *Attributes;

        PunctuatorLocation TypeKeyword;
        PunctuatorLocation GenericOf;
        PunctuatorLocation GenericLeftParen;
        PunctuatorLocation GenericRightParen;

        SpecificStatementAccessor(Type)
    };

    // ;EnumTypeStatement
    struct EnumTypeStatement : TypeStatement
    {
        Type *UnderlyingRepresentation;

        PunctuatorLocation As;

        SpecificStatementAccessor(EnumType)
    };

    // ********** Begin Constraints **********

    struct TypeConstraint;

    // ;Constraint
    struct Constraint : ParseTreeNode
    {
        
        enum Opcodes
        {
                    
            #define DEF_CONSTRAINT(opCode, type, CastFunc) opCode, 
            #include "ConstraintTable.h"
            #undef DEF_CONSTRAINT
        };

        Opcodes Opcode;
        Location TextSpan;

        Constraint * AsConstraint()
        {
            return this;
        }
        
        ConstraintAccessor(Type);
    };

    struct TypeConstraint : Constraint
    {
        ParseTree::Type *Type;

        SpecificConstraintAccessor(Type);
    };

    // ;ConstraintList
    struct ConstraintList : List<Constraint, ConstraintList>
    {
    };

    // ********** End Constraints **********

    // ;GenericParameter
    struct GenericParameter
    {
        enum Variance_Kind : unsigned char
        {
            Variance_None,
            Variance_In,
            Variance_Out
            // nb. we pack this into a 2-bit bitfield, so don't add more to it
        };
        IdentifierDescriptor Name;
        ConstraintList *Constraints;
        Variance_Kind Variance : 2;

        Location TextSpan;

        PunctuatorLocation As;
        PunctuatorLocation VarianceLocation;
        PunctuatorLocation LeftBrace;
        PunctuatorLocation RightBrace;
    };

    // ;GenericParameterList
    struct GenericParameterList : List<GenericParameter, GenericParameterList>
    {
    };

    // ;NamespaceStatement
    struct NamespaceStatement : BlockStatement
    {
        ParseTree::Name *Name;

        SpecificStatementAccessor(Namespace)
    };

    // ;AttributeStatement
    struct AttributeStatement : Statement
    {
        AttributeSpecifierList *Attributes;

        SpecificStatementAccessor(Attribute)
    };

    // ;NamedValueStatement
    struct NamedValueStatement : Statement
    {
        IdentifierDescriptor Name;
        Expression *Value;

        PunctuatorLocation Equals;

        SpecificStatementAccessor(NamedValue)
    };

    // ;EnumeratorStatement
    struct EnumeratorStatement : Statement
    {
        IdentifierDescriptor Name;
        AttributeSpecifierList *Attributes;

        SpecificStatementAccessor(Enumerator)
    };

    // ;EnumeratorWithValueStatement
    struct EnumeratorWithValueStatement : EnumeratorStatement
    {
        Expression *Value;

        PunctuatorLocation Equals;

        SpecificStatementAccessor(EnumeratorWithValue)
    };

    // ;TypeListStatement
    struct TypeListStatement : Statement
    {
        TypeList *Types;

        SpecificStatementAccessor(TypeList)
    };

    // ;ParameterSpecifier
    struct ParameterSpecifier
    {
        enum Specifiers
        {
            ByRef = 0x1,
            ByVal = 0x2,
            Optional = 0x4,
            ParamArray = 0x8
        };

        Specifiers Opcode;
        Location TextSpan;
    };

    // ;ParameterSpecifierList
    struct ParameterSpecifierList : List<ParameterSpecifier, ParameterSpecifierList>
    {
        ParameterSpecifierList *
        HasSpecifier
        (
            // The target specifiers can be a bit mask of multiple specifiers.
            unsigned TargetSpecifiers
        );
    };

    struct OptionalParameter;
    // ;Parameter
    struct Parameter
    {
        Declarator *Name;
        Type *Type;
        ParameterSpecifierList *Specifiers;

        AttributeSpecifierList *Attributes;

        PunctuatorLocation As;
        Location TextSpan;

        unsigned char
                IsQueryIterationVariable:1, // This parameter is created to represent iteration variable for a Query
                IsQueryRecord:1;                    // This parameter is created to represent iteration variable - record for a Query (meaningless if IsQueryIterationVariable isn't set)

        ParameterAccessor(Optional)
    };

    // ;OptionalParameter
    struct OptionalParameter : Parameter
    {
        Expression *DefaultValue;
        PunctuatorLocation Equals;

        SpecificParameterAccessor(Optional)
    };

    // ;ParameterList
    struct ParameterList : List<Parameter, ParameterList>
    {
    };

    // ;MethodSignatureStatement
    struct MethodSignatureStatement : Statement
    {
        SpecifierList *Specifiers;
        IdentifierDescriptor Name;
        ParameterList *Parameters;
        Type *ReturnType;

        AttributeSpecifierList *Attributes;
        AttributeSpecifierList *ReturnTypeAttributes;

        PunctuatorLocation MethodKind; // "Sub", "Get", etc.
        PunctuatorLocation LeftParen;
        PunctuatorLocation RightParen;
        PunctuatorLocation As;

        SpecificStatementAccessor(MethodSignature)
    };

    // ;MethodDeclarationStatement
    struct MethodDeclarationStatement : MethodSignatureStatement
    {
        NameList *Handles;
        NameList *Implements;
        GenericParameterList *GenericParameters;

        PunctuatorLocation HandlesOrImplements;
        PunctuatorLocation GenericOf;
        PunctuatorLocation GenericLeftParen;
        PunctuatorLocation GenericRightParen;

        SpecificStatementAccessor(MethodDeclaration)
    };

    // ;DelegateDeclarationStatement
    struct DelegateDeclarationStatement : MethodDeclarationStatement
    {
        PunctuatorLocation Delegate;

        SpecificStatementAccessor(DelegateDeclaration)
    };

    // ;MethodDefinitionStatement
    struct MethodDefinitionStatement : MethodDeclarationStatement
    {
        CodeBlockLocation BodyLocation;
        CodeBlockLocation EntireMethodLocation;
        bool HasOutlinableContent;
        MethodBodyStatement *Body;

        // If method definition has an end construct, TerminatingConstruct is it.
        Statement *TerminatingConstruct;
        bool HasProperTermination;

        // Declarations of static local variables must be available to declaration
        // semantics when the method definition is processed, and so are attached
        // here. Static locals are an abomination.
        StatementList *StaticLocalDeclarations;

        SpecificStatementAccessor(MethodDefinition)
    };

    // ;OperatorDefinitionStatement
    struct OperatorDefinitionStatement : MethodDefinitionStatement
    {
        tokens OperatorTokenType;
        Specifier *HangingSpecifier;  // Used for Widening and Narrowing specifiers that occurs after the 'Operator' token.

        SpecificStatementAccessor(OperatorDefinition)
    };

    // ;EventDeclarationStatement
    struct EventDeclarationStatement : MethodDeclarationStatement
    {
        // The location for the block event modifier, i.e. the 'Custom' modifier.
        // The location is in here and not in the BlockEventDeclarationStatement
        // so that in some error scenarios where we don't build a BlockEvent, we
        // still have the location for this modifier.
        //
        // 







        Location BlockEventModifier;

        SpecificStatementAccessor(EventDeclaration)
    };

    // ;BlockEventDeclarationStatement
    struct BlockEventDeclarationStatement : BlockStatement
    {
        EventDeclarationStatement *EventSignature;

        SpecificStatementAccessor(BlockEventDeclaration)
    };

    // ;ForeignMethodDeclarationStatement ( Dll Declare )
    struct ForeignMethodDeclarationStatement : MethodSignatureStatement
    {
        enum StringModes
        {
            None,
            ANSI,
            Unicode,
            Auto
        };

        Expression *Library;
        Expression *Alias;
        StringModes StringMode;

        PunctuatorLocation Declare;
        PunctuatorLocation Mode;
        PunctuatorLocation Lib;
        PunctuatorLocation Al;

        SpecificStatementAccessor(ForeignMethodDeclaration)
    };

    struct InitializerAutoPropertyDeclaration;
    struct NewAutoPropertyDeclaration;
    
    struct AutoPropertyInitialization : ParseTreeNode
    {
        enum Opcodes
        {
            #define DEF_AUTOPROP_INIT(opCode, type, castFunc) opCode, 
            #include "AutoPropertyTable.h"
            #undef DEF_AUTOPROP_INIT
        };
        
        Opcodes Opcode;

        Location TextSpan;

        AutoPropertyDeclarationAccessor(Initializer)
        AutoPropertyDeclarationAccessor(New)
    };

    // ;InitializerAutoPropertyDeclaration
    struct InitializerAutoPropertyDeclaration : AutoPropertyInitialization
    {
        Initializer *InitialValue;
        PunctuatorLocation Equals;

        SpecificAutoPropertyDeclarationAccessor(Initializer)
    };

    struct ObjectInitializerList;
    
    struct NewAutoPropertyDeclaration : AutoPropertyInitialization
    {
        // The arguments are stored both as trees and as text.
        ParenthesizedArgumentList Arguments;

        PunctuatorLocation New;

        // The initializer list that initializes the various
        // properties and fields of the object when using the
        // "With" syntax. Eg:
        //   Property x as new Customer With {.Id = 1, .Name = "A"}
        //
        ObjectInitializerList *ObjectInitializer;

        // The text for the arguments and the object initializer
        // list is stored here.
        //
        // This must be preserved as text for field initializers
        // between declaration semantics where symbols for the fields
        // are built and method body semantics where the initializer
        // is analyzed.
        //
        // NULL if no arguments and object initializer list or if there
        // are errors. Note that this is non-NULL even if parantheses for
        // the argument list is present with zero arguments.
        //
        DeferredText *DeferredInitializerText;

        PunctuatorLocation From;
        BracedInitializerList * CollectionInitializer;

        SpecificAutoPropertyDeclarationAccessor(New)
    };

    // ;PropertyStatement
    struct PropertyStatement : BlockStatement
    {
        SpecifierList *Specifiers;
        IdentifierDescriptor Name;
        ParameterList *Parameters;
        Type *PropertyType;
        NameList *Implements;

        AttributeSpecifierList *Attributes;
        AttributeSpecifierList *PropertyTypeAttributes;

        PunctuatorLocation Property;
        PunctuatorLocation LeftParen;
        PunctuatorLocation RightParen;
        PunctuatorLocation As;
        PunctuatorLocation Impl;

        SpecificStatementAccessor(Property)
    };

    // ;AutoPropertyStatement
    struct AutoPropertyStatement : PropertyStatement
    {
        AutoPropertyInitialization *pAutoPropertyDeclaration;
        SpecificStatementAccessor(AutoProperty)
    };

    // ;DeclaratorList
    struct DeclaratorList : List<Declarator, DeclaratorList>
    {
    };

    struct InitializerVariableDeclaration;
    struct NewVariableDeclaration;

    // ;VariableDeclaration
    struct VariableDeclaration : ParseTreeNode
    {
        enum Opcodes
        {
            #define DEF_VAR_DECL(OpCode, Type, CastFunc) OpCode,
            #include "VariableDeclarationTable.h"
            #undef DEF_VAR_DECL
        };

        Opcodes Opcode;
        Type *Type;

        DeclaratorList *Variables;

        Location TextSpan;

        PunctuatorLocation As;

        bool HasSyntaxError : 1;

        VariableDeclaration * AsVariableDeclaration()
        {
            return this;
        }
        
        VariableDeclarationAccessor(Initializer)
        VariableDeclarationAccessor(New)
    };

    // ;InitializerVariableDeclaration
    struct InitializerVariableDeclaration : VariableDeclaration
    {
        Initializer *InitialValue;
        PunctuatorLocation Equals;

        SpecificVariableDeclarationAccessor(Initializer)
    };

    // ;ObjectInitializerList
    // Used to represent the punctuator for "With" and the initializer
    // list for the following syntax: With {.Id = 1, .Name = "A"}.
    //
    // We could possibly use this same structure for the
    // NewObjectInitializerExpression node too. But then this will
    // increase the size of the NewObjectInitializerExpression node by
    // 8 bytes because it can get by with the "punctuator" location for
    // "With" vs requiring "Location" for ObjectInitializerList.
    struct ObjectInitializerList
    {
        // Storing the Location instead of a punctuator for the "With"
        // because in some cases the ObjectInitializerList needs to be
        // returned stand-alone rather as part of another construct/parsetree
        // node.
        Location With;

        // List of assignment initializers.
        BracedInitializerList *BracedInitializerList;
    };

    // ;NewVariableDeclaration
    struct NewVariableDeclaration : VariableDeclaration
    {
        // The arguments are stored both as trees and as text.
        ParenthesizedArgumentList Arguments;

        PunctuatorLocation New;

        // The initializer list that initializes the various
        // properties and fields of the object when using the
        // "With" syntax. Eg:
        //   Dim x as new Customer With {.Id = 1, .Name = "A"}
        //
        ObjectInitializerList *ObjectInitializer;

        // The text for the arguments and the object initializer
        // list is stored here.
        //
        // This must be preserved as text for field initializers
        // between declaration semantics where symbols for the fields
        // are built and method body semantics where the initializer
        // is analyzed.
        //
        // NULL if no arguments and object initializer list or if there
        // are errors. Note that this is non-NULL even if parantheses for
        // the argument list is present with zero arguments.
        //
        DeferredText *DeferredInitializerText;

        PunctuatorLocation From;
        BracedInitializerList * CollectionInitializer;
        
        SpecificVariableDeclarationAccessor(New)
    };

    // ;VariableDeclarationList
    struct VariableDeclarationList : List<VariableDeclaration, VariableDeclarationList>
    {
    };

    // Iterator for VariableDeclarations in a VariableDeclarationStatement
    class VariableDeclarationIter
    {
    public:
        VariableDeclarationIter( _In_ VariableDeclarationStatement *pVariableDeclarationStatement);
        VariableDeclaration *GetNext();        
        
    private:
        VariableDeclarationStatement *m_pCurrentVariableDeclarationStatement;
        VariableDeclarationList *m_pNextDeclarationList;

    };

    // ;VariableDeclarationStatement
    struct VariableDeclarationStatement : Statement
    {
        SpecifierList *Specifiers;
        VariableDeclarationList *Declarations;

        AttributeSpecifierList *Attributes;

        SpecificStatementAccessor(VariableDeclaration)
    };

    struct NamespaceImportDirective;
    struct AliasImportDirective;
    struct XmlNamespaceImportDirective;

    // ;ImportDirective
    struct ImportDirective : ParseTreeNode
    {
        enum Modes
        {
            #define DEF_IMPORT_DIRECTIVE(OpCode, Type, CastFunc) OpCode,
            #include "ImportDirectiveTable.h"
            #undef DEF_IMPORT_DIRECTIVE
        };

        Modes Mode;

        Location TextSpan;

        ImportDirectiveAccessor(Namespace)
        ImportDirectiveAccessor(Alias)
        ImportDirectiveAccessor(XmlNamespace)
    };

    struct NamespaceImportDirective : ImportDirective
    {
        Name *ImportedName;

        SpecificImportDirectiveAccessor(Namespace)
    };

    // ;NamespaceImportDirective
    struct AliasImportDirective : NamespaceImportDirective
    {
        IdentifierDescriptor Alias;

        PunctuatorLocation Equals;

        SpecificImportDirectiveAccessor(Alias)
    };

    struct XmlNamespaceImportDirective : ImportDirective
    {
        Expression *NamespaceDeclaration;       // xmlns:p = "ns"

        SpecificImportDirectiveAccessor(XmlNamespace)
    };

    // ;ImportsDirectiveList
    struct ImportDirectiveList : List<ImportDirective, ImportDirectiveList>
    {
    };

    // ;ImportsStatement
    struct ImportsStatement : Statement
    {
        ImportDirectiveList *Imports;

        SpecificStatementAccessor(Imports)
    };

    // ********** End Declaration Statements **********

    // ;RegionStatement
    struct RegionStatement : BlockStatement
    {
        Expression *Title;

        PunctuatorLocation Region;

        SpecificStatementAccessor(Region)
    };

    // ********** Begin Executable Statements **********

    struct ExpressionBlockStatement : ExecutableBlockStatement
    {
        Expression *Operand;

        SpecificStatementAccessor(ExpressionBlock)
    };

    // ;ExpressionStatement
    struct ExpressionStatement : Statement
    {
        Expression *Operand;

        SpecificStatementAccessor(Expression)
    };

    struct IfStatement : ExpressionBlockStatement
    {
        PunctuatorLocation Then;

        SpecificStatementAccessor(If)
    };

    struct ElseIfStatement : IfStatement
    {
        IfStatement *ContainingIf;

        SpecificStatementAccessor(ElseIf)
    };

    struct ElseStatement : ExecutableBlockStatement
    {
        IfStatement *ContainingIf;

        SpecificStatementAccessor(Else)
    };

    struct SelectStatement : ExpressionBlockStatement
    {
        PunctuatorLocation Case;

        SpecificStatementAccessor(Select)
    };

    struct TopTestDoStatement : ExpressionBlockStatement
    {
        PunctuatorLocation WhileOrUntil;

        SpecificStatementAccessor(TopTestDo)
    };

    struct BottomTestLoopStatement : ExpressionStatement
    {
        PunctuatorLocation WhileOrUntil;

        SpecificStatementAccessor(BottomTestLoop)
    };

    // ;CatchStatement
    struct CatchStatement : ExecutableBlockStatement
    {
        IdentifierDescriptor Name;
        Type *Type;
        Expression *WhenClause;

        PunctuatorLocation As;
        PunctuatorLocation When;

        ExecutableBlockStatement *ContainingTry;

        SpecificStatementAccessor(Catch)
    };

    // ;FinallyStatement
    struct FinallyStatement : ExecutableBlockStatement
    {
        ExecutableBlockStatement *ContainingTry;

        SpecificStatementAccessor(Finally)
    };

    struct ForStatement : ExecutableBlockStatement
    {
        Expression *ControlVariable;
        Expression *NextVariable;

        // set only if control variable declared in the For Statement
        VariableDeclarationStatement *ControlVariableDeclaration;

        unsigned short ResumeIndexForEndNext;

        SpecificStatementAccessor(For)
    };

    struct ForFromToStatement : ForStatement
    {
        Expression *InitialValue;
        Expression *FinalValue;
        Expression *IncrementValue;

        PunctuatorLocation Equals;
        PunctuatorLocation To;
        PunctuatorLocation Step;

        SpecificStatementAccessor(ForFromTo)
    };

    struct ForEachInStatement : ForStatement
    {
        Expression *Collection;

        PunctuatorLocation Each;
        PunctuatorLocation In;

        SpecificStatementAccessor(ForEachIn)
    };

    struct UsingStatement : ExecutableBlockStatement
    {
        Expression *ControlExpression;
        Statement *ControlVariableDeclaration;

        SpecificStatementAccessor(Using)
    };


    struct EndNextStatement : Statement
    {
        ExpressionList *Variables;

        SpecificStatementAccessor(EndNext)
    };

    struct LabelReferenceStatement : Statement
    {
        IdentifierDescriptor Label;

        bool LabelIsLineNumber;

        SpecificStatementAccessor(LabelReference)
    };

    struct OnErrorStatement : LabelReferenceStatement
    {
        enum GotoTypes
        {
            GotoLabel,
            Next,
            Zero,
            MinusOne
        };

        GotoTypes GotoType;

        PunctuatorLocation Error;
        PunctuatorLocation GotoOrResume;

        int OnErrorIndex;

        SpecificStatementAccessor(OnError)
    };

    struct ResumeStatement : LabelReferenceStatement
    {
        enum ResumeTypes
        {
            None,
            ResumeLabel,
            Next
        };

        ResumeTypes ResumeType;

        SpecificStatementAccessor(Resume)
    };

    struct CallStatement : Statement
    {
        Expression *Target;
        ParenthesizedArgumentList Arguments;

        PunctuatorLocation LeftParenthesis;

        // The "Call" keyword can be explicit or implied.
        bool CallIsExplicit;

        SpecificStatementAccessor(Call)
    };

     template <class ElementType, class ListType> class ListIter
    {
    public:
        ListIter( _In_ List<ElementType, ListType> *pList);
        
        void SetList( _In_ List<ElementType, ListType> *pList)
        {
            m_pNextListElement = pList;
        }
        
        ElementType *GetNext();       
        
    private:
        List<ElementType, ListType> *m_pNextListElement;
    };

    struct RaiseEventStatement : Statement
    {
        IdentifierDescriptor Event;
        ParenthesizedArgumentList Arguments;

        PunctuatorLocation LeftParenthesis;

        SpecificStatementAccessor(RaiseEvent)
    };

    struct AssignmentStatement : Statement
    {
        Expression *Target;
        Expression *Source;

        PunctuatorLocation Operator;

        SpecificStatementAccessor(Assignment)
    };

    struct EraseStatement : Statement
    {
        ExpressionList *Arrays;

        SpecificStatementAccessor(Erase)
    };

    struct AssignMidStatement : Statement
    {
        Expression  *Target;
        Expression  *Start;
        Expression  *Length;
        Expression  *Source;

        typeChars TypeCharacter;

        PunctuatorLocation LeftParen;
        PunctuatorLocation FirstComma;
        PunctuatorLocation SecondComma;
        PunctuatorLocation RightParen;
        PunctuatorLocation Equals;

        SpecificStatementAccessor(AssignMid);
    };

    struct RedimStatement : Statement
    {
        bool HasPreserve;
        ExpressionList *Redims;

        PunctuatorLocation Preserve;
        SpecificStatementAccessor(Redim)
    };

    struct Case;

    struct CaseList : List<Case, CaseList>
    {
    };

    struct CaseStatement : ExecutableBlockStatement
    {
        CaseList *Cases;

        SpecificStatementAccessor(Case)
    };

    struct HandlerStatement : Statement
    {
        Expression *Event;
        Expression *Delegate;

        PunctuatorLocation Comma;

        SpecificStatementAccessor(Handler)
    };


    // ********** End Executable Statements **********

    // ********** Begin Conditional Compilation Statements ***********

    // ;CCConstStatement
    struct CCConstStatement : NamedValueStatement
    {
        PunctuatorLocation Const;

        SpecificStatementAccessor(CCConst)
    };

    // ;CCBranchStatement
    struct CCBranchStatement : ExpressionStatement
    {
        // This type represents a Conditional Compilation branch expression
        // such as #if, #else etc ...  The BranchTaken member determines
        // if this branch was compiled into the code
        bool BranchTaken;

        // In the case where the branch is not taken, this location will be the
        // span of text which was skipped by the parser
        Location SkippedLocation;

        SpecificStatementAccessor(CCBranch)
    };

    // ;CCIfStatement
    struct CCIfStatement : CCBranchStatement
    {
        PunctuatorLocation IfOrElseIf;
        PunctuatorLocation Then;

        // If the CCIf has an end construct, TerminatingConstruct is it.
        // (This applies only to CCIf opcodes, not CCElseIf.)
        Statement *TerminatingConstruct;

        SpecificStatementAccessor(CCIf)
    };

    // ;;CCElseStatement
    //
    // Represents an #else block in the code
    struct CCElseStatement : CCBranchStatement
    {
        SpecificStatementAccessor(CCElse)
    };

    // ;CCEndStatement
    struct CCEndStatement : EndBlockStatement
    {
        PunctuatorLocation End;

        SpecificStatementAccessor(CCEnd)
    };

    // ********** End Conditional Compilation Statements ***********

    struct Comment
    {
        Location TextSpan;

        // The spelling includes the leading ' or REM.
        WCHAR *Spelling;
        size_t LengthInCharacters;
        bool IsRem;

        // Was this comment created from an XMLDocComment token?
        // This is independent of whether the parser is interpreting
        // XMLDoc comments or not.
        bool IsGeneratedFromXMLDocToken;
    };

    struct CommentList : List<Comment, CommentList>
    {
    };

    // ********** End Statements **********

    // ********** Begin Types **********
    struct AlreadyBoundType;
    struct AlreadyBoundDelayCalculatedType;

    struct NamedType;
    struct ArrayType;
    struct ArrayWithSizesType;
    struct NullableType;

    // ;Type
    struct Type : ParseTreeNode
    {
        enum Opcodes
        {
            #define DEF_TYPE(OpCode, Type, CastFunc) OpCode,
            #include "ParseTreeTypeTable.h"
            #undef DEF_TYPE
        };

        Opcodes Opcode;
        Location TextSpan;

        Type * AsType()
        {
            return this;
        }

        TypeAccessor(AlreadyBound)
        TypeAccessor(AlreadyBoundDelayCalculated)
        TypeAccessor(Named)
        TypeAccessor(Array)
        TypeAccessor(ArrayWithSizes)
        TypeAccessor(Nullable)
    };

    // This is a type that was already interpreted, but needed to be stored in
    // a parse tree for later interpretation. This is used for SQL to store temp variables
    // on the stack, then reference them later in an expression.
    struct AlreadyBoundType : Type
    {
        BCSYM * BoundType;

        SpecificTypeAccessor(AlreadyBound)
    };

    // This is a type that was already interpreted, but needed to be stored in
    // a parse tree for later interpretation. This is used for SQL to store temp variables
    // on the stack, then reference them later in an expression.
    struct AlreadyBoundDelayCalculatedType : Type
    {
        BCSYM * (*DelayedCalculation)(void * Parameter);

        void* Parameter;

        BCSYM * GetBoundType()
        {
            BCSYM *  BoundType = DelayedCalculation(Parameter);
            return BoundType;
        }

        SpecificTypeAccessor(AlreadyBoundDelayCalculated)

    };

    // ;NamedType
    struct NamedType : Type
    {
        Name *TypeName;

        SpecificTypeAccessor(Named)
    };

    // ;ArrayType
    struct ArrayType : Type
    {
        // ElementType is NULL if the array specifier occurs as part of a declarator.
        Type *ElementType;
        unsigned Rank;

        PunctuatorLocation LeftParen;

        SpecificTypeAccessor(Array)
    };

    // ;ArrayWithSizesType
    struct ArrayWithSizesType : ArrayType
    {
        //ExpressionList *Sizes;
        ArrayDimList    *Dims;

        SpecificTypeAccessor(ArrayWithSizes)
    };

    struct NullableType : Type
    {
        Type *ElementType;
        PunctuatorLocation QuestionMark;
        SpecificTypeAccessor(Nullable);
    };

    // ********** End Types **********

    // ********** Begin Names **********

    struct SimpleName;
    struct SimpleWithArgumentsName;
    struct QualifiedName;
    struct QualifiedWithArgumentsName;

    // ;Name
    struct Name : ParseTreeNode
    {
        enum Opcodes
        {
            #define DEF_NAME(OpCode,Type, CastFunc) OpCode,
            #include "NameTable.h"
            #undef DEF_NAME
        };

        Opcodes Opcode;
        Location TextSpan;

        unsigned GetDelimitedNameCount();

        bool IsSimple()
        {
            return Opcode == Simple || Opcode == SimpleWithArguments;
        }

        bool IsQualified()
        {
            return Opcode == Qualified || Opcode == QualifiedWithArguments;
        }

        Name * AsName()
        {
            return this;
        }
                

        NameAccessor(Simple)
        NameAccessor(SimpleWithArguments)
        NameAccessor(Qualified)
        NameAccessor(QualifiedWithArguments)
    };

    struct GenericArguments
    {
        enum Opcodes
        {
            WithoutTypes,
            WithTypes
        };

        Opcodes Opcode;
        TypeList *Arguments;

        PunctuatorLocation Of;
        PunctuatorLocation LeftParen;
        PunctuatorLocation RightParen;
    };

    // ;SimpleName
    struct SimpleName : Name
    {
        IdentifierDescriptor ID;

        SpecificNameAccessor(Simple)
    };

    // ;SimpleWithArgumentsName
    struct SimpleWithArgumentsName : SimpleName
    {
        GenericArguments Arguments;

        SpecificNameAccessor(SimpleWithArguments)
    };

    // ;QualifiedName
    struct QualifiedName : Name
    {
        Name *Base;
        IdentifierDescriptor Qualifier;

        PunctuatorLocation Dot;

        SpecificNameAccessor(Qualified)
    };

    // ;QualifiedWithArgumentsName
    struct QualifiedWithArgumentsName : QualifiedName
    {
        GenericArguments Arguments;

        SpecificNameAccessor(QualifiedWithArguments)
    };

    // ;NameList
    struct NameList : List<Name, NameList>
    {
    };

    // ;TypeList
    struct TypeList : List<Type, TypeList>
    {
    };

    // ********** End Names **********

    // ********** Begin Expressions **********

    struct UnaryExpression;
    struct ParenthesizedExpression;
    struct BinaryExpression;
    struct NameExpression;
    struct CallOrIndexExpression;
    struct QualifiedExpression;
    struct GenericQualifiedExpression;
    struct IntegralLiteralExpression;
    struct CharacterLiteralExpression;
    struct BooleanLiteralExpression;
    struct FloatingLiteralExpression;
    struct DateLiteralExpression;
    struct DecimalLiteralExpression;
    struct StringLiteralExpression;
    struct TypeValueExpression;

    struct FromExpression;
    struct AggregateExpression;
    struct QueryAggregateGroupExpression;
    struct CrossJoinExpression;
    struct WhereExpression;
    struct SelectExpression;
    struct GroupByExpression;
    struct AggregationExpression;
    struct QueryOperatorCallExpression;
    struct DistinctExpression;
    struct OrderByExpression;
    struct LinqSourceExpression;
    struct InnerJoinExpression;
    struct GroupJoinExpression;
    struct WhileExpression;
    struct SkipTakeExpression;
    struct LinqOperatorExpression;

    struct ImplicitConversionExpression;

    struct ConversionExpression;
    struct TypeReferenceExpression;
    struct GetTypeExpression;
    struct GetXmlNamespaceExpression;
    struct NewExpression;

    struct ArrayInitializerExpression;
    struct NewArrayInitializerExpression;
    struct ObjectInitializerExpression;
    struct NewObjectInitializerExpression;
    struct CollectionInitializerExpression;

    struct AlreadyBoundExpression;
    struct AlreadyBoundSymbolExpression;

    struct DeferredExpression;
    struct AddressOfExpression;
    struct LambdaExpression;
    struct XmlExpression;
    struct XmlNameExpression;
    struct XmlDocumentExpression;
    struct XmlElementExpression;
    struct XmlAttributeExpression;
    struct XmlCharDataExpression;
    struct XmlReferenceExpression;
    struct XmlPIExpression;
    struct XmlEmbeddedExpression;
    struct IIfExpression;

    // ;Expression
    struct Expression : ParseTreeNode
    {
        enum Opcodes
        {
            #define DEF_EXPRESSION(OpCode, Type, CastFunc) OpCode,
            #define DEF_EXPRESSION_ENUM_VALUE(Name, Value) Name = Value,
            #include "ExpressionTable.h"
            #undef DEF_EXPRESSION_ENUM_VALUE
            #undef DEF_EXPRESSION
            
        };

        bool
        IsQueryExpression
        (
        );
        
        bool
        ContainsArgumentList();
        
        ArgumentList*
        GetArgumentList();

        Opcodes Opcode;

        PunctuatorLocation FirstPunctuator;

        Location TextSpan;

        Expression * AsExpression()
        {
            return this;
        }

        ExpressionAccessor(Unary)
        ExpressionAccessor(Parenthesized)
        ExpressionAccessor(Binary)
        ExpressionAccessor(Name)
        ExpressionAccessor(CallOrIndex)
        ExpressionAccessor(IIf)
        ExpressionAccessor(Qualified)
        ExpressionAccessor(GenericQualified)
        ExpressionAccessor(IntegralLiteral)
        ExpressionAccessor(CharacterLiteral)
        ExpressionAccessor(BooleanLiteral)
        ExpressionAccessor(FloatingLiteral)
        ExpressionAccessor(DateLiteral)
        ExpressionAccessor(DecimalLiteral)
        ExpressionAccessor(StringLiteral)
        ExpressionAccessor(TypeValue)

        ExpressionAccessor(From)
        ExpressionAccessor(Aggregate)
        ExpressionAccessor(QueryAggregateGroup)
        ExpressionAccessor(CrossJoin)
        ExpressionAccessor(Where)
        ExpressionAccessor(Select)
        ExpressionAccessor(GroupBy)
        ExpressionAccessor(Aggregation)
        ExpressionAccessor(QueryOperatorCall)
        ExpressionAccessor(Distinct)
        ExpressionAccessor(OrderBy)
        ExpressionAccessor(LinqSource)
        ExpressionAccessor(InnerJoin)
        ExpressionAccessor(GroupJoin)
        ExpressionAccessor(While)
        ExpressionAccessor(SkipTake)
        ExpressionAccessor(LinqOperator)

        ExpressionAccessor(ImplicitConversion)

        ExpressionAccessor(Conversion)
        ExpressionAccessor(TypeReference)
        ExpressionAccessor(GetType)
        ExpressionAccessor(GetXmlNamespace)
        ExpressionAccessor(New)

        ExpressionAccessor(ArrayInitializer)
        ExpressionAccessor(NewArrayInitializer)
        ExpressionAccessor(ObjectInitializer)
        ExpressionAccessor(NewObjectInitializer)
        ExpressionAccessor(CollectionInitializer)

        ExpressionAccessor(AlreadyBound)
        ExpressionAccessor(AlreadyBoundSymbol)
        ExpressionAccessor(Deferred)
        ExpressionAccessor(AddressOf)
        ExpressionAccessor(Lambda)
        ExpressionAccessor(XmlName)
        ExpressionAccessor(XmlDocument)
        ExpressionAccessor(Xml)
        ExpressionAccessor(XmlElement)
        ExpressionAccessor(XmlAttribute)
        ExpressionAccessor(XmlCharData)
        ExpressionAccessor(XmlReference)
        ExpressionAccessor(XmlPI)
        ExpressionAccessor(XmlEmbedded)
    };

    struct UnaryExpression : Expression
    {
        Expression *Operand;

        SpecificExpressionAccessor(Unary)
    };

    struct ParenthesizedExpression : UnaryExpression
    {
        bool IsRightParenMissing;  // is the right paren missing?

        SpecificExpressionAccessor(Parenthesized)
    };

    struct BinaryExpression : Expression
    {
        Expression *Left;
        Expression *Right;

        SpecificExpressionAccessor(Binary)
    };

    struct NameExpression : Expression
    {
        IdentifierDescriptor Name;

        SpecificExpressionAccessor(Name)
    };

    struct CallOrIndexExpression : Expression
    {
        Expression *Target;
        ParenthesizedArgumentList Arguments;
        bool AlreadyResolvedTarget;

        SpecificExpressionAccessor(CallOrIndex)
    };

    struct Argument : ParseTreeNode
    {
        Location TextSpan;
        Expression *Value;

        // 






        IdentifierDescriptor Name;
        PunctuatorLocation ColonEquals;
        Expression* lowerBound;     //intLiteral 0 if any, used only for redim and new
        PunctuatorLocation To;      //used only for redim and new

    #if IDE 
        // The following fields are only used for attribue arguments, and they indicate
        // the start and end indexes of the text representing the value expression in
        // the Attribute.DeferredRepresentationAsCall field.
        long ValueStartPosition;
        long ValueWidth;
    #endif
    };

    struct ArgumentList : List<Argument, ArgumentList>
    {
    };

    struct ExpressionList : List<Expression, ExpressionList>
    {
    };

    struct ArrayDim : ParseTreeNode
    {
        Location TextSpan;
        Expression* lowerBound;     //intLiteral 0 if any
        PunctuatorLocation To;
        Expression *upperBound;
    };

    struct ArrayDimList : List<ArrayDim, ArrayDimList>
    {
    };

    struct QualifiedExpression : Expression
    {
        Expression *Base;
        Expression *Name; // this member must always be assigned to a non-NULL value!

        SpecificExpressionAccessor(Qualified)
    };

    struct GenericQualifiedExpression : Expression
    {
        Expression *Base;
        GenericArguments Arguments;

        SpecificExpressionAccessor(GenericQualified)
    };

    struct IntegralLiteralExpression : Expression
    {
        enum Bases
        {
            Octal,
            Decimal,
            Hexadecimal
        };

        __int64 Value;
        Bases Base;
        typeChars TypeCharacter;

        SpecificExpressionAccessor(IntegralLiteral)
    };

    struct CharacterLiteralExpression : Expression
    {
        WCHAR Value;

        SpecificExpressionAccessor(CharacterLiteral)
    };

    struct BooleanLiteralExpression : Expression
    {
        bool Value;

        SpecificExpressionAccessor(BooleanLiteral)
    };

    struct FloatingLiteralExpression : Expression
    {
        double Value;
        typeChars TypeCharacter;

        SpecificExpressionAccessor(FloatingLiteral)
    };

    struct DateLiteralExpression : Expression
    {
        __int64 Value;

        SpecificExpressionAccessor(DateLiteral)
    };

    struct DecimalLiteralExpression : Expression
    {
        DECIMAL Value;
        typeChars TypeCharacter;

        SpecificExpressionAccessor(DecimalLiteral)
    };

    // ;StringLiteralExpression
    struct StringLiteralExpression : Expression
    {
        WCHAR *Value;
        size_t LengthInCharacters;

        SpecificExpressionAccessor(StringLiteral)
    };
    struct XmlNameExpression : Expression
    {
        IdentifierDescriptor Prefix;
        IdentifierDescriptor LocalName;
        bool IsBracketed:1;
        bool IsElementName:1;

        SpecificExpressionAccessor(XmlName)
    };

    struct XmlEmbeddedExpression : UnaryExpression
    {
        ExpressionList *XmlRoots;
        unsigned AllowEmdedded:1;

        SpecificExpressionAccessor(XmlEmbedded)
    };

    struct XmlExpression : Expression
    {
        ExpressionList *Content;

        SpecificExpressionAccessor(Xml)
    };

    struct XmlDocumentExpression : XmlExpression
    {
        ExpressionList *Attributes;

        Location DeclTextSpan;

        SpecificExpressionAccessor(XmlDocument);
    };

    struct XmlElementExpression : XmlExpression
    {
        Expression *Name;
        ExpressionList *Attributes;
        XmlElementExpression *Parent;
        unsigned IsEmpty:1;         // <a/>
        unsigned HasBeginEnd:1;     // Found '<' at the beginning of the end element
        unsigned IsWellFormed:1;    // Element parsed completely but names may not match or may be short end tag
        unsigned HasEndName:1;      // Found some name in the end element, i.e. not short end tag
        unsigned NamesMatch:1;      // Names in begin and end elements match
        unsigned HasWSBeforeName:1; // <  Name
        unsigned HasWSBeforeEndName:1; // </ Name
        unsigned HasSignificantContent:1; // Content is significant, i.e. not just ws

        PunctuatorLocation BeginEndElementLoc;  // remove - redundant with EndNameSpan
        Location EndNameTextSpan;

        SpecificExpressionAccessor(XmlElement)
    };

    struct XmlAttributeExpression : Expression
    {
        Expression *Name;
        Expression *Value;
        tokens Delimiter;
        unsigned PrecededByWhiteSpace:1;
        unsigned IsImportedNamespace:1;

        PunctuatorLocation StartDelimiterLoc; // remove - use Value Textspan
        PunctuatorLocation EndDelimiterLoc;   // remove - use Value Textspan

        SpecificExpressionAccessor(XmlAttribute)
    };

    struct XmlCharDataExpression : StringLiteralExpression
    {
        unsigned IsWhitespace:1;
        unsigned IsSignificant:1;
        unsigned IsMovable:1;

        SpecificExpressionAccessor(XmlCharData)
    };

    struct XmlReferenceExpression : XmlCharDataExpression
    {
        STRING *Name;

        unsigned IsHexCharRef:1;
        unsigned HasSemiColon:1;
        unsigned IsDefined:1;

        SpecificExpressionAccessor(XmlReference)
    };

    struct XmlPIExpression : XmlExpression
    {
        Expression *Name;
        unsigned MissingWhiteSpace:1;

        SpecificExpressionAccessor(XmlPI)
    };

    struct TypeValueExpression : Expression
    {
        Expression *Value;
        Type *TargetType;
        bool HasIs;                     // TypeOf (value) Is (Target)

        SpecificExpressionAccessor(TypeValue)
    };


    struct LinqExpression : Expression
    {
    };


    struct FromItem
    {
        VariableDeclaration *ControlVariableDeclaration;

        unsigned char IsLetControlVar:1;
        unsigned char SuppressShadowingChecks:1; // shouldn't be set if IsLetControlVar == 0

        Expression * Source;

        Location TextSpan;

        PunctuatorLocation InOrEq;
    };

    struct FromList: List<FromItem, FromList>
    {
    };

    struct FromExpression : LinqExpression
    {
        FromList *FromItems;

        bool StartingQuery:1; // indicates first FROM in an expression
        bool ImplicitFrom:1; // indicates that the FROM keyword is implicit and shouldn't be prettylisted
        bool ImplicitLetforAggregate:1;

        SpecificExpressionAccessor(From)
    };


    struct LinqOperatorExpression : LinqExpression
    {
        LinqExpression * Source;
        SpecificExpressionAccessor(LinqOperator)
    };

    struct CrossJoinExpression : LinqOperatorExpression
    {
        LinqExpression * JoinTo;  // this can be only FromExpression or another CrossJoinExpression

        SpecificExpressionAccessor(CrossJoin)
    };


    // a In AA
    struct LinqSourceExpression : LinqExpression
    {
        VariableDeclaration *ControlVariableDeclaration;

        // 'In' location is stored in the FirstPunctuator
        Expression * Source;

        SpecificExpressionAccessor(LinqSource)
    };

    // <Source> Join <Source> On ...
    struct InnerJoinExpression : LinqOperatorExpression
    {
        // 'Join' location is stored in the FirstPunctuator
        LinqExpression * JoinTo; // this can be only LinqSourceExpression, InnerJoinExpression or GroupJoinExpression

        PunctuatorLocation On;

        Expression *Predicate;

        SpecificExpressionAccessor(InnerJoin)
    };


    struct InitializerList;

    // <Source> Group Join <Source> On ... Into ...
    struct GroupJoinExpression : InnerJoinExpression
    {
        // 'Group' location is stored in the FirstPunctuator
        bool HaveJoin;
        PunctuatorLocation Join;

        PunctuatorLocation Into;

        InitializerList *Projection; // This list contains Aggregation and Name expressions only

        SpecificExpressionAccessor(GroupJoin)
    };


    struct FilterExpression : LinqOperatorExpression
    {
        Expression *Predicate;
    };


    struct WhereExpression : FilterExpression
    {
        SpecificExpressionAccessor(Where)
    };

    struct WhileExpression : FilterExpression
    {
        PunctuatorLocation While;
        SpecificExpressionAccessor(While)
    };

    struct SkipTakeExpression : LinqOperatorExpression
    {
        Expression *Count;
        SpecificExpressionAccessor(SkipTake)
    };

    struct DistinctExpression : LinqOperatorExpression
    {
        SpecificExpressionAccessor(Distinct)
    };

    struct SelectExpression : LinqOperatorExpression
    {
        InitializerList *Projection;
        bool ForceNameInferenceForSingleElement;

        SpecificExpressionAccessor(Select)
    };


    struct OrderByItem
    {
        Expression *OrderExpression;

        bool IsAscending:1;
        bool IsDescending:1;

        Location TextSpan;

        PunctuatorLocation AscendingDescending;
    };

    struct OrderByList: List<OrderByItem, OrderByList>
    {
    };


    struct OrderByExpression : LinqOperatorExpression
    {
        OrderByList *OrderByItems;

        bool WithBy:1;

        PunctuatorLocation By;

        SpecificExpressionAccessor(OrderBy)
    };


    struct AggregationExpression : LinqExpression
    {
        IdentifierDescriptor AggFunc;

        // LParen location is stored in FirstPunctuator
        Expression * Argument; // NULL if missing
        PunctuatorLocation RParenLocation;

        unsigned char HaveLParen:1;
        unsigned char HaveRParen:1;

        SpecificExpressionAccessor(Aggregation)
    };

    // Special node to indicate special semantics for CallOrIndexExpression
    // This node is never created by the Parser, Semantics creates it while interpreting a Query
    // It is not to be used/created anywhere, but in QuerySemantics.cpp
    struct QueryOperatorCallExpression : LinqExpression
    {
        CallOrIndexExpression * operatorCall;

        SpecificExpressionAccessor(QueryOperatorCall)
    };


    struct GroupByExpression : LinqOperatorExpression
    {
        InitializerList *Element;

        PunctuatorLocation By;

        InitializerList *Key;

        PunctuatorLocation Into;

        InitializerList *Projection; // This list contains Aggregation and Name expressions only

        unsigned char HaveBy:1;
        unsigned char HaveInto:1;

        SpecificExpressionAccessor(GroupBy)
    };


    // AGGREGATE ... INTO <agg list>
    struct AggregateExpression : LinqOperatorExpression
    {
        // the Source may be NULL if this is stand-alone operator

        LinqExpression * AggSource;  // this can be FromExpression or any LinqOperatorExpression

        PunctuatorLocation Into;

        InitializerList *Projection; // This list contains Aggregation expressions only

        unsigned char HaveInto:1;

        SpecificExpressionAccessor(Aggregate)
    };

    // Special node !!!
    // This node is never created by the Parser, Semantics creates it while interpreting a Query
    // It is not to be used/created anywhere, but in QuerySemantics.cpp
    struct QueryAggregateGroupExpression : LinqExpression
    {
        LinqExpression * Group;

        // filled during interpretation
        ParseTree::IdentifierDescriptor ControlVariableName;
        ParseTree::AlreadyBoundType *ControlVariableType;
        unsigned QueryFlags;

        SpecificExpressionAccessor(QueryAggregateGroup)
    };


    struct ImplicitConversionExpression : Expression
    {
        Expression *Value;
        BCSYM * TargetType;

        SpecificExpressionAccessor(ImplicitConversion)
    };


    struct ConversionExpression : TypeValueExpression
    {
        PunctuatorLocation Comma;

        SpecificExpressionAccessor(Conversion)
    };

    struct TypeReferenceExpression : Expression
    {
        Type *ReferencedType;

        SpecificExpressionAccessor(TypeReference)
    };

    struct GetTypeExpression : Expression
    {
        Type *TypeToGet;

        SpecificExpressionAccessor(GetType)
    };

    struct GetXmlNamespaceExpression : Expression
    {
        IdentifierDescriptor Prefix;

        SpecificExpressionAccessor(GetXmlNamespace)
    };

    struct NewExpression : Expression
    {
        Type *InstanceType;
        ParenthesizedArgumentList Arguments;

        // Punctuator location not encoded because a general practice
        // seems to be if the punctuator term is always the start
        // of the node, it seems to be inferred from the start location
        // of the node.
        // So in this case, the location for "New" can be inferred from
        // the start of the this node. Exposing this information via an
        // accessor.
        // PunctuatorLocation New;

        Location GetLocationOfNew()
        {
            Location Loc;
            Loc.m_lBegLine = TextSpan.m_lBegLine;
            Loc.m_lBegColumn = TextSpan.m_lBegColumn;
            Loc.m_lEndLine = TextSpan.m_lBegLine;
            Loc.m_lEndColumn = TextSpan.m_lBegColumn + g_tkKwdNameLengths[tkNEW] - 1;

            return Loc;
        }

        SpecificExpressionAccessor(New)
    };

    // Node to represent the Array initializer without "New".
    // Eg: {1, 2, 3}, {{1,2},{3,4}}
    struct ArrayInitializerExpression : Expression
    {
        // List of array element values.
        BracedInitializerList *Elements;

        SpecificExpressionAccessor(ArrayInitializer)
    };
    
    // Node to represent Array initializer with "New".
    // Eg: New Integer() {1, 2, 3}
    //
    struct NewArrayInitializerExpression : ArrayInitializerExpression
    {
        Type *ArrayType;

        // Punctuator location not encoded because an existing general
        // practice seems to be if the punctuator term is always the start
        // of the node, it seems to be inferred from the start location
        // of the node.
        // So in this case, the location for "New" can be inferred from
        // the start of the this node. Exposing this information via an
        // accessor.
        // PunctuatorLocation New;

        Location GetLocationOfNew()
        {
            Location Loc;
            Loc.m_lBegLine = TextSpan.m_lBegLine;
            Loc.m_lBegColumn = TextSpan.m_lBegColumn;
            Loc.m_lEndLine = TextSpan.m_lBegLine;
            Loc.m_lEndColumn = TextSpan.m_lBegColumn + g_tkKwdNameLengths[tkNEW] - 1;

            return Loc;
        }

        SpecificExpressionAccessor(NewArrayInitializer)
    };

    // Node to represent an Object initializer without "New".
    // Eg: With {.x = 1, .y = 2, .z = z}
    struct ObjectInitializerExpression : Expression
    {
        PunctuatorLocation With;

        // List of assignment and expression initializers.
        BracedInitializerList *InitialValues;

        Location GetLocationOfWith()
        {
            Location Loc;
            Loc.m_lBegLine = TextSpan.m_lBegLine + With.Line;
            Loc.m_lBegColumn = With.Column;
            Loc.m_lEndLine = Loc.m_lBegLine;
            Loc.m_lEndColumn = Loc.m_lBegColumn + g_tkKwdNameLengths[tkWITH] - 1;

            return Loc;
        }

        SpecificExpressionAccessor(ObjectInitializer)
    };

    // Node to represent an Object initializer with "New".
    // Eg: New Point with {.x = 1, .y = 2, .z = z}
    //     New Point(1,2) with {.x = 1, .y = 2}
    //     New with {.x = 1, .y = 2, z}
    struct NewObjectInitializerExpression : ObjectInitializerExpression
    {
        NewExpression *NewExpression;

        Location GetLocationOfNew()
        {
            if (NewExpression)
            {
                return NewExpression->GetLocationOfNew();
            }

            Location Loc;
            Loc.m_lBegLine = TextSpan.m_lBegLine;
            Loc.m_lBegColumn = TextSpan.m_lBegColumn;
            Loc.m_lEndLine = TextSpan.m_lBegLine;
            Loc.m_lEndColumn = TextSpan.m_lBegColumn + g_tkKwdNameLengths[tkNEW] - 1;

            return Loc;
        }

        bool NoWithScope; // This flag indicates that Anonymous Type initialization shouldn't create its own WITH scope (used by Queries, shouldn't be set if NewExpression!=NULL)
        bool QueryErrorMode; // use Query specific error messages

        SpecificExpressionAccessor(NewObjectInitializer)
    };

    //Node to represent an expression of the Form:
    //
    //new TypeName(args) From {e1, ..., en}
    struct CollectionInitializerExpression : Expression
    {
        //The new part of the expression.
        NewExpression * NewExpression;
        //The location of the "From" keyword
        PunctuatorLocation From;
        //The initializer (the {e1,..., en} part). 
        BracedInitializerList* Initializer;

        Location GetLocationOfFrom()
        {
            Location Loc;
            Loc.m_lBegLine = TextSpan.m_lBegLine + From.Line;
            Loc.m_lBegColumn = From.Column;
            Loc.m_lEndLine = Loc.m_lBegLine;
            Loc.m_lEndColumn = Loc.m_lBegColumn + g_tkKwdNameLengths[tkFROM] - 1;

            return Loc;
        }

        SpecificExpressionAccessor(CollectionInitializer);
    };
    
    // This is an expression that was already interpreted, but needed to be stored in
    // a parse tree for later interpretation. This is used for SQL to store temp variables
    // on the stack, then reference them later in an expression.
    struct AlreadyBoundExpression : Expression
    {
        ILTree::Expression * BoundExpression;

        SpecificExpressionAccessor(AlreadyBound)
    };

    // This is an expression that wraps a symbol that was already interpreted, but needed to be stored in
    // a parse tree. This is used for AddressOf generation in WithEvents variables and other places.
    struct AlreadyBoundSymbolExpression : Expression
    {
        BCSYM * Symbol;
        ParseTree::Expression * BaseReference;   // optional
        bool UseBaseReferenceTypeAsSymbolContainerType;

        SpecificExpressionAccessor(AlreadyBoundSymbol)
    };

    // ;DeferredExpression - represents an expression that is to be (re)parsed later.
    struct DeferredExpression : Expression
    {
        Expression *Value;

        size_t TextLengthInCharacters;
        WCHAR Text[1];

        SpecificExpressionAccessor(Deferred)
    };

    // ;LambdaExpression - represents an inline function. There are five types:
    // Function() foo()                                -- !IsStatementLambda,  IsSingleLine,  IsFunction, !IsAsync
    // Sub() Expression                                -- !IsStatementLambda,  IsSingleLine, !IsFunction, !IsAsync
    // Sub() foo()                                     --  IsStatementLambda,  IsSingleLine, !IsFunction, !IsAsync
    // Function() : return foo() : End Function        --  IsStatementLambda, !IsSingleLine,  IsFunction, !IsAsync
    // Sub() : foo() : End Sub                         --  IsStatementLambda, !IsSingleLine, !IsFunction, !IsAsync
    // Async Function() foo()                          -- !IsStatementLambda,  IsSingleLine,  IsFunction,  IsAsync
    // Async Sub() foo()                               --  IsStatementLambda,  IsSingleLine, !IsFunction,  IsAsync
    // Async Function() : return foo() : End Function  --  IsStatementLambda, !IsSingleLine,  IsFunction,  IsAsync
    // Async Sub() : foo() : End Sub                   --  IsStatementLambda, !IsSingleLine, !IsFunction,  IsAsync

    // The second case (a Sub expression) is never generated from parsing user code:
    // It is only synthesized at times when we want to synthesize a small delegate (e.g. for relaxed delegates)
    //
    // With the Async/Iterator modifiers, it's possible that to write something like
    //    Dim x = Async _
    //              Function f()
    // i.e. the "Async" and "Function" are split over multiple lines
    //
    // Invariant:
    //     IsStatement IMPLY pHiddenBlock != NULL
    //     pHiddenBlock != NULL IMPLY IsStatement OR (IsSingleLine AND IsFunction() AND IsAsync())
    struct LambdaExpression : Expression
    {
        ParameterList *Parameters;
        bool AllowRelaxationSemantics : 1;
        bool IsStatementLambda : 1;
        bool IsSingleLine : 1; // used by the IDE, and also by the parser to ignore "End Sub" statements

        PunctuatorLocation MethodKind; // This is the "Sub/Function" keyword. (might be different from the start location in case of Async/Iterator modifiers)
        PunctuatorLocation LeftParen;
        PunctuatorLocation RightParen;

        unsigned __int64 MethodFlags; // DECLFLAGS
        SpecifierList *Specifiers;      // Async/Iterator
        
        bool IsFunction()
        {
            return (MethodFlags & DECLF_Function)!=0;
        }

        bool IsAsync()
        {
            return (MethodFlags & DECLF_Async)!=0;
        }
            

        // Accessor for the expression of a single line lambda.
        Expression *GetSingleLineLambdaExpression() 
        {
           VSASSERT( !IsStatementLambda && IsSingleLine, "This is not a single line lambda");

           if (IsFunction() && IsAsync())
           {
           
               ParseTree::LambdaBodyStatement* pLambdaBodyStatement = GetStatementLambdaBody();
               
               if ( pLambdaBodyStatement && pLambdaBodyStatement->Children && pLambdaBodyStatement->Children->Element)
               {
                   VSASSERT(pLambdaBodyStatement->Children->Element->Opcode == ParseTree::Statement::Return, "This statement must be return.");
                   return pLambdaBodyStatement->Children->Element->AsExpression()->Operand; // return the operand of the return
               }
               else
               {
                   return NULL;
               }               
           }
           else 
           {
               return Value;
           }       
        }

        // Setter for the expression of a single line lambda.
        void SetSingleLineLambdaExpression(Expression *pValue) 
        {
            IsStatementLambda = false;
            IsSingleLine = true;
            MethodFlags |= DECLF_Function;
            Value = pValue;
        }
        
        // Accessor for the LambdaBodyStatement of a statement lambda.
        // The LambdaBodyStatement should be the first child of the HiddenBlock
        // that is owned by the lambda.
        //
        // There is no setter for the LambdaBodyStatement because it should be
        // linked into the HiddenBlock's Children StatementList by LinkStatement().
        //
        LambdaBodyStatement *GetStatementLambdaBody() 
        {
            VSASSERT( IsStatementLambda || (IsFunction() && IsAsync()), "This is not a statement lambda");

            VSASSERT( pHiddenBlock && 
                      pHiddenBlock->Children &&
                      pHiddenBlock->Children->Element && 
                      (pHiddenBlock->Children->Element->Opcode == ParseTree::Statement::LambdaBody),
                      "Expect the first statement in the statement list to be a lambda body");  

            return pHiddenBlock->Children->Element->AsLambdaBody();
        }

        // Accessor for the HiddenBlock of a statement lambda.
        HiddenBlockStatement *GetStatementLambdaHiddenBlock()
        {
            return pHiddenBlock;
        }


        // Setter for the HiddenBlock of a statement lambda.
        void SetStatementLambdaHiddenBlock(HiddenBlockStatement *pValue, bool isFunction, bool isSingleLine)
        {
            // In the case of SingleLine function async lambda, 
            // IsStatementLambda should not be set to true.
            IsStatementLambda = !(isFunction && isSingleLine);
            IsSingleLine = isSingleLine;
            if (isFunction)
            {
                MethodFlags |= DECLF_Function;
            }
            else
            {
                MethodFlags &= ~DECLF_Function;
            }
            pHiddenBlock = pValue;
        }

        SpecificExpressionAccessor(Lambda)
    

        // GetIsStatementLambda is used to visit the parse trees and find statement lambdas through the ParseTreeSearcher.
        static bool GetIsStatementLambda(ParseTree::Expression *pExpr, Location *pLoc)
        {
            // SINGLESUB ??? To do: check that the pLoc logic still works for single-line sub lambdas.
            if (pExpr->Opcode == ParseTree::Expression::Lambda &&
                pExpr->AsLambda()->IsStatementLambda &&
                pExpr->AsLambda()->GetStatementLambdaBody() &&
                (!pLoc ||
                    (pExpr->AsLambda()->GetStatementLambdaBody()->TextSpan.m_lBegLine == pLoc->m_lBegLine ||
                        pExpr->AsLambda()->TextSpan.ContainsExtended(pLoc))))
            {
    /*            if (pLoc &&
                    pExpr->AsLambda()->GetStatementLambdaBody()->TerminatingConstruct &&
                    (pExpr->AsLambda()->GetStatementLambdaBody()->TerminatingConstruct->TextSpan.m_lBegLine == pLoc->m_lBegLine) &&
                    pExpr->AsLambda()->GetStatementLambdaBody()->TerminatingConstruct->ContainingList)
                    {
                        ParseTree::StatementList *pStatementList = pExpr->AsLambda()->GetStatementLambdaBody()->TerminatingConstruct->ContainingList;
                        
                        if (pStatementList->NextLexical &&
                            (pStatementList->NextLexical->Element->Opcode == ParseTree::Statement::LambdaBody || 
                                pStatementList->NextLexical->NextLexical->Element->Opcode == ParseTree::Statement::LambdaBody) )
                        {
                            // We return false here because we want to continue on and visit the next lambda body.
                            return false;
                        }
                    }*/
                // We have found the lambda body we are looking for, we can now end our visit of the parse trees..
                return true;
            }
            return false; 
        }

    private:
        union
        {
            Expression *Value;  // for expression-lambdas (i.e. single line function lambdas)
            HiddenBlockStatement *pHiddenBlock;  // for statement-lambdas (i.e. Sub lambdas, or multiline function lambdas)
        };    
    };

    // ;IfExpression - represents a ternary conditional operator, or a coalesce if operator. i.e if(x,y,z), if(x,y)inline function
    struct IIfExpression : Expression
    {
        ParenthesizedArgumentList Arguments;
        SpecificExpressionAccessor(IIf)
    };

    struct AddressOfExpression : UnaryExpression
    {
        bool UseLocationOfTargetMethodForStrict;

        SpecificExpressionAccessor(AddressOf)
    };

    // ********** End Expressions **********

    // ********** Begin Initializers **********

    struct ExpressionInitializer;
    struct DeferredInitializer;
    struct AssignmentInitializer;

    // ;Initializer
    struct Initializer : ParseTreeNode
    {
        enum Opcodes
        {
            #define DEF_INITIALIZER(OpCode, Type, CastFunc) OpCode,
            #include "InitializerTable.h"
            #undef DEF_INITIALIZER
        };

        Opcodes Opcode;

        PunctuatorLocation Key;
        bool FieldIsKey : 1;

        InitializerAccessor(Expression)
        InitializerAccessor(Deferred)
        InitializerAccessor(Assignment)

        Location *GetTextSpan();
    };



    // ;ExpressionInitializer - an initializer that respresents an expression.
    struct ExpressionInitializer : Initializer
    {
        ParseTree::Expression *Value;

        SpecificInitializerAccessor(Expression)
    };

    // ;DeferredInitializer - an initializer that is to be (re)parsed later.
    struct DeferredInitializer : Initializer
    {
        Initializer *Value;

        size_t TextLengthInCharacters;
        WCHAR Text[1];

        SpecificInitializerAccessor(Deferred)
    };

    // ;AssignmentInitializer - an initializer that represents a field assignment.
    // Eg: the ".x = 1" in "with {.x = 1}"
    struct AssignmentInitializer : Initializer
    {
        Location TextSpan;
        IdentifierDescriptor Name;
        PunctuatorLocation Dot;
        PunctuatorLocation Equals;
        Initializer *Initializer;   // 

        SpecificInitializerAccessor(Assignment)
    };


    #if 0
    struct KeyedInitializer
    {
        Initializer* Value;
        PunctuatorLocation Key;
        bool FieldIsKey : 1;
    };
    #endif

    // ;InitializerList
    struct InitializerList : List<Initializer, InitializerList>
    {
    };

    // ;AggregatedInitializerList - list of initializers along with punctuator locations
    // for "{" and "}".
    struct BracedInitializerList : ParseTreeNode 
    {
        InitializerList *InitialValues;

        Location TextSpan;

        // Intellisense may care if the initializer list is complete.
        bool RBracePresent;

        // Punctuator location not encoded because a general practice
        // seems to be if the punctuator term is always the start
        // or end of the node, it seems to be inferred from the start
        // location of the node.
        // So in this case, the location for "{" can be inferred from
        // the start of the this node. Exposing this information via an
        // accessor.
        // PunctuatorLocation LBrace;
        // PunctuatorLocation RBrace;

        Location GetLocationOfLBrace()
        {
            Location Loc;
            Loc.m_lBegLine = TextSpan.m_lBegLine;
            Loc.m_lBegColumn = TextSpan.m_lBegColumn;
            Loc.m_lEndLine = TextSpan.m_lBegLine;
            Loc.m_lEndColumn = TextSpan.m_lBegColumn;

            return Loc;
        }

        Location GetLocationOfRBrace()
        {
            Location Loc;

            if (RBracePresent)
            {
                Loc.m_lBegLine = TextSpan.m_lEndLine;
                Loc.m_lBegColumn = TextSpan.m_lEndColumn;
                Loc.m_lEndLine = TextSpan.m_lEndLine;
                Loc.m_lEndColumn = TextSpan.m_lEndColumn;
            }
            else
            {
                Loc.Invalidate();
            }

    #pragma prefast(suppress:6001, "The call to Invalidate ensures intitialization")
            return Loc;
        }
    };

    // ********** End Initializers **********

    // ********** Begin Cases **********

    struct RelationalCase;
    struct ValueCase;
    struct RangeCase;

    struct Case
    {
        enum Opcodes
        {
            #define DEF_CASE(OpCode, Type, CastFunc) OpCode,
            #include "CaseTable.h"
            #undef DEF_CASE
        };

        Opcodes Opcode;
        Location TextSpan;

        Case * AsCase()
        {
            return this;
        }
        CaseAccessor(Relational)
        CaseAccessor(Value)
        CaseAccessor(Range)
    };

    struct RelationalCase : Case
    {
        Expression::Opcodes RelationalOpcode;
        Expression *Value;

        PunctuatorLocation Is;
        PunctuatorLocation Operator;

        SpecificCaseAccessor(Relational)
    };

    struct ValueCase : Case
    {
        Expression *Value;

        SpecificCaseAccessor(Value)
    };

    struct RangeCase : Case
    {
        Expression *Low;
        Expression *High;

        PunctuatorLocation To;

        SpecificCaseAccessor(Range)
    };

    // ********** End Cases **********

    #if DEBUG

    void
    DumpStatement
    (
        Statement *Input,
        Compiler *Compiler
    );

    #endif

    // Abstract Base Class for Iterators used to find lambda body's
    class LambdaBodyIter
    {
    public:
        virtual LambdaBodyStatement *GetNext() = 0;
    };

   // Iterator to iterate through a CallStatement's Arguments and returns LambdaBodyStatements of Arguments that are statement lambdas. 
   template<class ElementType, class ListType>     
   class LambdaBodyListIter : public LambdaBodyIter
    {
    public: 
        LambdaBodyListIter( _In_ ParseTree::List<ElementType, ListType> *pList);

        void SetList( _In_ ParseTree::List<ElementType, ListType> *pList)
        {
            m_ListIter.SetList(pList);
        }
        
        LambdaBodyStatement *GetNext();
        LambdaBodyStatement *GetNext(_In_opt_ Location *pLocation);
        

    private:
        ListIter<ElementType, ListType> m_ListIter;
    };
    
    // Iterator to iterate through a VariableStatement's  declarations and returns LambdaBodyStatements of VariableDeclarations that have been initialized using statement lambdas.
    class VariableDeclarationStatementStatementLambdaBodyInitializationIter : public LambdaBodyIter
    {
    public: 
        VariableDeclarationStatementStatementLambdaBodyInitializationIter
        (
            _In_ ParseTree::VariableDeclarationStatement * pVariabledDeclarationStatement,
            _In_opt_ ParseTree::ArgumentList *pArgList = NULL,
            _In_opt_ ParseTree::InitializerList *pBracedList = NULL
        );
        LambdaBodyStatement *GetNext();
        LambdaBodyStatement *GetNext(_In_opt_ Location *pLocation);

    private:
        VariableDeclarationIter m_pVariableDeclarationIter;
        LambdaBodyListIter<Argument, ArgumentList> m_NewArgumentIter;
        LambdaBodyListIter<Initializer, InitializerList> m_NewBracedInitializerIter;
    };

}

