/***
*SourceAnnotations.h - Source Annotation definitions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines internal structures used by the Source Code analysis engine.
*
****/

#pragma once

#ifndef _M_CEE_SAFE

#ifndef _SIZE_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64    size_t;
#else  /* _WIN64 */
typedef unsigned int        size_t;
#endif  /* _WIN64 */
#define _SIZE_T_DEFINED
#endif  /* _SIZE_T_DEFINED */

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif  /* _WCHAR_T_DEFINED */


#pragma push_macro( "SA" )
#pragma push_macro( "REPEATABLE" )

#ifdef __cplusplus
#define SA( id ) id
#define REPEATABLE [repeatable]
#else  /* __cplusplus */
#define SA( id ) SA_##id
#define REPEATABLE
#endif  /* __cplusplus */

#ifdef __cplusplus
namespace vc_attributes
{
#endif  /* __cplusplus */

enum SA( YesNoMaybe )
{
        // Choose values that we can detect as invalid if they are or'd together
        SA( No ) = 0x0fff0001,
        SA( Maybe ) = 0x0fff0010,
        SA( Yes ) = 0x0fff0100
};

typedef enum SA( YesNoMaybe ) SA( YesNoMaybe );

enum SA( AccessType )
{
        SA( NoAccess ) = 0,
        SA( Read ) = 1,
        SA( Write ) = 2,
        SA( ReadWrite ) = 3
};

typedef enum SA( AccessType ) SA( AccessType );

#ifndef SAL_NO_ATTRIBUTE_DECLARATIONS

REPEATABLE
[source_annotation_attribute( SA( All ) )]
struct PreAttribute
{
#ifdef __cplusplus
        PreAttribute();
#endif  /* __cplusplus */

        unsigned int Deref;
        SA( YesNoMaybe ) Valid;
        SA( YesNoMaybe ) Null;
        SA( YesNoMaybe ) Tainted;
        SA( AccessType ) Access;
        unsigned int Notref;
        size_t ValidElementsConst;
        size_t ValidBytesConst;
        const wchar_t* ValidElements;
        const wchar_t* ValidBytes;
        const wchar_t* ValidElementsLength;
        const wchar_t* ValidBytesLength;
        size_t WritableElementsConst;
        size_t WritableBytesConst;
        const wchar_t* WritableElements;
        const wchar_t* WritableBytes;
        const wchar_t* WritableElementsLength;
        const wchar_t* WritableBytesLength;
        size_t ElementSizeConst;
        const wchar_t* ElementSize;
        SA( YesNoMaybe ) NullTerminated;
        const wchar_t* Condition;
};

REPEATABLE
[source_annotation_attribute( SA( All ) )]
struct PostAttribute
{
#ifdef __cplusplus
        PostAttribute();
#endif  /* __cplusplus */

        unsigned int Deref;
        SA( YesNoMaybe ) Valid;
        SA( YesNoMaybe ) Null;
        SA( YesNoMaybe ) Tainted;
        SA( AccessType ) Access;
        unsigned int Notref;
        size_t ValidElementsConst;
        size_t ValidBytesConst;
        const wchar_t* ValidElements;
        const wchar_t* ValidBytes;
        const wchar_t* ValidElementsLength;
        const wchar_t* ValidBytesLength;
        size_t WritableElementsConst;
        size_t WritableBytesConst;
        const wchar_t* WritableElements;
        const wchar_t* WritableBytes;
        const wchar_t* WritableElementsLength;
        const wchar_t* WritableBytesLength;
        size_t ElementSizeConst;
        const wchar_t* ElementSize;
        SA( YesNoMaybe ) NullTerminated;
        SA( YesNoMaybe ) MustCheck;
        const wchar_t* Condition;
};

[source_annotation_attribute( SA( All ) )]
struct FormatStringAttribute
{
#ifdef __cplusplus
        FormatStringAttribute();
#endif  /* __cplusplus */

        const wchar_t* Style;
        const wchar_t* UnformattedAlternative;
};

REPEATABLE
[source_annotation_attribute( SA( All ) )]
struct InvalidCheckAttribute
{
#ifdef __cplusplus
        InvalidCheckAttribute();
#endif  /* __cplusplus */

        long Value;
};

[source_annotation_attribute( SA( All ) )]
struct SuccessAttribute
{
#ifdef __cplusplus
        SuccessAttribute();
#endif  /* __cplusplus */

