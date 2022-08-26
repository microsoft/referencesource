#pragma once

template <class T>
class SimpleParseTreeVisitor;


template <class ReturnType, class NodeType>
struct VisitorDispatchInfo
{
    typedef void * (NodeType::*cast_function)();
    typedef ReturnType (SimpleParseTreeVisitor<ReturnType>::*visit_function)(void *);
    
    cast_function m_pCastFunction;
    visit_function m_pVisitFunction;
};


//Defines a base visitor class for parse trees.
//This should include virtually every single type of parse tree node.
//Normally you want to inherit from AutoParseTreeVisitor which defines default traversal logic for you.
//You can simple inherit from that class and just override the methods you are interested in.
//
//This class does not define any traversal logic, and simply just defines dispatch logic.
//It is useful for situations where you need to heavily customize the traveral process, such as semantic analysis.
//
//This class is also templated on the return type of the Visit methods. This is useful for scenarios where visitor
//implementatins need to return values (like semantic analysis). The default implementation (AutoParseTreeVisitor) uses void.
//
//Unfortunately our parse trees are not all contained within a unified type hierarchy. In particular there are several
//seperate hierarhcies used:
//
//          1. Statements
//          2. Expressions
//          3. Types
//          4. Names
//          5. Variable Declarations
//          6. Constraints 
//          7. Import Directives
//          8. Case nodes
//          9. Initializer nodes
//          10. Auto property initializers
//          11. Lists
//          12. Everything else.
//
//
//This visitor provides a single class that can be used to dispatch over all of them.
//Items #1-10 support dynamic dispatch because they have opcodes stored in them.
//However, they all have disjoint opcode sets, and so we can't have a single method for dispatching all of them.
//Instead we have a seperate entry point (VisitStatement, VisitExpression, etc.) for each hierarchy.
//
//
//All the other types (items 11 and 12) don't have opcodes stored in them and so they don't support dynamic dispatch.
//However, they are used pervasively in our parse trees and so we do define methods for dispatching to them statically. 
//AutoParseTreeVisitor will call those methods as it traverses the parse tree.
//
//For items #1-10 we automatically generate Visit methods for each unique opcode. If you add a new opcode to one of the following header files, 
//a new method, with appropriate default behavior, will be autoamtically inserted into this class. 
//     * ) StatementTable.h
//     * ) ExpressionTable.h
//     * ) ParseTreeTypeTable.h
//     * ) NameTable.h
//     * ) VariableDeclarationTable.h
//     * ) ConstraintTable.h
//     * ) ImportDirectiveTable.h
//     * ) CaseTable.h
//     * ) InitializerTable.h
//     * ) AutoPropertyInitializerTable.h
//
//
//At the moment we don't put v-tables into our parse trees because we zero initialize them on creation, which would clobber the v-table.
//We do, however, support quick table-based dispatch using opcodes values. This avoids the need to maintain a giant switch statement,
//and also makes it easier to add new types of parse tree nodes. The tables that drive this are generated automatically and are stored
//in the various VisitorDispatchInfo arrays below.
//
//For items # 11 and 12 we manually define a seperate Vist method for each unique type (VisitInitializerList, VisitAttribute, etc.).
//
template <class T>
class SimpleParseTreeVisitor
{
public:
    virtual T VisitStatement(ParseTree::Statement * pStatement)
    {
        ThrowIfNull(pStatement);
        ParseTree::Statement::Opcodes opCode = pStatement->Opcode;

        VisitorDispatchInfo<T, ParseTree::Statement> dispatchInfo = m_statementDispatchTable[opCode];

        void * pCastArg = (pStatement->*(dispatchInfo.m_pCastFunction))();
        return (this->*(dispatchInfo.m_pVisitFunction))(pCastArg);
    }
    
    virtual T VisitExpression(ParseTree::Expression * pExpression)
    {
        ThrowIfNull(pExpression);
        ParseTree::Expression::Opcodes opCode = pExpression->Opcode;

        VisitorDispatchInfo<T, ParseTree::Expression> dispatchInfo = m_expressionDispatchTable[opCode];

        void * pCastArg = (pExpression->*(dispatchInfo.m_pCastFunction))();
        return (this->*(dispatchInfo.m_pVisitFunction))(pCastArg);

    }
    
