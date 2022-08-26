//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Tables for defining symbols, types, operators, and other constructs.
//
//-------------------------------------------------------------------------------------------------


// Here is a complete list of types.
// The correctness of e.g. ClassifyPredefinedCLRConversion() depends on knowing a complete list
// of types. So it calls VSASSERT(TypeHelpers::IsKnownType(t), "unknown type") to make sure
// that its arguments are in this list. If you add new types to the language, you'll have to
// make sure they're handled properly; see comments about this in IsKnownType.
// If you have doubts, please talk with Microsoft (31/May/2008)
// 
// A complete list of types:
//
//   * Void, t_void. (It arises in VB as the type of untyped expressions, e.g. AddressOf and lambda.
//            It can also arise when we meta-import from other languages.
//            But VB itself has no void type.)
//
//   * Bad, t_bad. (a bad type comes from an incorrect VB program that won't compile. But functions
//           are still expected to do the best they can if given bad inputs.)
//
//   * Boolean, t_boolean (aka System.Boolean)
//
//   * Integral types...
//        * t_i1, t_ui1, t_i2, t_ui2, t_i4, t_ui4, t_i8, t_ui8
//          (also called int8,uint8,int16,uint16,... in the CLR spec, and System.Byte, System.SByte, ...)
//        * User-defined enums E(Of Binding) with specified integral underlying type
//
//   * Decimal, t_dec
//
//   * Real types...
//        * t_single, t_double (aka r4, r8 in the CLR spec, and aka System.Real and System.Double)
//
//   * Date, t_date
//
//   * Char, t_char
//
//   * String, t_string
//
//   * Arrays, t_array, with a specified base-element-type and rank
//
//   * ByRefs, t_ptr, of a specified type. But the places where these occur in the compiler are rare. Normally we only pass
//     around their base type.
//
//   * User-defined structures S(Of Binding), t_struct
//
//   * User-defined reference types t_ref...
//        * Interfaces I(Of Binding)
//        * Class C(Of Binding)
//        * Delegate D(Of Bidning)
//
//   * User-named generic parameters T, t_generic. These might have "struct" or "ref" constraint-flags,
//     and might be constrained to be other types. VB only allows constraints to be other generic-parameters,
//     or interfaces, or classes, but we might meta-import generic parameters with other constraints.
//
//
// Some types satisfy IsReferenceType(), and others satisfy IsValueType(), and the remainder satisfy neither.
// The only types which satisfy neither are:
//   * void
//   * bad
//   * generic parameters without a struct or a ref constraint, such that neither this generic parameter
//     nor any other generic parameter reachable through constraints has a type-constraint.



#ifdef DEF_BCSYM

//
// This gets included in FxTypeTable.h, in the FX namespace.
//

