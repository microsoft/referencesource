// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;
using System.Collections.Generic;
using System.Diagnostics;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    struct KeyPair<Key1, Key2> : IEquatable<KeyPair<Key1, Key2>>
    {
        Key1 m_pKey1;
        Key2 m_pKey2;

        public KeyPair(Key1 pKey1, Key2 pKey2)
        {
            m_pKey1 = pKey1;
            m_pKey2 = pKey2;
        }

        public bool Equals(KeyPair<Key1, Key2> other)
        {
            return object.Equals(this.m_pKey1, other.m_pKey1)
                && object.Equals(this.m_pKey2, other.m_pKey2);
        }

        public override bool Equals(object obj)
        {
            if (!(obj is KeyPair<Key1, Key2>)) return false;
            return this.Equals((KeyPair<Key1, Key2>)obj);
        }

        public override int GetHashCode()
        {
            return (this.m_pKey1 == null ? 0 : this.m_pKey1.GetHashCode())
                + (this.m_pKey2 == null ? 0 : this.m_pKey2.GetHashCode());
        }
    }

    class TypeTable
    {
        // Two way hashes
        Dictionary<KeyPair<AggregateSymbol, Name>, AggregateType> m_pAggregateTable;
        Dictionary<KeyPair<CType, Name>, ErrorType> m_pErrorWithTypeParentTable;
        Dictionary<KeyPair<AssemblyQualifiedNamespaceSymbol, Name>, ErrorType> m_pErrorWithNamespaceParentTable;
        Dictionary<KeyPair<CType, Name>, ArrayType> m_pArrayTable;
        Dictionary<KeyPair<CType, Name>, ParameterModifierType> m_pParameterModifierTable;

        // One way hashes
        Dictionary<CType, PointerType> m_pPointerTable;
        Dictionary<CType, NullableType> m_pNullableTable;
        Dictionary<TypeParameterSymbol, TypeParameterType> m_pTypeParameterTable;

        public TypeTable()
        {
            this.m_pAggregateTable = new Dictionary<KeyPair<AggregateSymbol, Name>, AggregateType>();
            this.m_pErrorWithNamespaceParentTable = new Dictionary<KeyPair<AssemblyQualifiedNamespaceSymbol, Name>, ErrorType>();
            this.m_pErrorWithTypeParentTable = new Dictionary<KeyPair<CType, Name>, ErrorType>();
            this.m_pArrayTable = new Dictionary<KeyPair<CType, Name>, ArrayType>();
            this.m_pParameterModifierTable = new Dictionary<KeyPair<CType, Name>, ParameterModifierType>();
            this.m_pPointerTable = new Dictionary<CType, PointerType>();
            this.m_pNullableTable = new Dictionary<CType, NullableType>();
            this.m_pTypeParameterTable = new Dictionary<TypeParameterSymbol, TypeParameterType>();
        }

        public AggregateType LookupAggregate(Name pName, AggregateSymbol pAggregate)
        {
            var key = new KeyPair<AggregateSymbol, Name>(pAggregate, pName);
            AggregateType result;
            if (m_pAggregateTable.TryGetValue(key, out result))
            {
                return result;
            }
            return null;
        }

        public void InsertAggregate(
                Name pName,
                AggregateSymbol pAggregateSymbol,
                AggregateType pAggregate)
        {
            Debug.Assert(LookupAggregate(pName, pAggregateSymbol) == null);
            m_pAggregateTable.Add(new KeyPair<AggregateSymbol, Name>(pAggregateSymbol, pName), pAggregate);
        }

        public ErrorType LookupError(Name pName, CType pParentType)
        {
            var key = new KeyPair<CType, Name>(pParentType, pName);
            ErrorType result;
            if (m_pErrorWithTypeParentTable.TryGetValue(key, out result))
            {
                return result;
            }
            return null;
        }

        public ErrorType LookupError(Name pName, AssemblyQualifiedNamespaceSymbol pParentNS)
        {
            var key = new KeyPair<AssemblyQualifiedNamespaceSymbol, Name>(pParentNS, pName);
            ErrorType result;
            if (m_pErrorWithNamespaceParentTable.TryGetValue(key, out result))
            {
                return result;
            }
            return null;
        }

        public void InsertError(Name pName, CType pParentType, ErrorType pError)
        {
            Debug.Assert(LookupError(pName, pParentType) == null);
            m_pErrorWithTypeParentTable.Add(new KeyPair<CType, Name>(pParentType, pName), pError);
        }

        public void InsertError(Name pName, AssemblyQualifiedNamespaceSymbol pParentNS, ErrorType pError)
        {
            Debug.Assert(LookupError(pName, pParentNS) == null);
            m_pErrorWithNamespaceParentTable.Add(new KeyPair<AssemblyQualifiedNamespaceSymbol, Name>(pParentNS, pName), pError);
        }

        public ArrayType LookupArray(Name pName, CType pElementType)
        {
            var key = new KeyPair<CType, Name>(pElementType, pName);
            ArrayType result;
            if (m_pArrayTable.TryGetValue(key, out result))
            {
                return result;
            }
            return null;
        }

        public void InsertArray(Name pName, CType pElementType, ArrayType pArray)
        {
            Debug.Assert(LookupArray(pName, pElementType) == null);
            m_pArrayTable.Add(new KeyPair<CType, Name>(pElementType, pName), pArray);
        }

        public ParameterModifierType LookupParameterModifier(Name pName, CType pElementType)
        {
            var key = new KeyPair<CType, Name>(pElementType, pName);
            ParameterModifierType result;
            if (m_pParameterModifierTable.TryGetValue(key, out result))
            {
                return result;
            }
            return null;
        }

        public void InsertParameterModifier(
                Name pName,
                CType pElementType,
                ParameterModifierType pParameterModifier)
        {
            Debug.Assert(LookupParameterModifier(pName, pElementType) == null);
            m_pParameterModifierTable.Add(new KeyPair<CType, Name>(pElementType, pName), pParameterModifier);
        }

        public PointerType LookupPointer(CType pElementType)
        {
            PointerType result;
            if (m_pPointerTable.TryGetValue(pElementType, out result))
            {
                return result;
            }
            return null;
        }

        public void InsertPointer(CType pElementType, PointerType pPointer)
        {
            m_pPointerTable.Add(pElementType, pPointer);
        }

        public NullableType LookupNullable(CType pUnderlyingType)
        {
            NullableType result;
            if (m_pNullableTable.TryGetValue(pUnderlyingType, out result))
            {
                return result;
            }
            return null;
        }

        public void InsertNullable(CType pUnderlyingType, NullableType pNullable)
        {
            m_pNullableTable.Add(pUnderlyingType, pNullable);
        }

        public TypeParameterType LookupTypeParameter(TypeParameterSymbol pTypeParameterSymbol)
        {
            TypeParameterType result;
            if (m_pTypeParameterTable.TryGetValue(pTypeParameterSymbol, out result))
            {
                return result;
            }
            return null;
        }

        public void InsertTypeParameter(
                TypeParameterSymbol pTypeParameterSymbol,
                TypeParameterType pTypeParameter)
        {
            m_pTypeParameterTable.Add(pTypeParameterSymbol, pTypeParameter);
        }
    }
}