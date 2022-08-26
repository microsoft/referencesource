//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Declares the interface to the VB compiler scanner.
//
//-------------------------------------------------------------------------------------------------

#pragma once

struct Token;
enum typeChars;

// URT imposed limit.  This limit should be removed for the next version.
const unsigned MaxIdentifierLength = 1023;

//inline char utilities
inline
bool IsLiteralDigit(WCHAR c)
{
    return ((c >= '0' && c <= '9') ||
             (c >= MAKEFULLWIDTH('0') && c <= MAKEFULLWIDTH('9')));
}

inline
bool IsHexDigit(WCHAR c)
{
    return ( IsLiteralDigit(c) ||
             (c >= 'a' && c <= 'f') ||
             (c >= 'A' && c <= 'F') ||
             (c >= MAKEFULLWIDTH('a') && c <= MAKEFULLWIDTH('f')) ||
             (c >= MAKEFULLWIDTH('A') && c <= MAKEFULLWIDTH('F')));
}

// Return the integral value of a character in an integral literal.
static
int IntegralLiteralCharacterValue(WCHAR LiteralCharacter)
{
    WCHAR Digit = MAKEHALFWIDTH(LiteralCharacter) & 0xff;

    if (IsLiteralDigit(Digit))
    {
        return Digit - '0';
    }
    else if (Digit >= 'A' && Digit <= 'F')
    {
        return 10 + (Digit - 'A');
    }
    else
    {
        VSASSERT(Digit >= 'a' && Digit <= 'f', "Surprising digit.");
        return 10 + (Digit - 'a');
    }
}

enum MethodDeclKind
{
    NormalKind,
    AsyncKind,
    IteratorKind,
    NoneKind
};


//-------------------------------------------------------------------------------------------------
//
// Lexcial States
//
enum LexicalState
{
    /* Primary States
     These are packed into the scanner state in 4 bits.  Because enums are signed integers we
     start the enum range out at -8 and go up from there.

     The reason that it matters that enums are signed is because we want to be able to utilize
     the topmost (sign) bit, which will always be zero for positive numbers and thus
     u----ilized if the enum just uses positive numbers.  The reason 8 (1000) can't be used as
     an enum value to utilize the topmost bit is because as a signed 4 bit number, 1000 means -8
     So comparisons of 1000 as a 4-bit signed value against an integer value of 8 don't work out
     because it would boil down to if ( -8 == 8 ) which would always fail. */

    VB = -8,
    VBImplicitLineContinuation, // indicates that the line ended with a token that implies a line continuation after it, e.g. dim i = foo.  <- '.' implies line continuation
    LastVBState = VBImplicitLineContinuation,
    XmlMarkup = LastVBState + 1,
    XmlElement,
    XmlAttributeValue,
    XmlSingleQuoteString,
    XmlDoubleQuoteString,
    XmlSingleSmartQuoteString,
    XmlDoubleSmartQuoteString,
    XmlContent,
    XmlCData,
    XmlComment,
    XmlPI,
    XmlPIData,
    XmlMisc
};

// The scanner needs some context when scanning certain kinds of lines so we know whether to interpret
// '<' as XML or an attribute.
enum LineType 
{ 
    // These are packed into the scanner state in two bits.  Because enums are signed ints, we start with
    // -2 and work up from there.  See notes on the LexicalState enum for why this signed business is important

    FunctionDecl = -2, // XML literals are highly restricted if the line we are returning is a function declaration
    ImportStatement = -1, // The line we are scanning looks like an imports statement 
    NotClassified = 0, // It's an 'anything goes' line - we'll make no assumptions
    CaseStatement // the line we are scanning looks like a CASE selector statement
};

