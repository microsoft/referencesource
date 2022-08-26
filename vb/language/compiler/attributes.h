//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Definitions to support importing and emitting custom attributes.
//  Some custom attribute support is shared between the import and
//  emit phases, so it's all gathered together in the Attribute class.
//
//-------------------------------------------------------------------------------------------------

#pragma once

// Compile-time asserts to make sure the combined values are what we expect
COMPILE_ASSERT(catClassMembers == 0x000017fc);
//COMPILE_ASSERT(catAll == 0x00001fff);

// CorAttributeTargets in corhdr.h is missing this enum value.  Add it here for
// now, remove it when the header gets fixed.
const CorAttributeTargets catReturnValue = (CorAttributeTargets) 0x2000;

const int MaxConstructors = 12;  // Maximum number of overloaded constructors
const int MaxSignatureLength = 11; // Arbitrary number

// Microsoft.VisualBasic Attributes
#define VB_NAMESPACE L"Microsoft.VisualBasic."
#define COMCLASSATTRIBUTE_CLASSNAME L"ComClassAttribute"
#define COMCLASSATTRIBUTE (VB_NAMESPACE COMCLASSATTRIBUTE_CLASSNAME)
#define MYGROUPCOLLECTIONATTRIBUTE (VB_NAMESPACE L"MyGroupCollectionAttribute")
#define HIDEMOUDLENAMEATTRIBUTE (VB_NAMESPACE L"HideModuleNameAttribute")
#define EMBEDDEDATTRIBUTE (VB_NAMESPACE L"Embedded")

// Microsoft.VisualBasic.CompilerServices Attributes
#define VBCOMPILERSERVICES_NAMESPACE L"Microsoft.VisualBasic.CompilerServices"
#define DESIGNERGENERATEDATTRIBUTE ( VBCOMPILERSERVICES_NAMESPACE L".DesignerGeneratedAttribute")
#define OPTIONCOMPAREATTRIBUTE (VBCOMPILERSERVICES_NAMESPACE L".OptionCompareAttribute")
#define OPTIONTEXTATTRIBUTE (VBCOMPILERSERVICES_NAMESPACE  L".OptionTextAttribute")
#define STANDARDMODULEATTRIBUTE ( VBCOMPILERSERVICES_NAMESPACE L".StandardModuleAttribute")

// System.Runtime.CompilerServices Attributes
#define ACCESSEDTHROUGHPROPERTYATTRIBUTE (L"System.Runtime.CompilerServices.AccessedThroughPropertyAttribute")
#define COMPILATIONRELAXATIONATTRIBUTE (L"System.Runtime.CompilerServices.CompilationRelaxationsAttribute")
#define METHODIMPLATTRIBUTE (L"System.Runtime.CompilerServices.MethodImplAttribute")
#define DECIMALCONSTANTATTRIBUTE (L"System.Runtime.CompilerServices.DecimalConstantAttribute")
#define DATETIMECONSTANTATTRIBUTE (L"System.Runtime.CompilerServices.DateTimeConstantAttribute")
#define CALLERLINENUMBERATTRIBUTE (L"System.Runtime.CompilerServices.CallerLineNumberAttribute")
#define CALLERFILEPATHATTRIBUTE (L"System.Runtime.CompilerServices.CallerFilePathAttribute")
#define CALLERMEMBERNAMEATTRIBUTE (L"System.Runtime.CompilerServices.CallerMemberNameAttribute")

#define EXTENSIONATTRIBUTE ( L"System.Runtime.CompilerServices.ExtensionAttribute")

#define IDISPATCHCONSTANTATTRIBUTE (L"System.Runtime.CompilerServices.IDispatchConstantAttribute")
#define INTERNALSVISIBLETOATTRIBUTE (L"System.Runtime.CompilerServices.InternalsVisibleToAttribute")
#define IUNKNOWNCONSTANTATTRIBUTE (L"System.Runtime.CompilerServices.IUnknownConstantAttribute")
#define RUNTIMECOMPATIBILITYATTRIBUTE (L"System.Runtime.CompilerServices.RuntimeCompatibilityAttribute")

// System.ComponentModel Attributes
#define CATEGORYATTRIBUTE (L"System.ComponentModel.CategoryAttribute")
#define EXPORTATTRIBUTE (L"System.ComponentModel.Composition.ExportAttribute")
#define EXPORTMETADATAATTRIBUTE (L"System.ComponentModel.Composition.ExportMetadataAttribute")
#define IMPORTATTRIBUTE (L"System.ComponentModel.Composition.ImportAttribute")
#define IMPORTMANYATTRIBUTE (L"System.ComponentModel.Composition.ImportManyAttribute")
#define PARTEXPORTSINHERITEDATTRIBUTE (L"System.ComponentModel.Composition.PartExportsInheritedAttribute")
#define DEFAULTEVENTATTRIBUTE (L"System.ComponentModel.DefaultEventAttribute")
#define DEFAULTVALUEATTRIBUTE (L"System.ComponentModel.DefaultValueAttribute")
#define DESIGNERCATEGORYATTRIBUTE (L"System.ComponentModel.DesignerCategoryAttribute")
#define DESIGNERSERIALIZATIONVISIBILITYATTRIBUTE (L"System.ComponentModel.DesignerSerializationVisibilityAttribute")
#define EDITORBROWSABLEATTRIBUTE (L"System.ComponentModel.EditorBrowsableAttribute")

// System.ComponentModel.Design Attributes
#define HELPKEYWORDATTRIBUTE (L"System.ComponentModel.Design.HelpKeywordAttribute")

// System.Diagnostics Attributes
#define CONDITIONALATTRIBUTE (L"System.Diagnostics.ConditionalAttribute")
#define DEBUGGABLEATTRIBUTE (L"System.Diagnostics.DebuggableAttribute")
#define DEBUGGERBROWSABLEATTRIBUTE (L"System.Diagnostics.DebuggerBrowsableAttribute")
#define DEBUGGERDISPLAYATTRIBUTE (L"System.Diagnostics.DebuggerDisplayAttribute")
#define DEBUGGERHIDDENATTRIBUTE (L"System.Diagnostics.DebuggerHiddenAttribute")

// System.Reflection Attributes
#define DEFAULTMEMBERATTRIBUTE INTEROP_DEFAULTMEMBER_TYPE_W
#define ASSEMBLYKEYFILEATTRIBUTE (L"System.Reflection.AssemblyKeyFileAttribute")
#define ASSEMBLYKEYNAMEATTRIBUTE (L"System.Reflection.AssemblyKeyNameAttribute")
#define ASSEMBLYVERSIONATTRIBUTE (L"System.Reflection.AssemblyVersionAttribute")
#define ASSEMBLYDELAYSIGNATTRIBUTE (L"System.Reflection.AssemblyDelaySignAttribute")
#define ASSEMBLYFLAGSATTRIBUTE (L"System.Reflection.AssemblyFlagsAttribute")

// System.Runtime.InteropServices Attributes
#define BESTFITMAPPINGATTRIBUTE (L"System.Runtime.InteropServices.BestFitMappingAttribute")
#define CLASSINTERFACEATTRIBUTE INTEROP_CLASSINTERFACE_TYPE_W
#define COCLASSATTRIBUTE (L"System.Runtime.InteropServices.CoClassAttribute")

#define COMIMPORT_RAW L"ComImport"
#define COMIMPORTATTRIBUTE (L"System.Runtime.InteropServices." COMIMPORT_RAW L"Attribute")
#define COMIMPORTATTRIBUTE_NAME (COMIMPORT_RAW) 

#define COMSOURCEINTERFACES (L"System.Runtime.InteropServices.ComSourceInterfacesAttribute")
#define COMEVENTINTERFACEATTRIBUTE (L"System.Runtime.InteropServices.ComEventInterfaceAttribute")
#define COMVISIBLEATTRIBUTE INTEROP_COMVISIBLE_TYPE_W
#define DISPIDATTRIBUTE (L"System.Runtime.InteropServices.DispIdAttribute")
#define DLLIMPORTATTRIBUTE (L"System.Runtime.InteropServices.DllImportAttribute")
#define FIELDOFFSETATTRIBUTE (L"System.Runtime.InteropServices.FieldOffsetAttribute")