//
//        Symbol Kind              Name
//        -----------------------  ---------------------------
DEF_BCSYM(Uninitialized           , "Uninitialized symbol"     )
DEF_BCSYM(VoidType                , "VoidType"                 )
DEF_BCSYM(PointerType             , "PointerType"              )
DEF_BCSYM(NamedType               , "NamedType"                )
DEF_BCSYM(GenericBadNamedRoot     , "GenericBadNamedRoot"      )
DEF_BCSYM(Hash                    , "Hash"                     )
DEF_BCSYM(Alias                   , "Alias"                    )
DEF_BCSYM(CCContainer             , "CCContainer"              )
DEF_BCSYM(Class                   , "Class"                    )
DEF_BCSYM(MethodImpl              , "MethodImpl"               )
DEF_BCSYM(SyntheticMethod         , "SyntheticMethod"          )
DEF_BCSYM(MethodDecl              , "MethodDecl"               )
DEF_BCSYM(EventDecl               , "EventDecl"                )
DEF_BCSYM(DllDeclare              , "DllDeclare"               )
DEF_BCSYM(Param                   , "Param"                    )
DEF_BCSYM(ParamWithValue          , "ParamWithValue"           )
DEF_BCSYM(Variable                , "Variable"                 )
DEF_BCSYM(VariableWithValue       , "VariableWithValue"        )
DEF_BCSYM(VariableWithArraySizes  , "VariableWithArraySizes"   )
DEF_BCSYM(CCConstant              , "CCConstant"               )
DEF_BCSYM(StaticLocalBackingField , "StaticLocalBackingField"  )
DEF_BCSYM(Expression              , "Expression"               )
DEF_BCSYM(Implements              , "Implements"               )
DEF_BCSYM(Interface               , "Interface"                )
DEF_BCSYM(HandlesList             , "HandlesList"              )
DEF_BCSYM(ImplementsList          , "ImplementsList"           )
DEF_BCSYM(Namespace               , "Namespace"                )
DEF_BCSYM(NamespaceRing           , "NamespaceRing"            )
DEF_BCSYM(XmlName                 , "XmlName"                  )
DEF_BCSYM(XmlNamespaceDeclaration , "XmlNamespaceDeclaration"  )
DEF_BCSYM(XmlNamespace            , "XmlNamespace"             )
DEF_BCSYM(Property                , "Property"                 )
DEF_BCSYM(ArrayType               , "ArrayType"                )
DEF_BCSYM(ArrayLiteralType        , "ArrayLiteralType"         )
DEF_BCSYM(ApplAttr                , "ApplAttr"                 )
DEF_BCSYM(UserDefinedOperator     , "UserDefinedOperator"      )
DEF_BCSYM(GenericParam            , "GenericParam"             )
DEF_BCSYM(GenericConstraint       , "GenericConstraint"        )
DEF_BCSYM(GenericTypeConstraint   , "GenericTypeConstraint"    )
DEF_BCSYM(GenericNonTypeConstraint, "GenericNonTypeConstraint" )
DEF_BCSYM(GenericBinding          , "GenericBinding"           )
DEF_BCSYM(GenericTypeBinding      , "GenericTypeBinding"       )
DEF_BCSYM(TypeForwarder           , "TypeForwarder"            )
DEF_BCSYM(ExtensionCallLookupResult, "ExtensionCallLookupResult" )
DEF_BCSYM(LiftedOperatorMethod     , "LiftedOperatorMethod"      )
#endif // DEF_BCSYM

#ifdef DEF_TYPE
// ******************************************************************
// !!WARNING!! There is code that depends on the order of this table!
// !!WARNING!! see comments at the top of this file!
// ******************************************************************
//
// 



DEF_TYPE(t_UNDEF    , ""           , ""        , "$UNDEF$"       ,  0xbad, tkNone    , false  , false   , false   , false    , false    , false     )
DEF_TYPE(t_void     , ""           , ""        , "Void"          ,  0xbad, tkNone    , false  , false   , false   , false    , false    , false     )
DEF_TYPE(t_bad      , ""           , ""        , "$BAD$"         ,  0xbad, tkNone    , false  , false   , false   , false    , false    , false     )
DEF_TYPE(t_bool     , "System"     , "Boolean" , "Boolean"       ,      1, tkBOOLEAN , false  , false   , false   , false    , true     , true      )
DEF_TYPE(t_i1       , "System"     , "SByte"   , "SByte"         ,      1, tkSBYTE   , true   , true    , false   , false    , true     , true      )
DEF_TYPE(t_ui1      , "System"     , "Byte"    , "Byte"          ,      1, tkBYTE    , true   , true    , true    , false    , true     , true      )
DEF_TYPE(t_i2       , "System"     , "Int16"   , "Short"         ,      2, tkSHORT   , true   , true    , false   , false    , true     , true      )
DEF_TYPE(t_ui2      , "System"     , "UInt16"  , "UShort"        ,      2, tkUSHORT  , true   , true    , true    , false    , true     , true      )
DEF_TYPE(t_i4       , "System"     , "Int32"   , "Integer"       ,      4, tkINTEGER , true   , true    , false   , false    , true     , true      )
DEF_TYPE(t_ui4      , "System"     , "UInt32"  , "UInteger"      ,      4, tkUINTEGER, true   , true    , true    , false    , true     , true      )
DEF_TYPE(t_i8       , "System"     , "Int64"   , "Long"          ,      8, tkLONG    , true   , true    , false   , false    , true     , true      )
DEF_TYPE(t_ui8      , "System"     , "UInt64"  , "ULong"         ,      8, tkULONG   , true   , true    , true    , false    , true     , true      )
DEF_TYPE(t_decimal  , "System"     , "Decimal" , "Decimal"       ,     16, tkDECIMAL , true   , false   , false   , false    , true     , true      )
DEF_TYPE(t_single   , "System"     , "Single"  , "Single"        ,      4, tkSINGLE  , true   , false   , false   , false    , true     , true      )
DEF_TYPE(t_double   , "System"     , "Double"  , "Double"        ,      8, tkDOUBLE  , true   , false   , false   , false    , true     , true      )
DEF_TYPE(t_date     , "System"     , "DateTime", "Date"          ,      8, tkDATE    , false  , false   , false   , false    , true     , true      )
DEF_TYPE(t_char     , "System"     , "Char"    , "Char"          ,      2, tkCHAR    , false  , false   , false   , false    , true     , true      )
DEF_TYPE(t_string   , "System"     , "String"  , "String"        ,  0xbad, tkSTRING  , false  , false   , false   , true     , true     , true      )
DEF_TYPE(t_ref      , ""           , ""        , "reference type",  0xbad, tkNone    , false  , false   , false   , true     , false    , false     )
DEF_TYPE(t_struct   , ""           , ""        , "value type"    ,  0xbad, tkNone    , false  , false   , false   , false    , false    , false     )
DEF_TYPE(t_array    , ""           , ""        , "array"         ,  0xbad, tkNone    , false  , false   , false   , true     , false    , false     )
DEF_TYPE(t_ptr      , ""           , ""        , "pointer"       ,  0xbad, tkNone    , false  , false   , false   , false    , false    , false     )
// 
DEF_TYPE(t_generic  , ""           , ""        , "generic"       ,  0xbad, tkNone    , false  , false   , false   , false    , false    , false     )
DEF_TYPE(t_max      , ""           , ""        , "$MAX$"         ,  0xbad, tkNone    , false  , false   , false   , false    , false    , false     )
#endif // DEF_TYPE