class ScannerState
{
#define TOKEN_BITS 9
#define TOKEN_BITMASK (0xffff << BITSFORTOKEN)
#define PARAMNESTING_BITS 7
#define PARAMNESTING_BITMASK (~TOKENBITMASK)
public:
    // This data structure must fit into 32 bits as it is passed as a Long value in some IDE interfaces
    union 
    {
        // Sadly C++ doesn't allow us to lay out a union into a bitfield and will start the union on the closest
        // word boundary, so we have to union all fields.
        struct
        {
            unsigned m_IsSimpleState:1; // Simple state means we can interpret the fields in the struct / false means the state fields shouldn't be interpreted as the current value is a pointer value instead
            LexicalState m_LexicalState:4;
            unsigned m_IsXmlError:1;
            unsigned m_IsDocument:1;
            unsigned m_InDTD:1;
            unsigned m_ScannedElement:1; // we have completed scanning an xml element, <xml></xml>
            unsigned m_IsEndElement:1; // end of an XML element </xml>
            unsigned m_ExplicitLineContinuation:1; // indicates an explicit line continuation occurred, e.g. '_'
            unsigned m_IsSimpleQuery:1; // we've seen 'Aggregate' or 'From' on this logical line
            unsigned m_IsRParensOfLambdaParameter:1; // Flags when XML is legal following a lambda decl. Tracks the closing Rparen, e.g. Sub foo() <- last rparen marked m_IsRParensOfLambdaParameter=true
            LineType m_LineType:2; // The type of line we are scanning (Function declaration, Imports statement, Case selector)
            unsigned m_Data:16;
        };
        struct
        {
            unsigned m_IsSimpleState:1;
            LexicalState m_LexicalState:4;
            unsigned m_IsXmlError:1;
            unsigned m_IsDocument:1;
            unsigned m_InDTD:1;
            unsigned m_ScannedElement:1;
            unsigned m_IsEndElement:1;
            unsigned m_ExplicitLineContinuation:1; // indicates than an explicit line continuation occurred, e.g. '_'
            unsigned m_IsSimpleQuery:1; // we've seen 'Aggregate' or 'From' on this logical line
            unsigned m_IsRParensOfLambdaParameter:1;
            LineType m_LineType:2; // The type of line we are scanning (Func decl, imports, case selector)
            unsigned m_Token:TOKEN_BITS;
            unsigned m_ParamNesting:PARAMNESTING_BITS;
        };
    };

    unsigned IncrementOpenXmlElements() { return ++m_Data; }
    unsigned DecrementOpenXmlElements() { return --m_Data; }
    unsigned OpenXmlElements() const { return m_Data; }
    void SetOpenXmlElements(unsigned i) { m_Data = i;}

    tokens TokenTypeBeforeLineContinuation() const {return (tokens)m_Token; }
    void SetTokenTypeBeforeLineContinuation(tokens TokenType)
    {
        VSASSERT(TokenType >= 0 && TokenType < tkCount, "Invalid token being set, it doesn't fit in the size allocated in m_Data which implies it is bigger than tkCount");
        m_Token = TokenType;
    }
    void SetTokenTypeBeforeLineContinuation(_In_ Token *t);

    unsigned LambdaParameterNesting()
    {
        return m_ParamNesting;
    }
    bool SetLambdaParameterNesting(unsigned nestingLevel)
    {
        // Will return true if it succesfully set it, will return false if it overflows
        // so caller can report an error.
        if (nestingLevel < (1 << PARAMNESTING_BITS))
        {
            m_ParamNesting = nestingLevel;
            return true;
        }
        return false;
    }

    long AsLong() {return *reinterpret_cast<long*>(this);}
    bool IsVBState() {return IsVBState(m_LexicalState);}
    bool IsVBImplicitLineContinuationState() {return m_LexicalState == VBImplicitLineContinuation;}
    bool IsXmlState() {return IsXmlState(m_LexicalState);}
    static bool IsVBState(LexicalState State) {return State <= LastVBState;}
    static bool IsXmlState(LexicalState State) {return State > LastVBState;}


    ScannerState& Init(LexicalState State)
    {
        // Things we initialize regardless of the new state
        m_IsSimpleState = true;
        m_LexicalState = State;
        m_IsXmlError = false;
        m_IsDocument = false;
        m_InDTD = false;
        m_ScannedElement = false;
        m_IsEndElement = false;
        m_ExplicitLineContinuation = false;
        m_IsSimpleQuery = false;
        m_IsRParensOfLambdaParameter = false;

        VSASSERT(sizeof(ScannerState) <= sizeof(DWORD), "This struct gets cast as DWORD's, so please make sure it won't be bigger.");

        switch ( State )
        {
            case VB:
                // 16 bits are shared between token type and lambda parameter nesting, 9 bits are used to store the token type,
                // and 7 bits are used to store the lambda nesting.
                VSASSERT(tkCount >= 0 && tkCount < (1 << TOKEN_BITS), 
                    "We have too many tokens now to fit in the 5 bits, we will need to get more space");

                SetTokenTypeBeforeLineContinuation(tkNone);
                SetLambdaParameterNesting(0);
                m_LineType = NotClassified;
                break;

            case VBImplicitLineContinuation:
                break; // allow the token that ended the last line, m_LineType, and the lambda param nesting level, to retain their values

            default:

                // For everything else (typically just the XML states)
                SetOpenXmlElements(0);
                break;
        } // switch ( State )

        return *this;
    }

