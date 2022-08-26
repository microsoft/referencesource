//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Fixes up all the locations in this parsetree to be the one that is passed in.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class LocationFixupVisitor : public ParseTreeVisitor
{
public:
    LocationFixupVisitor(Location* location)
    {
        m_location = location;
    }

    void Fixup(ParseTree::Expression* expression)
    {
        Visit(expression);
    }

private:
    virtual
    void Visit(ParseTree::Statement * statement)
    {
        statement->TextSpan = *m_location;
        ParseTreeVisitor::Visit(statement);
    }

    virtual 
    void Visit(ParseTree::Expression * expression)
    {
        expression->TextSpan = *m_location;
        ParseTreeVisitor::Visit(expression);
    }

    virtual 
    void VisitParameter(ParseTree::Parameter * parameter)
    {
        parameter->TextSpan = *m_location;
        ParseTreeVisitor::VisitParameter(parameter);
    }

    virtual 
    void VisitInitializer(ParseTree::Initializer * initializer)
    {
        //Intializers don't have a source location.
        ParseTreeVisitor::VisitInitializer(initializer);
    }

    virtual 
    void VisitFromItem(ParseTree::FromItem * fromItem)
    {
        fromItem->TextSpan = *m_location;
        ParseTreeVisitor::VisitFromItem(fromItem);
    }

    virtual 
    void VisitType(ParseTree::Type * type)
    {
        type->TextSpan = *m_location;
        ParseTreeVisitor::VisitType(type);
    }

    virtual 
    void VisitVariableDeclaration(ParseTree::VariableDeclaration * variableDeclaration)
    {
        variableDeclaration->TextSpan = *m_location;
        ParseTreeVisitor::VisitVariableDeclaration(variableDeclaration);
    }

    Location* m_location;
};