        const wchar_t* Condition;
};

REPEATABLE
[source_annotation_attribute( SA( All ) )]
struct PreBoundAttribute
{
#ifdef __cplusplus
        PreBoundAttribute();
#endif  /* __cplusplus */
        unsigned int Deref;
};

REPEATABLE
[source_annotation_attribute( SA( All ) )]
struct PostBoundAttribute
{
#ifdef __cplusplus
        PostBoundAttribute();
#endif  /* __cplusplus */
        unsigned int Deref;
};

REPEATABLE
[source_annotation_attribute( SA( All ) )]
struct PreRangeAttribute
{
#ifdef __cplusplus
        PreRangeAttribute();
#endif  /* __cplusplus */
        unsigned int Deref;
        const char* MinVal;
        const char* MaxVal;
};

REPEATABLE
[source_annotation_attribute( SA( All ) )]
struct PostRangeAttribute
{
#ifdef __cplusplus
        PostRangeAttribute();
#endif  /* __cplusplus */
        unsigned int Deref;
        const char* MinVal;
        const char* MaxVal;
};

REPEATABLE
[source_annotation_attribute( SA( All ) )]
struct DerefAttribute
{
#ifdef __cplusplus
        DerefAttribute();
#endif  /* __cplusplus */
        int unused;
};

REPEATABLE
[source_annotation_attribute( SA( All ) )]
struct NotrefAttribute
{
#ifdef __cplusplus
        NotrefAttribute();
#endif  /* __cplusplus */
        int unused;
};

REPEATABLE
[source_annotation_attribute( SA( All ) )]
struct AnnotesAttribute
{
#ifdef __cplusplus
        AnnotesAttribute();
#endif  /* __cplusplus */
        wchar_t *Name;
        wchar_t *p1;
        wchar_t *p2;
        wchar_t *p3;
        wchar_t *p4;
        wchar_t *p5;
        wchar_t *p6;
        wchar_t *p7;
        wchar_t *p8;
        wchar_t *p9;
};

[source_annotation_attribute( SA( All ) )]
REPEATABLE
struct AtAttribute
{
#ifdef __cplusplus
        AtAttribute();
#endif  /* __cplusplus */
        wchar_t *p1;
};

[source_annotation_attribute( SA( All ) )]
REPEATABLE
struct AtBufferAttribute
{
#ifdef __cplusplus
        AtBufferAttribute();
#endif  /* __cplusplus */
        wchar_t *p1;
        wchar_t *p2;
        wchar_t *p3;
};

[source_annotation_attribute( SA( All ) )]
REPEATABLE
struct WhenAttribute
{
#ifdef __cplusplus
        WhenAttribute();
#endif  /* __cplusplus */
        wchar_t *p1;
};

[source_annotation_attribute( SA( All ) )]
REPEATABLE
struct TypefixAttribute
{
#ifdef __cplusplus
        TypefixAttribute();
#endif  /* __cplusplus */
        wchar_t *p1;
};

[source_annotation_attribute( SA( All ) )]
REPEATABLE
struct ContextAttribute
{
#ifdef __cplusplus
        ContextAttribute();
#endif  /* __cplusplus */
        wchar_t *p1;
};

[source_annotation_attribute( SA( All ) )]
REPEATABLE
struct ExceptAttribute
{
#ifdef __cplusplus
        ExceptAttribute();
#endif  /* __cplusplus */
        int unused;
};

[source_annotation_attribute( SA( All ) )]
REPEATABLE
struct PreOpAttribute
{
#ifdef __cplusplus
        PreOpAttribute();
#endif  /* __cplusplus */
        int unused;
};

[source_annotation_attribute( SA( All ) )]
REPEATABLE
struct PostOpAttribute
{
#ifdef __cplusplus
        PostOpAttribute();
#endif  /* __cplusplus */
        int unused;
};

[source_annotation_attribute( SA( All ) )]
REPEATABLE
struct BeginAttribute
{
#ifdef __cplusplus
        BeginAttribute();
#endif  /* __cplusplus */
        int unused;
};

[source_annotation_attribute( SA( All ) )]
REPEATABLE
struct EndAttribute
{
#ifdef __cplusplus
        EndAttribute();
#endif  /* __cplusplus */
        int unused;
};

#endif  /* SAL_NO_ATTRIBUTE_DECLARATIONS */

#ifdef __cplusplus
};  // namespace vc_attributes
#endif  /* __cplusplus */

#pragma pop_macro( "REPEATABLE" )
#pragma pop_macro( "SA" )

#ifdef __cplusplus

#define SA_All All
#define SA_Class Class
#define SA_Constructor Constructor
#define SA_Delegate Delegate
#define SA_Enum Enum
#define SA_Event Event
#define SA_Field Field
#define SA_GenericParameter GenericParameter
#define SA_Interface Interface
#define SA_Method Method
#define SA_Module Module
#define SA_Parameter Parameter
#define SA_Property Property
#define SA_ReturnValue ReturnValue
#define SA_Struct Struct
#define SA_Typedef Typedef

