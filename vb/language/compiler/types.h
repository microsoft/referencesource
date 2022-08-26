//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Common types used by everybody.
//
//-------------------------------------------------------------------------------------------------

#pragma once

// NOTE: A module/project can always move between states even if that
//  : module/project contains errors.  Also, we rely upon the numerical
//  : value of these so we can use < and > to test relative states.

enum CompilationState
{
    CS_NoState,         // We have no compiler-generated state for this item.
    CS_Declared,        // Symbols have been created.
    CS_Bound,           // Symbols have been linked together.
    CS_TypesEmitted,    // Type metadata has been emitted.
    CS_Compiled,        // Method bodies have been compiled, code is on the disk.
    CS_MAX
};

// The list of steps that we must go through in order to compile a file.
// This enum is here for debugging and documentation purposes only.
//
enum CompilationSteps
{
    CS_NoStep,             // CS_NoState
    CS_BuiltSymbols,       // CS_Declared
    CS_BoundSymbols,       // CS_Bound
    CS_CheckCLSCompliance, // CS_TypesEmitted
    CS_EmitTypes,          // CS_TypesEmitted
    CS_EmitTypeMembers,    // CS_TypesEmitted
    CS_GeneratedCode,      // CS_Compiled
    CS_ENCRudeDetection,   // CS_Compiled
    CS_MAXSTEP
};

// Maps the step to the state it is a part of
inline CompilationState MapStepToState (CompilationSteps step)
{
    switch (step)
    {
    case CS_NoStep:
        return CS_NoState;

    case CS_BuiltSymbols:
        return CS_Declared;

    case CS_BoundSymbols:
        return CS_Bound;

    case CS_CheckCLSCompliance:
    case CS_EmitTypes:
    case CS_EmitTypeMembers:
        return CS_TypesEmitted;

    case CS_GeneratedCode:
    case CS_ENCRudeDetection:
        return CS_Compiled;

    default:
        VSFAIL("NYI");
        return CS_NoState;
    }
}


//
// Some names for states we decompile to on specific events.
// We do this because when, say, assembly manifest generation
// moved from CS_Bound to CS_TypesEmitted, it was terribly
// difficult to figure out which of the million CS_Bound's
// needed to be changed to CS_TypesEmitted's.
//
const CompilationState CS_DecompileOnAssemblyManifestChange = CS_Bound;
const CompilationState CS_DecompileOnResourceChange = CS_Bound;


//
// Access types.
//

enum ACCESS // Microsoft: I'm depending on these being ordered from most restrictive to least restrictive for comparative purposes in bindable
            // ACCESS_IntersectionProtectedFriend requires some explanation.  When determining the relative access of a friend member contained
            // by a Protected member, or vice-versa, what you end up with is an access that is not Protected Friend, is more restrictive than
            // either Protected or Friend by itself, yet doesn't have a corresponding access in the language.  But we need a way to represent
            // this state.  See Bindable::AccessExposureSemantics()
{ // !!! If you change this ordering, adjust the AccessTableStrings[] definition in Bindable.cpp to match.
    /// !!! if more are added or ordering changed, adjust the MapAccessToAccessOutsideAssembly array below to match.
    ACCESS_CompilerControlled,
    ACCESS_Private,
    ACCESS_IntersectionProtectedFriend, // There is weirdness when deciding what the accessibility of a friend thing contained in a protected thing (and vice-versa) is.  This is that state which sadly isn't represented in the language but we need to know about it so we can figure accessiblity correctly.
    ACCESS_Protected,
    ACCESS_Friend,
    ACCESS_ProtectedFriend, // Note that ProtectedFriend is a UNION of Protected & Friend
    ACCESS_Public,
    ACCESS_MAX = ACCESS_Public
};

#define MAXACCESSBITS 4

// Make sure the ACCESS enum doesn't go beyond the size we have allocated.
COMPILE_ASSERT(ACCESS_MAX < (1 << MAXACCESSBITS));

const ACCESS MapAccessToAccessOutsideAssembly[] =
{
    ACCESS_Private,
    ACCESS_Private,
    ACCESS_Private,
    ACCESS_Protected,
    ACCESS_Private,
    ACCESS_Protected,
    ACCESS_Public
};


//
// Define the bcsym enumeration.
//

enum BilKind
{
    #define DEF_BCSYM(x,y)    SYM_##x,
    #include "Symbols\TypeTables.h"
    #undef DEF_BCSYM
    SYM_Max
};

enum Vtypes
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  type,
    #include "Symbols\TypeTables.h"
    #undef DEF_TYPE
};

enum typeChars
{
    #define DEF_TYPECHAR(x,y,t)  x,
    #include "Symbols\TypeTables.h"
    #undef DEF_TYPECHAR
};

enum UserDefinedOperators
{
    #define DEF_OPERATOR(op, clsname, token, isunary, isbinary, isconversion)  op,
    #include "Symbols\TypeTables.h"
    #undef DEF_OPERATOR
};

// Events for the IEditSychronization interface.
enum EDITSYNC
{
    SYNC_STATEMENTS,
    SYNC_DECLARATIONS,
    SYNC_READYTOPRODUCE
};

////////////////////////////////////////////////////////////////////////////
// IEditSynchronization
////////////////////////////////////////////////////////////////////////////
// Sync between the library codefile events and user edits.
//
interface _declspec( uuid( "4B0DD5EE-48DF-4132-B01C-B1FF4C371BBF" ) )
IEditSynchronization : IUnknown
{
    STDMETHOD_( void, OnEvent      )( EDITSYNC, IUnknown * ) PURE;
    STDMETHOD_( void, SinkCodeFile )() PURE;
    STDMETHOD_( void, FlagUpdateUI )() PURE;

    STDMETHOD ( SyncToCaretPosition )( long, long ) PURE;
    STDMETHOD ( TryRefresh          )() PURE;
};
#define IID_IEditSynchronization __uuidof( IEditSynchronization )