#define GUID_RAW L"Guid"
#define GUIDATTRIBUTE (L"System.Runtime.InteropServices." GUID_RAW L"Attribute")
#define GUIDATTRIBUTE_NAME (GUID_RAW)

#define IMPORTEDFROMTYLELIB_RAW L"ImportedFromTypeLib"
#define IMPORTEDFROMTYPELIBATTRIBUTE (L"System.Runtime.InteropServices." IMPORTEDFROMTYLELIB_RAW L"Attribute")
#define IMPORTEDFROMTYPELIBATTRIBUTE_NAME (IMPORTEDFROMTYLELIB_RAW)

#define INATTRIBUTE (L"System.Runtime.InteropServices.InAttribute")
#define TYPEIDENTIFIERATTRIBUTE (L"System.Runtime.InteropServices.TypeIdentifierAttribute")
#define INTERFACETYPEATTRIBUTE INTEROP_INTERFACETYPE_TYPE_W
#define LCIDCONVERSIONATTRIBUTE (L"System.Runtime.InteropServices.LCIDConversionAttribute")
#define MARSHALASATTRIBUTE (L"System.Runtime.InteropServices.MarshalAsAttribute")
#define OUTATTRIBUTE (L"System.Runtime.InteropServices.OutAttribute")

#define PRIMARYINTEROPASSEMBLY_RAW L"PrimaryInteropAssembly"
#define PRIMARYINTEROPASSEMBLYATTRIBUTE (L"System.Runtime.InteropServices." PRIMARYINTEROPASSEMBLY_RAW L"Attribute")
#define PRIMARYINTEROPASSEMBLYATTRIBUTE_NAME (PRIMARYINTEROPASSEMBLY_RAW)

#define STRUCT_LAYOUT_ATTRIBUTE (L"System.Runtime.InteropServices.StructLayoutAttribute")
#define TYPELIBFUNCATTRIBUTE INTEROP_TYPELIBFUNC_TYPE_W
#define TYPELIBTYPEATTRIBUTE INTEROP_TYPELIBTYPE_TYPE_W
#define TYPELIBVARATTRIBUTE (L"System.Runtime.InteropServices.TypeLibVarAttribute")
#define UNMANAGEDFUNCTIONPOINTERATTRIBUTE (L"System.Runtime.InteropServices.UnmanagedFunctionPointerAttribute")
#define PRESERVESIGATTRIBUTE (L"System.Runtime.InteropServices.PreserveSigAttribute")
#define DEFAULTPARAMETERVALUEATTRIBUTE (L"System.Runtime.InteropServices.DefaultParameterValueAttribute")
#define OPTIONALATTRIBUTE (L"System.Runtime.InteropServices.OptionalAttribute")

//System.Web.Services Attributes
#define WEBMETHODATTRIBUTE (L"System.Web.Services.WebMethodAttribute")
#define WEBSERVICEATTRIBUTE (L"System.Web.Services.WebServiceAttribute")

// System Attributes
#define ATTRIBUTEUSAGEATTRIBUTE (L"System.AttributeUsageAttribute")
#define CLSCOMPLIANTATTRIBUTE (L"System.CLSCompliantAttribute")
#define MTATHREADATTRIBUTE (L"System.MTAThreadAttribute")
#define NONSERIALIZEDATTRIBUTE (L"System.NonSerializedAttribute")
#define OBSOLETEATTRIBUTE  (L"System.ObsoleteAttribute")
#define DEPRECATEDATTRIBUTE  (L"Windows.Foundation.Metadata.DeprecatedAttribute")
#define PARAMARRAYATTRIBUTE INTEROP_PARAMARRAY_TYPE_W
#define SERIALIZABLEATTRIBUTE (L"System.SerializableAttribute")
#define STATHREADATTRIBUTE (L"System.STAThreadAttribute")
#define FLAGSATTRIBUTE (L"System.FlagsAttribute")

// System.Sercurity.SecurityCritical Attribute
#define SECURITYCRITICALATTRIBUTE (L"System.Security.SecurityCriticalAttribute")

// System.Sercurity.SecuritySafeCritical Attribute
#define SECURITYSAFECRITICALATTRIBUTE (L"System.Security.SecuritySafeCriticalAttribute")

//
// ConditionalString holds one element in a list of ConditionalAttribute strings.
//
struct ConditionalString
{
    STRING *m_pstrConditional;
    struct ConditionalString *m_pConditionalStringNext;
};

DECLARE_ENUM(DebuggerBrowsableState)
    Never = 0,
    Collapsed = 2,
    RootHidden = 3
END_ENUM(DebuggerBrowsableState)

//============================================================================
// AttrKind enumerates all of the well-known custom attributes.
//
// The enum member names encode the attribute name ("Obsolete"),
// the types of the constructor's positional params ("_Void" or
// "_String_Bool") and if there are optional named properties
// that can follow the positional parameters ("_NamedProps").
//
// *** This is only used to keep track of the names. ***
//============================================================================
enum AttrKind
{
    // Microsoft.VisualBasic Attributes
    attrkindComClass_Void,
    attrkindComClass_String,
    attrkindComClass_String_String,
    attrkindComClass_String_String_String,
    attrkindMyGroupCollection_String_String_String_String,
    attrkindHideModuleName_Void,
    attrkindEmbedded_Void,

    // Microsoft.VisualBasic.CompilerServices Attributes
    attrkindDesignerGenerated_Void,
    attrkindOptionCompare_Void,
    attrkindStandardModule_Void,

    // System Attributes
    attrkindAttributeUsage_AttributeTargets_NamedProps,
    attrkindCLSCompliant_Bool,
    attrkindFlags_Void,

    // System.ComponentModel Attributes
    attrkindCategory_Void,
    attrkindCategory_String,
    attrkindDefaultEvent_String,
    attrkindDefaultValue_Bool,
    attrkindDefaultValue_Byte,
    attrkindDefaultValue_Char,
    attrkindDefaultValue_Double,
    attrkindDefaultValue_Int16,
    attrkindDefaultValue_Int32,
    attrkindDefaultValue_Int64,
    attrkindDefaultValue_Object,
    attrkindDefaultValue_Single,
    attrkindDefaultValue_String,
    attrkindDefaultValue_Type_String,
    attrkindDesignerCategory_Void,
    attrkindDesignerCategory_String,
    attrkindDesignerSerializationVisibilityAttribute_DesignerSerializationVisibilityType,
    attrkindEditorBrowsable_Void,
    attrkindEditorBrowsable_EditorBrowsableState,

    // System.ComponentModel.Design Attributes
    attrkindHelpKeyword_Void,
    attrkindHelpKeyword_String,
    attrkindHelpKeyword_Type,

    //System.Diagnostics
    attrkindConditional_String,
    attrkindDebuggable_DebuggingModes,
    attrkindDebuggable_Bool_Bool,
    attrkindDebuggerBrowsable_BrowsableState,
    attrkindDebuggerDisplay_String,
    attrkindDebuggerHidden_Void,

    // System Attributes
    attrkindMTAThread_Void,
    attrkindNonSerialized_Void,

    // Insert Here

    attrkindObsolete_Void,
    attrkindObsolete_String,
    attrkindObsolete_String_Bool,

    attrkindDeprecated_String_DeprType_UInt,
    attrkindDeprecated_String_DeprType_UInt_Platform,

    attrkindParamArray_Void,


    // System.Reflection
    attrkindAssemblyKeyFile_String,
    attrkindAssemblyKeyName_String,
    attrkindAssemblyDelaySign_Bool,
    attrkindAssemblyVersion_String,
    attrkindDefaultMember_String,
    attrkindAssemblyFlags_Int,
    attrkindAssemblyFlags_UInt,
    attrkindAssemblyFlags_AssemblyNameFlags,

