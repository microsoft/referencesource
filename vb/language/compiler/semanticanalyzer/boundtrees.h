//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Definition of BILTREE (stmtExpr) data structure.
//  Trees are produced by the Parser, read by Decls, read and manipulated by Exmgr and Codegen.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#pragma warning(push)
#pragma warning(disable: 4200)

typedef unsigned __int64 ExpressionFlags;

class BCSYM;
class BCSYM_Variable;
class BCSYM_Property;
class TemporaryManager;
class SourceFile;
class BCSYM_MethodImpl;
class BCSYM_Param;
class BCSYM_NamedType;
class BCSYM_Proc;
class BCSYM_NamedRoot;
class BCSYM_Class;
class BCSYM_Hash;
class BITSET;
class BCSYM_GenericBinding;
struct BlockScope;
struct CODE_BLOCK;
struct SWITCH_TABLE;
class BCSYM_ExtensionCallLookupResult;
class ErrorTable;

namespace ILTree
{
	class  ILNode;
	typedef ILNode      * PILNode;
	struct LabelStatement;
}

namespace ParseTree
{
    struct Expression;
}


enum BILOP
{
#define SX_NL(en,ok,st)  en,
#define SX_NL_EX(en,ok,st, st2)  en,
#include "BoundTreeTable.h"
#undef  SX_NL
#undef SX_NL_EX

    SX_COUNT
};

/*****************************************************************************
 *
 *  The following enum defines a set of bit flags that can be used
 *  to classify expression tree nodes. Note that some operators will
 *  have more than one bit set, as follows:
 *
 *          SXK_RELOP    implies    SXK_BINOP
 *          SXK_LOGOP    implies    SXK_BINOP
 */

enum SXKIND
{
    SXK_NONE    = 0x0000,           // unclassified operator

    SXK_CONST   = 0x0001,           // constant     operator
    SXK_LEAF    = 0x0002,           // leaf         operator
    SXK_UNOP    = 0x0004,           // unary        operator
    SXK_BINOP   = SXK_UNOP,         // binary    -- BINOP & UNOP not differentiated
    SXK_RELOP   = 0x0008,           // comparison   operator
    SXK_LOGOP   = 0x0010,           // logical      operator
    SXK_SPECIAL = 0x0040,           // has special fields
    SXK_STMT    = 0x0080,           // statement level (SL) node
    SXK_EXECUTABLE_BLOCK   = 0x0100|SXK_STMT,  // block construct (if, while, type, enum,)

    /* Define composite value(s) */

    //  SXK_SMPOP => children can be accessed via AsExpressionWithChildren
    SXK_SMPOP   = (SXK_UNOP|SXK_BINOP|SXK_RELOP|SXK_LOGOP)
};

struct DefAsgReEvalStmt : CSingleLink<DefAsgReEvalStmt>
{
    ILTree::PILNode   m_ReEvalStmt;
    unsigned int      ClosureDepth;
};


class DefAsgReEvalStmtList : public CSingleList<DefAsgReEvalStmt>
{
public:
    void AddReEvalStmt(ILTree::PILNode stmt, unsigned int ClosureDepth, NorlsAllocator  *alloc);
    void RemoveReEvalStmt(ILTree::PILNode stmt);
};

struct TrackingVariablesForImplicitCollision :
    CSingleLink<TrackingVariablesForImplicitCollision>
{
    TrackingVariablesForImplicitCollision
    (
        _In_ STRING*     name,
        Location    location,
        ERRID       errorId
    ) :
        Name(name),
        Location(location),
        ErrorId(errorId)
    {

    }

    STRING*     Name;
    Location    Location;
    ERRID       ErrorId;
};

class TrackingVariablesForImplicitCollisions :
    public CSingleList<TrackingVariablesForImplicitCollision>
{
};

//*****************************************************************************
// Label struct for Goto's and Labels
// Represents state information for a Label.
//*****************************************************************************
struct LABEL_ENTRY_BIL
{
    STRING          * Name;                           // label name
    bool              IsDefined:1;                    // Label has been previously defined.
    bool              IsLineNumber:1;                 // Label is a line number, not a text label.
    bool              IsBranchTargetFromEHContext:1;  // Label is the target of a Goto which exits a Try/Catch.

    void            * DefiningStatement;  // Parse tree for definition -- valid only during semantic analysis.
    CODE_BLOCK      * TargetCodeBlock;    // codeblock which this label begins.

    ILTree::LabelStatement * pLabelParent;  // Back pointer to the bill tree node that defines this label. Used for definite assignment.
};


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// Here are all the tree flags, separated by whether they are used by the
// new trees, the old trees only, or both. Try to keep this distinction.
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


// ***************************************************************************
// Both trees (BILTREE and OLDTREE)
// ***************************************************************************


  //---------------------------------------------------------------------
  //  The first set of flags can be used with a non-trivial set
  //  of nodes, and thus their values need to be unique. That
  //  is, one can take any expression node and test for one of
  //  these flags.
  //
  //  !!! NOTE !!!
  //  Look at SXF_COMMON to see which bits are considered common.
  //
  //  If we need more common bits, we can merge together the common bits used
  //  only in the frontend with the common bits only used in backend.
  //---------------------------------------------------------------------


  //****************************************************************
  //*  The following flags can be seen on ANY node and needs to be
  //*  propagated up the tree.
  //****************************************************************
  #define SF_ERROR            0x0001      // subtree contains an error
  #define SF_INHERIT(flags)   ((flags) & 0x0001)


  //****************************************************************
  //*  The following flags can be seen on any SB node
  //****************************************************************
  #define SBF_MATCHED         0x0002      // end block construct matched


  //****************************************************************
  //*  The following flags can be seen on any SB/SL node
  //****************************************************************
  #define SLF_FIRST_ON_LINE   0x0004      // statement is first on a logical line

#if DEBUG
  #define SLF_NEED_OBJREL     0x0008      // SL nodes ONLY: stmt needs object release (DEBUG ONLY)
#endif

  #define SLF_INIT_ME         0x0010      // SL nodes ONLY: indicates that this statement is the
                                          // MyBase or MyClass constructor calls or the SX_INIT-
                                          // STRUCTURE statement in instance constructors.
                                          //
                                          // There should one and only one such statement in any
                                          // in a instance constructor and there should be no such
                                          // statement in any procedures.
                                          //
                                          // This is used to delay on error try block init in
                                          // codegen till after this statement.
                                          // Bug VSWhidbey 204996

