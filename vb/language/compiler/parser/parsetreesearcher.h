//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Parse Tree Searcher
//
//-------------------------------------------------------------------------------------------------

#pragma once

class ParseTreeSearcher 
{
    //defines an HRESULT within FACILITY_ITF used by SearchForNodesWithExtraNewLines
    //to terminate the search when it finds a parse tree node with an extra new line.
    //Essentally, it calls VbThrow(FOUND_NODE_WITH_EXTRA_NEW_LINE) which is then
    //caught in the bodies of SearchParameter and SearchStaement.
    static const int FINISHED_SEARCH = 0x2004FFFF;

    template <typename TPredicate>
    class SearchParseTreeNodeImpl : public AutoParseTreeVisitor
    {
    public:
        SearchParseTreeNodeImpl(TPredicate& pred) : m_pFound(NULL), m_predicate(pred)
        {
        }

        template <typename TNodeStartType>
        ParseTree::ParseTreeNode* FindFirst( _In_ TNodeStartType *pStart )
        {
            m_pFound = NULL;
            __try
            {
                VisitGeneric(pStart);
            }
            __except(GetExceptionCode() == FINISHED_SEARCH )
            {
                // Finished
            }

            return m_pFound;
        }

    protected:
        virtual __override
        void VisitParseTreeNodeBase( _In_ ParseTree::ParseTreeNode * pNode)
        {
            if ( m_predicate(pNode) )
            {
                m_pFound = pNode;

                //We want to stop the search with the first node we find that has an extra new line.
                //Throwing seems like an easy way to do this (otherwise I would need to put a bunch of
                //if (ShouldContinueVisiting()) type of stuff into the implementation of AutoParseTreeVisitor which
                //would just complicate things more. 
                RaiseException(FINISHED_SEARCH, 0, 0, NULL);
            }
        }

    private:
        ParseTree::ParseTreeNode *m_pFound;
        TPredicate& m_predicate;
    };

    template <typename TPredicate>
    class SearchExpressionImpl : public AutoParseTreeVisitor
    {
    public:
        SearchExpressionImpl(TPredicate& pred) : m_pFound(NULL), m_predicate(pred)
        {
        }

        template <typename TNodeStartType>
        ParseTree::Expression* FindFirst( _In_ TNodeStartType *pStart ) 
        {
            m_pFound = NULL;
            __try
            {
                VisitGeneric(pStart);
            }
            __except(GetExceptionCode() == FINISHED_SEARCH )
            {
                // Finished
            }

            return m_pFound;
        }

    protected:
        virtual __override
        void VisitExpressionBase( _In_ ParseTree::Expression *pExpr )
        {
            if ( m_predicate(pExpr) )
            {
                m_pFound = pExpr;

                //We want to stop the search with the first node we find that has an extra new line.
                //Throwing seems like an easy way to do this (otherwise I would need to put a bunch of
                //if (ShouldContinueVisiting()) type of stuff into the implementation of AutoParseTreeVisitor which
                //would just complicate things more. 
                RaiseException(FINISHED_SEARCH, 0, 0, NULL);
            }
        }

    private:
        ParseTree::Expression *m_pFound;
        TPredicate& m_predicate;
    };

    template <typename TPredicate, typename TArg>
    class SearchExpressionImplWithArgument : public AutoParseTreeVisitor
    {
    public:
        SearchExpressionImplWithArgument(TPredicate& pred, TArg *pArg) : m_pFound(NULL), m_predicate(pred), m_pArg(pArg)
        {
        }

        template <typename TNodeStartType>
        ParseTree::Expression* FindFirst( _In_ TNodeStartType *pStart ) 
        {
            m_pFound = NULL;
            __try
            {
                VisitGeneric(pStart);
            }
            __except(GetExceptionCode() == FINISHED_SEARCH )
            {
                // Finished
            }

            return m_pFound;
        }

    protected:
        virtual __override
        void VisitExpressionBase( _In_ ParseTree::Expression *pExpr)
        {
            if ( m_predicate(pExpr, m_pArg) )
            {
                m_pFound = pExpr;

                //We want to stop the search with the first node we find that has an extra new line.
                //Throwing seems like an easy way to do this (otherwise I would need to put a bunch of
                //if (ShouldContinueVisiting()) type of stuff into the implementation of AutoParseTreeVisitor which
                //would just complicate things more. 
                RaiseException(FINISHED_SEARCH, 0, 0, NULL);
            }
        }

    private:
        ParseTree::Expression *m_pFound;
        TPredicate& m_predicate;
        TArg* m_pArg;
    };

public:
    template <typename TNodeType, typename TPredicate>
    static
    ParseTree::ParseTreeNode* FindFirstNode( _In_ TNodeType *pStart, const TPredicate& predicate)
    {
        SearchParseTreeNodeImpl<TPredicate> impl(predicate);
        ParseTree::ParseTreeNode* pParseTreeNode = impl.FindFirst(pStart);
        return pParseTreeNode;
    }

    template <typename TNodeType, typename TPredicate>
    static
    ParseTree::Expression* FindFirstExpression( _In_ TNodeType *pStart, const TPredicate& predicate)
    {
        SearchExpressionImpl<TPredicate> impl(predicate);
        ParseTree::Expression*pExpression = impl.FindFirst(pStart);
        return pExpression;
    }

    template <typename TNodeType, typename TPredicate, typename TArg>
    static
    ParseTree::Expression* FindFirstExpressionWithArgument( _In_ TNodeType *pStart, _In_opt_ TArg *pArg, const TPredicate& predicate)
    {
        SearchExpressionImplWithArgument<TPredicate, TArg> impl(predicate, pArg);
        ParseTree::Expression*pExpression = impl.FindFirst(pStart);
        return pExpression;
    }

};
