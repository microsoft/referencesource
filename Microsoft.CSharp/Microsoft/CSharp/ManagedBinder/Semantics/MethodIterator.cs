// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal partial class CMemberLookupResults
    {
        public partial class CMethodIterator
        {
            private SymbolLoader m_pSymbolLoader;
            private CSemanticChecker m_pSemanticChecker;
            // Inputs.
            private AggregateType m_pCurrentType;
            private MethodOrPropertySymbol m_pCurrentSym;
            private Declaration m_pContext;
            private TypeArray m_pContainingTypes;
            private CType m_pQualifyingType;
            private Name m_pName;
            private int m_nArity;
            private symbmask_t m_mask;
            private EXPRFLAG m_flags;
            // Internal state.
            private int m_nCurrentTypeCount;
            private bool m_bIsCheckingInstanceMethods;
            private bool m_bAtEnd;
            private bool m_bAllowBogusAndInaccessible;
            private bool m_bAllowExtensionMethods;
            // Flags for the current sym.
            private bool m_bCurrentSymIsBogus;
            private bool m_bCurrentSymIsInaccessible;
            // if Extension can be part of the results that are returned by the iterator
            // this may be false if an applicable instance method was found by bindgrptoArgs
            private bool m_bcanIncludeExtensionsInResults;
            // we have found a applicable extension and only continue to the end of the current
            // Namespace's extension methodlist
            private bool m_bEndIterationAtCurrentExtensionList;

            public CMethodIterator(CSemanticChecker checker, SymbolLoader symLoader, Name name, TypeArray containingTypes, CType @object, CType qualifyingType, Declaration context, bool allowBogusAndInaccessible, bool allowExtensionMethods, int arity, EXPRFLAG flags, symbmask_t mask)
            {
                Debug.Assert(name != null);
                Debug.Assert(symLoader != null);
                Debug.Assert(checker != null);
                Debug.Assert(containingTypes != null);
                m_pSemanticChecker = checker;
                m_pSymbolLoader = symLoader;
                m_pCurrentType = null;
                m_pCurrentSym = null;
                m_pName = name;
                m_pContainingTypes = containingTypes;
                m_pQualifyingType = qualifyingType;
                m_pContext = context;
                m_bAllowBogusAndInaccessible = allowBogusAndInaccessible;
                m_bAllowExtensionMethods = allowExtensionMethods;
                m_nArity = arity;
                m_flags = flags;
                m_mask = mask;
                m_nCurrentTypeCount = 0;
                m_bIsCheckingInstanceMethods = true;
                m_bAtEnd = false;
                m_bCurrentSymIsBogus = false;
                m_bCurrentSymIsInaccessible = false;
                m_bcanIncludeExtensionsInResults = m_bAllowExtensionMethods;
                m_bEndIterationAtCurrentExtensionList = false;
            }
            public MethodOrPropertySymbol GetCurrentSymbol()
            {
                return m_pCurrentSym;
            }
            public AggregateType GetCurrentType()
            {
                return m_pCurrentType;
            }
            public bool IsCurrentSymbolInaccessible()
            {
                return m_bCurrentSymIsInaccessible;
            }
            public bool IsCurrentSymbolBogus()
            {
                return m_bCurrentSymIsBogus;
            }
            public bool MoveNext(bool canIncludeExtensionsInResults, bool endatCurrentExtensionList)
            {
                if (m_bcanIncludeExtensionsInResults)
                {
                    m_bcanIncludeExtensionsInResults = canIncludeExtensionsInResults;
                }
                if (!m_bEndIterationAtCurrentExtensionList)
                {
                    m_bEndIterationAtCurrentExtensionList = endatCurrentExtensionList;
                }

                if (m_bAtEnd)
                {
                    return false;
                }

                if (m_pCurrentType == null) // First guy.
                {
                    if (m_pContainingTypes.size == 0)
                    {
                        // No instance methods, only extensions.
                        m_bIsCheckingInstanceMethods = false;
                        m_bAtEnd = true;
                        return false;
                    }
                    else
                    {
                        if (!FindNextTypeForInstanceMethods())
                        {
                            // No instance or extensions.

                            m_bAtEnd = true;
                            return false;
                        }
                    }
                }
                if (!FindNextMethod())
                {
                    m_bAtEnd = true;
                    return false;
                }
                return true;
            }
            public bool AtEnd()
            {
                return m_pCurrentSym == null;
            }
            private CSemanticChecker GetSemanticChecker()
            {
                return m_pSemanticChecker;
            }
            private SymbolLoader GetSymbolLoader()
            {
                return m_pSymbolLoader;
            }
            public bool CanUseCurrentSymbol()
            {
                m_bCurrentSymIsInaccessible = false;
                m_bCurrentSymIsBogus = false;

                // Make sure that whether we're seeing a ctor is consistent with the flag.
                // The only properties we handle are indexers.
                if (m_mask == symbmask_t.MASK_MethodSymbol && (
                        0 == (m_flags & EXPRFLAG.EXF_CTOR) != !m_pCurrentSym.AsMethodSymbol().IsConstructor() ||
                        0 == (m_flags & EXPRFLAG.EXF_OPERATOR) != !m_pCurrentSym.AsMethodSymbol().isOperator) ||
                    m_mask == symbmask_t.MASK_PropertySymbol && !m_pCurrentSym.AsPropertySymbol().isIndexer())
                {
                    // Get the next symbol.
                    return false;
                }

                // If our arity is non-0, we must match arity with this symbol.
                if (m_nArity > 0)
                {
                    if (m_mask == symbmask_t.MASK_MethodSymbol && m_pCurrentSym.AsMethodSymbol().typeVars.size != m_nArity)
                    {
                        return false;
                    }
                }

                // If this guy's not callable, no good.
                if (!ExpressionBinder.IsMethPropCallable(m_pCurrentSym, (m_flags & EXPRFLAG.EXF_USERCALLABLE) != 0))
                {
                    return false;
                }

                // Check access.
                if (!GetSemanticChecker().CheckAccess(m_pCurrentSym, m_pCurrentType, m_pContext, m_pQualifyingType))
                {
                    // Sym is not accessible. However, if we're allowing inaccessible, then let it through and mark it.
                    if (m_bAllowBogusAndInaccessible)
                    {
                        m_bCurrentSymIsInaccessible = true;
                    }
                    else
                    {
                        return false;
                    }
                }

                // Check bogus.
                if (GetSemanticChecker().CheckBogus(m_pCurrentSym))
                {
                    // Sym is bogus, but if we're allow it, then let it through and mark it.
                    if (m_bAllowBogusAndInaccessible)
                    {
                        m_bCurrentSymIsBogus = true;
                    }
                    else
                    {
                        return false;
                    }
                }

                // if we are done checking all the instance types ensure that currentsym is an 
                // extension method and not a simple static method
                if (!m_bIsCheckingInstanceMethods)
                {
                    if (!m_pCurrentSym.AsMethodSymbol().IsExtension())
                    {
                        return false;
                    }
                }

                return true;
            }

            private bool FindNextMethod()
            {
                while (true)
                {
                    if (m_pCurrentSym == null)
                    {
                        m_pCurrentSym = GetSymbolLoader().LookupAggMember(
                                m_pName, m_pCurrentType.getAggregate(), m_mask).AsMethodOrPropertySymbol();
                    }
                    else
                    {
                        m_pCurrentSym = GetSymbolLoader().LookupNextSym(
                                m_pCurrentSym, m_pCurrentType.getAggregate(), m_mask).AsMethodOrPropertySymbol();
                    }

                    // If we couldn't find a sym, we look up the type chain and get the next type.
                    if (m_pCurrentSym == null)
                    {
                        if (m_bIsCheckingInstanceMethods)
                        {
                            if (!FindNextTypeForInstanceMethods() && m_bcanIncludeExtensionsInResults)
                            {
                                // We didn't find any more instance methods, set us into extension mode.

                                m_bIsCheckingInstanceMethods = false;
                            }
                            else if (m_pCurrentType == null && !m_bcanIncludeExtensionsInResults)
                            {
                                return false;
                            }
                            else
                            {
                                // Found an instance method.
                                continue;
                            }
                        }
                        continue;
                    }

                    // Note that we do not filter the current symbol for the user. They must do that themselves.
                    // This is because for instance, BindGrpToArgs wants to filter on arguments before filtering
                    // on bogosity. See DevDiv Bug 24236.

                    // If we're here, we're good to go.

                    break;
                }
                return true;
            }

            private bool FindNextTypeForInstanceMethods()
            {
                // Otherwise, search through other types listed as well as our base class.
                if (m_pContainingTypes.size > 0)
                {
                    if (m_nCurrentTypeCount >= m_pContainingTypes.size)
                    {
                        // No more types to check.
                        m_pCurrentType = null;
                    }
                    else
                    {
                        m_pCurrentType = m_pContainingTypes.Item(m_nCurrentTypeCount++).AsAggregateType();
                    }
                }
                else
                {
                    // We have no more types to consider, so check out the base class.

                    m_pCurrentType = m_pCurrentType.GetBaseClass();
                }
                return m_pCurrentType != null;
            }
        }
    }
}