    virtual T VisitType(ParseTree::Type * pType)
    {
        ThrowIfNull(pType);
        ParseTree::Type::Opcodes opCode = pType->Opcode;

        VisitorDispatchInfo<T, ParseTree::Type> dispatchInfo = m_typeDispatchTable[opCode];

        void * pCastArg = (pType->*(dispatchInfo.m_pCastFunction))();
        return (this->*(dispatchInfo.m_pVisitFunction))(pCastArg);

    }
    
    virtual T VisitName(ParseTree::Name * pName)
    {
        ThrowIfNull(pName);
        ParseTree::Name::Opcodes opCode = pName->Opcode;

        VisitorDispatchInfo<T, ParseTree::Name> dispatchInfo = m_nameDispatchTable[opCode];

        void * pCastArg = (pName->*(dispatchInfo.m_pCastFunction))();
        return (this->*(dispatchInfo.m_pVisitFunction))(pCastArg);

    }

    virtual T VisitVariableDeclaration(ParseTree::VariableDeclaration * pVarDecl)
    {
        ThrowIfNull(pVarDecl);

        ParseTree::VariableDeclaration::Opcodes opCode = pVarDecl->Opcode;

        VisitorDispatchInfo<T, ParseTree::VariableDeclaration> dispatchInfo = m_variableDeclDispatchTable[opCode];

        void * pCastArg = (pVarDecl->*(dispatchInfo.m_pCastFunction))();
        return (this->*(dispatchInfo.m_pVisitFunction))(pCastArg);        
    }

    virtual T VisitConstraint(ParseTree::Constraint * pConstraint)
    {
        ThrowIfNull(pConstraint);

        ParseTree::Constraint::Opcodes opCode = pConstraint->Opcode;

        VisitorDispatchInfo<T, ParseTree::Constraint> dispatchInfo = m_constraintDispatchTable[opCode];

        void * pCastArg = (pConstraint->*(dispatchInfo.m_pCastFunction))();
        return (this->*(dispatchInfo.m_pVisitFunction))(pCastArg);        
    }    


   virtual T VisitImportDirective(ParseTree::ImportDirective * pDirective)
    {
        ThrowIfNull(pDirective);

        ParseTree::ImportDirective::Modes opCode = pDirective->Mode;

        VisitorDispatchInfo<T, ParseTree::ImportDirective> dispatchInfo = m_importDirectiveDispatchTable[opCode];

        void * pCastArg = (pDirective->*(dispatchInfo.m_pCastFunction))();
        return (this->*(dispatchInfo.m_pVisitFunction))(pCastArg);        
    }    

   virtual T VisitCase(ParseTree::Case * pCase)
    {
        ThrowIfNull(pCase);

        ParseTree::Case::Opcodes opCode = pCase->Opcode;

        VisitorDispatchInfo<T, ParseTree::Case> dispatchInfo = m_caseDispatchTable[opCode];

        void * pCastArg = (pCase->*(dispatchInfo.m_pCastFunction))();
        return (this->*(dispatchInfo.m_pVisitFunction))(pCastArg);        
    }

    virtual T VisitInitializer(ParseTree::Initializer * pInitializer)
    {
        ThrowIfNull(pInitializer);

        ParseTree::Initializer::Opcodes opCode = pInitializer->Opcode;

        VisitorDispatchInfo<T, ParseTree::Initializer> dispatchInfo = m_initializerDispatchTable[opCode];

        void * pCastArg = (pInitializer->*(dispatchInfo.m_pCastFunction))();
        return (this->*(dispatchInfo.m_pVisitFunction))(pCastArg);        
    }

    virtual T VisitAutoPropertyInit(ParseTree::AutoPropertyInitialization * pInit)
    {
        ThrowIfNull(pInit);

        ParseTree::AutoPropertyInitialization::Opcodes opCode = pInit->Opcode;

        VisitorDispatchInfo<T, ParseTree::AutoPropertyInitialization> dispatchInfo = m_autoPropInitDispatchTable[opCode];

        void * pCastArg = (pInit->*(dispatchInfo.m_pCastFunction))();
        return (this->*(dispatchInfo.m_pVisitFunction))(pCastArg);        
        
    }
protected:

    //Define Visit methods for every statement type
    #define DEF_STATEMENT(OpCode, Type, CastFunc) virtual T Visit##OpCode##Statement(ParseTree :: Type * pStatement) { return Default(pStatement) ; }
    #include "StatementTable.h"
    #undef DEF_STATEMENT

