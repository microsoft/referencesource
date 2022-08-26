//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Contains table of runtime language helpers.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#if DEBUG
  #define RTNM(x)                      (x),
  #define TLIB(x)                      (x),
  #define RTNV(x)                      (x),
#else
  #define RTNM(x)
  #define TLIB(x)
  #define RTNV(x)
#endif

#define ARG0()                               {t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF}
#define ARG1(t1)                             {t1,      t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF}
#define ARG2(t1, t2)                         {t1,      t2,      t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF}
#define ARG3(t1, t2, t3)                     {t1,      t2,      t3,      t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF}
#define ARG4(t1, t2, t3, t4)                 {t1,      t2,      t3,      t4,      t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF}
#define ARG5(t1, t2, t3, t4, t5)             {t1,      t2,      t3,      t4,      t5,      t_UNDEF, t_UNDEF, t_UNDEF, t_UNDEF}
#define ARG6(t1, t2, t3, t4, t5, t6)         {t1,      t2,      t3,      t4,      t5,      t6,      t_UNDEF, t_UNDEF, t_UNDEF}
#define ARG7(t1, t2, t3, t4, t5, t6, t7)     {t1,      t2,      t3,      t4,      t5,      t6,      t7,      t_UNDEF, t_UNDEF}
#define ARG8(t1, t2, t3, t4, t5, t6, t7, t8) {t1,      t2,      t3,      t4,      t5,      t6,      t7,      t8,      t_UNDEF}

RuntimeLibraryDescriptor g_rgRTLangLibraries[] =
{
    { RTNM(COMLibrary)       L"mscorlib" },
    { RTNM(VBLibrary)        L"Microsoft.VisualBasic" },
};

RuntimeClassDescriptor   g_rgRTLangClasses[] = // Maps the language helper class with the library it is found in and fully-qualified name of that class
{
    { RTNM(UndefinedRuntimeClass)          VBLibrary, NULL},

    { RTNM(VBStringClass)                  VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".StringType"},
    { RTNM(VBCharClass)                    VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".CharType"},
    { RTNM(VBDateClass)                    VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".DateType"},
    { RTNM(VBDecimalClass)                 VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".DecimalType"},
    { RTNM(VBBooleanClass)                 VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".BooleanType"},
    { RTNM(VBByteClass)                    VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".ByteType"},
    { RTNM(VBCharArrayClass)               VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".CharArrayType"},
    { RTNM(VBShortClass)                   VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".ShortType"},
    { RTNM(VBIntegerClass)                 VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".IntegerType"},
    { RTNM(VBLongClass)                    VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".LongType"},
    { RTNM(VBSingleClass)                  VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".SingleType"},
    { RTNM(VBDoubleClass)                  VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".DoubleType"},
    { RTNM(VBObjectClass)                  VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".ObjectType"},
    { RTNM(VBLateBindingV1Class)           VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".LateBinding"},
    { RTNM(VBLateBindingClass)             VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".NewLateBinding"},
    { RTNM(VBObjectFlowControlClass)       VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".ObjectFlowControl"},
    { RTNM(VBOperatorsClass)               VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".Operators"},
    { RTNM(VBEmbeddedOperatorsClass)       VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".EmbeddedOperators"},
    { RTNM(VBLikeOperatorClass)            VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".LikeOperator"},
    { RTNM(VBProjectDataClass)             VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".ProjectData"},
    { RTNM(VBUtilsClass)                   VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".Utils"},
    { RTNM(VBStandardModuleAttributeClass) VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".StandardModuleAttribute"},
    { RTNM(VBOptionTextAttributeClass)     VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".OptionTextAttribute"},
    { RTNM(VBConversionsClass)             VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".Conversions"},
    { RTNM(VBForLoopControlClass)          VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".ObjectFlowControl.ForLoopControl"},
    { RTNM(VBLateBinderForLoopControlClass) VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".LateBinderObjectFlowControl.ForLoopControl"},
    { RTNM(VBVersionedClass)               VBLibrary, VBCOMPILERSERVICES_NAMESPACE L".Versioned"},
    { RTNM(VBInformationClass)             VBLibrary, VB_NAMESPACE L"Information"},
    { RTNM(VBInteractionClass)             VBLibrary, VB_NAMESPACE L"Interaction"},
    { RTNM(VBFileSystem)                   VBLibrary, VB_NAMESPACE L"FileSystem"},
    { RTNM(VBApplicationBase)              VBLibrary, VB_NAMESPACE L"ApplicationServices.ApplicationBase"},
    { RTNM(VBWindowsFormsApplicationBase)  VBLibrary, VB_NAMESPACE L"ApplicationServices.WindowsFormsApplicationBase"},
    { RTNM(VBErrObject)                    VBLibrary, VB_NAMESPACE L"ErrObject"},
    { RTNM(COMTypeClass)                             COMLibrary, L"System.Type"},
    { RTNM(COMStringClass)                           COMLibrary, L"System.String"},
    { RTNM(COMMathClass)                             COMLibrary, L"System.Math"},
    { RTNM(COMDecimalClass)                          COMLibrary, L"System.Decimal"},
    { RTNM(COMDateTimeClass)                         COMLibrary, L"System.DateTime"},
    { RTNM(COMConvertClass)                          COMLibrary, L"System.Convert"},
    { RTNM(COMDebuggerClass)                         COMLibrary, L"System.Diagnostics.Debugger"},
    { RTNM(COMDebuggerNonUserCodeAttributeClass)     COMLibrary, L"System.Diagnostics.DebuggerNonUserCodeAttribute"},
    { RTNM(COMDebuggerStepThroughAttributeClass)     COMLibrary, L"System.Diagnostics.DebuggerStepThroughAttribute"},
    { RTNM(COMDebuggerDisplayAttributeClass)         COMLibrary, L"System.Diagnostics.DebuggerDisplayAttribute"},
    { RTNM(COMDebuggerBrowsableAttributeClass)       COMLibrary, L"System.Diagnostics.DebuggerBrowsableAttribute"},
    { RTNM(COMAssemblyClass)                         COMLibrary, L"System.Reflection.Assembly"},
    { RTNM(COMAssemblyFlagsAttributeClass)           COMLibrary, L"System.Reflection.AssemblyFlagsAttribute"},

    { RTNM(COMObjectClass)                           COMLibrary, L"System.Object"},
    { RTNM(COMDefaultMemberAttributeClass)           COMLibrary, INTEROP_DEFAULTMEMBER_TYPE_W},
    { RTNM(COMParamArrayAttributeClass)              COMLibrary, INTEROP_PARAMARRAY_TYPE_W},
    { RTNM(COMSTAThreadAttributeClass)               COMLibrary, DEFAULTDOMAIN_STA_TYPE_W},
    { RTNM(COMDecimalConstantAttributeClass)         COMLibrary, DECIMALCONSTANTATTRIBUTE},
    { RTNM(COMDateTimeConstantAttributeClass)        COMLibrary, DATETIMECONSTANTATTRIBUTE},
    { RTNM(COMDebuggableAttributeClass)              COMLibrary, L"System.Diagnostics.DebuggableAttribute"},
    { RTNM(COMRuntimeHelpersClass)                   COMLibrary, L"System.Runtime.CompilerServices.RuntimeHelpers"},
    { RTNM(COMArrayClass)                            COMLibrary, L"System.Array"},
    { RTNM(COMAccessedThroughPropertyAttributeClass) COMLibrary, L"System.Runtime.CompilerServices.AccessedThroughPropertyAttribute"},
    { RTNM(COMGuidAttributeClass)                    COMLibrary, L"System.Runtime.InteropServices.GuidAttribute"},
    { RTNM(COMClassInterfaceAttributeClass)          COMLibrary, L"System.Runtime.InteropServices.ClassInterfaceAttribute"},
    { RTNM(COMComSourceInterfacesAttributeClass)     COMLibrary, L"System.Runtime.InteropServices.ComSourceInterfacesAttribute"},
    { RTNM(COMInterfaceTypeAttributeClass)           COMLibrary, L"System.Runtime.InteropServices.InterfaceTypeAttribute"},
    { RTNM(COMDispIdAttributeClass)                  COMLibrary, L"System.Runtime.InteropServices.DispIdAttribute"},
    { RTNM(COMComVisibleAttributeClass)              COMLibrary, INTEROP_COMVISIBLE_TYPE_W},
    { RTNM(COMTypeIdentifierAttributeClass)          COMLibrary, L"System.Runtime.InteropServices.TypeIdentifierAttribute"},
    { RTNM(COMMarshalAsAttributeClass)               COMLibrary, L"System.Runtime.InteropServices.MarshalAsAttribute"},
    { RTNM(COMStructLayoutAttributeClass)            COMLibrary, L"System.Runtime.InteropServices.StructLayoutAttribute"},
    { RTNM(COMBestFitMappingAttributeClass)          COMLibrary, L"System.Runtime.InteropServices.BestFitMappingAttribute"},
    { RTNM(COMFieldOffsetAttributeClass)             COMLibrary, L"System.Runtime.InteropServices.FieldOffsetAttribute"},
    { RTNM(COMLCIDConversionAttributeClass)          COMLibrary, L"System.Runtime.InteropServices.LCIDConversionAttribute"},
    { RTNM(COMInAttributeClass)                      COMLibrary, L"System.Runtime.InteropServices.InAttribute"},
    { RTNM(COMOutAttributeClass)                     COMLibrary, L"System.Runtime.InteropServices.OutAttribute"},
    { RTNM(COMTypeLibTypeAttributeClass)             COMLibrary, L"System.Runtime.InteropServices.TypeLibTypeAttribute"},
    { RTNM(COMTypeLibFuncAttributeClass)             COMLibrary, L"System.Runtime.InteropServices.TypeLibFuncAttribute"},
    { RTNM(COMTypeLibVarAttributeClass)              COMLibrary, L"System.Runtime.InteropServices.TypeLibVarAttribute"},
    { RTNM(COMUnmanagedFunctionPointerAttributeClass) COMLibrary, L"System.Runtime.InteropServices.UnmanagedFunctionPointerAttribute"},
    { RTNM(COMActivatorClass)                        COMLibrary, L"System.Activator"},
    { RTNM(COMCompilationRelaxationsAttributeClass)  COMLibrary, L"System.Runtime.CompilerServices.CompilationRelaxationsAttribute"},
    { RTNM(COMCompilerGeneratedAttributeClass)       COMLibrary, L"System.Runtime.CompilerServices.CompilerGeneratedAttribute"},
    { RTNM(COMRuntimeCompatibilityAttributeClass)    COMLibrary, L"System.Runtime.CompilerServices.RuntimeCompatibilityAttribute"},
    { RTNM(COMFieldInfoClass)                        COMLibrary, L"System.Reflection.FieldInfo"},
    { RTNM(COMMethodBaseClass)                       COMLibrary, L"System.Reflection.MethodBase"},
    { RTNM(COMMethodInfoClass)                       COMLibrary, L"System.Reflection.MethodInfo"},
    { RTNM(COMDelegateClass)                         COMLibrary, L"System.Delegate"},
    { RTNM(COMMonitorClass)                          COMLibrary, L"System.Threading.Monitor"},
    { RTNM(COMFlagsAttributeClass)                   COMLibrary, L"System.FlagsAttribute"},
    { RTNM(COMEnvironmentClass)                      COMLibrary, L"System.Environment"},
    { RTNM(COMWinRTDesignerContextClass)             COMLibrary, L"System.Runtime.DesignerServices.WindowsRuntimeDesignerContext"},
    { RTNM(COMAppDomainClass)                        COMLibrary, L"System.AppDomain"},
};