    // Puts the scanner into VBImplicitLineContinuation state and remembers
    // the last token we saw before the line ended.
    ScannerState& BeginVBContinuationState(
        tokens ContinuationToken
    )
    {
        m_LexicalState = VBImplicitLineContinuation;
        SetTokenTypeBeforeLineContinuation( ContinuationToken );
        return *this;
    }

    ScannerState & EndVBContinuationState()
    {
        m_LexicalState = VB;
        m_LineType = NotClassified;
        SetTokenTypeBeforeLineContinuation( tkNone );
        return *this;
    }

    // Use when starting to scan Xml from VB
    ScannerState& BeginXmlState(LexicalState State)
    {
        VSASSERT(IsXmlState(State), "Must be an Xml state");

        ClearXmlState();
        m_LexicalState = State;
        return *this;
    }

    // Use when returning to VB after completing Xml scan
    ScannerState& EndXmlState()
    {
        ClearXmlState();
        m_LexicalState = VB;
        return *this;
    }

    void ClearXmlState()
    {
        m_IsXmlError = false;
        m_IsDocument = false;
        m_InDTD = false;
        m_ScannedElement = false;
        m_IsEndElement = false;
        m_ExplicitLineContinuation = false;
        SetOpenXmlElements(0);
    }
}; // class ScannerState 


// Helpers to identify non-reserved word tokens
tokens IdentifierAsKeyword(_In_ Token *T, Compiler *Compiler);
tokens IdentifierAsKeyword(_In_ Token *T, _In_ StringPool* pStringPool);
tokens TokenAsKeyword(_In_ Token *T, Compiler *Compiler);
tokens TokenAsKeyword(_In_ Token *T, _In_ StringPool* pStringPool);

//
// Token
//
// This class represents a token in the token stream
//

struct Token
{
    //
    // Informational functions
    //

    // Is this token a valid end of line?
    bool IsEndOfLine() const
    {
        return m_TokenType == tkEOL || m_TokenType == tkEOF;
    }

    bool IsContinuableEOL()
    {
        return m_TokenType == tkEOL && ! m_EOL.m_isFollowedByBlankLine;
    }

    // Is this token the end of the parse stream?
    // Microsoft: Consider - shouldn't this also consider tkNONE the end of the stream?  When we parse
    // buffers that we conjure up (as is done in vberrorfixgen.cpp) we don't seem to get a TEOF, but
    // rather get a tkNONE at the end of the line and if you don't stop there, you just end up in
    // an infinite loop in those places that fail to get the next line.f 
    bool IsEndOfParse() const
    {
        return m_TokenType == tkEOF || m_TokenType == tkNone;
    }

    // Is this token a keyword?
    bool IsKeyword() const
    {
        return IsKeyword(m_TokenType);
    }

    static bool IsKeyword(tokens TokenType) 
    {
        return TokenType <= tkKwdIdLast && TokenType != tkREM;
    }

    bool Is7to8NewKeyword();

    // Is this token a delimiter?
    bool IsDelimiter() const
    {
        return m_TokenType >= tkBang;
    }

    bool IsQueryKeyword( _In_ StringPool* pStringPool)
    {
        return g_tkKwdDescs[TokenAsKeyword(this,pStringPool)].kdIsQuerykwd;
    }

    bool IsLCUpperBoundKeyword(_In_ StringPool* pStringPool)
    {
        return g_tkKwdDescs[TokenAsKeyword(this,pStringPool)].kdIsLCUpperBound;
    }

    bool CanContinueAtEnd( _In_ StringPool* pStringPool)
    {
        return g_tkKwdDescs[TokenAsKeyword(this, pStringPool)].kdCanContinueAtEnd;
    }

    bool CanContinueAtStart( _In_ StringPool* pStringPool)
    {
        return g_tkKwdDescs[TokenAsKeyword(this, pStringPool)].kdCanContinueAtStart;
    }


    // Returns the non reserved token type of the token
    tokens NonReservedTokenType ()
    {
        if (m_TokenType == tkID)
        {
            return TokenOfString(m_Id.m_Spelling);
        }
        else
            return m_TokenType;
    }

    // Get the location of the Token
    Location GetLocation() const
    {
        long end = m_StartColumn + m_Width;
        if ( m_Width > 0 )
        {
            end -=1;
        }

        Location loc;
        loc.SetLocation(
            m_StartLine, 
            m_StartColumn,
            m_StartLine,
            end);
        return loc;
    }