    //Define Visit methods for every expression type
    #define DEF_EXPRESSION(OpCode, Type, CastFunc) virtual T Visit##OpCode##Expression(ParseTree :: Type * pExpr) { return Default(pExpr) ; }
    #include "ExpressionTable.h"
    #undef DEF_EXPRESSION

    //Define Visit methods for every Type type
    #define DEF_TYPE(OpCode, t, CastFunc) virtual T Visit##OpCode##Type(ParseTree :: t * pType) {  return Default(pType) ; }
    #include "ParseTreeTypeTable.h"
    #undef DEF_TYPE

    //Define Visit methods for every name type
    #define DEF_NAME(OpCode, Type, CastFunc) virtual T Visit##OpCode##Name(ParseTree :: Type * pName) { return Default(pName) ; }
    #include "NameTable.h"
    #undef DEF_NAME

    //Define Visit methods for every variable declaration type
    #define DEF_VAR_DECL(OpCode, Type, CastFunc) virtual T Visit##OpCode##VariableDeclaration(ParseTree :: Type * pVarDecl) { return Default(pVarDecl) ; }
    #include "VariableDeclarationTable.h"
    #undef DEF_VAR_DECL

    #define DEF_CONSTRAINT(OpCode, Type, CastFunc) virtual T Visit##OpCode##Constraint(ParseTree :: Type * pConstraint) { return Default(pConstraint) ; }
    #include "ConstraintTable.h"
    #undef DEF_CONSTRAINT

    #define DEF_IMPORT_DIRECTIVE(OpCode, Type, CastFunc) virtual T Visit##OpCode##ImportDirective(ParseTree :: Type * pDirective) { return Default(pDirective) ; }
    #include "ImportDirectiveTable.h"
    #undef DEF_IMPORT_DIRECTIVE
    
    #define DEF_CASE(OpCode, Type, CastFunc) virtual T Visit##OpCode##Case(ParseTree :: Type * pCase) { return Default(pCase) ; }
    #include "CaseTable.h"
    #undef DEF_CASE

    #define DEF_INITIALIZER(OpCode, Type, CastFunc) virtual T Visit##OpCode##Initializer(ParseTree :: Type * pInitializer)  { return Default(pInitializer) ; }
    #include "InitializerTable.h"
    #undef DEF_INITIALIZER

    //Auto property initializers
    #define DEF_AUTOPROP_INIT(OpCode, Type, CastFunc) virtual T Visit##OpCode##AutoPropInit(ParseTree::Type * pInit) { return Default(pInit); }
    #include "AutoPropertyTable.h"
    #undef DEF_AUTOPROP_INIT
    
    virtual T Default(void * pOther)
    {
        return T();
    }

    virtual T VisitAttributeList
    (
        ParseTree::AttributeList * pAttributeList
    )
    {
        return Default(pAttributeList);
    }

    virtual T VisitSpecifierList
    (
        ParseTree::SpecifierList * pSpecifierList
    )
    {
        return Default(pSpecifierList);
    }

    virtual T VisitAttributeSpecifierList
    (
        ParseTree::AttributeSpecifierList * pAttributeSpecifierList
    )
    {
        return Default(pAttributeSpecifierList);
    }

    virtual T VisitConstraintList
    (
        ParseTree::ConstraintList * pConstraintList
    )
    {
        return Default(pConstraintList);
    }

    virtual T VisitGenericParameterList
    (
        ParseTree::GenericParameterList * pGenericParameterList
    )
    {
        return Default(pGenericParameterList);
    }

    virtual T VisitParameterSpecifierList
    (
        ParseTree::ParameterSpecifierList * pParameterSpecifierList
    )
    {
        return Default(pParameterSpecifierList);
    }

    virtual T VisitParameterList
    (
        ParseTree::ParameterList * pParameterList
    )
    {
        return Default(pParameterList);
    }

    virtual T VisitDeclaratorList
    (
        ParseTree::DeclaratorList * pDeclaratorList
    )
    {
        return Default(pDeclaratorList);
    }

    virtual T VisitVariableDeclarationList
    (
        ParseTree::VariableDeclarationList * pVariableDeclarationList
    )
    {
        return Default(pVariableDeclarationList);
    }