//****************************************************************
  //*  The following flags can be seen on any SX node
  //****************************************************************
  #define SXF_LVALUE          0x0004
  #define SXF_PAREN_EXPR      0x0008      // parens around (expr)
  #define SXF_CONSTRAINEDCALL_BASEREF 0x0010    // parens
  #define SXF_USERDEFINEDOPERATOR 0x0020

  // Macros to get common and node specific bits
  #define SXF_COMMON(flags)      ((flags) & 0x00FC)
  #define SXF_SPECIFIC(flags)    ((flags) & 0xFF00)

  // Macro maps enumerator (SXE_xxx) to the specific area
  // (Note: there is a macro for each enumeration within
  //        the flags for a specific node)
  #define SXF_ENUMtoFLAG(enu) ((enu) << 8)


  //---------------------------------------------------------------------
  //  The remaining flags can be used only with one node,
  //  and thus their values need not be distinct (other
  //  than within the set that goes with a particular
  //  node, of course). That is, testing one of these
  //  flags only makes sense if the node kind is tested
  //  as well to make sure it's the right one for the
  //  particular flag.
  //
  //  SXF   - on SX_ nodes
  //  SLF   - on SL_ nodes
  //  SBF   - on SB_ nodes
  //---------------------------------------------------------------------


  //****************************************************************
  // Flags for specific to SX_ nodes
  //****************************************************************

  #define SXE_LATE_GET        0x0000      // used with SX_LATE       nodes
  #define SXE_LATE_SET        0x0002      // used with SX_LATE       nodes
  #define SXE_LATE_CALL       0x0004      // used with SX_LATE       nodes
  #define SXF_LATE_ENUM(flags) (((flags) & 0x0700) >> 8)

  #define SXF_CALL_RAISEEVENT 0x8000      // used with SX_CALL       nodes  // This is a RaiseEvent statement
  #define SXF_CALL_WAS_EXTENSION_CALL 0x4000 // used with SX_CALL and SX_DELEGATE_CTOR_CALL nodes. Indicates that the call statement was an extension call before overload resolution was performed. This signals the UI to not display the first parameter inside tool tips.
  #define SXF_CALL_WAS_OPERATOR 0x2000      // used with SX_CALL nodes and expression trees. Indicates that this call is the result of a user defined operator being substituted in InterpretUserDefinedOperator.
  #define SXF_CALL_WAS_IMPLICIT_INVOKE 0x1000 // used with SX_CALL nodes and expression trees. Indicates that this call is the result of resolving a delegate call to it's invoke method.
  #define SXF_CALL_WAS_QUERY_OPERATOR 0x0800 // used with SX_CALL nodes. Indicates that this call is the result of interpreting a query operator.

  #define SXF_SYM_MYBASE        0x8000      // used with SX_SYM        nodes
  #define SXF_SYM_MYCLASS       0x4000      // used with SX_SYM        nodes
  #define SXF_SYM_NONVIRT       0x2000      // Refer to a procedure non-virtually
  #define SXF_SYM_MAKENOBASE    0x1000      // Suppress synthesis of a base reference for a non-shared member
  #define SXF_SYM_PASSEDBYREF   0x0800      // Symbol passed byref direcly or via conversion, copy back etc. Used for definite assignment
  #define SXF_SYM_ASGVIACALL    0x0400      // Symbol being assigned via a call. used by definite assignemt to make up for assignments morphed
                                            // into calls. Like 'dim a as new struct(b)' -> call struct ctor(a,b)


  #define SXE_VARINDEX_GET    0x0000      // used with SX_VARINDEX   nodes
  #define SXE_VARINDEX_SET    0x0002      // used with SX_VARINDEX   nodes
  #define SXF_VARINDEX_ENUM(flags) (((flags) & 0x0700) >> 8)

  #define SXF_EXTENSION_CALL_ME_IS_SYNTHETIC 0x8000 //used with SX_EXTENSION_CALL nodes to indicate that the me argument to an extension call is synthetic.

  // The base reference of the late reference is an RValue. This enables generating
  // a run-time exception for late assignments to fields of RValues of value types.
  #define SXF_LATE_RVALUE_BASE  0x8000      // used with SX_LATE and SX_VARINDEX nodes
  // Don't generate an exception at runtime if the operation is impossible.
  // Used for passing late-bound property references ByBef.
  #define SXF_LATE_OPTIMISTIC   0x4000      // used with SX_LATE and SX_VARINDEX nodes

  #define SXF_RELOP_TEXT        0x8000      // used with RELOPS

  // The following flag only used for SX_ARG     nodes.
  #define SXF_ARG_NAMED         0x1000      // Named Arg

  // These flags are for SX_CTYPE and are only used for XML generation
  #define SXF_COERCE_EXPLICIT   0x8000      // Explicit conversion

  // These flags are for SX_ASG and SX_ASG_RESADR
  #define SXF_ASG_SUPPRESS_CLONE 0x1000     // Don't clone the object on assignment
  #define SXF_ASG_RESADR_READONLYVALUE 0x2000  // This is used to mark temporary expressions involving
                                               // Read only variables so that a better error can be given
                                               // if an attempt to assign to them is made.

  // These flags are for SX_LIST
  #define SXF_LIST_SUPPRESS_CLONE 0x1000    // Don't clone the object when passing as parameter

  // These flags are for SX_SEQ_OP2
  #define SXF_SEQ_OP2_XML                   0x1000  // This SX_SEQ_OP2 is the root of an Xml literal
  #define SXF_SEQ_OP2_RELAXED_LAMBDA_EXT    0x2000  // This lambda is wrapped in a seq_op2 which left side
                                                    // has an assignment to a temp. The right side contains a Lambda with a static
                                                    // method call to an extension function. whose first argument is the temp. The call
                                                    // node may be not the top node of the Lambda's body, it may have some conversion operations
                                                    // applied to it. And, in case of conversions between delegate types, there will be a delegate creation 
                                                    // node above the call node.
                                                    // These are only generated by relaxed delegates. Since there we can guarantee proper semantics.
  #define SXF_SEQ_OP2_RELAXED_LAMBDA_INST   0x4000  // Same as SXF_SEQ_OP2_RELAXED_LAMBDA_EXT but now for instance methods
                                                    //.and not first arg is temp but the operand.

  // These flags are for SX_ADDRESSOF and are only used for semantic analysis
  #define SXF_DISALLOW_ME_REFERENCE 0x1000  // The instance member can't be referenced through Me
  #define SXF_TARGET_METHOD_RESOLVED 0x2000 // Indicates that the method expression doesn't need to be
                                            // resolved via overload resolution. This is for the WithEvents
                                            // Handles case where the target method is a specific instance, not
                                            // a named reference from which to pick the best.
  #define SXF_USE_STRICT_OF_TARGET_METHOD 0x4000    // Signals AddressOf interpretation to use the location of the targetmethod rather than
                                                    // the location of the addressof for option strict. This is for the Handles case.
  #define SXF_USED_IN_REMOVEHANDLER 0x8000          // We want to report a warning that this one cant be removed because it is relaxed.
                                                    // can't know this yet when interpreting the removehandler

  #define SXF_OP_LIFTED_NULLABLE 0x1000 // Indicates lifted unary or binary nullable operation.

  // These flags are for SX_AWAIT only
  #define SXF_EXITS_TRY 0x1000          // This Await will exit out of a Try block

  #define SXF_STMT_ENSURE_NO_ITEMS_ADDED_TO_STACK_BY_POPPING 0x1000 // This statement will ensure that no matter what expression it encapsulates, it will keep the stack empty by generating a POP if needed.

  //****************************************************************
  // Flags for specific to SL_ nodes
  //****************************************************************

  // Flags on SL_OPTION
  #define SLF_OPTION_BASE           0x1000      // Option Base family
  #define SLF_OPTION_BASE_0         0x1100      // Option Base 0
  #define SLF_OPTION_BASE_1         0x1200      // Option Base 1
  #define SLF_OPTION_BASE_INVALID   0x1400      // Option Base <invalid>
  #define SLF_OPTION_IsBase(x)      (x & SLF_OPTION_BASE)

  #define SLF_OPTION_CMP            0x2000      // Option Compare family
  #define SLF_OPTION_CMP_TEXT       0x2100      // Option Compare Text
  #define SLF_OPTION_CMP_BINARY     0x2200      // Option Compare Binary
  #define SLF_OPTION_CMP_DATABASE   0x2400      // Option Compare Database
  #define SLF_OPTION_CMP_INVALID    0x2800      // Option Compare <invalid>
  #define SLF_OPTION_IsCompare(x)   (x & SLF_OPTION_CMP)

  #define SLF_OPTION_EXPLICIT       0x3000      // Option Explicit
  #define SLF_OPTION_PRIVATE        0x4000      // Option Private


  // Enum to select which Get/Put
  #define SLE_GETPUT_3          0x0000      // used with SX_GET/SX_PUT nodes
  #define SLE_GETPUT_4          0x0001      // used with SX_GET/SX_PUT nodes
  #define SLE_GETPUT_OWN3       0x0002      // used with SX_GET/SX_PUT nodes
  #define SLE_GETPUT_OWN4       0x0003      // used with SX_GET/SX_PUT nodes
  #define SLE_GETPUT_FIX3       0x0004      // used with SX_GET/SX_PUT nodes
  #define SLE_GETPUT_FIX4       0x0005      // used with SX_GET/SX_PUT nodes
  #define SLF_GETPUT_ENUM(flags) (((flags) & 0x0700) >> 8)

  #define MapGetPutFlags3to4(x) (x + 0x0100)   // Map 3 version to 4 version

  // Flags on SL_ONERR
  #define SLF_ONERR_NEXT              0x8000    // ON_ERR, RESUME
  #define SLF_ONERR_ZERO              0x4000    // ON_ERR, RESUME
  #define SLF_ONERR_MINUS1            0x1000    // ON_ERR, RESUME

  // Flags on SL_DECLARE
  #define SLF_DECLARE_SUB             0x1000    // Declare Sub
  #define SLF_DECLARE_FUNCTION        0x2000    // Declare Function

  #define SLE_DECLARE_UNICODE         0x0001    // Unicode
  #define SLE_DECLARE_ANSI            0x0002    // Ansi
  #define SLE_DECLARE_AUTO            0x0003    // Auto
  #define SLF_DECLARE_ENUM(flags) (((flags) & 0x0700) >> 8)

  // Flags on SL_EXIT, SL_RETURN, SL_GOTO, SL_RESUME, SL_YIELD

  #define SLF_EXITS_TRY               0x1000    // Branch exits try statement.
  #define SLF_EXITS_CATCH             0x2000
  #define SLF_RET_FROM_EXIT           0x4000
  #define SLF_EXITS_CATCH_TO_CORRESPONDING_TRY 0x8000   // Branch exits from catch statement to its associated try statement.

  // Flags on SL_REDIM.

  #define SLF_REDIM_PRESERVE          0x8000

  //****************************************************************
  // Flags for specific to SB_ nodes
  //****************************************************************

  // different type of do..loops
  #define SBF_DO_LOOP                 0x8000
  #define SBF_WHILE                   0x4000
  #define SBF_DO_WHILE                0x2000
  #define SBF_LOOP_WHILE              0x0800

  #define SBF_SELECT_HAS_CASE_ELSE    0x2000    // SL_SELECT as tableswitch
  #define SBF_SELECT_TABLE            0x1000    // SL_SELECT as tableswitch

  #define SBF_CASE_CONDITION          0x2000
  #define SBF_CASE_ISELSE             0x1000    // CASE

  #define SBF_IF_BLOCK_NO_XML         0x8000    // If block that is hidden from XML generation.
  #define SBF_LINE_IF                 0x4000

  #define SBF_FOR_CNS_LIMIT           0x8000    // SL_FOR - constant limit
  #define SBF_FOR_CNS_STEP            0x4000    // SL_FOR - constant step
  #define SBF_FOR_POSITIVE_STEP       0x2000    // SL_FOR - implies SLF_FOR_CNS_STEP
  #define SBF_FOR_NEGATIVE_STEP       0x1000    // SL_FOR - implies SLF_FOR_CNS_STEP

  // Flags on SB_WITH
  #define SBF_WITH_RECORD             0x1000
  #define SBF_WITH_LVALUE             0x2000

  // Flags on SB_TRY
  #define SBF_TRY_NOT_SYNTHETIC       0x1000    // Try block came from user code, not generated synthetically by Semantics.
  #define SBF_OVERLAPED_CATCH         0x2000    // Catch block overlaped or duplicated came from user code, not generated synthetically by Semantics.

  // Flags on SB_CATCH
  #define SBF_CATCH_ASYNCSUB          0x1000    // Async Sub's MoveNext method's generated catch block, should have IL start offset emitted to PDB

  /****************************************************************/
  /*  The following flags only used for SX_ICON nodes.            */
  /****************************************************************/

  #define SXF_CON_CONTAINS_NAMED_CONTANTS  0x1000  //  There were named constants encountered during folding of this constant.
  #define SXF_ICON_LITERAL      0x2000     //  Int Const is a literal
  #define SXF_ICON_OCT          0x4000     //  Int Const is Octal
  #define SXF_ICON_HEX          0x8000     //  Int Const is Hex


  /***************************************************************************/

  enum EXITKIND
  {
      EXIT_UNKNOWN,
      EXIT_DO,
      EXIT_FOR,
      EXIT_SUB,
      EXIT_FUNCTION,
      EXIT_PROPERTY,
      EXIT_TRY,
      EXIT_SELECT,
      EXIT_SUB_LAMBDA,
      EXIT_FUNCTION_LAMBDA
  };

namespace ILTree
{

    // Forward declarations
    struct Expression;
    struct Statement;
    struct ExecutableBlock;
    struct ProcedureBlock;
    struct StatementLambdaBlock;
    struct IfSelectBlock;
    struct IfGroup;
    struct SelectBlock;
    struct IfCaseBlock;
    struct IfBlock;
    struct CaseBlock;
    struct TryBlock;
    struct CatchBlock;
    struct FinallyBlock;
    struct GeneralLoopBlock;
    struct ForBlock;
    struct ForEachBlock;
    struct LoopBlock;
    struct WithBlock;
    struct StatementWithExpression;
    struct ExitStatement;
    struct ReturnStatement;
    struct YieldStatement;
    struct ContinueStatement;
    struct LabelStatement;
    struct GotoStatement;
    struct AsyncSwitchStatement;
    struct OnErrorStatement;
    struct RedimStatement;
    struct ExpressionWithChildren;
    struct BinaryExpression;
    struct AttributeApplicationExpression;
    struct ArgumentNameExpression;
    struct ArgumentExpression;
    struct IntegralConstantExpression;
    struct DecimalConstantExpression;
    struct FloatConstantExpression;
    struct SymbolReferenceExpression;
    struct DebugObjectExpression;
    struct IfExpression;
    struct CoalesceExpression;
    struct UserDefinedBinaryOperatorExpression;
    struct UserDefinedUnaryOperatorExpression;
    struct ShortCircuitBooleanOperatorExpression;
    struct LiftedCTypeExpression;
    struct CallExpression;
    struct DelegateConstructorCallExpression;
    struct PropertyReferenceExpression;
    struct LateBoundExpression;
    struct VariantIndexExpression;
    struct IndexExpression;
    struct NewExpression;
    struct InitStructureExpression;
    struct StringConstantExpression;
    struct OverloadedGenericExpression;
    struct ExtensionCallExpression;
    struct LambdaExpression;
    struct UnboundLambdaExpression;
    struct DeferredTempExpression;
    struct AnonymousTypeExpression;
    struct ArrayLiteralExpression;
	