    Location GetLocationFullWidth() const
    {
        Location loc;
        loc.SetLocation(
            m_StartLine,
            m_StartColumn,
            m_StartLine,
            m_StartColumn + m_Width);
        return loc;
    }

    // Remove the identifier type character from the location
    Location GetLocationExcludeTypeIdChar() const;

    //
    // Token data
    //

    union
    {
        // NOTE: m_DummyFirstField needs to be the first field in struct Token.
        // The COMPILE_ASSERT at the end of this struct that uses m_DummyFirstField
        // enfores this.
        //
        // This field is used for compile time asserts to ensure that
        // there are no virtual methods in this struct. This is important
        // so that the size of this extremely instantiated struct is not
        // unknowingly increased.
        //
        // Additionally this field is also used to figure out the start of
        // this struct in order to memzero out the struct correctly even in
        // case virtual methods are introduced to this struct in the future
        // for any reason at all.
        //
        // The pragma suppresses the warning given when allocating an array
        // of constant size 0.
        //
        #pragma warning (suppress  : 4200)
        void *m_DummyFirstField[0];

        tokens m_TokenType;
    };

    ScannerState m_State;

    // m_StartLine is the number of the line containing the Token.
    long m_StartLine;

    // m_StartCharacterPosition is the absolute character position (counting from
    // zero from the start of the input stream) on which the Token begins.
    long m_StartCharacterPosition;

    // m_StartColumn is the column (counting from zero) on which the Token begins.
    long m_StartColumn;

    // m_Width is the number of characters that make up the Token.
    long m_Width;

    enum LiteralBase
    {
        Decimal = 10,
        Hexadecimal = 16,
        Octal = 8
    };

    enum XmlCharTypes
    {
        XmlCharData,
        XmlDecCharRef,
        XmlHexCharRef
    };

    union
    {

        // For error tokens
        struct
        {
            ERRID m_errid;

            // Is this token for the line continuation character '_'
            // that occurs without a preceding white space or with
            // following text that does not make it a valid identifier
            // either.
            //
            bool m_IsTokenForInvalidLineContinuationChar;
        } m_Error;

        // For identifier and keyword tokens
        struct
        {
            bool m_IsBracketed;

            STRING *m_Spelling;

            typeChars m_TypeCharacter;
        } m_Id;

        // For integral literals

        struct
        {
            LiteralBase m_Base;

            __int64 m_Value;
            typeChars m_TypeCharacter;
        } m_IntLiteral;

        // For Decimal literals

        struct
        {
            DECIMAL m_Value;
            typeChars m_TypeCharacter;
        } m_DecimalLiteral;

        // For floating literals

        struct
        {
            double m_Value;
            typeChars m_TypeCharacter;
        } m_FloatLiteral;

        // For String literals

        struct
        {
            size_t m_Length;
            const WCHAR *m_Value;
#if IDE 
            bool m_HasUnReportedUnterminatedStringError;
#endif
        } m_StringLiteral;

        // For Char literals

        struct
        {
            WCHAR m_Value;

        } m_CharLiteral;

        // For date literals

        struct
        {
            __int64 m_Value;
        } m_DateLiteral;

        // For comments

        struct
        {
            size_t m_Length;
            const WCHAR *m_Spelling;
            bool m_IsRem : 1;
        } m_Comment;

        // For EOL tokens

        struct
        {
            bool m_NextLineAlreadyScanned; // This is how we now whether somebody has been peeking forward and thus whether we should ask the scanner to scan the text for the next line or just return the tokens for it
            bool m_isFollowedByBlankLine; 
        } m_EOL;

        struct
        {
            size_t m_Length;
            const WCHAR *m_Value;
            unsigned m_IsNewLine:1;
            unsigned m_IsWhiteSpace:1;
            unsigned m_Type:3;
        } m_XmlCharData;

    };

    // m_Next points to the next Token in the list/ring.
    Token *m_Next;
    // m_Prev points to the previous Token in the list/ring.
    Token *m_Prev;
};

// Compile assert to ensure that there are no virtual methods in struct Token
// and that m_DummyFirstField is the first field in struct Token.
COMPILE_ASSERT(FIELD_OFFSET(struct Token, m_DummyFirstField) == 0);

// Because of Xml Literals with expressions holes, the scanner is no longer a simple lexical scanner but a mini parser.  It must
// be restartable for the colorizer, so a stack has been added to keep track of the possible nested states.

class StateStack
{
public:
    NEW_CTOR_SAFE()

    Stack<ScannerState> m_Stack;