    // System.Runtime.CompilerServices
    attrkindAccessedThroughProperty_String,
    attrkindCompilationRelaxations_Int32,
    attrkindCompilationRelaxations_Enum,
    attrkindDateTimeConstant_Int64,
    attrkindDecimalConstant_Decimal,
    attrkindDecimalConstant_DecimalInt,
    attrkindExtension_Void,
    attrkindIDispatchConstant_void,
    attrkindInternalsVisibleTo_String,
    attrkindIUnknownConstant_void,
    attrkindRuntimeCompatibility_Void,
    attrkindCallerLineNumber_Void,
    attrkindCallerFilePath_Void,
    attrkindCallerMemberName_Void,

    // System.Runtime.InteropServices
    attrkindBestFitMapping_Bool,
    attrkindClassInterface_Int16,
    attrkindClassInterface_ClassInterfaceType,
    attrkindCoClass_Type,
    attrkindComEventInterface_Type_Type,
    attrkindComImport_Void,
    attrkindComSourceInterfaces_String,
    attrkindComSourceInterfaces_Type,
    attrkindComSourceInterfaces_Type_Type,
    attrkindComSourceInterfaces_Type_Type_Type,
    attrkindComSourceInterfaces_Type_Type_Type_Type,
    attrkindCOMVisible_Bool,
    attrkindDefaultParameterValue_Object,
    attrkindDispId_Int32,
    attrkindDllImport_String,
    attrkindFieldOffset_Int32,
    attrkindGuid_String,
    attrkindImportedFromTypeLib_String,
    attrkindIn_void,
    attrkindInterfaceType_Int16,
    attrkindInterfaceType_ComInterfaceType,
    attrkindLCIDConversion_Int32,
    attrkindMarshalAs_UnmanagedType,
    attrkindMarshalAs_Int16,
    attrkindOptional_void,
    attrkindOut_void,
    attrkindPreserveSig_void,
    attrkindPrimaryInteropAssembly_Int32_Int32,
    attrkindStructLayout_Int16,
    attrkindStructLayout_LayoutKind,
    attrkindMethodImpl_Int16,
    attrkindMethodImpl_MethodImplOptions,
    attrkindTypeIdentifier_void,
    attrkindTypeIdentifier_String_String,
    attrkindTypeLibFunc_Int16,
    attrkindTypeLibFunc_TypeLibFuncFlags,
    attrkindTypeLibType_Int16,
    attrkindTypeLibType_TypeLibTypeFlags,
    attrkindTypeLibVar_Int16,
    attrkindTypeLibVar_TypeLibVarFlags,
    attrkindUnmanagedFunctionPointer_CallingConvention,

    // System Attributes
    attrkindSerializable_Void,
    attrkindSTAThread_Void,

    // System.Web.Services Attributes
    attrkindWebMethod_Void_NamedProps,
    attrkindWebMethod_Boolean_NamedProps,
    attrkindWebMethod_Boolean_Transaction_NamedProps,
    attrkindWebMethod_Boolean_Transaction_Int32_NamedProps,
    attrkindWebMethod_Boolean_Transaction_Int32_Boolean_NamedProps,
    attrkindWebService_Void_NamedProps,

    // System.Security.SecurityCritical
    attrkindSecurityCritical_Void,
    attrkindSecurityCritical_Scope,

    // System.Security.SecuritySafeCritical
    attrkindSecuritySafeCritical_Void, 
    
    // Insert Here

    // Some special identifiers that don't map to attributes (i.e.,
    // there are no entries in g_rgAttrIdentity for these enums).
    attrkindMax,
    attrkindFirst = 0,
    attrkindNil = -1
};


//============================================================================
// Table of well-known custom attributes, identified by name and ctor sig.
//============================================================================
struct AttrIdentity
{
    AttrKind      m_attrkind;   // Which attribute is this?
    WCHAR        *m_pwszName;   // Fully-qualified name of attribute
    ULONG         m_cbSig;      // Actual size of m_sig, in bytes
    COR_SIGNATURE m_sig[MaxSignatureLength];     // Increase if necessary....
};

//============================================================================
// Structure to represent an attribute in the array of well known attributes.
//
// It contains the fully qualified name of the attribute and a pointer to an
// array of constructors.  The last element is denoted by containing
// attrkindNil for the AttrKind field..
//============================================================================
struct AttrIndex
{
    const WCHAR *wszName;

    unsigned char fCopyForNoPiaEmbeddedSymbols:1;
        
    // Debug only fields to verify that we have the correct information.
#if DEBUG
    bool IsAllowMultiple : 1;
    bool IsInheritable : 1;
#endif

    AttrIdentity Attributes[MaxConstructors];
};

AttrIndex g_rgAttributes[];

const AttrIndex *FindAttributeIndexByName
(
    _In_z_ const WCHAR *wszName   // IN: Attribute's fully-qualified name
);


const AttrIdentity *FindAttributeByID
(
    _In_z_ const WCHAR *wszName,   // IN: Attribute's fully-qualified name
    AttrKind AttributeID // IN: The Id of the specific constructor.
);

const AttrIdentity *FindAttributeBySignature
(
    const AttrIndex *pAttributeIndex, // IN: Pointer to the attribute index to search signatures in.
    ULONG cbSignature,    // IN: Length of signature in bytes
    PCCOR_SIGNATURE pSignature     // IN: CTOR signature to use to drive arg parsing
);

const AttrIdentity *FindAttributeBySignature
(
    _In_z_ const WCHAR *wszName,  // IN: Attribute's fully-qualified name. It can't be NULL
    ULONG cbSignature,    // IN: Length of signature in bytes
    PCCOR_SIGNATURE pSignature     // IN: CTOR signature to use to drive arg parsing
);


//============================================================================
// AttrVals is used to collect the settings of the various "well-known
// custom attributes."  (A custom attribute is considered "well-known"
// if the compiler needs to recognize and decode he attribute to produce
// correct results.)
//
// Any target that can have an attribute applied to it (e.g., BCSYM_Class)
// should allocate an AttrVals whenever an applied attribute is detected.
//
// AttrVals is also used at parse time to store a list of parsed attributes
// that have been applied to a given target.  That list is examined for
// well-known custom attributes (and the results of cracking those are stored
// directly in the AttrVals).  The list is also used to store all of the
// applied attributes (including non-well-known attributes) so they can be
// emitted and attached to the target's metadata.
//============================================================================
class WellKnownAttrVals
{

public:
    WellKnownAttrVals(NorlsAllocator* pAllocator);

    //============================================================================
    // Public getters.
    //============================================================================


    //
    // Because of the way AttrVals is used, all of the Getters below
    // handle the case where this == NULL.  I usually ----, ----, ----
    // this trick, but it really makes the callers' lives simpler.
    //
    // Getters all return false (and clear their OUT params) if
    // the given property is not present.
    //

    // Public getter for AttributeUsage attribute data
    bool GetAttributeUsageData(CorAttributeTargets *pattrtargetsValidOn, bool *pfAllowMultiple, bool *pfInherited);
    void SetAttributeUsageData(CorAttributeTargets targetsValidOn, bool fAllowMultiple, bool fInherited);

    // Public getter for Obsolete attribute data
    bool GetObsoleteData(_Deref_out_opt_z_ WCHAR **ppwszMessage, bool *pfError);
    void SetObsoleteData(_In_z_ WCHAR* pwszMessage, bool fError);

    // Public getter for Conditional attribute data
    bool GetConditionalData(ConditionalString **ppConditionalStringFirst);
    void AddConditionalData(_In_z_ WCHAR* pwszConditional);

    // Public getter for CLSCompliant attribute data
    bool GetCLSCompliantData(bool *pfCompliant, bool *pfIsInherited = NULL);
    void SetCLSCompliantData(bool fCompliant, bool fIsInherited);