	class ILNode
	{
	public:

	  // *************************************************************************
	  // common to all nodes
	  // *************************************************************************

#if DEBUG
	  BILOP           bilop;                    // operator
#else
	  BILOP           bilop:16;                 // operator
#endif // DEBUG

	  unsigned        uFlags:16;                // see SXF_xxxx below

	  Location        Loc;                      // location of the tree

#if DEBUG
	    virtual BILOP AddBILTREEVTableForDebug() { // so debugger can show correct class in watch window
	        return bilop;   // some code so debugger can differentiate between this bcsym and biltree vtables
	    }
	    const char  * pchWhichStruct;           // which struct to use for this op

	    bool IsScratch;                         // Used to identify nodes which are created as scratch
	                                            // nodes during copy.  Should not end up in the final
	                                            // biltree
#endif // DEBUG
	public:

	  static const unsigned short s_sxkindTable[SX_COUNT];

	    // Note: the initializer (in biltree.cpp) for this array requires a mapping
	    //       to the particular union, above.  For example:
	    //          _ExpressionCast ==> _SX_Nodes._ExpressionCast
	  static const USHORT  s_uBiltreeAllocationSize[SX_COUNT];
	  static const USHORT  s_uUserDefinedOperatorBiltreeAllocationSize[SX_COUNT];

	  static SXKIND Sxkind(BILOP bilop)
	  {
	      if (bilop >= 0 && bilop < _countof(ILNode::s_sxkindTable))
	      {
	          return (SXKIND) ILNode::s_sxkindTable[bilop];
	      }

	      return SXK_NONE;
	  }

	  bool  IsStmtNode()
	  {
	      // block nodes are statements too.
	      return (Sxkind(bilop) & SXK_STMT) != 0;
	  }

	  bool  IsExecutableBlockNode()
	  {
	      return (Sxkind(bilop) & (SXK_EXECUTABLE_BLOCK & ~SXK_STMT)) != 0;
	  }

	  bool  IsExprNode()
	  {
	      return (Sxkind(bilop) & SXK_STMT) == 0;
	  }

	  bool IsUserDefinedOperator()
	  {
	      return IsExprNode() && ( ( uFlags & SXF_USERDEFINEDOPERATOR ) != 0 );
	  }

#if DEBUG
	    //---------------------------------------------------------------------
	    // Operator Name table
	    //---------------------------------------------------------------------
	    static const char * s_szBilopNames[SX_COUNT];

	    //---------------------------------------------------------------------
	    // Which sub-struct table
	    //---------------------------------------------------------------------
	    static const char * s_szBiltreeWhichSubStruct[SX_COUNT];

	    bool _DerivesFromStmtOp2_();

#endif

	    SXKIND Sxkind();

	    void PropagateLocFrom(ILNode *pILNode);

	    //accessors for ILTrees. Will work like BCSYM

        struct Expression & AsExpression();
        struct Statement & AsStatement();
        struct ExecutableBlock & AsExecutableBlock();
        struct ProcedureBlock & AsProcedureBlock();
        struct StatementLambdaBlock & AsStatementLambdaBlock();
        struct IfSelectBlock & AsIfSelectBlock();
        struct IfGroup & AsIfGroup();
        struct SelectBlock & AsSelectBlock();
        struct IfCaseBlock & AsIfCaseBlock();
        struct IfBlock & AsIfBlock();
        struct CaseBlock & AsCaseBlock();
        struct TryBlock & AsTryBlock();
        struct CatchBlock & AsCatchBlock();
        struct FinallyBlock & AsFinallyBlock();
        struct GeneralLoopBlock & AsGeneralLoopBlock();
        struct ForBlock & AsForBlock();
        struct ForEachBlock & AsForEachBlock();
        struct LoopBlock & AsLoopBlock();
        struct WithBlock & AsWithBlock();
        struct StatementWithExpression & AsStatementWithExpression();
        struct ExitStatement & AsExitStatement();
        struct ReturnStatement & AsReturnStatement();
        struct YieldStatement & AsYieldStatement();
        struct ContinueStatement & AsContinueStatement();
        struct LabelStatement & AsLabelStatement();
        struct GotoStatement & AsGotoStatement();
        struct AsyncSwitchStatement & AsAsyncSwitchStatement();
        struct OnErrorStatement & AsOnErrorStatement();
        struct RedimStatement & AsRedimStatement();
        struct ExpressionWithChildren & AsExpressionWithChildren();
        struct BinaryExpression & AsBinaryExpression();
        struct AttributeApplicationExpression & AsAttributeApplicationExpression();
        struct ArgumentNameExpression & AsArgumentNameExpression();
        struct ArgumentExpression & AsArgumentExpression();
        struct IntegralConstantExpression & AsIntegralConstantExpression();
        struct DecimalConstantExpression & AsDecimalConstantExpression();
        struct FloatConstantExpression & AsFloatConstantExpression();
        struct SymbolReferenceExpression & AsSymbolReferenceExpression();
        struct DebugObjectExpression & AsDebugObjectExpression();
        struct IfExpression & AsIfExpression();
        struct CoalesceExpression & AsCoalesceExpression();
        struct AwaitExpression & AsAwaitExpression();
        struct AsyncSpillExpression & AsAsyncSpillExpression();
        struct UserDefinedBinaryOperatorExpression & AsUserDefinedBinaryOperatorExpression();
        struct UserDefinedUnaryOperatorExpression & AsUserDefinedUnaryOperatorExpression();
        struct ShortCircuitBooleanOperatorExpression & AsShortCircuitBooleanOperatorExpression();
        struct LiftedCTypeExpression & AsLiftedCTypeExpression();
        struct CallExpression & AsCallExpression();
        struct DelegateConstructorCallExpression & AsDelegateConstructorCallExpression();
        struct PropertyReferenceExpression & AsPropertyReferenceExpression();
        struct LateBoundExpression & AsLateBoundExpression();
        struct VariantIndexExpression & AsVariantIndexExpression();
        struct IndexExpression & AsIndexExpression();
        struct NewExpression & AsNewExpression();
        struct InitStructureExpression & AsInitStructureExpression();
        struct StringConstantExpression & AsStringConstant();
        struct OverloadedGenericExpression & AsOverloadedGenericExpression();
        struct ExtensionCallExpression & AsExtensionCallExpression();
        struct LambdaExpression & AsLambdaExpression();
        struct UnboundLambdaExpression & AsUnboundLambdaExpression();
        struct DeferredTempExpression & AsDeferredTempExpression();
        struct AnonymousTypeExpression & AsAnonymousTypeExpression();
        struct ArrayLiteralExpression & AsArrayLiteralExpression();
        struct NestedArrayLiteralExpression & AsNestedArrayLiteralExpression();
        struct ColInitExpression & AsColInitExpression();
        struct ColInitElementExpression & AsColInitElementExpression();
	}; // ILTree


    // ResumableInfo: for proces/lambdas that have Iterator/Async modifiers, this is where we store the information.
    // It is populated at the start of interpreting async/iterator method (or anonymous method)
    enum ResumableKind
    {
        UnknownResumable = 0,  // Resumability hasn't yet been ascertained for this method.
        TaskResumable = 1,     // This method uses the "new sm/AsyncTaskMethodBuilder/SetResult/SetException/return Task" pattern (for nonvoid-returning asyncs)
        SubResumable = 2,      // This method uses the "new sm/AsyncVoidMethodBuilder/SetResult/SetException/sm.MoveNext" pattern (for void-returning asyncs)
        IteratorResumable = 3, // This method uses the "new sm/return sm" pattern (for iterators)
        IterableResumable = 4, // This method uses the "new sm/return sm" pattern (for iterables)
        NotResumable = 5,      // It's not a resumable!
        ErrorResumable = 6     // An error has already been reported for this method's inconsistent parameters; don't complain further about Return/Yield/Await
    };


    // use ExpressionCast() accessor.
    struct Expression : ILNode
    {
        /* unsigned char */ Vtypes    vtype;

        // Used to store argument indexes for arguments to
        // late bound calls
        //
        unsigned short LateBoundCallArgumentIndex;

        // When bilop is SX_Nothing and its result type is
        // Object, this is useful to determine if the literal
        // Nothing was typed by itself or if there was an
        // explicit CObj or CType(Nothing, Object) around it.
        //
        // So this is set for every explicitly cast expression.
        //
        bool IsExplicitlyCast : 1;
        bool NameCanBeType : 1;     // aplies to simple names that can be bound both to a member and a type
        bool ForcedLiftedCatenationIIFCoalesce : 1;

        // Indicates that one of the descendants of this expression is an
        // SX_ASYNCSPILL. This is used to identify expressions that need to be
        // dehydrated/rehydrated in order to avoid spilling managed pointers.
        bool ContainsAsyncSpill : 1;

        BCSYM           * ResultType; // full type (bound IL only)

        Expression* RewrittenByStateMachine; // If this expression has been rewritten, this is what it was rewritten to

	};

	// use AsStatement() accessor.
	struct Statement : ILNode
	{
	    Statement * Prev;
	    Statement * Next;
	    ExecutableBlock * Parent;
	    unsigned short ResumeIndex;     // The value to use for the ResumeTarget state variable.
	    unsigned GroupId;               // Group Id of the statement.  All parse tree statements which result in
	                                    // multiple BILTREE statements will have the same GroupId.  Mainly used
	                                    // to track temporaries

	    Statement *GetFirstStatementInGroup(); // walks up as well as previous to find first statement in this group
	};


	// use slExecutableBlock() accessor.
#pragma prefast(push)
#pragma prefast(disable: 25094, "No destructors in the Tree")
	struct ExecutableBlock : Statement
	{
	  union
	  {
	    ILNode            * ptreeChild;
	    Statement * Child;            // readable alias
	  };

	  Location EndConstructLocation;

	  BCSYM_Hash *Locals;            // Local scope
	  unsigned LocalsCount;          // Count of syntactic declarations of locals.
	  BlockScope *LocalScope;        // Scope info for this executable block

	  // List of variables that will need error checking after the implicit variables have been added.
	  // For instance query variables.
	  // These check during expression interpretation in the method body. Statements later on might introduce
	  // variables in scope that should fail because they are now shadowing these variables.
	  // It is impractical to do a 2-pass on the method body, so this list tracks them.
	  TrackingVariablesForImplicitCollisions TrackingVarsForImplicit;

        // Track lambda expressions so that we can perform error checking to ensure the lambdas parameters
        // and locals do not shadow any variables in enclosing blocks.
        Queue<ILTree::LambdaExpression*, NorlsAllocWrapper> *TrackingLambdasForShadowing;