    StateStack() { } 
    ~StateStack() { 
        m_Stack.Destroy();
    }
    StateStack *Clone() 
    { 
        StateStack *Result = new StateStack();
        m_Stack.CopyTo(&Result->m_Stack); 
        return Result;
    }
    ScannerState Top() {
        return m_Stack.Top();
    }
};

class ColorizerState : ZeroInit<ColorizerState>
{
public:
    union
    {
        ScannerState m_ScannerState;

        StateStack *m_Stack;

        long        m_Long;
    };

    ColorizerState(long State)
    {
        // Clone stack if one exists
        Init(State);
    }

    ColorizerState(_In_ StateStack *Stack)
    {
        // Clone stack
        m_Stack = Stack->Clone();
    }

    ColorizerState(ScannerState State)
    {
        m_ScannerState = State;
    }

    ColorizerState(_In_ const ColorizerState& State)
    {
        Init(State);
    }

    ~ColorizerState();

    ColorizerState& operator = (long State) { 
        return Assign(State); 
    }

    ColorizerState& operator = (const ColorizerState& State) {
        return Assign(State); 
    }

    // AsLong transfers ownership of the memory, if any, in the ColorizerState
    // Null out stack so it isn't freed when the destructor is called.
    long AsLong()
    {
        long Result = m_Long;
        m_Stack = NULL;
        return Result;
    }

    bool IsSimpleState() {
        return m_ScannerState.m_IsSimpleState;
    }
#if IDE 
    bool HasLineContinuation()
    {
        ScannerState State;

        if (!IsSimpleState())
        {
            State = m_Stack->Top();
        }
        else
        {
            State = m_ScannerState;
        }

        if (State.IsXmlState())
            return true;

        return State.m_ExplicitLineContinuation;
    }

    static StateStack *ToStateStack(long State) 
    {
        if ((State & 0x01) != 0) 
        {
            return NULL; 
        }
    
        return (StateStack *)((void *)(size_t)State);
    }
    static bool LineEndsInsideXml(long State)
    {
        if (ToStateStack(State) != NULL)
            return true;
        ColorizerState cs(State);
        return cs.m_ScannerState.IsXmlState();
    }
    static bool LineEndsInXmlMisc(long State)
    {
        if (ToStateStack(State) != NULL) 
            return false;
        ColorizerState cs(State);
        return cs.m_ScannerState.m_LexicalState == XmlMisc && cs.m_ScannerState.m_ScannedElement;
    }
    static bool LineEndsInEmbeddedExpression(long State)
    {
        return ToStateStack(State) != NULL;
    }
    static bool HasLineContinuation(long State)
    {
        ColorizerState cs(State);

        return cs.HasLineContinuation();
    }
#endif

private:
    void Init(long State);


    void Init(_In_ const ColorizerState& ColorState)
    {
        this->m_ScannerState = ColorState.m_ScannerState;

        if (!ColorState.m_ScannerState.m_IsSimpleState)
        {
            ColorizerState *s = const_cast<ColorizerState*>(&ColorState);
            s->m_Stack = NULL;
        }
    }

    // Assign always transfers ownership of memory
    ColorizerState& Assign(long State)
    {
        ColorizerState NewState(State);
        *this = NewState;
        return *this;
    }

    ColorizerState& Assign(const ColorizerState& State);
};


//
// Scanner
//
// This class represents the stream of tokens
//

class Scanner
{

public:

    //
    // Constructor and destructor
    //

    Scanner(
        Compiler *pCompiler,
        _In_opt_count_(InputStreamLength) const WCHAR *InputStream,
        size_t InputStreamLength,
        long InitialAbsOffset,
        long InitialLineNumber,
        long InitialColumnNumber,
        MethodDeclKind InitialMethodDeclKind = NormalKind);

    Scanner(
        _In_ StringPool* pStringPool,
        _In_opt_count_(InputStreamLength) const WCHAR *InputStream,
        size_t InputStreamLength,
        long InitialAbsOffset,
        long InitialLineNumber,
        long InitialColumnNumber,
        MethodDeclKind InitialMethodDeclKind = NormalKind);

    Scanner(
        _In_ StringPool *pStringPool,
        _In_ const WCHAR *pInputStream,
        _In_ const WCHAR *pEndOfStream,
        long InitialAbsOffset,
        long InitialLineNumber,
        long InitialColumnNumber,
        MethodDeclKind InitialMethodDeclKind = NormalKind);

    ~Scanner ();

    //
    // Useful routines for functions outside of Scanner
    //