#ifdef DEF_ENDKIND
DEF_ENDKIND(IF       , "End If"       , tkIF       )
DEF_ENDKIND(USING    , "End Using"    , tkUSING    )
DEF_ENDKIND(WITH     , "End With"     , tkWITH     )
DEF_ENDKIND(SELECT   , "End Select"   , tkSELECT   )
DEF_ENDKIND(STRUCTURE, "End Structure", tkSTRUCTURE)
DEF_ENDKIND(ENUM     , "End Enum"     , tkENUM     )
DEF_ENDKIND(SUB      , "End Sub"      , tkSUB      )
DEF_ENDKIND(FUNCTION , "End Function" , tkFUNCTION )
DEF_ENDKIND(PROPERTY , "End Property" , tkPROPERTY )
DEF_ENDKIND(OPERATOR , "End Operator" , tkOPERATOR )
#endif // DEF_ENDKIND

#ifdef DEF_TYPECHAR
DEF_TYPECHAR(chType_NONE    , ""  , t_ref    )
DEF_TYPECHAR(chType_I2      , "S" , t_i2     )
DEF_TYPECHAR(chType_U2      , "US", t_ui2    )
DEF_TYPECHAR(chType_I4      , "I" , t_i4     )
DEF_TYPECHAR(chType_sI4     , "%" , t_i4     )
DEF_TYPECHAR(chType_U4      , "UI", t_ui4    )
DEF_TYPECHAR(chType_I8      , "L" , t_i8     )
DEF_TYPECHAR(chType_sI8     , "&" , t_i8     )
DEF_TYPECHAR(chType_U8      , "UL", t_ui8    )
DEF_TYPECHAR(chType_R4      , "F" , t_single )
DEF_TYPECHAR(chType_sR4     , "!" , t_single )
DEF_TYPECHAR(chType_R8      , "R" , t_double )
DEF_TYPECHAR(chType_sR8     , "#" , t_double )
DEF_TYPECHAR(chType_String  , "$" , t_string )
DEF_TYPECHAR(chType_Decimal , "D" , t_decimal)
DEF_TYPECHAR(chType_sDecimal, "@" , t_decimal)
#endif //DEF_TYPECHAR