	  BITSET * inDefAsgBitset;        // in bitset: definite assignment bitset, the last time this block was entered...
	  BITSET * outDefAsgBitset;       // out bitset: definite assignment bitset, the last time this block was exited...

	  unsigned short TerminatingResumeIndex;  // The value to use for the ResumeTarget state variable for the terminating construct of the block.
	};
#pragma prefast(pop)


	// ***********************************************************************
	// Structures pertaining to the various types of nodes. The last field
	// in enumtree.h indicates which union to follow.
	// ***********************************************************************


	// ***********************************************************************
	//
	// BLOCK NODES
	//
	// ***********************************************************************


	// An ExternalSourceDirective represents a #ExternalSource directive.
	// FirstLine and LastLine represent the lines between the start and end
	// constructs of the directive.
	//
	// The fields declared here as signed longs are logically unsigned, but
	// are declared to match the (also logically unsigned) signed long fields
	// in Location.
	struct ExternalSourceDirective
	{
	    long FirstLine;
	    long LastLine;
	    WCHAR *ExternalSourceFileName;
	    long ExternalSourceFileStartLine;
	    ExternalSourceDirective *Next;
	};

    // use AsProcedureBlock() accessor.
    struct ProcedureBlock : ExecutableBlock
    {
      BCSYM_Proc       *pproc;                // the procedure sym
      unsigned          DefinedLabelCount;    // count of label definitions

      // All of the following are used by semantics/Codegen only
      Location SigLocation;
      BCSYM_Variable *ReturnVariable;               // the return value (semantics and later)

      SymbolReferenceExpression *LiftedReturnVarRef;            // The symbol reference for the field
                                                    // corresponding to the lifted Return
                                                    // variable. NULL if not lifted. Note
                                                    // that if Return variable is lifted,
                                                    // BILTREE_sbProc::ReturnVariable above
                                                    //is set to be the field.

      // Information generated by semantics to be consumed by codegen.
      TemporaryManager *ptempmgr;                     // TemporaryManager for function

      unsigned short    OnErrorHandlerCount;          // The count of On Error Goto <label> statements.
      unsigned short    OnErrorResumeCount;           // The count of On Error Resume Next statements.
      unsigned          fSeenCatch:1;                 // seen Catch statement
      unsigned          fSeenOnErr:1;                 // seen On Error stmt in function
      unsigned          fSeenResume:1;                // seen Resume statement
      unsigned          fSeenLineNumber:1;            // seen line number (needed for Erl)
      unsigned          fSeenAwait:1;                 // seen Await expression
      unsigned          fEmptyBody:1;                 // was proc body empty (needed for DllImport)
      unsigned          fDefAsgNoRetValReported:1;    // no returnvalue has been reported by Definite assignment
    
      ResumableKind     m_ResumableKind;              // for whether this method was iterator/async
      Type*             m_ResumableGenericType;       // for Task<T>, IEnumerable<T>, IEnumerator<T>, this is that T.
    };

    struct StatementLambdaBlock : ExecutableBlock
    {
        unsigned fSeenCatch:1;
        LambdaExpression *pOwningLambdaExpression;
        Type *pReturnType;
    };
	// Use AsIfSelectBlock() accessor.
	// AsIfSelectBlock - If and Select container
	struct IfSelectBlock : ExecutableBlock
	{
	    CODE_BLOCK *EndBlock;     // code block of end of construct
	};


	// Use AsIfGroup() accessor.
	struct IfGroup : IfSelectBlock
	{
	};


	// Use AsSelectBlock() accessor.
	struct SelectBlock : IfSelectBlock
	{
	    ILNode         * SelectorCapture;    // bound assignment to SelectorTemporary.
	    BCSYM_Variable  * SelectorTemporary;  // temp for select value
	    SWITCH_TABLE    * SwitchTable;        // table switch
	    __int64           Minimum;
	    __int64           Maximum;
    
        SymbolReferenceExpression *LiftedSelectorTemporary;  // If used in a resumable method, we have to lift the temporaries
	};


	//*****************************************************************************
	// For Select Case cases: more than one can be associated with a case
	//*****************************************************************************
	struct CASELIST                            // -- clist
	{
	    CASELIST       * Next;

	    Expression * LowBound;   // Lower bound of case
	    Expression * HighBound;  // Higher bound of case

	    BILOP            RelationalOpcode  :16;     // operator if clRelop is non-zero
	    unsigned         IsRange           :1;      // case is "<from> To <to>"
	};


	// Use AsIfCaseBlock() accessor.
	  // AsIfCaseBlock - If and Case statements
	struct IfCaseBlock : ExecutableBlock
	{
	    union
	    {
	        Expression  * Conditional;   // the condition expr
	        CASELIST        * BoundCaseList; // (bound only)
	    };

	    CODE_BLOCK *FalseBlock; // used by codegen
	};


	// Use AsIfBlock() accessor.
	// AsIfBlock    -- SB_IF, SB_ELSE_IF, SB_ELSE, SB_ELSE2
	struct IfBlock : IfCaseBlock
	{
	};


	// Use AsCaseBlock() accessor.
	struct CaseBlock : IfCaseBlock
	{
	};

	// Use AsTryBlock() accessor.
	struct TryBlock : ExecutableBlock
	{
	    class BITSET * outDefAsgBitsetTryGroup; // out bitset: the general one for all the group
	    FinallyBlock *FinallyTree;         // Set by semantics.
	                                            // Used by control flow as a shortcut to the corresponding finally block if any
	                                            //  when null means the finally block is not known yet or there is no finally block at all
	                                            // Used by codegen
	};

	// Use AsCatchBlock() accessor.
	struct CatchBlock : ExecutableBlock
	{
	  Expression * WhenExpression;
	  Expression * CatchVariableTree;           // The variable where the Exception object will get stored
	  BCSYM_Variable * ExceptionTemporary;          // only used if the Catch Variable is ByRef
	  SymbolReferenceExpression * LiftedExceptionTemporary;  // in case we had to lift it

	  Statement *ExceptionStore;           // Statement-list used to store the exception to a non-local. !!! Warning: not visited by BoundTreeVisitor
	};

	// Use AsFinallyBlock() accessor.
#pragma prefast(push)
#pragma prefast(disable: 25094, "No destructors in the Tree")
	struct FinallyBlock : ExecutableBlock
	{
	    DefAsgReEvalStmtList m_DefAsgGoToFixupList;
	};
#pragma prefast(push)

	  // All loops have these in common.
	  // Bound BIL only - needed by codegen
	  //
	// Use AsGeneralLoopBlock() accessor.
	struct GeneralLoopBlock : ExecutableBlock
	{
	  // Note:
	  // ContinueBlock is used to hold the condition Block for Do and While.
	  // For all "for" statements, this is used to hold the IncrementStatement
	  //
	  // For the complex For Each i.e. for each on collections and (>1)-D
	  // arrays, the incrementer is the movenext call on the enumerator
	  // which is part of the condition. So in this case, this is used to hold
	  // a placeholder block that acts a label to the condition
	  // and thus to the MoveNext call.
	  //
	  CODE_BLOCK *ContinueBlock;    // code block for continue statement to jump to
	  CODE_BLOCK *EndBlock;         // code block of branch out

	  // a loop have two bitset outputs:
	  // 1. the local output bit set, the one reached by the end of the loop or 'continue' and skipped by Exit. It is used in
	  //    input bitset in order to simulate the loop.
	  // 2. the normal output bitset, the bitset for the next statement. It is based on previous one and it is directly accessed
	  //    by exit statement.
	  class BITSET    * outDefAsgBitsetLocal;
	};

	// Use AsForBlock() accessor.
	struct ForBlock : GeneralLoopBlock
	{
	    Expression *ControlVariableReference;
	    BCSYM_Variable *EnumeratorTemporary; // used in late-bound for-each calls
	    CODE_BLOCK *BodyBlock;           // code block for loop body
	    CODE_BLOCK *ConditionBlock;      // Code Block of condition

	    Expression *Initial;
	    Expression *Limit;
	    Expression *Step;
	    Statement *SignCheckStatement;
	    Statement *IncrementStatement;
	    Expression *Condition;
	    BCSYM_Variable *StepTemporary;
	    BCSYM_Variable *LimitTemporary;
    
        SymbolReferenceExpression *LiftedStepTemporary;  // If used in a resumable method, we have to lift the temporaries:
        SymbolReferenceExpression *LiftedLimitTemporary; // in which case, use these rather than the StepTemporary/LimitTemporary
        SymbolReferenceExpression *LiftedEnumeratorTemporary; 
	};

	  /* AsLoopBlock  -- SL_WHILE      -- 'While..Wend', 'DoWhile...Loop' */
	  /*         -- SL_DO         -- 'Do...Loop'                     */
	  /*         -- SL_LOOP_WHILE -- 'Do...LoopWhile'                */
	  //
	// Use AsLoopBlock() accessor.
	struct LoopBlock : GeneralLoopBlock
	{
	  Expression  * Conditional;
	} ;


    // AsForEachBlock  -- SB_FOR_EACH -- 'For Each/Next' statement
	// Use AsForEachBlock() accessor.
	struct ForEachBlock : LoopBlock
	{	
	   // ExpressionWithChildren tree to capture the enumerator value.
	   // (To be executed in the loop prologue.)
	   Expression *Initializer;

	   // Closure Initialization 
	   // It must be generated before CurrentIndexAssignment,
	   Statement *InitializeClosure;

	   // ExpressionWithChildren tree to assign the Current value to the loop control variable.
	   // (To be executed as the first code in the loop body.)
	   Expression *CurrentIndexAssignment;

	   // A tree that refers to the control variable.
	   Expression *ControlVariableReference;

	   Expression *ControlVaribleTempInitializer;

	   // A tree that represents the collection.
	   //
	   // This will be a Scratch node and should NOT EVER be generated as code.  It's
	   // here as a service for XmlGen and the IDE.
	   Expression *Collection;

	   // A Statement that represents the increments to the Array index in the For each
	   // in 1-D Array and string cases.
	   // This is not set (NULL) for the more complex for each on collections because the call
	   // to movenext which is the incrementer is part of the condition and this is stored
	   // in AsLoopBlock.Conditional.
	   //
	   Statement *IncrementStatement;

	};

	// Use AsWithBlock() accessor.
	  /* slWith  -- SL_With     -- 'With' statement */
	struct WithBlock : ExecutableBlock
	{
	  union
	  {
	    PILNode          ptreeWith;                // Unbound: with expr; Bound: assign to temp added
	    Expression  * WithValue;                // readable alias, unbound
	    Expression  * TemporaryBindAssignment;  // readable alias, bound
	  };                                            // NOTE: THIS CAN BE NULL IN CERTAIN SITUATIONS
	                                                //   eg: In certain cases of the synthetic "With"
	                                                //       generated for Aggr Init in

	  Expression  * TemporaryClearAssignment;   // Unbound: null; Bound: clear with temp
	                                                // Can be NULL