    // Public getter for Flags attribute data
    bool GetFlagsData();
    void SetFlagsData();

    // Public getter and setter for PersistContents attribute data
    bool GetPersistContentsData(bool *pfPersistContents);
    void SetPersistContentsData(bool fPersistsContents);

    // Public getter for DefaultMember attribute data
    bool GetDefaultMemberData(_Deref_out_opt_z_ WCHAR **ppwszDefaultMember);
    void SetDefaultMemberData(_In_z_ WCHAR* pwszDefaultMember);

    // Public getter and setter for SecurityCritical attribute data
    bool GetSecurityCriticalData();
    void SetSecurityCriticalData();

    // Public getter and setter for SecuritySafeCritical attribute data
    bool GetSecuritySafeCriticalData();
    void SetSecuritySafeCriticalData();

    // Public getter and setter for DesignerCategory attribute data
    bool GetDesignerCategoryData(_Deref_out_opt_z_ WCHAR **ppwszCategory);
    void SetDesignerCategoryData(_In_z_ WCHAR* pwszDesignerCategory);

    // Public getter and setter for AccessedThroughProperty attribute data
    bool GetAccessedThroughPropertyData(_Deref_out_opt_z_ WCHAR **ppwszProperty);
    void SetAccessedThroughPropertyData(_In_z_ WCHAR* pwszProperty);

    // Public getter and setter for CompilationRelaxations attribute data
    bool GetCompilationRelaxationsData();
    void SetCompilationRelaxationsData();

    // Public getter for CompilationRelaxations attribute data
    bool GetRuntimeCompatibilityData();
    void SetRuntimeCompatibilityData();

    // Public getter and setter for CompilationRelaxations attribute data
    bool GetDebuggableData();
    void SetDebuggableData();

    // Public getter and setter for the DebuggerHidden attribute
    bool GetDebuggerHiddenData();
    void SetDebuggerHiddenData();

    // Public getter for and setter the DebuggerBrowsable attribute
    bool GetDebuggerBrowsableData(_Out_ DebuggerBrowsableStateEnum* pData);
    void SetDebuggerBrowsableData(DebuggerBrowsableStateEnum data);

    bool GetDebuggerDisplayData();
    void SetDebuggerDisplayData();

    // Public getter for MYGROUPCOLLECTIONATTRIBUTE
    struct MyGroupCollectionBase
    {
        STRING* m_GroupName;
        STRING* m_CreateMethod;
        STRING* m_DisposeMethod;
        STRING* m_DefaultInstance;

        MyGroupCollectionBase()
        {
            m_GroupName = NULL;
            m_CreateMethod = NULL;
            m_DisposeMethod = NULL;
            m_DefaultInstance = NULL;
        }
    };
    typedef DynamicArray<MyGroupCollectionBase> MyGroupCollectionData;
    bool GetMyGroupCollectionData(MyGroupCollectionData **ppMyGroupCollectionData);
    void SetMyGroupCollectionData(MyGroupCollectionData* pMyGroupCollectionData);
    bool HasMyGroupCollectionData() { return ( this != NULL && m_fMyGroupCollection ); };
    void ResetMyGroupCollectionData();

    // Public getter and setter for HideModuleName attribute data
    bool GetHideModuleNameData();
    void SetHideModuleNameData();

    // Public getter and setter for DesgignerGenerated attribute data
    bool GetDesignerGeneratedData();
    void SetDesignerGeneratedData();

    // Public getter and setter for StandardModule attribute data
    bool GetStandardModuleData();
    void SetStandardModuleData();

    // Public getter and setter for Extension attribute data
    bool GetExtensionData();
    void SetExtensionData();

    // Public getter and setter for OptionCompare attribute data
    bool GetOptionCompareData();
    void SetOptionCompareData();

    // Public getter and setter for EditorBrowsable data
    bool GetEditorBrowsableData(long *pEditorBrowsableState);
    void SetEditorBrowsableData(long editorBrowsableState);

    // Public getter and setter for DllImport data
    bool GetDllImportData();
    void SetDllImportData();

    // Public getter and setter for STAThread data
    bool GetSTAThreadData();
    void SetSTAThreadData();

    // Public getter and setter for MTAThread data
    bool GetMTAThreadData();
    void SetMTAThreadData();

    // Public getter and setter for InterfaceType data
    bool GetInterfaceTypeData(long *pInterfaceType);
    void SetInterfaceTypeData(long interfaceType);

    // Public getter for TypeLibType data
    bool GetTypeLibTypeData(short *pTypeLibType);
    void SetTypeLibTypeData(short typeLibType);

    // Public getter for TypeLibFunc data
    bool GetTypeLibFuncData(short *pTypeLibFunc);
    void SetTypeLibFuncData(short typeLibFunc);

    bool GetTypeLibVarData(short *pTypeLibVar);
    void SetTypeLibVarData(short typeLibVar);

    // Public getter and setter for ClassInterface
    bool GetClassInterfaceData(short *pClassInterfaceType);
    void SetClassInterfaceData(short classInterfaceType);

    // Public getter and setter for COMVisible
    bool GetCOMVisibleData(bool *pCOMVisible);
    void SetCOMVisibleData(bool visible);

    // Public getter and setter for ParamArray
    bool GetParamArrayData();
    void SetParamArrayData();

    // Public getter and setter for DecimalConstant
    bool GetDecimalConstantData(DECIMAL *Value);
    void SetDecimalConstantData(DECIMAL value);

    // Public getter for DecimalConstant
    bool GetDateTimeConstantData(__int64 *Value);
    void SetDateTimeConstantData(__int64 value);

    // Public getter and setter for IDispatchConstant
    bool GetIDispatchConstantData();
    void SetIDispatchConstantData();

    // Public getter and setter for IUnknownConstant
    bool GetIUnknownConstantData();
    void SetIUnknownConstantData();

    // Public getter for MarshalAs: 
    bool GetMarshalAsData(void)
    {
        return this ? m_fMarshalAs : false;
    }
    void SetMarshalAsData();
    bool GetMarshalAsData(PCCOR_SIGNATURE *ppSig, ULONG *pSigBytes);
    void SetMarshalAsData(PCCOR_SIGNATURE pSig, ULONG sigBytes);
 
    bool GetImportedFromTypeLibData(_Deref_out_opt_z_ WCHAR **ppTlbFile);
    void SetImportedFromTypeLibData(_In_z_ WCHAR* pTlbFile);

    // Public getter and setter for Guid
    bool GetGuidData(_Deref_out_opt_z_ WCHAR **ppwszGuid);
    void SetGuidData(_In_z_ WCHAR* pwszGuid);

    bool GetPrimaryInteropAssemblyData();
    void SetPrimaryInteropAssemblyData();

    bool GetTypeIdentifierData();
    void SetTypeIdentifierData();
    bool GetTypeIdentifierData(_Deref_out_opt_z_ WCHAR **pGuid,_Deref_out_opt_z_ WCHAR **pName);
    void SetTypeIdentifierData(_In_z_ WCHAR* pwszGuid, _In_z_ WCHAR* pwszName);
    void SetStructLayoutDataByLayoutKind(int pLayoutKind);
    void SetStructLayoutData(__int16 pLayoutKind);
    bool GetStructLayoutData(__int16 *pLayoutKind);
    void SetMethodImplOptionData(__int16 pMethodImplOptions);
    bool GetMethodImplOptionSynchronizedData();
    void SetCharSet(CorTypeAttr charSet);
    bool GetCharSet(CorTypeAttr &charSet);
    void SetPackClassSize(DWORD dwPackSize, ULONG ulClassSize);
    bool GetPackClassSize(DWORD & dwPackSize, ULONG & ulClassSize);

    // Public getter for ComClass
    struct ComClassData
    {
        const WCHAR* m_ClassId;
        const WCHAR* m_InterfaceId;
        const WCHAR* m_EventId;
        bool m_fShadows;
        BCSYM_Class *m_PartialClassOnWhichApplied;