    static bool IsIdentifier(_In_z_ WCHAR *Spelling);
    static bool IsWhitespaceCharacter (WCHAR c);
    static bool IsPeriodCharacter (WCHAR c);
    static bool IsLeftParenCharacter (WCHAR c);
    static bool IsRightParenCharacter (WCHAR c);
    static bool IsLeftBraceCharacter (WCHAR c);
    static bool IsEqualsCharacter (WCHAR c);
    static bool IsGreatThanCharacter (WCHAR c);
    static bool IsLessThanCharacter (WCHAR c);
    static bool IsCommaCharacter (WCHAR c);
    static bool IsNewlineCharacter (WCHAR c);
    static bool IsDateSeparatorCharacter (WCHAR c);
#if 0   // Not used
    static bool IsDigit (WCHAR c);
#endif
    static bool IsValidIdentiferOrBracketedIdentifier(_In_z_ WCHAR *Spelling);
    static bool IsSingleQuote(WCHAR c);

    //
    // Stream reading functions
    //

    // Returns whether the stream is at the end of input
    bool NotAtEndOfInput()
    {
        return m_InputStreamPosition < m_InputStreamEnd;
    }

    // Copy a string from the text into the buffer.
    void CopyString(
        unsigned Start,
        unsigned End,
        _Out_capcount_((End - Start)) WCHAR *Buffer);

    void Scanner::SkipToNextConditionalLine();

    // GetNextLine starting with a specific state
    Token *GetNextLine (_In_ ColorizerState& State, bool ResetFirstToken, bool AllowXmlFeatures = true);

    // GetNextLine but start scanning in the mode specified by useThisLineType
    Token *Scanner::GetNextLine(LineType useThisLineType);

    // GetNextLine returns a list of all the tokens for the next line.
    Token *GetNextLine (tokens TokenType = tkNone);

    Token *  Scanner::GetAllLines();
    
    // Get scanner state for colorizer.  The caller takes ownership for the StateStack memory.
    ColorizerState GetState();

    // Get scanner state for at a particular token location.  The caller takes ownership for the StateStack memory.
    ColorizerState GetState(_In_ Token *T);

    //
    // Line marking functions that allow for rollback
    //

    struct LineMarker
    {
        long m_LineNumber;
        const WCHAR *m_LineStart;
        long m_CharacterPositionOfCurrentClusterStart;
    };

    // MarkLine returns a marker for the next line to be read.
    LineMarker MarkLine ();

    // ResetToLine resets to the line marker
    void ResetToLine (LineMarker lm);

    // AbandonTokens discards all tokens.
    void AbandonTokens ();

    // AbandonTokensBefore discards all tokens preceding T.
    void AbandonTokensBefore (Token *T);

    // AbandonTokens discards all tokens after and including T.
    void AbandonTokens(_In_ Token *T);

    // Get the last token that was created. This token must be on the same physical line.
    Token * GetLastToken() const;

    // Get the last token type that was created on the same *logical* line.
    tokens GetLastTokenType();

    Token * GetPrevToken(_In_ Token *t) {
        return t != m_FirstTokenOfLine ? t->m_Prev : NULL;
    }

    // Get a pointer to the token's text
    const WCHAR *TokenToStreamPosition(Token *T) {
        return m_InputStream + T->m_StartCharacterPosition - m_CharacterPositionOfCurrentClusterStart;
    }

    const WCHAR * GetInputStreamEnd() {
        return m_InputStreamEnd;
    }

    long GetLineNumber() { return m_LineNumber; }
    

   MethodDeclKind* GetMethodDeclKind(){ return &m_MethodDeclKind;}

   void SetMethodDeclKind(MethodDeclKind methodDeclKind){ m_MethodDeclKind = methodDeclKind;}

protected:

    
    //
    // Data members
    //

    const WCHAR *m_InputStream;
    const WCHAR *m_InputStreamEnd;
    const WCHAR *m_InputStreamPosition;

    long m_CharacterPositionOfCurrentClusterStart;

    long m_LineNumber;
    const WCHAR *m_LineStart;

    StringPool* m_pStringPool;
    NorlsAllocator m_Storage;

    // A Scanner keeps track of Tokens in a ring, and allocates Tokens as necessary
    // to prevent reusing an in-use Token.
    Token *m_FirstFreeToken;
    Token *m_FirstInUseToken;

    Token *m_FirstTokenOfLine;

    StateStack *m_PendingStates;
    ScannerState m_State;
    bool m_AllowXmlFeatures;