	  // During semantic analysis of a With that operates on a record type,
	  // RecordReference is a clonable reference to the With item.
	  Expression  * RecordReference;            // Can be NULL

	  // Used only for the "With" block that is synthesized for object
	  // initializers to hold a reference to the object being initialized.
	  Expression  * ObjectBeingInitialized;     // Can be NULL
	};

	  // ***********************************************************************
	  //
	  // STATEMENT NODES
	  //
	  // ***********************************************************************
	  // General statement nodes with 1 or 2 expression trees
	// Use AsStatementWithExpression() accessor.
	struct StatementWithExpression : Statement
	{
	  union
	  {
	    PILNode          ptreeOp1;
	    Expression  * Operand1;         // readable alias
	  };

	    void SetContainingFile(SourceFile *File)
	    {
	        VSASSERT(this->bilop == SL_STMT, "File specific statements unsupported for other node kinds!!!");
	        ContainingFile = File;
	    }

	    SourceFile* GetContainingFile()
	    {
	        VSASSERT(this->bilop == SL_STMT, "File specific statements unsupported for other node kinds!!!");
	        return ContainingFile;
	    }

	private:
	    SourceFile *ContainingFile;         // Used to indiciate the file in which a partial
	                                        // class member initializer is present. This is
	                                        // needed by codegen to emit the correct pdb info.
	                                        // Bug VSWhidbey 145009.
	                                        // This is NULL for all other statement except member
	                                        // level initializers.

	};


	// --------------------------------------------------------------------
	//  The following nodes are bound BIL
	// --------------------------------------------------------------------


	/* ExitStatementCast  -- SL_EXIT -- exit a Do or For loop */
	// Use ExitStatementCast() accessor.
	struct ExitStatement : Statement
	{
	    EXITKIND        exitkind;           //  Type of statement to exit.
	    ILNode *       ExitedStatement;
	    BITSET *        outDefAsgBitset;    // out bitset: definite assignment bitset, Used for
	                                        // fixing up the targets of try's out of try/catch,
	                                        // when out bitsets of finally blocks change
	};

	/* AsReturnStatement -- SL_RETURN -- return from a sub or function */
	// Use AsReturnStatement() accessor.
	struct ReturnStatement : Statement
	{
        PILNode        ReturnExpression;

	    BITSET *        outDefAsgBitset;    // out bitset: definite assignment bitset, Used for
	                                        // fixing up the targets of try's out of try/catch,
	                                        // when out bitsets of finally blocks change
	};

	/* AsYieldStatement -- SL_YIELD -- yield a value inside an iterator */
	// Use AsYieldStatement() accessor.
    struct YieldStatement : Statement
    {
        Expression * YieldExpression;
    };

	/* AsContinueStatement  -- SL_CONTINUE -- Continue in a Do or For loop */
	// Use AsContinueStatement() accessor.
	struct ContinueStatement : Statement
	{
	  ILNode *       EnclosingLoopStatement;
	};

	/* AsLabelStatement -- SL_LABEL    -- label definition */
	// Use AsLabelStatement() accessor.
	struct LabelStatement : Statement
	{
	    LABEL_ENTRY_BIL * Label;
	    BITSET * inDefAsgBitset;  // in bitset: definite assignment bitset,
	                              // the last time this label was visited
	  //BILTREE_sbExecutableBlock *   parent;               // back pointer to the owning block. Used by DefAsg
	};

	/* AsGotoStatement -- SL_GOTO -- 'GoTo' statement */
	// Use AsGotoStatement() accessor.
	struct GotoStatement : Statement
	{
	    LABEL_ENTRY_BIL * Label;   // Label to go to
	    BITSET * outDefAsgBitset;  // out bitset: definite assignment bitset, Used for
	                               // fixing up the targets of goto's out of try/catch,
	                               // when out bitsets of finally blocks change
	};

    /* AsAsyncSwitchStatement -- SL_ASYNCSWITCH -- efficient switch table generated by ResumableMethodRewriter; goes straight to codegen */
    // Use AsAsyncSwitchStatement() accessor.
    //
    // WARNING: This struct's sole purpose is to go from ResumableMethodRewriter on to Codegen.
    // Most of the compiler (e.g. BilTreeCopy, FlowAnalysis, ...) doesn't know how to cope with it.
    struct AsyncSwitchStatement : Statement
    {
        Expression * NormalizedSelector; // This generates "switch (NormalizedSelector) {
        unsigned long cEntries;          //   case 0: goto Labels[0] ...}"
        LABEL_ENTRY_BIL ** Labels;       // Selector must be of type UInt32.
    };

	/* AsOnErrorStatement -- SL_ON_ERR   -- 'On Error' statement */
	/* AsOnErrorStatement -- SL_RESUME   -- 'Resume' statement */
	// Use AsOnErrorStatement() accessor.
	struct OnErrorStatement : Statement
	{
	    LABEL_ENTRY_BIL * Label;
	    int OnErrorIndex;  // The value to use for the ActiveHandler state variable.  This is valid only for On Error statements.
	};


	// Use AsRedimStatement() accessor.
	struct RedimStatement : StatementWithExpression
	{
	                               // Operand1 is the array
	    Expression *Operand2;  // the list (of what?)
	    unsigned uDims:16;         // count of dimensions
	    Vtypes vtype:16;
	    BCSYM *ptyp;
	};

	// ***********************************************************************
	//
	// EXPRESSION NODES
	//
	// ***********************************************************************

	// All SXK_SMPOP node types should inherit from _sxOp_ struct.
	// This is set up so that routines that work on all SXK_SMPOP node
	// types can use the union, AsExpressionWithChildren.
	//

	// Use AsExpressionWithChildren() accessor.
	struct ExpressionWithChildren : Expression
	{
	    Expression * Left;
	    Expression * Right;
	};

	// Use AsBinaryExpression() accessor.
	struct BinaryExpression : ExpressionWithChildren
	{
	};

    // use AsAwaitExpression() accessor
    struct AwaitExpression : BinaryExpression
    {
        Expression * GetAwaiterDummy;
        Expression * IsCompletedDummy;
        Expression * GetResultDummy;
    };

    // Use AsAsyncSpillExpression() accessor
    struct AsyncSpillExpression : Expression
    {
        Expression* IsCompletedCondition;
        Expression* CreateAwaiterArray;
        Expression* AwaitXAssignment;
        Expression* OnCompletedCall;
        Expression* OnCompletedExtraCondition;
        Expression* OnCompletedExtraIfTrue;
        Expression* OnCompletedExtraIfFalse1;
        Expression* OnCompletedExtraIfFalse2;
        Expression* DoFinallyBodiesAssignment;
        Expression* StateAssignment;
        Expression* StateResetAssignment;
        Expression* RestoreAwaiterStatements;
        SymbolReferenceExpression* StackStorageReference;
        LABEL_ENTRY_BIL* ResumeLabel;
    };

	// use AsUserDefinedBinaryOperatorExpression() accessor.
	struct UserDefinedBinaryOperatorExpression : BinaryExpression
	{
	  BCSYM_Proc*               OperatorMethod;        // proc for the operator.
	  BCSYM_GenericBinding*     OperatorMethodContext;    // generic binding for operator.
	  __int64                   InterpretationFlags;
	};


	// use AsUserDefinedUnaryOperatorExpression() accessor.
	struct UserDefinedUnaryOperatorExpression : ExpressionWithChildren
	{
	  BCSYM_Proc*               OperatorMethod;        // proc for the operator.
	  BCSYM_GenericBinding*     OperatorMethodContext;    // generic binding for operator.
	  __int64                   InterpretationFlags;
	};

	// --------------------------------------------------------------------
	//  The following nodes are only used for unbound BIL (ie Parsing)
	// --------------------------------------------------------------------

	/* AsArgumentNameExpression - UNBOUND identifier (name, label, etc) */
	// Use AsArgumentNameExpression() accessor.
	struct ArgumentNameExpression : Expression
	{
	  STRING          * Name;
	  typeChars         TypeCharacter;
	};


	/* AsArgumentNameExpression    -- argument value descriptor (SX_ARG) */
	// Use AsArgumentNameExpression() accessor.
	struct ArgumentExpression : ExpressionWithChildren
	{
	  ArgumentNameExpression* Name;           // if SXF_ARG_NAMED
	};


	// --------------------------------------------------------------------
	//  The following nodes are bound BIL
	// --------------------------------------------------------------------


	// sx_ApplAttr is used to store a bound attribute, Left points to
	// an SX_Call for the contructor and Right points to an SX_List of
	// SX_Asg nodes for the field/property mappings.
	struct AttributeApplicationExpression : BinaryExpression
	{
	    ;
	};


	/* AsIntegralConstantExpression -- integer constant (SX_CNS_INT) */
	// Use AsIntegralConstantExpression() accessor.
	struct IntegralConstantExpression : Expression
	{
	  union
	  {
	    __int64           Value;
	  };
	  typeChars         TypeCharacter;
	};


	/* AsDecimalConstantExpression -- decimal constant (SX_CNS_DEC) */
	// Use sxCurDec() accessor.
	struct DecimalConstantExpression : Expression
	{
	  DECIMAL           Value;
	  typeChars         TypeCharacter;
	};


	/* AsFloatConstantExpression -- floating constant (SX_CNS_FLT) */
	// Use AsFloatConstantExpression() accessor.
	struct FloatConstantExpression : Expression
	{
	  double    Value;      //  Float value
	  typeChars         TypeCharacter;
	};


	/* AsSymbolReferenceExpression -- member or non-member variable or procedure (SX_SYM) */
	// Use AsSymbolReferenceExpression() accessor.
	struct SymbolReferenceExpression : Expression
	{
	  union
	  {
	    PILNode          ptreeQual;      // Null if no qualifier
	    Expression  * BaseReference;  // readable alias
	  };
	  union
	  {
	    BCSYM_NamedRoot * pnamed;
	    BCSYM_NamedRoot * Symbol;         // readable alias
	  };

	  BCSYM_GenericBinding *GenericBindingContext;

	  // cache used by definite assignment to avoid multiple runs over the same
	  // tree and uncontroled generation of generic bindings.
	  // the cache enconding is:
	  //    0           - not seen yet;
	  //    number+1    - the cached value is 'number'>=0
	  unsigned  defAsgSlotCache;
	  unsigned  defAsgSizeCache;
	};

	// Ternary IIF (condition, expr1, expr2). Translates into if(condition) expr1 else expr2.
	// condition has boolean type, expr1 and expr2 have same type with IIF node
	// Use AsIfExpression() accessor.
	struct IfExpression : BinaryExpression
	{
	                                // ptreeOp1 is expr1
	                                // ptreeOp2 is expr2
	    Expression   *condition;

	};