typedef ::vc_attributes::YesNoMaybe SA_YesNoMaybe;
const ::vc_attributes::YesNoMaybe SA_Yes   = ::vc_attributes::Yes;
const ::vc_attributes::YesNoMaybe SA_No    = ::vc_attributes::No;
const ::vc_attributes::YesNoMaybe SA_Maybe = ::vc_attributes::Maybe;

typedef ::vc_attributes::AccessType SA_AccessType;
const ::vc_attributes::AccessType SA_NoAccess  = ::vc_attributes::NoAccess;
const ::vc_attributes::AccessType SA_Read      = ::vc_attributes::Read;
const ::vc_attributes::AccessType SA_Write     = ::vc_attributes::Write;
const ::vc_attributes::AccessType SA_ReadWrite = ::vc_attributes::ReadWrite;

#ifndef SAL_NO_ATTRIBUTE_DECLARATIONS
typedef ::vc_attributes::PreAttribute          SA_Pre;
typedef ::vc_attributes::PostAttribute         SA_Post;
typedef ::vc_attributes::FormatStringAttribute SA_FormatString;
typedef ::vc_attributes::InvalidCheckAttribute SA_InvalidCheck; /*???*/
typedef ::vc_attributes::SuccessAttribute      SA_Success;
typedef ::vc_attributes::PreBoundAttribute     SA_PreBound;
typedef ::vc_attributes::PostBoundAttribute    SA_PostBound;
typedef ::vc_attributes::PreRangeAttribute     SA_PreRange;
typedef ::vc_attributes::PostRangeAttribute    SA_PostRange;

typedef ::vc_attributes::DerefAttribute        SAL_deref;
typedef ::vc_attributes::NotrefAttribute       SAL_notref;
typedef ::vc_attributes::PreOpAttribute        SAL_pre;
typedef ::vc_attributes::PostOpAttribute       SAL_post;
typedef ::vc_attributes::ExceptAttribute       SAL_except;

typedef ::vc_attributes::AtAttribute           SAL_at;
typedef ::vc_attributes::AtBufferAttribute     SAL_at_buffer;
typedef ::vc_attributes::WhenAttribute         SAL_when;
typedef ::vc_attributes::BeginAttribute        SAL_begin;
typedef ::vc_attributes::EndAttribute          SAL_end;
typedef ::vc_attributes::TypefixAttribute      SAL_typefix;
typedef ::vc_attributes::AnnotesAttribute      SAL_annotes;
typedef ::vc_attributes::ContextAttribute      SAL_context;

#endif  /* SAL_NO_ATTRIBUTE_DECLARATIONS */

#else  /* __cplusplus */

typedef struct PreAttribute          SA_Pre;
typedef struct PreAttribute          PreAttribute;
typedef struct PostAttribute         SA_Post;
typedef struct PostAttribute         PostAttribute;
typedef struct FormatStringAttribute SA_FormatString;
typedef struct InvalidCheckAttribute SA_InvalidCheck; /*???*/
typedef struct SuccessAttribute      SA_Success;
typedef struct PreBoundAttribute     SA_PreBound;
typedef struct PostBoundAttribute    SA_PostBound;
typedef struct PreRangeAttribute     SA_PreRange;
typedef struct PostRangeAttribute    SA_PostRange;

typedef struct DerefAttribute        SAL_deref;
typedef struct NotrefAttribute       SAL_notref;
typedef struct PreOpAttribute        SAL_pre;
typedef struct PostOpAttribute       SAL_post;
typedef struct ExceptAttribute       SAL_except;

typedef struct AtAttribute           SAL_at;
typedef struct AtBufferAttribute     SAL_at_buffer;
typedef struct WhenAttribute         SAL_when;
typedef struct BeginAttribute        SAL_begin;
typedef struct EndAttribute          SAL_end;
typedef struct TypefixAttribute      SAL_typefix;
typedef struct AnnotesAttribute      SAL_annotes;
typedef struct ContextAttribute      SAL_context;

#endif  /* __cplusplus */

#endif  /* _M_CEE_SAFE */

#ifdef _MANAGED

#ifdef CODE_ANALYSIS
#define SA_SUPPRESS_MESSAGE( category, id, ... ) [::System::Diagnostics::CodeAnalysis::SuppressMessage( category, id, __VA_ARGS__ )]
#define CA_SUPPRESS_MESSAGE( ... ) [System::Diagnostics::CodeAnalysis::SuppressMessage( __VA_ARGS__ )]
#define CA_GLOBAL_SUPPRESS_MESSAGE( ... ) [assembly:System::Diagnostics::CodeAnalysis::SuppressMessage( __VA_ARGS__ )]
#else  /* CODE_ANALYSIS */
#define SA_SUPPRESS_MESSAGE( category, id, ... )
#define CA_SUPPRESS_MESSAGE( ... )
#define CA_GLOBAL_SUPPRESS_MESSAGE( ... )
#endif  /* CODE_ANALYSIS */

#endif  /* _MANAGED */
