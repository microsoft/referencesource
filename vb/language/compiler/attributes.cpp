//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Code to handle emitting and importing custom attributes.  In
//  particular, this file implements methods of the AttrVals and
//  Attribute classes.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#if DEBUG
#define ATTR_USAGE_ENTRY(IsAllowMultiple, IsInheritable)    \
                (IsAllowMultiple),                          \
                (IsInheritable),
#else
#define ATTR_USAGE_ENTRY(IsAllowMultiple, IsInheritable)
#endif

#define ATTRIBUTE_ENTRY_NULL                \
    {                                       \
        attrkindNil,                        \
        NULL,                               \
        0,                                  \
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}   \
    }

//============================================================================
// g_rgAttributes identifies all of the well-known custom attributes.
//
// Each entry contains the name of the attribute and the list of possible
// constructors (AttrIdentity Structs). The entries must be alphabetically sorted by the
// fully qualified name, and the list of constructors must end with an entry that has
// attrkindNil as the m_attrKind field.
//
// Each entry contains information about its attached AttributeUsageAttribute,
// which says how the attribute can be used.  (It would be nice to gather these
// data programatically, or at least assert at runtime that we have them right.)
//
// Attributes that only specify ValidOn default to AllowMultiple = false
// and Inherited = true.
//
// NOTE: Any well-known custom attribute with Inherited = true must be reflected
// in AttrVals::InheritFromSymbol.
// NOTE: The number of signatures permitted on an attribute index is driven by
// MaxSignatureLength - if any of the pre-defined attributes have more then
// this amount, you need to adjust MaxSignatureLength accordingly.
//
//============================================================================
const bool DoCopyForNoPiaEmbeddedSymbols = true;
const bool DoNotCopyForNoPiaEmbeddedSymbols = false;


AttrIndex g_rgAttributes[] =
{
    {
        //---------------------------------------------------------------------
        // Microsoft.VisualBasic.ComClassAttribute:
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple=false, Inherited=false)
        //---------------------------------------------------------------------
        COMCLASSATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindComClass_Void,
                COMCLASSATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            {
                attrkindComClass_String,
                COMCLASSATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            {
                attrkindComClass_String_String,
                COMCLASSATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING, ELEMENT_TYPE_STRING }
            },

            {
                attrkindComClass_String_String_String,
                COMCLASSATTRIBUTE,
                6,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 3, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING, ELEMENT_TYPE_STRING, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // Microsoft.VisualBasic.CompilerServices.DesignerGenerated
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = false)
        //
        // Classes marked with this attribute will be treated differently so that
        // designer code works.
        //---------------------------------------------------------------------
        DESIGNERGENERATEDATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDesignerGenerated_Void,
                DESIGNERGENERATEDATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // Microsoft.VisualBasic.CompilerServices.OptionCompareAttribute
        //
        // AttributeUsage(ValidOn = Parameter, AllowMultiple = false, Inherited = false)
        //
        // We mark VB function arguments with this custom attribute so that
        // the current Option Compare setting is used when no value is specified
        //---------------------------------------------------------------------
        OPTIONCOMPAREATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindOptionCompare_Void,
                OPTIONCOMPAREATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // Microsoft.VisualBasic.CompilerServices.StandardModule
        //
        // AttributeUsage(ValidOn = All, AllowMultiple = false, Inherited = false)
        //
        // This one's a little weird, too.  We mark VB "Module" constructs
        // with this custom attribute so that, on import, the module's
        // contents gets promoted into the default namespace.  We need to use
        // an attribute for this because COM+ has no notion of a module.
        //
        //---------------------------------------------------------------------
        STANDARDMODULEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindStandardModule_Void,
                STANDARDMODULEATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // Microsoft.VisualBasic.Embedded
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        EMBEDDEDATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindEmbedded_Void,
                EMBEDDEDATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // Microsoft.VisualBasic.HideModuleName
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        HIDEMOUDLENAMEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindHideModuleName_Void,
                HIDEMOUDLENAMEATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },


    {
        //---------------------------------------------------------------------
        // System.Microsoft.VisualBasic.MyGroupCollection
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        MYGROUPCOLLECTIONATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindMyGroupCollection_String_String_String_String,
                MYGROUPCOLLECTIONATTRIBUTE,
                7,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 4, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING,
                                ELEMENT_TYPE_STRING, ELEMENT_TYPE_STRING, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.AttributeUsageAttribute
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = true)
        //---------------------------------------------------------------------
        ATTRIBUTEUSAGEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, true)
        {
            {
                attrkindAttributeUsage_AttributeTargets_NamedProps,
                ATTRIBUTEUSAGEATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_U }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.CLSCompliantAttribute
        //
        // AttributeUsage(ValidOn = All, AllowMultiple = false, Inherited = true)
        //---------------------------------------------------------------------
        CLSCOMPLIANTATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, true)
        {
            {
                attrkindCLSCompliant_Bool,
                CLSCOMPLIANTATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_BOOLEAN }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.ComponentModel.CategoryAttribute
        //
        // AttributeUsage(ValidOn = All, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        CATEGORYATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindCategory_Void,
                CATEGORYATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            {
                attrkindCategory_String,
                CATEGORYATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },
    {
        //---------------------------------------------------------------------
        // System.ComponentModel.DefaultEventAttribute
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DEFAULTEVENTATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDefaultEvent_String,
                DEFAULTEVENTATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.ComponentModel.DefaultValueAttribute
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DEFAULTVALUEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDefaultValue_Bool,
                DEFAULTVALUEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_BOOLEAN }
            },

            {
                attrkindDefaultValue_Byte,
                DEFAULTVALUEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_U1 }
            },

            {
                attrkindDefaultValue_Char,
                DEFAULTVALUEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_CHAR }
            },

            {
                attrkindDefaultValue_Double,
                DEFAULTVALUEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_R8 }
            },

            {
                attrkindDefaultValue_Int16,
                DEFAULTVALUEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2 }
            },

            {
                attrkindDefaultValue_Int32,
                DEFAULTVALUEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I4 }
            },

            {
                attrkindDefaultValue_Int64,
                DEFAULTVALUEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I8 }
            },

            {
                attrkindDefaultValue_Object,
                DEFAULTVALUEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_OBJECT }
            },

            {
                attrkindDefaultValue_Single,
                DEFAULTVALUEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_R4 }
            },

            {
                attrkindDefaultValue_String,
                DEFAULTVALUEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            {
                attrkindDefaultValue_Type_String,
                DEFAULTVALUEATTRIBUTE,
                6,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE, ELEMENT_TYPE_STRING }
            },


            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.ComponentModel.Design.HelpKeywordAttribute
        //
        // AttributeUsage(ValidOn = All, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        HELPKEYWORDATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindHelpKeyword_Void,
                HELPKEYWORDATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            {
                attrkindHelpKeyword_String,
                HELPKEYWORDATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            {
                attrkindHelpKeyword_Type,
                HELPKEYWORDATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.ComponentModel.DesignerCategoryAttribute
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = true)
        //
        // The next one is a little weird.  Whenever we detect a class that
        // has this attribute, we'll report the attribute CTOR's arg value
        // to the WFC designer.  (This CTOR takes no parameters, but the
        // next one takes a String parameter.  We need to recognize and
        // report both.)
        //
        //---------------------------------------------------------------------
        DESIGNERCATEGORYATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, true)
        {
            {
                attrkindDesignerCategory_Void,
                DESIGNERCATEGORYATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            {
                //
                // The CTOR-takes-String version of the WFC designer attribute.
                //
                attrkindDesignerCategory_String,
                DESIGNERCATEGORYATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.ComponentModel.DesignerSerializationVisibilityAttribute
        //
        // AttributeUsage(ValidOn = ???, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DESIGNERSERIALIZATIONVISIBILITYATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDesignerSerializationVisibilityAttribute_DesignerSerializationVisibilityType,
                DESIGNERSERIALIZATIONVISIBILITYATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_U }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.ComponentModel.EditorBrowsableAttribute
        //
        // AttributeUsage(ValidOn = All, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        EDITORBROWSABLEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindEditorBrowsable_Void,
                EDITORBROWSABLEATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            {
                attrkindEditorBrowsable_EditorBrowsableState,
                EDITORBROWSABLEATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Diagnostics.ConditionalAttribute
        //
        // AttributeUsage(ValidOn = Method, AllowMultiple = true, Inherited = false)
        //---------------------------------------------------------------------
        CONDITIONALATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(true, false)
        {
            {
                attrkindConditional_String,
                CONDITIONALATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Diagnostics.DebuggableAttribute
        //
        // AttributeUsage(ValidOn = AssemblyOrModule, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DEBUGGABLEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDebuggable_DebuggingModes,
                DEBUGGABLEATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4 }
            },

            {
                attrkindDebuggable_Bool_Bool,
                DEBUGGABLEATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_BOOLEAN,  ELEMENT_TYPE_BOOLEAN }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Diagnostics.DebuggerBrowsableAttribute
        //
        // AttributeUsage(ValidOn = Field | Property, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DEBUGGERBROWSABLEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDebuggerBrowsable_BrowsableState,
                DEBUGGERBROWSABLEATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_U }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Diagnostics.DebuggerDisplayAttribute
        //
        // AttributeUsage(ValidOn = Everything , AllowMultiple = true, Inherited = false)
        //---------------------------------------------------------------------
        DEBUGGERDISPLAYATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(true, false)
        {
            {
                attrkindDebuggerDisplay_String,
                DEBUGGERDISPLAYATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Diagnostics.DebuggerHiddenAttribute
        //
        // AttributeUsage(ValidOn = Constructor | Method  | Property, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DEBUGGERHIDDENATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDebuggerHidden_Void,
                DEBUGGERHIDDENATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.FlagsAttribute
        //
        // AttributeUsage(ValidOn = Enum, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        FLAGSATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindFlags_Void,
                FLAGSATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.MTAThreadAttribute
        //
        // AttributeUsage(ValidOn = Method, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        MTATHREADATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindMTAThread_Void,
                MTATHREADATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.NonSerializedAttribute
        //
        // AttributeUsage(ValidOn = Field, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        NONSERIALIZEDATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindNonSerialized_Void,
                NONSERIALIZEDATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.ObsoleteAttribute
        //
        // AttributeUsage(ValidOn = All, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        OBSOLETEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindObsolete_Void,
                OBSOLETEATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            {
                attrkindObsolete_String,
                OBSOLETEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            {
                attrkindObsolete_String_Bool,
                OBSOLETEATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING, ELEMENT_TYPE_BOOLEAN }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

	{
        //---------------------------------------------------------------------
        // System.ParamArrayAttribute
        //
        // AttributeUsage(ValidOn = Parameter, AllowMultiple = false, Inherited = true)
        //---------------------------------------------------------------------
        PARAMARRAYATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, true)
        {
            {
                attrkindParamArray_Void,
                PARAMARRAYATTRIBUTE,
                3,
                INTEROP_PARAMARRAY_SIG
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Reflection.AssemblyDelaySignAttribute
        //
        // AttributeUsage(ValidOn = Assembly, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        ASSEMBLYDELAYSIGNATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindAssemblyDelaySign_Bool,
                ASSEMBLYDELAYSIGNATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_BOOLEAN }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Reflection.AssemblyFlagsAttribute
        //
        // AttributeUsage(ValidOn = Assembly, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        ASSEMBLYFLAGSATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindAssemblyFlags_Int,
                ASSEMBLYFLAGSATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I4}
            },

            {
                attrkindAssemblyFlags_UInt,
                ASSEMBLYFLAGSATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_U4}
            },
            
            {
                attrkindAssemblyFlags_AssemblyNameFlags,
                ASSEMBLYFLAGSATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4}
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Reflection.AssemblyKeyFileAttribute
        //
        // AttributeUsage(ValidOn = Assembly, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        ASSEMBLYKEYFILEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindAssemblyKeyFile_String,
                ASSEMBLYKEYFILEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Reflection.AssemblyKeyNameAttribute
        //
        // AttributeUsage(ValidOn = Assembly, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        ASSEMBLYKEYNAMEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindAssemblyKeyName_String,
                ASSEMBLYKEYNAMEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Reflection.AssemblyVersionAttribute
        //
        // AttributeUsage(ValidOn = Assembly, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        ASSEMBLYVERSIONATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindAssemblyVersion_String,
                ASSEMBLYVERSIONATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Reflection.DefaultMemberAttribute
        //
        // AttributeUsage(ValidOn = Class | Struct, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DEFAULTMEMBERATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDefaultMember_String,
                DEFAULTMEMBERATTRIBUTE,
                4,
                INTEROP_DEFAULTMEMBER_SIG
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.AccessedThroughPropertyAttribute
        //
        // AttributeUsage(ValidOn = Field, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        ACCESSEDTHROUGHPROPERTYATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindAccessedThroughProperty_String,
                ACCESSEDTHROUGHPROPERTYATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.CallerFilePathAttribute
        //
        // AttributeUsage(ValidOn = Module | Class, AllowMultiple = false, Inherited = false)
        //
        // Marks a class or module as extension container.
        //
        //---------------------------------------------------------------------
        CALLERFILEPATHATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindCallerFilePath_Void,
                CALLERFILEPATHATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.CallerLineNumberAttribute
        //
        // AttributeUsage(ValidOn = Module | Class, AllowMultiple = false, Inherited = false)
        //
        // Marks a class or module as extension container.
        //
        //---------------------------------------------------------------------
        CALLERLINENUMBERATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindCallerLineNumber_Void,
                CALLERLINENUMBERATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },


    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.CallerMemberNameAttribute
        //
        // AttributeUsage(ValidOn = Module | Class, AllowMultiple = false, Inherited = false)
        //
        // Marks a class or module as extension container.
        //
        //---------------------------------------------------------------------
        CALLERMEMBERNAMEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindCallerMemberName_Void,
                CALLERMEMBERNAMEATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.CompilationRelaxationsAttribute
        //
        // AttributeUsage(ValidOn = Class | Module | Assembly | Method, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        COMPILATIONRELAXATIONATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindCompilationRelaxations_Int32,
                COMPILATIONRELAXATIONATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I4 }
            },
            {
                attrkindCompilationRelaxations_Enum,
                COMPILATIONRELAXATIONATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_U }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.DateTimeConstantAttribute
        //
        // AttributeUsage(ValidOn = Field and Parameter, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DATETIMECONSTANTATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDateTimeConstant_Int64,
                DATETIMECONSTANTATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I8 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.DecimalConstantAttribute
        //
        // AttributeUsage(ValidOn = Field and Parameter, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DECIMALCONSTANTATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDecimalConstant_Decimal,
                DECIMALCONSTANTATTRIBUTE,
                8,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 5, ELEMENT_TYPE_VOID, ELEMENT_TYPE_U1, ELEMENT_TYPE_U1, ELEMENT_TYPE_U4, ELEMENT_TYPE_U4, ELEMENT_TYPE_U4 }
            },

            {
                attrkindDecimalConstant_DecimalInt,
                DECIMALCONSTANTATTRIBUTE,
                8,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 5, ELEMENT_TYPE_VOID, ELEMENT_TYPE_U1, ELEMENT_TYPE_U1, ELEMENT_TYPE_I4, ELEMENT_TYPE_I4, ELEMENT_TYPE_I4 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.Extension
        //
        // AttributeUsage(ValidOn = Module | Class, AllowMultiple = false, Inherited = false)
        //
        // Marks a class or module as extension container.
        //
        //---------------------------------------------------------------------
        EXTENSIONATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindExtension_Void,
                EXTENSIONATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.IDispatchConstantAttribute
        //
        // AttributeUsage(ValidOn = Field | Parameter, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        IDISPATCHCONSTANTATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindIDispatchConstant_void,
                IDISPATCHCONSTANTATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.InternalsVisibleToAttribute
        // AttributeUsage(ValidOn = Assembly, AllowMultiple = true, Inherited = false)
        //---------------------------------------------------------------------
        INTERNALSVISIBLETOATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(true, false)
        {
            {
                attrkindInternalsVisibleTo_String,
                INTERNALSVISIBLETOATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.IUnknownConstantAttribute
        // AttributeUsage(ValidOn = Field | Parameter, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        IUNKNOWNCONSTANTATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindIUnknownConstant_void,
                IUNKNOWNCONSTANTATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.MethodImplAttribute
        //
        // AttributeUsage(ValidOn = Constructor + Method, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        METHODIMPLATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindMethodImpl_Int16,
                METHODIMPLATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2 }
            },
            {
                attrkindMethodImpl_MethodImplOptions,
                METHODIMPLATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4}
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.CompilerServices.RuntimeCompatibilityAttribute
        //
        // AttributeUsage(ValidOn = Assembly, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        RUNTIMECOMPATIBILITYATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindRuntimeCompatibility_Void,
                RUNTIMECOMPATIBILITYATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.BestFitMappingAttribute
        //
        // AttributeUsage(ValidOn = Struct, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        BESTFITMAPPINGATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindBestFitMapping_Bool,
                BESTFITMAPPINGATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID,  ELEMENT_TYPE_BOOLEAN }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.ClassInterface
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        CLASSINTERFACEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindClassInterface_Int16,
                CLASSINTERFACEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID,  ELEMENT_TYPE_I2 }
            },

            {
                attrkindClassInterface_ClassInterfaceType,
                CLASSINTERFACEATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.CoClassAttribute
        //
        // AttributeUsage(ValidOn = Interface, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        COCLASSATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindCoClass_Type,
                COCLASSATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.ComEventInterfaceAttribute
        //
        // AttributeUsage(ValidOn = Interface, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        COMEVENTINTERFACEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindComEventInterface_Type_Type,
                COMEVENTINTERFACEATTRIBUTE,
                7,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.COMImportAttribute
        //
        // AttributeUsage(ValidOn = Class or Interface, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        COMIMPORTATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindComImport_Void,
                COMIMPORTATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.ComSourceInterfacesAttribute
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = true)
        //---------------------------------------------------------------------
        COMSOURCEINTERFACES,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, true)
        {
            {
                attrkindComSourceInterfaces_String,
                COMSOURCEINTERFACES,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            {
                attrkindComSourceInterfaces_Type,
                COMSOURCEINTERFACES,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE }
            },

            {
                attrkindComSourceInterfaces_Type_Type,
                COMSOURCEINTERFACES,
                7,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE }
            },

            {
                attrkindComSourceInterfaces_Type_Type_Type,
                COMSOURCEINTERFACES,
                9,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 3, ELEMENT_TYPE_VOID, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE }
            },

            {
                attrkindComSourceInterfaces_Type_Type_Type_Type,
                COMSOURCEINTERFACES,
                11,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 4, ELEMENT_TYPE_VOID, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE, ELEMENT_TYPE_CLASS, SERIALIZATION_TYPE_TYPE }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.ComVisibleAttribute
        //
        // AttributeUsage(ValidOn = Type, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        COMVISIBLEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindCOMVisible_Bool,
                COMVISIBLEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_BOOLEAN }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.DefaultParameterValueAttribute
        //
        // AttributeUsage(ValidOn = Parameter, AllowMultiple=false, Inherited=false)
        //---------------------------------------------------------------------
        DEFAULTPARAMETERVALUEATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols, 
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDefaultParameterValue_Object,
                DEFAULTPARAMETERVALUEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_OBJECT }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.DispIdAttribute
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DISPIDATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols, // DispIdAttribute is copied separately
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDispId_Int32,
                DISPIDATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I4 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.DllImportAttribute
        //
        // AttributeUsage(ValidOn = Method, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DLLIMPORTATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDllImport_String,
                DLLIMPORTATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.FieldOffsetAttribute
        //
        // AttributeUsage(ValidOn = Field, AllowMultiple=false, Inherited=false)
        //---------------------------------------------------------------------
        FIELDOFFSETATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindFieldOffset_Int32,
                FIELDOFFSETATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I4 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.GuidAttribute
        //
        // AttributeUsage(ValidOn = Assembly + Class + Struct + Enum + Interface + Delegate, AllowMultiple=false, Inherited=false)
        //---------------------------------------------------------------------
        GUIDATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,  // GuidAttribute is copied separately
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindGuid_String,
                GUIDATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },


            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.ImportedFromTypeLibAttribute
        //
        // AttributeUsage(ValidOn = Assembly, Inherited=false)
        //----------------------------------------------------------------------
        IMPORTEDFROMTYPELIBATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindImportedFromTypeLib_String,
                IMPORTEDFROMTYPELIBATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.InAttribute
        //
        // AttributeUsage(ValidOn = Method, AllowMultiple=false, Inherited=false)
        //---------------------------------------------------------------------
        INATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindIn_void,
                INATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.InterfaceTypeAttribute
        //
        // AttributeUsage(ValidOn = Interface, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        INTERFACETYPEATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindInterfaceType_Int16,
                INTERFACETYPEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2 }
            },

            {
                attrkindInterfaceType_ComInterfaceType,
                INTERFACETYPEATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_U }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.LCIDConversionAttribute
        //
        // AttributeUsage(ValidOn = Method, AllowMultiple=false, Inherited=false)
        //---------------------------------------------------------------------
        LCIDCONVERSIONATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols, // LCIDConversionAttribute is copied separately
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindLCIDConversion_Int32,
                LCIDCONVERSIONATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I4 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.MarshalAsAttribute
        //
        // AttributeUsage(ValidOn = Field and Parameter, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        MARSHALASATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindMarshalAs_UnmanagedType,
                MARSHALASATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I2 }
            },

            {
                attrkindMarshalAs_Int16,
                MARSHALASATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.OptionalAttribute
        //
        // AttributeUsage(ValidOn = Parameter, AllowMultiple=false, Inherited=false)
        //---------------------------------------------------------------------
        OPTIONALATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols, 
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindOptional_void,
                OPTIONALATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.OutAttribute
        //
        // AttributeUsage(ValidOn = Method, AllowMultiple=false, Inherited=false)
        //---------------------------------------------------------------------
        OUTATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindOut_void,
                OUTATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.PreserveSigAttribute
        //
        // AttributeUsage(ValidOn = Method, AllowMultiple=false, Inherited=false)
        //---------------------------------------------------------------------
        PRESERVESIGATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols, // PreserveSigAttribute is copied separately
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindPreserveSig_void,
                PRESERVESIGATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.PrimaryInteropAssemblyAttribute
        //
        // AttributeUsage(ValidOn = Assembly, AllowMultiple=true, Inherited=false)
        //---------------------------------------------------------------------
        PRIMARYINTEROPASSEMBLYATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(true, false)
        {
            {
                attrkindPrimaryInteropAssembly_Int32_Int32,
                PRIMARYINTEROPASSEMBLYATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I4, ELEMENT_TYPE_I4 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.StructLayoutAttribute
        //
        // AttributeUsage(ValidOn = Class and Struct, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        STRUCT_LAYOUT_ATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindStructLayout_Int16,
                STRUCT_LAYOUT_ATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2 }
            },
            {
                attrkindStructLayout_LayoutKind,
                STRUCT_LAYOUT_ATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4}
            },
            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.TypeIdentifierAttribute
        //
        // AttributeUsage(ValidOn = Class + Struct + Enum + Interface + Delegate, AllowMultiple=false, Inherited=false)
        //---------------------------------------------------------------------
        TYPEIDENTIFIERATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindTypeIdentifier_void,
                TYPEIDENTIFIERATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            {
                attrkindTypeIdentifier_String_String,
                TYPEIDENTIFIERATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING, ELEMENT_TYPE_STRING }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.TypeLibFuncAttribute
        //
        // AttributeUsage(ValidOn = Method, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        TYPELIBFUNCATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindTypeLibFunc_Int16,
                TYPELIBFUNCATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2 }
            },

            {
                attrkindTypeLibFunc_TypeLibFuncFlags,
                TYPELIBFUNCATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4, }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.TypeLibTypeAttribute
        //
        // AttributeUsage(ValidOn = Class | Interface | Struct | Enum, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        TYPELIBTYPEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {

            {
                attrkindTypeLibType_Int16,
                TYPELIBTYPEATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2 }
            },

            {
                attrkindTypeLibType_TypeLibTypeFlags,
                TYPELIBTYPEATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.TypeLibVarAttribute
        //
        // AttributeUsage(ValidOn = Field, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        TYPELIBVARATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindTypeLibVar_Int16,
                TYPELIBVARATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_I2 }
            },

            {
                attrkindTypeLibVar_TypeLibVarFlags,
                TYPELIBVARATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },


   {
        //---------------------------------------------------------------------
        // System.Runtime.InteropServices.UnmanagedFunctionPointerAttribute
        //
        // AttributeUsage(ValidOn = Method, AllowMultiple=false, Inherited=false)
        //---------------------------------------------------------------------
        UNMANAGEDFUNCTIONPOINTERATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindUnmanagedFunctionPointer_CallingConvention,
                UNMANAGEDFUNCTIONPOINTERATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Security.SecurityCriticalAttribute
        //
        // AttributeUsage(ValidOn = Assembly | Class | Struct | Enum | Constructor | Method | Field | Interface | Delegate, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        SECURITYCRITICALATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindSecurityCritical_Void,
                SECURITYCRITICALATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            {
                attrkindSecurityCritical_Scope,
                SECURITYCRITICALATTRIBUTE,
                5,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4 }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Security.SecuritySafeCriticalAttribute
        //
        // AttributeUsage(ValidOn = Class | Struct | Enum | Constructor | Method | Field | Interface | Delegate, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        SECURITYSAFECRITICALATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindSecuritySafeCritical_Void,
                SECURITYSAFECRITICALATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.SerializableAttribute
        //
        // AttributeUsage(ValidOn = Class | Struct | Enum | Delegate, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        SERIALIZABLEATTRIBUTE,
        DoCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindSerializable_Void,
                SERIALIZABLEATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.STAThreadAttribute
        //
        // AttributeUsage(ValidOn = Method, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        STATHREADATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindSTAThread_Void,
                STATHREADATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Web.Services.WebMethodAttribute
        //
        // AttributeUsage(ValidOn = Method, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        WEBMETHODATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindWebMethod_Void_NamedProps,
                WEBMETHODATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            {
                attrkindWebMethod_Boolean_NamedProps,
                WEBMETHODATTRIBUTE,
                4,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_BOOLEAN }
            },

            {
                attrkindWebMethod_Boolean_Transaction_NamedProps,
                WEBMETHODATTRIBUTE,
                6,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_BOOLEAN, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4}
            },

            {
                attrkindWebMethod_Boolean_Transaction_Int32_NamedProps,
                WEBMETHODATTRIBUTE,
                7,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 3, ELEMENT_TYPE_VOID, ELEMENT_TYPE_BOOLEAN, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4, ELEMENT_TYPE_I4}
            },

            {
                attrkindWebMethod_Boolean_Transaction_Int32_Boolean_NamedProps,
                WEBMETHODATTRIBUTE,
                8,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 4, ELEMENT_TYPE_VOID, ELEMENT_TYPE_BOOLEAN, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4, ELEMENT_TYPE_I4, ELEMENT_TYPE_BOOLEAN}
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // System.Web.Services.WebServiceAttribute
        //
        // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        WEBSERVICEATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindWebService_Void_NamedProps,
                WEBSERVICEATTRIBUTE,
                3,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    },

    {
        //---------------------------------------------------------------------
        // Windows.Foundation.Metadata.DeprecatedAttribute
        //
        // AttributeUsage(ValidOn = All, AllowMultiple = false, Inherited = false)
        //---------------------------------------------------------------------
        DEPRECATEDATTRIBUTE,
        DoNotCopyForNoPiaEmbeddedSymbols,
        ATTR_USAGE_ENTRY(false, false)
        {
            {
                attrkindDeprecated_String_DeprType_UInt,
                DEPRECATEDATTRIBUTE,
                7,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 3, ELEMENT_TYPE_VOID,  ELEMENT_TYPE_STRING, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4, ELEMENT_TYPE_U4 }
            },

            {
                attrkindDeprecated_String_DeprType_UInt_Platform,
                DEPRECATEDATTRIBUTE,
                9,
                { IMAGE_CEE_CS_CALLCONV_HASTHIS, 4, ELEMENT_TYPE_VOID,  ELEMENT_TYPE_STRING, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4, ELEMENT_TYPE_U4, ELEMENT_TYPE_VALUETYPE, ELEMENT_TYPE_I4  }
            },

            ATTRIBUTE_ENTRY_NULL
        }
    }
};

#if DEBUG

void VerifyAttributeTable()
{
    const WCHAR *pPreviousName = L"";
    ULONG cConstructors = 0;

    for (ULONG iIndex = 0; iIndex < DIM(g_rgAttributes); iIndex++)
    {
        // Verify that the names of the attributes are sorted alphabetically
        VSASSERT(
            CompareNoCase(pPreviousName, g_rgAttributes[iIndex].wszName) < 0,
            "Attribute names are not sorted alphabetically");

        pPreviousName = g_rgAttributes[iIndex].wszName;
        VSASSERT(wcslen(pPreviousName) <= MaxIdentifierLength, "Attribute name too long");

        ULONG iConstructors;

        for (iConstructors = 0; iConstructors < MaxConstructors; iConstructors++)
        {
            if (g_rgAttributes[iIndex].Attributes[iConstructors].m_attrkind != attrkindNil)
            {
                cConstructors++;

                // Verify the list of constructors is properly terminated.
                VSASSERT(
                    CompareNoCase(
                        g_rgAttributes[iIndex].Attributes[iConstructors].m_pwszName,
                         g_rgAttributes[iIndex].wszName) == 0,
                        "Names do not match");
            }
            else
            {
                break;
            }

            VSASSERT(iConstructors < MaxConstructors, "List of constructors is not properly terminated");
        }
    }

    // Verify that the numberof constructors match
    VSASSERT(cConstructors == attrkindMax, "Number of known constructors doesn't match");
}
#endif

//============================================================================
// Compare the names of two attribute indices.
//
// Returns:
//    <0: if name of AttrIndex1 is less than the name of AttrIndex2
//    =0: if name of AttrIndex1 is less equal to the name of AttrIndex2
//    >0: if name of AttrIndex1 is greater than to the name of AttrIndex2
//============================================================================
static int  _cdecl CompareAttributeIndicesByName
(
    const void *AttrIndex1,
    const void *AttrIndex2

)
{
    return CompareNoCase(
        ((const AttrIndex *)AttrIndex1)->wszName,
        ((const AttrIndex *)AttrIndex2)->wszName);
}

//============================================================================
// Looks for an AttrIndex Entry in the g_rgAttributes Array
// that matches the given name.
//============================================================================
const AttrIndex *FindAttributeIndexByName
(
    _In_z_ const WCHAR *wszAttrName   // IN: Attribute's fully-qualified name
)
{
    VSASSERT(wszAttrName != NULL, "Invalid Attribute Name");

#if DEBUG
    static bool IsTableVerified = false;

    if (!IsTableVerified)
    {
        VerifyAttributeTable();
        IsTableVerified = true;
    }
#endif

    AttrIndex attrIndex;

    attrIndex.wszName = wszAttrName;

    return (const AttrIndex *) bsearch(
        &attrIndex,
        g_rgAttributes,
        DIM(g_rgAttributes),
        sizeof(g_rgAttributes[0]),
        CompareAttributeIndicesByName
        );
}

//============================================================================
// Given a name of an attribute and the specific constructor id,
// return the information of the attribute.
//
// Returns:
//    - A pointer to the Attridentity object that matches the given signature; or
//    - NULL, if otherwise
//============================================================================
const AttrIdentity *FindAttributeByID
(
    _In_z_ const WCHAR *wszName,   // IN: Attribute's fully-qualified name. Can't be NULL.
    AttrKind AttributeID // IN: The Id of the specific constructor.
)
{
    VSASSERT(wszName != NULL, "Invalid name");
    VSASSERT(AttributeID != attrkindNil, "Invalid ID");

    const AttrIndex *pAttributeIndex = FindAttributeIndexByName(wszName);

    if (pAttributeIndex)
    {
        long iIndex = 0;

        while (pAttributeIndex->Attributes[iIndex].m_attrkind != attrkindNil)
        {
            // If the ID's match, return the attribute.
            if (pAttributeIndex->Attributes[iIndex].m_attrkind == AttributeID)
            {
                return &pAttributeIndex->Attributes[iIndex];
            }

            iIndex++;
        }
    }

    VSFAIL("Id was not found");
    return NULL;
}


