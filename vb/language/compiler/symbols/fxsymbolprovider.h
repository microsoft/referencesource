//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  The type tables for intrinsic types, and helper methods.
//
//-------------------------------------------------------------------------------------------------

#pragma once

namespace FX
{

    // ----------------------------------------------------------------------------
    // Enum for finding well-known .NET class symbols
    // ----------------------------------------------------------------------------

    enum TypeName
    {
        #define DEF_CLRSYM(type, clrscope, clrname, innetcf, runtimever)  type,
        #include "TypeTables.h"
        #undef DEF_CLRSYM
    };


    // The compiler caches a set of framework symbols that are used during
    // semantic analysis and codegen.  This captures the information we want
    // to have on hand - the symbol and the project that defined the symbol
    // We intend to store these in a CSingleList
    struct WellKnownFrameworkType : public CSingleLink<WellKnownFrameworkType>
    {
        BCSYM_NamedRoot *pFXType; // the framework type we want to have available during codegen/semantics
        CompilerProject *pCompilerProject; // the compiler project where this symbol came from
    };

    class FXSymbolProvider
    {

    public:

        FXSymbolProvider(Compiler * pCompiler);

        //
        // retrieve types
        //

        bool IsTypeAvailable(TypeName Type);
        BCSYM_NamedRoot* GetType(TypeName Type);

        // Overloads for intrinsic types

        BCSYM_NamedRoot * GetType(Vtypes vtype);
        STRING * GetIntrinsicName(Vtypes vtype);
        STRING* GetIntrinsicVBName(Vtypes vtype) const;
        STRING* GetTypeName(TypeName type);
        STRING* GetNamespaceName(TypeName type);
        STRING* GetFullyQualifiedName(TypeName type);

        //
        // Load types
        //

        bool LoadType(_In_z_ STRING *Namespace, BCSYM_NamedRoot *Class, CompilerProject *OwningProject );
        TypeName LoadType(_In_z_ STRING *Namespace, _In_z_ STRING *Class);
        bool LoadIntrinsicType(_In_z_ STRING *Namespace, BCSYM_Class *Class);

        //
        // Cleanup types
        //

        void RemoveFXTypesForProject(CompilerProject *Project);
    
        void ReportTypeMissingErrors(_In_ TypeName Type, _In_ ErrorTable *pErrorTable, _In_ const Location &ErrorLocation);
        //
        // Quick helpers for types
        //

        Type * GetIntegerType();
        Type * GetUnsignedIntegerType();
        Type * GetCharType();
        Type * GetSignedByteType();
        Type * GetByteType();
        Type * GetLongType();
        Type * GetUnsignedLongType();
        Type * GetShortType();
        Type * GetUnsignedShortType();  
        Type * GetDoubleType();
        Type * GetSingleType();
        Type * GetStringType();
        Type * GetBooleanType();
        Type * GetDecimalType();
        Type * GetDateType();

        Type * GetDelegateType();
        Type * GetMultiCastDelegateType();
        Type * GetObjectType();
        Type * GetRootArrayType();
        Type * GetExpressionType();
        Type * GetElementInitType();
        Type * GetNewExpressionType();
        Type * GetGenericExpressionType();
        Type * GetParameterExpressionType();
        Type * GetMemberBindingType();
        Type * GetTypeType();
        Type * GetFieldInfoType();
        Type * GetMethodInfoType();
        Type * GetConstructorInfoType();
        Type * GetMethodBaseType();

        BCSYM_NamedRoot * GetNullableIntrinsicSymbol(Vtypes vtype);
        BCSYM_NamedRoot * GetNullableType(BCSYM* pNamed, Symbols* pSymbols);
        BCSYM_NamedRoot * GetFuncSymbol(UINT nArgs, BCSYM* typeArgs[], Symbols* pSymbols);
        BCSYM_NamedRoot * GetTupleSymbol(UINT nArgs, BCSYM* typeArgs[], Symbols* pSymbols);

    private:

        Compiler * m_pCompiler;
        CSingleList<WellKnownFrameworkType> m_FXTypes[TypeMax]; // the well-known framework symbols we want to hang on to for codegen and semantics
        BCSYM_NamedRoot * m_NullableIntrinsicTypes[t_max + 1];

        // For intrinsic types

        struct IntrinsicClassTable
        {
            STRING *           m_pstrName;
            STRING *           m_pstrVBName;
            BCSYM_NamedRoot  * m_pnamed;
        };
        IntrinsicClassTable m_Intrinsics[t_max + 1];

    };

    //
    // inlined
    //

