// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ExpressionIterator is an iterator for EXPRLISTs, designed to be used
    // in the following way:
    //
    //     for (ExpressionIterator it(list); !it.AtEnd(); it.MoveNext())
    //     {
    //         EXPR expr = it.Current();
    //         ...
    //         ... // work with expr
    //     }
    //
    // The constructor takes an EXPR which can point to either an EXPRLIST
    // or a non-list EXPR, or nothing. 
    //
    // Upon construction, the iterator's current element is the first element
    // in the list, which is to say it's not necessary to call MoveNext
    // before usage. AtEnd is true iff the iterator's current element is
    // beyond the last element of the list. These semantics differ from
    // IEnumerator in the framework, but are natural for usage in the C++
    // for-loop as above.
    //
    // Outside of a for loop, usage might look like this:
    //
    //     ExpressionIterator it(list);
    //     EXPR expr1 = it.Current();
    //     it.MoveNext();
    //     EXPR expr2 = it.Current();
    //     it.MoveNext();
    //     EXPR expr3 = it.Current();
    //     it.MoveNext();
    //
    // This would get the first three elements in a list. Also available is the
    // static Count:
    //
    //     int n = ExpressionIterator::Count(list);

    internal class ExpressionIterator
    {
        public ExpressionIterator(EXPR pExpr) { Init(pExpr); }

        public bool AtEnd() { return m_pCurrent == null && m_pList == null; }

        public EXPR Current() { return m_pCurrent; }

        public void MoveNext()
        {
            if (AtEnd())
            {
                return;
            }
            else if (m_pList == null)
            {
                m_pCurrent = null;
            }
            else
            {
                Init(m_pList.GetOptionalNextListNode());
            }
        }

        public static int Count(EXPR pExpr)
        {
            int c = 0;
            for (ExpressionIterator it = new ExpressionIterator(pExpr); !it.AtEnd(); it.MoveNext())
            {
                ++c;
            }
            return c;
        }

        private EXPRLIST m_pList;
        private EXPR m_pCurrent;

        private void Init(EXPR pExpr)
        {
            if (pExpr == null)
            {
                m_pList = null;
                m_pCurrent = null;
            }
            else if (pExpr.isLIST())
            {
                m_pList = pExpr.asLIST();
                m_pCurrent = m_pList.GetOptionalElement();
            }
            else
            {
                m_pList = null;
                m_pCurrent = pExpr;
            }
        }
    }
}