	// Binary IIF (coalesce) operator. IIF(Expr1,Expr2) is equivalent to IIF(Expr1 ISNOT Nothing, Expr1 , Expr2)
	// Transformed into a ternary IIF in lowering
	// use AsCoalesceExpression() accesor.
	struct CoalesceExpression : BinaryExpression
	{
	  // ptreeOp1 is Expr1
	  // ptreeOp2 is Expr2
	  // the result type is type of Expr2 (when Expr1 widens to Expr2), or type of Expr1 when Expr2 widens to Expr1.
	  // When Expr1 is Nullable and Expr2 is not nullable, the type of the value in Expr1 is used for computing the
	  // result type.
	};

	// Short circuit operator.
	struct ShortCircuitBooleanOperatorExpression : UserDefinedBinaryOperatorExpression
	{
	    // We need to store IfTrue/IfFalse.
	    BCSYM_Proc*     ConditionOperator;
	    BCSYM_GenericBinding* ConditionOperatorContext;
	};

	// Lifted user defined CTYPE operator. Temporary node fully resolved into in iif null check in lowering step.
	// Source and target types are nullable types, i,e S? -> T?. Yet the operator method is an user defined Ctype for S->T
	// Use AsLiftedCTypeExpression() accessor.
	struct LiftedCTypeExpression : UserDefinedUnaryOperatorExpression
	{
	    // ptreeOp1 is input expression
	    // OperatorMethod, OperatorMethodContext, InterpretationFlags define the lifted method to convert S->T
	};

	// Use AsCallExpression() accessor.
	struct CallExpression : BinaryExpression
	{
	                                // ptreeOp1 is the Proc
	                                // ptreeOp2 is the Args
	                                // pmemvTemp is the for RetVal (variant, currency, date)
	  union
	  {
	    PILNode          ptreeThis;  // &object if method call (qualifier)
	    Expression   *MeArgument; // readable alias
	  };

	    BILOP             OperatorOpcode;   // Used to track the operator bilop for the user
	                                        // defined operator.
	};

	// Use AsDelegateConstructorCallExpression accessor.
	struct DelegateConstructorCallExpression : Expression
	{
	    PILNode          Constructor;      // the delegate constructor to call
	    PILNode          ObjectArgument;   // expression for referencing the method
	    PILNode          Method;           // method to construct a delegate for
	};

	// Use AsPropertyReferenceExpression() accessor.
	struct PropertyReferenceExpression : BinaryExpression
	{
	  typeChars         TypeCharacter;
	};

	/* AsLateBoundExpression  -- late-bound member/method access (SX_LATE_XXX) */
	// Use AsLateBoundExpression() accessor.
	struct LateBoundExpression : ExpressionWithChildren
	{
	                                     // ptreeOp1 is the Object
	                                     // ptreeOp2 is the Args

	  Expression    * LateIdentifier; // Method to call

	  // If and only if there is no Me argument in the call, LateClass gives
	  // the class in which to search for the method to call.
	  BCSYM             * LateClass;

	  PILNode            AssignmentInfoArrayParam;
	  Expression    * TypeArguments;
	};

	/* AsVariantIndexExpression  -- indexing a variant as an array (SX_VARINDEX) */
	// Use AsVariantIndexExpression() accessor.
	struct VariantIndexExpression : BinaryExpression
	{
	                                    // ptreeOp1 is the Variant
	                                    // ptreeOp2 is the Args
	};

	/* AsIndexExpression  -- array access (SX_INDEX, SX_INDEX_REDIM)
	               (access an array element) */
	// Use AsIndexExpression() accessor.
	struct IndexExpression : BinaryExpression
	{
	                                  // ptreeOp1 is the Array
	                                  // ptreeOp2 is the Subscript List
	  union
	  {
	    USHORT            uCount;       // number of dims
	    USHORT            DimensionCount; // readable alias
	  };
	};


	// Use AsNewExpression() accessor.
	struct NewExpression : Expression
	{
	  union
	  {
	    PILNode          ptreeNew;   // CALL bound
	                                  // MUST BE SAME OFFSET AS ptreeOp1

	    Expression  * ConstructorCall;   // readable alias -- bound
	  };

	  union
	  {
	    BCSYM  * pmod;          // class to new (bound only)
	    BCSYM  * Class;         // readable alias -- bound
	  };
	};

	// Use AsInitStructureExpression() accessor.
	struct InitStructureExpression : Expression
	{
	  Expression  *StructureReference;
	  BCSYM          *StructureType;
	};

	// SX_CNS_STR
	// Use AsStringConstant() accessor.
	struct StringConstantExpression : Expression
	{
	  const WCHAR *         Spelling;    // null terminated string
	  size_t          Length;
	};

	// Use AsOverloadedGenericExpression() accessor.
	struct OverloadedGenericExpression : Expression
	{
	  Expression  *BaseReference;
	  BCSYM          **TypeArguments;
	  Location        *TypeArgumentLocations;
	  unsigned         TypeArgumentCount;
	};

	// Use AsExtensionCallExpression() accessor.
	struct ExtensionCallExpression : Expression
	{
	    //Stores the list of implicit arguments to the extension call
	    //Generally this list will contain a single item,
	    //the receiver (me reference) of the extension method call.
	    //It should be an SX_LIST whose elements are SX_ARG structures.
	    //It will be prepended to the list of arguments supplied to the function
	    //in code.
	    ExpressionWithChildren * ImplicitArgumentList;

	    //Stores explicit type arguments for the call. These are set inside the
	    //GenericQualifiedExpression branch of Semantics::IntepretExpression
	    BCSYM ** TypeArguments;
	    unsigned int TypeArgumentCount;
	    BCSYM_ExtensionCallLookupResult * ExtensionCallLookupResult;
	    Location * TypeArgumentLocations;

	    //The error to display if we bind to an instance method or an extension method.
	    //The error will not be displayed if we bound to a shared method defined in the receiver type.
	    //Extension method name lookup will synthesize a me refrence in order to do partial type inference.
	    //This will results in errors being reported if me synthesis is not possilbe (because we are in a static method,
	    //or because a call is qualified with a typename). However, overload instancemethods and extension methods
	    //allows shared methods to be bound to. If that happens, we do not want to generate an error.
	    //As a result, we track the error inside me synthesis, store it's ID here, and then report it later.
	    unsigned ImplicitMeErrorID;
	};


    // LambdaExpression - represents an inline function. There are ten types:
    // Function() foo()                                   -- !IsStatementLambda,  IsFunctionLambda, !IsAsyncKeywordUsed, !IsIteratorKeywordUsed, !IsConvertedFromAsyncSingleLineFuntion
    // Sub() Expression                                   -- !IsStatementLambda, !IsFunctionLambda, !IsAsyncKeywordUsed, !IsIteratorKeywordUsed, !IsConvertedFromAsyncSingleLineFuntion
    // Sub() foo()                                        --  IsStatementLambda, !IsFunctionLambda, !IsAsyncKeywordUsed, !IsIteratorKeywordUsed, !IsConvertedFromAsyncSingleLineFuntion
    // Function() : return foo() : End Function           --  IsStatementLambda,  IsFunctionLambda, !IsAsyncKeywordUsed, !IsIteratorKeywordUsed, !IsConvertedFromAsyncSingleLineFuntion
    // Sub() : foo() : End Sub                            --  IsStatementLambda, !IsFunctionLambda, !IsAsyncKeywordUsed, !IsIteratorKeywordUsed, !IsConvertedFromAsyncSingleLineFuntion
    // Async Function() foo()                             --  IsStatementLambda,  IsFunctionLambda,  IsAsyncKeywordUsed, !IsIteratorKeywordUsed,  IsConvertedFromAsyncSingleLineFuntion
    // Async Sub() foo()                                  --  IsStatementLambda, !IsFunctionLambda,  IsAsyncKeywordUsed, !IsIteratorKeywordUsed, !IsConvertedFromAsyncSingleLineFuntion
    // Async Function() : return foo() : End Function     --  IsStatementLambda,  IsFunctionLambda,  IsAsyncKeywordUsed, !IsIteratorKeywordUsed, !IsConvertedFromAsyncSingleLineFuntion
    // Async Sub() : foo() : End Sub                      --  IsStatementLambda, !IsFunctionLambda,  IsAsyncKeywordUsed, !IsIteratorKeywordUsed, !IsConvertedFromAsyncSingleLineFuntion
    // Iterator Function() : return foo() : End Function  --  IsStatementLambda,  IsFunctionLambda, !IsAsyncKeywordUsed,  IsIteratorKeywordUsed, !IsConvertedFromAsyncSingleLineFuntion

    // Use AsLambdaExpression() accessor.
    struct LambdaExpression : Expression
    {        
        BCSYM_Param    *FirstParameter;

        TemporaryManager *TemporaryManager;

        unsigned __int64 ConvertResultTypeFlags; //ExpressionFlags to pass to ConvertWithErrorChecking
        unsigned char ExactlyMatchExpressionResultType:1; // Do not apply conversions to the LambdaExpression to match delegate's return type, LambdaExpression's type should match exactly
        unsigned char IsStatementLambda:1; // Distinguish between Expression Lambdas and Statement Lambdas
        unsigned char IsFunctionLambda:1; // Distinguish between Function and Sub lambda expressions.
        unsigned char IsAsyncKeywordUsed:1; // e.g. "Dim f = Async Function() : ... : End Function"
        unsigned char IsIteratorKeywordUsed:1; // e.g. "Dim f = Iterator Function() : ... : End Function"
        //unsigned char IsSingleLineLambda:1; // Distinguish between single-line and multi-line lambdas
        unsigned char IsRelaxedDelegateLambda:1; // Mark lambdas used to create relaxed delegates.
        unsigned char IsExplicitlyConverted:1;
        unsigned char IsConvertedFromAsyncSingleLineFunction:1;
        ResumableKind m_ResumableKind;   // for whether this lambda was iterator/async
        Type *m_ResumableGenericType;    // for Task<T>, IEnumerable<T>, IEnumerator<T>, this is that T.

        bool            IsPartOfQuery;
        Location BodySpan;   // BodySpan goes from the end of "Function()" to the start of "End Function"
        // <4 Sub() <5                     <4..4> (the normal ILTree) TextSpan
        //    Console.WriteLine("hello")   <5..5> BodySpan
        // 5> End Sub 4>
        // NOTE: like everything else here, we rely on the fact that (1) this class is zero-inited upon
        // allocation, and (2) a location that's zero-inited is considered invalid.

        BCSYM_Hash *GetLocalsHash();

        void SetLocalsHash(_In_ BCSYM_Hash *pValue);

        Expression **GetAddressOfExpressionLambdaBody();

        ILNode *GetLambdaBody();

        Expression *GetExpressionLambdaBody()
        {
            // This routine must stay inside BoundTrees.h so that vbdgee can compile okay. vbdgee includes the header but not the .cpp of this struct.
            VSASSERT( !IsStatementLambda, "This was not an expression lambda");
            return !IsStatementLambda ? pLambdaExpression : NULL;
        }

        Type *GetExpressionLambdaReturnType();

        void SetExpressionLambdaBody(_In_ Expression *pValue);