    inline BCSYM_NamedRoot* FXSymbolProvider::GetType(Vtypes vtype)
    {
        if (vtype >= 0 && vtype < _countof(m_Intrinsics))
        {
            BCSYM_NamedRoot *pnamed = m_Intrinsics[vtype].m_pnamed;
            VSASSERT(pnamed != NULL, "NULL intrinsic symbol!");

            return pnamed;
        }

        return NULL;
    }

    inline STRING* FXSymbolProvider::GetIntrinsicName(Vtypes vtype)
    {
        if (vtype >= 0 && vtype < _countof(m_Intrinsics))
        {
            VSASSERT(m_Intrinsics[vtype].m_pstrName != NULL, "NULL intrinsic name!");
            return m_Intrinsics[vtype].m_pstrName;
        }

        return NULL;
    }

    inline STRING* FXSymbolProvider::GetIntrinsicVBName(Vtypes vtype) const
    {
        if (vtype >= 0 && vtype < _countof(m_Intrinsics))
        {
            VSASSERT(m_Intrinsics[vtype].m_pstrVBName != NULL, "NULL intrinsic VB name!");
            return m_Intrinsics[vtype].m_pstrVBName;
        }

        return NULL;
    }

    inline Type * FXSymbolProvider::GetIntegerType()
    {
        return GetType(t_i4);
    }

    inline Type * FXSymbolProvider::GetUnsignedIntegerType()
    {
        return GetType(t_ui4);
    }

    inline Type * FXSymbolProvider::GetCharType()
    {
        return GetType(t_char);
    }

    inline Type * FXSymbolProvider::GetSignedByteType()
    {
        return GetType(t_i1);
    }

    inline Type * FXSymbolProvider::GetByteType()
    {
        return GetType(t_ui1);
    }

    inline Type * FXSymbolProvider::GetLongType()
    {
        return GetType(t_i8);
    }

    inline Type * FXSymbolProvider::GetUnsignedLongType()
    {
        return GetType(t_ui8);
    }

    inline Type * FXSymbolProvider::GetShortType()
    {
        return GetType(t_i2);
    }

    inline Type * FXSymbolProvider::GetUnsignedShortType()
    {
        return GetType(t_ui2);
    }

    inline Type * FXSymbolProvider::GetDoubleType()
    {
        return GetType(t_double);
    }

    inline Type * FXSymbolProvider::GetSingleType()
    {
        return GetType(t_single);
    }

    inline Type * FXSymbolProvider::GetStringType()
    {
        return GetType(t_string);
    }

    inline Type * FXSymbolProvider::GetBooleanType()
    {
        return GetType(t_bool);
    }

    inline Type * FXSymbolProvider::GetDecimalType()
    {
        return GetType(t_decimal);
    }

    inline Type * FXSymbolProvider::GetDateType()
    {
        return GetType(t_date);
    }

    inline Type * FXSymbolProvider::GetDelegateType()
    {
        return GetType(FX::DelegateType);
    }

    inline Type * FXSymbolProvider::GetMultiCastDelegateType()
    {
        return GetType(FX::MultiCastDelegateType);
    }

    inline Type * FXSymbolProvider::GetObjectType()
    {
        return GetType(FX::ObjectType);
    }

    inline Type * FXSymbolProvider::GetRootArrayType()
    {
        return GetType(FX::ArrayType);
    }

    inline Type * FXSymbolProvider::GetExpressionType()
    {
        return GetType(FX::ExpressionType);
    }

    inline Type * FXSymbolProvider::GetElementInitType()
    {
        return GetType(FX::ElementInitType);
    }

    inline Type * FXSymbolProvider::GetNewExpressionType()
    {
        return GetType(FX::NewExpressionType);
    }

    inline Type * FXSymbolProvider::GetGenericExpressionType()
    {
        return GetType(FX::GenericExpressionType);
    }

    inline Type * FXSymbolProvider::GetParameterExpressionType()
    {
        return GetType(FX::ParameterExpressionType);
    }

    inline Type * FXSymbolProvider::GetMemberBindingType()
    {
        return GetType(FX::MemberBindingType);
    }

    inline Type * FXSymbolProvider::GetTypeType()
    {
        return GetType(FX::TypeType);
    }

    inline Type * FXSymbolProvider::GetFieldInfoType()
    {
        return GetType(FX::FieldInfoType);
    }

    inline Type * FXSymbolProvider::GetMethodInfoType()
    {
        return GetType(FX::MethodInfoType);
    }

    inline Type * FXSymbolProvider::GetConstructorInfoType()
    {
        return GetType(FX::ConstructorInfoType);
    }

    inline Type * FXSymbolProvider::GetMethodBaseType()
    {
        return GetType(FX::MethodBaseType);
    }

}; // namespace FX