    virtual T VisitImportDirectiveList
    (
        ParseTree::ImportDirectiveList * pImportDirectiveList
    )
    {
        return Default(pImportDirectiveList);
    }

    virtual T VisitCaseList
    (
        ParseTree::CaseList * pCaseList
    )
    {
        return Default(pCaseList);
    }

    virtual T VisitCommentList
    (
        ParseTree::CommentList * pCommentList
    )
    {
        return Default(pCommentList);
    }

    virtual T VisitNameList
    (
        ParseTree::NameList * pNameList
    )
    {
        return Default(pNameList);
    }

    virtual T VisitTypeList
    (
        ParseTree::TypeList * pTypeList
    )
    {
        return Default(pTypeList);
    }

    virtual T VisitArgumentList
    (
        ParseTree::ArgumentList * pArgumentList
    )
    {
        return Default(pArgumentList);
    }
    
    virtual T VisitExpressionList
    (
        ParseTree::ExpressionList * pExpressionList
    )
    {
        return Default(pExpressionList);
    }

    virtual T VisitArrayDimList
    (
        ParseTree::ArrayDimList * pArrayDimList
    )
    {
        return Default(pArrayDimList);
    }

    virtual T VisitFromList
    (
        ParseTree::FromList * pFromList
    )
    {
        return Default(pFromList);
    }

    virtual T VisitOrderByList
    (
        ParseTree::OrderByList * pOrderByList
    )
    {
        return Default(pOrderByList);
    }

    virtual T VisitInitializerList
    (
        ParseTree::InitializerList * pInitializerList
    )
    {
        return Default(pInitializerList);
    }

    virtual T VisitParenthesizedArgumentList
    (
        ParseTree::ParenthesizedArgumentList * pArgumentList
    )
    {
        return Default(pArgumentList);
    }

    virtual T VisitAttribute
    (
        ParseTree::Attribute * pAttribute
    )
    {
        return Default(pAttribute);
    }

    virtual T VisitArgument
    (
        ParseTree::Argument * pArgument
    )
    {
        return Default(pArgument);
    }

    virtual T VisitBracedInitializerList
    (
        ParseTree::BracedInitializerList * pInitializerList
    )
    {
        return Default(pInitializerList);
    }

    virtual T VisitExternalSourceDirective
    (
        ParseTree::ExternalSourceDirective * pExternalSourceDirective
    )
    {
        return Default(pExternalSourceDirective);
    }

    virtual T VisitStatementList
    (
        ParseTree::StatementList * pStatementList
    )
    {
        return Default(pStatementList);
    }
  
    virtual T VisitGenericArguments
    (
        ParseTree::GenericArguments * pGenericArguments
    )
    {
        return Default(pGenericArguments);
    }

    virtual T VisitAttributeSpecifier
    (
        ParseTree::AttributeSpecifier * pAttributeSpecifier
    )
    {
        return Default(pAttributeSpecifier);
    }

    virtual T VisitGenericParameter
    (
        ParseTree::GenericParameter * pParam
    )
    {
        return Default(pParam);
    }

    virtual T VisitParameter
    (
        ParseTree::Parameter * pParam
    )
    {
        return Default(pParam);
    }

    virtual T VisitDeclarator
    (
        ParseTree::Declarator * pDeclarator
    )
    {
        return Default(pDeclarator);
    }

    virtual T VisitComment
    (
        ParseTree::Comment * pComment
    )
    {
        return Default(pComment);
    }

    virtual T VisitArrayDim
    (
        ParseTree::ArrayDim * pArrayDim
    )
    {
        return Default(pArrayDim);
    }

    virtual T VisitFromItem
    (
        ParseTree::FromItem * pFromItem
    )
    {
        return Default(pFromItem);
    }

    virtual T VisitOrderByItem
    (
        ParseTree::OrderByItem * pOrderByItem
    )
    {
        return Default(pOrderByItem);
    }

    virtual T VisitObjectInitializerList
    (
        ParseTree::ObjectInitializerList * pInitList
    )
    {
        return Default(pInitList);
    }

    virtual T VisitSpecifier
    (
        ParseTree::Specifier * pSpecifier
    )
    {
        return Default(pSpecifier);
    }

    virtual T VisitParameterSpecifier
    (
        ParseTree::ParameterSpecifier * pSpecifier
    )
    {
        return Default(pSpecifier);
    }