//============================================================================
// Given a name and a signature, the function looks up the
// a matching AttrIdentity struct in the array of well known
// attributes.
//
// Returns:
//    - A pointer to the Attridentity object that matches the given signature; or
//    - NULL, if otherwise
//============================================================================
const AttrIdentity *FindAttributeBySignature
(
    _In_z_ const WCHAR *wszName, // IN: Attribute's fully-qualified name. It can't be NULL.
    ULONG cbSignature,    // IN: Length of signature in bytes
    PCCOR_SIGNATURE pSignature     // IN: CTOR signature to use to drive arg parsing
)
{
    VSASSERT(wszName != NULL, "Invalid name");

    const AttrIndex *pAttributeIndex = FindAttributeIndexByName(wszName);
    if (pAttributeIndex)
    {
        return FindAttributeBySignature(pAttributeIndex, cbSignature, pSignature);
    }

    return NULL;
}

//============================================================================
// Given am AttrIndex  and a signature, the function looks up the
// a matching AttrIdentity within the list of constructors of the given
// attribute index.
//
// Note that the function will only consider positional parameters
// when doing the comparison. It will ignore named properties.
//
// Returns:
//    - A pointer to the Attridentity object that matches the given signature; or
//    - NULL, if otherwise
//============================================================================
//============================================================================
const AttrIdentity *FindAttributeBySignature
(
    const AttrIndex *pAttributeIndex, // IN: Attribute Index. It cant be NULL
    ULONG cbSignature,    // IN: Length of signature in bytes
    PCCOR_SIGNATURE pSignature     // IN: CTOR signature to use to drive arg parsing
)
{
    VSASSERT(pAttributeIndex != NULL, "Invalid NULL Pointer");

    if (pAttributeIndex)
    {
        long iIndex = 0;

        while (pAttributeIndex->Attributes[iIndex].m_attrkind != attrkindNil)
        {
            const AttrIdentity *pAttribute = &pAttributeIndex->Attributes[iIndex];

            // Make sure the headers match.
            if (pAttribute->m_sig[0] == pSignature[0] &&
                pAttribute->m_sig[1] == pSignature[1] &&
                pAttribute->m_sig[2] == pSignature[2])
            {
                ULONG cParameters = pAttribute->m_sig[1];   // Number of parameters to compare
                ULONG iParameterIndex = 0;                  // Current parameter
                ULONG iSignatureIndex = 3;                  // Position of the parameter within the signature
                ULONG iTableIndex = 3;                  // Position of the parameter within the table version of the signature

                // Scan the rest of the parameters
                while (iParameterIndex < cParameters)
                {
                    if (pAttribute->m_sig[iTableIndex] == pSignature[iSignatureIndex])
                    {
                        // HACK Microsoft 9/4/2002:
                        // Since we are not going to look up the tokens that follow
                        // ELEMENT_TYPE_CLASS and ELEMENT_TYPE_SIGNATURE,
                        // we just ignore the value following these constants.
                        if (pSignature[iSignatureIndex] == ELEMENT_TYPE_CLASS ||
                            pSignature[iSignatureIndex] == ELEMENT_TYPE_VALUETYPE)
                        {
                            // Ignore the byte following either ELEMENT_TYPE_CLASS or
                            // ELEMENT_TYPE_VALUETYPE for the signature stored in the table;
                            iTableIndex++;

                            // Check the value of the token and advance the right number of positions
                            if ((pSignature[iSignatureIndex + 1] & 0xC0) == 0xC0)
                            {
                                // Four byte encoding
                                iSignatureIndex += 4;
                            }
                            else if ((pSignature[iSignatureIndex + 1] & 0x80) == 0x80)
                            {
                                // Two byte encoding
                                iSignatureIndex += 2;
                            }
                            else
                            {
                                // One byte encoding
                                iSignatureIndex++;
                            }
                        }
                    }
                    else
                    {
                        // This is not a match so break early.
                        break;
                    }

                    iSignatureIndex++;
                    iTableIndex++;
                    iParameterIndex++;
                }

                if (iParameterIndex == cParameters)
                {
                    return pAttribute;
                }
            }

            iIndex++;
        }
    }

    return NULL;
}

//============================================================================
// Decodes a COM+ boolean as serialized in a custom attribute arg blob.
//============================================================================
bool Attribute::----Bool
(
    BYTE **ppbArgBlob   // IN OUT: Arg blob to decode (updated to point past on exit)
)
{
    bool val = (**ppbArgBlob) ? true : false;
    (*ppbArgBlob) += 1;

    return val;
}

//============================================================================
// Decodes a COM+ I1 as serialized in a custom attribute data blob.
//============================================================================
__int8 Attribute::----I1
(
    BYTE **ppbArgBlob   // IN OUT: Arg blob to decode (updated to point past on exit)
)
{
    __int8 val = *(__int8 *)*ppbArgBlob;
    (*ppbArgBlob) += 1;

    return val;
}

//============================================================================
// Decodes a COM+ I2 as serialized in a custom attribute data blob.
//============================================================================
__int16 Attribute::----I2
(
    BYTE **ppbArgBlob   // IN OUT: Arg blob to decode (updated to point past on exit)
)
{
#ifdef _WIN64
    __int16 val = (*ppbArgBlob)[1];
    val = (val << 8) | (*ppbArgBlob)[0];
#else
    __int16 val = *(__int16 *)*ppbArgBlob;
#endif
    (*ppbArgBlob) += 2;

    return val;
}

//============================================================================
// Decodes a COM+ I4 as serialized in a custom attribute data blob.
//============================================================================
__int32 Attribute::----I4
(
    BYTE **ppbArgBlob   // IN OUT: Arg blob to decode (updated to point past on exit)
)
{
#ifdef _WIN64
    __int32 val;
    //size_t temp = (size_t) *ppbArgBlob;
    if (((size_t) (*ppbArgBlob)) & 0x03)
    {
        val = (*ppbArgBlob)[3];
        val = (val << 8) | (*ppbArgBlob)[2];
        val = (val << 8) | (*ppbArgBlob)[1];
        val = (val << 8) | (*ppbArgBlob)[0];
    }
    else
        val = *(__int32 *)*ppbArgBlob;
#else
    __int32 val = *(__int32 *)*ppbArgBlob;
#endif
    (*ppbArgBlob) += 4;

    return val;
}

//============================================================================
// Decodes a COM+ I8 as serialized in a custom attribute data blob.
//============================================================================
__int64 Attribute::----I8
(
    BYTE **ppbArgBlob   // IN OUT: Arg blob to decode (updated to point past on exit)
)
{
// Int64/UInt64/double should always be aligned at the 64-bit boundary for certain
// ARM instructions
#if _WIN64 || ARM   
    __int64 val;
    //size_t temp = (size_t) *ppbArgBlob;
    if (((size_t) (*ppbArgBlob)) & 0x07)
    {
        val = (*ppbArgBlob)[7];
        val = (val << 8) | (*ppbArgBlob)[6];
        val = (val << 8) | (*ppbArgBlob)[5];
        val = (val << 8) | (*ppbArgBlob)[4];
        val = (val << 8) | (*ppbArgBlob)[3];
        val = (val << 8) | (*ppbArgBlob)[2];
        val = (val << 8) | (*ppbArgBlob)[1];
        val = (val << 8) | (*ppbArgBlob)[0];
    }
    else
        val = *(__int64 *)*ppbArgBlob;
#else
    __int64 val = *(__int64 *)*ppbArgBlob;
#endif
    (*ppbArgBlob) += 8;

    return val;
}

//============================================================================
// Decodes a COM+ R4 as serialized in a custom attribute data blob.
//============================================================================
float Attribute::----R4
(
    BYTE **ppbArgBlob   // IN OUT: Arg blob to decode (updated to point past on exit)
)
{
#ifdef _WIN64
    float val;

    if ((size_t)(*ppbArgBlob) & 0x7)
    {
        memcpy(&val, (*ppbArgBlob), 4);
    }
    else
        val = *(float *) *ppbArgBlob;
#else
    float val = *(float *) *ppbArgBlob;
#endif

    (*ppbArgBlob) += 4;

    return val;
}

//============================================================================
// Decodes a COM+ R8 as serialized in a custom attribute data blob.
//============================================================================
double Attribute::----R8
(
    BYTE **ppbArgBlob   // IN OUT: Arg blob to decode (updated to point past on exit)
)
{
// Int64/UInt64/double should always be aligned at the 64-bit boundary for certain
// ARM instructions
#if _WIN64 || ARM
    // VSWhidbey#58053, Microsoft: We cannot expect the CLR to give us a blob
    // that is correctly byte-aligned.  So, we have to protect ourselves.
    // Detect if we are mis-aligned and if we are then do the appropriate
    // thing.
    double val;
    if (((size_t) (*ppbArgBlob)) & 0x07)
    {
        memcpy(&val, (*ppbArgBlob), 8);
    }
    else
        val = *(double *) *ppbArgBlob;
#else
    double val = *(double *) *ppbArgBlob;
#endif

    (*ppbArgBlob) += 8;

    return val;
}

//============================================================================
// Decodes a COM+ String as serialized in a custom attribute data blob.
//============================================================================
WCHAR *Attribute::----String
(
    BYTE **ppbArgBlob,      // IN OUT: Arg blob to decode (updated to point past on exit)
    NorlsAllocator *pnra    // IN: Scratch allocator used to allocate string
)
{
    WCHAR *pwszOut = NULL;

    // Strings are stored as UTF8, but 0xFF means NULL string.
    if (**ppbArgBlob == 0xFF)
    {
        pwszOut = NULL;
        (*ppbArgBlob) += 1;
    }
    else
    {
        int cbString, cchString;
        PCSTR pstrData;
        PCCOR_SIGNATURE psig = (PCCOR_SIGNATURE)*ppbArgBlob;

        cbString = CorSigUncompressData(psig);
        pstrData = (PCSTR) psig;
        cchString = UnicodeLengthOfUTF8(pstrData, cbString) + 1;

        // Overflow checks
        IfFalseThrow(cchString > 0);

        // Only copy the data if an allocator was provided.
        if (pnra)
        {
            pwszOut = (WCHAR *) pnra->Alloc(VBMath::Multiply(cchString, sizeof(WCHAR)));
            UTF8ToUnicode(pstrData, cbString, pwszOut, cchString);
            pwszOut[cchString - 1] = L'\0';
        }

        *ppbArgBlob = (BYTE *)pstrData + cbString;
    }

    return pwszOut;
}


// Returns a string that represents the given serialized System.Type
// expressed as a fully-qualified type name.
WCHAR *Attribute::ConvertSerializedSystemTypeToTypeName
(
    _In_opt_z_ WCHAR *pwszSystemType,
    NorlsAllocator *pnra
)
{
    return
        m_pcompiler->GetCLRTypeNameEncoderDecoder()->CLRNameToVBName(
            pwszSystemType,
            pnra,
            m_pcompilerproj,
            m_pMetaemit);
}

//============================================================================
// ----s all well-known custom attributes attached to mdtoken and
// returns their settings through ppattrvals (which is allocated by
// this routine if needed).
//============================================================================
/* static */
void Attribute::----AllAttributesOnToken
(
    mdToken          mdtoken,
    IMetaDataImport *pimdImport,
    NorlsAllocator  *pnra,
    Compiler        *pcompiler,
    CompilerProject *pcompilerproject,
    MetaDataFile    *pMetaDataFile,
    BCSYM           *psym   // IN OUT
)
{
    VSASSERT(psym->GetPAttrVals() == NULL, "Double loading!");

    // Create an instance of the Attribute class
    // (NULL and CS_MAX mean we're importing metadata)
    Attribute attribute(pimdImport, NULL, pnra, pcompiler, pcompilerproject, pMetaDataFile);

    // ...and call the non-static method to do all the work.
    attribute._----AllAttributesOnToken(mdtoken, psym);
#if FV_TRACK_MEMORY && IDE
    pcompiler->GetInstrumentation()->TrackSymbolAttributeUsage(pnra, psym);
#endif

}


void Attribute::_----AllAttributesOnToken
(
    mdToken          mdtoken,
    BCSYM           *psym   // IN OUT
)
{
    HCORENUM hcorenum = 0;
    mdToken  rgtokenAttr[10];
    ULONG    cAttr, iAttr;

    do
    {
        // Get next batch of attributes.
        IfFailThrow(m_pimdImport->EnumCustomAttributes(
                                    &hcorenum,
                                    mdtoken,
                                    0,
                                    rgtokenAttr,
                                    DIM(rgtokenAttr),
                                    &cAttr));

        // Process each attribute.
        for (iAttr = 0; iAttr < cAttr; ++iAttr)
        {
            mdToken tokenAttr;
            const void *pvArgBlob;
            ULONG cbArgBlob;

#pragma prefast(suppress: 26017, "cAttr is bounded by DIM(rgtokenAttr)")
            IfFailThrow(m_pimdImport->GetCustomAttributeProps(
                            rgtokenAttr[iAttr],
                            NULL,
                            &tokenAttr,
                            &pvArgBlob,
                            &cbArgBlob));

            ----OneAttribute(tokenAttr, (PBYTE)pvArgBlob, cbArgBlob, psym);
        }
    } while (cAttr > 0);

    if (hcorenum)
    {
        m_pimdImport->CloseEnum(hcorenum);
    }

    if (psym->IsType())
    {
        // Check the pseudo-custom attributes, which are stored as bit flags on the type properties.
        // We will store these on the WellKnownAttrVals struct, like real attributes, because this is
        // consistent with the way they are documented and the way users create them.
        DWORD dwFlags = 0;
        IfFailThrow(m_pimdImport->GetTypeDefProps(mdtoken, NULL, 0, NULL, &dwFlags, NULL));

        bool needToStoreLayout = false;
        bool needToStoreCharSet = false;

        if (psym->IsClass())
        {
            needToStoreLayout  = ((dwFlags & tdLayoutMask) != tdSequentialLayout);

            // Dev10 #702321
            needToStoreCharSet  = ((dwFlags & tdStringFormatMask) == tdUnicodeClass || (dwFlags & tdStringFormatMask) == tdAutoClass);
        }

        if (dwFlags & tdImport || dwFlags & tdSerializable || dwFlags & tdWindowsRuntime || needToStoreLayout || needToStoreCharSet)
        {
            if (psym->GetPWellKnownAttrVals() == NULL)
            {
                psym->AllocAttrVals(m_pnra, CS_MAX, NULL, NULL, true);
            }
        }
        
        if (dwFlags & tdImport)
        {
            psym->GetPWellKnownAttrVals()->SetComImportData();
        }
        
        if (dwFlags & tdWindowsRuntime)
        {
            psym->GetPWellKnownAttrVals()->SetWindowsRuntimeImportData();
        }

        if (dwFlags & tdSerializable)
        {
            psym->GetPWellKnownAttrVals()->SetSerializableData();
        }
        
        if (needToStoreLayout)
        {
            //m_pWellKnownAttrVals is created, no assertion needed here
            psym->GetPWellKnownAttrVals()->SetStructLayoutData(dwFlags & tdLayoutMask);
        }

        // Dev10 #702321
        if (needToStoreCharSet)
        {
            psym->GetPWellKnownAttrVals()->SetCharSet((CorTypeAttr)(dwFlags & tdStringFormatMask));
        }
    }

// Verify attribute usage for well known attributes
#if DEBUG
    if (psym->IsClass())
    {
        // Build the fully qualified name.
        STRING *strName = ConcatNameSpaceAndName(
            psym->PClass()->GetCompiler(),
            psym->PClass()->GetNameSpace(),
            psym->PClass()->GetName());

        // Verify if it is a well known attribute
        const AttrIndex *pAttribute = FindAttributeIndexByName(strName);
        if (pAttribute)
        {
            CorAttributeTargets corTargets;
            bool IsAllowMultiple = false;
            bool IsInheritable = false;

            if (psym->GetPWellKnownAttrVals()->GetAttributeUsageData(&corTargets, &IsAllowMultiple, &IsInheritable))
            {
                // For .NetCF Asserts look at VSWHIDBEY#68651, #153779 and #114078.

                // Backwards compatibility with the Everett CLR
                if (!(m_pcompilerhost->GetRuntimeVersion() <= RuntimeVersion1 &&
                    (StrCmp(strName, NONSERIALIZEDATTRIBUTE) == 0 ||
                    StrCmp(strName, COMSOURCEINTERFACES) == 0)))
                {
                // NetCF Bug: 18559
                    if (m_pcompilerhost->IsStarliteHost() && StrCmp(strName, COMSOURCEINTERFACES) == 0)
                    {
                    }
                    else
                    {
                        VSASSERT(
                            IsAllowMultiple == pAttribute->IsAllowMultiple && IsInheritable == pAttribute->IsInheritable,
                            "Attribute usage doesn't match table");
                    }
                }
            }
            else
            {
                VSFAIL("Not attribute usage information");
            }
        }
    }
#endif
}

//============================================================================
// Returns true if the member represented by the token is annotated with the
// specified attribute, false if not, and HasValue() will be false if we were
// unable to ---- attributes because we don't have an Import interface - which
// can happen in scenarios where the assembly in question is not on disk.  That
// can occur when we are doing remote debugging, for instance.
//
// Doesn't ---- the attribute arguments - just checks for the presence of the
// attribute.
//============================================================================
/* static */
TriState<bool> Attribute::HasAttribute
(
    MetaDataFile *pMetaDataFile, // the metadatafile that owns the member we want to ----
    mdToken Token, // the token of the member we are examining for a specified attribute
    AttrKind SpecifiedAttribute // see if the token is marked with this attribute
)
{
    TriState<bool> hasAttribute;
    // In some situations (such as remote or device debugging scenarios where a dll referenced by an assembly may not be on disk) GetImport() can return NULL.
    if ( pMetaDataFile->GetImport() != NULL )
    {   
        Attribute attribute(pMetaDataFile->GetImport(), NULL /* MetaEmit */, pMetaDataFile->SymbolStorage(), 
            pMetaDataFile->GetCompiler(), pMetaDataFile->GetProject(), pMetaDataFile );

        hasAttribute.SetValue( attribute._HasAttribute(Token, SpecifiedAttribute));
    }

    return hasAttribute;
}

//============================================================================
// Worker function to determine whether the member represented by the specified
// token is annotated with the specified attribute.
//============================================================================
bool Attribute::_HasAttribute
(
    mdToken Token, // the token of the metadata member we want to check for the specified attribute
    AttrKind SpecifiedAttribute // the attribute we are looking for
)
{
    CorEnum hcorenum(m_pimdImport);
    mdToken rgtokenAttr[10];
    ULONG   cAttr;

    // Don't call this function if m_pimdImport is null (which can happen in remote or device debugging situations)
    // The front end to this function, HasAttribute(), should not call us if m_pimdImport is NULL.
    ThrowIfNull( m_pimdImport ); 

    do
    {
        // Get the batch of attributes on the token.
        IfFailThrow(m_pimdImport->EnumCustomAttributes(
                                    &hcorenum,
                                    Token,
                                    0,
                                    rgtokenAttr,
                                    _countof(rgtokenAttr),
                                    &cAttr));

        // Process each attribute until we find the one we are looking for
        for (ULONG iAttr = 0; iAttr < cAttr; ++iAttr)
        {
            mdToken tokenAttr;
            IfFailThrow(m_pimdImport->GetCustomAttributeProps(
                            rgtokenAttr[iAttr],
                            NULL, // out,optional token of the object the attribute modifies
                            &tokenAttr, // out the type of the custom attribute
                            NULL, // out, optional - the value of the custom attribute.  We aren't cracking it later so ignore
                            NULL)); // out, optional, the size of the value of the custom attribute.  We aren't cracking it later so ignore

            AttrIdentity *pAttribute = ----OneAttribute( tokenAttr,
                                                          NULL, // no pbArgBlob,
                                                          0, // no size of pbArgBlob
                                                          NULL, // no symbol to put ----ed results on
                                                          false); // don't ---- arg blob
            if (pAttribute && pAttribute->m_attrkind == SpecifiedAttribute)
            {
                return true;
            }
        }
    } while (cAttr > 0);

    return false;
}

//============================================================================
// Returns the name and signature of the custom attribute named by tokenAttr.
//
// Can return false if everything went okay but tokenAttr just wasn't
// an attribute, or if the signature was bogus.  OUT params are not
// set in this case.
//============================================================================
bool Attribute::GetNameAndSigFromToken
(
    mdToken                         tokenAttr,      // Attribute to query
    _Out_cap_(cchAttrName) WCHAR *wszAttrName,   // OUT: Name of attribute
    ULONG                           cchAttrName,    // IN: Size of name buffer
    PCCOR_SIGNATURE                 *ppsignature,   // OUT: CTOR's signature
    ULONG                           *pcbSignature   // OUT: Length of CTOR's signature
)
{
    bool fAttribute = false;
    mdToken tokenType;
    WCHAR   wszCtor[PEBuilder::c_cchNameMax];
    ULONG   cchCtor;

    if (TypeFromToken(tokenAttr) == mdtMemberRef || TypeFromToken(tokenAttr) == mdtMethodDef)
    {
        if (TypeFromToken(tokenAttr) == mdtMemberRef)
        {
            // Get name of member and signature.
            IfFailThrow(m_pimdImport->GetMemberRefProps(
                        tokenAttr,
                        &tokenType,
                        wszCtor, DIM(wszCtor), &cchCtor,
                        ppsignature, pcbSignature));
        }
        else
        {
            VSASSERT(TypeFromToken(tokenAttr) == mdtMethodDef, "Huh?");

            // Get name and signature.
            IfFailThrow(m_pimdImport->GetMethodProps(
                        tokenAttr,
                        &tokenType,
                        wszCtor, DIM(wszCtor), &cchCtor,
                        NULL,
                        ppsignature, pcbSignature,
                        NULL, NULL));
        }

        if (wcscmp(wszCtor, COR_CTOR_METHOD_NAME_W) != 0)
        {
            goto Exit;     // Not a constructor -- ignore
        }
    }
    else
    {
        goto Exit;   // Not a valid attribute -- ignore
    }

    // Get attribute name
    if (TypeFromToken(tokenType) == mdtTypeRef)
    {
        IfFailThrow(m_pimdImport->GetTypeRefProps(
                    tokenType,
                    NULL,
                    wszAttrName,
                    cchAttrName,
                    &cchAttrName));
    }
    else if (TypeFromToken(tokenType) == mdtTypeDef)
    {
        IfFailThrow(m_pimdImport->GetTypeDefProps(
                    tokenType,
                    wszAttrName,
                    cchAttrName,
                    &cchAttrName,
                    NULL,
                    NULL));
    }
    else
    {
        goto Exit; // Not a valid attribute -- ignore
    }

    // If we made it this far, we've really succeeded
    fAttribute = true;

Exit:
    return fAttribute;
}


//============================================================================
// Processes one custom attribute (and its constructor's args and
// named properties if desired) and returns the result (if any) through ppattrvals.
// Note that ppattrvals is allocated if one is needed and none is passed in.
//============================================================================
AttrIdentity *Attribute::----OneAttribute
(
    mdToken          tokenAttr,     // Token of attribute to ----
    BYTE            *pbArgBlob,     // Arg blob that's attached to the custom attribute.
    ULONG            cbArgBlob,     // Size of arg blob
    BCSYM           *psym,          // IN/OUT: ----ed results go here.
    bool            ----ArgBlob    // Whether to ---- the argument blob (defaults to true)
)
{
#ifdef _WIN64
    DynamicFixedSizeHashTable<size_t, AttrIdentity *> *pTokenHash = m_pMetaDataFile->GetAttributeTokenHash();
#else
    DynamicFixedSizeHashTable<mdToken, AttrIdentity *> *pTokenHash = m_pMetaDataFile->GetAttributeTokenHash();
#endif
    AttrIdentity *pAttribute = NULL;
    AttrIdentity **ppattr = NULL;

    if (pTokenHash)
    {
#ifdef _WIN64
        {
        size_t tkAttr = tokenAttr;
        ppattr = pTokenHash->HashFind(tkAttr);
        }
#else
        ppattr = pTokenHash->HashFind(tokenAttr);
#endif
    }

    if (ppattr)
    {
        pAttribute = *ppattr;
    }
    else
    {
        WCHAR           wszAttrName[PEBuilder::c_cchNameMax];
        PCCOR_SIGNATURE psignature;
        ULONG           cbSignature = 0;
        bool            fAttribute;

        fAttribute = GetNameAndSigFromToken(
                    tokenAttr,                      // Attribute to query
                    wszAttrName, DIM(wszAttrName),  // Attribute's name
                    &psignature, &cbSignature);    // Attribute CTOR's signature

        // GetNameAndSigFromToken can return false if everything went okay,
        // but tokenAttr just wasn't an attribute.  We silently ignore these cases.
        if (fAttribute)
        {
            // Find the attribute corresponding to the given name.
            const AttrIndex *pAttributeIndex = FindAttributeIndexByName(wszAttrName);

            if (pAttributeIndex)
            {
                pAttribute = (AttrIdentity *)FindAttributeBySignature(
                    pAttributeIndex,
                    cbSignature,
                    psignature);

                VSASSERT(pAttribute != NULL, "Constructor not in list");
            }
        }

        if (pTokenHash)
        {
#ifdef _WIN64
            {
                size_t tkAttr = tokenAttr;
                pTokenHash->HashAdd(tkAttr, pAttribute);
            }
#else
            pTokenHash->HashAdd(tokenAttr, pAttribute);
#endif
        }
    }

    if (----ArgBlob && pAttribute)
    {
        ----ArgBlob(pbArgBlob, cbArgBlob, pAttribute,  psym, false/*isAssembly*/, NULL /* Source File */); //in metadata Assembly modifier cannot show
    }

    return pAttribute;
}

WCHAR *MyWcstok_s(_In_z_ WCHAR *buf, _Inout_z_ WCHAR **context);

