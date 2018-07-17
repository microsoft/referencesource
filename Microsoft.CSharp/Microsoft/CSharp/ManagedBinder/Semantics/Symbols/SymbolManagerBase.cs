// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    struct AidContainer
    {
        internal static readonly AidContainer NullAidContainer = default(AidContainer);

        enum Kind
        {
            None = 0,
            File,
            ExternAlias
        }

        object m_value;

        public AidContainer(FileRecord file)
        {
            m_value = file;
        }
    }

    class BSYMMGR
    {
        internal HashSet<KAID> bsetGlobalAssemblies; // Assemblies in the global alias.

        // Special nullable members.
        public PropertySymbol propNubValue;
        public MethodSymbol methNubCtor;

        SymFactory symFactory;
        MiscSymFactory miscSymFactory;

        NamespaceSymbol rootNS;         // The "root" (unnamed) namespace.

        // Map from aids to INFILESYMs and EXTERNALIASSYMs
        protected List<AidContainer> ssetAssembly;
        // Map from aids to MODULESYMs and OUTFILESYMs

        protected NameManager m_nameTable;
        protected SYMTBL tableGlobal;

        // The hash table for type arrays.
        protected Dictionary<TypeArrayKey, TypeArray> tableTypeArrays;

        private InputFile m_infileUnres;

        const int LOG2_SYMTBL_INITIAL_BUCKET_CNT = 13;    // Initial local size: 8192 buckets.

        private static readonly TypeArray taEmpty = new TypeArray(new CType[] { });

        public BSYMMGR(NameManager nameMgr, TypeManager typeManager)
        {
            this.m_nameTable = nameMgr;
            this.tableGlobal = new SYMTBL();
            this.symFactory = new SymFactory(this.tableGlobal, this.m_nameTable);
            this.miscSymFactory = new MiscSymFactory(this.tableGlobal);

            this.ssetAssembly = new List<AidContainer>();

            this.m_infileUnres = new InputFile();
            this.m_infileUnres.isSource = false;
            this.m_infileUnres.SetAssemblyID(KAID.kaidUnresolved);

            this.ssetAssembly.Add(new AidContainer(m_infileUnres));
            this.bsetGlobalAssemblies = new HashSet<KAID>();
            this.bsetGlobalAssemblies.Add(KAID.kaidThisAssembly);
            this.tableTypeArrays = new Dictionary<TypeArrayKey, TypeArray>();
            this.rootNS = symFactory.CreateNamespace(m_nameTable.Add(""), null);
            GetNsAid(rootNS, KAID.kaidGlobal);
        }

        public void Init()
        {
            /*
            tableTypeArrays.Init(&this->GetPageHeap(), this->getAlloc());
            tableNameToSym.Init(this);
            nsToExtensionMethods.Init(this);

            // Some root symbols.
            Name* emptyName = m_nameTable->AddString(L"");
            rootNS = symFactory.CreateNamespace(emptyName, NULL);  // Root namespace
            nsaGlobal = GetNsAid(rootNS, kaidGlobal);

            m_infileUnres.name = emptyName;
            m_infileUnres.isSource = false;
            m_infileUnres.idLocalAssembly = mdTokenNil;
            m_infileUnres.SetAssemblyID(kaidUnresolved, allocGlobal);

            size_t isym;
            isym = ssetAssembly.Add(&m_infileUnres);
            ASSERT(isym == 0);
             */

            InitPreLoad();
        }


        public NameManager GetNameManager()
        {
            return m_nameTable;
        }

        public SYMTBL GetSymbolTable()
        {
            return tableGlobal;
        }

        public static TypeArray EmptyTypeArray()
        {
            return taEmpty;
        }

        public AssemblyQualifiedNamespaceSymbol GetRootNsAid(KAID aid)
        {
            return GetNsAid(rootNS, aid);
        }

        public NamespaceSymbol GetRootNS()
        {
            return rootNS;
        }

        public KAID AidAlloc(InputFile sym)
        {
            ssetAssembly.Add(new AidContainer(sym));
            return (KAID)(ssetAssembly.Count - 1 + KAID.kaidUnresolved);
        }

        public BetterType CompareTypes(TypeArray ta1, TypeArray ta2)
        {
            // IF YOU CHANGE THIS METHOD BE SURE TO UPDATE CMethodMemberRef::CompareSignature IN THE LANGAUGE SERVICE!!!

            if (ta1 == ta2)
            {
                return BetterType.Same;
            }
            if (ta1.Size != ta2.Size)
            {
                // The one with more parameters is more specific.
                return ta1.Size > ta2.Size ? BetterType.Left : BetterType.Right;
            }

            BetterType nTot = BetterType.Neither;

            for (int i = 0; i < ta1.Size; i++)
            {
                CType type1 = ta1.Item(i);
                CType type2 = ta2.Item(i);
                BetterType nParam = BetterType.Neither;

            LAgain:
                if (type1.GetTypeKind() != type2.GetTypeKind())
                {
                    if (type1.IsTypeParameterType())
                    {
                        nParam = BetterType.Right;
                    }
                    else if (type2.IsTypeParameterType())
                    {
                        nParam = BetterType.Left;
                    }
                }
                else
                {
                    switch (type1.GetTypeKind())
                    {
                        default:
                            Debug.Assert(false, "Bad kind in CompareTypes");
                            break;
                        case TypeKind.TK_TypeParameterType:
                        case TypeKind.TK_ErrorType:
                            break;

                        case TypeKind.TK_PointerType:
                        case TypeKind.TK_ParameterModifierType:
                        case TypeKind.TK_ArrayType:
                        case TypeKind.TK_NullableType:
                            type1 = type1.GetBaseOrParameterOrElementType();
                            type2 = type2.GetBaseOrParameterOrElementType();
                            goto LAgain;

                        case TypeKind.TK_AggregateType:
                            nParam = CompareTypes(type1.AsAggregateType().GetTypeArgsAll(), type2.AsAggregateType().GetTypeArgsAll());
                            break;
                    }
                }

                if (nParam == BetterType.Right || nParam == BetterType.Left)
                {
                    if (nTot == BetterType.Same || nTot == BetterType.Neither)
                    {
                        nTot = nParam;
                    }
                    else if (nParam != nTot)
                    {
                        return BetterType.Neither;
                    }
                }
            }

            return nTot;

            // IF YOU CHANGE THIS METHOD BE SURE TO UPDATE CMethodMemberRef::CompareSignature IN THE LANGAUGE SERVICE!!!
        }

        public SymFactory GetSymFactory()
        {
            return this.symFactory;
        }

        public MiscSymFactory GetMiscSymFactory()
        {
            return this.miscSymFactory;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Build the data structures needed to make FPreLoad fast. Make sure the 
        // namespaces are created. Compute and sort hashes of the NamespaceSymbol * value and type
        // name (sans arity indicator).

        void InitPreLoad()
        {
            for (int i = 0; i < (int)PredefinedType.PT_COUNT; ++i)
            {
                NamespaceSymbol ns = GetRootNS();
                string name = PredefinedTypeFacts.GetName((PredefinedType)i);
                int start = 0;
                while (start < name.Length)
                {
                    int iDot = name.IndexOf('.', start);
                    if (iDot == -1) break;
                    string sub = (iDot > start) ? name.Substring(start, iDot - start) : name.Substring(start);
                    Name nm = this.GetNameManager().Add(sub);
                    NamespaceSymbol sym = this.LookupGlobalSymCore(nm, ns, symbmask_t.MASK_NamespaceSymbol).AsNamespaceSymbol();
                    if (sym == null)
                    {
                        ns = this.symFactory.CreateNamespace(nm, ns);
                    }
                    else
                    {
                        ns = sym;
                    }
                    start += sub.Length + 1;
                }
            }
        }

        public Symbol LookupGlobalSymCore(Name name, ParentSymbol parent, symbmask_t kindmask)
        {
            return tableGlobal.LookupSym(name, parent, kindmask);
        }

        public Symbol LookupAggMember(Name name, AggregateSymbol agg, symbmask_t mask)
        {
            return tableGlobal.LookupSym(name, agg, mask);
        }

        public static Symbol LookupNextSym(Symbol sym, ParentSymbol parent, symbmask_t kindmask)
        {
            Debug.Assert(sym.parent == parent);

            sym = sym.nextSameName;
            Debug.Assert(sym == null || sym.parent == parent);

            // Keep traversing the list of symbols with same name and parent.
            while (sym != null)
            {
                if ((kindmask & sym.mask()) > 0)
                    return sym;

                sym = sym.nextSameName;
                Debug.Assert(sym == null || sym.parent == parent);
            }

            return null;
        }

        public Name GetNameFromPtrs(object u1, object u2)
        {
            // Note: this won't produce the same names as the native logic
            if (u2 != null)
            {
                return this.m_nameTable.Add(string.Format(CultureInfo.InvariantCulture, "{0:X}-{1:X}", u1.GetHashCode(), u2.GetHashCode()));
            }
            else
            {
                return this.m_nameTable.Add(string.Format(CultureInfo.InvariantCulture, "{0:X}", u1.GetHashCode()));
            }
        }

        public AssemblyQualifiedNamespaceSymbol GetNsAid(NamespaceSymbol ns, KAID aid)
        {
            Name name = GetNameFromPtrs(aid, 0);
            Debug.Assert(name != null);

            AssemblyQualifiedNamespaceSymbol nsa = LookupGlobalSymCore(name, ns, symbmask_t.MASK_AssemblyQualifiedNamespaceSymbol).AsAssemblyQualifiedNamespaceSymbol();
            if (nsa == null)
            {
                // Create a new one.
                nsa = symFactory.CreateNamespaceAid(name, ns, aid);
            }

            Debug.Assert(nsa.GetNS() == ns);

            return nsa;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Allocate a type array; used to represent a parameter list.
        // We use a hash table to make sure that allocating the same type array twice 
        // returns the same value. This does two things:
        //
        // 1) Save a lot of memory.
        // 2) Make it so parameter lists can be compared by a simple pointer comparison
        // 3) Allow us to associate a token with each signature for faster metadata emit

        protected struct TypeArrayKey : IEquatable<TypeArrayKey>
        {
            CType[] types;
            int hashCode;

            public TypeArrayKey(CType[] types)
            {
                this.types = types;
                this.hashCode = 0;
                for (int i = 0, n = types.Length; i < n; i++)
                {
                    this.hashCode ^= types[i].GetHashCode();
                }
            }

            public bool Equals(TypeArrayKey other)
            {
                if (other.types == this.types) return true;
                if (other.types.Length != this.types.Length) return false;
                if (other.hashCode != this.hashCode) return false;
                for (int i = 0, n = this.types.Length; i < n; i++)
                {
                    if (!this.types[i].Equals(other.types[i]))
                        return false;
                }
                return true;
            }

            public override bool Equals(object obj)
            {
                if (obj is TypeArrayKey)
                {
                    return this.Equals((TypeArrayKey)obj);
                }
                return false;
            }

            public override int GetHashCode()
            {
                return this.hashCode;
            }
        }

        public TypeArray AllocParams(int ctype, CType[] prgtype)
        {
            if (ctype == 0)
            {
                return taEmpty;
            }
            Debug.Assert(ctype == prgtype.Length);
            return AllocParams(prgtype);
        }

        // TODO: define this method
        public TypeArray AllocParams(int ctype, TypeArray array, int offset)
        {
            CType[] types = array.ToArray();
            CType[] newTypes = new CType[ctype];
#if SILVERLIGHT
            Array.Copy(types, offset, newTypes, 0, ctype);
#else
            Array.ConstrainedCopy(types, offset, newTypes, 0, ctype);
#endif
            return AllocParams(newTypes);
        }

        public TypeArray AllocParams(params CType[] types)
        {
            if (types == null || types.Length == 0)
            {
                return taEmpty;
            }
            TypeArrayKey key = new TypeArrayKey(types);
            TypeArray result;
            if (!tableTypeArrays.TryGetValue(key, out result))
            {
                result = new TypeArray(types);
                tableTypeArrays.Add(key, result);
            }
            return result;
        }

        public TypeArray ConcatParams(CType[] prgtype1, CType[] prgtype2)
        {
            CType[] combined = new CType[prgtype1.Length + prgtype2.Length];
            Array.Copy(prgtype1, combined, prgtype1.Length);
            Array.Copy(prgtype2, 0, combined, prgtype1.Length, prgtype2.Length);
            return AllocParams(combined);
        }

        public TypeArray ConcatParams(TypeArray pta1, TypeArray pta2)
        {
            return ConcatParams(pta1.ToArray(), pta2.ToArray());
        }
    }
}