        ComClassData()
        {
            m_ClassId = NULL;
            m_InterfaceId = NULL;
            m_EventId = NULL;
            m_fShadows = false;
            m_PartialClassOnWhichApplied = NULL;
        }
    };

    bool GetComClassData(ComClassData *pComClassData);
    void SetComClassData(ComClassData comClassData);

    struct ComEventInterfaceData
    {
        WCHAR *m_SourceInterface;
        WCHAR *m_EventProvider;

        ComEventInterfaceData()
        {
            m_SourceInterface = NULL;
            m_EventProvider = NULL;
        }
    };

    bool GetComEventInterfaceData(ComEventInterfaceData *pComEventInterfaceData);
    void SetComEventInterfaceData(ComEventInterfaceData comEventInterfaceData);

    // Public getter and setter for ComImport
    bool GetComImportData();
    void SetComImportData();

    // Public getter and setter for WindowsRuntimeImport
    bool GetWindowsRuntimeImportData();
    void SetWindowsRuntimeImportData();

    // Public getter and setter for ComSourceInterfaces
    bool GetComSourceInterfacesData();
    void SetComSourceInterfacesData();

    // Public getter and setter  for DispId
    bool GetDispIdData(long *pDispId);
    void SetDispIdData(long dispId);

    bool GetBestFitMappingData(bool *pMapping, bool *pThrow);
    void SetBestFitMappingData(bool fMapping, bool fThrow);

    bool GetFieldOffsetData(int *pOffset);
    void SetFieldOffsetData(int offset);

    bool GetLCIDConversionData(int *pData);
    void SetLCIDConversionData(int data);

    bool GetInData();
    void SetInData() 
    { 
        m_fIn = true; 
    }
    
    bool GetOutData();
    void SetOutData()
    {
        m_fOut = true;
    }

    bool GetUnmanagedFunctionPointerData(long *pData);
    void SetUnmanagedFunctionPointerData(long data);

    // Public getter and setter for CoClass
    bool GetCoClassData(_Deref_out_opt_z_ WCHAR **ppwszTypeName);
    void SetCoClassData(_In_z_ WCHAR* pwszTypeName);

    // Public getter and setter for Category
    bool GetCategoryData(_Deref_out_opt_z_ WCHAR **ppwszCategoryName);
    void SetCategoryData(_In_z_ WCHAR* pwszCategoryName);

    // Public getter and setter for DefaultEvent
    bool GetDefaultEventData(_Deref_out_z_ WCHAR **ppwszProperty, _Deref_out_opt_ BCSYM_Class** ppPartialClassOnWhichApplied);
    void SetDefaultEventData(_In_z_ WCHAR* pwszProperty, BCSYM_Class* pPartialClassOnWhichApplied);

    // Public getter and setter for DefaultValue
    bool GetDefaultValueData(ConstantValue* pDefaultValue, bool* pfIsNotHandled);
    void SetDefaultValueData(ConstantValue defaultValue, bool fIsNotHandled);

    // Public getter and setter for HelpKeyword
    bool GetHelpKeywordData(_Deref_out_opt_z_ WCHAR **ppwszHelpKeyword);
    void SetHelpKeywordData(_In_z_ WCHAR* pwszHelpKeyword);

    // Public getter and setter for NonSerialized
    bool GetNonSerializedData();
    void SetNonSerializedData();

    // Public getter for Serializable
    bool GetSerializableData();
    void SetSerializableData();

    // Public getter for WebMethod attribute data
    bool GetWebMethodData(AttrKind *pAttrId, bool *pfEnableSession, short *pTransactionOption, int *pCacheDuration, bool *pBufferResponse, _Deref_out_opt_z_ WCHAR **pwszDescription);
    void SetWebMethodData(AttrKind attrId, bool fEnableSession, short transactionOption, int cacheDuration, bool fBufferResponse, _In_z_ WCHAR* pwszDescription);

    // Public getter for DefaultParameterValue data
    bool GetDefaultParamValueData(BYTE*& argBlob, ULONG &cbArgBlob);
    void SetDefaultParamValueData(BYTE* argBlob, ULONG cbArgBlob);

    // Public getter and setter for WebService data
    bool GetWebServiceData(_Deref_out_z_ WCHAR** ppwszNamespace, _Deref_out_z_ WCHAR** ppwszDescription, _Deref_out_z_ WCHAR** ppwszName);
    void SetWebServiceData(_In_z_ WCHAR* pwszNamespace, _In_z_ WCHAR* pwszDescription, _In_z_ WCHAR* pwszName);

    bool GetEmbeddedData();
    void SetEmbeddedData()
    {
        m_fEmbedded = true;
    }

    //Public getter and setter for CallerLineNumber data
    bool GetCallerLineNumberData();
    void SetCallerLineNumberData();
    
    //Public getter and setter for CallerFilePath data
    bool GetCallerFilePathData();
    void SetCallerFilePathData();
    
    //Public getter and setter for CallerMemberName data
    bool GetCallerMemberNameData();
    void SetCallerMemberNameData();

    // Inherits well-known custom attributes settings.  Copies settings
    // out of psymBase and into 'this'.
    void InheritFromSymbol(BCSYM *psymBase);

    // Clears the result of the previous attribute ---- cached
    // in this AttrVals.
    void Clear----Results();

private:

    // The presence of well-known attribute FooAttribute is indicated
    // if m_fFoo is true.
#define DEF_ATTR_TYPE(attributeName) bool m_f ## attributeName : 1;
#include "AttributeTypes.h"
#undef DEF_ATTR_TYPE

#if _WIN64
    enum WellKnownAttrKindKeys : __int64
#else
    enum WellKnownAttrKindKeys
#endif
    {
#define DEF_ATTR_TYPE(attributeName) attributeName ## _Key,
#include "AttributeTypes.h"
#undef DEF_ATTR_TYPE
    };

    struct AttributeUsageData
    {
        CorAttributeTargets m_attrtargetsValidOn;
        bool                m_fAllowMultiple;
        bool                m_fInherited;
    };

    struct DecimalConstantData
    {
        DECIMAL     m_Value;
    };

    struct DateTimeConstantData
    {
        __int64     m_Value;
    };

    // IDispatchConstantData: No data stored -- presence of attribute is all we need to know

    // IUnknownConstantData: No data stored -- presence of attribute is all we need to know

    struct ObsoleteData
    {
        // 
        WCHAR      *m_pwszMessage;
        bool        m_fError;       // If false, it's just a warning
    };

    struct CLSCompliantData
    {
        bool m_fCompliant;
        bool m_fIsInherited;
    };

    struct PersistContentsData
    {
        bool m_fPersistContents;
    };

    struct DefaultMemberData
    {
        WCHAR *m_pwszDefaultMember;
    };

    struct DesignerCategoryData
    {
        WCHAR *m_pwszCategory;  // Optional -- can be NULL
    };

    struct DebuggerBrowsableData
    {
        long m_value;
    };

    struct AccessedThroughPropertyData
    {
        WCHAR *m_pwszProperty;
    };

    MyGroupCollectionData* m_daMyGroupAttrData;

    // HideModuleName: No data stored -- presence of attribute is all we need to know

    // StandardModuleData: No data stored -- presence of attribute is all we need to know

    // Flags: No data stored -- presence of attribute is all we need to know

    struct EditorBrowsableData
    {
        long m_EditorBrowsableState;
    };

    // DllImportData: No data stored -- presence of attribute is all we need to know

    // STAThreadData: No data stored -- presence of attribute is all we need to know

    // MTAThreadData: No data stored -- presence of attribute is all we need to know


    struct BestFitMappingData
    {
        bool m_BestFitMappingData;
        bool m_BestFitMappingThrowOnUnmappableChar;
    };
    
    struct COM2InteropData
    {
        // The InterfaceType
        long m_InterfaceType;