//============================================================================
// Processes the given arg blob and returns the result (if any)
// through ppattrvals (which is allocated on demand).
//============================================================================
void Attribute::----ArgBlob
(
    _In_count_(cbArgBlob) BYTE            *pbArgBlob,     // IN: Arg blob that's attached to the custom attribute
    ULONG            cbArgBlob,     // IN: Size of arg blob
    const AttrIdentity *pAttribute, // IN: Descriptor for the attribute signature. It shouldn't be NULL
    _Inout_ BCSYM          *psym,           // IN/OUT: ---- results go here (allocated on demand)
    bool            isAssembly,     // IN - is this an assembly attribute
    _In_opt_ SourceFile     *pSourceFile,    // IN - Source file containing the attribute (can be NULL)
    _In_opt_ Location       *pLoc            // IN - Location of the attribute
)
{
    NamedProps      namedprops;

    VSASSERT(pAttribute != NULL, "Invalid NULL Pointer");

    // If wszAttrName didn't match any well-known custom attribute, ignore
    if (pAttribute == NULL)
    {
        goto Error;
    }

    // HACK-O-RAMA for VS 104280.  An undocumented metadata special case:
    // if the arg blob has zero length, we should treat it as the minimal
    // arg blob -- four bytes (1, 0, 0, 0) that represent the special two-byte
    // header followed by a two-byte named-argument count.
    if (cbArgBlob == 0)
    {
        // This also indicates 0 number of arguments for the constructors.
        // So if expected number of arguments for the constructor of this
        // well known attribute type is non-zero, don't treat the attribute
        // as a well known attribute.  VS Everett Security Bug #
        if (((BYTE *)pAttribute->m_sig)[1] != 0)
        {
            goto Error;
        }

        static BYTE rgbMinimalBlob[] = { 1, 0, 0, 0 };
        pbArgBlob = rgbMinimalBlob;
        cbArgBlob = 4;
    }

    // Make sure that data is in the correct format. Check & eat the prolog.
#ifdef _WIN64
    // Alignment problem. at pbargblob.
    WORD temp = pbArgBlob[1];
    temp = (temp << 8) | pbArgBlob[0];
    if (cbArgBlob < sizeof(WORD) || temp != 1)
#else
    if (cbArgBlob < sizeof(WORD) || *(WORD *)pbArgBlob != 1)
#endif
    {
        goto Error;     // Bogus sig
    }

    // Update size and blob
    cbArgBlob -= sizeof(WORD);
    pbArgBlob += sizeof(WORD);

    // Default is that well-known attributes have no named properties
    namedprops.m_cProps = 0;

    // Dev10 #570413
    // Due to this fix, some well-known attributes do not save anything into WellKnownAttrVals.
    // This switch is to deal with them and get out before allocating AttrVals.
    switch(pAttribute->m_attrkind)
    {
        case attrkindPreserveSig_void:
            AssertIfFalse(psym->IsProc());
            if (psym->IsProc())
            {
                psym->PProc()->SetPreserveSig(true);
            }
            return;

        case attrkindOptional_void:
            // Nothing to do
            return;
    }

    // Need to allocate an OUT AttrVals if we don't already have one
    if (psym->GetPAttrVals() == NULL)
    {
        VSASSERT( m_pMetaDataFile, "Unexpected: How can a non-metadata symbol be called without allocating space for AttrVals ?");

        // Since this AttrVals will be hung off a symbol, allocate from the symbol allocator.
        const bool AllocForWellKnownAttributesToo = true;
        psym->AllocAttrVals(m_pnra, CS_MAX, NULL, NULL, AllocForWellKnownAttributesToo);
    }

    WellKnownAttrVals *pattrvals;
    pattrvals = psym->GetPWellKnownAttrVals();

    VSASSERT( pattrvals,
                    "How can we have NULL well known attributes ? Out of order cracking of attributes ?");

    switch (pAttribute->m_attrkind)
    {
        case attrkindAttributeUsage_AttributeTargets_NamedProps:
            {
                CorAttributeTargets targetsValidOn;
                bool fAllowMultiple = false;
                bool fInherited = false;

                //
                // Set up named props (bool AllowMultiple and bool Inherited)
                //
                namedprops.m_cProps = 2;
                namedprops.m_rgwszPropName[0] = L"AllowMultiple";
                namedprops.m_rgpvPropVal[0] = &fAllowMultiple;
                namedprops.m_rgwszPropName[1] = L"Inherited";
                namedprops.m_rgpvPropVal[1]= &fInherited;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &targetsValidOn,
                    NULL);
                pattrvals->SetAttributeUsageData(targetsValidOn, fAllowMultiple, fInherited);
                break;
            }

        case attrkindDecimalConstant_Decimal:
        case attrkindDecimalConstant_DecimalInt:
            {
                DECIMAL decimalValue;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &decimalValue.scale,
                    &decimalValue.sign,
                    &decimalValue.Hi32,
                    &decimalValue.Mid32,
                    &decimalValue.Lo32,
                    NULL);
                pattrvals->SetDecimalConstantData(decimalValue);
            break;
            }

        case attrkindDateTimeConstant_Int64:
            {
                __int64 dateTimeValue;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &dateTimeValue,
                    NULL);

                pattrvals->SetDateTimeConstantData(dateTimeValue);
                break;
            }

        case attrkindIDispatchConstant_void:
            pattrvals->SetIDispatchConstantData();
            break;

        case attrkindIUnknownConstant_void:
            pattrvals->SetIUnknownConstantData();
            break;

        case attrkindParamArray_Void:
            pattrvals->SetParamArrayData();
            break;

        case attrkindObsolete_Void:
            pattrvals->SetObsoleteData(NULL /* pwszMessage */, false /* fError */);
            break;

        case attrkindObsolete_String:
            {
                WCHAR* pwszObsoleteMessage1;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszObsoleteMessage1,
                    NULL);

                pattrvals->SetObsoleteData(pwszObsoleteMessage1, false /* fError */);
                break;
            }

        case attrkindObsolete_String_Bool:
            {
                WCHAR* pwszObsoleteMessage2;
                bool fError;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszObsoleteMessage2,
                    &fError,
                    NULL);

                pattrvals->SetObsoleteData(pwszObsoleteMessage2, fError);
            break;
            }

        case attrkindDeprecated_String_DeprType_UInt:
            {
                WCHAR* pwszDeprecatedMessage;
                __int32 deprecationType = 0;
                __int32 version = 0;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszDeprecatedMessage,
                    &deprecationType,
                    &version,
                    NULL);

                // First argument is mapped to obsolete message.
                // Second argument (DeprecationType) determines if we need to generate an error or a warning:
                //	a) DeprecationType.Deprecate(0) corresponds to a compiler warning.
                //	b) DeprecationType.Remove(1) corresponds to an error.

                // We ignore the third argument (version).

                pattrvals->SetObsoleteData(pwszDeprecatedMessage, deprecationType == 1);
            break;
            }

        case attrkindDeprecated_String_DeprType_UInt_Platform:
            {
                WCHAR* pwszDeprecatedMessage;
                __int32 deprecationType = 0;
                __int32 version = 0;
                __int32 plaform = 0;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszDeprecatedMessage,
                    &deprecationType,
                    &version,
                    &plaform,
                    NULL);

                // First argument is mapped to obsolete message.
                // Second argument (DeprecationType) determines if we need to generate an error or a warning:
                //	a) DeprecationType.Deprecate(0) corresponds to a compiler warning.
                //	b) DeprecationType.Remove(1) corresponds to an error.
				
                // We ignore the last two arguments (version and platform).

                pattrvals->SetObsoleteData(pwszDeprecatedMessage, deprecationType == 1);
            break;
            }

        case attrkindConditional_String:
            {
                WCHAR *pwszConditional;

                // ConditionalAttribute is a little bit different because we
                // need to keep a *list* of conditional strings.

                // Get the string
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszConditional,
                    NULL);

                // Link onto the head of the list
                pwszConditional = m_pcompiler->AddString(pwszConditional);
                //pConditionalString->m_pConditionalStringNext = pattrvals->m_ConditionalData.m_pConditionalStringFirst;
                //pattrvals->m_ConditionalData.m_pConditionalStringFirst = pConditionalString;
                pattrvals->AddConditionalData(pwszConditional);
                break;
            }

        case attrkindCLSCompliant_Bool:
            {
                // Single Use Attribute: So ---- this only once
                // i.e. ---- this only if not yet seen one
                //
                bool fCompliant;
                if (!pattrvals->GetCLSCompliantData(&fCompliant))
                {
                    ----AttributeData(
                        &namedprops,
                        m_pnra,
                        pAttribute->m_sig,
                        pbArgBlob,
                        &fCompliant,
                        NULL);
                    pattrvals->SetCLSCompliantData(fCompliant, false /* fIsInherited */);
                }
                break;
            }

        case attrkindFlags_Void:
            pattrvals->SetFlagsData();
            break;

        case attrkindDebuggerBrowsable_BrowsableState:
            {
                DebuggerBrowsableStateEnum debuggerBrowsableState = (DebuggerBrowsableStateEnum)0;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &debuggerBrowsableState,
                    NULL);
                pattrvals->SetDebuggerBrowsableData(debuggerBrowsableState);
                break;
            }

        case attrkindDebuggerDisplay_String:
            pattrvals->SetDebuggerDisplayData();
            break;

        case attrkindDesignerSerializationVisibilityAttribute_DesignerSerializationVisibilityType:
            {
                int DesignerSerializationVisibilityType = 0;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &DesignerSerializationVisibilityType,
                    NULL);
                const int DESIGNERSERIALIZATIONVISIBILITYTYPE_CONTENT = 2;
                if (DesignerSerializationVisibilityType == DESIGNERSERIALIZATIONVISIBILITYTYPE_CONTENT)
                {
                    pattrvals->SetPersistContentsData(true /* fPersistsContents */);
                }
                else
                {
                    pattrvals->SetPersistContentsData(false /* fPersistsContents */);
                }
                break;
            }

        case  attrkindAssemblyKeyFile_String:
            if (pSourceFile)
            {
                WCHAR *pValue = NULL;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pValue,
                    NULL);

                // Microsoft: Generate a warning if the user is using this attrbute.

                if( pValue != NULL && ::wcslen( pValue ) > 0 )
                {
                    SourceFile::GetCurrentErrorTableForFile( pSourceFile )->CreateError(
                        WRNID_UseSwitchInsteadOfAttribute,
                        pLoc,
                        L"/keyfile",
                        ASSEMBLYKEYFILEATTRIBUTE);
                }

                pSourceFile->SetFileContainsAssemblyKeyFileAttr(true, m_pcompiler->AddString(pValue));
            }
            break;

        case  attrkindAssemblyKeyName_String:
            if (pSourceFile)
            {
                WCHAR *pValue = NULL;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pValue,
                    NULL);

                // Microsoft: Generate a warning if the user is using this attrbute.

                if( pValue != NULL && ::wcslen( pValue ) > 0 )
                {
                    SourceFile::GetCurrentErrorTableForFile( pSourceFile )->CreateError(
                        WRNID_UseSwitchInsteadOfAttribute,
                        pLoc,
                        L"/keycontainer",
                        ASSEMBLYKEYNAMEATTRIBUTE);
                }

                pSourceFile->SetFileContainsAssemblyKeyNameAttr(true, m_pcompiler->AddString(pValue));
            }
            break;

        case  attrkindAssemblyVersion_String:
            if (pSourceFile)
            {
                WCHAR *pValue = NULL;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pValue,
                    NULL);

                pSourceFile->SetFileContainsAssemblyVersionAttr(true, m_pcompiler->AddString(pValue));
            }
            break;

        case attrkindAssemblyDelaySign_Bool:
            if (pSourceFile)
            {
                bool fDelaySign = false;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &fDelaySign,
                    NULL);

                if (fDelaySign)
                {
                    SourceFile::GetCurrentErrorTableForFile( pSourceFile )->CreateError(
                        WRNID_UseSwitchInsteadOfAttribute,
                        pLoc,
                        L"/delaysign",
                        ASSEMBLYDELAYSIGNATTRIBUTE);
                }

                m_pcompilerproj->SetDelaySign(fDelaySign);
            }
            break;
        case attrkindAssemblyFlags_AssemblyNameFlags:
        case attrkindAssemblyFlags_UInt:        
        case attrkindAssemblyFlags_Int:
            if (pSourceFile)
            {
                int AssemblyFlags = 0;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &AssemblyFlags,
                    NULL);

                m_pcompilerproj->GetAssemblyIdentity()->SetAssemblyFlags(
                    AssemblyFlags, 
                    pAttribute->m_attrkind == attrkindAssemblyFlags_Int 
                    ? AssemblyFlagsAttributeIntCtor
                    : pAttribute->m_attrkind == attrkindAssemblyFlags_UInt
                        ? AssemblyFlagsAttributeUIntCtor
                        : AssemblyFlagsAttributeAssemblyNameFlagsCtor);        
            }
            break;
        case attrkindDefaultMember_String:
            {
                WCHAR* pwszDefaultMemberName;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszDefaultMemberName,
                    NULL);

                pattrvals->SetDefaultMemberData(pwszDefaultMemberName);
                break;
            }

        case attrkindSecurityCritical_Scope:        
        case attrkindSecurityCritical_Void:
            {
                pattrvals->SetSecurityCriticalData();               
                break;          
            }

        case attrkindSecuritySafeCritical_Void:
            {
                pattrvals->SetSecuritySafeCriticalData();               
                break;          
            }

        case attrkindEditorBrowsable_Void:
            // Set to the default value: EditorBrowsableState.Always = 0
            pattrvals->SetEditorBrowsableData(0 /* editorBrowsableState */);
            break;

        case attrkindEditorBrowsable_EditorBrowsableState:
            {
                long editorBrowsableState;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &editorBrowsableState,
                    NULL);

                pattrvals->SetEditorBrowsableData(editorBrowsableState);
            break;
            }

        case attrkindDllImport_String:
            pattrvals->SetDllImportData();
            // We ignore all of the DllImport's parameters and named properties
            break;

        case attrkindWebMethod_Void_NamedProps:
        case attrkindWebMethod_Boolean_NamedProps:
        case attrkindWebMethod_Boolean_Transaction_NamedProps:
        case attrkindWebMethod_Boolean_Transaction_Int32_NamedProps:
        case attrkindWebMethod_Boolean_Transaction_Int32_Boolean_NamedProps:
            {
                AttrKind attrKind = pAttribute->m_attrkind;

                // Set default values
                bool fEnableSession = false;
                short transactionOption = 0; //TransactionOption.Disabled
                int cacheDuration = 0;
                bool fBufferResponse = true;
                WCHAR* pwszDescription = NULL;

                //
                // Set up named props (string Description)
                //
                namedprops.m_cProps = 1;
                namedprops.m_rgwszPropName[0] = L"Description";
                namedprops.m_rgpvPropVal[0] = &pwszDescription;

                switch (pAttribute->m_attrkind)
                {
                    case attrkindWebMethod_Void_NamedProps:
                        ----AttributeData(
                            &namedprops,
                            m_pnra,
                            pAttribute->m_sig,
                            pbArgBlob,
                            NULL);
                        break;

                    case attrkindWebMethod_Boolean_NamedProps:
                        ----AttributeData(
                            &namedprops,
                            m_pnra,
                            pAttribute->m_sig,
                            pbArgBlob,
                            &fEnableSession,
                            NULL);
                        break;

                    case attrkindWebMethod_Boolean_Transaction_NamedProps:
                        ----AttributeData(
                            &namedprops,
                            m_pnra,
                            pAttribute->m_sig,
                            pbArgBlob,
                            &fEnableSession,
                            &transactionOption,
                            NULL);
                        break;

                    case attrkindWebMethod_Boolean_Transaction_Int32_NamedProps:
                        ----AttributeData(
                            &namedprops,
                            m_pnra,
                            pAttribute->m_sig,
                            pbArgBlob,
                            &fEnableSession,
                            &transactionOption,
                            &cacheDuration,
                            NULL);
                        break;

                    case attrkindWebMethod_Boolean_Transaction_Int32_Boolean_NamedProps:
                        ----AttributeData(
                            &namedprops,
                            m_pnra,
                            pAttribute->m_sig,
                            pbArgBlob,
                            &fEnableSession,
                            &transactionOption,
                            &cacheDuration,
                            &fBufferResponse,
                            NULL);
                         break;

                    }

                pattrvals->SetWebMethodData(attrKind, fEnableSession, transactionOption, cacheDuration, fBufferResponse, pwszDescription);
            }
            break;

        case attrkindSTAThread_Void:
            pattrvals->SetSTAThreadData();
            break;

        case attrkindMTAThread_Void:
            pattrvals->SetMTAThreadData();
            break;

        case attrkindDebuggable_Bool_Bool:
        case attrkindDebuggable_DebuggingModes:
            pattrvals->SetDebuggableData();
            break;

        case attrkindDebuggerHidden_Void:
            pattrvals->SetDebuggerHiddenData();
            break;

        case attrkindMarshalAs_UnmanagedType:
        case attrkindMarshalAs_Int16:
            // This is a "pseudo-custom attribute". Users can attach it to their parameters,
            // but in the IL it is actually stored as a parameter flag.
            pattrvals->SetMarshalAsData();
            break;

        case attrkindPrimaryInteropAssembly_Int32_Int32:
            pattrvals->SetPrimaryInteropAssemblyData();
            break;

        case attrkindStructLayout_Int16:
            {
                __int16 structLayoutData = 0;
                ----AttributeData(
                   &namedprops,
                   m_pnra,
                   pAttribute->m_sig,
                   pbArgBlob,
                   &structLayoutData,
                   NULL);
                pattrvals->SetStructLayoutDataByLayoutKind(structLayoutData);
            }
            break;

        case attrkindStructLayout_LayoutKind: 
            {
                __int32 structLayoutData = 0;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &structLayoutData,
                    NULL);
                pattrvals->SetStructLayoutDataByLayoutKind(structLayoutData);
            }
            break;

        case attrkindMethodImpl_Int16:
            {
                __int16 methodImplData = 0;
                ----AttributeData(
                   &namedprops,
                   m_pnra,
                   pAttribute->m_sig,
                   pbArgBlob,
                   &methodImplData,
                   NULL);
                pattrvals->SetMethodImplOptionData(methodImplData);
            }
            break;

        case attrkindMethodImpl_MethodImplOptions:
            {
                __int32 methodImplData = 0;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &methodImplData,
                    NULL);
                pattrvals->SetMethodImplOptionData(methodImplData);
            }
            break;

        case attrkindInterfaceType_Int16:
            {
            // Need to decode into an int16, but store in m_COM2InteropData.m_InterfaceType
            // (which is a long, to handle the other CTOR flavor of this attribute)
                __int16 InterfaceType_Int16;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &InterfaceType_Int16,
                    NULL);
                pattrvals->SetInterfaceTypeData(InterfaceType_Int16);
            }
            break;

        case attrkindInterfaceType_ComInterfaceType:
            {
                long interfaceType_Long;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &interfaceType_Long,
                    NULL);
                pattrvals->SetInterfaceTypeData(interfaceType_Long);
            }
            break;

        case attrkindTypeLibType_Int16:
            {
                short typeLibType;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &typeLibType,
                    NULL);

                pattrvals->SetTypeLibTypeData(typeLibType);
            
                break;
            }

        case attrkindTypeLibType_TypeLibTypeFlags:
            {
                __int32 typeLibTypeData = 0;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &typeLibTypeData,
                    NULL);
                pattrvals->SetTypeLibTypeData(typeLibTypeData);
            break;
        }

        case attrkindTypeLibVar_Int16:
            {
                short typeLibVar;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &typeLibVar,
                    NULL);

                pattrvals->SetTypeLibVarData(typeLibVar);
                break;
            }

        case attrkindTypeLibVar_TypeLibVarFlags:
            {
                __int32 typeLibVarData = 0;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &typeLibVarData,
                    NULL);

                pattrvals->SetTypeLibVarData(typeLibVarData);
                break;
            }

        case attrkindBestFitMapping_Bool:
            {
                bool fMapping = true;
                bool fThrow = false;

                namedprops.m_cProps = 1;
                namedprops.m_rgwszPropName[0] = L"ThrowOnUnmappableChar";
                namedprops.m_rgpvPropVal[0] = &fThrow;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &fMapping,
                    NULL);

                pattrvals->SetBestFitMappingData(fMapping, fThrow);

                break;
            }

        case attrkindFieldOffset_Int32:
            {
                int offset;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &offset,
                    NULL);

                pattrvals->SetFieldOffsetData(offset);
                break;
            }

        case attrkindLCIDConversion_Int32:
            {
                int conversionData;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &conversionData,
                    NULL);

                pattrvals->SetLCIDConversionData(conversionData);
                break;
            }

        case attrkindImportedFromTypeLib_String:
            {
                WCHAR* pswzImportedFromTypeLib;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pswzImportedFromTypeLib,
                    NULL);
                pattrvals->SetImportedFromTypeLibData(pswzImportedFromTypeLib);
                break;
            }

        case attrkindIn_void:
            pattrvals->SetInData();
            break;

        case attrkindOut_void:
            pattrvals->SetOutData();
            break;

        case attrkindUnmanagedFunctionPointer_CallingConvention:
            {
                long functionPointerData;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &functionPointerData,
                    NULL);

                pattrvals->SetUnmanagedFunctionPointerData(functionPointerData);
                break;
            }

        case attrkindClassInterface_Int16:
            {
                short classInterfaceType;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &classInterfaceType,
                    NULL);
                pattrvals->SetClassInterfaceData(classInterfaceType);
                break;
            }

        case attrkindClassInterface_ClassInterfaceType:
            {
                __int32 classInterfaceData = 0;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &classInterfaceData,
                    NULL);

                pattrvals->SetClassInterfaceData(classInterfaceData);
                break;
            }
        case attrkindTypeLibFunc_Int16:
            {
                short typeLibFuncData;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &typeLibFuncData,
                    NULL);

                pattrvals->SetTypeLibFuncData(typeLibFuncData);
                break;
            }

        case attrkindTypeLibFunc_TypeLibFuncFlags:
            {
                __int32 typeLibFuncData = 0;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &typeLibFuncData,
                    NULL);

                pattrvals->SetTypeLibFuncData(typeLibFuncData);
                break;
            }

        case attrkindCOMVisible_Bool:
            {
                bool fVisible;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &fVisible,
                    NULL);
                pattrvals->SetCOMVisibleData(fVisible);
                break;
            }

        case attrkindDesignerCategory_Void:
            pattrvals->SetDesignerCategoryData(NULL /* pwszDesignerCategory */);
            break;

        case attrkindDesignerCategory_String:
            {
                WCHAR* pwszCategory;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszCategory,
                    NULL);

                pattrvals->SetDesignerCategoryData(pwszCategory);
                break;
            }

        case attrkindAccessedThroughProperty_String:
            {
                WCHAR* pwszProperty;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszProperty,
                    NULL);

                pattrvals->SetAccessedThroughPropertyData(pwszProperty);
                break;
            }

        case attrkindCompilationRelaxations_Int32:
        case attrkindCompilationRelaxations_Enum:
            if (isAssembly)
            {
                pattrvals->SetCompilationRelaxationsData();
            }
            break;

        case attrkindRuntimeCompatibility_Void:
            pattrvals->SetRuntimeCompatibilityData();
            break;

        case attrkindDesignerGenerated_Void:
            pattrvals->SetDesignerGeneratedData();
            break;

        case attrkindStandardModule_Void:
            pattrvals->SetStandardModuleData();
            break;

        case attrkindOptionCompare_Void:
            pattrvals->SetOptionCompareData();
            break;

        case attrkindGuid_String:
            {
                WCHAR* pwszGuid;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszGuid,
                    NULL);

                pattrvals->SetGuidData(pwszGuid);
                break;
            }

        case attrkindTypeIdentifier_void:
            pattrvals->SetTypeIdentifierData();

            // Dev10 #735384
            if ( m_pMetaDataFile != NULL ) // m_pMetaDataFile != NULL implies that the atribute is from metadata.
            {
                m_pMetaDataFile->GetProject()->SetPotentiallyEmbedsPiaTypes(true);
            }

            break;

        case attrkindTypeIdentifier_String_String:
            {
                WCHAR* pwszGuid;
                WCHAR* pwszName;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszGuid,
                    &pwszName,
                    NULL);

                pattrvals->SetTypeIdentifierData(pwszGuid, pwszName);

                // Dev10 #735384
                if ( m_pMetaDataFile != NULL ) // m_pMetaDataFile != NULL implies that the atribute is from metadata.
                {
                    m_pMetaDataFile->GetProject()->SetPotentiallyEmbedsPiaTypes(true);
                }

                break;
            }

        case attrkindComClass_Void:
        case attrkindComClass_String:
        case attrkindComClass_String_String:
        case attrkindComClass_String_String_String:
        {
            WellKnownAttrVals::ComClassData comClassData;

            //
            // Set up named props (bool Shadows and bool Creatable)
            //
            namedprops.m_cProps = 1;
            namedprops.m_rgwszPropName[0] = L"InterfaceShadows";
            namedprops.m_rgpvPropVal[0] = &comClassData.m_fShadows;

            // Set up the parameters depending on which constructor was used
            {
                const WCHAR **opt_classid = NULL;
                const WCHAR **opt_interfaceid = NULL;
                const WCHAR **opt_eventid = NULL;

                switch (pAttribute->m_attrkind)
                {
                case attrkindComClass_Void:
                    break;
                case attrkindComClass_String:
                        opt_classid = &comClassData.m_ClassId;
                        break;
                    case attrkindComClass_String_String:
                        opt_classid = &comClassData.m_ClassId;
                        opt_interfaceid = &comClassData.m_InterfaceId;
                        break;
                    case attrkindComClass_String_String_String:
                        opt_classid = &comClassData.m_ClassId;
                        opt_interfaceid = &comClassData.m_InterfaceId;
                        opt_eventid = &comClassData.m_EventId;
                    break;
                }

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    opt_classid,
                    opt_interfaceid,
                    opt_eventid,
                    NULL);
            }

            if (psym->IsClass())
            {
                comClassData.m_PartialClassOnWhichApplied = psym->PClass();
            }

            pattrvals->SetComClassData(comClassData);

            break;
        }
        case attrkindComImport_Void:
            pattrvals->SetComImportData();
            break;

        case attrkindComEventInterface_Type_Type:
            {
                WellKnownAttrVals::ComEventInterfaceData comEventInterfaceData;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &comEventInterfaceData.m_SourceInterface,
                    &comEventInterfaceData.m_EventProvider,
                    NULL);
                comEventInterfaceData.m_SourceInterface =
                            ConvertSerializedSystemTypeToTypeName(comEventInterfaceData.m_SourceInterface, m_pnra);
                comEventInterfaceData.m_EventProvider =
                            ConvertSerializedSystemTypeToTypeName(comEventInterfaceData.m_EventProvider, m_pnra);

                pattrvals->SetComEventInterfaceData(comEventInterfaceData);
                break;
            }

        case attrkindComSourceInterfaces_String:
        case attrkindComSourceInterfaces_Type:
        case attrkindComSourceInterfaces_Type_Type:
        case attrkindComSourceInterfaces_Type_Type_Type:
        case attrkindComSourceInterfaces_Type_Type_Type_Type:
            pattrvals->SetComSourceInterfacesData();
            break;

        case attrkindDispId_Int32:
            {
                long dispId;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &dispId,
                    NULL);

                pattrvals->SetDispIdData(dispId);
                break;
            }

        case attrkindCoClass_Type:
            {
                WCHAR* pwszCoClassTypeName;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszCoClassTypeName,
                    NULL);

                pwszCoClassTypeName = ConvertSerializedSystemTypeToTypeName(pwszCoClassTypeName, m_pnra);

                pattrvals->SetCoClassData(pwszCoClassTypeName);
                break;
            }
            
        case attrkindCategory_Void:
           pattrvals->SetCategoryData(NULL);
           break;

        case attrkindCategory_String:
            {
                WCHAR* pwszCategoryName;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszCategoryName,
                    NULL);
                
                pattrvals->SetCategoryData(pwszCategoryName);
                
                break;
            }
        case attrkindMyGroupCollection_String_String_String_String:
            {
                WellKnownAttrVals::MyGroupCollectionData* pCollectionData;
                bool hasMyGroupCollection = pattrvals->GetMyGroupCollectionData(&pCollectionData);

                VSASSERT(hasMyGroupCollection == false && pCollectionData == NULL, "Un initialized MyGroupColection data");
                // ignore for non classes, generics or partials
                if (pSourceFile &&
                    psym->IsClass() &&
                    !psym->PClass()->IsPartialType() &&
                    !IsGenericOrHasGenericParent(psym->PClass()))
                {
                    WCHAR* pwszGroupName = NULL;
                    WCHAR* pwszCreateMethod = NULL;
                    WCHAR* pwszDisposeMethod=NULL;
                    WCHAR* pwszDefaultInstance = NULL;
                    ----AttributeData(
                        &namedprops,
                        m_pnra,
                        pAttribute->m_sig,
                        pbArgBlob,
                        &pwszGroupName,
                        &pwszCreateMethod,
                        &pwszDisposeMethod,
                        &pwszDefaultInstance,
                        NULL);
                    //internalize the group name string for faster compare
                    if (pwszGroupName != NULL && *pwszGroupName != L'\0'
                        && pwszCreateMethod != NULL && *pwszCreateMethod != L'\0'
                        //&& pwszDisposeMethod != NULL && *pwszDisposeMethod != L'\0' //no set prop if dispose is missing
                        )
                    {
                        bool defaultInstanceEnabled = psym->PClass()->GetCompilerFile()->IsSolutionExtension() &&
                            pwszDefaultInstance != NULL && wcslen(pwszDefaultInstance);

                        WCHAR *tokContextName = NULL;
                        WCHAR *tokContextCreate = NULL;
                        WCHAR *tokContextDispose = NULL;
                        WCHAR *tokContextDefaultI = NULL;


                        WCHAR *pwszName = MyWcstok_s(pwszGroupName, &tokContextName);
                        WCHAR *pwszCreate = MyWcstok_s(pwszCreateMethod, &tokContextCreate);
                        WCHAR *pwszDispose = MyWcstok_s(pwszDisposeMethod, &tokContextDispose);
                        WCHAR *pwszDefaultI = defaultInstanceEnabled ? MyWcstok_s(pwszDefaultInstance, &tokContextDefaultI) : NULL;

                        while ( pwszName != NULL
                            && pwszCreate != NULL
                            // && pwszDispose != NULL // no set property if dispose missing
                            )
                        {
                            if (!pattrvals->GetMyGroupCollectionData(&pCollectionData))
                            {
                                // first time use, create the data
                                pCollectionData = new WellKnownAttrVals::MyGroupCollectionData();
                                pattrvals->SetMyGroupCollectionData(pCollectionData);
                            }

                            WellKnownAttrVals::MyGroupCollectionBase &pf = pCollectionData->Add();
                            pf.m_GroupName = m_pcompiler->AddString(pwszName);
                            pf.m_CreateMethod = m_pcompiler->AddString(pwszCreate);
                            pf.m_DisposeMethod = (pwszDispose != NULL) ? m_pcompiler->AddString(pwszDispose) : NULL;
                            pf.m_DefaultInstance = (pwszDefaultI != NULL) ? m_pcompiler->AddString(pwszDefaultI) : NULL;

                            pwszName = MyWcstok_s(NULL, &tokContextName);
                            pwszCreate = MyWcstok_s(NULL, &tokContextCreate);
                            pwszDispose = MyWcstok_s(NULL, &tokContextDispose);
                            pwszDefaultI = defaultInstanceEnabled ? MyWcstok_s(NULL, &tokContextDefaultI) : NULL;
                        }
                    }
#if DEBUG
                    else
                    {
                        VSDEBUGPRINTIF(
                            VSFSWITCH(fMyGroupAndDefaultInst),
                            "MyGroup: Bad MyGroupCollection attribute on '%S'\n",  psym->PClass()->GetName());
                    }
#endif //DEBUG
                }
#if DEBUG
                else
                {
                    VSDEBUGPRINTIF(
                        VSFSWITCH(fMyGroupAndDefaultInst),
                        "MyGroup: MyGroupCollection attribute ignored on '%S' (metadata/not a class/generic/partial)\n",
                        psym->IsClass() ? psym->PClass()->GetName() : L" ");
                }
#endif //DEBUG

                break;
            }

        case attrkindHideModuleName_Void:
            pattrvals->SetHideModuleNameData();
            break;

        case attrkindEmbedded_Void:
            pattrvals->SetEmbeddedData();
            break;

       case attrkindDefaultEvent_String:
            {
                WCHAR* pwszDefaultEvent = NULL;
                BCSYM_Class* pPartialClassOnWhichApplied = NULL;

                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &pwszDefaultEvent,
                    NULL);

                if (psym->IsClass())
                {
                    pPartialClassOnWhichApplied = psym->PClass();
                }

                pattrvals->SetDefaultEventData(pwszDefaultEvent, pPartialClassOnWhichApplied);
                break;
            }

        // Don't know what to do with these two constructors:
        // Our UI won't support them, but we still need to know that they
        // were present in the code. As a side note, the
        // attrKindDefaultValue_Object constructor can only be
        // present in imported types, because the VB Compiler can't compile
        // the constants of type object required by this particular
        // constructor.
        case attrkindDefaultValue_Object:
        case attrkindDefaultValue_Type_String:
            {

                // Clear default value
                ConstantValue defaultValue;
                defaultValue.TypeCode = t_bad;
                defaultValue.Mask.First = 0;
                defaultValue.Mask.Second = 0;

                // Set not handled flag
                bool fIsNotHandled = true;

                pattrvals->SetDefaultValueData(defaultValue, fIsNotHandled);

                break;
            }

        case attrkindDefaultValue_Bool:
        {
            bool Value;
            ----AttributeData(
                               &namedprops,
                               m_pnra,
                               pAttribute->m_sig,
                               pbArgBlob,
                               &Value,
                               NULL);

                ConstantValue defaultValue;
                defaultValue.Integral = Value;
                defaultValue.TypeCode = t_bool;

                bool fIsNotHandled = false;

                pattrvals->SetDefaultValueData(defaultValue, fIsNotHandled);
            break;
        }

        case attrkindDefaultValue_Byte:
        {
            unsigned char Value;
            ----AttributeData(
                               &namedprops,
                               m_pnra,
                               pAttribute->m_sig,
                               pbArgBlob,
                               &Value,
                               NULL);

                ConstantValue defaultValue;
                defaultValue.Integral = Value;
                defaultValue.TypeCode = t_ui1;

                bool fIsNotHandled = false;

                pattrvals->SetDefaultValueData(defaultValue, fIsNotHandled);
            break;
        }

        case attrkindDefaultValue_Char:
            {
                WCHAR Value;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &Value,
                    NULL);

                ConstantValue defaultValue;
                defaultValue.Integral = Value;
                defaultValue.TypeCode = t_char;

                bool fIsNotHandled = false;

                pattrvals->SetDefaultValueData(defaultValue, fIsNotHandled);
                break;
            }

        case attrkindDefaultValue_Double:
            {
                ConstantValue defaultValue;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &defaultValue.Double,
                    NULL);

                defaultValue.TypeCode = t_double;

                bool fIsNotHandled = false;

                pattrvals->SetDefaultValueData(defaultValue, fIsNotHandled);
                break;
            }

        case attrkindDefaultValue_Int16:
        {
            short Value;
            ----AttributeData(
                &namedprops,
                m_pnra,
                pAttribute->m_sig,
                pbArgBlob,
                &Value,
                NULL);

            ConstantValue defaultValue;
            defaultValue.Integral = Value;
            defaultValue.TypeCode = t_i2;

            bool fIsNotHandled = false;

            pattrvals->SetDefaultValueData(defaultValue, fIsNotHandled);
            break;
        }

        case attrkindDefaultValue_Int32:
        {
            long Value;
            ----AttributeData(
                &namedprops,
                m_pnra,
                pAttribute->m_sig,
                pbArgBlob,
                &Value,
                NULL);

            ConstantValue defaultValue;
            defaultValue.Integral = Value;
            defaultValue.TypeCode = t_i4;

            bool fIsNotHandled = false;

            pattrvals->SetDefaultValueData(defaultValue, fIsNotHandled);
            break;
        }

        case attrkindDefaultValue_Int64:
            {
                __int64 Value;
                ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                    &Value,
                    NULL);

                ConstantValue defaultValue;
                defaultValue.Integral = Value;
                defaultValue.TypeCode = t_i8;

                bool fIsNotHandled = false;

                pattrvals->SetDefaultValueData(defaultValue, fIsNotHandled);
                break;
            }

        case attrkindDefaultValue_Single:
            {
                ConstantValue defaultValue;
                ----AttributeData(
                                   &namedprops,
                                   m_pnra,
                                   pAttribute->m_sig,
                                   pbArgBlob,
                                   &defaultValue.Single,
                                   NULL);

                defaultValue.TypeCode = t_single;

                bool fIsNotHandled = false;

                pattrvals->SetDefaultValueData(defaultValue, fIsNotHandled);
                break;
            }

        case attrkindDefaultValue_String:
            {
                ConstantValue defaultValue;
                ----AttributeData(
                                   &namedprops,
                                   m_pnra,
                                   pAttribute->m_sig,
                                   pbArgBlob,
                                   &defaultValue.String.Spelling,
                               NULL);

            //WARNING: Microsoft 9/23/2002: encoded string might contain NULL characters,
            //in which case all string handling for attributes won't work.
            //This shouldn't be a common scenario so no special handling will be done
            // at this time in ----AttributeData to store strings with their lengths in
            // a special structure. If however, this turns out to be a scenario, all structures
            // that sotre well known attribute data as strings should be changed to include
            // the length of the string as well.
                if (defaultValue.String.Spelling)
                {
                    defaultValue.String.LengthInCharacters =
                            (int)wcslen(defaultValue.String.Spelling);
                }
                else
                {
                    defaultValue.String.LengthInCharacters = 0;
                }

                defaultValue.TypeCode = t_string;

                bool fIsNotHandled = false;

                pattrvals->SetDefaultValueData(defaultValue, fIsNotHandled);
                break;
            }
        case attrkindHelpKeyword_Void:
            pattrvals->SetHelpKeywordData(NULL /* pwszHelpKeyword */);
            break;

        case attrkindHelpKeyword_String:
            {
                WCHAR* pwszHelpKeyword;

            ----AttributeData(
                    &namedprops,
                    m_pnra,
                    pAttribute->m_sig,
                    pbArgBlob,
                        &pwszHelpKeyword,
                    NULL);
                pattrvals->SetHelpKeywordData(pwszHelpKeyword);

            break;
            }

        case attrkindHelpKeyword_Type:
            {
                WCHAR* pwszHelpKeyword;

                ----AttributeData(
                        &namedprops,
                        m_pnra,
                        pAttribute->m_sig,
                        pbArgBlob,
                        &pwszHelpKeyword,
                        NULL);

                pwszHelpKeyword = ConvertSerializedSystemTypeToTypeName(pwszHelpKeyword, m_pnra);

                pattrvals->SetHelpKeywordData(pwszHelpKeyword);

                break;
            }

        case attrkindNonSerialized_Void:
            pattrvals->SetNonSerializedData();
            break;

        case attrkindSerializable_Void:
            pattrvals->SetSerializableData();
            break;

        case attrkindWebService_Void_NamedProps:
            {
                WCHAR* pwszNamespace;
                WCHAR* pwszDescription;
                WCHAR* pwszName;

                //
                // Set up named props (WCHAR Name, WCHAR Namespace WCHAR Description)
                //
                namedprops.m_cProps = 3;
                namedprops.m_rgwszPropName[0] = L"Namespace";
                namedprops.m_rgpvPropVal[0] = &pwszNamespace;
                namedprops.m_rgwszPropName[1] = L"Description";
                namedprops.m_rgpvPropVal[1] = &pwszDescription;
                namedprops.m_rgwszPropName[2] = L"Name";
                namedprops.m_rgpvPropVal[2] = &pwszName;

                ----AttributeData(
                                   &namedprops,
                                   m_pnra,
                                   pAttribute->m_sig,
                                   pbArgBlob,
                                   NULL);

                pattrvals->SetWebServiceData(pwszNamespace, pwszDescription, pwszName);

                break;
            }

        case attrkindInternalsVisibleTo_String:
            {
                WCHAR *pwcsFriendDeclaration = NULL;

                ----AttributeData(
                        &namedprops,
                        m_pnra,
                        pAttribute->m_sig,
                        pbArgBlob,
                        &pwcsFriendDeclaration,
                        NULL);

                // It's possible to get back NULL here if the user passed in the keyword
                // Nothing to the attribute data.  We are expecting a non-null string in
                // the friend handling code so convert to an empty string since there won't
                // ever be an assembly with a fully qualified name of ""
                const WCHAR *toAdd = pwcsFriendDeclaration ? pwcsFriendDeclaration : L"" ;

                // If we have a source file, then it means that we are cracking in bindable.
                // So we need to store the information and then process it after all the other
                // attributes are ----ed. Otherwise, it's a meta file, and we can just
                // add this directly.
                if( pSourceFile != NULL )
                {
                    VSASSERT( pLoc, "Source files should have a location for attribute!" );
                    m_pcompilerproj->AddUnprocessedFriend(
                            m_pcompiler->AddString( toAdd ),
                            pSourceFile,
                            pLoc
                            );
                }
                else
                {
                    m_pcompilerproj->AddFriend(
                            m_pcompiler->AddString( toAdd ),
                            NULL,
                            NULL
                            );
                }
                break;
            }

        case attrkindExtension_Void:
            {
                pattrvals->SetExtensionData();

                // If this is a method then record the name in the ExtensionMethodExistsCache.
                // The is one cache per project, and each cache contains the names of any extension method found.
                // This cache speed up name lookup by allowing VB to skip extension method lookup for names not
                // found in the cache.  This cache can be eliminated if VB moves extension method lookup to a later point
                // in semantics.
                if (psym->IsProc())
                {
                    STRING *name = psym->PProc()->GetName();

                    HashSet<STRING_INFO*> *pExtensionMethodExists =
                        m_pcompilerproj->GetExtensionMethodExistsCache();

                    STRING_INFO *nameInfo = StringPool::Pstrinfo(name);

                    pExtensionMethodExists->Add(nameInfo);
                }
            
            }
            break;

       case attrkindDefaultParameterValue_Object:
            if ( m_pMetaDataFile != NULL ) // m_pMetaDataFile != NULL implies that the atribute is from a metafile.
            {
                BYTE* argBlob = (BYTE*)m_pnra->Alloc(cbArgBlob);
                memcpy(argBlob, pbArgBlob, cbArgBlob);

                pattrvals->SetDefaultParamValueData(argBlob, cbArgBlob);
            }
            break;

       case attrkindCallerLineNumber_Void:
           pattrvals->SetCallerLineNumberData();
           break;

       case attrkindCallerFilePath_Void:
           pattrvals->SetCallerFilePathData();
           break;

       case attrkindCallerMemberName_Void:
           pattrvals->SetCallerMemberNameData();
           break;

        default:
            VSFAIL("How did we get here?");
            break;
    }