RuntimeMemberDescriptor  g_rgRTLangMembers[] =
{
    {
        RTNM(UndefinedRuntimeMember    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        0,
        /* parent class  */       UndefinedRuntimeClass,
        /* method name   */       NULL,
        /* ret type      */       t_UNDEF,
        /* arg types     */       ARG0()
    },

    //
    // Microsoft.VisualBasic.Globals.StandardModuleAttribute
    //

    {
        RTNM(StandardModuleAttributeCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       VBStandardModuleAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG0()
    },

    //
    // Microsoft.VisualBasic.Globals.OptionTextAttribute
    //

    {
        RTNM(OptionTextAttributeCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       VBOptionTextAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG0()
    },

    //
    // Microsoft.VisualBasic.CompilerServices.BooleanType
    //

    {
        RTNM(ObjectToBooleanV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
         RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBBooleanClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToBooleanV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBBooleanClass,
        /* method name   */       WIDE("FromString"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1(t_string)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.ByteType
    //

    {
        RTNM(StringToByteV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBByteClass,
        /* method name   */       WIDE("FromString"),
        /* ret type      */       t_ui1,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToByteV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBByteClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       t_ui1,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.CharArrayType
    //

    {
        RTNM(StringToCharArrayV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBCharArrayClass,
        /* method name   */       WIDE("FromString"),
        /* ret type      */       (Vtypes)(m_array | t_char),
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToCharArrayV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBCharArrayClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       (Vtypes)(m_array | t_char),
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.CharType
    //

    {
        RTNM(StringToCharV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBCharClass,
        /* method name   */       WIDE("FromString"),
        /* ret type      */       t_char,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToCharV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBCharClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       t_char,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.DateType
    //

    {
        RTNM(StringToDateV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBDateClass,
        /* method name   */       WIDE("FromString"),
        /* ret type      */       t_date,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToDateV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBDateClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       t_date,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.DecimalType
    //

    {
        RTNM(BooleanToDecimalV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBDecimalClass,
        /* method name   */       WIDE("FromBoolean"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1(t_bool)
    },

    {
        RTNM(StringToDecimalV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBDecimalClass,
        /* method name   */       WIDE("FromString"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToDecimalV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBDecimalClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.DoubleType
    //

    {
        RTNM(StringToDoubleV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBDoubleClass,
        /* method name   */       WIDE("FromString"),
        /* ret type      */       t_double,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToDoubleV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBDoubleClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       t_double,
        /* arg types     */       ARG1((Vtypes) c_object)
    },


    //
    // Microsoft.VisualBasic.CompilerServices.IntegerType
    //

    {
        RTNM(StringToIntegerV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBIntegerClass,
        /* method name   */       WIDE("FromString"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToIntegerV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBIntegerClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.LateBinding
    //

    {
        RTNM(LateGetV1Member)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingV1Class,
        /* method name   */       WIDE("LateGet"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG6((Vtypes)c_object, (Vtypes)c_class, t_string, (Vtypes)(m_array | c_object), (Vtypes)(m_array | t_string), (Vtypes)(m_array | t_bool))
    },

    {
        RTNM(LateSetV1Member)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingV1Class,
        /* method name   */       WIDE("LateSet"),
        /* ret type      */       t_void,
        /* arg types     */       ARG5((Vtypes)c_object, (Vtypes)c_class, t_string, (Vtypes)(m_array | c_object), (Vtypes)(m_array | t_string))
    },

    {
        RTNM(LateSetComplexV1Member)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingV1Class,
        /* method name   */       WIDE("LateSetComplex"),
        /* ret type      */       t_void,
        /* arg types     */       ARG7((Vtypes)c_object, (Vtypes)c_class, t_string, (Vtypes)(m_array | c_object), (Vtypes)(m_array | t_string), t_bool, t_bool)
    },

    {
        RTNM(LateCallV1Member)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingV1Class,
        /* method name   */       WIDE("LateCall"),
        /* ret type      */       t_void,
        /* arg types     */       ARG6((Vtypes)c_object, (Vtypes)c_class, t_string, (Vtypes)(m_array | c_object), (Vtypes)(m_array | t_string), (Vtypes)(m_array | t_bool))
    },

    //
    // Microsoft.VisualBasic.CompilerServices.LongType
    //

    {
        RTNM(StringToLongV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLongClass,
        /* method name   */       WIDE("FromString"),
        /* ret type      */       t_i8,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToLongV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLongClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       t_i8,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.ObjectType
    //

    {
        RTNM(ObjectCompareV1Member    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("ObjTst"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG3( (Vtypes)c_object,  (Vtypes)c_object, t_bool)
    },

    {
        RTNM(ObjectPlusV1Member    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("PlusObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(ObjectNegateV1Member    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("NegObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(ObjectAddV1Member    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("AddObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ObjectSubtractV1Member    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("SubObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ObjectMultiplicationV1Member    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("MulObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ObjectDivisionV1Member    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("DivObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ObjectPowerV1Member    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("PowObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ObjectModuloV1Member    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("ModObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ObjectIntegerDivisionV1Member   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("IDivObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ObjectShiftLeftV1Member   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("ShiftLeftObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, t_i4)
    },

    {
        RTNM(ObjectShiftRightV1Member   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("ShiftRightObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, t_i4)
    },

    {
        RTNM(ObjectNotV1Member    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("NotObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(ObjectBitAndV1Member )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("BitAndObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ObjectBitOrV1Member  )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("BitOrObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ObjectBitXorV1Member )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("BitXorObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ObjectConcatenationV1Member )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("StrCatObj"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ObjectLikeV1Member   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectClass,
        /* method name   */       WIDE("LikeObj"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, (Vtypes)c_enum)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.ShortType
    //

    {
        RTNM(StringToShortV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBShortClass,
        /* method name   */       WIDE("FromString"),
        /* ret type      */       t_i2,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToShortV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBShortClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       t_i2,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.SingleType
    //

    {
        RTNM(StringToSingleV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBSingleClass,
        /* method name   */       WIDE("FromString"),
        /* ret type      */       t_single,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToSingleV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBSingleClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       t_single,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.StringType
    //

    {
        RTNM(BooleanToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromBoolean"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_bool)
    },

    {
        RTNM(ByteToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromByte"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_ui1)
    },

    {
        RTNM(IntegerToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromInteger"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_i4)
    },

    {
        RTNM(LongToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromLong"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_i8)
    },

    {
        RTNM(SingleToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromSingle"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(DoubleToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromDouble"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(DateToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromDate"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_date)
    },

    {
        RTNM(DecimalToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromDecimal"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(CharToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromChar"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_char)
    },

    // This is a wrapper in the VB runtime because the debug api's can't call
    // CharArrayToStringV1Member directly for some reason.
    {
        RTNM(ArrayOfCharToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromCharArray"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1((Vtypes)(m_array | t_char))
    },

    // This is a wrapper in the VB runtime because the debug api's can't call
    // CharArrayToStringV1Member directly for some reason.
    {
        RTNM(CharAndCountToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromCharAndCount"),
        /* ret type      */       t_string,
        /* arg types     */       ARG2(t_char, t_i4)
    },

    // This is a wrapper in the VB runtime because the debug api's can't call
    // CharArrayToStringV1Member directly for some reason.
    {
        RTNM(ArrayOfCharToStringV1MemberSubset)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromCharArraySubset"),
        /* ret type      */       t_string,
        /* arg types     */       ARG3((Vtypes)(m_array | t_char), t_i4, t_i4)
    },

    {
        RTNM(ObjectToStringV1Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("FromObject"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringCompareV1Member )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("StrCmp"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG3(t_string, t_string, t_bool)
    },

    {
        RTNM(StringLikeV1Member )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("StrLike"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3(t_string, t_string, (Vtypes)c_enum)
    },

    {
        RTNM(MidStmtStrMember )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBStringClass,
        /* method name   */       WIDE("MidStmtStr"),
        /* ret type      */       t_void,
        /* arg types     */       ARG4(t_ptr, t_i4, t_i4 ,t_string)
    },

    //
    // Microsoft.VisualBasic.Information
    //

    {
        RTNM(IsNumericV1Member   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBInformationClass,
        /* method name   */       WIDE("IsNumeric"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1((Vtypes)c_object)
    },

    {
        RTNM(TypeNameV1Member   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBInformationClass,
        /* method name   */       WIDE("TypeName"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1((Vtypes)c_object)
    },

    {
        RTNM(SystemTypeNameV1Member   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBInformationClass,
        /* method name   */       WIDE("SystemTypeName"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(VbTypeNameV1Member   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBInformationClass,
        /* method name   */       WIDE("VbTypeName"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_string)
    },

    //
    // Microsoft.VisualBasic.Interaction
    //

    {
        RTNM(CallByNameV1Member   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBInteractionClass,
        /* method name   */       WIDE("CallByName"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG4((Vtypes)c_object, t_string, (Vtypes)c_enum, (Vtypes)(m_array | c_object))
    },

    //
    // Microsoft.VisualBasic.CompilerServices.Conversions
    //

    {
        RTNM(ObjectToBooleanMember)
        TLIB(TLB_Desktop | TLB_Starlite)
         RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToBoolean"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToBooleanMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToBoolean"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(StringToSignedByteMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToSByte"),
        /* ret type      */       t_i1,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToSignedByteMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToSByte"),
        /* ret type      */       t_i1,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToByteMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToByte"),
        /* ret type      */       t_ui1,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToByteMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToByte"),
        /* ret type      */       t_ui1,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToCharArrayMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToCharArrayRankOne"),
        /* ret type      */       (Vtypes)(m_array | t_char),
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToCharArrayMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToCharArrayRankOne"),
        /* ret type      */       (Vtypes)(m_array | t_char),
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToCharMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToChar"),
        /* ret type      */       t_char,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToCharMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToChar"),
        /* ret type      */       t_char,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToDateMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToDate"),
        /* ret type      */       t_date,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToDateMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToDate"),
        /* ret type      */       t_date,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(BooleanToDecimalMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToDecimal"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1(t_bool)
    },

    {
        RTNM(StringToDecimalMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToDecimal"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToDecimalMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToDecimal"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToDoubleMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToDouble"),
        /* ret type      */       t_double,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToDoubleMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToDouble"),
        /* ret type      */       t_double,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToIntegerMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToInteger"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToIntegerMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToInteger"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToUnsignedIntegerMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToUInteger"),
        /* ret type      */       t_ui4,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToUnsignedIntegerMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToUInteger"),
        /* ret type      */       t_ui4,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToLongMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToLong"),
        /* ret type      */       t_i8,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToLongMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToLong"),
        /* ret type      */       t_i8,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToUnsignedLongMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToULong"),
        /* ret type      */       t_ui8,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToUnsignedLongMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToULong"),
        /* ret type      */       t_ui8,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToShortMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToShort"),
        /* ret type      */       t_i2,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToShortMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToShort"),
        /* ret type      */       t_i2,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToUnsignedShortMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToUShort"),
        /* ret type      */       t_ui2,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToUnsignedShortMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToUShort"),
        /* ret type      */       t_ui2,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(StringToSingleMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToSingle"),
        /* ret type      */       t_single,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(ObjectToSingleMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToSingle"),
        /* ret type      */       t_single,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(BooleanToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_bool)
    },

    {
        RTNM(ByteToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_ui1)
    },

    {
        RTNM(IntegerToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_i4)
    },

    {
        RTNM(UnsignedIntegerToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_ui4)
    },

    {
        RTNM(LongToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_i8)
    },

    {
        RTNM(UnsignedLongToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_ui8)
    },

    {
        RTNM(SingleToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(DoubleToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(DateToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_date)
    },

    {
        RTNM(DecimalToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(CharToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_char)
    },

    // This is a wrapper in the VB runtime because the debug api's can't call
    // CharArrayToStringMember directly for some reason.
    {
        RTNM(ArrayOfCharToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("FromCharArray"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1((Vtypes)(m_array | t_char))
    },

    // This is a wrapper in the VB runtime because the debug api's can't call
    // CharArrayToStringMember directly for some reason.
    {
        RTNM(CharAndCountToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("FromCharAndCount"),
        /* ret type      */       t_string,
        /* arg types     */       ARG2(t_char, t_i4)
    },

    // This is a wrapper in the VB runtime because the debug api's can't call
    // CharArrayToStringMember directly for some reason.
    {
        RTNM(ArrayOfCharToStringMemberSubset)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("FromCharArraySubset"),
        /* ret type      */       t_string,
        /* arg types     */       ARG3((Vtypes)(m_array | t_char), t_i4, t_i4)
    },

    {
        RTNM(ObjectToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ToString"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(ChangeTypeMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBConversionsClass,
        /* method name   */       WIDE("ChangeType"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG2((Vtypes)c_object, (Vtypes)c_class)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.ObjectFlowControl
    //

    {
        RTNM(SyncLockCheckMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBObjectFlowControlClass,
        /* method name   */       WIDE("CheckForSyncLockOnValueType"),
        /* ret type      */       t_void,
        /* arg types     */       ARG1((Vtypes)c_object)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.ObjectFlowControl.ForLoopControl
    //

    {
        RTNM(SingleForCheckMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBForLoopControlClass,
        /* method name   */       WIDE("ForNextCheckR4"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3(t_single, t_single, t_single)
    },

    {
        RTNM(DoubleForCheckMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBForLoopControlClass,
        /* method name   */       WIDE("ForNextCheckR8"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3(t_double, t_double, t_double)
    },

    {
        RTNM(DecimalForCheckMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBForLoopControlClass,
        /* method name   */       WIDE("ForNextCheckDec"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3(t_decimal, t_decimal, t_decimal)
    },

    {
        RTNM(ObjectForInitMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBForLoopControlClass,
        /* method name   */       WIDE("ForLoopInitObj"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG6((Vtypes)c_object, (Vtypes)c_object, (Vtypes)c_object, (Vtypes)c_object, (Vtypes)(m_byref | c_object), (Vtypes)(m_byref | c_object))
    },



    {
        RTNM(ObjectForNextMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBForLoopControlClass,
        /* method name   */       WIDE("ForNextCheckObj"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, (Vtypes)(m_byref | c_object))
    },

    {
        RTNM(LateBinderObjectForInitMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */      VBLateBinderForLoopControlClass,
        /* method name   */      WIDE("ForLoopInitObj"),
        /* ret type      */      t_bool,
        /* arg types     */      ARG6((Vtypes)c_object, (Vtypes)c_object, (Vtypes)c_object, (Vtypes)c_object, (Vtypes)(m_byref | c_object), (Vtypes)(m_byref | c_object))
    },

    {
        RTNM(LateBinderObjectForNextMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */    VBLateBinderForLoopControlClass,
        /* method name   */    WIDE("ForNextCheckObj"),
        /* ret type      */    t_bool,
        /* arg types     */    ARG3((Vtypes)c_object, (Vtypes)c_object, (Vtypes)(m_byref | c_object))
    },

    //
    // Microsoft.VisualBasic.CompilerServices.NewLateBinder
    //

    {
        RTNM(LateCanEvaluateMember)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingClass,
        /* method name   */       WIDE("LateCanEvaluate"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG6((Vtypes)c_object, (Vtypes)c_class, t_string, (Vtypes)(m_array | c_object), t_bool, t_bool)
    },

    {
        RTNM(LateGetMember)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingClass,
        /* method name   */       WIDE("LateGet"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG7((Vtypes)c_object, (Vtypes)c_class, t_string, (Vtypes)(m_array | c_object), (Vtypes)(m_array | t_string), (Vtypes)(m_array | c_class), (Vtypes)(m_array | t_bool))
    },

    {
        RTNM(LateSetMember)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingClass,
        /* method name   */       WIDE("LateSet"),
        /* ret type      */       t_void,
        /* arg types     */       ARG6((Vtypes)c_object, (Vtypes)c_class, t_string, (Vtypes)(m_array | c_object), (Vtypes)(m_array | t_string), (Vtypes)(m_array | c_class))
    },

    {
        RTNM(LateSetComplexMember)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingClass,
        /* method name   */       WIDE("LateSetComplex"),
        /* ret type      */       t_void,
        /* arg types     */       ARG8((Vtypes)c_object, (Vtypes)c_class, t_string, (Vtypes)(m_array | c_object), (Vtypes)(m_array | t_string), (Vtypes)(m_array | c_class), t_bool, t_bool)
    },

    {
        RTNM(LateCallMember)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingClass,
        /* method name   */       WIDE("LateCall"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG8((Vtypes)c_object, (Vtypes)c_class, t_string, (Vtypes)(m_array | c_object), (Vtypes)(m_array | t_string), (Vtypes)(m_array | c_class), (Vtypes)(m_array | t_bool), t_bool)
    },

    {
        RTNM(LateIndexGetMember)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingClass,
        /* method name   */       WIDE("LateIndexGet"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)(m_array | c_object), (Vtypes)(m_array | t_string))
    },

    {
        RTNM(LateIndexSetMember)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingClass,
        /* method name   */       WIDE("LateIndexSet"),
        /* ret type      */       t_void,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)(m_array | c_object), (Vtypes)(m_array | t_string))
    },

    {
        RTNM(LateIndexSetComplexMember)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLateBindingClass,
        /* method name   */       WIDE("LateIndexSetComplex"),
        /* ret type      */       t_void,
        /* arg types     */       ARG5((Vtypes)c_object, (Vtypes)(m_array | c_object), (Vtypes)(m_array | t_string), t_bool, t_bool)
    },


    //
    // Microsoft.VisualBasic.CompilerServices.Operators
    //

    {
        RTNM(CompareObjectEqualMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("CompareObjectEqual"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(ConditionalCompareObjectEqualMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("ConditionalCompareObjectEqual"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(CompareObjectNotEqualMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("CompareObjectNotEqual"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(ConditionalCompareObjectNotEqualMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("ConditionalCompareObjectNotEqual"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(CompareObjectLessMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("CompareObjectLess"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(ConditionalCompareObjectLessMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("ConditionalCompareObjectLess"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(CompareObjectLessEqualMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("CompareObjectLessEqual"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(ConditionalCompareObjectLessEqualMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("ConditionalCompareObjectLessEqual"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(CompareObjectGreaterEqualMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("CompareObjectGreaterEqual"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(ConditionalCompareObjectGreaterEqualMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("ConditionalCompareObjectGreaterEqual"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(CompareObjectGreaterMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("CompareObjectGreater"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(ConditionalCompareObjectGreaterMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("ConditionalCompareObjectGreater"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(ObjectCompareMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("CompareObject"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, t_bool)
    },

    {
        RTNM(CompareStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("CompareString"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG3(t_string, t_string, t_bool)
    },

    {
        RTNM(PlusObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("PlusObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(NegateObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("NegateObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(NotObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("NotObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    {
        RTNM(AndObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("AndObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(OrObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("OrObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(XorObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("XorObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(AddObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("AddObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(SubtractObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("SubtractObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(MultiplyObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("MultiplyObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(DivideObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("DivideObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ExponentObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("ExponentObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(ModObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("ModObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(IntDivideObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("IntDivideObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(LeftShiftObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("LeftShiftObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(RightShiftObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("RightShiftObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    {
        RTNM(LikeStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLikeOperatorClass,
        /* method name   */       WIDE("LikeString"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG3(t_string, t_string, (Vtypes)c_enum)
    },

    {
        RTNM(LikeObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBLikeOperatorClass,
        /* method name   */       WIDE("LikeObject"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG3((Vtypes)c_object, (Vtypes)c_object, (Vtypes)c_enum)
    },

    {
        RTNM(ConcatenateObjectMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBOperatorsClass,
        /* method name   */       WIDE("ConcatenateObject"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG2((Vtypes) c_object, (Vtypes) c_object)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.EmbeddedOperators
    //
    {
        RTNM(EmbeddedCompareStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBEmbeddedOperatorsClass,
        /* method name   */       WIDE("CompareString"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG3(t_string, t_string, t_bool)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.ProjectData
    //

    {
        RTNM(EndStatementMember)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBProjectDataClass,
        /* method name   */       WIDE("EndApp"),
        /* ret type      */       t_void,
        /* arg types     */       ARG0()
    },

    {
        RTNM(CreateErrorMember)
        TLIB(TLB_Desktop | TLB_Starlite)
         RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBProjectDataClass,
        /* method name   */       WIDE("CreateProjectError"),
        /* ret type      */       (Vtypes)c_exception,
        /* arg types     */       ARG1(t_i4)
    },

    {
        RTNM(ClearErrorMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBProjectDataClass,
        /* method name   */       WIDE("ClearProjectError"),
        /* ret type      */       t_void,
        /* arg types     */       ARG0()
    },

    {
        RTNM(SetErrorMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBProjectDataClass,
        /* method name   */       WIDE("SetProjectError"),
        /* ret type      */       t_void,
        /* arg types     */       ARG1((Vtypes)c_exception)
    },

    {
        RTNM(SetErrorMemberWithLineNum)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBProjectDataClass,
        /* method name   */       WIDE("SetProjectError"),
        /* ret type      */       t_void,
        /* arg types     */       ARG2((Vtypes)c_exception, t_i4)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.Utils
    //

    {
        RTNM(CopyArrayMember   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBUtilsClass,
        /* method name   */       WIDE("CopyArray"),
        /* ret type      */       (Vtypes)c_array,
        /* arg types     */       ARG2((Vtypes)c_array, (Vtypes)c_array)
    },

    //
    // Microsoft.VisualBasic.CompilerServices.Versioned
    //

    {
        RTNM(IsNumericMember   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBVersionedClass,
        /* method name   */       WIDE("IsNumeric"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1((Vtypes)c_object)
    },

    {
        RTNM(TypeNameMember   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBVersionedClass,
        /* method name   */       WIDE("TypeName"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1((Vtypes)c_object)
    },

    {
        RTNM(SystemTypeNameMember   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBVersionedClass,
        /* method name   */       WIDE("SystemTypeName"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(VbTypeNameMember   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBVersionedClass,
        /* method name   */       WIDE("VbTypeName"),
        /* ret type      */       t_string,
        /* arg types     */       ARG1(t_string)
    },

    {
        RTNM(CallByNameMember   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       VBVersionedClass,
        /* method name   */       WIDE("CallByName"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG4((Vtypes)c_object, t_string, (Vtypes)c_enum, (Vtypes)(m_array | c_object))
    },


    //
    // System.Runtime.CompilerServices.AccessedThroughPropertyAttribute
    //

    {
        RTNM(AccessedThroughPropertyAttributeCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMAccessedThroughPropertyAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_string)
    },

    //
    // System.Convert
    //

    {
        RTNM(DecimalToBooleanMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToBoolean"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToSignedByteMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToSByte"),
        /* ret type      */       t_i1,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToByteMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToByte"),
        /* ret type      */       t_ui1,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToShortMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToInt16"),
        /* ret type      */       t_i2,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToUnsignedShortMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToUInt16"),
        /* ret type      */       t_ui2,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToIntegerMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToInt32"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToUnsignedIntegerMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToUInt32"),
        /* ret type      */       t_ui4,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToLongMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToInt64"),
        /* ret type      */       t_i8,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToUnsignedLongMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToUInt64"),
        /* ret type      */       t_ui8,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToSingleMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToSingle"),
        /* ret type      */       t_single,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToDoubleMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToDouble"),
        /* ret type      */       t_double,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(IntegerToBooleanMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToBoolean"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1(t_i4)
    },

    {
        RTNM(UnsignedIntegerToBooleanMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToBoolean"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1(t_ui4)
    },

    {
        RTNM(LongToBooleanMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToBoolean"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1(t_i8)
    },

    {
        RTNM(UnsignedLongToBooleanMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToBoolean"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1(t_ui8)
    },

    {
        RTNM(SingleToBooleanMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToBoolean"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(DoubleToBooleanMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToBoolean"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(SingleToSignedByteMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToSByte"),
        /* ret type      */       t_i1,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(SingleToByteMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToByte"),
        /* ret type      */       t_ui1,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(SingleToShortMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToInt16"),
        /* ret type      */       t_i2,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(SingleToUnsignedShortMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToUInt16"),
        /* ret type      */       t_ui2,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(SingleToIntegerMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToInt32"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(SingleToUnsignedIntegerMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToUInt32"),
        /* ret type      */       t_ui4,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(SingleToLongMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToInt64"),
        /* ret type      */       t_i8,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(SingleToUnsignedLongMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToUInt64"),
        /* ret type      */       t_ui8,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(DoubleToSignedByteMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToSByte"),
        /* ret type      */       t_i1,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(DoubleToByteMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToByte"),
        /* ret type      */       t_ui1,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(DoubleToShortMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToInt16"),
        /* ret type      */       t_i2,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(DoubleToUnsignedShortMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToUInt16"),
        /* ret type      */       t_ui2,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(DoubleToIntegerMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToInt32"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(DoubleToUnsignedIntegerMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToUInt32"),
        /* ret type      */       t_ui4,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(DoubleToLongMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToInt64"),
        /* ret type      */       t_i8,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(DoubleToUnsignedLongMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMConvertClass,
        /* method name   */       WIDE("ToUInt64"),
        /* ret type      */       t_ui8,
        /* arg types     */       ARG1(t_double)
    },

    //
    // System.DateTime
    //

    {
        RTNM(DateFromConstantMember   )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDateTimeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_i8)
    },

    {
        RTNM(CompareDateMember )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDateTimeClass,
        /* method name   */       WIDE("Compare"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG2(t_date, t_date)
    },

    {
        RTNM(DateConstZeroField)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_FIELD | RTF_STATIC,
        /* parent class  */       COMDateTimeClass,
        /* method name   */       WIDE("MinValue"),
        /* ret type      */       t_date,
        /* arg types     */       ARG0()
    },

    {
        RTNM(DateTimeEqual)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDateTimeClass,
        /* method name   */       WIDE("op_Equality"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_date, t_date)
    },

    {
        RTNM(DateTimeGreaterThan)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDateTimeClass,
        /* method name   */       WIDE("op_GreaterThan"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_date, t_date)
    },

    {
        RTNM(DateTimeGreaterThanOrEqual)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDateTimeClass,
        /* method name   */       WIDE("op_GreaterThanOrEqual"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_date, t_date)
    },

    {
        RTNM(DateTimeInequality)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDateTimeClass,
        /* method name   */       WIDE("op_Inequality"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_date, t_date)
    },

    {
        RTNM(DateTimeLessThan)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDateTimeClass,
        /* method name   */       WIDE("op_LessThan"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_date, t_date)
    },

    {
        RTNM(DateTimeLessThanOrEqual)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDateTimeClass,
        /* method name   */       WIDE("op_LessThanOrEqual"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_date, t_date)
    },

    //
    // System.Decimal
    //

    {
        RTNM(CompareDecimalMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("Compare"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    {
        RTNM(DecimalFromConstantMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDecimalClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG5(t_i4, t_i4, t_i4, t_bool, t_ui1)
    },

    {
        RTNM(DecimalFromInt64Member)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDecimalClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_i8)
    },

    {
        RTNM(DecimalConstZeroField)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_FIELD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("Zero"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG0()
    },

    {
        RTNM(DecimalConstOneField)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_FIELD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("One"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG0()
    },

    {
        RTNM(DecimalConstMinusOneField)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_FIELD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("MinusOne"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG0()
    },

    {
        RTNM(DecimalNegateMember    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("Negate"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalModuloMember    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("Remainder"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    {
        RTNM(DecimalAddMember    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("Add"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    {
        RTNM(DecimalSubtractMember    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("Subtract"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    {
        RTNM(DecimalMultiplicationMember    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("Multiply"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    {
        RTNM(DecimalDivisionMember    )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("Divide"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    {
        RTNM(IntegerToDecimalMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDecimalClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_i4)
    },

    {
        RTNM(UnsignedIntegerToDecimalMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDecimalClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_ui4)
    },

    {
        RTNM(LongToDecimalMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDecimalClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_i8)
    },

    {
        RTNM(UnsignedLongToDecimalMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDecimalClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_ui8)
    },

    {
        RTNM(SingleToDecimalMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDecimalClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(DoubleToDecimalMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDecimalClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(DecimalToSignedByteCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_i1,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToByteCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_ui1,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToShortCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_i2,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToUnsignedShortCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_ui2,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToIntegerCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_i4,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToUnsignedIntegerCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_ui4,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToLongCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_i8,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToUnsignedLongCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_ui8,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToSingleCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_single,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(DecimalToDoubleCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_double,
        /* arg types     */       ARG1(t_decimal)
    },

    {
        RTNM(IntegerToDecimalCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Implicit"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1(t_i4)
    },

    {
        RTNM(UnsignedIntegerToDecimalCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Implicit"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1(t_ui4)
    },

    {
        RTNM(LongToDecimalCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Implicit"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1(t_i8)
    },

    {
        RTNM(UnsignedLongToDecimalCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Implicit"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1(t_ui8)
    },

    {
        RTNM(SingleToDecimalCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1(t_single)
    },

    {
        RTNM(DoubleToDecimalCast)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Explicit"),
        /* ret type      */       t_decimal,
        /* arg types     */       ARG1(t_double)
    },

    {
        RTNM(DecimalEqual)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Equality"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    {
        RTNM(DecimalGreaterThan)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_GreaterThan"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    {
        RTNM(DecimalGreaterThanOrEqual)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_GreaterThanOrEqual"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    {
        RTNM(DecimalInequality)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_Inequality"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    {
        RTNM(DecimalLessThan)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_LessThan"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    {
        RTNM(DecimalLessThanOrEqual)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDecimalClass,
        /* method name   */       WIDE("op_LessThanOrEqual"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG2(t_decimal, t_decimal)
    },

    //
    // System.Diagnostics.DebuggableAttribute
    //

    {
        RTNM(DebuggableAttributeCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDebuggableAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG2(t_bool, t_bool)
    },

    {
        RTNM(DebuggableAttributeCtor2)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDebuggableAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1((Vtypes)c_enum)
    },

    //
    // System.Diagnostics.Debugger
    //

    {
        RTNM(SystemDebugBreakMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDebuggerClass,
        /* method name   */       WIDE("Break"),
        /* ret type      */       t_void,
        /* arg types     */       ARG0()
    },

    //
    // System.Diagnostics.DebuggerNonUserCode
    //

    {
        RTNM(DebuggerNonUserCodeAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDebuggerNonUserCodeAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG0()
    },

    //
    // System.Diagnostics.DebuggerStepThrough
    //

    {
        RTNM(DebuggerStepThroughAttributeCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDebuggerStepThroughAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG0()
    },

    //
    // System.Diagnostics.DebuggerDisplay
    //

    {
        RTNM(DebuggerDisplayAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDebuggerDisplayAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_string)
    },

    //
    // System.Diagnostics.DebuggerBrowsable
    //

    {
        RTNM(DebuggerBrowsableAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDebuggerBrowsableAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_i4)
    },

    //
    // System.Math
    //

    {
        RTNM(NumericPowerMember     )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMMathClass,
        /* method name   */       WIDE("Pow"),
        /* ret type      */       t_double,
        /* arg types     */       ARG2(t_double, t_double)
    },

    {
        RTNM(NumericRoundMember      )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMMathClass,
        /* method name   */       WIDE("Round"),
        /* ret type      */       t_double,
        /* arg types     */       ARG1(t_double)
    },

    //
    // System.ParamArrayAttribute
    //

    {
        RTNM(ParamArrayAttributeCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMParamArrayAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG0()
    },

    //
    // System.STAThreadAttribute
    //

    {
        RTNM(STAThreadAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMSTAThreadAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG0()
    },

    //
    // System.Reflection.Assembly
    //

    {
        RTNM(LoadAssembly)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD,
        /* parent class  */       COMAssemblyClass,
        /* method name   */       WIDE("Load"),
        /* ret type      */       (Vtypes) t_ref,
        /* arg types     */       ARG1(t_string)
    },

    //
    // Sysmte.Reflection.AssemblyFlagsAttribute
    //
    {
        RTNM(AssemblyFlagsAttributeIntCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMAssemblyFlagsAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_i4)
    },

    {
        RTNM(AssemblyFlagsAttributeUIntCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMAssemblyFlagsAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_ui4)
    },

    {
        RTNM(AssemblyFlagsAttributeAssemblyNameFlagsCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMAssemblyFlagsAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1((Vtypes)c_enum)
    },

    //
    // System.Reflection.DefaultMemberAttribute
    //

    {
        RTNM(DefaultMemberAttributeCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDefaultMemberAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_string)
    },

    //
    // System.Reflection.FieldInfo
    //

    {
        RTNM(GetFieldFromHandleMember  )
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMFieldInfoClass,
        /* method name   */       WIDE("GetFieldFromHandle"),
        /* ret type      */       (Vtypes)c_class,
        /* arg types     */       ARG1((Vtypes)c_typehandle)
    },

    {
        RTNM(GetFieldFromHandleGenericMember  )
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMFieldInfoClass,
        /* method name   */       WIDE("GetFieldFromHandle"),
        /* ret type      */       (Vtypes)c_class,
        /* arg types     */       ARG2((Vtypes)c_typehandle, (Vtypes)c_typehandle)
    },

    //
    // System.Reflection.MethodBase
    //

    {
        RTNM(GetMethodFromHandleMember  )
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMMethodBaseClass,
        /* method name   */       WIDE("GetMethodFromHandle"),
        /* ret type      */       (Vtypes)c_class,
        /* arg types     */       ARG1((Vtypes)c_typehandle)
    },

    {
        RTNM(GetMethodFromHandleGenericMember  )
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMMethodBaseClass,
        /* method name   */       WIDE("GetMethodFromHandle"),
        /* ret type      */       (Vtypes)c_class,
        /* arg types     */       ARG2((Vtypes)c_typehandle, (Vtypes)c_typehandle)
    },

    //
    // System.Reflection.MethodInfo
    //

    {
        RTNM(IsGenericMethodDefinitionMember  )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_GET_PROP | RTF_VIRTUAL,
        /* parent class  */       COMMethodInfoClass,
        /* method name   */       WIDE("IsGenericMethodDefinition"),
        /* ret type      */       t_bool,
        /* arg types     */       ARG0()
    },

    {
        RTNM(CreateDelegateMemberFromMethodInfo )
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_VIRTUAL,
        /* parent class  */       COMMethodInfoClass,
        /* method name   */       WIDE("CreateDelegate"),
        /* ret type      */       (Vtypes)c_class,
        /* arg types     */       ARG2((Vtypes)c_class, (Vtypes)c_object)
    },

    //
    // System.Runtime.CompilerServices.RuntimeHelpers
    //

    {
        RTNM(GetObjectValue)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMRuntimeHelpersClass,
        /* method name   */       WIDE("GetObjectValue"),
        /* ret type      */       (Vtypes) c_object,
        /* arg types     */       ARG1((Vtypes) c_object)
    },

    //
    // System.Runtime.CompilerServices.DecimalConstantAttribute
    //

    {
        RTNM(DecimalConstantAttributeCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDecimalConstantAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG5(t_ui1, t_ui1, t_ui4, t_ui4, t_ui4)
    },

    //
    // System.Runtime.CompilerServices.DateTimeConstantAttribute
    //

    {
        RTNM(DateTimeConstantAttributeCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDateTimeConstantAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_i8)
    },

    //
    // System.String
    //

    {
        RTNM(StringConcatenationMember )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMStringClass,
        /* method name   */       WIDE("Concat"),
        /* ret type      */       t_string,
        /* arg types     */       ARG2(t_string, t_string)
    },

    {
        RTNM(CharArrayToStringMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMStringClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1((Vtypes)(m_array | t_char))
    },

    //
    // System.Type
    //

    {
        RTNM(GetTypeFromHandleMember  )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMTypeClass,
        /* method name   */       WIDE("GetTypeFromHandle"),
        /* ret type      */       (Vtypes)c_class,
        /* arg types     */       ARG1((Vtypes)c_typehandle)
    },

    {
        RTNM(GetTypeFromString )
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMTypeClass,
        /* method name   */       WIDE("GetType"),
        /* ret type      */       (Vtypes)c_class,
        /* arg types     */       ARG3(t_string, t_bool, t_bool)
    },

    //
    // System.Array
    //
    {
        RTNM(CreateArrayInstance  )
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMArrayClass,
        /* method name   */       WIDE("CreateInstance"),
        /* ret type      */       (Vtypes)t_ref,
        /* arg types     */       ARG2((Vtypes)c_class, (Vtypes)(m_array | t_i4))
    },

    //
    // System.Runtime.InteropServices.GuidAttribute
    //
    {
        RTNM(GuidAttributeCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMGuidAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_string)
    },

    //
    // System.Runtime.InteropServices.ClassInterfaceAttribute
    //
    {
        RTNM(ClassInterfaceAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMClassInterfaceAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_i4)
    },

    //
    // System.Runtime.InteropServices.ComSourceInterfacesAttribute
    //
    {
        RTNM(ComSourceInterfacesAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMComSourceInterfacesAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_string)
    },

    //
    // System.Runtime.InteropServices.InterfaceTypeAttribute
    //
    {
        RTNM(InterfaceTypeAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMInterfaceTypeAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_i4)
    },

    //
    // System.Runtime.InteropServices.DispIdAttribute
    //
    {
        RTNM(DispIdAttributeCtor)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMDispIdAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_i4)
    },

    //
    // System.Runtime.InteropServices.ComVisibleAttribute
    //
    {
        RTNM(ComVisibleAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMComVisibleAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_bool)
    },

    //
    // System.Runtime.InteropServices.TypeIdentifierAttribute
    //
    {
        RTNM(TypeIdentifierAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMTypeIdentifierAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG0()
    },

    //
    // System.Runtime.InteropServices.TypeIdentifierAttribute
    //
    {
        RTNM(TypeIdentifierAttributePlusGuidCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMTypeIdentifierAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG2(t_string, t_string)
    },

    //
    // System.Runtime.InteropServices.MarshalAsAttribute
    //
    {
        RTNM(MarshalAsAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMMarshalAsAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG1(t_i2)
    },

    //
    // System.Runtime.InteropServices.StructLayoutAttribute
    //
    {
        RTNM(StructLayoutAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMStructLayoutAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG1(t_i2)
    },

    //
    // System.Runtime.InteropServices.BestFitMappingAttribute
    //
    {
        RTNM(BestFitMappingAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMBestFitMappingAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG1(t_bool)
    },

    //
    // System.Runtime.InteropServices.FieldOffsetAttribute
    //
    {
        RTNM(FieldOffsetAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMFieldOffsetAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG1(t_i4)
    },

    //
    // System.Runtime.InteropServices.LCIDConversionAttribute
    //
    {
        RTNM(LCIDConversionAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMLCIDConversionAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG1(t_i4)
    },

    //
    // System.Runtime.InteropServices.InAttribute
    //
    {
        RTNM(InAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMInAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG0()
    },

           //
    // System.Runtime.InteropServices.OutAttribute
    //
    {
        RTNM(OutAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMOutAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG0()
    },

    //
    // System.Runtime.InteropServices.TypeLibTypeAttribute
    //
    {
        RTNM(TypeLibTypeAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMTypeLibTypeAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG1(t_i2)
    },

    //
    // System.Runtime.InteropServices.TypeLibFuncAttribute
    //
    {
        RTNM(TypeLibFuncAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMTypeLibFuncAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG1(t_i2)
    },

    //
    // System.Runtime.InteropServices.TypeLibVarAttribute
    //
    {
        RTNM(TypeLibVarAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMTypeLibVarAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG1(t_i2)
    },


    //
    // System.Runtime.InteropServices.UnmanagedFunctionPointerAttribute
    //
    {
        RTNM(UnmanagedFunctionPointerAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMUnmanagedFunctionPointerAttributeClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG1(t_i4)
    },

    //
    // System.Activator
    //

    {
        RTNM(CreateInstanceMember)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMActivatorClass,
        /* method name   */       WIDE("CreateInstance"),
        /* ret type      */       (Vtypes)c_object,
        /* arg types     */       ARG1((Vtypes)c_class)
    },

    {
        RTNM(GenericCreateInstanceMember)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMActivatorClass,
        /* method name   */       WIDE("CreateInstance"),
        /* ret type      */       t_generic,
        /* arg types     */       ARG0()
    },

    //
    // System.Runtime.CompilerServices.CompilationRelaxations.Attribute
    //

    {
        RTNM(CompilationRelaxationsAttributeCtor)
        TLIB(TLB_Desktop /*| TLB_Starlite*/) // 
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMCompilationRelaxationsAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG1(t_i4)
    },

    //
    // System.Runtime.CompilerServices.CompilerGenerated.Attribute
    //

    {
        RTNM(CompilerGeneratedAttributeCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMCompilerGeneratedAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG0()
    },

    //
    // System.Runtime.CompilerServices.CompilationRelaxations.Attribute
    //

    {
        RTNM(RuntimeCompatibilityAttributeCtor)
        TLIB(TLB_Desktop /*| TLB_Starlite*/) // 
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */       COMRuntimeCompatibilityAttributeClass,
        /* method name   */       COR_CTOR_METHOD_NAME_W,
        /* ret type      */       t_void,
        /* arg types     */       ARG0()
    },

    //
    // System.Delegate.CreateInstance()
    //

    {
        RTNM(CreateDelegateMember)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMDelegateClass,
        /* method name   */       WIDE("CreateDelegate"),
        /* ret type      */       (Vtypes)c_class,
        /* arg types     */       ARG4((Vtypes)c_class, (Vtypes)c_object, (Vtypes)c_class, t_bool)
    },

    //
    // System.Threading.Monitor.Enter with two arguments
    //
        
    {
        RTNM(MonitorEnter_ObjectByRefBoolean)
        TLIB(TLB_Desktop | TLB_Starlite)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */       COMMonitorClass,
        /* method name   */       WIDE("Enter"),
        /* ret type      */       t_void,
        /* arg types     */       ARG2((Vtypes)c_object, (Vtypes)(m_byref | t_bool))
    },

        //
        // System.FlagsAttribute
        //
          
    {
        RTNM(FlagsAttributeCtor)
        TLIB(TLB_Desktop) // | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMFlagsAttributeClass,
        /* method name   */     COR_CTOR_METHOD_NAME_W,
        /* ret type      */         t_void,
        /* arg types     */         ARG0()
     },

     
        //
        // System.Environment
        //
          
    {
        RTNM(GetCurrentManagedThreadId)
        TLIB(TLB_Desktop) // | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_GET_PROP,
        /* parent class  */        COMEnvironmentClass,
        /* method name   */        WIDE("CurrentManagedThreadId"),
        /* ret type      */         t_i4,
        /* arg types     */         ARG0()
     },


     //
     // System.Runtime.DesignerServices
     //

    {
        RTNM(WinRTDesignerContextCtor)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD | RTF_CONSTRUCTOR,
        /* parent class  */        COMWinRTDesignerContextClass,
        /* method name   */        COR_CTOR_METHOD_NAME_W,
        /* ret type      */        t_void,
        /* arg types     */        ARG3((Vtypes)c_class, t_string, t_bool)
    },

    {
        RTNM(WinRTDesignerContextGetAssembly)
        TLIB(TLB_Desktop)
        RTNV(RuntimeVersion2)
        RTF_METHOD,
        /* parent class  */        COMWinRTDesignerContextClass,
        /* method name   */        WIDE("GetAssembly"),
        /* ret type      */        t_ref,
        /* arg types     */        ARG1(t_string)
    },


    //
    // System.AppDomain
    //

     {
        RTNM(AppDomainIsAppXModel)
        TLIB(TLB_Desktop) // | TLB_Starlite)
        RTNV(RuntimeVersion1)
        RTF_METHOD | RTF_STATIC,
        /* parent class  */        COMAppDomainClass,
        /* method name   */        WIDE("IsAppXModel"),
        /* ret type      */        t_bool,
        /* arg types     */        ARG0()
     },
};

//============================================================================
// Helper for "DoesMemberMatchDescriptor" that answers whether a method
// symbol matches an RuntimeMemberDescriptor.
//============================================================================
// 
bool RuntimeMemberDescriptor::DoesTypeMatchVtype(
    BCSYM * ptyp,
    Vtypes vtype,
    Compiler * pCompiler)
{
    if (vtype == ptyp->GetVtype())
    {
        return true;
    }

TryAgain:

    if ((vtype & m_array) && ptyp->IsArrayType())
    {
        ptyp = ptyp->PArrayType()->GetRoot();   // get the vtype from the symbol

        vtype = (Vtypes)(vtype & ~m_array);   // strip off the array bit

        goto TryAgain;
    }
    else if ((vtype & m_byref) && ptyp->IsPointerType())
    {
        ptyp = ptyp->PPointerType()->GetRoot();   // get the vtype from the symbol

        vtype = (Vtypes)(vtype & ~m_byref);   // strip off the byref bit

        goto TryAgain;
    }

    switch (vtype)
    {
    case (Vtypes)c_object:
        return ptyp->IsObject();

    case (Vtypes)c_enum:
        return ptyp->IsEnum();

    case (Vtypes)c_exception:
    case (Vtypes)c_array:
    case (Vtypes)c_typehandle:
        return ptyp->IsClass();

    case (Vtypes)c_class:
        // Microsoft: 

        if( ptyp->GetVtype() != t_string )
        {
            return ptyp->IsClass() || ptyp->IsInterface();
        }
        else
        {
            return false;
        }

    default:
        return vtype == ptyp->GetVtype();
    }
}

//
//  Does the NamedRoot match the Descriptor
//
bool RuntimeMemberDescriptor::DoesMemberMatchDescriptor(        
    BCSYM_NamedRoot * pMember,  // Member to compare against
    unsigned short usFlags,
    Vtypes vtypRet,  // retrun type of the member          
    Vtypes *vtypArgs, // argument types of the member
    Compiler * pCompiler)
{
    if (pMember->IsBad())
        return false;

    if (usFlags & RTF_FIELD)
    {
        VSASSERT(pMember->IsVariable(), "Can't overload a field.");

        return true;
    }
    else
    {
        BCSYM_Proc *pProc = pMember->PProc();

         // Check the return type.
        if (!pProc->GetType())
        {
            if (vtypRet != t_void)
            {
                return false;
            }
        }
        else if (!DoesTypeMatchVtype(pProc->GetType(), vtypRet, pCompiler))
        {
            return false;
        }

        // Check all of the parameters.
        BCSYM_Param *pParam = pProc->GetFirstParam();
 
        // This loop terminates by the fact that vtypArgs ends with t_UNDEF
        while (pParam)
        {
            if (!DoesTypeMatchVtype(pParam->GetType(), *vtypArgs, pCompiler))
                return false;
            vtypArgs++;
            pParam = pParam->GetNext();
        }

        if (*vtypArgs == t_UNDEF)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}


#if DEBUG

void CheckRTLangHelper()
{
    int i;

    VSASSERT( sizeof(g_rgRTLangMembers)/sizeof(g_rgRTLangMembers[0]) == MaxRuntimeMember, 
               "CheckRTLangHelper: inconsistent g_rgRTLangMembers[] table size");

    VSASSERT( sizeof(g_rgRTLangClasses)/sizeof(g_rgRTLangClasses[0]) == MaxRuntimeClass,  
               "CheckRTLangHelper: inconsistent g_rgRTLangClasses[] table size");

    VSASSERT( sizeof(g_rgRTLangLibraries)/sizeof(g_rgRTLangLibraries[0]) == MaxRuntimeLibrary, 
               "CheckRTLangHelper: inconsistent g_rgRTLangLibraries[] table size");

    for (i = 0; i < MaxRuntimeLibrary; i++)
    {
        VSASSERT( g_rgRTLangLibraries[i].rtCheck == i,
                   "CheckRTLangLangHelper: inconsistent g_rgRTLangLibraries enumeration.");
    }

    for (i = 0; i < MaxRuntimeClass; i++)
    {
        VSASSERT( g_rgRTLangClasses[i].rtCheck == i,
                   "CheckRTLangLangHelper: inconsistent g_rgRTLangClasses enumeration.");
    }

    for (i = 0; i < MaxRuntimeMember; i++ )
    {
        VSASSERT(g_rgRTLangMembers[i].rtCheck == i,
                  "CheckRTLangLangHelper: inconsistent g_rgRTLangMembers enumeration");
    }
}

void VerifyRTLangEnum(unsigned rt)
{
    VSASSERT( rt != UndefinedRuntimeMember && rt < MaxRuntimeMember,
               "VerifyRTLangEnum: invalid RTC_LANG_MEMBERS enum");
}

void VerifyRTLangClassEnum(unsigned rt)
{
    VSASSERT( rt != UndefinedRuntimeClass && rt < MaxRuntimeClass,
               "VerifyRTLangClassEnum: invalid RTC_LANG_CLASSES enum");
}

#endif