        // The typelib flags
        short m_TypeLibType;

        // The typelib func flags
        short m_TypeLibFunc;

        short m_TypeLibVar;

        // The class interface type (formerly HasDefaultInterface)
        short m_ClassInterfaceType;

        // COM2 Visibility
        bool m_fCOMVisible;

        // DispId
        long m_DispId;

        // CoClass
        WCHAR *m_pwszCoClassTypeName;
    };

    struct MarshalAsData
    {
        PCCOR_SIGNATURE pNativeType;
        ULONG cbNativeType;
    };
    
    // GuidData
    struct GuidData
    {
        WCHAR *m_pwszGuid;
    };

    struct TypeIdentifierData
    {
        WCHAR *m_pGuid;
        WCHAR *m_pName;
    };

    struct PackAndClassSizeData
    {
        DWORD m_dwPackSize;
        ULONG m_ulClassSize;
    };

    struct DefaultEventData
    {
        WCHAR *m_pwszDefaultEvent;
        BCSYM_Class *m_PartialClassOnWhichApplied;
    };

    struct DefaultValueData
    {
        ConstantValue m_DefaultValue;
        bool m_fIsNotHandled;
    };

    struct HelpKeywordData
    {
        WCHAR *m_pwszHelpKeyword;
    };

    // Flags : No data stored -- presence of attribute is all we need to know
    // NonSerialized : No data stored -- presence of attribute is all we need to know
    // Serializable : No data stored -- presence of attribute is all we need to know

    struct WebMethodData
    {
        AttrKind m_attrId;
        bool m_fEnableSession;
        short m_TransactionOption;
        int m_CacheDuration;
        bool m_fBufferResponse;
        WCHAR *m_pwszDescription;
    };

    // WebService :
    struct WebServiceData
    {
        AttrKind m_attrId;
        WCHAR *m_pwszName;
        WCHAR *m_pwszNamespace;
        WCHAR *m_pwszDescription;
    };

    struct DefaultParamValueObjectData
    {
        ULONG m_cbArgBlob;
        BYTE* m_argBlob;
    };

    NorlsAllocator* m_pAllocator;
    DynamicHashTable<WellKnownAttrKindKeys, void*, NorlsAllocWrapper> m_dataTable;
};

// Non----edData is where we hide some implementation details.
// All of the other data in AttrVals is just a cache for attribute
// ---- results.  Non----edData is where we store our other
// bookkeeping data.  We store these all together so Clear----Results
// doesn't need to be constantly updated.
struct Non----edData
{
    // During parsing, we need a place to hold a linked list of
    // attributes that have been parsed, but not examined yet.
    // That list starts here.
    BCSYM_ApplAttr *m_psymApplAttrHead;

    // During compilation, we need to know which symbols in a SourceFile
    // have attached AttrVals.  We root that list in SourceFile and thread
    // it through here.
    BCSYM *m_psymNextWithAttr;

    // We record the context of parameters here.  We need this so that we
    // can later re-parse the parameter and its attributes in the context
    // of the function to which the parameter belongs.
    BCSYM_NamedRoot *m_psymParamContext;

};

class AttrVals
{
public:
    //============================================================================
    // Implementation.
    //============================================================================


    // Returns head of the list of BCSYM_ApplAttr nodes
    BCSYM_ApplAttr *GetPsymApplAttrHead()
    {
        return m_Non----edData.m_psymApplAttrHead;
    }

    // Returns head of the list of BCSYM_ApplAttr nodes
    BCSYM *GetPsymNextSymbolWithApplAttr()
    {
        return m_Non----edData.m_psymNextWithAttr;
    }

    BCSYM_NamedRoot *GetPsymContextOfParamWithApplAttr()
    {
        return m_Non----edData.m_psymParamContext;
    }

    WellKnownAttrVals *GetPWellKnownAttrVals()
    {
        if (!this)
        {
            return NULL;
        }

        return m_pWellKnownAttrVals;
    }

    void SetPWellKnownAttrVals(WellKnownAttrVals *pWellKnownAttrVals)
    {
        m_pWellKnownAttrVals->ResetMyGroupCollectionData();
        m_pWellKnownAttrVals = pWellKnownAttrVals;
    }

    Non----edData* GetPNon----edData()
    {
        return &m_Non----edData;
    }

private:

    Non----edData m_Non----edData;

    WellKnownAttrVals *m_pWellKnownAttrVals;

};


//============================================================================
// This method is used to obtain the bound trees for the parameters of an
// attribute.
//============================================================================
ILTree::Expression *GetBoundTreesForAttribute(BCSYM_Hash      *Lookup,
                                  BCSYM_NamedRoot *pnamedContextOfApplAttr,
                                  BCSYM_ApplAttr  *AttributeSymbol,
                                  ErrorTable      *Errors,
                                  NorlsAllocator  *BoundTreeStorage,
                                  CompilerHost    *pCompilerHost);

// NamedProps is used to parse named properties used at a custom
// attribute application site.
struct NamedProps
{
    // Maximum number of named props recognized by any well-known custom attribute
    const static int m_cPropsMax = 3;

    int    m_cProps;                        // Number of named props
    const WCHAR *m_rgwszPropName[m_cPropsMax];    // List of prop names
    void  *m_rgpvPropVal[m_cPropsMax];      // Where to store decoded prop values
};


//============================================================================
// The Attribute class is used to handle many operations on custom attributes.
// Many of these methods used to belong to MetaImport and MetaEmit, but it
// makes more sense to gather all of that functionality together in one class.
//
// Clients should (obviously) call only the public entry points of this class.
// What's not so obvious is that these methods are static -- they take
// all the params they need to call the (private) Attribute constructor, and
// then perform the requested operation using that class instance.  The net
// result is that the caller never needs to declare an instance of the
// Attribute class.
//============================================================================
class Attribute
{
    // MetaEmit needs access to ----String and some other private stuff.
    friend class MetaEmit;

public:

    static void
    ReportError
    (
        ErrorTable * pErrorTable,
        ERRID errid,
        BCSYM * pSym
    );


    // Public, static entry-point.  This function constructs an instance
    // of the Attribute class and calls _----AllAttributesOnToken,
    // which does all of the real work.
    static void ----AllAttributesOnToken(mdToken mdtoken, IMetaDataImport *pimdImport,
                        NorlsAllocator *pnra, Compiler *pcompiler, CompilerProject *pCompilerProject, MetaDataFile *pMetaDataFile,
                        BCSYM *psym);

    // Public, static entry-point.  This ----s all of the attributes
    // applied to psym and accumulates the well-known
    // attribute bits into ppattrval, which is allocated on demand.
    static void ----AllAttributesOnSymbol(BCSYM *psym, NorlsAllocator *pnra,
                        Compiler *pcompiler, CompilerHost *pcompilerhost, ErrorTable *perrortable,
                        SourceFile *psourcefile);

    // Public, static entry-point.  Taked the BCSYM_ApplAttr (which is attached to
    // the given BCSYM) and returns a bound tree representing the applied attribute.
    static ILTree::Expression *GetBoundTreeFromApplAttr(BCSYM_ApplAttr *psymApplAttr, BCSYM *psym,
                        ErrorTable *perrortable, NorlsAllocator *pnra, CompilerHost *pcompilerHost);

    // Public, static entry-point.  This function constructs an instance
    // of the Attribute class and calls _EncodeBoundTree, which does the real work.
    static void EncodeBoundTree(ILTree::AttributeApplicationExpression *ptreeApplAttr, MetaEmit *pMetaemit, NorlsAllocator *pnra,
                        Compiler *pcompiler, CompilerProject *pcompilerproj,
                        bool fWellKnownAttr, _Deref_out_z_ STRING **ppstrAttrName,
                        BCSYM_Class **ppsymAttrClass, BCSYM_Proc **ppsymAttrCtor,
                        BYTE **ppbArgBlob,  ULONG *pcbArgBlob, ULONG *pcPosParams,
                        COR_SIGNATURE **ppSignature, ULONG *pcbSignature,
                        bool *pfError, ErrorTable * pErrorTable = NULL);