Error:;
}


//============================================================================
// Processes the named properties/fields found in the arg blob pointed
// to by *ppbArgBlob.
//============================================================================
void Attribute::----NamedProperties
(
    int             cNamedProps,    // Number of named properties in data blob
    NamedProps     *pnamedprops,    // IN OUT: Named props to look for
    NorlsAllocator *pnra,           // IN: Nra to use for allocation
    BYTE          **ppbArgBlob
)
{
    while (cNamedProps-- > 0)
    {
        WCHAR *pwszPropName;
        BYTE   bType;
        bool   fValue;

        // Confirm and consume the field/prop byte
        VSASSERT(**ppbArgBlob == SERIALIZATION_TYPE_FIELD ||
                 **ppbArgBlob == SERIALIZATION_TYPE_PROPERTY,
                 "Bogus field/property field.");
        (*ppbArgBlob)++;

        // Consume the property's type
        bType = **ppbArgBlob;
        (*ppbArgBlob)++;

        // Decode the property's name
        pwszPropName = ----String(ppbArgBlob, pnra);

#if 0   // Contrary to the spec, the literal's type does NOT get encoded.
        bTypeLiteral = **ppbArgBlob;
        (*ppbArgBlob)++;
#endif

#pragma warning(disable:22004)//,"pnamedprops->m_cProps is bounded by the 'for' loop")
        VSASSERT(pnamedprops->m_cProps <= NamedProps::m_cPropsMax,
            "Property storage overflow.");
        if (pnamedprops->m_cProps > NamedProps::m_cPropsMax)
        {
            VSFAIL("Bad boundaries.");
            return;
        }
        // See if this property name matches one in pnamedprops
        bool needSync = false;
        bool propFound = false;
        for (int i = 0; i < pnamedprops->m_cProps; i++)
        {
            if (CompareNoCase(pwszPropName, pnamedprops->m_rgwszPropName[i]) == 0)
            {
                propFound = true;
                switch (bType)
                {
                    case ELEMENT_TYPE_BOOLEAN:
                        // Consume the boolean data
                        fValue = ----Bool(ppbArgBlob);
                        *(bool *)(pnamedprops->m_rgpvPropVal[i]) = fValue;
                        break;

                    case ELEMENT_TYPE_STRING:
                        // Consume the string data
                        *(WCHAR **)(pnamedprops->m_rgpvPropVal[i]) = ----String(ppbArgBlob, pnra);
                        break;

                    default:
                        // We only implement the types required by the named properties
                        // of well-known attributes.  For now, the only type we need
                        // to support is boolean.
                        needSync = true;
                        VSFAIL("Do we need to implement another ELEMENT_TYPE for named properties?");
                }

                break;
            }
        }
        // if the property is not found or has some unknown type try to consume the value
        // and land smoothly for the next prop
        if (!propFound || needSync)
        {
            switch (bType)
            {
                case SERIALIZATION_TYPE_TYPE:
                    // The caller is responsible for calling ConvertSerializedSystemTypeToTypeName
                    // to obtain the fully-qualified name if that is the desired information.
                    ----String(ppbArgBlob, NULL);
                    break;

                case ELEMENT_TYPE_STRING:
                    ----String(ppbArgBlob, NULL);
                    break;

                case ELEMENT_TYPE_BOOLEAN:
                    ----Bool(ppbArgBlob);
                    break;

                case ELEMENT_TYPE_U1:
                    ----I1(ppbArgBlob);
                    break;

                case ELEMENT_TYPE_I2:
                case ELEMENT_TYPE_U2:
                case ELEMENT_TYPE_CHAR:
                    ----I2(ppbArgBlob);
                    break;

                case ELEMENT_TYPE_U:
                case ELEMENT_TYPE_I4:
                case ELEMENT_TYPE_U4:
                    ----I4(ppbArgBlob);
                    break;

                case ELEMENT_TYPE_I8:
                case ELEMENT_TYPE_U8:
                    ----I8(ppbArgBlob);
                    break;

                case ELEMENT_TYPE_R4:
                    ----R4(ppbArgBlob);
                    break;

                case ELEMENT_TYPE_R8:
                    ----R8(ppbArgBlob);
                    break;

                default:
                    VSFAIL("Do we need to implement cracking a new ELEMENT_TYPE?");
                    return;
            }
        }
#pragma warning(default:22004)//,"pnamedprops->m_cProps is bounded by the 'for' loop")
    }
}


//============================================================================
// Parses the arg blob in pbArgBlob accoring to the attribute's signature
// (pbSignature) and possible named properties (pnamedprops).
//============================================================================
void Attribute::----AttributeData
(
    NamedProps      *pnamedprops,   // IN OUT
    NorlsAllocator  *pnra,          // IN: Nra to use for allocation
    PCCOR_SIGNATURE  psignatureOrig,// IN: Constructor's signature
    BYTE            *pbArgBlob,     // IN: Attribute's arg blob (serialized params and props)
    ...                             // OUT -- NULL-terminated list of output locations
)
{
    va_list  arglist;
    int      cargs;
    __int16  cNamedProps;

    // It's easier to parse the sig as a (BYTE *) than as a PCCOR_SIGNATURE.
    BYTE    *pbsig = (BYTE *)psignatureOrig;

    // Verify and discard calling convention
    VSASSERT(*pbsig == IMAGE_CEE_CS_CALLCONV_HASTHIS, "Wrong calling convention.");
    pbsig++;

    // Consume and verify the number of args.
    // NOTE: The test against 6 is rather random -- increase if required.
    cargs = *pbsig++;
    VSASSERT(cargs < 6,  "What well-known custom attribute has this many args?");
    VSASSERT(cargs >= 0, "Sign extention problems?");

    // Verify and consume return type (must be VOID for constructors)
    VSASSERT(*pbsig == ELEMENT_TYPE_VOID, "Return type of CTOR must be VOID.");
    pbsig++;

    // For each CTOR argument, get its type, read it out of pbArgBlob, and return
    // via OUT param.  Note that we only implement this for ELEMENT_TYPEs that
    // we need to parse for well-known attributes.  This list may grow
    va_start(arglist, pbArgBlob);
    while (cargs > 0)
    {
    LSwitchAgain:
        switch (*pbsig)
        {
            case ELEMENT_TYPE_CLASS:
            case ELEMENT_TYPE_VALUETYPE:
                // Ignore CLASS/VALUETYPE byte.  Next byte holds real type of argument.
                // Can't just loop around because we haven't consumed an arg yet.
                pbsig++;
                goto LSwitchAgain;

            case SERIALIZATION_TYPE_TYPE:
                // The caller is responsible for calling ConvertSerializedSystemTypeToTypeName
                // to obtain the fully-qualified name if that is the desired information.

                // fall through, because it ends up being the same case as string.

            case ELEMENT_TYPE_STRING:
            {
                WCHAR **wszStringArgument = va_arg(arglist, WCHAR **);

                if (wszStringArgument)
                {
                    *wszStringArgument = ----String(&pbArgBlob, pnra);
                }
                else
                {
                    ----String(&pbArgBlob, NULL);
                }
                break;
            }

            case ELEMENT_TYPE_BOOLEAN:
                *va_arg(arglist, bool *)= ----Bool(&pbArgBlob);
                break;

            case ELEMENT_TYPE_U1:
                *va_arg(arglist, __int8 *) = ----I1(&pbArgBlob);
                break;

            case ELEMENT_TYPE_I2:
            case ELEMENT_TYPE_U2:
            case ELEMENT_TYPE_CHAR:
                *va_arg(arglist, __int16 *) = ----I2(&pbArgBlob);
                break;

            case ELEMENT_TYPE_U:
            case ELEMENT_TYPE_I4:
            case ELEMENT_TYPE_U4:
                *va_arg(arglist, __int32 *)= ----I4(&pbArgBlob);
                break;

            case ELEMENT_TYPE_I8:
            case ELEMENT_TYPE_U8:
                *va_arg(arglist, __int64 *)= ----I8(&pbArgBlob);
                break;

            case ELEMENT_TYPE_R4:
                *va_arg(arglist,  float *)= ----R4(&pbArgBlob);
                break;

            case ELEMENT_TYPE_R8:
                *va_arg(arglist, double *)= ----R8(&pbArgBlob);
                break;

            default:
                VSFAIL("Do we need to implement cracking a new ELEMENT_TYPE?");
                break;
        }

        // Next argument
        cargs--, pbsig++;
    }

    // Consume and verify number of named properties.
    // NOTE: The test against 10 is arbitrary -- increase it if necessary --
    // but this assert has been very handy in tracking down decoding bugs.
    cNamedProps = ----I2(&pbArgBlob);
    VSASSERT(cNamedProps < 10,
             "What well-known custom attribute has this many named properties?");

    VSASSERT(cNamedProps == 0 || pnamedprops, "Invalid state");

    // If this attribute supports named properties, look for them now
    if (cNamedProps > 0 && pnamedprops->m_cProps > 0)
    {
        ----NamedProperties(cNamedProps, pnamedprops, pnra, &pbArgBlob);
    }

    // Last but not least, confirm that caller passed in NULL as last argument.
    // This helps make sure the parse is doing what the caller expects!
    // Note that we can only assert this after we've consumed all named properties.
    VSASSERT(va_arg(arglist, void *) == NULL, "Parser didn't reach end of data!");

    va_end(arglist);
}


//============================================================================
// Encodes the given bound tree (which represents an applied attribute).
//
// Note: Since this function can be called before semantic analysis has
// been performed on the tree, this function can assume nothing about the
// structure or validity of the tree passed to it.  If an unexpected
// tree form is encounted, the function still returns NOERROR, but will set
// its out parameters to NULL and zero.
//
// 


/* static */
void Attribute::EncodeBoundTree
(
    ILTree::AttributeApplicationExpression      *ptreeApplAttr,     // IN:  Tree to encode
    MetaEmit                *pMetaemit,         // IN:  MetaEmit helper to emit AssemblyRefs
    NorlsAllocator          *pnra,              // IN:  Where to allocate from
    Compiler                *pcompiler,         // IN:  Compiler doing the work
    CompilerProject         *pcompilerproj,     // IN:  Project being built
    bool                    fWellKnownAttr,     // IN:  True if *must* be well-known attribute
    _Deref_out_z_ STRING    **ppstrAttrName,    // OUT: Name of attribute (fully qualified)
    BCSYM_Class             **ppsymAttrClass,   // OUT: Class symbol for attribute
    BCSYM_Proc              **ppsymAttrCtor,    // OUT: Specific attribute CTOR being applied
    BYTE                    **ppbArgBlob,       // OUT: Attribute arg blob
    ULONG                   *pcbArgBlob,        // OUT: Arg blob's size in bytes
    ULONG                   *pcPosParams,       // OUT: Count of positional params found
    COR_SIGNATURE           **ppSignature,      // OUT: Signature
    ULONG                   *pcbSignature,      // Number of bytes in the signature.
    bool                    *pfError,            // OUT: Did a recoverable error occur?
    ErrorTable * pErrorTable
)
{
    AssertIfFalse(pMetaemit == NULL || pErrorTable != NULL);  // We should have an Error table at least every time we have Metaemit.
    
    // Build the blob using a new NorlsAllocator so we don't have to worry
    // about all of the intermediate allocs taking up space.  (Reallocs
    // on NorlsAllocators are fragile if anyone allocates
    // another block in between the original alloc and its realloc.)
    NorlsAllocator nraTemp(NORLSLOC);

    // CS_NoState guarantees we won't accidentally allocate an AttrVals.
    // (We'd assert if we tried.)
    Attribute attribute(NULL, pMetaemit, &nraTemp, pcompiler, pcompilerproj, NULL, pErrorTable);

    // All attribute signature have the following header:

    attribute.m_Signatures.SizeArray( MaxSignatureLength );
    attribute.m_Signatures.AddElement( IMAGE_CEE_CS_CALLCONV_HASTHIS );
    attribute.m_Signatures.AddElement( 0 );
    attribute.m_Signatures.AddElement( ELEMENT_TYPE_VOID );

    // _EncodeBoundTree returns result through Attribute's data members
    attribute._EncodeBoundTree(ptreeApplAttr, fWellKnownAttr);
    if (attribute.m_fError) goto Error;

    // Set OUT params by copying out of the Attribute instance.
    // Note that these allocations come from the real NRA, not temp one.
    *ppstrAttrName  = pcompiler->AddString(attribute.m_pstrAttrName);
    *ppsymAttrClass = attribute.m_psymAttrClass;
    *ppsymAttrCtor  = attribute.m_psymAttrCtor;
    *pcbArgBlob     = attribute.m_cbArgBlob;
    *pcPosParams    = attribute.m_cPosParams;
    *pcbSignature   = attribute.m_Signatures.Count() * sizeof(COR_SIGNATURE);
    *pfError        = attribute.m_fError;

    *ppSignature = (COR_SIGNATURE *)pnra->Alloc(VBMath::Multiply(
        sizeof(COR_SIGNATURE), 
        attribute.m_Signatures.Count()));
    memcpy(*ppSignature, attribute.m_Signatures.Array(), sizeof(COR_SIGNATURE) * attribute.m_Signatures.Count());

    *ppbArgBlob = (BYTE *)pnra->Alloc(attribute.m_cbArgBlob);
    memcpy(*ppbArgBlob, attribute.m_pbArgBlob, attribute.m_cbArgBlob);

Error:
    if (attribute.m_fError)
    {
        // Clear OUT params
        *ppstrAttrName  = NULL;
        *ppsymAttrClass = NULL;
        *ppsymAttrCtor = NULL;
        *ppbArgBlob = NULL;
        *ppSignature = NULL;
        *pcbArgBlob = 0;
        *pcPosParams = 0;
        *pcbSignature = 0;
        *pfError = true;
    }
}


//============================================================================
// This is the private, non-static version of Attribute::EncodeBoundTree (no
// underscore).
//============================================================================
void Attribute::_EncodeBoundTree
(
    ILTree::AttributeApplicationExpression *ptreeApplAttr,  // IN:  Bound tree to encode
    bool                fWellKnownAttr  // IN:  True if must be well-known attribute

)
{
    ULONG       ibNamedProps;       // Byte offset in blob to count of named params

    // Arg blob always begins with hard coded two-byte prefix (0x01, 0x00)
    m_pbArgBlob = (BYTE *)m_pnra->Alloc(2);
    m_pbArgBlob[0] = 0x01;
    m_pbArgBlob[1] = 0x00;
    m_cbArgBlob = 2;

    // Get the attribute's name.
    GetEmittedNameAndSymbol(ptreeApplAttr, &m_pstrAttrName, &m_psymAttrCtor, &m_psymAttrClass);
    if (m_fError) goto Error;

    // We can bail early if this symbol name doesn't match any well-known attribute
    if (fWellKnownAttr && FindAttributeIndexByName(m_pstrAttrName) == NULL)
    {
        // Unknown name -- flag recoverable error and abort
        m_fError = true;
        goto Error;
    }

    // Encode positional params.
    EncodeAllPositionalParams(ptreeApplAttr);
    if (m_fError) goto Error;

    // Reserve space for count of named props, initially set to zero.
    // Remember the offset into the blob so we can update this count later.
    ibNamedProps = m_cbArgBlob;
    ExtendArgBlob(&m_cNamedProps, sizeof(m_cNamedProps));

    // Encode named props.
    EncodeAllNamedProps(ptreeApplAttr);
    if (m_fError) goto Error;

    // Go back and update the named prop count in the blob
#ifdef _WIN64
    VSASSERT(*(m_pbArgBlob + ibNamedProps) == 0, "Named props count should be zero!");
    VSASSERT(*(m_pbArgBlob + ibNamedProps + 1) == 0, "Named props count should be zero!");
    *(m_pbArgBlob + ibNamedProps) = (BYTE) m_cNamedProps;
    *(m_pbArgBlob + ibNamedProps + 1) = (BYTE) (m_cNamedProps >> 8);
#else
    VSASSERT(*(USHORT *)(m_pbArgBlob + ibNamedProps) == 0, "Named props count should be zero!");
    *(USHORT *)(m_pbArgBlob + ibNamedProps) = m_cNamedProps;
#endif

Error:;
}

//============================================================================
// This method is used to test existence of DefaultParamValue attribute,
// if it exists, then get the argument. 
//============================================================================
bool WellKnownAttrVals::GetDefaultParamValueData
(
    BYTE  *&argBlob, 
    ULONG &cbArgBlob
)
{
    if (this == NULL || !m_fDefaultParameterValue)
    {
        argBlob = NULL;
        cbArgBlob = 0;
        return false;
    }

    DefaultParamValueObjectData* pData = reinterpret_cast<DefaultParamValueObjectData*>(m_dataTable.GetValue(DefaultParameterValue_Key));
    ASSERT(pData != NULL, "We have the bit set for DefaultParameterValue, but no data!");

    argBlob = pData->m_argBlob;
    cbArgBlob = pData->m_cbArgBlob;

    return true;
}

void WellKnownAttrVals::SetDefaultParamValueData(BYTE* argBlob, ULONG cbArgBlob)
{
    m_fDefaultParameterValue = true;

    DefaultParamValueObjectData* pData = m_pAllocator->Alloc<DefaultParamValueObjectData>();

    pData->m_argBlob = argBlob;
    pData->m_cbArgBlob = cbArgBlob;

    m_dataTable.SetValue(DefaultParameterValue_Key, pData);
}

//============================================================================
// Public, static entry-point.  Taked the BCSYM_ApplAttr (which is attached to
// the given BCSYM) and returns a bound tree representing the applied attribute.
//============================================================================
/* static */
ILTree::Expression *Attribute::GetBoundTreeFromApplAttr
(
    BCSYM_ApplAttr  *psymApplAttr,      // IN:  Applied attribute to parse
    BCSYM           *psym,              // IN:  Symbol attribute was applied to
    ErrorTable      *perrortable,       // IN:  Where to report errors
    NorlsAllocator  *pnra,              // IN:  Where to allocate bound tree from
    CompilerHost    *pcompilerHost     // IN:  CompilerHost
)
{
    Scope  *pscope;
    ILTree::Expression *ptreeExprBound = NULL;

    // The context in which the attribute was applied.
    //      - If the attribute was applied on a container,
    //        then the the context is the container itself
    //      - If the attribute was appiled on a namedroot,
    //        the the context is the container of the named
    //        root.
    //      - If the attribute was appiled on a parameter,
    //        then the context is the method the parameter
    //        belongs to
    // Why do we need this ? - This is needed because bindable
    // processes the attributes on a container as part of
    // the container itself unlike other named roots for which
    // the attributes are processed as part of their container.
    //
    // Bindable does this because of the ----ymmetry between
    // attributes on namespaces where the lookup and the use
    // context are the same vs. on other types where the lookup
    // is the parent of the use context.
    //
    // This needs to be passed to semantics when interpreting
    // the attribute so that it in turn can pass the correct
    // usage context to the obsolete checker which in turn can
    // attach any delayed obsolete checks to the bindable
    // instance of this container.
    //
    BCSYM_NamedRoot *pnamedContextOfApplAttr = NULL;

    // Figure out scope for parser -- in the case of a class, we want the
    // container of the class.
    if (psym->IsContainer())
    {
        if (psym->IsNamespace())
        {
            pscope = ViewAsScope(psym->PContainer());
        }
        else
        {
            pscope = ViewAsScope(psym->PContainer()->GetContainer());
        }

        pnamedContextOfApplAttr = psym->PContainer();
    }
    else if (psym->IsMember())
    {
        pscope = ViewAsScope(psym->PMember()->GetContainer());
        pnamedContextOfApplAttr = psym->PMember();
    }
    else if (psym->IsParam())
    {
        // Param context was cached away in its AttrVals.  Retrieve and verify it.
        BCSYM *psymContext;

        VSASSERT(psym->GetPAttrVals() != NULL, "Bad param context!");
        psymContext = psym->GetPAttrVals()->GetPsymContextOfParamWithApplAttr();
        VSASSERT(psymContext != NULL && psymContext->IsProc() && psymContext->IsMember(),
                    "Bad param context!");

        pscope = ViewAsScope(psymContext->PMember()->GetContainer());
        pnamedContextOfApplAttr = psymContext->PMember();
    }
    else
    {
        VSFAIL("What have we here?");
        goto Error;
    }





    ptreeExprBound = GetBoundTreesForAttribute(
                    pscope,
                    pnamedContextOfApplAttr,
                    psymApplAttr,
                    perrortable,
                    pnra,
                    pcompilerHost);

Error:
    // Check for error -- if any, clear OUT param
    if (ptreeExprBound != NULL)
    {
        if (ptreeExprBound->bilop != SX_APPL_ATTR ||
            IsBad(ptreeExprBound))
        {
            ptreeExprBound = NULL;
        }
    }

    return ptreeExprBound;
}

//============================================================================
// Public, static entry-point.  This ----s all of the attributes
// applied to psym and accumulates the well-known
// attribute bits into ppattrval, which is allocated on demand.
//============================================================================
/* static */
void Attribute::----AllAttributesOnSymbol
(
    BCSYM           *psym,      // Can be BCSYM_NamedRoot or BCSYM_Param
    NorlsAllocator  *pnra,
    Compiler        *pcompiler,
    CompilerHost    *pcompilerhost,
    ErrorTable      *perrortable,
    SourceFile      *psourcefile   // Source file containing symbol
)
{

    bool      fError = false;
    AttrVals *pattrvalsSym;   // AttrVals attached to this symbol (if any)

    VSASSERT(psym->IsNamedRoot() || psym->IsParam(), "Must be BCSYM_NamedRoot or BCSYM_Param");
    VSASSERT(psourcefile, "Unexpected NULL SourceFile");

    pattrvalsSym = psym->GetPAttrVals();

    // If pattrvalsSym is NULL, this symbol has no attached symbols
    if (pattrvalsSym != NULL)
    {
        BCSYM_ApplAttr *psymApplAttr;

        // Enumerate over all attributes attached to this symbol

        BCITER_ApplAttrs ApplAttrsIter(psym);
        while (psymApplAttr = ApplAttrsIter.GetNext())
        {
            ILTree::Expression *ptreeExprBound;

            ptreeExprBound = GetBoundTreeFromApplAttr(psymApplAttr,
                                              psym,
                                              perrortable,
                                              pnra,
                                              pcompilerhost);

            VSASSERT( !ptreeExprBound || ptreeExprBound->bilop == SX_APPL_ATTR,
                            "How can bound tree for an applied attribute be anything else ?");

            if (ptreeExprBound)
            {
                // 






                if (psymApplAttr->GetAttributeSymbol()->DigThroughNamedType() &&
                    psymApplAttr->GetAttributeSymbol()->DigThroughNamedType() ==
                        pcompilerhost->GetFXSymbolProvider()->GetType(FX::CLSCompliantAttributeType) &&
                    psourcefile &&
                    psourcefile->GetProject() &&
                    ((psymApplAttr->IsModule() &&
                        !psourcefile->GetProject()->OutputIsModule()) ||
                     (psymApplAttr->IsAssembly() &&
                        psourcefile->GetProject()->OutputIsModule())))
                {
                    continue;
                }

                psymApplAttr->SetBoundTree(&ptreeExprBound->AsAttributeApplicationExpression());

                // Note that we ignore recoverable errors (returned via &fError)
                const AttrIndex * pAttributeIndex = Attribute::----BoundTree(
                    &ptreeExprBound->AsAttributeApplicationExpression(),
                    pnra,
                    pcompiler,
                    psourcefile,
                    &fError,
                    psym,
                    psymApplAttr->IsAssembly());

                psymApplAttr->SetAttrIndex(pAttributeIndex);
            } // done for non null bound trees
        }
    }
}


// These macros are used in the next few functions to make traversing
// the bound tree a little bit easier.
#define ErrIfTrue(expr)             do { if (expr) { m_fError = true; goto Error; } } while (0)
#define ErrIfWrongNode(ptree, op)   ErrIfTrue((ptree == NULL) || (ptree)->bilop != (op) || IsBad(ptree))


//============================================================================
// Public, static function that converts the given bound tree into the
// appropriate bits in an AttrVals struct.  If the bound tree represents
// something other than a well-known custom attribute, this function is a nop.
//
// Returns an AttrIndex for the attribute in g_rgAttributes array or NULL if it is not a well-known attribute.
//
//============================================================================
/* static */
const AttrIndex * Attribute::----BoundTree
(
    ILTree::AttributeApplicationExpression *ptreeApplAttr,  // IN:  Tree to encode
    NorlsAllocator     *pnra,           // IN:  Where to allocate from
    Compiler           *pcompiler,      // IN:  Compiler doing the work
    SourceFile         *psourcefile,    // IN:  Source file containing this attribute usage.
    bool               *pfError,        // OUT: Did a recoverable error occur?
    BCSYM              *psym,           // IN/OUT: ---- results go here
    bool                isAssembly       // IN:  is this an Assembly attribute
)
{
    STRING *pstrAttrName = NULL;
    BCSYM_Class *psymAttrClass = NULL;
    BCSYM_Proc  *psymAttrCtor;
    BYTE *pbArgBlob = NULL;
    ULONG cbArgBlob = 0;
    ULONG cPosParams = 0;
    ULONG cbSignatureLength = 0;
    COR_SIGNATURE *pSignature = NULL;
    CompilerProject *pcompilerproj = psourcefile->GetCompilerProject();

    Attribute    attribute(NULL, NULL, pnra, pcompiler, pcompilerproj, NULL);

    // Call EncodeBoundTree to get everything we need to ---- this bound tree
    EncodeBoundTree(ptreeApplAttr,
                    NULL,
                    pnra,
                    pcompiler,
                    pcompilerproj,
                    true,          // Must be well-known attribute
                    &pstrAttrName,
                    &psymAttrClass,
                    &psymAttrCtor,
                    &pbArgBlob,
                    &cbArgBlob,
                    &cPosParams,
                    &pSignature,
                    &cbSignatureLength,
                    pfError);
    if (*pfError) goto Error;

    // Find the attribute that matches the given one.
    const AttrIndex *pAttributeIndex = FindAttributeIndexByName(pstrAttrName);
    const AttrIdentity *pCurrentAttribute = NULL;

    if (pAttributeIndex != NULL)
    {
        pCurrentAttribute = FindAttributeBySignature(
            pAttributeIndex,
            cbSignatureLength,
            pSignature);

        // Dev 10 410674
        // It's valid to have no constructor that matches. It's just an error reported in bindable,
        // because the attribute will have no applicability.
        // VSASSERT(pCurrentAttribute != NULL, "We do not have this constructor in the list");

        if (pCurrentAttribute != NULL)
        {
            // Now we can try to ---- this attribute's arg blob
            attribute.----ArgBlob(
                        pbArgBlob,
                        cbArgBlob,
                        pCurrentAttribute,
                        psym,
                        isAssembly,
                        psourcefile,
                        &ptreeApplAttr->Loc);

            return pAttributeIndex;
        }
    }

Error:;

    return NULL;
}

//============================================================================
// Returns the fully-qualified name, BCSYM_Class and ctor of the given
// applied attribute.
//============================================================================
void Attribute::GetEmittedNameAndSymbol
(
    ILTree::AttributeApplicationExpression *ptreeApplAttr,      // IN:  Tree to encode
    _Deref_out_z_ STRING **ppstrAttrName,   // OUT: Fully-qualified name of attribute
    BCSYM_Proc        **ppsymCtor,          // OUT: Constructor being invoked
    BCSYM_Class       **ppsymClass          // OUT: Class symbol for attribute
)
{
    BCSYM_NamedRoot *psymNamedRoot;
    ILTree::ILNode         *ptree;

    VSASSERT(ptreeApplAttr == NULL || ptreeApplAttr->bilop == SX_APPL_ATTR,
        "Expected SX_APPL_ATTR node at root of bound tree.");

    // Confirm SX_APPL_ATTR
    ErrIfWrongNode(ptreeApplAttr, SX_APPL_ATTR);

    // Get and confirm SX_CALL
    ptree = ptreeApplAttr->Left;
    ErrIfWrongNode(ptree, SX_CALL);

    // Get and confirm SX_SYM
    ptree = ptree->AsExpressionWithChildren().Left;
    ErrIfWrongNode(ptree, SX_SYM);

    // Extract symbol
    psymNamedRoot = ptree->AsSymbolReferenceExpression().Symbol;
    ErrIfTrue(psymNamedRoot == NULL);

    // This should be a constructor.
    ErrIfTrue(!psymNamedRoot->IsProc() || !psymNamedRoot->PProc()->IsInstanceConstructor());
    *ppsymCtor = psymNamedRoot->PProc();

    // Move up to the class name
    *ppsymClass = psymNamedRoot->GetContainingClass();
    ErrIfTrue((*ppsymClass) == NULL);

    // Get name of the class
    *ppstrAttrName =
        (*ppsymClass)->GetContainer()->IsNamespace() ?
            (*ppsymClass)->GetQualifiedEmittedName() :
            (*ppsymClass)->GetEmittedName();

Error:;
}


//============================================================================
// Encodes the positional parameters passed to the attribute specified by
// the given bound tree.  This encoding is appended to the arg blob.
//============================================================================
void Attribute::EncodeAllPositionalParams
(
    ILTree::AttributeApplicationExpression *ptreeApplAttr
)
{
    ILTree::ILNode *ptree;

    VSASSERT(ptreeApplAttr == NULL || ptreeApplAttr->bilop == SX_APPL_ATTR,
        "Expected SX_APPL_ATTR node at root of bound tree.");

    VSASSERT(m_cPosParams == 0, "Positional arg count should be zero.");

    // Confirm SX_APPL_ATTR
    ErrIfWrongNode(ptreeApplAttr, SX_APPL_ATTR);

    // Get and confirm SX_CALL
    ptree = ptreeApplAttr->Left;
    ErrIfWrongNode(ptree, SX_CALL);

    ptree = ptree->AsExpressionWithChildren().Right;

    // For each positional param
    while (ptree && ptree->bilop == SX_LIST)
    {
        EncodeConstant(ptree->AsExpressionWithChildren().Left);
        if (m_fError) goto Error;

        // Encode the signature to match the ones in our tables.
        EncodeSignatureParameter(ptree->AsExpressionWithChildren().Left->ResultType);
        m_cPosParams++;
        m_Signatures.Element(1)++;

        // Move to next positional arg
        ptree = ptree->AsExpressionWithChildren().Right;
        if (ptree == NULL)
            break;
        ErrIfWrongNode(ptree, SX_LIST);
    }

Error:;
}


