// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Security;
using System.Security.Permissions;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // Name used for AGGDECLs in the symbol table.

    // AggregateSymbol - a symbol representing an aggregate type. These are classes,
    // interfaces, and structs. Parent is a namespace or class. Children are methods,
    // properties, and member variables, and types (including its own AGGTYPESYMs).

    class AggregateSymbol : NamespaceOrAggregateSymbol
    {
        public Type AssociatedSystemType;
        public Assembly AssociatedAssembly;

        // This InputFile is some infile for the assembly containing this AggregateSymbol.
        // It is used for fast access to the filter BitSet and assembly ID.
        InputFile infile;

        // The instance type. Created when first needed.
        AggregateType atsInst;

        AggregateType m_pBaseClass;     // For a class/struct/enum, the base class. For iface: unused.
        AggregateType m_pUnderlyingType; // For enum, the underlying type. For iface, the resolved CoClass. Not used for class/struct.

        TypeArray m_ifaces;         // The explicit base interfaces for a class or interface.
        TypeArray m_ifacesAll;      // Recursive closure of base interfaces ordered so an iface appears before all of its base ifaces.

        TypeArray m_typeVarsThis; // Type variables for this generic class, as declarations.
        TypeArray m_typeVarsAll;     // The type variables for this generic class and all containing classes.

        TypeManager m_pTypeManager;     // This is so AGGTYPESYMs can instantiate their baseClass and ifacesAll members on demand.

        // First UD conversion operator. This chain is for this type only (not base types).
        // The hasConversion flag indicates whether this or any base types have UD conversions.
        MethodSymbol m_pConvFirst;

        // ------------------------------------------------------------------------
        // 
        // Put members that are bits under here in a contiguous section.
        //
        // ------------------------------------------------------------------------

        AggKindEnum aggKind;

        bool m_isLayoutError; // Whether there is a cycle in the layout for the struct

        // Where this came from - fabricated, source, import
        // REVIEW : Remove isSource? Since incremental is gone.
        // Fabricated AGGs have isSource == true but hasParseTree == false.
        // N.B.: in incremental builds, it is quite possible for
        // isSource==TRUE and hasParseTree==FALSE. Be
        // sure you use the correct variable for what you are trying to do!
        bool m_isSource;    // This class is defined in source, although the 
        // source might not be being read during this compile.

        // Predefined
        bool m_isPredefined;    // A special predefined type.
        PredefinedType m_iPredef;        // index of the predefined type, if isPredefined.

        // Flags
        bool m_isAbstract;      // Can it be instantiated?
        bool m_isSealed;        // Can it be derived from?

        // Attribute

        bool m_isUnmanagedStruct; // Set if the struct is known to be un-managed (for unsafe code). Set in FUNCBREC.
        bool m_isManagedStruct; // Set if the struct is known to be managed (for unsafe code). Set during import.

        // Constructors
        bool m_hasPubNoArgCtor; // Whether it has a public instance ructor taking no args

        // private struct members should not be checked for assignment or references
        bool m_hasExternReference;

        // User defined operators

        bool m_isSkipUDOps; // Never check for user defined operators on this type (eg, decimal, string, delegate).

        bool m_isComImport;     // Does it have [ComImport]

        bool isAnonymousType;    // true if the class is an anonymous type
        // When this is unset we don't know if we have conversions.  When this 
        // is set it indicates if this type or any base type has user defined 
        // conversion operators
        bool? m_hasConversion;

        // ----------------------------------------------------------------------------
        // AggregateSymbol
        // ----------------------------------------------------------------------------

        public AggregateSymbol GetBaseAgg()
        {
            return m_pBaseClass == null ? null : m_pBaseClass.getAggregate();
        }

        public AggregateType getThisType()
        {
            if (atsInst == null)
            {
                Debug.Assert(GetTypeVars() == GetTypeVarsAll() || isNested());

                AggregateType pOuterType = this.isNested() ? GetOuterAgg().getThisType() : null;

                atsInst = m_pTypeManager.GetAggregate(this, pOuterType, GetTypeVars());
            }

            //Debug.Assert(GetTypeVars().Size == atsInst.GenericArguments.Count);
            return atsInst;
        }

        public void InitFromInfile(InputFile infile)
        {
            this.infile = infile;
            m_isSource = infile.isSource;
        }

        public bool FindBaseAgg(AggregateSymbol agg)
        {
            for (AggregateSymbol aggT = this; aggT != null; aggT = aggT.GetBaseAgg())
            {
                if (aggT == agg)
                    return true;
            }
            return false;
        }

        public NamespaceOrAggregateSymbol Parent
        {
            get { return parent.AsNamespaceOrAggregateSymbol(); }
        }

        public new AggregateDeclaration DeclFirst()
        {
            return (AggregateDeclaration)base.DeclFirst();
        }

        public AggregateDeclaration DeclOnly()
        {
            //Debug.Assert(DeclFirst() != null && DeclFirst().DeclNext() == null);
            return DeclFirst();
        }

        public bool InAlias(KAID aid)
        {
            Debug.Assert(infile != null);
            //Debug.Assert(DeclFirst() == null || DeclFirst().GetAssemblyID() == infile.GetAssemblyID());
            Debug.Assert(0 <= aid);
            if (aid < KAID.kaidMinModule)
                return infile.InAlias(aid);
            return (aid == GetModuleID());
        }

        public KAID GetModuleID()
        {
            return 0;
        }

        public KAID GetAssemblyID()
        {
            Debug.Assert(infile != null);
            //Debug.Assert(DeclFirst() == null || DeclFirst().GetAssemblyID() == infile.GetAssemblyID());
            return infile.GetAssemblyID();
        }

        public bool IsUnresolved()
        {
            return infile != null && infile.GetAssemblyID() == KAID.kaidUnresolved;
        }

        public bool isNested()
        {
            return parent != null && parent.IsAggregateSymbol();
        }

        public AggregateSymbol GetOuterAgg()
        {
            return parent != null && parent.IsAggregateSymbol() ? parent.AsAggregateSymbol() : null;
        }

#if false
    IMetaDataImport2 * GetMetaImportV2()
    {
        return this.GetModule().GetMetaImportV2();
    }

    IMetaDataImport * GetMetaImport()
    {
        return this.GetModule().GetMetaImport();
    }
#endif

        public bool isPredefAgg(PredefinedType pt)
        {
            return this.m_isPredefined && (PredefinedType)this.m_iPredef == pt;
        }

        // ----------------------------------------------------------------------------
        // The following are the Accessor functions for AggregateSymbol.
        // ----------------------------------------------------------------------------

        public AggKindEnum AggKind()
        {
            return (AggKindEnum)aggKind;
        }

        public void SetAggKind(AggKindEnum aggKind)
        {
            // NOTE: When importing can demote types:
            //  - enums with no underlying type go to struct
            //  - delegates which are abstract or have no .ctor/Invoke method goto class
            this.aggKind = aggKind;

            //An interface is always abstract
            if (aggKind == AggKindEnum.Interface)
            {
                this.SetAbstract(true);
            }
        }

        public bool IsClass()
        {
            return AggKind() == AggKindEnum.Class;
        }

        public bool IsDelegate()
        {
            return AggKind() == AggKindEnum.Delegate;
        }

        public bool IsInterface()
        {
            return AggKind() == AggKindEnum.Interface;
        }

        public bool IsStruct()
        {
            return AggKind() == AggKindEnum.Struct;
        }

        public bool IsEnum()
        {
            return AggKind() == AggKindEnum.Enum;
        }

        public bool IsValueType()
        {
            return AggKind() == AggKindEnum.Struct || AggKind() == AggKindEnum.Enum;
        }

        public bool IsRefType()
        {
            return AggKind() == AggKindEnum.Class ||
                AggKind() == AggKindEnum.Interface || AggKind() == AggKindEnum.Delegate;
        }

        public bool IsStatic()
        {
            return (m_isAbstract && m_isSealed);
        }



        public bool IsAnonymousType()
        {
            return isAnonymousType;
        }

        public void SetAnonymousType(bool isAnonymousType)
        {
            this.isAnonymousType = isAnonymousType;
        }

        public bool IsAbstract()
        {
            return m_isAbstract;
        }

        public void SetAbstract(bool @abstract)
        {
            m_isAbstract = @abstract;
        }

        public bool IsPredefined()
        {
            return m_isPredefined;
        }

        public void SetPredefined(bool predefined)
        {
            m_isPredefined = predefined;
        }

        public PredefinedType GetPredefType()
        {
            // UNDONE:  add this back in some day ... Debug.Assert(IsPredefined());
            return (PredefinedType)m_iPredef;
        }

        public void SetPredefType(PredefinedType predef)
        {
            m_iPredef = predef;
        }

        public bool IsLayoutError()
        {
            return m_isLayoutError == true;
        }

        public void SetLayoutError(bool layoutError)
        {
            m_isLayoutError = layoutError;
        }

        public bool IsSealed()
        {
            return m_isSealed == true;
        }

        public void SetSealed(bool @sealed)
        {
            m_isSealed = @sealed;
        }

        ////////////////////////////////////////////////////////////////////////////////

        public bool HasConversion(SymbolLoader pLoader)
        {
            pLoader.RuntimeBinderSymbolTable.AddConversionsForType(AssociatedSystemType);

            if (!m_hasConversion.HasValue)
            {
                // ok, we tried defining all the conversions, and we didn't get anything
                // for this type.  However, we will still think this type has conversions
                // if it's base type has conversions.
                m_hasConversion = GetBaseAgg() != null && GetBaseAgg().HasConversion(pLoader);
            }

            return m_hasConversion.Value;
        }

        ////////////////////////////////////////////////////////////////////////////////

        public void SetHasConversion()
        {
            m_hasConversion = true;
        }

        ////////////////////////////////////////////////////////////////////////////////

        public bool IsUnmanagedStruct()
        {
            return m_isUnmanagedStruct == true;
        }

        public void SetUnmanagedStruct(bool unmanagedStruct)
        {
            m_isUnmanagedStruct = unmanagedStruct;
        }

        public bool IsManagedStruct()
        {
            return m_isManagedStruct == true;
        }

        public void SetManagedStruct(bool managedStruct)
        {
            m_isManagedStruct = managedStruct;
        }

        public bool IsKnownManagedStructStatus()
        {
            Debug.Assert(this.IsStruct());
            Debug.Assert(!IsManagedStruct() || !IsUnmanagedStruct());
            return IsManagedStruct() || IsUnmanagedStruct();
        }

        public bool HasPubNoArgCtor()
        {
            return m_hasPubNoArgCtor == true;
        }

        public void SetHasPubNoArgCtor(bool hasPubNoArgCtor)
        {
            m_hasPubNoArgCtor = hasPubNoArgCtor;
        }

        public bool HasExternReference()
        {
            return m_hasExternReference == true;
        }

        public void SetHasExternReference(bool hasExternReference)
        {
            m_hasExternReference = hasExternReference;
        }


        public bool IsSkipUDOps()
        {
            return m_isSkipUDOps == true;
        }

        public void SetSkipUDOps(bool skipUDOps)
        {
            m_isSkipUDOps = skipUDOps;
        }

        public void SetComImport(bool comImport)
        {
            m_isComImport = comImport;
        }
        
        public bool IsSource()
        {
            return m_isSource == true;
        }

        public TypeArray GetTypeVars()
        {
            return m_typeVarsThis;
        }

        public void SetTypeVars(TypeArray typeVars)
        {
            if (typeVars == null)
            {
                m_typeVarsThis = null;
                m_typeVarsAll = null;
            }
            else
            {
                TypeArray outerTypeVars;
                if (this.GetOuterAgg() != null)
                {
                    Debug.Assert(this.GetOuterAgg().GetTypeVars() != null);
                    Debug.Assert(this.GetOuterAgg().GetTypeVarsAll() != null);

                    outerTypeVars = this.GetOuterAgg().GetTypeVarsAll();
                }
                else
                {
                    outerTypeVars = BSYMMGR.EmptyTypeArray();
                }

                m_typeVarsThis = typeVars;
                m_typeVarsAll = m_pTypeManager.ConcatenateTypeArrays(outerTypeVars, typeVars);
            }
        }

        public TypeArray GetTypeVarsAll()
        {
            return m_typeVarsAll;
        }

        public AggregateType GetBaseClass()
        {
            return m_pBaseClass;
        }

        public void SetBaseClass(AggregateType baseClass)
        {
            m_pBaseClass = baseClass;
        }

        public AggregateType GetUnderlyingType()
        {
            return m_pUnderlyingType;
        }

        public void SetUnderlyingType(AggregateType underlyingType)
        {
            m_pUnderlyingType = underlyingType;
        }

        public TypeArray GetIfaces()
        {
            return m_ifaces;
        }

        public void SetIfaces(TypeArray ifaces)
        {
            m_ifaces = ifaces;
        }

        public TypeArray GetIfacesAll()
        {
            return m_ifacesAll;
        }

        public void SetIfacesAll(TypeArray ifacesAll)
        {
            m_ifacesAll = ifacesAll;
        }

        public TypeManager GetTypeManager()
        {
            return m_pTypeManager;
        }

        public void SetTypeManager(TypeManager typeManager)
        {
            m_pTypeManager = typeManager;
        }

        public MethodSymbol GetFirstUDConversion()
        {
            return m_pConvFirst;
        }

        public void SetFirstUDConversion(MethodSymbol conv)
        {
            m_pConvFirst = conv;
        }

        public new bool InternalsVisibleTo(Assembly assembly)
        {
            return m_pTypeManager.InternalsVisibleTo(AssociatedAssembly, assembly);
        }
    }
}