        StatementLambdaBlock *GetStatementLambdaBody();

        void SetStatementLambdaBody(_In_ StatementLambdaBlock *pValue);

    private:
        BCSYM_Hash *pLocalsHash;
       union 
        {
            Expression *pLambdaExpression;

            StatementLambdaBlock *pLambdaStatementBlock;
        };
    };

    // Use AsUnboundLambdaExpression() accessor.
    struct UnboundLambdaExpression : Expression
    {
        BCSYM_Param           *FirstParameter;

    private:
        union
        {
            // These fields are private so we can have getter/setter methods to VSASSERT the
            // consistency of them with respect to various flag fields, e.g. IsStatementLambda, IsFunctionLambda...
            ParseTree::Expression *LambdaExpression;         // for !IsStatementLambda
            ParseTree::LambdaBodyStatement*pLambdaStatement; // for  IsStatementLambda
        };
    public:
        ParseTree::Expression *GetLambdaExpression()
        {
            VSASSERT(!IsStatementLambda, "Internal error: can't get expression of a statement lambda");
            return LambdaExpression;
        }

        void SetLambdaExpression(ParseTree::Expression *LambdaExpression)
        {
            this->LambdaExpression = LambdaExpression;
        }

        ParseTree::LambdaBodyStatement *GetLambdaStatement()
        {
            VSASSERT(IsStatementLambda || IsAsyncKeywordUsed, "Internal error: can't get statement-body of an expression lambda");
            return pLambdaStatement;
        }

        void SetLambdaStatement(ParseTree::LambdaBodyStatement *pLambdaStatement)
        {
            this->pLambdaStatement = pLambdaStatement;
        }
        

        bool                  AllowRelaxationSemantics;
        unsigned __int64      InterpretBodyFlags; //ExpressionFlags to pass to InterpretExpression of the body
        unsigned char ExactlyMatchExpressionResultType:1; // Do not apply conversions to the LambdaExpression to match delegate's return type, LambdaExpression's type should match exactly
        unsigned char IsStatementLambda:1; // Distinguish between Expression Lambdas and Statement Lambdas. Note:
                                           // for a statement lambda masquerading as an expression, this flag is false.
                                           // Thus, this flag can be used to decide whether to GetLambdaExpression() or GetLambdaStatement()
        unsigned char IsSingleLine : 1; // Is it a line-lambda?
        unsigned char IsFunctionLambda:1; // Distinguish between Sub and Function lambdas
        unsigned char IsAsyncKeywordUsed:1;
        unsigned char IsIteratorKeywordUsed:1;
        Location BodySpan;   // see comment about BodySpan in "LambdaExpression" above for its geometry

    };

        // Use sxDeferredTemp() accessor.
    struct DeferredTempExpression : Expression
    {
        unsigned Id;
        unsigned __int64      InterpretFlags; //ExpressionFlags to pass to InterpretExpression of the body
        ParseTree::Expression *InitialValue;
    };

	// ----------------------------------------------------------------------------
	// Contains the binding information for anonymous types. This would be:
	// 1. The bound property corresponding to this member.
	// 2. The bound expression.
	// 3. The temporary (if applicable) that contains the value for the bound
	//    expression.
	//
	// Expression trees cannot hold temps; thus, for expression trees, all expressions
	// must be non-temp-based.
	// ----------------------------------------------------------------------------

    struct BoundMemberInfoList
    {
        BCSYM_Property* Property;
        Expression* BoundExpression;
        Expression* Temp;

        BoundMemberInfoList() :
            Property( NULL ),
            BoundExpression( NULL ),
            Temp( NULL )
        {

        }
    };

    // Use AsAnonymousTypeExpression() accessor.
    // This tree must not go to code gen. The lowering phase would rebuild it.
    // It contains bound information for anonymous types.
    struct AnonymousTypeExpression : Expression
    {
        ULONG BoundMembersCount;
        BoundMemberInfoList* BoundMembers;
        BCSYM_Proc* Constructor;
        BCSYM_GenericBinding* AnonymousTypeBinding;
    };

    struct ArrayLiteralExpression : Expression
    {
        ExpressionWithChildren * ElementList; 
        unsigned NumDominantCandidates; // if there were !=1 DominantCandidates, then Dominant will be "Object" by assumption
        unsigned Rank;
        unsigned * Dims;
    };

    struct NestedArrayLiteralExpression : Expression
    {
        ExpressionWithChildren * ElementList;
    };

    //Represents a collection initializer
    struct ColInitExpression : Expression
    {
        Expression * NewExpression;
        //A list of SX_ColInitElement nodes
        ExpressionWithChildren * Elements;
        SymbolReferenceExpression * ResultTemporary;
    };

    struct ColInitElementExpression : Expression
    {
        Expression * CallExpression;
        Expression * CopyOutArguments;
        //Stores the flags that were passed into the 
        //InterpretCallExpression call that 
        //created the expression. 
        ExpressionFlags CallInterpretationFlags; 
    };

	/*****************************************************************************/
    /*
    /* Inline functions to access the private union members of BILTREE
    /*
    /*****************************************************************************/
    inline struct Expression &  ILNode::AsExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        return(*(Expression *)this);
    }
    inline struct Statement &  ILNode::AsStatement()
    {
        VSASSERT(this->IsStmtNode(), "ILNode: using wrong node type!");
        return(*(Statement *)this);
    }
#pragma prefast(push)
#pragma prefast(disable: 25094, "No destructors in the ILNode")
    inline struct ExecutableBlock &  ILNode::AsExecutableBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        return(*(ExecutableBlock *)this);
    }
#pragma prefast(pop)
    inline struct ProcedureBlock &  ILNode::AsProcedureBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT((strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ProcedureBlock") == 0) ||
                 (strcmp(s_szBiltreeWhichSubStruct[this->bilop], "sbProcDef") == 0),
                 "ILNode: using wrong node type!");
        return(*(ProcedureBlock *)this);
    }

    inline struct StatementLambdaBlock & ILNode::AsStatementLambdaBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(strcmp(s_szBiltreeWhichSubStruct[this->bilop], "StatementLambdaBlock") == 0,
                 "ILNode: using wrong node type!");
        return(*(StatementLambdaBlock *)this);

    }

    inline struct IfSelectBlock &  ILNode::AsIfSelectBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(this->bilop == SB_IF || this->bilop == SB_SELECT,
                 "ILNode: using wrong node type!");
        return(*(IfSelectBlock *)this);
    }
    inline struct IfGroup &  ILNode::AsIfGroup()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "IfGroup"),
                 "ILNode: using wrong node type!");
        return(*(IfGroup *)this);
    }
    inline struct SelectBlock &  ILNode::AsSelectBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "SelectBlock"),
                 "ILNode: using wrong node type!");
        return(*(SelectBlock *)this);
    }
    inline struct IfCaseBlock &  ILNode::AsIfCaseBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(this->bilop == SB_IF ||
                 this->bilop == SB_CASE ||
                 this->bilop == SB_ELSE ||
                 this->bilop == SB_ELSE_IF,
                 "ILNode: using wrong node type!");
        return(*(IfCaseBlock *)this);
    }
    inline struct IfBlock &  ILNode::AsIfBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "IfBlock"),
                 "ILNode: using wrong node type!");
        return(*(IfBlock *)this);
    }
    inline struct CaseBlock &  ILNode::AsCaseBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "CaseBlock"),
                 "ILNode: using wrong node type!");
        return(*(CaseBlock *)this);
    }
    inline struct TryBlock &  ILNode::AsTryBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "TryBlock"),
                 "ILNode: using wrong node type!");
        return(*(TryBlock *)this);
    }
    inline struct CatchBlock &  ILNode::AsCatchBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "CatchBlock"),
                 "ILNode: using wrong node type!");
        return(*(CatchBlock *)this);
    }
    inline struct FinallyBlock &  ILNode::AsFinallyBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "FinallyBlock"),
                 "ILNode: using wrong node type!");
        return(*(FinallyBlock *)this);
    }
    inline struct GeneralLoopBlock &  ILNode::AsGeneralLoopBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(this->bilop == SB_FOR   ||
                 this->bilop == SB_FOR_EACH  ||
                 this->bilop == SB_DO    ||
                 this->bilop == SB_LOOP,
                 "ILNode: using wrong node type!");
        return(*(GeneralLoopBlock *)this);
    }
    inline struct ForBlock &  ILNode::AsForBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ForBlock"),
                  "ILNode: using wrong node type!");
        return(*(ForBlock *)this);
    }
    inline struct ForEachBlock &  ILNode::AsForEachBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ForEachBlock"),
                 "ILNode: using wrong node type!");
        return(*(ForEachBlock *)this);
    }
    inline struct LoopBlock &  ILNode::AsLoopBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "LoopBlock") ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ForEachBlock"),
                 "ILNode: using wrong node type!");
        return(*(LoopBlock *)this);
    }
    inline struct WithBlock &  ILNode::AsWithBlock()
    {
        VSASSERT(this->IsExecutableBlockNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "WithBlock"),
                 "ILNode: using wrong node type!");
        return(*(WithBlock *)this);
    }

#ifdef DEBUG
    inline bool ILNode::_DerivesFromStmtOp2_()
    {
        return    !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "StatementWithExpression") ||
                  !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "RedimStatement");    // inherits slStmtOp2

    }