#ifdef DEF_OPERATOR
//                                                                          Is     Is      Is
//           Operator                CLS Name                 Token         Unary  Binary  Conversion
//           ----------------------  -----------------------  ------------  -----  ------  ----------
DEF_OPERATOR(OperatorUNDEF         , "$UNDEF$"              , tkNone      , false, false , false     )
DEF_OPERATOR(OperatorNarrow        , "op_Explicit"          , tkCTYPE     , true , false , true      )
DEF_OPERATOR(OperatorWiden         , "op_Implicit"          , tkCTYPE     , true , false , true      )
DEF_OPERATOR(OperatorIsTrue        , "op_True"              , tkISTRUE    , true , false , false     )
DEF_OPERATOR(OperatorIsFalse       , "op_False"             , tkISFALSE   , true , false , false     )
DEF_OPERATOR(OperatorNegate        , "op_UnaryNegation"     , tkMinus     , true , false , false     )
DEF_OPERATOR(OperatorNot           , "op_OnesComplement"    , tkNOT       , true , false , false     )
DEF_OPERATOR(OperatorUnaryPlus     , "op_UnaryPlus"         , tkPlus      , true , false , false     )
DEF_OPERATOR(OperatorPlus          , "op_Addition"          , tkPlus      , false, true  , false     )
DEF_OPERATOR(OperatorMinus         , "op_Subtraction"       , tkMinus     , false, true  , false     )
DEF_OPERATOR(OperatorMultiply      , "op_Multiply"          , tkMult      , false, true  , false     )
DEF_OPERATOR(OperatorDivide        , "op_Division"          , tkDiv       , false, true  , false     )
DEF_OPERATOR(OperatorPower         , "op_Exponent"          , tkPwr       , false, true  , false     )
DEF_OPERATOR(OperatorIntegralDivide, "op_IntegerDivision"   , tkIDiv      , false, true  , false     )
DEF_OPERATOR(OperatorConcatenate   , "op_Concatenate"       , tkConcat    , false, true  , false     )
DEF_OPERATOR(OperatorShiftLeft     , "op_LeftShift"         , tkShiftLeft , false, true  , false     )
DEF_OPERATOR(OperatorShiftRight    , "op_RightShift"        , tkShiftRight, false, true  , false     )
DEF_OPERATOR(OperatorModulus       , "op_Modulus"           , tkMOD       , false, true  , false     )
DEF_OPERATOR(OperatorOr            , "op_BitwiseOr"         , tkOR        , false, true  , false     )
DEF_OPERATOR(OperatorXor           , "op_ExclusiveOr"       , tkXOR       , false, true  , false     )
DEF_OPERATOR(OperatorAnd           , "op_BitwiseAnd"        , tkAND       , false, true  , false     )
DEF_OPERATOR(OperatorLike          , "op_Like"              , tkLIKE      , false, true  , false     )
DEF_OPERATOR(OperatorEqual         , "op_Equality"          , tkEQ        , false, true  , false     )
DEF_OPERATOR(OperatorNotEqual      , "op_Inequality"        , tkNE        , false, true  , false     )
DEF_OPERATOR(OperatorLess          , "op_LessThan"          , tkLT        , false, true  , false     )
DEF_OPERATOR(OperatorLessEqual     , "op_LessThanOrEqual"   , tkLE        , false, true  , false     )
DEF_OPERATOR(OperatorGreaterEqual  , "op_GreaterThanOrEqual", tkGE        , false, true  , false     )
DEF_OPERATOR(OperatorGreater       , "op_GreaterThan"       , tkGT        , false, true  , false     )
DEF_OPERATOR(OperatorMAXVALID      , "$MAXVALID$"           , tkNone      , false, false , false     )
DEF_OPERATOR(OperatorNot2          , "op_LogicalNot"        , tkNOT       , true , false , false     )
DEF_OPERATOR(OperatorOr2           , "op_LogicalOr"         , tkOR        , false, true  , false     )
DEF_OPERATOR(OperatorAnd2          , "op_LogicalAnd"        , tkAND       , false, true  , false     )
DEF_OPERATOR(OperatorShiftLeft2    , "op_UnsignedLeftShift" , tkShiftLeft , false, true  , false     )
DEF_OPERATOR(OperatorShiftRight2   , "op_UnsignedRightShift", tkShiftRight, false, true  , false     )
DEF_OPERATOR(OperatorMAX           , "$MAX$"                , tkNone      , false, false , false     )
#endif