    // m_SeenAsyncOrIteratorInCurrentLine keeps track all Async or Iterator
    // keyword in the current line. During scanning XML, multiline statement 
    // lambda will be scanned in a single call of ScanVBLine(). Therefore, 
    // a stack is needed to track all Async or Iterator keywords.
    // Bug Dev11 157250 can repro if not using stack.
    Stack<MethodDeclKind> m_SeenAsyncOrIteratorInCurrentLine;

    MethodDeclKind m_MethodDeclKind;

    //
    // Private routines
    //

    // GetNextLineHelper returns a list of all the tokens for the next line.
    Token *GetNextLineHelper ();


    // Advance characters in the stream
    void AdvanceChars (long AdvanceCount)
    {
        m_InputStreamPosition += AdvanceCount;
    }

    // Return next character and advance position
    WCHAR ReadNextChar ()
    {
        return *(m_InputStreamPosition++);
    }

    // Return next character and don't advance position
    WCHAR PeekNextChar ()
    {
        return *m_InputStreamPosition;
    }

    // Return n'th next character and don't adance position
    WCHAR PeekAheadChar (_In_ unsigned uAheadCount)
    {
        // See note below for dev10 654336 in NotCloseToEndOfInput about to know when you are at the end of the buffer.
        VSASSERT( NotCloseToEndOfInput( uAheadCount ), "Peeking past the end of the buffer is probably not what you wanted" );
        return *(m_InputStreamPosition + uAheadCount);
    }

    // Check to see if WithinChars characters (including the character at
    // InputStreamPosition) are available in the input stream.
    bool NotCloseToEndOfInput (unsigned WithinChars)
    {
     /* Microsoft: dev10 654336 
        We need to check for m_InputStreamPosition < m_InputStreamEnd because
        of how the buffer pointers are determined in the scanner.  m_InputStreamEnd is determined by: 
        m_InputStreamEnd = InputStream + InputStreamLength in the scanner’s ctor.  So this means that m_InputStreamEnd 
        always points 1 memory location past the last valid byte in the buffer.  E.g.  Let’s say we have a buffer of “HI”
        Let’s say that our InputStream address is 0.  Given a length of two for “HI” (assume ansi characters) this gives
        us a m_InputStreamEnd of 2 (0+2=2).  

        Slot
        0 1 2
        H I x

        2 is actually pointing 1 past the end of the buffer.
        So checks against whether we are at the end of buffer should be ‘<’ m_InputStreamEnd instead of ‘<=’.  
        For posterity, this function precipitated access violations on a couple files in the Maui test which we hit because
        the line continuation work was asking questions about the line following the last line of the file. */
        return (m_InputStreamPosition + WithinChars) < m_InputStreamEnd;
    }

    void SetStateStack(_In_ StateStack *Stack) {
        delete m_PendingStates; m_PendingStates = Stack;
    }

    // Pops the next state from the states stack
    void PopState();

    // Pushes the current state on the states stack
    void PushState();

    // Are there pending scnner states?
    bool PendingStates() {
        return m_PendingStates && !m_PendingStates->m_Stack.Empty();
    }

    // PrecededByNonWhitespace returns true if the current character not the first
    // character on a line and is not immediately preceded by whitespace, and returns false
    // otherwise.
    bool PrecededByNonWhitespace ();

    // TryFollowingEquals produces a token for a character that can optionally be followed
    // by an '=' character, and advances m_InputStreamPosition to point at the first
    // character following the character(s) consumed to produce the token.
    void
    TryFollowingEquals (
        tokens DefaultToken,
        tokens AlternativeToken,
        _In_ const WCHAR *Start);

    void
    TryFollowingEquals (
        tokens OneCharToken,
        tokens AlternativeToken);

    // TryTwoFollowingAlternatives produces a token for a character that can optionally be
    // followed by two different characters, and advances m_InputStreamPosition to
    // point at the first character following the character(s) consumed to produce the
    // token.
    void
    TryTwoFollowingAlternatives (
        tokens one_char_token,
        WCHAR alternative_char1,
        tokens alternative_token1,
        WCHAR alternative_char2,
        tokens alternative_token2);

    // NextFreeToken returns the next not-in-use token, allocating one if necessary.
    Token *NextFreeToken ();

    // MakeToken fills in the common fields of the next not-in-use token. It assumes that
    // the token begins at m_wchInputStreamPosition.
    Token *
    MakeToken (
        tokens TokenType,
        size_t TokenWidth);

    Token *
    MakeStringLiteralToken (
        _In_ const WCHAR *p_wchTermination,
        unsigned uEscapedQuoteCount,
        unsigned uClosingQuoteCount);