    // Parses the serialized data (arg blob) in pbArgBlob accoring to the
    // attribute's signature (pbSignature) and possible named properties (pnamedprops).
    static void ----AttributeData(NamedProps *pnamedprops, NorlsAllocator  *pnra,
                               PCCOR_SIGNATURE psignatureOrig, BYTE *pbArgBlob, ...);


    // Check for incorrect uses of attributes (e.g., using
    // both STAThread and MTAThread on the same method)
    static void VerifyAttributeUsage(BCSYM *psymCheck, ErrorTable *pErrorTable, CompilerHost * pCompilerHost);
    static void VerifyComClassAttributeUsage(BCSYM_Class *ClassSym, ErrorTable *pErrorTable);
    static void VerifyDefaultEventAttributeUsage(BCSYM_Class *ClassSym, ErrorTable *pErrorTable);
    static void VerifyExtensionAttributeUsage(BCSYM_Class *ClassSym, ErrorTable *pErrorTable);

    //Verifies the usage of the extension attribute on procedure symbols.
    //If the provided pErrorTable is non null, then errors will be reported for
    //validation failures.
    //If the validation succeeds, then the "callable as extension method" flag
    //will be set on the the procedure symbol, and the "contains extension method flag"
    //will be set on the procedure's containing type.
    //This method is suitable for use checkings procedures defined both in source and
    //in meta-data, and properly understands the different validation rules needed for both.
    //Also, the method may be called on any procedure, not just those decorated with the extension
    //attribute, and will properly skip all processing in the even that the
    //attribute is missing.
    static bool VerifyExtensionAttributeUsage(BCSYM_Proc *ProcSym, ErrorTable *pErrorTable, CompilerHost * pCompilerHost);
    static void VerifyWebMethodAttributeUsage(BCSYM_Proc *ProcSym, ErrorTable *pErrorTable);
    static void VerifyNonSerializedAttributeUsage(BCSYM_NamedRoot *ProcField, ErrorTable *pErrorTable);
    static BCSYM_ApplAttr * GetAppliedAttribute(Symbol * pSymbolToSearch, Type * pAttributeType, bool SearchThisSymbolOnly = true);

    // Check Guid attribute format
    static bool VerifyGuidAttribute(const WCHAR* guid, bool AllowBlank);

    static BCSYM_NamedRoot *BindDefaultEvent(BCSYM_Class *ClassSym, _In_z_ STRING *strName);

    // Static function that converts the given bound tree into the
    // appropriate bits in an AttrVals struct.  If the bound tree represents
    // something other than a well-known custom attribute, this function is a nop.
    // Returns an AttrIndex for the attribute in g_rgAttributes array or NULL if it is not a well-known attribute.
    static const AttrIndex * ----BoundTree(ILTree::AttributeApplicationExpression *ptreeApplAttr, NorlsAllocator *pnra,
                                  Compiler *pcompiler, SourceFile *psourcefile,
                                  bool *pfError, BCSYM *psym,
                                  bool isAssembly);

    // See if the member represented by Token is annotated with the specified attribute
    static TriState<bool> HasAttribute(MetaDataFile *pMetaDataFile, mdToken Token, AttrKind SpecifiedAttribute);

private:
    static bool
    ValidateExtensionAttributeMetaDataConditions
    (
        Procedure * pProc,
        Container * pContainer
    );

    static bool
    ValidateGenericConstraitnsOnExtensionMethodDefinition
    (
        Procedure * pProc,
        ErrorTable * pErrorTable
    );
    static bool
    ValidateTypeDoesNotDependOnPartialyOpenParameter
    (
        GenericParameter * pParameter,
        ISet<GenericParameter *> * pCloseableParameters,
        ISet<Type *> * pProcessedTypes,
        Procedure * pProc,
        ErrorTable * pErrorTable
    );

    static void
    PushTypeIfNotAlreadySeen
    (
        Type * pType,
        IStackOrQueue<Type *> * pStack,
        ISet<Type *> * pProcessedTypes
    );


    //============================================================================
    // Constructor.
    //============================================================================

    // Ctor is private -- it can only be called from public, static methods
    Attribute
    (
        IMetaDataImport *pimdImport,
        MetaEmit         *pMetaemit,
        NorlsAllocator   *pnra,
        Compiler         *pcompiler,
        CompilerProject  *pcompilerproj,
        MetaDataFile     *pMetaDataFile,
        ErrorTable * pErrorTable = NULL
    )
        : m_pimdImport(pimdImport)  // Cached copy -- no AddRef
        , m_pMetaemit(pMetaemit)
        , m_pnra(pnra)
        , m_pcompiler(pcompiler)
        , m_pcompilerhost(pcompilerproj->GetCompilerHost())
        , m_pcompilerproj(pcompilerproj)
        , m_pMetaDataFile(pMetaDataFile)
        , m_pstrAttrName(NULL)
        , m_psymAttrCtor(NULL)
        , m_psymAttrClass(NULL)
        , m_pbArgBlob(NULL)
        , m_cbArgBlob(0)
        , m_cPosParams(0)
        , m_cNamedProps(0)
        , m_fError(false)
        , m_pErrorTable(pErrorTable)
    {
    }

    //============================================================================
    // Simple cracking (decoding) functions
    //============================================================================

    // Decodes a COM+ boolean as serialized in a custom attribute arg blob.
    static bool ----Bool(BYTE **ppbArgBlob);

    // Decodes a COM+ I1 as serialized in a custom attribute arg blob.
    static __int8 ----I1(BYTE **ppbArgBlob);

    // Decodes a COM+ I2 as serialized in a custom attribute arg blob.
    static __int16 ----I2(BYTE **ppbArgBlob);

    // Decodes a COM+ I4 as serialized in a custom attribute arg blob.
    static __int32 ----I4(BYTE **ppbArgBlob);

    // Decodes a COM+ I8 as serialized in a custom attribute arg blob.
    static __int64 ----I8(BYTE **ppbArgBlob);

    // Decodes a COM+ R4 as serialized in a custom attribute arg blob.
    static float ----R4(BYTE **ppbArgBlob);

    // Decodes a COM+ R8 as serialized in a custom attribute arg blob.
    static double ----R8(BYTE **ppbArgBlob);

    // Decodes a COM+ String as serialized in a custom attribute arg blob.
    static WCHAR *----String(BYTE **ppbArgBlob, NorlsAllocator *pnra);

    // Returns a string that represents the given serialized System.Type
    // expressed as a fully-qualified type name.
    WCHAR *ConvertSerializedSystemTypeToTypeName(_In_opt_z_ WCHAR *pwszSystemType, NorlsAllocator *pnra);

    //============================================================================
    // Top-level cracking functions
    //============================================================================

    // ----s all well-known custom attributes attached to mdtoken and
    // returns their settings through psym.  Clients should call the static,
    // public, non-underscore version of this function.
    void _----AllAttributesOnToken(mdToken mdtoken, BCSYM *psym);

    // Returns the name and signature of the custom attribute named by tokenAttr.
    //
    // Can return S_FALSE if everything went okay but tokenAttr just wasn't
    // an attribute, or if the signature was bogus.  OUT params are not
    // set in this case.
    bool GetNameAndSigFromToken(mdToken tokenAttr,
                                _Out_cap_(cchAttrName) WCHAR *wszAttrName,
                                ULONG cchAttrName,
                                PCCOR_SIGNATURE *ppsignature,
                                ULONG *pcbSignature);

    // Processes one custom attribute (and its constructor's args and
    // named properties) and returns the result (if any) through psym.
    // Note that ppattrvals is allocated if one is needed and none is passed in.
    AttrIdentity *----OneAttribute(mdToken tokenAttr, BYTE *pbArgBlob, ULONG cbArgBlob, BCSYM *psym, bool ----ArgBlob = true);