#ifdef DEF_CLRSYM
//                                                                                                                         In      Minimum
//         Type                                      CLR Scope                          CLR Name                           NETCF?  Runtime Version
//         ----------------------------------------  ---------------------------------  ---------------------------------  ------  -----------------
DEF_CLRSYM(ExceptionType                           , "System"                         , "Exception"                      , true  , RuntimeVersion1  )
DEF_CLRSYM(DelegateType                            , "System"                         , "Delegate"                       , true  , RuntimeVersion1  )
DEF_CLRSYM(MultiCastDelegateType                   , "System"                         , "MultiCastDelegate"              , true  , RuntimeVersion1  )
DEF_CLRSYM(TypeType                                , "System"                         , "Type"                           , true  , RuntimeVersion1  )
DEF_CLRSYM(ValueTypeType                           , "System"                         , "ValueType"                      , true  , RuntimeVersion1  )
DEF_CLRSYM(EnumType                                , "System"                         , "Enum"                           , true  , RuntimeVersion1  )
DEF_CLRSYM(ArrayType                               , "System"                         , "Array"                          , true  , RuntimeVersion1  )
DEF_CLRSYM(ObjectType                              , "System"                         , "Object"                         , true  , RuntimeVersion1  )
DEF_CLRSYM(AsyncCallbackType                       , "System"                         , "AsyncCallback"                  , true  , RuntimeVersion1  )
DEF_CLRSYM(IAsyncResultType                        , "System"                         , "IAsyncResult"                   , true  , RuntimeVersion1  )
DEF_CLRSYM(AttributeType                           , "System"                         , "Attribute"                      , true  , RuntimeVersion1  )
DEF_CLRSYM(AttributeUsageAttributeType             , "System"                         , "AttributeUsageAttribute"        , true  , RuntimeVersion1  )
DEF_CLRSYM(VoidType                                , "System"                         , "Void"                           , true  , RuntimeVersion1  )
DEF_CLRSYM(IntPtrType                              , "System"                         , "IntPtr"                         , true  , RuntimeVersion1  )
DEF_CLRSYM(UIntPtrType                             , "System"                         , "UIntPtr"                        , true  , RuntimeVersion1  )
DEF_CLRSYM(IDisposableType                         , "System"                         , "IDisposable"                    , true  , RuntimeVersion1  )
DEF_CLRSYM(DBNullType                              , "System"                         , "DBNull"                         , true  , RuntimeVersion1  )
DEF_CLRSYM(EventArgsType                           , "System"                         , "EventArgs"                      , true  , RuntimeVersion1  )
DEF_CLRSYM(CLSCompliantAttributeType               , "System"                         , "CLSCompliantAttribute"          , true  , RuntimeVersion1  )
DEF_CLRSYM(TypedReferenceType                      , "System"                         , "TypedReference"                 , true  , RuntimeVersion1  )
DEF_CLRSYM(WeakReferenceType                       , "System"                         , "WeakReference"                  , true  , RuntimeVersion1  )
DEF_CLRSYM(ArgIteratorType                         , "System"                         , "ArgIterator"                    , false , RuntimeVersion1  )
DEF_CLRSYM(RuntimeArgumentHandleType               , "System"                         , "RuntimeArgumentHandle"          , false , RuntimeVersion1  )
DEF_CLRSYM(GenericNullableType                     , "System"                         , "Nullable`1"                     , true  , RuntimeVersion2  )

DEF_CLRSYM(GenericIEquatableType                   , "System"                         , "IEquatable`1"                   , true  , RuntimeVersion2  )

DEF_CLRSYM(ArrayListType                           , "System.Collections"             , "ArrayList"                      , true  , RuntimeVersion1  )
DEF_CLRSYM(IEnumerableType                         , "System.Collections"             , "IEnumerable"                    , true  , RuntimeVersion1  )
DEF_CLRSYM(IEnumeratorType                         , "System.Collections"             , "IEnumerator"                    , true  , RuntimeVersion1  )

DEF_CLRSYM(ICollectionType                         , "System.Collections"             , "ICollection"                    , true  , RuntimeVersion1  )
DEF_CLRSYM(IDictionaryType                         , "System.Collections"             , "IDictionary"                    , true  , RuntimeVersion1  )
DEF_CLRSYM(DictionaryEntryType                     , "System.Collections"             , "DictionaryEntry"                , true  , RuntimeVersion1  )
DEF_CLRSYM(IListType                               , "System.Collections"             , "IList"                          , true  , RuntimeVersion1  )
DEF_CLRSYM(GenericIEnumerableType                  , "System.Collections.Generic"     , "IEnumerable`1"                  , true  , RuntimeVersion2  )