//============================================================================
// Encodes one positional parameter into the signature.
//============================================================================
void Attribute::EncodeSignatureParameter
(
    BCSYM *pParameterType // [in] Symbol that represents the parameter type.
)
{
    VSASSERT(pParameterType != NULL, "Invalid NULL Symbol");

    Vtypes vtype = t_void;

    // If there is a symbol, get the vtype out of the symbol
    if (pParameterType)
    {
        if (pParameterType->IsEnum())
        {
            vtype = t_struct;
        }
        else
        {
            vtype = pParameterType->GetVtype();
        }
    }

    switch (vtype)
    {
        case t_UNDEF:
            VSFAIL("Encoding bogus type!");
            break;

        case t_bad:
            VSFAIL("EncodeType - How did a bad type get through semantic analysis?");
            break;

        case t_void:
            EncodeParameterCORType(ELEMENT_TYPE_VOID);
            break;

        case t_bool:
            EncodeParameterCORType(ELEMENT_TYPE_BOOLEAN);
            break;

        case t_i1:
            EncodeParameterCORType(ELEMENT_TYPE_I1);
            break;

        case t_ui1:
            EncodeParameterCORType(ELEMENT_TYPE_U1);
            break;

        case t_i2:
            EncodeParameterCORType(ELEMENT_TYPE_I2);
            break;

        case t_ui2:
            EncodeParameterCORType(ELEMENT_TYPE_U2);
            break;

        case t_i4:
            EncodeParameterCORType(ELEMENT_TYPE_I4);
            break;

        case t_ui4:
            EncodeParameterCORType(ELEMENT_TYPE_U4);
            break;

        case t_i8:
            EncodeParameterCORType(ELEMENT_TYPE_I8);
            break;

        case t_ui8:
            EncodeParameterCORType(ELEMENT_TYPE_U8);
            break;

        case t_single:
            EncodeParameterCORType(ELEMENT_TYPE_R4);
            break;

        case t_double:
            EncodeParameterCORType(ELEMENT_TYPE_R8);
            break;

        case t_char:
            EncodeParameterCORType(ELEMENT_TYPE_CHAR);
            break;

        case t_string:
            EncodeParameterCORType(ELEMENT_TYPE_STRING);
            break;

        case t_ref:
        case t_struct:
        {
            if (pParameterType->IsObject())
            {
                EncodeParameterCORType(ELEMENT_TYPE_OBJECT);
            }
            else if (pParameterType == m_pcompilerhost->GetFXSymbolProvider()->GetType(FX::IntPtrType))
            {
                EncodeParameterCORType(ELEMENT_TYPE_I);
            }
            else if (pParameterType == m_pcompilerhost->GetFXSymbolProvider()->GetType(FX::UIntPtrType))
            {
                EncodeParameterCORType(ELEMENT_TYPE_U);
            }
            else
            {
                if (vtype == t_struct)
                {
                    // 








                    EncodeParameterCORType(ELEMENT_TYPE_VALUETYPE);
                    EncodeSignatureParameter(
                        pParameterType->PClass()->SimpleBind(
                            NULL,
                            STRING_CONST(m_pcompiler, EnumValueMember))->PMember()->GetType());
                }
                else
                {
                    // 






                    EncodeParameterCORType(ELEMENT_TYPE_CLASS);
                    EncodeParameterCORType(SERIALIZATION_TYPE_TYPE);
                }
            }
        }
        break;

        case t_array:
        {
            VSASSERT(pParameterType->PArrayType() &&
                     pParameterType->PArrayType()->GetRoot() &&
                     pParameterType->PArrayType()->GetRank() == 1, "1-D array expected!!!");

            EncodeParameterCORType(ELEMENT_TYPE_SZARRAY);
            EncodeSignatureParameter(pParameterType->PArrayType()->GetRoot());
        }
        break;

        default:
            VSFAIL("EncodeType - unhandled type.");
        }
}

//============================================================================
// Add the given parameter type to the signature
// and increase the byte count.
//============================================================================
void Attribute::EncodeParameterCORType
(
    COR_SIGNATURE ParameterType
)
{
    m_Signatures.AddElement( ParameterType );
}

//============================================================================
// Extends the arg blob and appends cbData bytes of pvData.
//============================================================================
void Attribute::ExtendArgBlob
(
    void  *pvData,
    ULONG  cbData
)
{
    // Grow the arg blob by cbData bytes
    IfFalseThrow(m_cbArgBlob + cbData >= m_cbArgBlob);
    m_pbArgBlob = (BYTE *)m_pnra->Resize(m_pbArgBlob, m_cbArgBlob, m_cbArgBlob + cbData);

    // Append the new data
    memcpy(m_pbArgBlob + m_cbArgBlob, pvData, cbData);
    m_cbArgBlob += cbData;
}


//============================================================================
// Encodes the given length-counted Unicode string and appends to the arg blob.
//============================================================================
void Attribute::EncodeString
(
    _In_opt_count_(cch) const WCHAR *pwsz,  // IN:  String to append
    ULONG        cch    // IN:  Length of string
)
{
    if (pwsz == NULL)
    {
        // NULL strings are encoded as a single byte 0xFF
        VSASSERT(cch == 0, "NULL string must have a length of zero.");
        BYTE bEOS = 0xff;
        ExtendArgBlob(&bEOS, 1);
    }
    else
    {
        BYTE              *pbUTF8;
        ULONG              cbUTF8;
        ULONG              cbPackedLen;
        BYTE               rgbPackedLen[4];
        NorlsAllocator     nraString(NORLSLOC);

        // Convert to UTF8 string format
        ConvertUnicodeToUTF8(pwsz, cch, &nraString, &pbUTF8, &cbUTF8);

        // Pack string len into 1, 2, or 4 bytes and append to arg blob
        cbPackedLen = CorSigCompressData(cbUTF8, rgbPackedLen);
        VSASSERT(cbPackedLen >= 1 && cbPackedLen <= 4, "Integer can't be encoded.");
        ExtendArgBlob(rgbPackedLen, cbPackedLen);

        // Append encoded string to arg blob (but ignore empty strings)
        if (cbUTF8 != 0)
        {
            ExtendArgBlob(pbUTF8, cbUTF8);
        }
    }
}


//============================================================================
// Encodes one positional parameter. This encoding is appended to the arg blob.
//============================================================================
void Attribute::EncodeConstant
(
    ILTree::ILNode *ptree
)
{
    ErrIfTrue(ptree == NULL);

    switch (ptree->bilop)
    {
        case SX_CNS_INT:    // Integer constant
            {
                ILTree::IntegralConstantExpression &treeIntCon = ptree->AsIntegralConstantExpression();

                switch (treeIntCon.vtype)
                {
                    case t_bool:
                        VSASSERT(treeIntCon.Value == 0 || treeIntCon.Value == 1, "unexpected boolean constant");
                        __fallthrough;
                    case t_i1:
                    case t_ui1:
                    case t_i2:
                    case t_ui2:
                    case t_i4:
                    case t_ui4:
                    case t_i8:
                    case t_ui8:
                    case t_char:
                        ExtendArgBlob(&treeIntCon.Value, CbOfVtype(treeIntCon.vtype));
                        break;

                    default:
                        VSFAIL("Integer constant has unexpected type.");
                        m_fError = true;
                        goto Error;
                }
            }
            break;

        case SX_CNS_FLT:    // Float constant
            {
                ILTree::FloatConstantExpression &treeFltCon = ptree->AsFloatConstantExpression();
                switch (treeFltCon.vtype)
                {
                    case t_single:
                        float flt;
                        flt = (float)treeFltCon.Value;
                        ExtendArgBlob(&flt, sizeof(flt));
                        break;

                    case t_double:
                        ExtendArgBlob(&treeFltCon.Value, sizeof(treeFltCon.Value));
                        break;

                    default:
                        VSFAIL("Float constant has unexpected type.");
                        m_fError = true;
                        goto Error;
                }
            }
            break;

        case SX_CNS_STR:    // String constant
            {
                ILTree::StringConstantExpression &treeStrCon = ptree->AsStringConstant();

                switch (treeStrCon.vtype)
                {
                    case t_string:
                        // Append string to arg blob
                        EncodeString(treeStrCon.Spelling, (ULONG)treeStrCon.Length);
                        break;

                    default:
                        VSFAIL("String constant has unexpected type.");
                        m_fError = true;
                        goto Error;
                }
            }
            break;

        case SX_METATYPE:   // Metatype literal (e.g., GetType(TypeName))
            {
                // Tree is SX_METATYPE --left--> SX_NOTHING.pType
                ILTree::Expression *ptreeExprNothing;
                BCSYM   *psymType;
                STRING  *pstrType;

                // Tree is SX_METATYPE --left--> SX_NOTHING.pType
                ptreeExprNothing = ptree->AsBinaryExpression().Left;
                ErrIfWrongNode(ptreeExprNothing, SX_NOTHING);
                psymType = ptreeExprNothing->ResultType;
                ErrIfTrue(psymType == NULL ||
                    (!psymType->IsClass() && !psymType->IsInterface() && !psymType->IsArrayType()));

                // Types get encoded as fully-qualified name.  Append to arg blob.
                pstrType = EncodeConstantType(psymType, ptree->Loc);
                EncodeString(pstrType, (ULONG)StringPool::StringLength(pstrType));
            }
            break;

        case SX_NOTHING:   // "Nothing" literal (as String or Object param)
            {
                BCSYM *resultType = ptree->AsExpression().ResultType;
                if (resultType->IsObject())
                {
                    // Nothing for Object is encoded as a null string
                    BYTE bSerialType = SERIALIZATION_TYPE_STRING;
                    ExtendArgBlob(&bSerialType, sizeof(bSerialType));
                    EncodeString(NULL, 0);
                }
                else if (resultType &&
                         m_pcompilerhost->GetFXSymbolProvider()->GetTypeType() == resultType)
                {
                    // Nothing for System.Type is encoded as a null string.
                    EncodeString(NULL, 0);
                }                
                else if (resultType->IsArrayType())
                {
                    VSASSERT(resultType->PArrayType()->GetRank() == 1,
                                "Non 1-D arrays disallowed in attributes!!!");

                    DWORD NullArraySize = 0xFFFFFFFF;
                    ExtendArgBlob(&NullArraySize, sizeof(NullArraySize));
                }
                else
                {
                    // "Nothing" literal (as String param)

                    VSASSERT(ptree->AsExpression().vtype == t_string,
                        "Expected 'Nothing' attr arg to be of type String, wasn't.");

                    // Append string to arg blob.  NULL strings are encoded in a
                    // non-obvious way, so rely on helper
                    EncodeString(NULL, 0);
                }
            }
            break;

        case SX_CTYPE:
        case SX_WIDE_COERCE:
            {
                if (ptree->AsExpression().ResultType->IsObject())
                {
                    if (!ptree->AsExpression().AsBinaryExpression().Left->ResultType->IsObject())
                    {
                        EncodeNamedPropType(ptree->AsExpression().AsBinaryExpression().Left->ResultType, ptree->Loc);
                    }
                }

                EncodeConstant(ptree->AsExpression().AsBinaryExpression().Left);
                break;
            }

        case SX_CREATE_ARRAY:
            {
                VSASSERT(ptree->AsExpression().ResultType->IsArrayType() &&
                         ptree->AsExpression().ResultType->PArrayType()->GetRank() == 1,
                            "Non 1-D arrays disallowed in attributes!!!");

                ILTree::Expression *Dimensions = ptree->AsBinaryExpression().Left->AsExpressionWithChildren().Left;

                VSASSERT(Dimensions->bilop == SX_CNS_INT && ptree->AsBinaryExpression().Left->AsExpressionWithChildren().Right == NULL,
                            "Non 1-D arrays disallowed in attributes!!!");

                // Encode the size of the Array

                DWORD ArraySize =
                    (DWORD)Dimensions->AsIntegralConstantExpression().Value;

                ExtendArgBlob(&ArraySize, sizeof(ArraySize));

                // Write out the elements

                ILTree::ILNode *ArrayElementInitializers = ptree->AsBinaryExpression().Right;

#if DEBUG
                unsigned Index = 0;
#endif DEBUG

                if (ArraySize > 0)
                {
                    for(ExpressionList *ElementInit = &ArrayElementInitializers->AsExpression();
                        ElementInit;
                        ElementInit = ElementInit->AsExpressionWithChildren().Right)
                    {
                        VSASSERT(ElementInit->bilop == SX_LIST, "An array initializer is not a list tree.");

                        ILTree::Expression *Element = ElementInit->AsExpressionWithChildren().Left;

                        VSASSERT(Element, "Missing array initializer!!!");

                        EncodeConstant(Element);

#if DEBUG
                        Index++;
#endif DEBUG
                    }
                }

                VSASSERT(Index == ArraySize, "Array shape inconsistency detected!!!");
            }
            break;

        case SX_ARRAYLITERAL:
            {
                ILTree::ArrayLiteralExpression litExpr = ptree->AsArrayLiteralExpression();

                VSASSERT(litExpr.Rank == 1, "Non 1-D arrays disallowed in attributes!!!");

                // Write out the array length

                DWORD ArraySize = (DWORD)litExpr.Dims[0];
                ExtendArgBlob(&ArraySize, sizeof(ArraySize));

                // Write out the individual elements

                ILTree::Expression * subtree = litExpr.ElementList;
#if DEBUG
                unsigned Index = 0;
#endif DEBUG

                while (subtree && subtree->bilop == SX_LIST)
                {
                    ILTree::Expression *Element = subtree->AsExpressionWithChildren().Left;
                    VSASSERT(Element, "Missing array initializer!!!");
                    EncodeConstant(Element);

                    subtree = subtree->AsExpressionWithChildren().Right;
#if DEBUG
                    Index++;
#endif DEBUG
                }

                VSASSERT(Index == ArraySize, "Array shape inconsistency detected!!!");
            }
            break;

        default:
            VSFAIL("Constant has unexpected type.");
            m_fError = true;
            goto Error;
    }

Error:;
}


//============================================================================
// Encodes the given type and appends it to the arg blob.  This
// encoding is specific to attribute named properties.
//
// The encoding includes the name of the assembly the type is
// in (unless the type is in the assembly we're building or
// not in an assembly at all) followed by the full name of the
// type, but with nested classes represented by "+" instead of ".".
//============================================================================
STRING *Attribute::EncodeConstantType
(
    BCSYM *psymType, // Can be an array, class, or interface
    Location & referencingLocation
)
{
    return
        m_pcompiler->GetCLRTypeNameEncoderDecoder()->TypeToCLRName(
            psymType,
            m_pcompilerproj,
            m_pMetaemit,
            true, // IncludeAssemblySpec
            m_pErrorTable,
            &referencingLocation);
}


//============================================================================
// Encodes the given type and appends it to the given arg blob.
// This routine is similar to, but intentionally different from, MetaEmit::EncodeType.
//============================================================================
void Attribute::EncodeNamedPropType
(
    BCSYM *psymType,
    Location & referencingLocation
)
{
    BYTE    bSerialType;

    if (psymType->IsEnum())
    {
        STRING *pstrEnumName;

        // Enums are weird fish.  First encode SERIALIZATION_TYPE_ENUM, then the
        // fully-qualified enum name.
        bSerialType = SERIALIZATION_TYPE_ENUM;
        ExtendArgBlob(&bSerialType, sizeof(bSerialType));

        pstrEnumName = EncodeConstantType(psymType, referencingLocation);
        EncodeString(pstrEnumName, (ULONG)StringPool::StringLength(pstrEnumName));
    }
    else
    {
        switch (psymType->GetVtype())
        {
            // Here are the types VB knows how to encode into an attribute arg blob
            case t_bool:   bSerialType = SERIALIZATION_TYPE_BOOLEAN; break;
            case t_i1:     bSerialType = SERIALIZATION_TYPE_I1;      break;
            case t_ui1:    bSerialType = SERIALIZATION_TYPE_U1;      break;
            case t_i2:     bSerialType = SERIALIZATION_TYPE_I2;      break;
            case t_ui2:    bSerialType = SERIALIZATION_TYPE_U2;      break;
            case t_i4:     bSerialType = SERIALIZATION_TYPE_I4;      break;
            case t_ui4:    bSerialType = SERIALIZATION_TYPE_U4;      break;
            case t_i8:     bSerialType = SERIALIZATION_TYPE_I8;      break;
            case t_ui8:    bSerialType = SERIALIZATION_TYPE_U8;      break;
            case t_single: bSerialType = SERIALIZATION_TYPE_R4;      break;
            case t_double: bSerialType = SERIALIZATION_TYPE_R8;      break;
            case t_char:   bSerialType = SERIALIZATION_TYPE_CHAR;    break;
            case t_string: bSerialType = SERIALIZATION_TYPE_STRING;  break;

            case t_array:
                if (psymType->PArrayType()->GetRank() == 1 &&
                    !psymType->PArrayType()->GetRoot()->IsArrayType())
                {
                    bSerialType = SERIALIZATION_TYPE_SZARRAY;
                    ExtendArgBlob(&bSerialType, sizeof(bSerialType));
                    EncodeNamedPropType(psymType->PArrayType()->GetRoot(), referencingLocation);
                    return;
                }
                __fallthrough; // Fall through
            case t_ref:
                if (psymType == m_pcompilerhost->GetFXSymbolProvider()->GetType(FX::ObjectType))
                {
                    bSerialType = SERIALIZATION_TYPE_TAGGED_OBJECT;
                    break;
                }

                if (psymType == m_pcompilerhost->GetFXSymbolProvider()->GetType(FX::TypeType))
                {
                    bSerialType = SERIALIZATION_TYPE_TYPE;
                    break;
                }
                __fallthrough; // Fall through
            default:
                VSFAIL("Unknown type in Attribute::EncodeNamedPropType.");
                __fallthrough; // Fall through
            case t_UNDEF:
            case t_void:
            case t_bad:
            case t_ptr:
            case t_date:
            case t_decimal:
            case t_struct:
                m_fError = true;
                goto Error;
        }

        ExtendArgBlob(&bSerialType, sizeof(bSerialType));
    }

Error:;
}

//============================================================================
// Encodes one property/field assignment.
//============================================================================
void Attribute::EncodeOneNamedProp
(
    ILTree::SymbolReferenceExpression      *ptreeSym,       // IN:  LHS of prop assign (symbol)
    ILTree::ILNode            *ptreeVal        // IN:  RHS of prop assign (const value)
)
{
    BCSYM_NamedRoot *psymNR;
    STRING          *pstrSym;
    BYTE             bPropOrField;          // Differentiates field from property

    psymNR = ptreeSym->Symbol;
    ErrIfTrue(psymNR == NULL);
    pstrSym = psymNR->GetName();
    ErrIfTrue(pstrSym == NULL);

    // Append field/property determination to arg blob
    bPropOrField = psymNR->IsProperty() ?
                   SERIALIZATION_TYPE_PROPERTY :
                   SERIALIZATION_TYPE_FIELD;
    ExtendArgBlob(&bPropOrField, sizeof(bPropOrField));

    // Append type of property to arg blob
    EncodeNamedPropType(psymNR->PMember()->GetType(), ptreeSym->Loc);

    // Append name of property to arg blob
    EncodeString(pstrSym, (ULONG)StringPool::StringLength(pstrSym));

    // Append constant value to arg blob
    EncodeConstant(ptreeVal);

Error:;
}


//============================================================================
// Encodes the named properties (and fields) passed to the attribute
// specified by the given bound tree.  This encoding is appended to the arg blob.
//============================================================================
void Attribute::EncodeAllNamedProps
(
    ILTree::AttributeApplicationExpression *ptreeApplAttr
)
{
    ILTree::ILNode *ptree;

    VSASSERT(ptreeApplAttr == NULL || ptreeApplAttr->bilop == SX_APPL_ATTR,
        "Expected SX_APPL_ATTR node at root of bound tree.");

    VSASSERT(m_cNamedProps == 0, "Named prop count should be zero.");

    // Confirm SX_APPL_ATTR
    ErrIfWrongNode(ptreeApplAttr, SX_APPL_ATTR);

    // Walk list of named property assignments
    ptree = ptreeApplAttr;
    while (1)
    {
        ILTree::ILNode *ptreeAssign, *ptreeAssignLHS, *ptreeAssignRHS;

        ptree = ptree->AsExpressionWithChildren().Right;
        if (ptree == NULL)
        {
            break;
        }

        ErrIfWrongNode(ptree, SX_LIST);

        // Get assignment node
        ptreeAssign = ptree->AsExpressionWithChildren().Left;
        ErrIfWrongNode(ptreeAssign, SX_ASG);

        // Let LHS and RHS of assignment and validate
        ptreeAssignLHS = ptreeAssign->AsExpressionWithChildren().Left;
        ptreeAssignRHS = ptreeAssign->AsExpressionWithChildren().Right;

        ErrIfWrongNode(ptreeAssignLHS, SX_SYM);
        ErrIfTrue(ptreeAssignRHS == NULL);

        // Encode LHS and RHS
        EncodeOneNamedProp(&ptreeAssignLHS->AsSymbolReferenceExpression(), ptreeAssignRHS);
        if (m_fError) goto Error;

        // Keep count of named props successfully processed
        m_cNamedProps++;
    }

Error:;
}


//============================================================================
// Check for incorrect uses of attributes (e.g., using
// both STAThread and MTAThread on the same method)
//============================================================================
/*static*/
void Attribute::VerifyAttributeUsage
(
    BCSYM *psymCheck,
    ErrorTable *pErrorTable,
    CompilerHost * pCompilerHost
)
{
    VSASSERT(psymCheck->GetPAttrVals(), "How can a symbol without any attributes be on this list?");

    WellKnownAttrVals *pAttrVals = psymCheck->GetPWellKnownAttrVals();

    if (pAttrVals->GetConditionalData(NULL /* ppConditionalStringFirst */) && psymCheck->IsMethodDecl() && psymCheck->PMethodDecl()->GetType())
    {
        pErrorTable->CreateErrorWithSymbol(WRNID_ConditionalNotValidOnFunction, psymCheck);
    }

    if (pAttrVals->GetStructLayoutData(NULL) && psymCheck->IsNamedRoot() && IsGenericOrHasGenericParent(psymCheck->PNamedRoot()))
    {
        pErrorTable->CreateErrorWithSymbol(ERRID_StructLayoutAttributeNotAllowed, psymCheck);
    }

    // Can't specify both STAThreadAttribute and MTAThreadAttribute on a symbol
    if (pAttrVals->GetSTAThreadData() && pAttrVals->GetMTAThreadData())
    {
        // Decide if we can give the name of the symbol in the error
        if (psymCheck->IsNamedRoot())
        {
            pErrorTable->CreateErrorWithSymbol(
                ERRID_STAThreadAndMTAThread1,
                psymCheck, psymCheck->IsNamedRoot() ? psymCheck->PNamedRoot()->GetName() : L"");
        }
        else
        {
            // We can get here if someone applies STAThread and MTAThread to a parameter
            pErrorTable->CreateErrorWithSymbol(ERRID_STAThreadAndMTAThread0, psymCheck);
        }
    }

    // Verify attribute usage for a com class
    if (psymCheck->IsClass())
    {
        VerifyComClassAttributeUsage(psymCheck->DigThroughAlias()->PClass(), pErrorTable);
        VerifyDefaultEventAttributeUsage(psymCheck->DigThroughAlias()->PClass(), pErrorTable);
        VerifyExtensionAttributeUsage(psymCheck->DigThroughAlias()->PClass(), pErrorTable);
    }
    else if (psymCheck->IsVariable())
    {
        VerifyNonSerializedAttributeUsage(psymCheck->DigThroughAlias()->PVariable(), pErrorTable);
    }
    else if (psymCheck->IsProc())
    {
        // Verify WebMethod attribute usage
        VerifyWebMethodAttributeUsage(psymCheck->DigThroughAlias()->PProc(), pErrorTable);

        if (psymCheck->PProc()->IsAsyncKeywordUsed() && pAttrVals->GetMethodImplOptionSynchronizedData())
        {
                pErrorTable->CreateErrorWithSymbol(ERRID_SynchronizedAsyncMethod, psymCheck);
        }

        if (psymCheck->IsProperty() && pAttrVals->GetDebuggerHiddenData())
        {
            BCSYM_Property *pProperty = psymCheck->PProperty();

            if (!(pProperty->GetProperty() && pProperty->GetProperty()->GetPWellKnownAttrVals()->GetDebuggerHiddenData()) &&
                !(pProperty->SetProperty() && pProperty->SetProperty()->GetPWellKnownAttrVals()->GetDebuggerHiddenData()))
            {
                pErrorTable->CreateErrorWithSymbol(WRNID_DebuggerHiddenIgnoredOnProperties, psymCheck);
            }
        }

        if (psymCheck->IsEventDecl())
        {
            VerifyNonSerializedAttributeUsage(psymCheck->PNamedRoot(), pErrorTable);
        }

        WCHAR *pwszMessage;
        bool fError;
        if (psymCheck->PNamedRoot()->CreatedByEventDecl() && pAttrVals->GetObsoleteData(&pwszMessage, &fError))
        {
            AssertIfFalse(psymCheck->PNamedRoot()->CreatedByEventDecl()->IsBlockEvent());
            pErrorTable->CreateErrorWithSymbol(ERRID_ObsoleteInvalidOnEventMember, psymCheck);
        }

        if (pAttrVals->GetDllImportData())
        {
            // DllImport attributes are not valid on:
            //    Methods in an interface
            //    Instance methods
            //    Property Get/Set accessors
            //    Declare statements
            //    Generic methods or methods defined in a generic container
            //    Resumable methods

            if (psymCheck->IsEventDecl())
            {
                // already reported on Event decl.
                return;
            }

            if (psymCheck->IsDllDeclare())
            {
                pErrorTable->CreateErrorWithSymbol(ERRID_DllImportNotLegalOnDeclare, psymCheck);
                return;
            }

            if (psymCheck->PProc()->IsPropertyGet() || psymCheck->PProc()->IsPropertySet())
            {
                pErrorTable->CreateErrorWithSymbol(ERRID_DllImportNotLegalOnGetOrSet, psymCheck);
                return;
            }

            if (psymCheck->PProc()->CreatedByEventDecl())
            {
                VSASSERT(!psymCheck->IsSyntheticMethod(), "Attributes unexpected on synthetic event method!!!");
                pErrorTable->CreateErrorWithSymbol(ERRID_DllImportNotLegalOnEventMethod, psymCheck);
                return;
            }

            if (psymCheck->PProc()->GetContainer()->IsInterface())
            {
                pErrorTable->CreateErrorWithSymbol(ERRID_DllImportOnInterfaceMethod, psymCheck);
                return;
            }

            if (IsGenericOrHasGenericParent(psymCheck->PProc()))
            {
                pErrorTable->CreateErrorWithSymbol(ERRID_DllImportOnGenericSubOrFunction, psymCheck);
                return;
            }

            if (!psymCheck->PProc()->IsShared())
            {
                pErrorTable->CreateErrorWithSymbol(ERRID_DllImportOnInstanceMethod, psymCheck);
                return;
            }
            if (psymCheck->PProc()->IsAsyncKeywordUsed() || psymCheck->PProc()->IsIteratorKeywordUsed())
            {
                pErrorTable->CreateErrorWithSymbol(ERRID_DllImportOnResumableMethod, psymCheck);
                return;
            }
        }

        VerifyExtensionAttributeUsage(psymCheck->PProc(), pErrorTable, pCompilerHost);
    }
}


void Attribute::VerifyComClassAttributeUsage
(
    BCSYM_Class *ClassSym,
    ErrorTable *pErrorTable
)
{
    Compiler *tmpCompiler = ClassSym->GetCompiler();

    // Validate ComClassAttribute usage
    WellKnownAttrVals::ComClassData ComClassData;
    if (!ClassSym->GetPWellKnownAttrVals()->GetComClassData(&ComClassData) ||
        ComClassData.m_PartialClassOnWhichApplied != ClassSym)
    {
        return;
    }

    if (IsGenericOrHasGenericParent(ClassSym))
    {
        pErrorTable->CreateErrorWithSymbol(
            ERRID_ComClassOnGeneric,
            ClassSym);
        return;
    }

    // Can't apply this attribute to a Module
    if (ClassSym->IsStdModule())
    {
        pErrorTable->CreateErrorWithSymbol(
            ERRID_InvalidAttributeUsage2,
            ClassSym,
            COMCLASSATTRIBUTE_CLASSNAME,
            ClassSym->GetName());
        return;
    }

    // Validate the class guid
    if (!VerifyGuidAttribute(ComClassData.m_ClassId, true))
    {
        pErrorTable->CreateErrorWithSymbol(
                        ERRID_BadAttributeUuid2,
                        ClassSym,
                        COMCLASSATTRIBUTE_CLASSNAME,
                        ComClassData.m_ClassId);
    }

    // Validate the interface guid
    if (!VerifyGuidAttribute(ComClassData.m_InterfaceId, true))
    {
        pErrorTable->CreateErrorWithSymbol(
                        ERRID_BadAttributeUuid2,
                        ClassSym,
                        COMCLASSATTRIBUTE_CLASSNAME,
                        ComClassData.m_InterfaceId);
    }

    // Validate the event guid
    if (!VerifyGuidAttribute(ComClassData.m_EventId, true))
    {
        pErrorTable->CreateErrorWithSymbol(
                        ERRID_BadAttributeUuid2,
                        ClassSym,
                        COMCLASSATTRIBUTE_CLASSNAME,
                        ComClassData.m_EventId);
    }

    // Can't specify ComClass and Guid
    WCHAR *dummyGuid = NULL;
    if (ClassSym->GetPWellKnownAttrVals()->GetGuidData(&dummyGuid))
    {
        pErrorTable->CreateErrorWithSymbol(
                        ERRID_ComClassAndReservedAttribute1,
                        ClassSym,
                        L"GuidAttribute");
    }

    // Can't specify ComClass and ClassInterface
    short ClassInterfaceType;
    if (ClassSym->GetPWellKnownAttrVals()->GetClassInterfaceData(&ClassInterfaceType))
    {
        pErrorTable->CreateErrorWithSymbol(
                        ERRID_ComClassAndReservedAttribute1,
                        ClassSym,
                        L"ClassInterfaceAttribute");
    }

    // Can't specify ComClass and ComSourceInterfaces
    if (ClassSym->GetPWellKnownAttrVals()->GetComSourceInterfacesData())
    {
        pErrorTable->CreateErrorWithSymbol(
                        ERRID_ComClassAndReservedAttribute1,
                        ClassSym,
                        L"ComSourceInterfacesAttribute");
    }

    // Can't specify ComClass and ComVisible(False)
    bool ComVisible;
    if (ClassSym->GetPWellKnownAttrVals()->GetCOMVisibleData(&ComVisible) && !ComVisible)
    {
        pErrorTable->CreateErrorWithSymbol(
                        ERRID_ComClassAndReservedAttribute1,
                        ClassSym,
                        L"ComVisibleAttribute(False)");
    }


    // Can't specify the same value for iid and eventsid
    // It is not an error to reuse the classid guid though.
    if (ComClassData.m_InterfaceId != NULL && *ComClassData.m_InterfaceId != L'\0' &&
        ComClassData.m_EventId != NULL && *ComClassData.m_EventId != L'\0' &&
        CompareNoCase(ComClassData.m_InterfaceId, ComClassData.m_EventId) == 0)
    {
        pErrorTable->CreateErrorWithSymbol(
                        ERRID_ComClassDuplicateGuids1,
                        ClassSym,
                        ClassSym->GetName());
    }

    // Class must be Public
    BCSYM_NamedRoot *pNonPublic;
    if (!ClassSym->IsPublicFromAssembly(false, &pNonPublic))
    {
        if (ClassSym->IsSameContainer(pNonPublic))
        {
            pErrorTable->CreateErrorWithSymbol(
                ERRID_ComClassRequiresPublicClass1,
                ClassSym,
                ClassSym->GetName());
        }
        else
        {
            pErrorTable->CreateErrorWithSymbol(
                ERRID_ComClassRequiresPublicClass2,
                ClassSym,
                ClassSym->GetName(),
                pNonPublic->GetName());
        }
    }

    // Class cannot be Abstract
    if (ClassSym->IsMustInherit())
    {
        pErrorTable->CreateErrorWithSymbol(
            ERRID_ComClassCantBeAbstract0,
            ClassSym);
    }

    // Check for nest type name collisions on this class and
    // on all base classes.
    STRING* InterfaceName = ClassSym->GetComClassInterfaceName();
    STRING* EventInterfaceName = ClassSym->GetComClassEventsInterfaceName();
    for (BCSYM_Class *BaseClass = ClassSym;
         BaseClass != NULL;
         BaseClass = BaseClass->GetCompilerBaseClass())
    {
        BCSYM_Container *ContainerSymbol = BaseClass->PContainer();
        BCITER_CHILD ChildIterator(ContainerSymbol);

        for (BCSYM_NamedRoot *Child = ChildIterator.GetNext();
             Child != NULL;
             Child = ChildIterator.GetNext())
        {
            if (!StringPool::IsEqual(Child->GetName(), InterfaceName) &&
                !StringPool::IsEqual(Child->GetName(), EventInterfaceName))
                continue;

            // Error--name collision on this class
            if (BaseClass == ClassSym)
            {
                ErrorTable *pErrorTableForContext = Child->GetErrorTableForContext();
                StringBuffer strInterface;
                strInterface.AppendSTRING(tmpCompiler->TokenToString(tkINTERFACE));
                strInterface.AppendChar(L' ');
                strInterface.AppendSTRING(Child->GetName());
                STRING* strClass = tmpCompiler->TokenToString(tkCLASS);

                VSASSERT(pErrorTableForContext,
                            "No error table for this context ? How can that be ?");

                pErrorTableForContext->CreateErrorWithSymbol(
                    ERRID_MemberConflictWithSynth4,
                    Child,
                    strInterface.GetString(),   // 1 - member name
                    COMCLASSATTRIBUTE_CLASSNAME,// 2 - create for
                    strClass,                   // 3 - symbol kind
                    ClassSym->GetName());       // 4 - symbol name
            }
            // Warning--name shadows a base class member
            else if (!Child->IsPrivateInternal() && !ComClassData.m_fShadows)
            {
                pErrorTable->CreateErrorWithSymbol(
                    WRNID_ComClassInterfaceShadows5,
                    ClassSym,
                    ClassSym->GetName(), // 1 - class name
                    tmpCompiler->TokenToString(tkINTERFACE), // 2 - "Interface"
                    Child->GetName(), // 3 - interface name
                    tmpCompiler->TokenToString(tkCLASS), // 4 - Class
                    BaseClass->GetName()); // 5 - base class name
            }
        }
    }

    // Warn if no interfaces will be emitted.
    ComClassMembersInAContainer ClassInterfaceIter(ClassSym);
    ComClassEventsInAContainer EventsInterfaceIter(ClassSym);
    if (ClassInterfaceIter.Count() == 0 &&
        EventsInterfaceIter.Count() == 0)
    {
        pErrorTable->CreateErrorWithSymbol(
            WRNID_ComClassNoMembers1,
            ClassSym,
            ClassSym->GetName());
    }

    // Warn for Property X As Object : Set(ByVal o As Object)
    BCSYM_NamedRoot *NamedRoot ;
    ClassInterfaceIter.Reset();
    for (NamedRoot = ClassInterfaceIter.Next();
         NamedRoot != NULL;
         NamedRoot = ClassInterfaceIter.Next())
    {
        if (NamedRoot->IsProc())
        {
            if (NamedRoot->PProc()->IsGeneric())
            {
                // Generic methods cannot be exposed to COM
                //
                NamedRoot->GetErrorTableForContext()->CreateErrorWithSymbol(
                        ERRID_ComClassGenericMethod,
                        NamedRoot);
            }
            else if (NamedRoot->PProc()->IsPropertySet())
            {
                BCSYM_Property *pProperty = NamedRoot->PProc()->GetAssociatedPropertyDef();
                if (pProperty->GetCompilerType() != NULL &&
                    pProperty->GetCompilerType()->IsObject())
                {
                    ErrorTable *pErrorTableForContext = pProperty->GetErrorTableForContext();
                    StringBuffer buffer;

                    VSASSERT(pErrorTableForContext,
                                "No error table for this context ? How can that be ?");

                    pErrorTableForContext->CreateErrorWithSymbol(
                        WRNID_ComClassPropertySetObject1,
                        pProperty,
                        pErrorTable->ExtractErrorName(pProperty, ClassSym, buffer));
                }
            }
        }
    }
}