    Token *
    MakeXmlNameToken(
        tokens TokenType,
        size_t TokenWidth);

    Token *
    MakeXmlNameToken(
        tokens TokenType,
        _In_z_ STRING *IdString);

    Token *
    Scanner::MakeXmlCharToken
    (
        tokens TokenType,
        size_t TokenWidth,
        bool IsAllWhitespace = false);

    Token *
    MakeXmlWhiteSpaceToken(
        size_t TokenWidth,
        bool IsNewLine = false);

    Token *
    MakeXmlReferenceToken(
        size_t TokenWidth,
        _In_ const WCHAR *Value,
        Token::XmlCharTypes);

    // The Eat... methods consume characters but produce no tokens. All assume that
    // *m_wchInputStreamPosition begins the sequence they eat, and leave
    // m_wchInputStreamPosition pointing at the first character that follows the eaten
    // sequence.
    void EatNewline (WCHAR wchStartCharacter );
    void EatWhitespace ();
    void EatLineContinuation();
    void EatThroughLogicalNewline ();

    unsigned  FindEndOfWhitespace(unsigned peekPos);
    void CheckForAnotherBlankLine(Token * pToken);

    // The Scan... methods consume characters and produce one or more tokens. All assume
    // that the token begins at *m_wchInputStreamPosition, and that the first character
    // is a valid beginning for the token. All update m_wchInputStreamPosition to point at
    // the first character following the scanned character(s).
    Token *ScanVBLine ();
    void ScanComment (_In_ const WCHAR *p_wchPosition, bool fRem);
    void ScanIdentifier ();
    void ScanBracketedIdentifier ();
    void ScanXmlAttributeIdentifier ();
    void ScanXmlAttributeNCName ();
    void ScanBracketedXmlQualifiedName ();
    void ScanGetXmlNamespace();
    void ScanNumericLiteral ();
    void ScanStringLiteral ();
    bool ScanIntLiteral(__int64 *IntegerValue, _Inout_ const WCHAR **CurrentPosition);
    // ScanDateLiteral returns true if the scan of a date literal succeeded, and false
    // otherwise.
    bool ScanDateLiteral ();
    void ScanLeftAngleBracket ();
    void ScanRightAngleBracket ();

    // IsLineContinuation returns true if an '_' is to be treated as a continuation
    // character (because it is followed by optional whitespace and a newline), and false
    // otherwise.
    bool IsLineContinuation ();
    
    // LineAllowsImplicitLineContinuation() is called when we are on an EOL to see if the
    // last token we scanned implies a line continuation or not.
    bool LineAllowsImplicitLineContinuation();

    // Looks at preceding token to determine whether scanner has entered into an Xml Literal.
    // Returns true if '<' should be intereperted as xml literal or false if '<' is a relational operator.
    bool XmlCanFollow(_In_opt_ Token *Prev);

    Token *CheckXmlForCallOrCodeAttribute();
    Token *CheckXmlForStatement();
    Token *SkipXmlWhiteSpace(_In_opt_ Token *t);

    // Xml Scanning helpers
    void ScanXmlMarkup();
    void ScanXmlMisc();
    void ScanXmlElement();
    void ScanXmlAttributeValue();
    void ScanXmlString(const WCHAR TerminatingChar, tokens TerminatingToken);
    void ScanXmlQName();
    void ScanXmlNCName();
    void ScanXmlReference();
    void ScanXmlCharRef();
    void ScanXmlWhiteSpace();
    void ScanXmlContent();
    void ScanXmlComment();
    void ScanXmlCData();
    void ScanXmlPI();
    void ScanXmlPIData();
    void ScanXmlLT(LexicalState State);
    void ScanXmlVBExpr(LexicalState State);
    const WCHAR * ScanXmlDefault(_In_ const WCHAR *Here, tokens TokenType);
#if 0
    bool LookAheadForKeyword();
#endif

private:

    // A Scanner cannot be copied. To prevent attempts to copy one from succeeding,
    // declare but do not define a private copy constructor and an assignment operator.
    Scanner (const Scanner &);
    Scanner & operator = (const Scanner &);
    void Init(
        _In_opt_count_(InputStreamLength) const WCHAR *InputStream,
        size_t InputStreamLength,
        long InitialAbsOffset, 
        long InitialLineNumber, 
        long InitialColumnNumber,
        MethodDeclKind InitialMethodDeclKind);
};

inline bool IsIdentifierCharacter(WCHAR c);