DEF_CLRSYM(GenericIEnumeratorType                  , "System.Collections.Generic"     , "IEnumerator`1"                  , true  , RuntimeVersion2  )

DEF_CLRSYM(GenericICollectionType                  , "System.Collections.Generic"     , "ICollection`1"                  , true  , RuntimeVersion2  )
DEF_CLRSYM(GenericIDictionaryType                  , "System.Collections.Generic"     , "IDictionary`2"                  , true  , RuntimeVersion2  )
DEF_CLRSYM(GenericKeyValuePairType                 , "System.Collections.Generic"     , "KeyValuePair`2"                 , true  , RuntimeVersion2  )
DEF_CLRSYM(GenericIListType                        , "System.Collections.Generic"     , "IList`1"                        , true  , RuntimeVersion2  )
DEF_CLRSYM(GenericIReadOnlyListType                , "System.Collections.Generic"     , "IReadOnlyList`1"                , true  , RuntimeVersion2  )
DEF_CLRSYM(GenericIReadOnlyCollectionType          , "System.Collections.Generic"     , "IReadOnlyCollection`1"          , true  , RuntimeVersion2  )
DEF_CLRSYM(GenericListType                         , "System.Collections.Generic"     , "List`1"                         , true  , RuntimeVersion2  )
DEF_CLRSYM(GenericCollectionType                   , "System.Collections.ObjectModel" , "Collection`1"                   , true  , RuntimeVersion2  )
DEF_CLRSYM(GenericReadOnlyCollectionType           , "System.Collections.ObjectModel" , "ReadOnlyCollection`1"           , true  , RuntimeVersion2  )
DEF_CLRSYM(DebuggerNonUserCodeAttributeType        , "System.Diagnostics"             , "DebuggerNonUserCodeAttribute"   , false , RuntimeVersion2  )
DEF_CLRSYM(DebuggerStepThroughAttributeType        , "System.Diagnostics"             , "DebuggerStepThroughAttribute"   , false , RuntimeVersion1  )
DEF_CLRSYM(AssemblyDelaySignAttributeType          , "System.Reflection"              , "AssemblyDelaySignAttribute"     , true  , RuntimeVersion1  )
DEF_CLRSYM(DebuggerDisplayAttributeType            , "System.Diagnostics"             , "DebuggerDisplayAttribute"       , false , RuntimeVersion2  )
DEF_CLRSYM(AssemblyKeyFileAttributeType            , "System.Reflection"              , "AssemblyKeyFileAttribute"       , true  , RuntimeVersion1  )
DEF_CLRSYM(AssemblyKeyNameAttributeType            , "System.Reflection"              , "AssemblyKeyNameAttribute"       , true  , RuntimeVersion1  )
DEF_CLRSYM(AssemblyVersionAttributeType            , "System.Reflection"              , "AssemblyVersionAttribute"       , true  , RuntimeVersion1  )
DEF_CLRSYM(AssemblyFlagsAttributeType              , "System.Reflection"              , "AssemblyFlagsAttribute"         , true  , RuntimeVersion1  )
DEF_CLRSYM(MissingType                             , "System.Reflection"              , "Missing"                        , true  , RuntimeVersion1  )

DEF_CLRSYM(FieldInfoType                           , "System.Reflection"              , "FieldInfo"                      , true  , RuntimeVersion1  )
DEF_CLRSYM(MethodInfoType                          , "System.Reflection"              , "MethodInfo"                     , true  , RuntimeVersion1  )
DEF_CLRSYM(MethodBaseType                          , "System.Reflection"              , "MethodBase"                     , true  , RuntimeVersion1  )
DEF_CLRSYM(ConstructorInfoType                     , "System.Reflection"              , "ConstructorInfo"                , true  , RuntimeVersion1  )

DEF_CLRSYM(CompilationRelaxationsAttributeType     , "System.Runtime.CompilerServices", "CompilationRelaxationsAttribute", false , RuntimeVersion1  )
DEF_CLRSYM(CompilerGeneratedAttributeType          , "System.Runtime.CompilerServices", "CompilerGeneratedAttribute"     , false , RuntimeVersion2  )
DEF_CLRSYM(RuntimeCompatibilityAttributeType       , "System.Runtime.CompilerServices", "RuntimeCompatibilityAttribute"  , false , RuntimeVersion2  )
DEF_CLRSYM(AsyncStateMachineAttributeType          , "System.Runtime.CompilerServices", "AsyncStateMachineAttribute"     , true , RuntimeVersion2	)
DEF_CLRSYM(IteratorStateMachineAttributeType       , "System.Runtime.CompilerServices", "IteratorStateMachineAttribute"  , true , RuntimeVersion2	)