    // Processes the given arg blob and returns the result (if any)
    // through psym.
    void ----ArgBlob(
            _In_count_(cbArgBlob) BYTE *pbArgBlob,
            ULONG cbArgBlob,
            const AttrIdentity *pAttribute,
            _Inout_ BCSYM *psym, bool isAssembly,
            _In_opt_ SourceFile *pSourceFile,
            _In_opt_ Location* pLoc = NULL);

    // Processes the named properties/fields found in the arg blob.
    static void ----NamedProperties(int cNamedProps, NamedProps *pnamedprops, NorlsAllocator *pnra, BYTE **ppbArgBlob);

    // worker function for HasAttribute
    bool _HasAttribute(mdToken Token, AttrKind SpecifiedAttribute);

    //============================================================================
    // Emitting functions
    //============================================================================

    // Encodes the given bound tree (which represents an applied attribute).
    // Non-static version of EncodeBoundTree (no underscore).
    void _EncodeBoundTree(ILTree::AttributeApplicationExpression *ptreeApplAttr, bool fWellKnownAttr);

    // Returns the fully-qualified name, BCSYM_Class and ctor of the given applied attribute.
    void GetEmittedNameAndSymbol(ILTree::AttributeApplicationExpression *ptreeApplAttr, _Deref_out_z_ STRING **ppstrAttrName,
                             BCSYM_Proc **ppsymCtor, BCSYM_Class **ppsymClass);

    // Encodes the positional parameters passed to the attribute specified
    // by the given bound tree.  This encoding is appended to the CTOR arg blob.
    void EncodeAllPositionalParams(ILTree::AttributeApplicationExpression *ptreeApplAttr);

    // Encodes the given length-counted Unicode string and appends to the arg blob.
    void EncodeString(_In_opt_count_(cch) const WCHAR *pwsz, ULONG cch);

    // Encodes a tree that represents a constant value, and appends it to the arg blob.
    void EncodeConstant(ILTree::ILNode *ptree);

    // Encodes the COR type into the signature.
    void EncodeParameterCORType(COR_SIGNATURE ParameterType);

    // Encodes the parameter type into the signature
    void EncodeSignatureParameter(BCSYM *pParameterType);

    // Encodes a tree that represents a constant type, as in "GetType(TypeName)".
    // The symbol passed in represents TypeName, and can be an array, class, or interface.
    STRING *EncodeConstantType(BCSYM *psymType, Location & referencingLocation);

    // Extends the arg blob and appends cbData bytes of pvData.
    void ExtendArgBlob(void *pvData, ULONG cbData);

    // Encodes the given type and appends it to the arg blob.  This
    // encoding is specific to attribute named properties.
    void EncodeNamedPropType(BCSYM *psymType, Location & referencingLocation);

    // Encodes one property/field assignment.
    void EncodeOneNamedProp(ILTree::SymbolReferenceExpression *ptreeSym, ILTree::ILNode *ptreeVal);

    // Encodes an applied attribute's named properties (and fields) and appends
    // the encoding to the arg blob.
    void EncodeAllNamedProps(ILTree::AttributeApplicationExpression *ptreeApplAttr);

    //============================================================================
    // Data members
    //============================================================================

    // This data member is used to emit AssemblyRefs when EncodeConstantType of MetaType literal
    MetaEmit         *m_pMetaemit;      // Use to encode AssemblyRefs (NULL if not encoding)
    ErrorTable * m_pErrorTable;  // Use to report errors for type encoding

    // This data members are used when importing an attribute:
    IMetaDataImport *m_pimdImport;      // Where we're importing from (NULL if encoding)

    // These data members are used both when importing and encoding:
    NorlsAllocator  *m_pnra;            // Where to alloc from when necessary
    Compiler        *m_pcompiler;       // The compiler that's running us
    CompilerHost    *m_pcompilerhost;   // The compilerhost
    CompilerProject *m_pcompilerproj;   // Project being built
    MetaDataFile    *m_pMetaDataFile;   // The metadata file containing the symbol we're cracking

    // These data members are used when encoding an attribute:
    STRING          *m_pstrAttrName;    // Name of attribute
    BCSYM_Proc      *m_psymAttrCtor;    // Attribute constructor being used
    BCSYM_Class     *m_psymAttrClass;   // Class symbol for attribute
    BYTE            *m_pbArgBlob;       // The custom attribute's CTOR arg blob
    ULONG            m_cbArgBlob;       // Blob's size in bytes
    ULONG            m_cPosParams;      // Count of positional params in this arg blob
    USHORT           m_cNamedProps;     // Count of named props in this arg blob (USHORT to match COM+)
    bool             m_fError;          // Did a recoverable error occur?

    //Microsoft For performance reasons, do not allocate a buffer
    //for this array with every Attribute instance. It is often not needed.
    //Allocate it only when preparing to perform operations
    //that will need it.
    DynamicArray< COR_SIGNATURE > m_Signatures;
};

// Helps encode constant types when emitting applied attributes
// Helps decode constant types when importing applied attributes
class CLRTypeConstantEncoderDecoder
{
public:

    CLRTypeConstantEncoderDecoder(Compiler *CompilerInstance)
    : m_Compiler(CompilerInstance)
    , m_CompilerHost(NULL)
    , m_CurrentProjectContext(NULL)
    , m_Metaemit(NULL)
    , m_TypeNameBuilder(nullptr)
    , m_UseStringVersion(false)
    , m_pErrorTable(NULL)
    {
        m_Location.Invalidate();
    }

    ~CLRTypeConstantEncoderDecoder()
    {
        if (m_TypeNameBuilder != nullptr)
        {
            delete m_TypeNameBuilder;
            m_TypeNameBuilder = nullptr;
        }
    }

    // Encode the VB type as an equivalent clr type name
    STRING *TypeToCLRName(BCSYM *Type, CompilerProject *ProjectContext, MetaEmit *MetaEmit, bool IncludeAssemblySpec = true, ErrorTable * pErrorTable = NULL, Location * pLocation = NULL);

    // Decode the clr type name to the equivalent VB typename.
    // 
    WCHAR *CLRNameToVBName(_In_opt_z_ WCHAR *CLRTypeName, NorlsAllocator *Allocator, CompilerProject *ProjectContext, MetaEmit *MetaEmit);


private:

    // Encoding routines

    void EncodeType(BCSYM *Type, bool IncludeAssemblySpec = true);
    void EncodeTypeName(BCSYM_Container *Container);
    void EncodeAssemblyName(BCSYM_Container *Container);
    void EncodeGenericTypeArguments(BCSYM *Type);
    void EncodeTypeArgumentsInOrder(BCSYM_GenericTypeBinding *Binding);
    void EncodeArray(BCSYM *Type);

    void InitCurrentContext(CompilerProject *ProjectContext, MetaEmit *MetaEmit, ErrorTable * pErrorTable = NULL, Location * pLocation = NULL)
    {
        m_CurrentProjectContext = ProjectContext;
        m_Metaemit = MetaEmit;
        m_pErrorTable = pErrorTable;

        if (pLocation != NULL)
        {
            m_Location = *pLocation;
        }
        else
        {
            m_Location.Invalidate();
        }
    }

    void ClearCurrentContext()
    {
        m_CurrentProjectContext = NULL;
        m_Metaemit = NULL;
        m_pErrorTable = NULL;
        m_Location.Invalidate();
    }

    void InitTypeNameBuilder();

    Compiler *m_Compiler;
    CompilerHost *m_CompilerHost;

    CompilerProject *m_CurrentProjectContext;
    MetaEmit *m_Metaemit;
    ErrorTable * m_pErrorTable;  // Use to report errors for type encoding
    Location m_Location;  // Use to report errors for type encoding

    TypeNameBuilder *m_TypeNameBuilder;

    StringBuffer m_sbTypeNameBuilder;
    bool m_UseStringVersion;
};
