// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // AggregateType
    //
    // Represents a genericructed (or instantiated) type. Parent is the AggregateSymbol.
    // ----------------------------------------------------------------------------

    partial class AggregateType : CType
    {
        private TypeArray m_pTypeArgsThis;
        private TypeArray m_pTypeArgsAll;         // includes args from outer types
        private AggregateSymbol m_pOwningAggregate;

#if ! CSEE // The EE can't cache these since the AGGSYMs may change as things are imported.
        private AggregateType baseType;  // This is the result of calling SubstTypeArray on the aggregate's baseClass.
        private TypeArray ifacesAll;  // This is the result of calling SubstTypeArray on the aggregate's ifacesAll.
        private TypeArray winrtifacesAll; //This is the list of collection interfaces implemented by a WinRT object.
#else // !CSEE

        public short proxyOID ; // oid for the managed proxy in the host running inside the debugee
        public short typeConverterID ;
#endif // !CSEE

        public bool fConstraintsChecked;    // Have theraints been checked yet?
        public bool fConstraintError;       // Did theraints check produce an error?

        // These two flags are used to track hiding within interfaces.
        // Their use and validity is always localized. See e.g. MemberLookup::LookupInInterfaces.
        public bool fAllHidden;             // All members are hidden by a derived interface member.
        public bool fDiffHidden;            // Members other than a specific kind are hidden by a derived interface member or class member.

        public AggregateType outerType;          // the outer type if this is a nested type

        public void SetOwningAggregate(AggregateSymbol agg)
        {
            m_pOwningAggregate = agg;
        }

        public AggregateSymbol GetOwningAggregate()
        {
            return m_pOwningAggregate;
        }

        public AggregateType GetBaseClass()
        {
#if CSEE
            AggregateType atsBase = getAggregate().GetBaseClass();
            if (!atsBase || GetTypeArgsAll().size == 0 || atsBase.GetTypeArgsAll().size == 0)
                return atsBase;

            return getAggregate().GetTypeManager().SubstType(atsBase, GetTypeArgsAll()).AsAggregateType();
#else // !CSEE

            if (baseType == null)
            {
                baseType = getAggregate().GetTypeManager().SubstType(getAggregate().GetBaseClass(), GetTypeArgsAll()) as AggregateType;
            }

            return baseType;
#endif // !CSEE
        }

        public void SetTypeArgsThis(TypeArray pTypeArgsThis)
        {
            TypeArray pOuterTypeArgs;
            if (outerType != null)
            {
                Debug.Assert(outerType.GetTypeArgsThis() != null);
                Debug.Assert(outerType.GetTypeArgsAll() != null);

                pOuterTypeArgs = outerType.GetTypeArgsAll();
            }
            else
            {
                pOuterTypeArgs = BSYMMGR.EmptyTypeArray();
            }

            Debug.Assert(pTypeArgsThis != null);
            m_pTypeArgsThis = pTypeArgsThis;
            SetTypeArgsAll(pOuterTypeArgs);
        }

        public void SetTypeArgsAll(TypeArray outerTypeArgs)
        {
            Debug.Assert(m_pTypeArgsThis != null);

            // Here we need to check our current type args. If we have an open placeholder,
            // then we need to have all open placeholders, and we want to flush
            // our outer type args so that they're open placeholders. 
            //
            // This is because of the following scenario:
            //
            // class B<T>
            // {
            //     class C<U>
            //     {
            //     }
            //     class D
            //     {
            //         void Foo()
            //         {
            //             Type T = typeof(C<>);
            //         }
            //     }
            // }
            //
            // The outer type will be B<T>, but the inner type will be C<>. However,
            // this will eventually be represented in IL as B<>.C<>. As such, we should
            // keep our data structures clear - if we have one open type argument, then
            // all of them must be open type arguments.
            //
            // Ensure that invariant here.

            TypeArray pCheckedOuterTypeArgs = outerTypeArgs;
            TypeManager pTypeManager = getAggregate().GetTypeManager();

            if (m_pTypeArgsThis.Size > 0 && AreAllTypeArgumentsUnitTypes(m_pTypeArgsThis))
            {
                if (outerTypeArgs.Size > 0 && !AreAllTypeArgumentsUnitTypes(outerTypeArgs))
                {
                    // We have open placeholder types in our type, but not the parent.
                    pCheckedOuterTypeArgs = pTypeManager.CreateArrayOfUnitTypes(outerTypeArgs.Size);
                }
            }
            m_pTypeArgsAll = pTypeManager.ConcatenateTypeArrays(pCheckedOuterTypeArgs, m_pTypeArgsThis);
        }

        public bool AreAllTypeArgumentsUnitTypes(TypeArray typeArray)
        {
            if (typeArray.Size == 0)
            {
                return true;
            }

            for (int i = 0; i < typeArray.size; i++)
            {
                Debug.Assert(typeArray.Item(i) != null);
                if (!typeArray.Item(i).IsOpenTypePlaceholderType())
                {
                    return false;
                }
            }
            return true;
        }

        public TypeArray GetTypeArgsThis()
        {
            return m_pTypeArgsThis;
        }

        public TypeArray GetTypeArgsAll()
        {
            return m_pTypeArgsAll;
        }

        public TypeArray GetIfacesAll()
        {
            if (ifacesAll == null)
            {
                ifacesAll = getAggregate().GetTypeManager().SubstTypeArray(getAggregate().GetIfacesAll(), GetTypeArgsAll());
            }
            return ifacesAll;
        }

        public TypeArray GetWinRTCollectionIfacesAll(SymbolLoader pSymbolLoader)
        {
            if (winrtifacesAll == null)
            {
                TypeArray ifaces = GetIfacesAll();
                System.Collections.Generic.List<CType> typeList = new System.Collections.Generic.List<CType>();

                for (int i = 0; i < ifaces.size; i++)
                {
                    AggregateType type = ifaces.Item(i).AsAggregateType();
                    Debug.Assert(type.isInterfaceType());

                    if (type.IsCollectionType())
                    {
                        typeList.Add(type);
                    }
                }
                winrtifacesAll = pSymbolLoader.getBSymmgr().AllocParams(typeList.Count, typeList.ToArray());
            }
            return winrtifacesAll;
        }

        // UNDONE: Can we redo the implementation of this so that it does not
        // UNDONE: use the global symbol loader?  Iterate over the type children
        // UNDONE: to look for the Invoke method.

        public TypeArray GetDelegateParameters(SymbolLoader pSymbolLoader)
        {
            Debug.Assert(isDelegateType());
            MethodSymbol invoke = pSymbolLoader.LookupInvokeMeth(this.getAggregate());
            if (invoke == null || !invoke.isInvoke())
            {
                // This can happen if the delegate is internal to another assembly. 
                return null;
            }
            return this.getAggregate().GetTypeManager().SubstTypeArray(invoke.Params, this);
        }

        public CType GetDelegateReturnType(SymbolLoader pSymbolLoader)
        {
            Debug.Assert(isDelegateType());
            MethodSymbol invoke = pSymbolLoader.LookupInvokeMeth(this.getAggregate());
            if (invoke == null || !invoke.isInvoke())
            {
                // This can happen if the delegate is internal to another assembly. 
                return null;
            }
            return this.getAggregate().GetTypeManager().SubstType(invoke.RetType, this);
        }
    }
}