//
// Verify that the default event exists and is accessible.
void Attribute::VerifyDefaultEventAttributeUsage
(
    BCSYM_Class *ClassSym,
    ErrorTable *pErrorTable
)
{
    VSASSERT(ClassSym != NULL, "Invalid Class");

    WCHAR *wszEventName = NULL;
    BCSYM_Class* pPartialClassOnWhichApplied = NULL;

    if (!ClassSym->GetPWellKnownAttrVals()->GetDefaultEventData(&wszEventName, &pPartialClassOnWhichApplied) ||
        pPartialClassOnWhichApplied != ClassSym)
    {
        return;
    }

    if (wszEventName && wcslen(wszEventName) > 0)
    {
        STRING *strName = ClassSym->GetCompiler()->AddString(wszEventName);
        VSASSERT(strName != NULL, "Invalid STRING");

        if (!BindDefaultEvent(ClassSym, strName))
        {
            // No match, report the error.
            pErrorTable->CreateErrorWithSymbol(
                        ERRID_DefaultEventNotFound1,
                        ClassSym,
                        strName);
        }
    }
}

void Attribute::VerifyExtensionAttributeUsage(BCSYM_Class *ClassSym, ErrorTable *pErrorTable)
{
    Assume(ClassSym, L"The ClassSym parameter should not be NULL!");
    Assume(pErrorTable, L"The ErrorTable parameter should not be NULL!");
    Assume(ClassSym->GetPWellKnownAttrVals(), L"Code should not have reached this point of GetPWellKnownAttrVals returns NULL!");

    if (ClassSym->GetPWellKnownAttrVals()->GetExtensionData() && ! ClassSym->IsStdModule())
    {
        pErrorTable->CreateErrorWithSymbol(ERRID_ExtensionOnlyAllowedOnModuleSubOrFunction, ClassSym);
    }
}


void Attribute::ReportError(ErrorTable * pErrorTable,ERRID errid, BCSYM * pSym)
{
    if (pErrorTable)
    {
        pErrorTable->CreateErrorWithSymbol(errid, pSym);
    }
}

BCSYM_ApplAttr * Attribute::GetAppliedAttribute(Symbol * pSymbolToSearch, Type * pAttributeType, bool SearchThisSymbolOnly)
{
#if DEBUG
    // Right now, only partial methods should ever have SearchThisSymbolOnly == false.

    if( !SearchThisSymbolOnly )
    {
        VSASSERT( pSymbolToSearch->IsMethodImpl() &&
                  pSymbolToSearch->PMethodImpl()->IsPartialMethodImplementation(),
                  "Only partial methods should have this set to false." );
    }
#endif
    BCITER_ApplAttrs iterator(pSymbolToSearch, SearchThisSymbolOnly);

    BCSYM_ApplAttr * pCurrent = NULL;

    while (pCurrent = iterator.GetNext())
    {
        if (TypeHelpers::EquivalentTypes(pCurrent->GetAttributeSymbol(), pAttributeType))
        {
            return pCurrent;
        }
    }
    return NULL;
}

bool Attribute::ValidateExtensionAttributeMetaDataConditions
(
    Procedure * pProc,
    Container * pContainer
)
{
    ThrowIfNull(pProc);
    ThrowIfNull(pContainer);

    return
        pProc->IsShared() &&
        !pContainer->PClass()->IsGeneric() &&
        pContainer->GetParent()->IsNamespace() &&
        (
            !Bindable::DefinedInMetaData(pContainer) ||
            (
                pContainer->GetPWellKnownAttrVals() &&
                pContainer->GetPWellKnownAttrVals()->GetExtensionData()
            )
        ) &&
        !pProc->IsUserDefinedOperatorMethod();
}


void Attribute::PushTypeIfNotAlreadySeen
(
    Type * pType,
    IStackOrQueue<Type *> * pStack,
    ISet<Type *> * pProcessedTypes
)
{
    ThrowIfNull(pType);
    ThrowIfNull(pStack);
    ThrowIfNull(pProcessedTypes);

    if (! pProcessedTypes->Contains(pType))
    {
        pProcessedTypes->Add(pType);
        pStack->PushOrEnqueue(pType);
    }
}

bool Attribute::ValidateTypeDoesNotDependOnPartialyOpenParameter
(
    GenericParameter * pParameter,
    ISet<GenericParameter *> * pCloseableParameters,
    ISet<Type *> * pProcessedTypes,
    Procedure * pProc,
    ErrorTable * pErrorTable
)
{
    ThrowIfNull(pParameter);
    ThrowIfNull(pCloseableParameters);
    ThrowIfNull(pProcessedTypes);
    ThrowIfNull(pProc);
    ThrowIfNull(pProc->GetFirstParam());


    Compiler * pCompiler = pParameter->GetCompiler();

    Filo<BCSYM *> stack;
    Type * pType = DigThroughNamedTypeIfPossible(pParameter);
    PushTypeIfNotAlreadySeen(pType, &stack, pProcessedTypes);


    while (stack.Count())
    {
        pType = DigThroughNamedTypeIfPossible(stack.PopOrDequeue());

        if (pType->IsGenericParam())
        {

            GenericTypeConstraint * pTypeConstraint = pType->PGenericParam()->GetTypeConstraints();

            while (pTypeConstraint)
            {
                PushTypeIfNotAlreadySeen(pTypeConstraint->GetType(), &stack, pProcessedTypes);
                pTypeConstraint = pTypeConstraint->Next();
            }

            if (! pCloseableParameters->Contains(pType->PGenericParam()))
            {
                if (pErrorTable)
                {
                    pErrorTable->CreateErrorWithSymbol
                    (
                        ERRID_ExtensionMethodUncallable1,
                        pProc,
                        pProc->GetErrorName(pCompiler)
                    );
                }
                return false;
            }
        }
        else if (pType->IsGenericTypeBinding())
        {
            GenericBindingArgumentIterator iterator(pType->PGenericBinding());

            while (iterator.MoveNext())
            {
                PushTypeIfNotAlreadySeen(iterator.Current(), &stack, pProcessedTypes);
            }
        }
        else if (pType->IsPointerType())
        {
            PushTypeIfNotAlreadySeen(TypeHelpers::GetReferencedType(pType->PPointerType()), &stack, pProcessedTypes);
        }
        else if (pType->IsArrayType())
        {
            PushTypeIfNotAlreadySeen(TypeHelpers::GetElementType(pType->PArrayType()), &stack, pProcessedTypes);
        }
    }
    return true;
}

bool Attribute::ValidateGenericConstraitnsOnExtensionMethodDefinition
(
    Procedure * pProc,
    ErrorTable * pErrorTable
)
{
    ThrowIfNull(pProc);

    //The set of generic parameters referenced by the first argument of the procedure
    //Any parameter in this set may list any other parameter in this set as a constraint.
    //However, none of these parameters may be constrained by an parameter outside of this set.
    //This is because we must be able to deduce the type a generic extension method is defined on
    //soly based on the receiver of a method call (using partial type inference). Because generic
    //constraints affect the applicability of an extension method, we must be able to validate
    //the constraints of any generic parameters that are inferred during partial type inference.
    //If we cannot validate a constraint because one or more type parameters are still open after
    //partial inference, then the method will not be applicable.
    //We can detect this by building this set, and then iterating over each element and check the set of
    //type parameters referenced by that type parameters constraints. If any items in that set do not
    //exist in this set, then we know that extension method name lookup will not be able to validate that
    //constraint after the result of partial type inference. This then allows us to display an error
    //and let the customer know that he defined an extension method that could never be called as one.

    HashSet<GenericParameter *> firstParamSet;
    Parameter * pFirstParam = pProc->GetFirstParam();
    pFirstParam->GetType()->BuildReferencedParameterSet(&firstParamSet);

    HashSet<Type *> processedTypes;

    HashSetIterator<GenericParameter *> iterator(&firstParamSet);

    while (iterator.MoveNext())
    {
        if (! ValidateTypeDoesNotDependOnPartialyOpenParameter(iterator.Current(), &firstParamSet, &processedTypes, pProc, pErrorTable))
        {
            return false;
        }
    }
    return true;

}

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
bool Attribute::VerifyExtensionAttributeUsage(BCSYM_Proc *ProcSym, ErrorTable *pErrorTable, CompilerHost * pCompilerHost)
{
    ThrowIfNull(ProcSym);
    ThrowIfFalse(pCompilerHost || !pErrorTable); // Can only have no compilerhost if we also don't have an errortable.

    BCSYM_ApplAttr * pAttribute = NULL;
    BCSYM_Container * pContainer = ProcSym->GetContainer();
    Assume(pContainer, L"Why was the procedure's container NULL?");
    SourceFile * pSourceFile = pContainer->GetSourceFile();

    if (pCompilerHost)
    {
        Compiler * pCompiler = pCompilerHost->GetCompiler();

        if (ProcSym->GetPWellKnownAttrVals()->GetExtensionData() && ! IsBad(ProcSym))
        {
            NorlsAllocator nraScratch(NORLSLOC);

            if (pSourceFile)
            {
                // For extension methods, we need to see if the attribute may have been set
                // in the partial declaration. So pass false as the third parameter.

                pAttribute =
                    GetAppliedAttribute
                    (
                        ProcSym,
                        Semantics::GetExtensionAttributeClass
                        (
                            pCompiler,
                            pCompilerHost,
                            pSourceFile,
                            &nraScratch,
                            pContainer
                        ),
                        ProcSym->IsPartialMethodImplementation() ? false : true
                    );
            }
            ThrowIfFalse(pAttribute || !pSourceFile);
        }
        else
        {
            return false;
        }
    }

    ERRID errorId = 0;
    bool fIsExtensionMethod = false;
    bool fIsSolutionExtension = pSourceFile && pSourceFile->IsSolutionExtension();

    if
    (
        // Extension methods can only be defined in modules, except in case of metadata assemblies and synthesized source files
        !pContainer->IsClass() ||
        !
        (
            pContainer->PClass()->IsStdModule()  ||
            Bindable::DefinedInMetaData(pContainer) ||
            fIsSolutionExtension
        )
    )
    {
        if (pAttribute)
        {
            ReportError(pErrorTable, ERRID_ExtensionMethodNotInModule, pAttribute);
        }
    }
    else if
    (
        // Allow non-method extensions, but only in the case of synthesized source files.
        // In particular, we need to allow the XmlHelper class' Value and AttributeValue properties to be extension properties.
        (ProcSym->IsPropertyGet() ||
         ProcSym->IsPropertySet() ||
         ProcSym->IsEventAccessor() ||
         ProcSym->IsEventDecl() ||
         ProcSym->IsProperty()) &&
        !fIsSolutionExtension
    )
    {
        if (pAttribute)
        {
            ReportError(pErrorTable,ERRID_ExtensionOnlyAllowedOnModuleSubOrFunction, pAttribute);
        }
    }
    else if (!ProcSym->GetParameterCount())
    {
        ReportError(pErrorTable,ERRID_ExtensionMethodNoParams, ProcSym);
    }
    else
    {
        BCSYM_Param * pFirstParam = ProcSym->GetFirstParam();
        Assume(pFirstParam, L"The parameter count was non-zero but the first parameter was null. This should never happen!");

        if (pFirstParam->IsOptional())
        {
            ReportError(pErrorTable,ERRID_ExtensionMethodOptionalFirstArg, pFirstParam);
        }
        else if (pFirstParam->IsParamArray())
        {
            ReportError(pErrorTable,ERRID_ExtensionMethodParamArrayFirstArg, pFirstParam);
        }
        else if
        (
            ValidateExtensionAttributeMetaDataConditions(ProcSym, pContainer) &&
            ValidateGenericConstraitnsOnExtensionMethodDefinition(ProcSym, pErrorTable)
        )
        {
            ProcSym->SetCallableAsExtensionMethod(true);
            fIsExtensionMethod = true;

            // Do not mark synthesized source files as containing extension methods, as these never need to be searched.
            // The extension properties in the Xml helper extension are bound directly, and not through the extension
            // search mechanism.  Also, we want to skip searching the user's assembly if they don't use extension
            // methods, but do use Xml features.
            if (!fIsSolutionExtension)
            {
                pContainer->PClass()->SetContainsExtensionMethods();

                if (pSourceFile)
                {
                    if (! pSourceFile->ContainsExtensionMethods())
                    {
                        pSourceFile->SetContainsExtensionMethods(true);
                        pSourceFile->SetExtensionAttributeApplication(pAttribute);
                    }
                }
            }
        }
    }

    return fIsExtensionMethod;
}


//
// Check the WebMethod's to make sure they don't have optional parameters
//
void Attribute::VerifyWebMethodAttributeUsage
(
    BCSYM_Proc *ProcSym,
    ErrorTable *pErrorTable
)
{
    Compiler *tmpCompiler = ProcSym->GetCompiler();

    {
        // Validate WebMethodAttribute usage
        AttrKind attrId;
        bool fEnableSession;
        short TransactionOption;
        int CacheDuration;
        bool fBufferResponse;
        WCHAR *wszDescription;

        if (!ProcSym->GetPWellKnownAttrVals()->GetWebMethodData(
                &attrId,
                &fEnableSession,
                &TransactionOption,
                &CacheDuration,
                &fBufferResponse,
                &wszDescription))
        {
            return;
        }
    }

    // 



























    BCSYM_Param *ParamSym;
    for (ParamSym = ProcSym->GetFirstParam(); ParamSym; ParamSym = ParamSym->GetNext())
    {
        if (ParamSym->IsOptional())
        {
            pErrorTable->CreateErrorWithSymbol(
                            ERRID_InvalidOptionalParameterUsage1,
                            ProcSym,
                            L"WebMethod");
        }
    }
}

//
// Check the non-serialialized attribute and ensure it is used in a serializable class.
//
void Attribute::VerifyNonSerializedAttributeUsage(BCSYM_NamedRoot *ProcField, ErrorTable *pErrorTable)
{

    AssertIfFalse(ProcField->IsVariable() || ProcField->IsEventDecl());

    // The non-serialized attribute is not present
    if (!ProcField->GetPWellKnownAttrVals()->GetNonSerializedData())
    {
        return;
    }

    //Verify that non-serialized attribute can not be applied on custom event or any event in interfaces
    // The assumption is that if the backing field does not exist then the event is either a custom event or in an interface.
    // Bug 674331
    if (ProcField->IsEventDecl() && ProcField->PEventDecl()->GetDelegateVariable() == NULL)
    { 
        pErrorTable->CreateError(
                        ERRID_InvalidAttributeUsage2,
                        ProcField->PEventDecl()->GetLocation(),
                        L"NonSerialized",
                        ProcField->GetName());
        return;
    }

    // Verify that the container is a serializable class
    if (!ProcField->GetContainingClass()->GetPWellKnownAttrVals()->GetSerializableData())
    {
            pErrorTable->CreateErrorWithSymbol(
                            ERRID_InvalidNonSerializedUsage,
                            ProcField);
    }
}


// Check Guid attribute format
/* static */
bool Attribute::VerifyGuidAttribute(const WCHAR* GuidIn, bool AllowBlank)
{
    bool Result = false;

    if (AllowBlank && (GuidIn == NULL || *GuidIn == L'\0'))
    {
        Result = true;
    }
    else if (GuidIn != NULL && wcslen(GuidIn) == 36)
    {
        WCHAR GuidFmt[40];
        GUID Guid;
        GuidFmt[0] = L'{';
        wcscpy_s(&GuidFmt[1], _countof(GuidFmt) - 1, GuidIn);
        GuidFmt[37] = L'}';
        GuidFmt[38] = L'\0';
        Result = SUCCEEDED(IIDFromString(GuidFmt, &Guid));
    }

    return Result;
}

BCSYM_NamedRoot *Attribute::BindDefaultEvent(BCSYM_Class *ClassSym, _In_z_ STRING *strName)
{
    BCSYM_Class *pCurrentClass = ClassSym;

    // 


    // Iterate over all the base classes.
    while (pCurrentClass && !pCurrentClass->IsBad())
    {
        BCITER_CHILD MemberIterator(pCurrentClass);
        BCSYM_NamedRoot *pMember = NULL;

        // Iterate over all the members.
        while (pMember = MemberIterator.GetNext())
        {
            pMember = pMember->DigThroughAlias()->PNamedRoot();

            if (!pMember->IsBad() &&
                pMember->IsEventDecl() &&
                (pMember->GetAccess() == ACCESS_Public ||
                pMember->GetAccess() == ACCESS_Friend) &&
                StringPool::IsEqual(pMember->GetName(), strName))
            {
                // We have a match so the default event is valid.
                return pMember;
            }
        }

        pCurrentClass = pCurrentClass->GetCompilerBaseClass();
    }

    return NULL;
}

//============================================================================
// Public getter for AttributeUsage attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetAttributeUsageData
(
    CorAttributeTargets *pattrtargetsValidOn,   // OUT
    bool                *pfAllowMultiple,       // OUT
    bool                *pfInherited            // OUT
)
{
    if (this == NULL || !m_fAttributeUsage)
    {
        // Clear OUT params
        *pattrtargetsValidOn = (CorAttributeTargets)0;
        *pfAllowMultiple     = false;
        *pfInherited         = false;
        return false;
    }

    AttributeUsageData* pData = reinterpret_cast<AttributeUsageData*>(m_dataTable.GetValue(AttributeUsage_Key));
    // Set OUT params
    *pattrtargetsValidOn = pData->m_attrtargetsValidOn;
    *pfAllowMultiple     = pData->m_fAllowMultiple;
    *pfInherited         = pData->m_fInherited;
    return true;
}


//============================================================================
// Public getter for Obsolete attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetObsoleteData
(
    _Deref_out_opt_z_ WCHAR **ppwszMessage,   // OUT
    bool   *pfError         // OUT
)
{
    if (this == NULL || !m_fObsolete)
    {
        // Clear OUT params
        *ppwszMessage = NULL;
        *pfError = false;
        return false;
    }

    ObsoleteData* pData = reinterpret_cast<ObsoleteData*>(m_dataTable.GetValue(Obsolete_Key));

    // Set OUT params
    *ppwszMessage = pData->m_pwszMessage;
    *pfError      = pData->m_fError;
    return true;
}

//============================================================================
// Public getter for Obsolete attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetObsoleteData(_In_z_ WCHAR* pwszMessage, bool fError)
{
    m_fObsolete = true;

    ObsoleteData* pData = m_pAllocator->Alloc<ObsoleteData>();
    pData->m_pwszMessage = pwszMessage;
    pData->m_fError = fError;

    m_dataTable.SetValue(Obsolete_Key, pData);
}

//============================================================================
// Public getter for Conditional attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetConditionalData
(
    ConditionalString **ppConditionalStringFirst   // OUT
)
{
    if (this == NULL || !m_fConditional)
    {
        // Clear OUT params
        if (ppConditionalStringFirst != NULL)
        {
            *ppConditionalStringFirst = NULL;
        }

        return false;
    }


    if (ppConditionalStringFirst != NULL)
    {
        //ConditionalData* pData = 

        // Set OUT params
        *ppConditionalStringFirst = reinterpret_cast<ConditionalString*>(m_dataTable.GetValue(Conditional_Key));
    }

    return true;
}

//============================================================================
// Public setter for Conditional attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::AddConditionalData(_In_z_ WCHAR* pConditionalString)
{
    m_fConditional = true;

    void* pRawData;

    if (!m_dataTable.GetValue(Conditional_Key, &pRawData))
    {
        // We don't yet have a list of conditional strings in the table, so
        // this must be the first one we're adding.

        ConditionalString* pData;

        pData = m_pAllocator->Alloc<ConditionalString>();
        pData->m_pstrConditional = pConditionalString;
        pData->m_pConditionalStringNext = NULL;
        
        m_dataTable.SetValue(Conditional_Key, pData);
    }
    else
    {
        ConditionalString* pData = reinterpret_cast<ConditionalString*>(pRawData);

        ConditionalString* pNewConditionalString = m_pAllocator->Alloc<ConditionalString>();
        pNewConditionalString->m_pstrConditional = pConditionalString;
        pNewConditionalString->m_pConditionalStringNext = pData;
        
        m_dataTable.SetValue(Conditional_Key, pNewConditionalString);
    }
}

//============================================================================
// Public getter for CLSCompliant attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetCLSCompliantData
(
    bool *pfCompliant,   // OUT
    bool *pfIsInherited  // OUT
)
{
    if (this == NULL || !m_fCLSCompliant)
    {
        // Clear OUT params
        *pfCompliant = false;

        if (pfIsInherited)
        {
            *pfIsInherited = false;
        }

        return false;
    }

    CLSCompliantData* pData = reinterpret_cast<CLSCompliantData*>(m_dataTable.GetValue(CLSCompliant_Key));

    // Set OUT params
    *pfCompliant = pData->m_fCompliant;

    if (pfIsInherited)
    {
        *pfIsInherited = pData->m_fIsInherited;
    }

    return true;
}