DEF_CLRSYM(DispatchWrapperType                     , "System.Runtime.InteropServices" , "DispatchWrapper"                , false , RuntimeVersion1  )
DEF_CLRSYM(UnknownWrapperType                      , "System.Runtime.InteropServices" , "UnknownWrapper"                 , false , RuntimeVersion1  )
DEF_CLRSYM(DefaultCharSetAttributeType             , "System.Runtime.InteropServices" , "DefaultCharSetAttribute"        , false , RuntimeVersion2  )
DEF_CLRSYM(MarshalType                             , "System.Runtime.InteropServices" , "Marshal"                        , false , RuntimeVersion2  )
DEF_CLRSYM(SecuritySafeCriticalAttributeType       , "System.Security"                , "SecuritySafeCriticalAttribute"  , true  , RuntimeVersion2  )
DEF_CLRSYM(SecurityCriticalAttributeType           , "System.Security"                , "SecurityCriticalAttribute"      , true  , RuntimeVersion2  )
DEF_CLRSYM(SecurityAttributeType                   , "System.Security.Permissions"    , "SecurityAttribute"              , false , RuntimeVersion1  )
DEF_CLRSYM(SecurityActionType                      , "System.Security.Permissions"    , "SecurityAction"                 , true  , RuntimeVersion1  )
DEF_CLRSYM(WindowsRuntimeMarshalType               , "System.Runtime.InteropServices.WindowsRuntime" , "WindowsRuntimeMarshal"           , false , RuntimeVersion2  )
DEF_CLRSYM(EventRegistrationTokenType              , "System.Runtime.InteropServices.WindowsRuntime" , "EventRegistrationToken"          , false , RuntimeVersion2  )
DEF_CLRSYM(EventRegistrationTokenTableType         , "System.Runtime.InteropServices.WindowsRuntime" , "EventRegistrationTokenTable`1"   , false , RuntimeVersion2  )

// The minimum runtime for MarshalAsType is set to 2.0 because it has differnt minimum runtime versions for the compact framework and
// and the regular framework, so we choose 2.0 as the "lowest common demoninator"
// All this effects is an assert, so we should be ok.

DEF_CLRSYM(MarshalAsType                           , "System.Runtime.InteropServices" , "MarshalAsAttribute"             , true  , RuntimeVersion2  )

DEF_CLRSYM(GenericFuncType                         , "System"                         , "Func`1"                         , false , RuntimeVersion2  )
DEF_CLRSYM(GenericFunc2Type                        , "System"                         , "Func`2"                         , false , RuntimeVersion2  )
DEF_CLRSYM(GenericFunc3Type                        , "System"                         , "Func`3"                         , false , RuntimeVersion2  )
DEF_CLRSYM(GenericFunc4Type                        , "System"                         , "Func`4"                         , false , RuntimeVersion2  )
DEF_CLRSYM(GenericFunc5Type                        , "System"                         , "Func`5"                         , false , RuntimeVersion2  )

DEF_CLRSYM(ActionType                              , "System"                         , "Action"                         , false , RuntimeVersion2  )
DEF_CLRSYM(TaskType                                , "System.Threading.Tasks"         , "Task"                           , false , RuntimeVersion2  )
DEF_CLRSYM(GenericTaskType                         , "System.Threading.Tasks"         , "Task`1"                         , false , RuntimeVersion2  )
DEF_CLRSYM(AsyncTaskMethodBuilderType              , "System.Runtime.CompilerServices", "AsyncTaskMethodBuilder"         , false , RuntimeVersion2  )
DEF_CLRSYM(GenericAsyncTaskMethodBuilderType       , "System.Runtime.CompilerServices", "AsyncTaskMethodBuilder`1"       , false , RuntimeVersion2  )
DEF_CLRSYM(AsyncVoidMethodBuilderType              , "System.Runtime.CompilerServices", "AsyncVoidMethodBuilder"         , false , RuntimeVersion2  )
DEF_CLRSYM(INotifyCompletionType                   , "System.Runtime.CompilerServices", "INotifyCompletion"              , false , RuntimeVersion2  )
DEF_CLRSYM(ICriticalNotifyCompletionType           , "System.Runtime.CompilerServices", "ICriticalNotifyCompletion"      , false , RuntimeVersion2  )
DEF_CLRSYM(IAsyncStateMachineType                  , "System.Runtime.CompilerServices", "IAsyncStateMachine"             , false , RuntimeVersion2  )