    template <typename TStartType>
    T VisitGeneric(TStartType *pStart)
    {
        COMPILE_ASSERT(false);
    }

    template <>
    T VisitGeneric<ParseTree::Statement>( _In_ ParseTree::Statement *pStatement )
    {
        return VisitStatement(pStatement);
    }

    template <>
    T VisitGeneric<ParseTree::Parameter>( _In_ ParseTree::Parameter *pParameter )
    {
        return VisitParameter(pParameter);
    }

    template <>
    T VisitGeneric<ParseTree::ParameterList>(_In_ ParseTree::ParameterList * pParameterList)
    {
        return VisitParameterList(pParameterList);
    }

    template <>
    T VisitGeneric<ParseTree::Expression>(_In_ ParseTree::Expression * pExpression)
    {
        return VisitExpression(pExpression);
    }

    template<>
    T VisitGeneric<ParseTree::Initializer>(_In_ ParseTree::Initializer* pInit)
    {
        return VisitInitializer(pInit);
    }

    template<>
    T VisitGeneric<ParseTree::Argument>(_In_ ParseTree::Argument *pArg)
    {
        return VisitArgument(pArg);
    }

private:
    
    typedef T (SimpleParseTreeVisitor<T>::*visit_function)(void *);

    static const VisitorDispatchInfo<T, ParseTree::Statement> m_statementDispatchTable[];
    static const VisitorDispatchInfo<T, ParseTree::Expression> m_expressionDispatchTable[];
    static const VisitorDispatchInfo<T, ParseTree::Name> m_nameDispatchTable[];
    static const VisitorDispatchInfo<T, ParseTree::Type> m_typeDispatchTable[];
    static const VisitorDispatchInfo<T, ParseTree::VariableDeclaration> m_variableDeclDispatchTable[];
    static const VisitorDispatchInfo<T, ParseTree::Constraint> m_constraintDispatchTable[];
    static const VisitorDispatchInfo<T, ParseTree::ImportDirective> m_importDirectiveDispatchTable[];
    static const VisitorDispatchInfo<T, ParseTree::Case> m_caseDispatchTable[];
    static const VisitorDispatchInfo<T, ParseTree::Initializer> m_initializerDispatchTable[];
    static const VisitorDispatchInfo<T, ParseTree::AutoPropertyInitialization> m_autoPropInitDispatchTable[];
    
};

template <class T>
const VisitorDispatchInfo<T, ParseTree::Statement> SimpleParseTreeVisitor<T>::m_statementDispatchTable[] =
{
    #define DEF_STATEMENT(OpCode, Type, CastFunc) \
    { \
        (VisitorDispatchInfo<T, ParseTree::Statement>::cast_function)&ParseTree::Statement:: CastFunc, \
        (SimpleParseTreeVisitor<T>::visit_function)&SimpleParseTreeVisitor:: Visit##OpCode##Statement \
    },
    #include "StatementTable.h"
    #undef DEF_STATEMENT

};

template <class T>
const VisitorDispatchInfo<T, ParseTree::Expression> SimpleParseTreeVisitor<T>::m_expressionDispatchTable[] =
{
    #define DEF_EXPRESSION(OpCode, Type, CastFunc) \
    { \
        (VisitorDispatchInfo<T, ParseTree::Expression>::cast_function)&ParseTree::Expression:: CastFunc, \
        (SimpleParseTreeVisitor<T>::visit_function)&SimpleParseTreeVisitor:: Visit##OpCode##Expression \
    },
    #include "ExpressionTable.h"
    #undef DEF_EXPRESSION
};

template <class T>
const VisitorDispatchInfo<T, ParseTree::Name> SimpleParseTreeVisitor<T>::m_nameDispatchTable[] =
{
    #define DEF_NAME(OpCode, Type, CastFunc) \
    { \
        (VisitorDispatchInfo<T, ParseTree::Name>::cast_function)&ParseTree::Name:: CastFunc, \
        (SimpleParseTreeVisitor<T>::visit_function)&SimpleParseTreeVisitor:: Visit##OpCode##Name \
    },
    #include "NameTable.h"
    #undef DEF_NAME
    
};