//============================================================================
// Public getter for Flags attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetFlagsData()
{
    if (this == NULL || !m_fFlags)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public setter for Flags attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetFlagsData()
{
    m_fFlags = true;
}
// Public getter for PersistContents attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetPersistContentsData
(
    bool *pfPersistContents   // OUT
)
{
    if (this == NULL || !m_fPersistContents)
    {
        // Clear OUT params
        *pfPersistContents = false;
        return false;
    }

    PersistContentsData* pData = reinterpret_cast<PersistContentsData*>(m_dataTable.GetValue(PersistContents_Key));

    // Set OUT params
    *pfPersistContents = pData->m_fPersistContents;
    return true;
}

//============================================================================
// Public setter for PersistContents attribute data stored in
// WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetPersistContentsData(bool fPersistsContents)
{
    m_fPersistContents = true;

    PersistContentsData* pData = m_pAllocator->Alloc<PersistContentsData>();
    pData->m_fPersistContents = fPersistsContents;

    m_dataTable.SetValue(PersistContents_Key, pData);
}

//============================================================================
// Public getter for DefaultMember attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetDefaultMemberData
(
    _Deref_out_opt_z_ WCHAR **ppwszDefaultMember
)
{
    if (this == NULL || !m_fDefaultMember)
    {
        // Clear OUT params
        *ppwszDefaultMember = NULL;
        return false;
    }

    DefaultMemberData* pData = reinterpret_cast<DefaultMemberData*>(m_dataTable.GetValue(DefaultMember_Key));

    // Set OUT params
    *ppwszDefaultMember = pData->m_pwszDefaultMember;
    return true;
}

//============================================================================
// Public getter for DefaultMember attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetDefaultMemberData(_In_z_ WCHAR* pwszDefaultMember)
{
    m_fDefaultMember = true;

    DefaultMemberData* pData = m_pAllocator->Alloc<DefaultMemberData>();
    pData->m_pwszDefaultMember = pwszDefaultMember;

    m_dataTable.SetValue(DefaultMember_Key, pData);
}

//============================================================================
// Public getter for SecurityCritical attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetSecurityCriticalData()
{
    if (this == NULL || !m_fSecurityCritical)
    {
        return false;
    }

    return true;
}

//============================================================================
// Public Setter for SecurityCritical attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetSecurityCriticalData()
{
    m_fSecurityCritical= true;
}

//============================================================================
// Public getter for SecuritySafeCritical attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetSecuritySafeCriticalData()
{
    if (this == NULL || !m_fSecuritySafeCritical)
    {
        return false;
    }

    return true;
}

//============================================================================
// Public Setter for SecuritySafeCritical attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetSecuritySafeCriticalData()
{
    m_fSecuritySafeCritical= true;
}

//============================================================================
// Public getter for DesignerCategory attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetDesignerCategoryData
(
    _Deref_out_opt_z_ WCHAR **ppwszCategory
)
{
    if (this == NULL || !m_fDesignerCategory)
    {
        // Clear OUT params
        *ppwszCategory = NULL;
        return false;
    }

    DesignerCategoryData* pData = reinterpret_cast<DesignerCategoryData*>(m_dataTable.GetValue(DesignerCategory_Key));

    // Set OUT params
    *ppwszCategory = pData->m_pwszCategory;
    return true;
}


//============================================================================
// Public getter for AccessedThroughProperty attribute data stored in AttrVals.
// WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetAccessedThroughPropertyData
(
    _Deref_out_opt_z_ WCHAR **ppwszProperty
)
{
    if (this == NULL || !m_fAccessedThroughProperty)
    {
        // Clear OUT params
        *ppwszProperty = NULL;
        return false;
    }

    AccessedThroughPropertyData* pData = reinterpret_cast<AccessedThroughPropertyData*>(m_dataTable.GetValue(AccessedThroughProperty_Key));

    // Set OUT params
    *ppwszProperty = pData->m_pwszProperty;
    return true;
}

//============================================================================
// Public setter for AccessedThroughProperty attribute data stored in
// WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetAccessedThroughPropertyData(_In_z_ WCHAR* pwszProperty)
{
    m_fAccessedThroughProperty = true;

    AccessedThroughPropertyData* pData = m_pAllocator->Alloc<AccessedThroughPropertyData>();
    pData->m_pwszProperty = pwszProperty;

    m_dataTable.SetValue(AccessedThroughProperty_Key, pData);
}
// Public getter for CompilationRelaxations attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetCompilationRelaxationsData()
{
    if (this == NULL || !m_fCompilationRelaxationsAssembly)
    {
        //no out param
        return false;
    }
    // no OUT params
    return true;
}

//============================================================================
// Public setter for CompilationRelaxations attribute data stored in
// WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetCompilationRelaxationsData()
{
    m_fCompilationRelaxationsAssembly = true;
}

//============================================================================
// Public getter for RuntimeCompatibility attribute data stored in
// WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetRuntimeCompatibilityData()
{
    if (this == NULL || !m_fRuntimeCompatibility)
    {
        //no out param
        return false;
    }
    // no OUT params
    return true;
}

//============================================================================
// Public getter for RuntimeCompatibility attribute data stored in
// WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetRuntimeCompatibilityData()
{
    m_fRuntimeCompatibility = true;
}

//============================================================================
// Public getter for Debuggable attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetDebuggableData()
{
    if (this == NULL || !m_fDebuggable)
    {
        //no out param
        return false;
    }
    // no OUT params
    return true;
}

//============================================================================
// Public setter for Debuggable attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetDebuggableData()
{
    m_fDebuggable = true;
}

//============================================================================
// Public getter for DebuggerHidden attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetDebuggerHiddenData()
{
    if (this == NULL || !m_fDebuggerHidden)
    {
        //no out param
        return false;
    }
    // no OUT params
    return true;
}

//============================================================================
// Public setter for DebuggerHidden attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetDebuggerHiddenData()
{
    m_fDebuggerHidden = true;
}

//============================================================================
// Public getter for MyGroupCollection attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetMyGroupCollectionData(MyGroupCollectionData** ppdaMyGroupCollectionData)
{
    if (this == NULL || !m_fMyGroupCollection)
    {
        // Clear OUT params
        if (ppdaMyGroupCollectionData)
            //memset(ppdaMyGroupCollectionData, 0, sizeof(*pMyGroupCollectionData));
            //VSASSERT(m_daMyGroupAttrData != NULL, "Bad MyGroupCollectionData info");
            *ppdaMyGroupCollectionData = NULL;

        return false;
    }

    // Set OUT params
    if (ppdaMyGroupCollectionData)
        *ppdaMyGroupCollectionData = m_daMyGroupAttrData;
    return true;
}

//============================================================================
// Public setter for MyGroupCollection attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetMyGroupCollectionData(MyGroupCollectionData* pMyGroupCollectionData)
{
    m_fMyGroupCollection = true;

    m_daMyGroupAttrData = pMyGroupCollectionData;
}

void WellKnownAttrVals::ResetMyGroupCollectionData()
{
    if (this == NULL || !m_fMyGroupCollection)
    {
        return;
    }
    m_daMyGroupAttrData->Destroy();
    m_fMyGroupCollection = false;
    delete m_daMyGroupAttrData;
    m_daMyGroupAttrData = NULL;

}
//============================================================================
// Public getter for HideModuleName attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetHideModuleNameData()
{
    if (this == NULL || !m_fHideModuleName)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public getter for Embedded attribute data stored in AttrVals.
//============================================================================
void WellKnownAttrVals::SetHideModuleNameData()
{
    m_fHideModuleName = true;
}

//============================================================================
// Public getter for GetDesignerGeneratedData attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetDesignerGeneratedData()
{
    if (this == NULL || !m_fDesignerGenerated)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public setter for DesignerGenerated attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetDesignerGeneratedData()
{
    m_fDesignerGenerated = true;
}

//============================================================================
// Public getter for StandardModule attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetStandardModuleData()
{
    if (this == NULL || !m_fStandardModule)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public setter for StandardModule attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetStandardModuleData()
{
    m_fStandardModule = true;
}

//============================================================================
// Public getter for Extension attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetExtensionData()
{
    if (this != NULL)
    {
        return m_fExtension;
    }
    else
    {
        return false;
    }
}

//============================================================================
// Public setter for Extension attribute data stored in AttrVals.
//============================================================================

void WellKnownAttrVals::SetExtensionData()
{
    m_fExtension = true;
}

//============================================================================
// Public getter for OptionCompare attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetOptionCompareData()
{
    if (this == NULL || !m_fOptionCompare)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public getter for OptionCompare attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetOptionCompareData()
{
    m_fOptionCompare = true;
}

//============================================================================
// Public getter for EditorBrowsable attribute data stored in
// WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetEditorBrowsableData
(
    long *pEditorBrowsable     // OUT
)
{
    if (this == NULL || !m_fEditorBrowsable)
    {
        // Clear OUT params
        *pEditorBrowsable = 0;
        return false;
    }

    EditorBrowsableData* pData = reinterpret_cast<EditorBrowsableData*>(m_dataTable.GetValue(EditorBrowsable_Key));

    // Set OUT params
    *pEditorBrowsable = pData->m_EditorBrowsableState;
    return true;
}

//============================================================================
// Public setter for EditorBrowsable attribute data stored in
// WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetEditorBrowsableData(long editorBrowsable)
{
    m_fEditorBrowsable = true;

    EditorBrowsableData* pData = m_pAllocator->Alloc<EditorBrowsableData>();
    pData->m_EditorBrowsableState = editorBrowsable;

    m_dataTable.SetValue(EditorBrowsable_Key, pData);
}

//============================================================================
// Public getter for DebuggerBrowsable attribute data stored in 
// WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetDebuggerBrowsableData( _Out_  DebuggerBrowsableStateEnum *pBrowsableState )
{
    if ( this == NULL || !m_fDebuggerBrowsable )
    {
        *pBrowsableState = DebuggerBrowsableState::Never;
        return false;
    }

    DebuggerBrowsableData* pData = reinterpret_cast<DebuggerBrowsableData*>(m_dataTable.GetValue(DebuggerBrowsable_Key));

    *pBrowsableState = (DebuggerBrowsableStateEnum)(pData->m_value);
    return true;
}

//============================================================================
// Public setter for DebuggerBrowsable attribute data stored in 
// WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetDebuggerBrowsableData(DebuggerBrowsableStateEnum browsableState)
{
    m_fDebuggerBrowsable = true;

    DebuggerBrowsableData* pData = m_pAllocator->Alloc<DebuggerBrowsableData>();
    pData->m_value = browsableState;

    m_dataTable.SetValue(DebuggerBrowsable_Key, pData);
}

//============================================================================
// Public getter for DebuggerDisplay attribute
//============================================================================
bool WellKnownAttrVals::GetDebuggerDisplayData()
{
    if ( this == NULL || !m_fDebuggerDisplay)
    {
        return false;
    }

    return true;
}

//============================================================================
// Public setter for DebuggerDisplay attribute
//============================================================================
void WellKnownAttrVals::SetDebuggerDisplayData()
{
    m_fDebuggerDisplay = true;
}

//============================================================================
// Public getter for DllImport attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetDllImportData()
{
    if (this == NULL || !m_fDllImport)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public getter for DllImport attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetDllImportData()
{
    m_fDllImport = true;
}

//============================================================================
// Public getter for STAThread attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetSTAThreadData()
{
    if (this == NULL || !m_fSTAThread)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public setter for STAThread attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetSTAThreadData()
{
    m_fSTAThread = true;
}

//============================================================================
// Public getter for MTAThread attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetMTAThreadData()
{
    if (this == NULL || !m_fMTAThread)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public setter for MTAThread attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetMTAThreadData()
{
    m_fMTAThread = true;
}

//============================================================================
// Public getter for MarshalAs attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetMarshalAsData(PCCOR_SIGNATURE *ppSig, ULONG *pcbSig)
{
    // MarshalAsAttribute is actually a "pseudo-custom attribute". There is an
    // attribute class, but it is actually a parameter flag and not a normal
    // attribute. Therefore this attribute is used in two completely different
    // scenarios, which fortunately do not overlap: a user can attach MarshalAs
    // to one of their pinvoke declarations in order to trigger certain parameter
    // marshalling behavior, or we can read marshalling data in via MetaImport
    // and write it back out again as part of the NoPIA feature. 
    if (this == NULL || !m_fMarshalAs)
    {
        if (ppSig)
        {
            *ppSig = NULL;
        }
        if (pcbSig)
        {
            *pcbSig = 0;
        }
        return false;
    }

    void* pRawData;
    if (!m_dataTable.GetValue(MarshalAs_Key, &pRawData))
    {
        if (ppSig)
        {
            *ppSig = NULL;
        }
        if (pcbSig)
        {
            *pcbSig = 0;
        }
    }
    else
    {
        MarshalAsData* pData = reinterpret_cast<MarshalAsData*>(pRawData);

        if (ppSig)
        {
            *ppSig = pData->pNativeType;
        }
        if (pcbSig)
        {
            *pcbSig = pData->cbNativeType;
        }
    }
    return true;
}

void WellKnownAttrVals::SetMarshalAsData(PCCOR_SIGNATURE pSig, ULONG sigBytes)
{
    m_fMarshalAs = true;

    MarshalAsData* pData = m_pAllocator->Alloc<MarshalAsData>();

    pData->pNativeType = pSig;
    pData->cbNativeType = sigBytes;

    m_dataTable.SetValue(MarshalAs_Key, pData);
}

void WellKnownAttrVals::SetMarshalAsData()
{
    m_fMarshalAs = true;
}

bool WellKnownAttrVals::GetImportedFromTypeLibData(_Deref_out_opt_z_ WCHAR **ppTlbFile)
{
    if (this == NULL || !m_fImportedFromTypeLib)
    {
        if (ppTlbFile)
        {
            *ppTlbFile = NULL;
        }
        return false;
    }

    if (ppTlbFile)
    {
        *ppTlbFile = reinterpret_cast<WCHAR*>(m_dataTable.GetValue(ImportedFromTypeLib_Key));
    }
    return true;
}

void WellKnownAttrVals::SetImportedFromTypeLibData(_In_z_ WCHAR* pTlbFile)
{
    m_fImportedFromTypeLib = true;

    m_dataTable.SetValue(ImportedFromTypeLib_Key, pTlbFile);
}

//===========================================================================
// Public getter for Guid attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetGuidData(_Deref_out_opt_z_ WCHAR **ppwszGuid)
{
    if (this == NULL || !m_fGuid)
    {
        *ppwszGuid = NULL;
        return false;
    }

    GuidData* pData = reinterpret_cast<GuidData*>(m_dataTable.GetValue(Guid_Key));

    *ppwszGuid = pData->m_pwszGuid;
    return true;
}

//============================================================================
// Public setter for Guid attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetGuidData(_In_z_ WCHAR* pwszGuid)
{
    m_fGuid = true;

    GuidData* pData = m_pAllocator->Alloc<GuidData>();
    pData->m_pwszGuid = pwszGuid;

    m_dataTable.SetValue(Guid_Key, pData);
}

bool WellKnownAttrVals::GetPrimaryInteropAssemblyData()
{
    if (this == NULL || !m_fPrimaryInteropAssembly)
    {
        return false;
    }
    return true;
}

void WellKnownAttrVals::SetPrimaryInteropAssemblyData()
{
    m_fPrimaryInteropAssembly = true;
}

// Returns True if TypeIdentifier attribute is applied. Safe to call on a NULL pointer.
bool WellKnownAttrVals::GetTypeIdentifierData
(
)
{
    if (this == NULL || !m_fTypeIdentifier)
    {
        return false;
    }

    return true;
}

void WellKnownAttrVals::SetTypeIdentifierData()
{
    m_fTypeIdentifier = true;
}

bool WellKnownAttrVals::GetTypeIdentifierData
(
    _Deref_out_opt_z_ WCHAR **pGuid, 
    _Deref_out_opt_z_ WCHAR **pName
)
{
    if (this == NULL || !m_fTypeIdentifier)
    {
        if (pGuid) 
        {
            *pGuid = NULL;
        }
        if (pName) 
        {
            *pName = NULL;
        }
        return false;
    }
    void* pRawData;
    if (!m_dataTable.GetValue(TypeIdentifier_Key, &pRawData))
    {
        if (pGuid)
        {
            *pGuid = NULL;
        }
        if (pName)
        {
            *pName = NULL;
        }
    }
    else
    {
        TypeIdentifierData* pData = reinterpret_cast<TypeIdentifierData*>(pRawData);

        if (pGuid) 
        {
            *pGuid = pData->m_pGuid;
        }
        if (pName) 
        {
            *pName = pData->m_pName;
        }
    }

    return true;
}

void WellKnownAttrVals::SetTypeIdentifierData(_In_z_ WCHAR* pGuid, _In_z_ WCHAR* pName)
{
    m_fTypeIdentifier = true;

    TypeIdentifierData* pData = m_pAllocator->Alloc<TypeIdentifierData>();
    pData->m_pGuid = pGuid;
    pData->m_pName = pName;

    m_dataTable.SetValue(TypeIdentifier_Key, pData);
}

//===========================================================================
// SetStructLayoutDataByLayoutKind sets layout data by LayoutKind(System.
// Runtime.InteropServices.LayoutKind). LayoutKind
// is a .net Enum type, it contains three values,
// Sequential, Explicit and Auto, respectively they corresponds to integer 0,
// 2 and 3.
// If the input of this function is not 0, 2 or 3, then m_StructLayoutData is 
// set to tdLayoutMask, which is used to represent an invalid value here. 
//===========================================================================
void WellKnownAttrVals::SetStructLayoutDataByLayoutKind(int layoutKind)
{
    m_fStructLayout = true;

    __int16 structLayout;
    switch(layoutKind)
    {
    case 0:
        structLayout = tdSequentialLayout;
        break;
    case 2:
        structLayout = tdExplicitLayout;
        break;
    case 3:
        structLayout = tdAutoLayout;
        break;

    default:
        structLayout = tdAutoLayout;
        break;
    }

    m_dataTable.SetValue(StructLayout_Key, reinterpret_cast<void*>(structLayout));
}

void WellKnownAttrVals::SetStructLayoutData(__int16 layoutKind)
{
    m_fStructLayout = true;

    AssertIfFalse((layoutKind & (~tdLayoutMask)) == 0);

    m_dataTable.SetValue(StructLayout_Key, reinterpret_cast<void*>(layoutKind));
}

bool WellKnownAttrVals::GetStructLayoutData(__int16 *pLayoutKind)
{
    if (this == NULL || !m_fStructLayout)
    {
        if (pLayoutKind)
        {
            *pLayoutKind = 0;
        }
        return false;
    }
    if (pLayoutKind)
    {
// The warning 4302 is that we are casting from void * to short and losing information. Since we
// stored a short for this key, we can suppress this warning.
#pragma warning (suppress:4302)
        *pLayoutKind = reinterpret_cast<__int16>(m_dataTable.GetValue(StructLayout_Key));
    }
    return true;
}


//===========================================================================
// SetMethodImplDataByMethodImplOptions sets method-impl data by MethodImplOptions
// (System.Runtime.CompilerServices.MethodImplOptions). MethodImplOptions
// is a .net Enum type. It has several values, but the only one we keep track
// of is MethodImplOptions.Synchronized, numeric value 32
//===========================================================================
void WellKnownAttrVals::SetMethodImplOptionData(__int16 methodImplOptions)
{
    m_fMethodImplOptionSynchronized = ((methodImplOptions & 32) != 0);
}

bool WellKnownAttrVals::GetMethodImplOptionSynchronizedData()
{
    if (this == NULL || !m_fMethodImplOptionSynchronized)
    {
        return false;
    }
    return true;
}


// Set CharSet flag for the type
void WellKnownAttrVals::SetCharSet(CorTypeAttr charSet)
{
    m_fCharSet = true;
    AssertIfFalse(charSet == tdAnsiClass || charSet == tdUnicodeClass || charSet == tdAutoClass);

    CorTypeAttr* pData = m_pAllocator->Alloc<CorTypeAttr>();
    *pData = charSet;

    m_dataTable.SetValue(CharSet_Key, reinterpret_cast<void*>(pData));
}

// Get CharSet flag for the type. Currently available only for types imported from Metadata.
bool WellKnownAttrVals::GetCharSet(CorTypeAttr &charSet)
{
    if (this == NULL || !m_fCharSet)
    {
        charSet = (CorTypeAttr)0;
        return false;
    }
    CorTypeAttr* pData = reinterpret_cast<CorTypeAttr*>(m_dataTable.GetValue(CharSet_Key));

    charSet = *pData;
    return true;
}

// Set pack size and class size for the type.
void WellKnownAttrVals::SetPackClassSize(DWORD dwPackSize, ULONG ulClassSize)
{
    m_fPackClassSize = true;

    PackAndClassSizeData* pData = m_pAllocator->Alloc<PackAndClassSizeData>();

    pData->m_dwPackSize = dwPackSize; 
    pData->m_ulClassSize = ulClassSize;

    m_dataTable.SetValue(PackClassSize_Key, pData);
}


// Get pack size and class size for the type. Currently available only for types imported from Metadata.
bool WellKnownAttrVals::GetPackClassSize(DWORD & dwPackSize, ULONG & ulClassSize)
{
    if (this == NULL || !m_fPackClassSize)
    {
        dwPackSize = 0; 
        ulClassSize = 0;
        return false;
    }
    
    PackAndClassSizeData* pData = reinterpret_cast<PackAndClassSizeData*>(m_dataTable.GetValue(PackClassSize_Key));

    dwPackSize = pData->m_dwPackSize; 
    ulClassSize = pData->m_ulClassSize;
    return true;
}




//============================================================================
// Public getter for ComClass attribute data stored in WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetComClassData(ComClassData* pData)
{
    if (this == NULL || !m_fComClass)
    {
        // Clear OUT params
        if (pData)
            memset(pData, 0, sizeof(*pData));

        return false;
    }

    // Set OUT params
    if (pData)
    {
        ComClassData* pComClassData = reinterpret_cast<ComClassData*>(m_dataTable.GetValue(ComClass_Key));
        *pData = *pComClassData;
    }

    return true;
}

//============================================================================
// Public setter for ComClass attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetComClassData(ComClassData data)
{
    m_fComClass = true;

    ComClassData* pData = m_pAllocator->Alloc<ComClassData>();
    *pData = data;

    m_dataTable.SetValue(ComClass_Key, pData);
}

//============================================================================
// Public getter for ComEventInterface attribute data stored in WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetComEventInterfaceData(ComEventInterfaceData *pData)
{
    if (this == NULL || !m_fComEventInterface)
    {
        if (pData)
        {
            memset(pData, 0, sizeof(*pData));
        }
        return false;
    }

    if (pData)
    {
        ComEventInterfaceData* pComEventInterfaceData = reinterpret_cast<ComEventInterfaceData*>(m_dataTable.GetValue(ComEventInterface_Key));
        *pData = *pComEventInterfaceData;
    }

    return true;
}

//============================================================================
// Public getter for ComEventInterface attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetComEventInterfaceData(ComEventInterfaceData data)
{
    m_fComEventInterface = true;

    ComEventInterfaceData* pComEventInterfaceData = m_pAllocator->Alloc<ComEventInterfaceData>();
    *pComEventInterfaceData = data;

    m_dataTable.SetValue(ComEventInterface_Key, pComEventInterfaceData);
}

//===========================================================================
// Public getter for ComImport attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetComImportData()
{
    if (this == NULL || !m_fComImport)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//===========================================================================
// Public setter for ComImport attribute data stored in AttrVals.
//============================================================================

void WellKnownAttrVals::SetComImportData()
{
    m_fComImport = true;
}

//===========================================================================
// Public getter for WindowsRuntimeImport attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetWindowsRuntimeImportData()
{
    if (this == NULL || !m_fWindowsRuntimeImport)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//===========================================================================
// Public setter for WindowsRuntimeImport attribute data stored in AttrVals.
//============================================================================

void WellKnownAttrVals::SetWindowsRuntimeImportData()
{
    m_fWindowsRuntimeImport = true;
}

//============================================================================
// Public getter for ComSourceInterfaces attribute data stored in WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetComSourceInterfacesData()
{
    if (this == NULL || !m_fComSourceInterfaces)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public setter for ComSourceInterfaces attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetComSourceInterfacesData()
{
    m_fComSourceInterfaces = true;
}

//============================================================================
// Public getter for DispId attribute data stored in WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetDispIdData(long *pDispId)
{
    if (this == NULL || !m_fDispId)
    {
        // Clear OUT params
        *pDispId = 0;

        return false;
    }

// The warnings are that we are casting from void * to long and losing information. Since we
// stored a long for this key, we can suppress this warning.
#pragma warning (push)
#pragma warning (disable:4311)
#pragma warning (disable:4302)
	long dispId = reinterpret_cast<long>(m_dataTable.GetValue(DispId_Key));
#pragma warning (pop)

    // Set OUT params
    *pDispId = dispId;
    return true;
}

//============================================================================
// Public setter for DispId attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetDispIdData(long dispId)
{
    m_fDispId = true;

    m_dataTable.SetValue(DispId_Key, reinterpret_cast<void*>(dispId));
}


bool WellKnownAttrVals::GetBestFitMappingData(bool *pMapping, bool *pThrow)
{
    if (this == NULL || !m_fBestFitMapping)
    {
        if (pMapping)
        {
            *pMapping = false;
        }
        if (pThrow)
        {
            *pThrow = false;
        }
        return false;
    }

    BestFitMappingData* pData = reinterpret_cast<BestFitMappingData*>(m_dataTable.GetValue(BestFitMapping_Key));

    if (pMapping)
    {
        *pMapping = pData->m_BestFitMappingData;
    }
    if (pThrow)
    {
        *pThrow = pData->m_BestFitMappingThrowOnUnmappableChar;
    }
    return true;
}

void WellKnownAttrVals::SetBestFitMappingData(bool fMapping, bool fThrow)
{
    m_fBestFitMapping = true;

    BestFitMappingData* pData = m_pAllocator->Alloc<BestFitMappingData>();
    pData->m_BestFitMappingData = fMapping;
    pData->m_BestFitMappingThrowOnUnmappableChar = fThrow;

    m_dataTable.SetValue(BestFitMapping_Key, pData);
}

bool WellKnownAttrVals::GetFieldOffsetData(int *pOffset)
{
    if (this == NULL || !m_fFieldOffset)
    {
        if (pOffset)
        {
            *pOffset = 0;
        }
        return false;
    }
    if (pOffset)
    {
// The warnings are that we are casting from void * to int and losing information. Since we
// stored a int for this key, we can suppress this warning.
#pragma warning (push)
#pragma warning (disable:4311)
#pragma warning (disable:4302)
		*pOffset = reinterpret_cast<int>(m_dataTable.GetValue(FieldOffset_Key));
#pragma warning (push)
	}
    return true;
}

void WellKnownAttrVals::SetFieldOffsetData(int offset)
{
    m_fFieldOffset = true;
    
    m_dataTable.SetValue(FieldOffset_Key, reinterpret_cast<void*>(offset));
}

bool WellKnownAttrVals::GetLCIDConversionData(int *pData)
{
    if (this == NULL || !m_fLCIDConversion)
    {
        if (pData)
        {
            *pData = 0;
        }
        return false;
    }
    if (pData)
    {
// The warnings are that we are casting from void * to int and losing information. Since we
// stored a int for this key, we can suppress this warning.
#pragma warning (push)
#pragma warning (disable:4311)
#pragma warning (disable:4302)
		*pData = reinterpret_cast<int>(m_dataTable.GetValue(LCIDConversion_Key));
#pragma warning (pop)
    }
    return true;
}

void WellKnownAttrVals::SetLCIDConversionData(int data)
{
    m_fLCIDConversion = true;

    m_dataTable.SetValue(LCIDConversion_Key, reinterpret_cast<void*>(data));
}

bool WellKnownAttrVals::GetInData()
{
    if (this == NULL || !m_fIn)
    {
        return false;
    }
    return true;
}

bool WellKnownAttrVals::GetOutData()
{
    if (this == NULL || !m_fOut)
    {
        return false;
    }
    return true;
}

bool WellKnownAttrVals::GetEmbeddedData()
{
    if (this == NULL || !m_fEmbedded)
    {
        return false;
    }
    return true;
}

bool WellKnownAttrVals::GetCallerLineNumberData()
{
    if (this == NULL || !m_fCallerLineNumber)
    {
        return false;
    }
    return true;
}

void WellKnownAttrVals::SetCallerLineNumberData()
{
    m_fCallerLineNumber = true;
}

bool WellKnownAttrVals::GetCallerFilePathData()
{
    if (this == NULL || !m_fCallerFilePath)
    {
        return false;
    }
    return true;
}

void WellKnownAttrVals::SetCallerFilePathData()
{
    m_fCallerFilePath = true;
}

bool WellKnownAttrVals::GetCallerMemberNameData()
{
    if (this == NULL || !m_fCallerMemberName)
    {
        return false;
    }
    return true;
}

void WellKnownAttrVals::SetCallerMemberNameData()
{
    m_fCallerMemberName = true;
}


bool WellKnownAttrVals::GetUnmanagedFunctionPointerData(long *pUfp)
{
    if (this == NULL || !m_fUnmanagedFunctionPointer)
    {
        if (pUfp)
        {
            *pUfp = 0;
        }
        return false;
    }

    if (pUfp)
    {
// The warnings are that we are casting from void * to long and losing information. Since we
// stored a long for this key, we can suppress this warning.
#pragma warning (push)
#pragma warning (disable:4311)
#pragma warning (disable:4302)
		*pUfp = reinterpret_cast<long>(m_dataTable.GetValue(UnmanagedFunctionPointer_Key));
#pragma warning (pop)
	}
    return true;
}

void WellKnownAttrVals::SetUnmanagedFunctionPointerData(long unmanagedFunctionPointer)
{
    m_fUnmanagedFunctionPointer = true;

    m_dataTable.SetValue(UnmanagedFunctionPointer_Key, reinterpret_cast<void*>(unmanagedFunctionPointer));
}

//===========================================================================
// Public getter for CoClass attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetCoClassData(_Deref_out_opt_z_ WCHAR **ppwszTypeName)
{
    if (this == NULL || !m_fCoClass)
    {
        // Clear OUT params
        *ppwszTypeName = NULL;

        return false;
    }

    WCHAR* pwszTypeName = reinterpret_cast<WCHAR*>(m_dataTable.GetValue(CoClass_Key));

    // Set OUT params
    *ppwszTypeName = pwszTypeName;
    return true;
}

//============================================================================
// Public setter for CoClass attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetCoClassData(_In_z_ WCHAR* pwszTypeName)
{
    m_fCoClass = true;

    m_dataTable.SetValue(CoClass_Key, pwszTypeName);
}

//===========================================================================
// Public getter for CoClass attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetCategoryData(_Deref_out_opt_z_ WCHAR **ppwszCategoryName)
{
    if (this == NULL || !m_fCategory)
    {
        // Clear OUT params
        *ppwszCategoryName = NULL;

        return false;
    }

    WCHAR* pwszCategoryName = reinterpret_cast<WCHAR*>(m_dataTable.GetValue(Category_Key));

    // Set OUT params
    *ppwszCategoryName = pwszCategoryName;
    return true;
}

//============================================================================
// Public setter for CoClass attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetCategoryData(_In_z_ WCHAR* pwszCategoryName)
{
    m_fCategory = true;

    m_dataTable.SetValue(Category_Key, pwszCategoryName);
}

//============================================================================
// Public getter for InterfaceType attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetInterfaceTypeData
(
    long *pInterfaceType     // OUT
)
{
    if (this == NULL || !m_fInterfaceType)
    {
        // Clear OUT params
        *pInterfaceType = 0;
        return false;
    }

// The warnings are is that we are casting from void * to long and losing information. Since we
// stored a long for this key, we can suppress this warning.
#pragma warning (push)
#pragma warning (disable:4311)
#pragma warning (disable:4302)
	long interfaceType = reinterpret_cast<long>(m_dataTable.GetValue(InterfaceType_Key));
#pragma warning (pop)

    // Set OUT params
    *pInterfaceType = interfaceType;
    return true;
}

//============================================================================
// Public getter for InterfaceType attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetInterfaceTypeData(long interfaceType)
{
    COMPILE_ASSERT(sizeof(long) <= sizeof(void*));

    m_fInterfaceType = true;

    m_dataTable.SetValue(InterfaceType_Key, reinterpret_cast<void*>(interfaceType));
}


//============================================================================
// Public getter for TypeLibType attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetTypeLibTypeData
(
    short *pTypeLibType     // OUT
)
{
    if (this == NULL || !m_fTypeLibType)
    {
        // Clear OUT params
        *pTypeLibType = 0;
        return false;
    }

// The warning 4302 is that we are casting from void * to short and losing information. Since we
// stored a short for this key, we can suppress this warning.
#pragma warning (suppress:4302)
    short typeLibType = reinterpret_cast<short>(m_dataTable.GetValue(TypeLibType_Key));

    // Set OUT params
    *pTypeLibType = typeLibType;
    return true;
}

//============================================================================
// Public setter for TypeLibType attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetTypeLibTypeData(short typeLibType)
{
    m_fTypeLibType = true;

    m_dataTable.SetValue(TypeLibType_Key, reinterpret_cast<void*>(typeLibType));
}

//============================================================================
// Public getter for TypeLibFunc attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetTypeLibFuncData
(
    short *pTypeLibFunc     // OUT
)
{
    if (this == NULL || !m_fTypeLibFunc)
    {
        // Clear OUT params
        *pTypeLibFunc = 0;
        return false;
    }

// The warning 4302 is that we are casting from void * to short and losing information. Since we
// stored a short for this key, we can suppress this warning.
#pragma warning (suppress:4302)
    short typeLibFunc = reinterpret_cast<short>(m_dataTable.GetValue(TypeLibFunc_Key));

    // Set OUT params
    *pTypeLibFunc = typeLibFunc;
    return true;
}

//============================================================================
// Public setter for TypeLibFunc attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetTypeLibFuncData(short typeLibFunc)
{
    m_fTypeLibFunc = true;

    m_dataTable.SetValue(TypeLibFunc_Key, reinterpret_cast<void*>(typeLibFunc));
}

//============================================================================
// Public getter for TypeLibVar attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetTypeLibVarData
(
    short *pTypeLibVar     // OUT
)
{
    if (this == NULL || !m_fTypeLibVar)
    {
        // Clear OUT params
        *pTypeLibVar = 0;
        return false;
    }

// The warning 4302 is that we are casting from void * to short and losing information. Since we
// stored a short for this key, we can suppress this warning.
#pragma warning (suppress:4302)
    short typeLibVar = reinterpret_cast<short>(m_dataTable.GetValue(TypeLibVar_Key));

    // Set OUT params
    *pTypeLibVar = typeLibVar;
    return true;
}

//============================================================================
// Public setter for TypeLibVar attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetTypeLibVarData(short typeLibVar)
{
    m_fTypeLibVar = true;

    m_dataTable.SetValue(TypeLibVar_Key, reinterpret_cast<void*>(typeLibVar));
}

//============================================================================
// Public getter for ClassInterface attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetClassInterfaceData
(
    short *pClassInterfaceType   // OUT
)
{
    if (this == NULL || !m_fClassInterface)
    {
        // OUT params
        *pClassInterfaceType = 0;
        return false;
    }

// The warning 4302 is that we are casting from void * to short and losing information. Since we
// stored a short for this key, we can suppress this warning.
#pragma warning (suppress:4302)
    short classInterfaceType = reinterpret_cast<short>(m_dataTable.GetValue(ClassInterface_Key));

    // Set OUT params
    *pClassInterfaceType = classInterfaceType;
    return true;
}

//============================================================================
// Public setter for ClassInterface attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetClassInterfaceData(short classInterfaceType)
{
    m_fClassInterface = true;

    m_dataTable.SetValue(ClassInterface_Key, reinterpret_cast<void*>(classInterfaceType));
}
//============================================================================
// Public getter for COMVisible attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetCOMVisibleData
(
    bool *pfCOMVisible   // OUT
)
{
    if (this == NULL || !m_fCOMVisible)
    {
        // Clear OUT params
        *pfCOMVisible = true;
        return false;
    }

// The warning 4302 is that we are casting from void * to bool and losing information. Since we
// stored a bool for this key, we can suppress this warning.
#pragma warning (suppress:4302)
    bool fCOMVisible = reinterpret_cast<bool>(m_dataTable.GetValue(COMVisible_Key));

    // Set OUT params
    *pfCOMVisible = fCOMVisible;
    return true;
}

//============================================================================
// Public setter for COMVisible attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetCOMVisibleData(bool fCOMVisible)
{
    m_fCOMVisible = true;

    m_dataTable.SetValue(COMVisible_Key, reinterpret_cast<void*>(fCOMVisible));
}

//============================================================================
// Public getter for ParamArray attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetParamArrayData()
{
    if (this == NULL || !m_fParamArray)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public setter for ParamArray attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetParamArrayData()
{
    m_fParamArray = true;
}

//============================================================================
// Public getter for DecimalConstant attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetDecimalConstantData
(
    DECIMAL *Value
)
{
    if (this == NULL || !m_fDecimalConstant)
    {
        // Clear OUT params
        memset(Value, 0, sizeof(DECIMAL));
        return false;
    }

    DecimalConstantData* pData = reinterpret_cast<DecimalConstantData*>(m_dataTable.GetValue(DecimalConstant_Key));

    // Set OUT params
    *Value = pData->m_Value;
    return true;
}

//============================================================================
// Public getter for DecimalConstant attribute data stored in
// WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetDecimalConstantData(DECIMAL value)
{
    m_fDecimalConstant = true;

    DecimalConstantData* pData = m_pAllocator->Alloc<DecimalConstantData>();
    pData->m_Value = value;

    m_dataTable.SetValue(DecimalConstant_Key, pData);
}

//============================================================================
// Public getter for DateTimeConstant attribute data stored in 
// WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetDateTimeConstantData
(
    __int64 *Value
)
{
    if (this == NULL || !m_fDateTimeConstant)
    {
        // Clear OUT params
        *Value = 0;
        return false;
    }

    DateTimeConstantData* pData = reinterpret_cast<DateTimeConstantData*>(m_dataTable.GetValue(DateTimeConstant_Key));

    // Set OUT params
    *Value = pData->m_Value;
    return true;
}

//============================================================================
// Public getter for DateTimeConstant attribute data stored in 
// WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetDateTimeConstantData(__int64 value)
{
    m_fDateTimeConstant = true;

    DateTimeConstantData* pData = m_pAllocator->Alloc<DateTimeConstantData>();
    pData->m_Value = value;

    m_dataTable.SetValue(DateTimeConstant_Key, pData);
}

//============================================================================
// Public getter for IDispatchConstant attribute data stored in
// WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetIDispatchConstantData()
{
    if (this == NULL || !m_fIDispatchConstant)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public setter for IDispatchConstant attribute data stored in
// WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetIDispatchConstantData()
{
    m_fIDispatchConstant = true;
}

//============================================================================
// Public getter for IUnknownConstant attribute data stored in
// WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetIUnknownConstantData()
{
    if (this == NULL || !m_fIUnknownConstant)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public setter for IUnknownConstant attribute data stored in
// WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetIUnknownConstantData()
{
    m_fIUnknownConstant = true;
}

//============================================================================
// Public getter for DefaultEvent attribute data stored in WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetDefaultEventData
(
    _Deref_out_z_ WCHAR **ppwszDefaultEvent,
    _Deref_out_opt_ BCSYM_Class** ppPartialClassOnWhichApplied
)
{
    if (this == NULL || !m_fDefaultEvent)
    {
        // Clear OUT params
        *ppwszDefaultEvent = NULL;

        return false;
    }

    DefaultEventData* pData = reinterpret_cast<DefaultEventData*>(m_dataTable.GetValue(DefaultEvent_Key));

    // Set OUT params
    *ppwszDefaultEvent = pData->m_pwszDefaultEvent;
    if (ppPartialClassOnWhichApplied)
    {
        *ppPartialClassOnWhichApplied = pData->m_PartialClassOnWhichApplied;
    }

    return true;
}

//============================================================================
// Public setter for DefaultEvent attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetDefaultEventData(_In_z_ WCHAR* pwszDefaultEvent, BCSYM_Class* pPartialClassOnWhichApplied)
{
    m_fDefaultEvent = true;

    DefaultEventData* pData = m_pAllocator->Alloc<DefaultEventData>();

    pData->m_pwszDefaultEvent = pwszDefaultEvent;
    pData->m_PartialClassOnWhichApplied = pPartialClassOnWhichApplied;

    m_dataTable.SetValue(DefaultEvent_Key, pData);
}

//============================================================================
// Public getter for DefaultValue attribute data stored in WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetDefaultValueData(ConstantValue* pDefaultValue, bool* pfIsNotHandled)
{
    if (this == NULL || !m_fDefaultValue)
    {
        // Clear OUT params
        memset(pDefaultValue, 0, sizeof(*pDefaultValue));
        *pfIsNotHandled = false;

        return false;
    }

    DefaultValueData* pData = reinterpret_cast<DefaultValueData*>(m_dataTable.GetValue(DefaultValue_Key));
    *pDefaultValue = pData->m_DefaultValue;
    *pfIsNotHandled = pData->m_fIsNotHandled;

    return true;
}

//============================================================================
// Public setter for DefaultValue attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetDefaultValueData(ConstantValue defaultValue, bool fIsNotHandled)
{
    m_fDefaultValue = true;

    DefaultValueData* pData = m_pAllocator->Alloc<DefaultValueData>();

    pData->m_DefaultValue = defaultValue;
    pData->m_fIsNotHandled = fIsNotHandled;

    m_dataTable.SetValue(DefaultValue_Key, pData);
}

//============================================================================
// Public getter for HelpKeyword attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetHelpKeywordData
(
    _Deref_out_opt_z_ WCHAR **ppwszHelpKeyword  // OUT
)
{
    if (this == NULL || !m_fHelpKeyword)
    {
        // Clear OUT params
        *ppwszHelpKeyword = NULL;
        return false;
    }

    HelpKeywordData* pData = reinterpret_cast<HelpKeywordData*>(m_dataTable.GetValue(HelpKeyword_Key));

    // Set out params
    *ppwszHelpKeyword = pData->m_pwszHelpKeyword;
    return true;
}

//============================================================================
// Public setter for HelpKeyword attribute data stored in AttrVals.
//============================================================================

void WellKnownAttrVals::SetHelpKeywordData(_In_z_ WCHAR* pwszHelpKeyword)
{
    m_fHelpKeyword = true;

    HelpKeywordData* pData = m_pAllocator->Alloc<HelpKeywordData>();

    pData->m_pwszHelpKeyword = pwszHelpKeyword;

    m_dataTable.SetValue(HelpKeyword_Key, pData);
}

//============================================================================
// Public getter for NonSerialized attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetNonSerializedData()
{
    if (this == NULL || !m_fNonSerialized)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public setter for NonSerialized attribute data stored in AttrVals.
//============================================================================
void WellKnownAttrVals::SetNonSerializedData()
{
    m_fNonSerialized = true;
}

//============================================================================
// Public getter for Serializable attribute data stored in AttrVals.
//============================================================================
bool WellKnownAttrVals::GetSerializableData()
{
    if (this == NULL || !m_fSerializable)
    {
        // No OUT params
        return false;
    }

    // No OUT params
    return true;
}

//============================================================================
// Public setter for Serializable attribute data stored in WellKnownAttrVals.
//============================================================================

void WellKnownAttrVals::SetSerializableData()
{
    m_fSerializable = true;
}

//============================================================================
// Public getter for WebMethod attribute data stored in WellKnownAttrVals.
//============================================================================
bool WellKnownAttrVals::GetWebMethodData
(
    AttrKind *pAttrId,
    bool *pfEnableSession,
    short *pTransactionOption,
    int *pCacheDuration,
    bool *pBufferResponse,
    _Deref_out_opt_z_ WCHAR **pwszDescription
)
{
    if (this == NULL || !m_fWebMethod)
    {
        // Clear OUT params
        *pAttrId = attrkindNil;
        *pfEnableSession = false;
        *pTransactionOption = 0;
        *pCacheDuration = 0;
        *pBufferResponse = false;
        *pwszDescription = NULL;

        return false;
    }

    WebMethodData* pData = reinterpret_cast<WebMethodData*>(m_dataTable.GetValue(WebMethod_Key));

    // Set OUT params
    *pAttrId = pData->m_attrId;
    *pfEnableSession = pData->m_fEnableSession;
    *pTransactionOption = pData->m_TransactionOption;
    *pCacheDuration = pData->m_CacheDuration;
    *pBufferResponse = pData->m_fBufferResponse;
    *pwszDescription = pData->m_pwszDescription;

    return true;
}

//============================================================================
// Public getter for WebMethod attribute data stored in WellKnownAttrVals.
//============================================================================
void WellKnownAttrVals::SetWebMethodData(AttrKind attrId,
                      bool fEnableSession,
                      short transactionOption,
                      int cacheDuration,
                      bool fBufferResponse,
                      _In_z_ WCHAR* pwszDescription)
{
    m_fWebMethod = true;

    WebMethodData* pData = m_pAllocator->Alloc<WebMethodData>();
    pData->m_attrId = attrId;
    pData->m_fEnableSession = fEnableSession;
    pData->m_TransactionOption = transactionOption;
    pData->m_CacheDuration = cacheDuration;
    pData->m_fBufferResponse = fBufferResponse;
    pData->m_pwszDescription = pwszDescription;

    m_dataTable.SetValue(WebMethod_Key, pData);
}

//============================================================================
// Public getter for WebService attribute data stored in AttrVals.
//============================================================================

bool WellKnownAttrVals::GetWebServiceData(WCHAR** ppwszNamespace, WCHAR** ppwszDescription, WCHAR** ppwszName)
{
    if (this == NULL || !m_fWebService)
    {
        // Clear the out parameters
        *ppwszNamespace = NULL;
        *ppwszDescription = NULL;
        *ppwszName = NULL;
    }

    WebServiceData* pData = reinterpret_cast<WebServiceData*>(m_dataTable.GetValue(WebService_Key));

    *ppwszNamespace = pData->m_pwszNamespace;
    *ppwszDescription = pData->m_pwszDescription;
    *ppwszName = pData->m_pwszName;

    return true;
}

//============================================================================
// Public setter for WebService attribute data stored in AttrVals.
//============================================================================

void WellKnownAttrVals::SetWebServiceData(_In_z_ WCHAR* pwszNamespace, _In_z_ WCHAR* pwszDescription, _In_z_ WCHAR* pwszName)
{
    m_fWebService = true;

    WebServiceData* pData = m_pAllocator->Alloc<WebServiceData>();

    pData->m_pwszName = pwszName;
    pData->m_pwszNamespace = pwszNamespace;
    pData->m_pwszDescription = pwszDescription;

    m_dataTable.SetValue(WebService_Key, pData);
}

//============================================================================
// This method is used to obtain the bound trees for the parameters of an
// attribute.
//============================================================================
ILTree::Expression *GetBoundTreesForAttribute
(
    BCSYM_Hash      *psymScope,
    BCSYM_NamedRoot *pnamedContextOfApplAttr,
    BCSYM_ApplAttr  *papplattr,
    ErrorTable      *pErrorTable,
    NorlsAllocator  *pnrls,
    CompilerHost    *pCompilerHost
)
{
    BCSYM *psymDigResult = papplattr->GetAttributeSymbol()->DigThroughNamedType();
    if (!psymDigResult->IsClass() || psymDigResult->IsEnum())
    {
        // 
        return NULL;
    }
    BCSYM_Class *psymAttrClass = psymDigResult->PClass();
    WCHAR *wszExpression = papplattr->GetExpression()->GetExpressionText();
    Location Hidden = Location::GetHiddenLocation();
    Location *ploc = papplattr->GetExpression()->HasLocation() ?
        papplattr->GetExpression()->GetLocation() : &Hidden;

    NorlsAllocator nrlsParseTree(NORLSLOC);
    ParseTree::Expression *pexprParseTree = NULL;

    if (!pCompilerHost)
    {
        pCompilerHost = psymAttrClass->GetCompiler()->GetDefaultCompilerHost();
    }

    CompilerFile* pFile = pnamedContextOfApplAttr->GetCompilerFile();
    Parser parser(
        &nrlsParseTree,
        psymAttrClass->GetCompiler(),
        pCompilerHost,
        false,
        pFile ? pFile->GetProject()->GetCompilingLanguageVersion() : LANGUAGE_CURRENT);
    Semantics symanalyzer(pnrls,
                          pErrorTable,
                          psymAttrClass->GetCompiler(),
                          pCompilerHost,
                          pnamedContextOfApplAttr->GetSourceFile(),
                          NULL,
                          false);

    //
    // Generate the stream of tokens and parse them.
    //
    Scanner tokstream(psymAttrClass->GetCompiler(), 
                      wszExpression, 
                      VBMath::Convert<long>(wcslen(wszExpression)), 
                      0, 
                      ploc->m_lBegLine, 
                      ploc->m_lBegColumn );

    BCSYM_Container *pProjectLevelCondCompScope = NULL;
    BCSYM_Container *pConditionalCompilationConstants = NULL;

    // Dev10 #789970 Get proper conditional complation scope.
    BCSYM_NamedType * pNamedAttrClass = papplattr->GetAttrClass();    

    if (pNamedAttrClass != NULL)
    {
        BCSYM_NamedRoot * pContext = pNamedAttrClass->GetContext();

        if (pContext != NULL)
        {
            CompilerFile * pCompilerFile = pContext->GetContainingCompilerFile();

            if (pCompilerFile != NULL && pCompilerFile->IsSourceFile())
            {
                SourceFile * pSourceFile = pCompilerFile->PSourceFile();
                CompilerProject * pProject = pSourceFile->GetCompilerProject();

                if (pProject != NULL)
                {
                    pProjectLevelCondCompScope = pProject->GetProjectLevelCondCompScope();
                }
                
                pConditionalCompilationConstants = pSourceFile->GetConditionalCompilationConstants();
            }
        }
    }
    
    IfFailThrow(parser.ParseOneExpression(&tokstream, pErrorTable, &pexprParseTree, NULL,
                                pProjectLevelCondCompScope, pConditionalCompilationConstants));

    // Check if there was any syntax error.
    if (pexprParseTree->Opcode == ParseTree::Expression::SyntaxError)
    {
        goto SyntaxError;
    }

    //
    // If the parser did not create a CallOrIndex node, create one. This
    // will happen if the user did not type any parenthesis.
    //

    if (pexprParseTree->Opcode == ParseTree::Expression::Name ||
        pexprParseTree->Opcode == ParseTree::Expression::DotQualified)
    {
        ParseTree::CallOrIndexExpression *CallOrIndexExpression = 
            new (nrlsParseTree) ParseTree::CallOrIndexExpression();
        IfNullThrow(CallOrIndexExpression);

        CallOrIndexExpression->Opcode = ParseTree::Expression::CallOrIndex;
        CallOrIndexExpression->Target = pexprParseTree;
        CallOrIndexExpression->TextSpan = *ploc;
        CallOrIndexExpression->AlreadyResolvedTarget = false;
        pexprParseTree = CallOrIndexExpression;
    }
    else if (pexprParseTree->Opcode != ParseTree::Expression::CallOrIndex)
    {
        // Not a legal opcode for an attribute.  Treat like a syntax error.
        goto SyntaxError;
    }

    //
    // Munge the trees so that we have a constructor call that ends in ".New"

    ParseTree::QualifiedExpression *Qualified = 
        new (nrlsParseTree) ParseTree::QualifiedExpression();
    IfNullThrow(Qualified);

    ParseTree::NameExpression *Name =
        new (nrlsParseTree) ParseTree::NameExpression();
    IfNullThrow(Name);
    Name->Opcode = ParseTree::Expression::Name;
    Name->Name.Name = psymAttrClass->GetCompiler()->AddString(L"New");

    Qualified->Name = Name;
    Qualified->Opcode = ParseTree::Expression::DotQualified;
    Qualified->Base = pexprParseTree->AsCallOrIndex()->Target;
    Qualified->TextSpan = *ploc;
    pexprParseTree->AsCallOrIndex()->Target = Qualified;
       	
    // We also need to munge the name of the attribute class to end in "Attribute" if needed!
    // The attribute name in Qualified->Base is not going to be interpreted as an attribute
    // when the constructor call is analyzed. Semantics of with/without ?Attribute? suffix
    // will not apply. We need to make it up for it here and make sure the exact name of the
    // class is specified in constructor call.
    if (papplattr->GetAttributeSymbol()->DigThroughNamedType()->IsClass())
    {
        ParseTree::IdentifierDescriptor *AttributeName = NULL;
        if (Qualified->Base->Opcode == ParseTree::Expression::Name)
        {
            // the attribute invocation is a simple name.
            // It can be the exact name of the class. It can be the name of an imports. Or,
            // it can be the same name as the class but actually an import,
            // i.e "imports foo=NS1.NS2.fooAttribute"
            // the safe way is to provide the fully qualified name of the class atribute is bound to.
            ParseTree::Expression *fullNameParseTree = NULL;
            STRING* wszFullName=papplattr->GetAttributeSymbol()->DigThroughNamedType()->PClass()->GetGlobalQualifiedName();
            
            Scanner tokstreamFullName(psymAttrClass->GetCompiler(), 
                                      wszFullName, 
                                      VBMath::Convert<long>(wcslen(wszFullName)), 
                                      0, 
                                      ploc->m_lBegLine, 
                                      ploc->m_lBegColumn );

            IfFailThrow(parser.ParseOneExpression(&tokstreamFullName, pErrorTable, &fullNameParseTree));

            // The scanner above will parse a bigger string than is in the original source file. The tokens and symbols produced here will therefore also be outside of the
            // space that fits in the line. This causes issues on editing since the editor might clear the tracked location of the symbol if there is an error for that symbol.
            // I attempted to use a bound symbol, this works but it relies on this to report the errors about the symbol (accessiblity, obsolete etc)
            // I don't want to mimic that logic here now this late in the cycle. so I'll just walk the parsetree and fixup the locations.
            // Next release we should eliminate all custom parsing. We should just build parstree's instead of strings and only use strings as the sourcefile.
            LocationFixupVisitor fixupVisitor(ploc);
            fixupVisitor.Fixup(fullNameParseTree);

            Qualified->Base = fullNameParseTree;

        }
        else if (Qualified->Base->Opcode == ParseTree::Expression::DotQualified)
        {
            VSASSERT(Qualified->Base->AsQualified()->Name != NULL, "QualifiedExpression::Name must be set!");

            if(Qualified->Base->AsQualified()->Name &&
                Qualified->Base->AsQualified()->Name->Opcode == ParseTree::Expression::Name)
            {
                // in qualified case imports are out of the question, go and substitute anyway
                AttributeName = &Qualified->Base->AsQualified()->Name->AsName()->Name;
            }
            else
            {
                goto SyntaxError;
            }
        }
        else
        {
            goto SyntaxError;
        }
        if (AttributeName) //just a name change
        {
            AttributeName->Name = papplattr->GetAttributeSymbol()->DigThroughNamedType()->PClass()->GetName();
        }
    }


    //
    // Remove the named arguments from the list of arguments.
    //

    ParseTree::ArgumentList *CurrentArgument =
        pexprParseTree->AsCallOrIndex()->Arguments.Values;
    ParseTree::ArgumentList *PreviousArgument = NULL;
    ParseTree::ArgumentList *NamedArguments = NULL;

    while (CurrentArgument &&
            (!CurrentArgument->Element ||
             !CurrentArgument->Element->Name.Name))
    {
        PreviousArgument = CurrentArgument;
        CurrentArgument = CurrentArgument->Next;
    }

    if (CurrentArgument)
    {
        if (PreviousArgument)
        {
            PreviousArgument->Next = NULL;
        }
        else
        {
            pexprParseTree->AsCallOrIndex()->Arguments.Values = NULL;
        }

        NamedArguments = CurrentArgument;
    }

    //
    // Now ask the Semantic Analyzer to generate the bound trees.
    //

    return symanalyzer.InterpretAttribute(
                                        pexprParseTree,
                                        NamedArguments,
                                        psymScope,
                                        pnamedContextOfApplAttr,
                                        psymAttrClass,
                                        ploc);

SyntaxError:
    return NULL;
}


//============================================================================
// Inherits well-known custom attributes settings.  Copies settings
// out of psymBase and into 'this'.
//============================================================================

void WellKnownAttrVals::SetCLSCompliantData(bool fCompliant, bool fIsInherited)
{
    m_fCLSCompliant = true;

    CLSCompliantData* pData = m_pAllocator->Alloc<CLSCompliantData>();
    pData->m_fCompliant = fCompliant;
    pData->m_fIsInherited = fIsInherited;

    m_dataTable.SetValue(CLSCompliant_Key, pData);
}

void WellKnownAttrVals::SetDesignerCategoryData(_In_z_ WCHAR* pwszDesignerCategory)
{
    m_fDesignerCategory = true;

    DesignerCategoryData* pData = m_pAllocator->Alloc<DesignerCategoryData>();
    pData->m_pwszCategory = pwszDesignerCategory;

    m_dataTable.SetValue(DesignerCategory_Key, pData);
}

void WellKnownAttrVals::SetAttributeUsageData(CorAttributeTargets targetsValidOn, bool fAllowMultiple, bool fInherited)
{
    m_fAttributeUsage = true;

    AttributeUsageData* pData = m_pAllocator->Alloc<AttributeUsageData>();
    pData->m_attrtargetsValidOn = targetsValidOn;
    pData->m_fAllowMultiple = fAllowMultiple;
    pData->m_fInherited = fInherited;

    m_dataTable.SetValue(AttributeUsage_Key, pData);
}

void WellKnownAttrVals::InheritFromSymbol(BCSYM *psymBase)
{
    WellKnownAttrVals *pAttrValsBase = psymBase->GetPWellKnownAttrVals();
    VSASSERT(pAttrValsBase != NULL, "Nothing to inherit!");

    //
    // Note that we use the GetXXXData methods here instead of directly
    // accessing the contents of pAttrValsBase.  We do this so that
    // additional inheritable data added to the AttrVals will generate
    // a compilation error when the GetXXXData method is udpated.
    //

    //
    // CLSCompliantAttribute (AllowMultiple = false, Inherited = true)
    //
    bool fCLSCompliant;
    if (!m_fCLSCompliant && pAttrValsBase->GetCLSCompliantData(&fCLSCompliant))
    {
        SetCLSCompliantData(fCLSCompliant, true /* fIsInherited */);
    }

    //
    // ComSourceInterfacesAttribute(ValidOn = Class, AllowMultiple = false, Inherited = true)
    //
    if (!m_fComSourceInterfaces && pAttrValsBase->GetComSourceInterfacesData())
    {
        m_fComSourceInterfaces = true;
    }

    //
    // DesignerCategoryAttribute (AllowMultiple = false, Inherited = true)
    //
    WCHAR *pwszDesignerCategory;
    if (!m_fDesignerCategory && pAttrValsBase->GetDesignerCategoryData(&pwszDesignerCategory))
    {
        SetDesignerCategoryData(pwszDesignerCategory);
    }

    //
    // AttributeUsage(ValidOn = Class, AllowMultiple = false, Inherited = true)
    //
    CorAttributeTargets targetsValidOn;
    bool fAllowMultiple;
    bool fInherited;
    if (!m_fAttributeUsage &&
        pAttrValsBase->GetAttributeUsageData(
            &targetsValidOn,
            &fAllowMultiple,
            &fInherited))
    {
        SetAttributeUsageData(targetsValidOn, fAllowMultiple, fInherited);
    }

    //
    // ParamArrayAttribute(AllowMultiple = false, Inherited = true)
    //
    // This attribute doesn't apply to classes, so it really can't be inherited.
    VSASSERT(!m_fParamArray && !pAttrValsBase->GetParamArrayData(),
        "Attribute shouldn't be inherited");

    //
    // NonSerializedAttribute(AllowMultiple = false, Inherited = true)
    //
    // This attribute doesn't apply to classes, so it really can't be inherited.
    VSASSERT(!m_fNonSerialized && !pAttrValsBase->GetNonSerializedData(),
        "Attribute shouldn't be inherited");
}

WellKnownAttrVals::WellKnownAttrVals(NorlsAllocator* pAllocator) :
    m_pAllocator(pAllocator),
    m_dataTable(pAllocator, DefaultHashFunction<WellKnownAttrKindKeys>(), 1UL /* capacity */)
{
}

STRING *CLRTypeConstantEncoderDecoder::TypeToCLRName
(
    BCSYM *Type,
    CompilerProject *ProjectContext,
    MetaEmit *MetaEmit,
    bool IncludeAssemblySpec /*= true*/,
    ErrorTable * pErrorTable /*= NULL*/, 
    Location * pLocation /*= NULL*/
)
{

    m_UseStringVersion = m_CompilerHost != NULL && m_CompilerHost->GetRuntimeVersion() < RuntimeVersion2;

    InitCurrentContext(ProjectContext, MetaEmit, pErrorTable, pLocation);

    if (!m_UseStringVersion)
    {
        InitTypeNameBuilder();

        VSASSERT(m_TypeNameBuilder, "NULL TypeNameBuilder unexpected!!!");
    }
    else
    {
        m_sbTypeNameBuilder.Clear();
    }

    EncodeType(Type, IncludeAssemblySpec);

    STRING *CLRTypeName = NULL;

    // ToString
    if (!m_UseStringVersion)
    {
        CComBSTR bstrEncodedTypeName = NULL;

        IfFailThrow(m_TypeNameBuilder->ToString(&bstrEncodedTypeName));

        VSASSERT(bstrEncodedTypeName, "Getting the encoded type name failed unexpectedly!!!");

        // 





        CLRTypeName = m_Compiler->AddString((WCHAR *)bstrEncodedTypeName);
    }
    else
    {
        CLRTypeName = m_Compiler->AddString(m_sbTypeNameBuilder.GetString());

        m_sbTypeNameBuilder.Clear();
    }

    // clear the current project and metainfo context
    ClearCurrentContext();

    return CLRTypeName;
}

void CLRTypeConstantEncoderDecoder::EncodeType
(
    BCSYM *Type,
    bool IncludeAssemblySpec
)
{
    VSASSERT(Type && !Type->IsBad(), "Bad type symbol unexpected!!!");
    VSASSERT(Type->ChaseToType()->IsContainer(), "Non-container type unexpected!!!");
    VSASSERT(!Type->IsPointerType(), "pointed type unexpected!!!");

    if (Type->IsNamespace())
    {
        STRING *Name;
        Name = Type->PNamedRoot()->GetQualifiedEmittedName();

        if (!m_UseStringVersion && Name)
        {
            IfFailThrow(m_TypeNameBuilder->AddName(Name));
        }
        else if(Name)
        {
            m_sbTypeNameBuilder.AppendSTRING(Name);
        }

        return;
    }

    VSASSERT(Type->IsType(), "Non-type symbol unexpected!!!");

    BCSYM *ChasedType = Type->ChaseToType();
    BCSYM_Container *Container = ChasedType->PContainer();

    // Encode type name
    EncodeTypeName(Container);

    // Encode generic type arguments
    EncodeGenericTypeArguments(ChasedType);

    // Encode array
    EncodeArray(Type);

    if (IncludeAssemblySpec)
    {
        // Encode Assembly name
        EncodeAssemblyName(Container);
    }
}

void CLRTypeConstantEncoderDecoder::EncodeAssemblyName
(
    BCSYM_Container *Container
)
{
    {
        CompilerProject *Project = Container->GetContainingProject();

        // We only need to prepend the assembly type when we're referencing
        // a type from an external assembly (not from a code module, and
        // not from within the current project).

        if (Project != NULL &&                                                              // Symbol must come from some project...
            Project != m_CurrentProjectContext &&                                           // ... but not this one ...
            !Project->IsCodeModule())                                                       // ... and not from a code module.
        {
            bool fIncludeAssemblySpec = true;

            // Dev10 #748848
            // Dev10 #685614 Do not prepend the assembly for an embeddable type, it will be in this assembly.
            // We want to do this check only when we are actually emitting attributes, [m_Metaemit != NULL] condition should ensure this.
            // This function is also called earlier when we are cracking attributes, if we cache the type then, but never reach CS_TypesEmitted
            // and in the process the source changes so that there is no attribute, we may be left with unused type in the cache because
            // the cache is cleared only if we decompile from TypesEmitted to Bound. 
            if (m_Metaemit != NULL &&
                m_CurrentProjectContext != NULL &&
                m_pErrorTable != NULL) 
            {
                AssertIfFalse(m_CurrentProjectContext->GetCompState() >= CS_TypesEmitted);

                Compiler * pCompiler = m_CurrentProjectContext->GetCompiler();
                CompilerProject * pProjectBeingCompiled = pCompiler->GetProjectBeingCompiled();

                AssertIfFalse(pProjectBeingCompiled == m_CurrentProjectContext);
                pCompiler->SetProjectBeingCompiled(m_CurrentProjectContext);
                
                if (TypeHelpers::IsEmbeddableInteropType(Container))    
                {
                    fIncludeAssemblySpec = false;
                    
                    // Make sure we embed the type
                    m_CurrentProjectContext->CachePiaTypeRef(Container, &m_Location, m_pErrorTable);
                }

                pCompiler->SetProjectBeingCompiled(pProjectBeingCompiled);
            }

            if (fIncludeAssemblySpec)
            {   
                // Deployment will need to know about the project this type comes from.
                // If emitting types (not going to bound), add an assembly ref to the project if one does not exist.
                if (m_Metaemit)
                {
                    // LookupProjectRef adds an AssemblyRef to the assembly if it does not exist
                    m_Metaemit->LookupProjectRef(Project, Container, NULL);
                }
                // Can be NULL for the expression evaluator
                else if (m_CurrentProjectContext)
                {
                    VSASSERT(m_CurrentProjectContext->GetCompState() < CS_TypesEmitted, "No metaemit object while encoding.");
                }

                if (!m_UseStringVersion)
                {
                    IfFailThrow(m_TypeNameBuilder->AddAssemblySpec(Project->GetAssemblyIdentity()->GetAssemblyIdentityString()));
                }
                else
                {
                    m_sbTypeNameBuilder.AppendString(L", ");
                    m_sbTypeNameBuilder.AppendString(Project->GetAssemblyIdentity()->GetAssemblyIdentityString());
                }
            }
        }
    }
}

void CLRTypeConstantEncoderDecoder::EncodeTypeName
(
    BCSYM_Container *Container
)
{
    BCSYM_Container *Parent = Container->GetContainer();
    STRING *Name;

    if (Parent->IsNamespace())
    {
        Name = Container->GetQualifiedEmittedName();
    }
    else
    {
        // Encode the parent type
        EncodeTypeName(Parent);

        // Encode the nested type
        Name = (Container->GetEmittedName() ? Container->GetEmittedName() : Container->GetName());
    }

    if (!m_UseStringVersion)
    {
        IfFailThrow(m_TypeNameBuilder->AddName(Name));
    }
    else
    {
        if (m_sbTypeNameBuilder.GetStringLength() > 0)
        {
            m_sbTypeNameBuilder.AppendChar(L'+');
        }

        m_sbTypeNameBuilder.AppendSTRING(Name);
    }
}

void CLRTypeConstantEncoderDecoder::EncodeGenericTypeArguments
(
    BCSYM *Type
)
{
    if (!Type->IsGenericTypeBinding())
    {
        return;
    }

    AssertIfTrue(m_UseStringVersion);

    // THe string version doesn't support generics.
    if (!m_UseStringVersion)
    {
        IfFailThrow(m_TypeNameBuilder->OpenGenericArguments());

        EncodeTypeArgumentsInOrder(Type->PGenericTypeBinding());

        IfFailThrow(m_TypeNameBuilder->CloseGenericArguments());
    }
}

void CLRTypeConstantEncoderDecoder::EncodeTypeArgumentsInOrder
(
    BCSYM_GenericTypeBinding *Binding
)
{
    AssertIfTrue(m_UseStringVersion);

    // Note that type arguments are supposed to be encoded starting with
    // the parent binding's type arguments appearing first.

    if (Binding->GetParentBinding())
    {
        EncodeTypeArgumentsInOrder(Binding->GetParentBinding());
    }

    unsigned ArgumentCount = Binding->GetArgumentCount();

    for (unsigned Index = 0; Index < ArgumentCount; Index++)
    {
        IfFailThrow(m_TypeNameBuilder->OpenGenericArgument());

        EncodeType(Binding->GetArgument(Index));

        IfFailThrow(m_TypeNameBuilder->CloseGenericArgument());
    }
}

void CLRTypeConstantEncoderDecoder::EncodeArray
(
    BCSYM *Type
)
{
    if (!Type->IsArrayType())
    {
        return;
    }

    BCSYM_ArrayType *ArrayType = Type->PArrayType();
    unsigned Rank = ArrayType->GetRank();

    // The COM+ metadata uses a C#-inspired string representation for
    // arrays, but with a twist: the arrays are listed in reverse order.
    // For example, "GetType(integer(,,,,)()(,,))"
    // is encoded as "System.Int32[,,][][,,,,]".
    //

    EncodeArray(ArrayType->GetRoot());

    if (Rank == 1)
    {
        if (!m_UseStringVersion)
        {
            IfFailThrow(m_TypeNameBuilder->AddSzArray());
        }
        else
        {
            m_sbTypeNameBuilder.AppendString(L"[]");
        }
    }
    else
    {
        if (!m_UseStringVersion)
        {
            IfFailThrow(m_TypeNameBuilder->AddArray((DWORD)Rank));
        }
        else
        {
            m_sbTypeNameBuilder.AppendChar(L'[');

            while (Rank > 1)
            {
                m_sbTypeNameBuilder.AppendChar(L',');
                Rank--;
            }

            m_sbTypeNameBuilder.AppendChar(L']');
        }
    }
}

WCHAR *CLRTypeConstantEncoderDecoder::CLRNameToVBName
(
    _In_opt_z_ WCHAR *CLRTypeNameString,
    NorlsAllocator *Allocator,
    CompilerProject *ProjectContext,
    MetaEmit *MetaEmit
)
{
    // 





    if (CLRTypeNameString == NULL)
    {
        return NULL;
    }

    InitCurrentContext(ProjectContext, MetaEmit);
    InitTypeNameBuilder();

    WCHAR *VBTypeName = NULL;
    unsigned VBTypeNameLength = 0;
    CLRTypeName *TypeInfo = NULL;
    DWORD ErrorInfo = 0;
    DWORD Count = 0;
    const unsigned MaxScratchNames = 8;
    BSTR Buffer[MaxScratchNames];
    BSTR *Names = Buffer;
    NorlsAllocator ScratchAllocator(NORLSLOC);
    DWORD NamesRead = 0;
    HRESULT hr = NOERROR;
    WCHAR *Tmp = NULL;

    IfFailGo(CLRTypeName::ParseTypeName(CLRTypeNameString, &ErrorInfo, &TypeInfo));
    IfFailGo(TypeInfo->GetNameCount(&Count));

    if (Count == 0)
    {
        VSFAIL("Unnamed type unexpected!!!");
        goto Error;
    }

    if (Count > MaxScratchNames)
    {
        Names = (BSTR *)ScratchAllocator.Alloc(VBMath::Multiply(Count, sizeof(BSTR)));
    }

    IfFailGo(TypeInfo->GetNames(Count, Names, &NamesRead));

    VSASSERT(Count == NamesRead, "Inconsistencies during system type name parsing!!!");

    // for safety
    Count = NamesRead;

    // Calculate the length of the required string (with overflow check)
    unsigned Index;
    for (Index = 0; Index < Count; Index++)
    {
#pragma prefast(suppress: 26017, "Count is bounded by the namesread out parameter")
        IfFalseThrow((VBTypeNameLength += SysStringLen(Names[Index])) >= SysStringLen(Names[Index]));
    }

    // Add the space for the delimiters i.e. the "."
    // Overflow check:
    VBTypeNameLength = VBMath::Add(VBTypeNameLength, (Count - 1));  // We know that Count is 
                                                                    // greater than 0 by this point

    // Alloc the string
    VBTypeName = (WCHAR *)Allocator->Alloc(VBMath::Multiply(
        VBMath::Add(VBTypeNameLength, 1),
        sizeof(WCHAR)));
    Tmp = VBTypeName;

    // Append the names

    for (Index = 0; Index < Count; Index++)
    {
        if (Index != 0)
        {
            *Tmp = L'.';
            Tmp++;
        }

        if (Names[Index])
        {
            memcpy(Tmp, Names[Index], SysStringLen(Names[Index]) * sizeof(WCHAR)); // We know this math is safe if we got this far
            Tmp += SysStringLen(Names[Index]);

            SysFreeString(Names[Index]);
        }
    }

    *Tmp = 0;
    VSASSERT(wcslen(VBTypeName) == VBTypeNameLength, "Inconsistency during system type name parsing!!!");

    // for safety
    VBTypeName[VBTypeNameLength] = 0;

    goto Exit;

Error:
    VBTypeName = NULL;

Exit:
    RELEASE(TypeInfo);

    ClearCurrentContext();

    return VBTypeName;
}

void CLRTypeConstantEncoderDecoder::InitTypeNameBuilder()
{
    AssertIfTrue(m_UseStringVersion);

    if (m_TypeNameBuilder == nullptr)
    {
        m_TypeNameBuilder = new TypeNameBuilder();
    }
    else
    {
        IfFailThrow(m_TypeNameBuilder->Clear());
    }
}

WCHAR *MyWcstok_s(_In_z_ WCHAR *buf, _Inout_z_ WCHAR **context)
{
    static WCHAR wszSeparator[] = L",";
    const WCHAR sep = L',';

    // Skip leading  whitespaces
    if (buf)
    {
        while (*buf == L' ' || *buf == L'\t')
            buf++;
    }
    if (context && *context)
    {
        while (**context == L' ' || **context == L'\t')
            (*context)++;
    }

    WCHAR *start = buf ? buf : *context;
    WCHAR *result;
    if (start == 0 || *start == L'\0')
    {
        return NULL;
    }

    // consider leading separator as a null arg.
    if (*start == sep )
    {
        *context = ++start;
        return NULL;
    }

#pragma prefast(suppress: 6387, "it's ok that context may be null here")
    result = wcstok_s(buf, wszSeparator, context);
    result = (result && *result == L'\0') ? NULL : result;
    return result;
}