#endif

    inline struct StatementWithExpression &  ILNode::AsStatementWithExpression()
    {
        VSASSERT(this->IsStmtNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "StatementWithExpression") ||
                 this->_DerivesFromStmtOp2_(),
                 "ILNode: using wrong node type!");
        return(*(StatementWithExpression *)this);
    }
    inline struct ExitStatement &  ILNode::AsExitStatement()
    {
        VSASSERT(this->IsStmtNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ExitStatement"),
                 "ILNode: using wrong node type!");
        return(*(ExitStatement *)this);
    }
    inline struct ReturnStatement &  ILNode::AsReturnStatement()
    {
        VSASSERT(this->IsStmtNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ReturnStatement"),
                 "ILNode: using wrong node type!");
        return(*(ReturnStatement *)this);
    }
    inline struct YieldStatement & ILNode::AsYieldStatement()
    {
        VSASSERT(this->IsStmtNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "YieldStatement"),
                 "ILNode: using wrong node type!");
        return(*(YieldStatement *)this);
    }
    inline struct ContinueStatement &  ILNode::AsContinueStatement()
    {
        VSASSERT(this->IsStmtNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ContinueStatement"),
                 "ILNode: using wrong node type!");
        return(*(ContinueStatement *)this);
    }
    inline struct LabelStatement &  ILNode::AsLabelStatement()
    {
        VSASSERT(this->IsStmtNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "LabelStatement"),
                 "ILNode: using wrong node type!");
        return(*(LabelStatement *)this);
    }
    inline struct GotoStatement &  ILNode::AsGotoStatement()
    {
        VSASSERT(this->IsStmtNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "GotoStatement"),
                 "ILNode: using wrong node type!");
        return(*(GotoStatement *)this);
    }
    inline struct AsyncSwitchStatement &  ILNode::AsAsyncSwitchStatement()
    {
        VSASSERT(this->IsStmtNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "AsyncSwitchStatement"),
                 "ILNode: using wrong node type!");
        return(*(AsyncSwitchStatement *)this);
    }
    inline struct OnErrorStatement &  ILNode::AsOnErrorStatement()
    {
        VSASSERT(this->IsStmtNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "OnErrorStatement"),
                  "ILNode: using wrong node type!");
        return(*(OnErrorStatement *)this);
    }
    inline struct RedimStatement &  ILNode::AsRedimStatement()
    {
        VSASSERT(this->IsStmtNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "RedimStatement"),
                 "ILNode: using wrong node type!");
        return(*(RedimStatement *)this);
    }
    inline struct ExpressionWithChildren &  ILNode::AsExpressionWithChildren()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(Sxkind(this->bilop) & SXK_SMPOP,
                 "ILNode: using wrong node type!");
        return(*(ExpressionWithChildren *)this);
    }
    inline struct BinaryExpression &  ILNode::AsBinaryExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "BinaryExpression")  ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "AwaitExpression") ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "CallExpression")   ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "LateBoundExpression")   ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "VariantIndexExpression") ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "IfExpression") ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "CoalesceExpression") ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "IndexExpression")  ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "BinaryExpression") ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ShortCircuitBooleanOperatorExpression"),
                 "ILNode: using wrong node type!");
        return(*(BinaryExpression *)this);
    }
    inline struct AttributeApplicationExpression &  ILNode::AsAttributeApplicationExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "AttributeApplicationExpression"),
                 "ILNode: using wrong node type!");
        return(*(AttributeApplicationExpression *)this);
    }
    inline struct ArgumentNameExpression &  ILNode::AsArgumentNameExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ArgumentNameExpression"),
                 "ILNode: using wrong node type!");
        return(*(ArgumentNameExpression *)this);
    }
    inline struct ArgumentExpression &  ILNode::AsArgumentExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ArgumentExpression"),
                 "ILNode: using wrong node type!");
        return(*(ArgumentExpression *)this);
    }
    inline struct IntegralConstantExpression &  ILNode::AsIntegralConstantExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "IntegralConstantExpression"),
                 "ILNode: using wrong node type!");
        return(*(IntegralConstantExpression *)this);
    }
    inline struct DecimalConstantExpression &  ILNode::AsDecimalConstantExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "DecimalConstantExpression"),
                 "ILNode: using wrong node type!");
        return(*(DecimalConstantExpression *)this);
    }
    inline struct FloatConstantExpression &  ILNode::AsFloatConstantExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "FloatConstantExpression"),
                 "ILNode: using wrong node type!");
        return(*(FloatConstantExpression *)this);
    }
    inline struct SymbolReferenceExpression &  ILNode::AsSymbolReferenceExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "SymbolReferenceExpression"),
                 "ILNode: using wrong node type!");
        return(*(SymbolReferenceExpression *)this);
    }
    inline struct DebugObjectExpression &  ILNode::AsDebugObjectExpression()
    {
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "DebugObjectExpression"),
                 "ILNode: using wrong node type!");
        return(*(DebugObjectExpression *)this);
    }
    inline struct IfExpression &  ILNode::AsIfExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "IfExpression"),
                 "ILNode: using wrong node type!");
        return(*(IfExpression *)this);
    }

    inline struct CoalesceExpression &  ILNode::AsCoalesceExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "CoalesceExpression"),
                 "ILNode: using wrong node type!");
        return(*(CoalesceExpression *)this);
    }
    inline struct AwaitExpression & ILNode::AsAwaitExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "AwaitExpression"),
                 "ILNode: using wrong node type!");
        return(*(AwaitExpression *)this);
    }
    inline struct AsyncSpillExpression & ILNode::AsAsyncSpillExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "AsyncSpillExpression"),
                 "ILNode: using wrong node type!");
        return (*(AsyncSpillExpression *)this);
    }
    inline struct UserDefinedBinaryOperatorExpression &  ILNode::AsUserDefinedBinaryOperatorExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "BinaryExpression")  ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ShortCircuitBooleanOperatorExpression"),
                 "ILNode: using wrong node type!");
        return(*(UserDefinedBinaryOperatorExpression *)this);
    }
    inline struct UserDefinedUnaryOperatorExpression &  ILNode::AsUserDefinedUnaryOperatorExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "BinaryExpression")  ||
                 !strcmp(s_szBiltreeWhichSubStruct[this->bilop], "LiftedCTypeExpression"),
                 "ILNode: using wrong node type!");
        return(*(UserDefinedUnaryOperatorExpression *)this);
    }
    inline struct ShortCircuitBooleanOperatorExpression &  ILNode::AsShortCircuitBooleanOperatorExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ShortCircuitBooleanOperatorExpression"),
                 "ILNode: using wrong node type!");
        return(*(ShortCircuitBooleanOperatorExpression *)this);
    }
    inline struct LiftedCTypeExpression &  ILNode::AsLiftedCTypeExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "LiftedCTypeExpression"),
                 "ILNode: using wrong node type!");
        return(*(LiftedCTypeExpression *)this);
    }
    inline struct CallExpression &  ILNode::AsCallExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "CallExpression"),
                 "ILNode: using wrong node type!");
        return(*(CallExpression *)this);
    }
    inline struct DelegateConstructorCallExpression &  ILNode::AsDelegateConstructorCallExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "DelegateConstructorCallExpression"),
                  "ILNode: using wrong node type!");
        return(*(DelegateConstructorCallExpression *)this);
    }
    inline struct PropertyReferenceExpression &  ILNode::AsPropertyReferenceExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "PropertyReferenceExpression"),
                 "ILNode: using wrong node type!");
        return(*(PropertyReferenceExpression *)this);
    }
    inline struct LateBoundExpression &  ILNode::AsLateBoundExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "LateBoundExpression"),
                 "ILNode: using wrong node type!");
        return(*(LateBoundExpression *)this);
    }
    inline struct VariantIndexExpression &  ILNode::AsVariantIndexExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "VariantIndexExpression"),
                 "ILNode: using wrong node type!");
        return(*(VariantIndexExpression *)this);
    }
    inline struct IndexExpression &  ILNode::AsIndexExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "IndexExpression"),
                 "ILNode: using wrong node type!");
        return(*(IndexExpression *)this);
    }
    inline struct NewExpression &  ILNode::AsNewExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "NewExpression"),
                 "ILNode: using wrong node type!");
        return(*(NewExpression *)this);
    }
    inline struct InitStructureExpression &  ILNode::AsInitStructureExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "InitStructureExpression"),
                 "ILNode: using wrong node type!");
        return(*(InitStructureExpression *)this);
    }
    inline struct StringConstantExpression &  ILNode::AsStringConstant()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "StringConstantExpression"),
                 "ILNode: using wrong node type!");
        return(*(StringConstantExpression *)this);
    }
    inline struct OverloadedGenericExpression &  ILNode::AsOverloadedGenericExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "OverloadedGenericExpression"),
                 "ILNode: using wrong node type!");
        return(*(OverloadedGenericExpression *)this);
    }

    inline struct ExtensionCallExpression & ILNode::AsExtensionCallExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ExtensionCallExpression"),
                 "ILNode: using wrong node type!");
        return(*(ExtensionCallExpression *)this);
    }


    inline struct LambdaExpression &  ILNode::AsLambdaExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "LambdaExpression"),
                 "ILNode: using wrong node type!");
        return(*(LambdaExpression *)this);
    }
    inline struct UnboundLambdaExpression &  ILNode::AsUnboundLambdaExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "UnboundLambdaExpression"),
                 "ILNode: using wrong node type!");
        return(*(UnboundLambdaExpression *)this);
    }

    inline struct DeferredTempExpression &  ILNode::AsDeferredTempExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "DeferredTempExpression"),
                 "ILNode: using wrong node type!");
        return(*(DeferredTempExpression *)this);
    }

    inline struct AnonymousTypeExpression &  ILNode::AsAnonymousTypeExpression()
    {
        VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
        VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "AnonymousTypeExpression"),
                 "ILNode: using wrong node type!");
        return(*(AnonymousTypeExpression *)this);
    }

    inline struct ArrayLiteralExpression & ILNode::AsArrayLiteralExpression()
    {
	    VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
	    VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ArrayLiteralExpression"),
	             "ILNode: using wrong node type!");
	    return(*(ArrayLiteralExpression *)this);        
    }

    inline struct NestedArrayLiteralExpression & ILNode::AsNestedArrayLiteralExpression()
    {
	    VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
	    VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "NestedArrayLiteralExpression"),
	             "ILNode: using wrong node type!");
	    return(*(NestedArrayLiteralExpression *)this);                
    }

    inline struct ColInitExpression & ILNode::AsColInitExpression()
    {
	    VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
	    VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ColInitExpression"),
	             "ILNode: using wrong node type!");
	    return(*(ColInitExpression *)this);                        
    }


    inline struct ColInitElementExpression & ILNode::AsColInitElementExpression()
    {
	    VSASSERT(this->IsExprNode(), "ILNode: using wrong node type!");
	    VSASSERT(!strcmp(s_szBiltreeWhichSubStruct[this->bilop], "ColInitElementExpression"),
	             "ILNode: using wrong node type!");
	    return(*(ColInitElementExpression *)this);                                
    }
    
	/*****************************************************************************/
	/* Functions getting info about trees -- based on BILTREE tables
	/*****************************************************************************/

	inline SXKIND Sxkind(BILOP bilop)
	{
	    if (bilop >= 0 && bilop < _countof(ILNode::s_sxkindTable))
	    {
	        return (SXKIND) ILNode::s_sxkindTable[bilop];
	    }

	    return SXK_NONE;
	}

	inline SXKIND ILNode::Sxkind()
	{
	    if (bilop >= 0 && bilop < _countof(ILNode::s_sxkindTable))
	    {
	        return (SXKIND) ILNode::s_sxkindTable[bilop];
	    }

	    return SXK_NONE;
	}

	inline bool BilopIsConst(BILOP bilop)
	{
	    return !!(Sxkind(bilop) & SXK_CONST);
	}

	inline void ILNode::PropagateLocFrom(ILNode *pILNode)
	{
	    // propagate location info
	    this->Loc = pILNode->Loc;
	}

#if DEBUG

	inline const char * BilopName(BILOP bilop)
	{
	    VSASSERT(bilop < SX_COUNT, "");

	    // The array is sized such that BILOP enum values are valid indices
#pragma warning(disable:26000)
	    return ILNode::s_szBilopNames[(UINT)bilop];
#pragma warning(default:26000)


	}

#endif // DEBUG

#pragma warning(pop)
}