DEF_CLRSYM(IAsyncActionType                        , "Windows.Foundation"             , "IAsyncAction"                   , false , RuntimeVersion2  )
DEF_CLRSYM(IAsyncActionWithProgressType            , "Windows.Foundation"             , "IAsyncActionWithProgress`1"     , false , RuntimeVersion2  )
DEF_CLRSYM(IAsyncOperationType                     , "Windows.Foundation"             , "IAsyncOperation`1"              , false , RuntimeVersion2  )
DEF_CLRSYM(IAsyncOperationWithProgressType         , "Windows.Foundation"             , "IAsyncOperationWithProgress`2"  , false , RuntimeVersion2  )

DEF_CLRSYM(GenericTuple1Type                       , "System"                         , "Tuple`1"                        , false , RuntimeVersion2  )
DEF_CLRSYM(GenericTuple2Type                       , "System"                         , "Tuple`2"                        , false , RuntimeVersion2  )
DEF_CLRSYM(GenericTuple3Type                       , "System"                         , "Tuple`3"                        , false , RuntimeVersion2  )
DEF_CLRSYM(GenericTuple4Type                       , "System"                         , "Tuple`4"                        , false , RuntimeVersion2  )
DEF_CLRSYM(GenericTuple5Type                       , "System"                         , "Tuple`5"                        , false , RuntimeVersion2  )
DEF_CLRSYM(GenericTuple6Type                       , "System"                         , "Tuple`6"                        , false , RuntimeVersion2  )
DEF_CLRSYM(GenericTuple7Type                       , "System"                         , "Tuple`7"                        , false , RuntimeVersion2  )
DEF_CLRSYM(GenericTuple8Type                       , "System"                         , "Tuple`8"                        , false , RuntimeVersion2  )

DEF_CLRSYM(IsVolatileType                          , "System.Runtime.CompilerServices", "IsVolatile"                     , true  , RuntimeVersion1  )

// NOTE The following symbols currently do not belong to mscorlib.
// Anyone introducing new symbols below this line should do the additional work in metaimport.cpp, compiler.h, compilerproject.cpp
// to make sure we set the symbols to NULL when decompiling metadata reference.
//
// You MUST ALSO update FxTypeTable.h to do the right decompilation!

DEF_CLRSYM(IQueryableType                          , "System.Linq"                    , "IQueryable"                     , false , RuntimeVersion2  )
DEF_CLRSYM(GenericIQueryableType                   , "System.Linq"                    , "IQueryable`1"                   , false , RuntimeVersion2  )
DEF_CLRSYM(LinqEnumerableType                      , "System.Linq"                    , "Enumerable"                     , false , RuntimeVersion2  )
DEF_CLRSYM(ExpressionType                          , "System.Linq.Expressions"        , "Expression"                     , false , RuntimeVersion2  )
DEF_CLRSYM(ElementInitType                         , "System.Linq.Expressions"        , "ElementInit"                    , false , RuntimeVersion2  )
DEF_CLRSYM(NewExpressionType                       , "System.Linq.Expressions"        , "NewExpression"                  , false , RuntimeVersion2  )
DEF_CLRSYM(LambdaExpressionType                    , "System.Linq.Expressions"        , "LambdaExpression"               , false , RuntimeVersion2  )
DEF_CLRSYM(GenericExpressionType                   , "System.Linq.Expressions"        , "Expression`1"                   , false , RuntimeVersion2  )
DEF_CLRSYM(ParameterExpressionType                 , "System.Linq.Expressions"        , "ParameterExpression"            , false , RuntimeVersion2  )
DEF_CLRSYM(MemberBindingType                       , "System.Linq.Expressions"        , "MemberBinding"                  , false , RuntimeVersion2  )

// End Note

DEF_CLRSYM(TypeMax                                 , ""                               , ""                               , true  , RuntimeVersion1  )
#endif