template <class T>
const VisitorDispatchInfo<T, ParseTree::Type> SimpleParseTreeVisitor<T>::m_typeDispatchTable[] =
{
    #define DEF_TYPE(OpCode, t, CastFunc) \
    { \
        (VisitorDispatchInfo<T, ParseTree::Type>::cast_function)&ParseTree::Type:: CastFunc, \
        (SimpleParseTreeVisitor<T>::visit_function)&SimpleParseTreeVisitor:: Visit##OpCode##Type \
    },
    #include "ParseTreeTypeTable.h"
    #undef DEF_TYPE
};

template <class T>
const VisitorDispatchInfo<T, ParseTree::VariableDeclaration> SimpleParseTreeVisitor<T>::m_variableDeclDispatchTable[] =
{
    #define DEF_VAR_DECL(OpCode, Type, CastFunc) \
    { \
        (VisitorDispatchInfo<T, ParseTree::VariableDeclaration>::cast_function)&ParseTree::VariableDeclaration:: CastFunc, \
        (SimpleParseTreeVisitor<T>::visit_function)&SimpleParseTreeVisitor:: Visit##OpCode##VariableDeclaration \
    },
    #include "VariableDeclarationTable.h"
    #undef DEF_VAR_DECL
};

template <class T>
const VisitorDispatchInfo<T, ParseTree::Constraint> SimpleParseTreeVisitor<T>::m_constraintDispatchTable[] =
{
    #define DEF_CONSTRAINT(OpCode, Type, CastFunc) \
    { \
        (VisitorDispatchInfo<T, ParseTree::Constraint>::cast_function)&ParseTree::Constraint:: CastFunc, \
        (SimpleParseTreeVisitor<T>::visit_function)&SimpleParseTreeVisitor:: Visit##OpCode##Constraint \
    },
    #include "ConstraintTable.h"
    #undef DEF_CONSTRAINT
};

template <class T>
const VisitorDispatchInfo<T, ParseTree::ImportDirective> SimpleParseTreeVisitor<T>::m_importDirectiveDispatchTable[] =
{
    #define DEF_IMPORT_DIRECTIVE(OpCode, Type, CastFunc) \
    { \
        (VisitorDispatchInfo<T, ParseTree::ImportDirective>::cast_function)&ParseTree::ImportDirective:: CastFunc, \
        (SimpleParseTreeVisitor<T>::visit_function)&SimpleParseTreeVisitor:: Visit##OpCode##ImportDirective \
    },
    #include "ImportDirectiveTable.h"
    #undef DEF_IMPORT_DIRECTIVE
};

template <class T>
const VisitorDispatchInfo<T, ParseTree::Case> SimpleParseTreeVisitor<T>::m_caseDispatchTable[] =
{
    #define DEF_CASE(OpCode, Type, CastFunc) \
    { \
        (VisitorDispatchInfo<T, ParseTree::Case>::cast_function)&ParseTree::Case:: CastFunc, \
        (SimpleParseTreeVisitor<T>::visit_function)&SimpleParseTreeVisitor:: Visit##OpCode##Case \
    },
    #include "CaseTable.h"
    #undef DEF_CASE
}; 

template <class T>
const VisitorDispatchInfo<T, ParseTree::Initializer> SimpleParseTreeVisitor<T>::m_initializerDispatchTable[] =
{
    #define DEF_INITIALIZER(OpCode, Type, CastFunc) \
    { \
        (VisitorDispatchInfo<T, ParseTree::Initializer>::cast_function)&ParseTree::Initializer:: CastFunc, \
        (SimpleParseTreeVisitor<T>::visit_function)&SimpleParseTreeVisitor:: Visit##OpCode##Initializer \
    },
    #include "InitializerTable.h"
    #undef DEF_INITIALIZER
};     

template <class T>
const VisitorDispatchInfo<T, ParseTree::AutoPropertyInitialization> SimpleParseTreeVisitor<T>::m_autoPropInitDispatchTable[] =
{
    #define DEF_AUTOPROP_INIT(OpCode, Type, CastFunc) \
    { \
        (VisitorDispatchInfo<T, ParseTree::AutoPropertyInitialization>::cast_function)&ParseTree::AutoPropertyInitialization:: CastFunc, \
        (SimpleParseTreeVisitor<T>::visit_function)&SimpleParseTreeVisitor:: Visit##OpCode##AutoPropInit \
    },
    #include "AutoPropertyTable.h"
    #undef DEF_AUTOPROP_INIT
};     



