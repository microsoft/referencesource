/*---------------------------------------------------------------

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2003.  Microsoft Corporation.  All rights reserved.

SafeInt.hpp

This header implements an integer handling class designed to catch
unsafe integer operations

This header compiles properly at warning level 4.

Please read the leading comments before using the class.

Version 1.0.3
---------------------------------------------------------------*/
#ifndef SAFEINT_HPP
#define SAFEINT_HPP

#include <assert.h>

/*
*  The SafeInt class is designed to have as low an overhead as possible
*  while still ensuring that all integer operations are conducted safely.
*  Nearly every operator has been overloaded, with a very few exceptions.
*
*  A usability-safety trade-off has been made to help ensure safety. This 
*  requires that every operation return either a SafeInt or a bool. If we 
*  allowed an operator to return a base integer type T, then the following 
*  can happen:
*  
*  char i = SafeInt<char>(32) * 2 + SafeInt<char>(16) * 4;
*
*  The * operators take precedence, get overloaded, return a char, and then 
*  you have:
*
*  char i = (char)64 + (char)64; //overflow!
*  
*  This situation would mean that safety would depend on usage, which isn't
*  acceptable. The problem that this leaves us with is that you'd like to be able 
*  to do something like:
*
*  void* ptr = malloc(SafeInt<unsigned short>(23) * SafeInt<unsigned short>(HowMany));
*
*  and have it be a safe operation. The way out of this is to use the following type of
*  construct:
*
*   SafeInt<int> s = 1, s1 = 2;
*  	int	m = (s | s1).Value();
*
*  A little clunky, and less programmer-friendly than would be ideal, but it is safe.
*
*  One key operator that is missing is an implicit cast. The reason for
*  this is that if there is an implicit cast operator, then we end up with
*  an ambiguous compile-time precedence. Because of this amiguity, there
*  are two methods that are provided:
*
*  SafeInt::Value() - returns the value of the object as an integer
*  SafeInt::Ptr()   - returns the address of the internal integer
*  Note - the '&' (address of) operator has been overloaded and returns
*         the address of the internal integer.
*
*  The SafeInt class should be used in any circumstances where ensuring
*  integrity of the calculations is more important than performance. See Performance
*  Notes below for additional information. 
*
*  Many of the conditionals will optimize out or be inlined for a release
*  build (especially with /Ox), but it does have significantly more overhead, 
*  especially for signed numbers. If you do not _require_ negative numbers, use 
*  unsigned integer types - certain types of problems cannot occur, and this class
*  performs most efficiently.
*
*  Here's an example of when the class should ideally be used -
*
*  void* AllocateMemForStructs(int StructSize, int HowMany)
*  {
*     SafeInt<unsigned long> s(StructSize);
*
*     s *= HowMany;
*
*     return malloc(s.Value());
*
*  }
*
*  Here's when it should NOT be used:
*
*  void foo()
*  {
*    int i;
*
*    for(i = 0; i < 0xffff; i++)
*      ....
*  }
*
*  Error handling - a SafeInt class will throw exceptions if something
*  objectionable happens. The exceptions are SafeIntException classes,
*  which contain one signed long as a code (for compatability with Windows
*  errors). The values that may be assigned currently are:
*
*  ERROR_ARITHMETIC_OVERFLOW
*  EXCEPTION_INT_DIVIDE_BY_ZERO
*
*  Typical usage might be:
*
*  bool foo()
*  {
*    SafeInt<unsigned long> s; //note that s == 0 unless set
*
*    try{
*      s *= 23;
*      ....
*    }
*    catch(SafeIntException err)
*    {
*       //handle errors here
*    }
*  }
*
*  Performance:
*
*  Due to the highly nested nature of this class, you can expect relatively poor
*  performance in unoptimized code. In tests of optimized code vs. correct inline checks
*  in native code, this class has been found to take approximately 8% more CPU time,
*  most of which is due to exception handling. Solutions:
*
*  1) Compile optimized code - /Ox is best, /O2 also performs well. Interestingly, /O1
*     (optimize for size) does not work as well. 
*  2) If that 8% hit is really a serious problem, walk through the code and inline the
*     exact same checks as the class uses.
*  3) Some operations are more difficult than others - avoid using signed integers, and if
*     possible keep them all the same size. 64-bit integers are also expensive. Mixing 
*     different integer sizes and types may prove expensive. Be aware that literals are
*     actually ints. For best performance, cast them to the type desired.
*
*
*  Binary Operators
*  
*  All of the binary operators have certain assumptions built into the class design. 
*  This is to ensure correctness. Notes on each class of operator follow:
*  
*  Arithmetic Operators (*,/,+,-,%)
*  There are three possible variants:
*  SafeInt<T> op SafeInt<T>
*  SafeInt<T> op U
*  U op SafeInt<T>
*  
*  The SafeInt<T> op SafeInt<U> variant is explicitly not supported, and if you try to do 
*  this the compiler with throw the following error:
*  
*  error C2593: 'operator *' is ambiguous
*  
*  This is because the arithmetic operators are required to return a SafeInt of some type. 
*  The compiler cannot know whether you'd prefer to get a type T or a type U returned. If 
*  you need to do this, you need to extract the value contained within one of the two using 
*  the SafeInt::Value() method. For example:
*  
*  SafeInt<T> t, result;
*  SafeInt<U> u;
*  
*  result = t * u.Value();
*  
*  Comparison Operators
*  Because each of these operators return type bool, mixing SafeInts of differing types is 
*  allowed.
*  
*  Shift Operators
*  Shift operators always return the type on the left hand side of the operator. Mixed type 
*  operations are allowed because the return type is always known.
*  
*  Boolean Operators
*  Like comparison operators, these overloads always return type bool, and mixed-type SafeInts 
*  are allowed. Additionally, specific overloads exist for type bool on both sides of the 
*  operator.
*  
*  Binary Operators
*  Mixed-type operations are discouraged, however some provision has been made in order to 
*  enable things like:
*  
*  SafeInt<char> c = 2;
*  
*  if(c & 0x02)
*    ...
*  
*  The "0x02" is actually an int, and it needs to work. The rule is that if the non-SafeInt type 
*  can be cast to the type of the SafeInt, and back to the original type without losing any 
*  significant bits then the operation is allowed.
*  
*  
*  Documented issues:
*
*  This header compiles correctly at /W4 using VC++ 7.1 (Version 13.10.3077). 
*  It is strongly recommended that any code doing integer manipulation be compiled at /W4 
*  - there are a number of warnings which pertain to integer manipulation enabled that are 
*  not enabled at /W3 (default for VC++)
*
*  Perf note - postfix operators are slightly more costly than prefix operators.
*  Unless you're actually assigning it to something, ++SafeInt is less expensive than SafeInt++
*
*  The comparison operator behavior in this class varies from the ANSI definition, which is 
*  arguably broken. As an example, consider the following:
*  
*  unsigned int l = 0xffffffff;
*  char c = -1;
*  
*  if(c == l)
*    printf("Why is -1 equal to 4 billion???\n");
*  
*  The problem here is that c gets cast to an int, now has a value of 0xffffffff, and then gets 
*  cast again to an unsigned int, losing the true value. This behavior is despite the fact that
*  an _int64 exists, and the following code will yield a different (and intuitively correct)
*  answer:
*  
*  if((_int64)c == (_int64)l))
*    printf("Why is -1 equal to 4 billion???\n");
*  else
*    printf("Why doesn't the compiler upcast to 64-bits when needed?\n");
*  
*  Note that combinations with smaller integers won't display the problem - if you 
*  changed "unsigned int" above to "unsigned short", you'd get the right answer.
*
*  If you prefer to retain the ANSI standard behavior insert 
*  #define ANSI_CONVERSIONS 
*  into your source. Behavior differences occur in the following cases:
*  8, 16, and 32-bit signed int, unsigned 32-bit int
*  any signed int, unsigned 64-bit int
*  Note - the signed int must be negative to show the problem
*  
*  
*  Revision history:
*
*  Oct 12, 2003 - Created
*  Author - David LeBlanc - dleblanc@microsoft.com
*
*  Oct 27, 2003 - fixed numerous items pointed out by michmarc and bdawson
*  Dec 28, 2003 - 1.0
*                 added support for mixed-type operations
*                 thanks to vikramh
*                 also fixed broken _int64 multiplication section
*                 added extended support for mixed-type operations where possible
*  Jan 28, 2004 - 1.0.1
*                 changed WCHAR to wchar_t
*                 fixed a construct in two mixed-type assignment overloads that was 
*                 not compiling on some compilers
*                 Also changed name of private method to comply with standards on 
*                 reserved names
*                 Thanks to Niels Dekker for the input
*  Feb 12, 2004 - 1.0.2
*                 Minor changes to remove dependency on Windows headers
*                 Consistently used _int16, _int32 and _int64 to ensure
*                 portability
*  May 10, 2003 - 1.0.3
*                 Corrected bug in one case of GreaterThan
*                 
*/

#pragma warning(push)
//this avoids warnings from the unary '-' operator being applied to unsigned numbers
//the overload itself resolves to nothing for the unsigned case
#pragma warning(disable:4146)
// conditional expression is constant - these are used intentionally
#pragma warning(disable:4127)

//use these if the compiler does not support _intXX
#ifdef NEEDS_INT_DEFINED
#define _int16 short
#define _int32 long
#define _int64 long long
#endif

/* catch these to handle errors
** Currently implemented code values:
** ERROR_ARITHMETIC_OVERFLOW
** EXCEPTION_INT_DIVIDE_BY_ZERO
*/

#ifndef ERROR_ARITHMETIC_OVERFLOW
#define ERROR_ARITHMETIC_OVERFLOW 534L
#endif

#ifndef EXCEPTION_INT_DIVIDE_BY_ZERO
#define EXCEPTION_INT_DIVIDE_BY_ZERO ((unsigned _int32)0xC0000094L)
#endif

class SafeIntException
{
public:
	SafeIntException(){m_code = 0;}
	SafeIntException(_int32 code)
	{
		m_code = code;
	}
	_int32 m_code;
};

template<typename T> class SafeInt
{
public:
	SafeInt()
	{
		m_int = 0;
	}

	//Having a constructor for every type of int 
	//avoids having the compiler evade our checks when doing implicit casts - 
	//e.g., SafeInt<char> s = 0x7fffffff;
	SafeInt(T i)
	{
		//always safe
		m_int = i;
	}

	//provide explicit boolean converter
	SafeInt(bool b)
	{
		m_int = b ? 1 : 0;
	}

	template <typename U> SafeInt(SafeInt<U> u)
	{
		*this = SafeInt<T>(u.Value());
	}

	template <typename U> SafeInt(U i)
	{
		//use signed-unsigned test on U
		if(SafeInt<U>::IsSigned())
		{
			//U is signed
			//whether T is signed or unsigned, must test range if sizeof T is smaller 
			//than sizeof U
			//if sizeof(T) >= sizeof(U) this optimizes out, only test for negative
			//for completely safe cases, optimizes to NOOP
			if(sizeof(T) < sizeof(U))
			{
				//test size
				if(i > (U)SafeInt<T>::MaxInt() || i < (U)SafeInt<T>::MinInt())
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
			}
			else //test +/- for sizeof(T) >= sizeof(U) and T unsigned
			if(!IsSigned())
			{
				if(i < 0)
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
			}
		}
		else
		{
			//U is unsigned
			//if sizeof T <= sizeof U AND T is signed,
			//test upper bound because MaxInt(unsigned) > MaxInt(signed)
			//OR
			//if sizeof T < sizeof U and T is unsigned
			if((IsSigned() && sizeof(T) <= sizeof(U)) ||
			(!IsSigned() && sizeof(T) < sizeof(U)) )
			{
				if(i > (U)MaxInt())
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
			}	
		}

		m_int = (T)i;
	}

	~SafeInt(){};

	//helpful methods
	//these compile down to something as efficient as macros and allow run-time testing 
	//of type by the developer

	template <typename U> static bool IsSigned(SafeInt<U>)
	{
		return( (U)-1 < 0 );
	}

	template <typename U> static bool IsSigned(U)
	{
		return( (U)-1 < 0 );
	}

	static bool IsSigned()
	{
		return( (T)-1 < 0 );
	}

	static unsigned char BitCount(){return (sizeof(T)*8);}
	template <typename U> static unsigned char BitCount(U){return (sizeof(U)*8);}

	static bool Is64Bit(){return sizeof(T) == 8;}
	static bool Is32Bit(){return sizeof(T) == 4;}
	static bool Is16Bit(){return sizeof(T) == 2;}
	static bool Is8Bit(){return sizeof(T) == 1;}

	template <typename U> static bool Is64Bit(U){return sizeof(U) == 8;}
	template <typename U> static bool Is32Bit(U){return sizeof(U) == 4;}
	template <typename U> static bool Is16Bit(U){return sizeof(U) == 2;}
	template <typename U> static bool Is8Bit(U){return sizeof(U) == 1;}

	//both of the following should optimize away
	static T MaxInt()
	{
		if(IsSigned())
		{
			return (T)~((T)1 << (BitCount()-1));
		}
		//else
		return (T)(~(T)0);
	}

	static T MinInt()
	{
		if(IsSigned())
		{
			return (T)((T)1 << (BitCount()-1));
		}
		else
		{
			return ((T)0);
		}
	}

	//now start overloading operators
	//assignment operator
	//constructors exist for all int types and will ensure safety

	template <typename U> inline SafeInt<T>& operator =(U rhs)
	{
		//use constructor to test size
		//constructor is optimized to do minimal checking based
		//on whether T can contain U
		*this = SafeInt<T>(rhs);
		return *this;
	}

	inline SafeInt<T>& operator =(T rhs)
	{
		m_int = rhs;
		return *this;
	}

	template <typename U> inline SafeInt<T>& operator =(SafeInt<U> rhs)
	{
		*this = SafeInt<T>(rhs.Value());
		return *this;
	}

	inline SafeInt<T>& operator =(SafeInt<T> rhs)
	{
		m_int = rhs.m_int;
		return *this;
	}

	//casting operator not implemented
	//because it causes ambiguous compilation

	//Use the methods below to gain access to the data
	T Value() const {return m_int;}
	//and if you need a pointer to the data
	//this could be dangerous, but allows you to correctly pass
	//instances of this class to APIs that take a pointer to an integer
	//also see overloaded address-of operator below
	T* Ptr(){return &m_int;}
	const T* Ptr() const {return &m_int;}

	//or if SafeInt<T>::Ptr() is inconvenient, use the overload
	// operator & 
	//This allows you to do unsafe things!
	//It is meant to allow you to more easily
	//pass a SafeInt into things like ReadFile
	T* operator &(){return &m_int;}
	const T* operator &() const {return &m_int;}

	//unary operators
	bool operator !() const {return (!m_int) ? true : false;}
	
	//operator + (unary) 
	//note - normally, the '+' and '-' operators will upcast to a signed int
	//for T < 32 bits. This class changes behavior to preserve type
	SafeInt<T> operator +(void) const { return *this; };

	//unary  - 
		
	SafeInt<T> operator -() const
	{
		if(IsSigned())
		{
			//corner case
			if(m_int == MinInt())
				throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

			//cast improves perf in the case of small ints
			return SafeInt<T>((T)-m_int);
		}
		//no-op for unsigned - generates warning 4146 at warning levels 2 and above
		return SafeInt<T>((T)-m_int);
	}

	//prefix increment operator
	SafeInt<T>& operator ++()
	{
		if(m_int == MaxInt())
		{
			throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
		}
		++m_int;
		return *this;
	}
	
	//prefix decrement operator
	SafeInt<T>& operator --()
	{
		if(m_int == MinInt())
		{
			throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
		}
		--m_int;
		return *this;
	}

	//postfix increment operator
	SafeInt<T> operator ++(int) //dummy arg to comply with spec
	{
		if(m_int == MaxInt())
		{
			throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
		}

		SafeInt<T> tmp = m_int;

		m_int++;
		return tmp;
	}

	//postfix decrement operator
	SafeInt<T> operator --(int) //dummy arg to comply with spec
	{
		if(m_int == MinInt())
		{
			throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
		}

		SafeInt<T> tmp = m_int;
		m_int--;
		return tmp;
	}

	//one's complement
	//note - this operator will normally change size to an int
	//cast in return improves perf and maintains type
	SafeInt<T> operator ~() const {return SafeInt<T>((T)~m_int);}

	//binary operators
	//
	// arithmetic binary operators
	// % modulus
	// * multiplication
	// / division
	// + addition
	// - subtraction
	//
	// For each of the arithmetic operators, you will need to 
	// use them as follows:
	//
	// SafeInt<char> c = 2;
	// SafeInt<int>  i = 3;
	//
	// SafeInt<int> i2 = i op c.Value();
	// OR
	// SafeInt<char> i2 = i.Value() op c;
	//
	// The base problem is that if the lhs and rhs inputs are different SafeInt types
	// it is not possible in this implementation to determine what type of SafeInt
	// should be returned. You have to let the class know which of the two inputs
	// need to be the return type by forcing the other value to the base integer type.
	// The case of:
	//
	// SafeInt<T> i, j, k;
	// i = j op k;
	//
	// works just fine and no unboxing is needed because the return type is not ambiguous.

	//modulus
	//modulus has some convenient properties - 
	//first, the magnitude of the return can never be
	//larger than the lhs operand, and it must be the same sign
	//as well. It does, however, suffer from the same promotion
	//problems as comparisons, division and other operations
	template <typename U>
	SafeInt<T> operator %(U rhs)
	{
		return MixedSizeModulus(*this, rhs);
	}

	SafeInt<T> operator %(SafeInt<T> rhs)
	{
		if(rhs.Value() == 0)
			throw SafeIntException(EXCEPTION_INT_DIVIDE_BY_ZERO);

		//this is always safe
		return SafeInt<T>((T)(m_int % rhs.Value()));
	}

	//modulus assignment
	template <typename U>
	SafeInt<T>& operator %=(U rhs)
	{
		*this = MixedSizeModulus(*this, rhs);
		return *this;
	}

	template <typename U>
	SafeInt<T>& operator %=(SafeInt<U> rhs)
	{
		*this = MixedSizeModulus(*this, rhs.Value());
		return *this;
	}

	//multiplication
	template <typename U>
	SafeInt<T> operator *(U rhs)
	{
		return MixedSizeMultiply(*this, rhs);
	}

	SafeInt<T> operator *(SafeInt<T> rhs)
	{
		return SafeInt<T>(multiply(m_int, rhs.Value()));
	}

	//multiplication assignment
	SafeInt<T>& operator *=(SafeInt<T> rhs)
	{
		m_int = multiply(rhs.m_int, m_int);
		return *this;
	}

	template <typename U> 
	SafeInt<T>& operator *=(U rhs)
	{
		*this = MixedSizeMultiply(*this, rhs);
		return *this;
	}

	template <typename U> 
	SafeInt<T>& operator *=(SafeInt<U> rhs)
	{
		*this = MixedSizeMultiply(*this, rhs.Value());
		return *this;
	}

	//division
	template <typename U>
	SafeInt<T> operator /(U rhs)
	{
		return MixedSizeDivision(*this, rhs);
	}

	SafeInt<T> operator /(SafeInt<T> rhs)
	{
		return MixedSizeDivision(*this, rhs.Value());
	}

	//division assignment
	SafeInt<T>& operator /=(SafeInt<T> i)
	{
		*this = MixedSizeDivision(*this, i.Value());
		return *this;
	}

	template <typename U> SafeInt<T>& operator /=(U i)
	{
		*this = MixedSizeDivision(*this, i);
		return *this;
	}

	template <typename U> SafeInt<T>& operator /=(SafeInt<U> i)
	{
		*this = MixedSizeDivision(*this, i.Value());
		return *this;
	}

	//for addition and subtraction

	//addition
	inline SafeInt<T> operator +(SafeInt<T> rhs)
	{
		return SafeInt<T>(addition(m_int, rhs.Value()));
	}

	template <typename U>
	inline SafeInt<T> operator +(U rhs)
	{
		return MixedSizeAddition(*this, rhs);
	}

	//addition assignment
	SafeInt<T>& operator +=(SafeInt<T> rhs)
	{
		m_int = addition(m_int, rhs.m_int);
		return *this;
	}

	template <typename U>
	SafeInt<T>& operator +=(U rhs)
	{
		*this = MixedSizeAddition(*this, rhs);
		return *this;
	}

	template <typename U> 
	SafeInt<T>& operator +=(SafeInt<U> rhs)
	{
		*this = MixedSizeAddition(*this, rhs.Value());
		return *this;
	}

	//subtraction
	template <typename U>
	SafeInt<T> operator -(U rhs)
	{
		return MixedSizeSubtraction(*this, rhs);
	}

	SafeInt<T> operator -(SafeInt<T> rhs)
	{
		return SafeInt<T>(subtraction(m_int, rhs.m_int));
	}

	//subtraction assignment
	SafeInt<T>& operator -=(SafeInt<T> rhs)
	{
		m_int = subtraction(m_int, rhs.m_int);
		return *this;
	}

	template <typename U> 
	SafeInt<T>& operator -=(U rhs)
	{
		*this = MixedSizeSubtraction(*this, rhs);
		return *this;
	}

	template <typename U> 
	SafeInt<T>& operator -=(SafeInt<U> rhs)
	{
		*this = MixedSizeSubtraction(*this, rhs.Value());
		return *this;
	}

	//comparison operators
	//additional overloads defined outside the class at the bottom of
	//the header to allow for cases where the SafeInt is the rhs value

	// less than
	template <typename U>
	bool operator <(U rhs)
	{
		return LessThan(m_int, rhs);
	}

	bool operator <(SafeInt<T> rhs)
	{
		return m_int < rhs.m_int;
	}

	//greater than or eq.
	template <typename U>
	bool operator >=(U rhs){return !(*this < rhs);}

	bool operator >=(SafeInt<T> rhs)
	{
		return m_int >= rhs.Value();
	}

	// greater than
	template <typename U>
	bool operator >(U rhs)
	{
		return SafeInt<T>::GreaterThan(m_int, rhs);
	}

	bool operator >(SafeInt<T> rhs)
	{
		return m_int > rhs.m_int;
	}

	//less than or eq.
	template <typename U>
	bool operator <=(U rhs){return !(*this > rhs);}

	//same type - easy path
	bool operator <=(SafeInt<T> rhs)
	{
		return m_int <= rhs.Value();
	}

	//equality
	template <typename U>
	bool operator ==(U rhs){return Equals(m_int, rhs);}

	//need an explicit override for type bool
	bool operator ==(bool rhs)
	{
		return (m_int == 0 ? false : true) == rhs;
	}

	bool operator ==(SafeInt<T> rhs){return m_int == rhs.Value();}

	//!= operators
	template <typename U>
	bool operator !=(U rhs){return !Equals(m_int, rhs);}

	bool operator !=(bool b)
	{
		return (m_int == 0 ? false : true) != b;
	}

	bool operator !=(SafeInt<T> rhs){return m_int != rhs.Value();}

	//shift operators
	//Note - shift operators ALWAYS return the same type as the lhs
	//specific version for SafeInt<T> not needed - 
	//code path is exactly the same as for SafeInt<U> as rhs

	//left shift
	//Also, shifting > bitcount is undefined - trap in debug
	template <typename U> 
	SafeInt<T> operator <<(U bits)
	{
		if(IsSigned(bits))
            assert(bits >= 0);

		assert(bits < BitCount());

		return SafeInt<T>((T)(m_int << bits));
	}

	template <typename U> 
	SafeInt<T> operator <<(SafeInt<U> bits)
	{
		if(IsSigned(bits))
            assert(bits >= 0);

		assert(bits < BitCount());

		return SafeInt<T>((T)(m_int << bits.Value()));
	}

	//left shift assignment

	template <typename U>
	SafeInt<T>& operator <<=(U bits)
	{
		if(IsSigned(bits))
			assert(bits >= 0);

		assert(bits < BitCount());

		m_int <<= bits;
		return *this;
	}

	template <typename U>
	SafeInt<T>& operator <<=(SafeInt<U> bits)
	{
		if(IsSigned(bits))
			assert(bits.Value() >= 0);

		assert(bits.Value() < BitCount());

		m_int <<= bits.Value();
		return *this;
	}

	//right shift
	template <typename U> 
	SafeInt<T> operator >>(U bits)
	{
		if(IsSigned(bits))
            assert(bits >= 0);

		assert(bits < BitCount());

		return SafeInt<T>((T)(m_int >> bits));
	}

	template <typename U> 
	SafeInt<T> operator >>(SafeInt<U> bits)
	{
		if(IsSigned(bits))
            assert(bits >= 0);

		assert(bits < BitCount());

		return SafeInt<T>((T)(m_int >> bits.Value()));
	}

	//right shift assignment
	template <typename U>
	SafeInt<T>& operator >>=(U bits)
	{
		if(IsSigned(bits))
			assert(bits >= 0);

		assert(bits < BitCount());

		m_int >>= bits;
		return *this;
	}

	template <typename U>
	SafeInt<T>& operator >>=(SafeInt<U> bits)
	{
		if(IsSigned(bits))
			assert(bits.Value() >= 0);

		assert(bits.Value() < BitCount());

		m_int >>= bits.Value();
		return *this;
	}

	//bitwise operators
	//this only makes sense if we're dealing with the same type and size
	//demand a type T, or something that fits into a type T

	//bitwise &
	SafeInt<T> operator &(SafeInt<T> rhs)
	{
		return SafeInt<T>((T)(m_int & rhs.m_int));
	}

	template <typename U>
	SafeInt<T> operator &(U rhs)
	{
		//if U can fit into T without truncating, force U to T
		if(sizeof(U) <= sizeof(T))
			return SafeInt<T>(m_int & (T)rhs);

		//might still be safe
		//cast rhs down to a T, then back up to U
		//check to see if it is equal to the original value
		//this allows things like
		//SafeInt<char>(2) & 4 (literal is an int) to work
		if( (U)((T)rhs) == rhs)
			return SafeInt<T>(m_int & (T)rhs);
		
		throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

	}

	//bitwise & assignment
	SafeInt<T>& operator &=(SafeInt<T> rhs)
	{
		m_int &= rhs.m_int;
		return *this;
	}

	template <typename U> 
	SafeInt<T>& operator &=(U rhs)
	{
		*this = *this & rhs;
		return *this;
	}

	template <typename U> 
	SafeInt<T>& operator &=(SafeInt<U> rhs)
	{
		*this = *this & rhs.Value();
		return *this;
	}

	//XOR
	SafeInt<T> operator ^(SafeInt<T> rhs)
	{
		return SafeInt<T>((T)(m_int ^ rhs.m_int));
	}

	template <typename U>
	SafeInt<T> operator ^(U rhs)
	{
		//if U can fit into T without truncating, force U to T
		if(sizeof(U) <= sizeof(T))
			return SafeInt<T>(m_int ^ (T)rhs);
		
		if( (U)((T)rhs) == rhs)
			return SafeInt<T>(m_int ^ (T)rhs);

		throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

	}

	//XOR assignment
	SafeInt<T>& operator ^=(SafeInt<T> i)
	{
		m_int ^= i.m_int;
		return *this;
	}

	template <typename U> 
	SafeInt<T>& operator ^=(U rhs)
	{
		*this = *this ^ rhs;
		return *this;
	}

	template <typename U> 
	SafeInt<T>& operator ^=(SafeInt<U> rhs)
	{
		*this = *this ^ rhs.Value();
		return *this;
	}

	//bitwise OR
	SafeInt<T> operator |(SafeInt<T> rhs)
	{
		return SafeInt<T>((T)(m_int | rhs.m_int));
	}

	template <typename U>
	SafeInt<T> operator |(U rhs)
	{
		//if U can fit into T without truncating, force U to T
		if(sizeof(U) <= sizeof(T))
			return SafeInt<T>(m_int | (T)rhs);
		
		if( (U)((T)rhs) == rhs)
			return SafeInt<T>(m_int | (T)rhs);

		throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

	}

	//bitwise OR assignment
	SafeInt<T>& operator |=(SafeInt<T> i)
	{
		m_int |= i.m_int;
		return *this;
	}

	template <typename U> 
	SafeInt<T>& operator |=(U rhs)
	{
		*this = *this | rhs;
		return *this;
	}

	template <typename U> 
	SafeInt<T>& operator |=(SafeInt<U> rhs)
	{
		*this = *this | rhs.Value();
		return *this;
	}

	//logical operators
	//logical operators are like comparison operators
	//because the return value is the same regardless of 
	//what type is on the RHS or the LHS

	//and as it turns out, we need some overloads
	//bool constructor has a little overhead
	//possible combinations:
	// SafeInt<T>, SafeInt<T> - internal
	// SafeInt<T>, U          - internal
	// SafeInt<T>, bool       - internal
	// bool, SafeInt<T>       - external
	// U, SafeInt<T>          - external
	// SafeInt<U>, SafeInt<T> - external

	//logical OR
	bool operator ||(SafeInt<T> rhs)
	{
		return m_int || rhs.Value();
	}

	template <typename U>
	bool operator ||(U rhs)
	{
		return m_int || rhs;
	}

	bool operator ||(bool rhs) 
	{
		return m_int || rhs;
	}

	//logical &&
	bool operator &&(SafeInt<T> rhs)
	{
		return m_int && rhs.Value();
	}

	template <typename U>
	bool operator &&(U rhs)
	{
		return m_int && rhs;
	}

	bool operator &&(bool rhs) 
	{
		return m_int && rhs;
	}

	//miscellaneous helper functions
	SafeInt<T> Min(SafeInt<T> test, SafeInt<T> floor = SafeInt<T>::MinInt()) const
	{
		T tmp = test.Value() < m_int ? test.Value() : m_int;
		return tmp < floor ? floor : tmp;
	}

	SafeInt<T> Max(SafeInt<T> test, SafeInt<T> upper = SafeInt<T>::MaxInt()) const 
	{
		T tmp = test.Value() > m_int ? test.Value() : m_int;
		return tmp > upper ? upper : tmp;
	}

	void Swap( SafeInt<T>& with )
	{
		T temp( m_int );
		m_int = with.m_int;
		with.m_int = temp;
	}

	static SafeInt<T> SafeAtoI(const char* input)
	{
		return SafeTtoI(input);
	}

	static SafeInt<T> SafeWtoI(const wchar_t* input)
	{
		return SafeTtoI(input);
	}

private:
	//note - this looks complex, but most of the conditionals 
	//are constant and optimize away
	//for example, a signed 64-bit check collapses to:
/*
	if(lhs == 0 || rhs == 0)
		return 0;

	if(MaxInt()/+lhs < +rhs)
	{
		//overflow
		throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
	}
	//ok
	return lhs * rhs;

	Which ought to inline nicely
*/
	static T multiply(T lhs, T rhs)
	{
		if(Is64Bit())
		{
			//fast track this one - and avoid DIV_0 below
			if(lhs == 0 || rhs == 0)
				return 0;

			//we're 64 bit - slow, but the only way to do it
			if(IsSigned())
			{
				if(!IsMixedSign(lhs, rhs))
				{
					//both positive or both negative
					//result will be positive, check for lhs * rhs > MaxInt
					if(lhs > 0)
					{
						//both positive
						if(MaxInt()/lhs < rhs)
						{
							//overflow
							throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
						}
					}
					else
					{
						//both negative

						//comparison gets tricky unless we force it to positive
						//EXCEPT that -MinInt is undefined - can't be done
						//And MinInt always has a greater magnitude than MaxInt
						if(lhs == MinInt() || rhs == MinInt())
						{
							//overflow
							throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
						}

						if(MaxInt()/(-lhs) < (-rhs) )
						{
							//overflow
							throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
						}
					}
				}
				else
				{
					//mixed sign - this case is difficult
					//test case is lhs * rhs < MinInt => overflow
					//if lhs < 0 (implies rhs > 0), 
					//lhs < MinInt/rhs is the correct test
					//else if lhs > 0 
					//rhs < MinInt/lhs is the correct test
					//avoid dividing MinInt by a negative number, 
					//because MinInt/-1 is a corner case

					if(lhs < 0)
					{
						if(lhs < MinInt()/rhs)
						{
							//overflow
							throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
						}
					}
					else
					{
						if(rhs < MinInt()/lhs)
						{
							//overflow
							throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
						}
					}
				}

				//ok
				return lhs * rhs;
			}
			else
			{
				//unsigned, easy case
				if(MaxInt()/lhs < rhs)
				{
					//overflow
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
				}
				//ok
				return lhs * rhs;
			}
		}
		else if(Is32Bit())
		{
			//we're 32-bit
			if(IsSigned())
			{
				signed _int64 tmp = (signed _int64)lhs * (signed _int64)rhs;

				//upper 33 bits must be the same
				//most common case is likely that both are positive - test first
				if( (tmp & 0xffffffff80000000LL) == 0 || 
					(tmp & 0xffffffff80000000LL) == 0xffffffff80000000LL)
				{
					//this is OK
					return (T)tmp;
				}

				//overflow
				throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
				
			}
			else
			{
				unsigned _int64 tmp = (unsigned _int64)lhs * (unsigned _int64)rhs;
				if (tmp & 0xffffffff00000000ULL) //overflow
				{
					//overflow
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
				}
				return (T)tmp;
			}
		}
		else if(Is16Bit())
		{
			//16-bit
			if(IsSigned())
			{
				signed _int32 tmp = (signed _int32)lhs * (signed _int32)rhs;
				//upper 17 bits must be the same
				//most common case is likely that both are positive - test first
				if( (tmp & 0xffff8000) == 0 || (tmp & 0xffff8000) == 0xffff8000)
				{
					//this is OK
					return (T)tmp;
				}

				//overflow
				throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
			}
			else
			{
				unsigned _int32 tmp = (unsigned _int32)lhs * (unsigned _int32)rhs;
				if (tmp & 0xffff0000) //overflow
				{
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
				}
				return (T)tmp;
			}
		}
		else //8-bit
		{
			assert(Is8Bit());

			if(IsSigned())
			{
				signed _int16 tmp = (signed _int16)lhs * (signed _int16)rhs;
				//upper 9 bits must be the same
				//most common case is likely that both are positive - test first
				if( (tmp & 0xff80) == 0 || (tmp & 0xff80) == 0xff80)
				{
					//this is OK
					return (T)tmp;
				}

				//overflow
				throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
			}
			else
			{
				unsigned _int16 tmp = ((unsigned _int16)lhs) * ((unsigned _int16)rhs);

				if (tmp & 0xff00) //overflow
				{
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
				}
				return (T)tmp;
			}
		}
	}

	static inline T addition(T lhs, T rhs)
	{
		if(IsSigned())
		{
			//test for +/- combo
			if(!IsMixedSign(lhs, rhs)) 
			{
				//either two negatives, or 2 positives
				if(rhs < 0)
				{
					//two negatives
					if(lhs < (T)(MinInt() - rhs)) //remember rhs < 0
					{
						throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
					}
					//ok
				}
				else
				{
					//two positives
					if((T)(MaxInt() - lhs) < rhs)
					{
						throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
					}
					//OK
				}
			}
			//else overflow not possible
			return lhs + rhs;
		}
		else //unsigned
		{
			if((T)(MaxInt() - lhs) < rhs)
			{
				throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
				
			}
			return (lhs + rhs);
		}
	}

	static T subtraction(T lhs, T rhs)
	{
		if(IsSigned())
		{
			if(IsMixedSign(lhs, rhs)) //test for +/- combo
			{
				//mixed positive and negative
				//two cases - +X - -Y => X + Y - check for overflow against MaxInt()
				//            -X - +Y - check for overflow against MinInt()

				if(lhs >= 0) //first case
				{
					//test is X - -Y > MaxInt()
					//equivalent to X > MaxInt() - |Y|
					//Y == MinInt() creates special case
					//Even 0 - MinInt() can't be done
					//note that the special case collapses into the general case, due to the fact
					//MaxInt() - MinInt() == -1, and lhs is non-negative
					if(lhs > (T)(MaxInt() + rhs)) //remember that rhs is negative
					{
						throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
					}
					//fall through to return value
				}
				else
				{
					//second case
					//test is -X - Y < MinInt()
					//or      -X < MinInt() + Y
					//we do not have the same issues because abs(MinInt()) > MaxInt()
					if(lhs < (T)(MinInt() + rhs))
					{
						throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
					}
					//fall through to return value
				}
			}
			// else 
			//both negative, or both positive
			//no possible overflow
			return (lhs - rhs);
		}
		else
		{
			//easy unsigned case
			if(lhs < rhs)
			{
				throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
			}
			return (lhs - rhs);
		}
	}

	template <typename U>
	static SafeInt<T> MixedSizeModulus(SafeInt<T> lhs, U rhs)
	{
		//this is a simpler case than other arithmetic operations
		//first, sign of return must be same as sign of lhs
		//next, magnitude of return can never be larger than lhs

		//always test this:
		if(rhs == 0)
			throw SafeIntException(EXCEPTION_INT_DIVIDE_BY_ZERO);

		//problem cases are:
		//either T or U 32 or 64-bit unsigned, other is signed
		//signed value must be negative to create problem

		//first problem case
		//T unsigned 32-bit, U signed
		if(sizeof(T) == 4 && !SafeInt<T>::IsSigned() && 
				sizeof(U) <= 4 && SafeInt<U>::IsSigned()) 
		{
			if(rhs < 0)
				return SafeInt<T>((_int64)lhs.Value() % (_int64)rhs);
		}

		//second problem case
		//T signed <=32-bit, U unsigned 32-bit
		if(sizeof(U) == 4 && !SafeInt<U>::IsSigned() && 
				sizeof(T) <= 4 && SafeInt<T>::IsSigned())
		{
			if(lhs.Value() < 0)
				return SafeInt<T>((_int64)lhs.Value() % (_int64)rhs);
		}

		//third problem case
		//T unsigned 64-bit, U signed
		if(sizeof(T) == 8 && !SafeInt<T>::IsSigned() && 
			SafeInt<U>::IsSigned())
		{
			if(rhs < 0)
			{
				//return must be positive
				return SafeInt<T>((T)lhs.Value() % (T)(-rhs));
			}

			//else it must be safe to cast U to T
			return SafeInt<T>((T)lhs.Value() % (T)rhs);
		}

		//fourth problem case
		//T signed, U unsigned 64-bit
		if(sizeof(U) == 8 && !SafeInt<U>::IsSigned() && 
			SafeInt<T>::IsSigned())
		{
			if(lhs.Value() < 0)
			{
				//first cast -lhs to U - must fit
				//modulus operation returns type U, must fit into T (2nd cast to T)
				//negation forces to int, re-cast to T
				return SafeInt<T>((T)-(T)( ((U)(-lhs.Value())) % (U)rhs));
			}

			return SafeInt<T>((U)lhs.Value() % rhs);
		}

		//else no problem
		return SafeInt<T>(lhs.Value() % rhs);
	}

	template <typename U>
	static SafeInt<T> MixedSizeDivision(SafeInt<T> lhs, U rhs)
	{
		//first test corner cases

		if(rhs == 0)
			throw SafeIntException(EXCEPTION_INT_DIVIDE_BY_ZERO);

		//only if both are signed, check corner case
		if(SafeInt<U>::IsSigned() && SafeInt<T>::IsSigned())
		{
			//corner case where lhs = MinInt and rhs = -1
			if(lhs == SafeInt<T>::MinInt() && rhs == -1)
				throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
		}

		//it is safe to divide lhs by an arbitrary larger number
		//unless T is unsigned and U is signed

		//if both have the same sign, there is no problem
		if(SafeInt<U>::IsSigned() == SafeInt<T>::IsSigned())
		{
			if(sizeof(T) >= sizeof(U))
				return SafeInt<T>((T)((T)lhs.Value()/(T)rhs));

			return SafeInt<T>((T)((U)lhs.Value()/(U)rhs));
		}
		
		//now we have mixed sign case, which can lead to problems
		//first consider T signed, U unsigned

		if(SafeInt<T>::IsSigned())
		{
			if(sizeof(U) < sizeof(T))
			{
				//simply upcast to T - rhs always fits into T
				return SafeInt<T>((T)(lhs.Value()/(T)rhs));
			}

			if(sizeof(U) < 4 && sizeof(T) < 4)
			{
				//even if U is bigger, upcast to int
				return SafeInt<T>((T)((_int32)lhs.Value()/(_int32)rhs));
			}

			//now U is either 32 or 64-bit, T same size or smaller
			if(sizeof(U) == 4)
			{
				//upcast to 64-bit
				return SafeInt<T>((T)((_int64)lhs.Value()/(_int64)rhs));
			}

			//U is unsigned _int64
			//now it matters whether lhs < 0
			if(lhs.Value() < 0)
			{
				if(rhs > (U)SafeInt<T>::MaxInt() + 1)
					return SafeInt<T>(0);

				//corner case
				if(lhs.Value() == SafeInt<T>::MinInt() &&
					rhs == (U)SafeInt<T>::MaxInt() + 1)
				{
					return SafeInt<T>((T)-1);
				}

				//finally, rhs fits into T - just cast
				return SafeInt<T>((T)(lhs.Value()/(T)rhs));
			}

			//lhs >= 0
			//T now has to fit into U
			return SafeInt<T>((T)((U)lhs.Value()/(U)rhs));
		}

		//now lhs is unsigned, rhs signed
		if(rhs < 0)
			throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

		if(sizeof(T) >= sizeof(U))
		{
			//all U fits into T
			return SafeInt<T>((T)(lhs.Value()/(T)rhs));
		}
			
		//now sizeof(U) > sizeof(T) and rhs > 0
		//all T fits into U
		return SafeInt<T>((T)((U)lhs.Value()/rhs));
	}

	template <typename U> 
	static SafeInt<T> MixedSizeAddition(SafeInt<T> lhs, U rhs)
	{
		if(SafeInt<T>::IsSigned() == SafeInt<U>::IsSigned())
		{
			//easy case - just upcast U to T
			if(sizeof(T) >= sizeof(U))
			{
				return SafeInt<T>(addition(lhs.Value(), (T)rhs));
			}

			//otherwise, U > T
			//where possible, do the upcast inline
			//this avoids range checks inside the addition call
			//and additional range checks on assignment
			if(sizeof(U) == 2)
			{
				//an int can hold any possible range of _int16 + char
				//do bounds checking in constructor
				if(IsSigned())
					return SafeInt<T>((_int32)lhs.Value() + (_int32)rhs);
				//else unsigned

				return SafeInt<T>((unsigned _int32)lhs.Value() + (unsigned _int32)rhs);
			}

			if(sizeof(U) == 4)
			{
				//it is possibly overkill to go to 64-bit, but other alternatives
				//involve more conditionals. This is likely cheaper overall
				if(IsSigned())
					return SafeInt<T>((_int64)lhs.Value() + (_int64)rhs);

				return SafeInt<T>((unsigned _int64)lhs.Value() + (unsigned _int64)rhs);
			}

			//else U is an _int64
			//this will have to be done the expensive way
			{
				return SafeInt<T>(SafeInt<U>(lhs.Value()) + rhs); //more checking here
			}
		}

		//else mixed sign
		//first consider the case of signed T, unsigned U
		if(SafeInt<T>::IsSigned())
		{
			if(sizeof(T) > sizeof(U))
			{
				//piece of cake, U fits in T
				return SafeInt<T>(addition(lhs.Value(), (T)rhs));
			}

			//else sizeof(T) <= sizeof(U)
			if(sizeof(U) < 4)
			{
				//upcast to int, which is what the compiler normally does
				//catch overflows in the constructor
				return SafeInt<T>((_int32)lhs.Value() + (_int32)rhs);
			}

			if(sizeof(U) == 4)
			{
				return SafeInt<T>((_int64)lhs.Value() + (_int64)rhs);
			}

			//else U is unsigned 64-bit
			//this will have to be done the expensive way
			//this particular part of the problem is especially
			//difficult - the result has to fit correctly into a signed int[8|16|32]
			//lhs input could be negative
			//rhs input greater than an _int64 MAX_INT could be legal
			if(sizeof(T) < 4)
			{
				//test if rhs is even possibly legal
				if((unsigned _int64)rhs > (unsigned _int64)SafeInt<unsigned _int16>::MaxInt())
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

				//stuff them all into an int, return
				return SafeInt<T>((_int32)lhs.Value() + (_int32)rhs);
			}
			else
			if(sizeof(T) == 4)
			{
				//you can't possibly add more than 0xFFFFFFFF
				//to a signed int and have it work, so
				if((unsigned _int64)rhs > (unsigned _int64)SafeInt<unsigned _int32>::MaxInt())
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

				//now that we know that rhs fits into an unsigned _int32
				//result must fit into an _int64
				return SafeInt<T>((_int64)lhs.Value() + (_int64)rhs);
			}
			else
			{
				//T == signed _int64
				if(lhs.Value() >= 0)
				{
					//they both need to fit into an _int64
					//given that lhs is positive, rhs must be <= _int64 MaxInt
					if((unsigned _int64)rhs > (unsigned _int64)SafeInt<_int64>::MaxInt())
						throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
					
					return SafeInt<T>(addition((T)lhs.Value(), (T)rhs));
				}

				//else rhs could range all the way up to a unsigned _int64 MAX_INT
				//rearrange as rhs - (-lhs)
				//this still works even if lhs == _int64 MIN_INT
				return SafeInt<T>(rhs - (unsigned _int64)(-(lhs.Value())));
			}
		}

		//now we have the case of unsigned T, signed U
		//we just solved this problem above - since A+B == B+A
		//code is largely duplicated, but there are optimizations
		//remember that U could be negative

		if(sizeof(T) < 4 && sizeof(U) < 4)
		{
			//upcast to int, which is what the compiler normally does
			//catch overflows in the constructor
			return SafeInt<T>((_int32)lhs.Value() + (_int32)rhs);
		}

		if(sizeof(T) < 8 && sizeof(U) < 8)
		{
			//either T or U are 32-bit
			//all possible combinations fit into an _int64
			return SafeInt<T>((_int64)lhs.Value() + (_int64)rhs);
		}

		if(sizeof(U) == 8 && sizeof(T) < 8)
		{
			//all possible values of T fit into an _int64
			SafeInt<U> u(rhs);
			u += (_int64)lhs.Value();
			return SafeInt<T>(u.Value());
		}

		//else T is unsigned 64-bit
		//this will have to be done the expensive way
		//rhs input could be negative
		if(rhs >= 0)
		{
			//all possible values of rhs can fit into an unsigned _int64
			return SafeInt<T>(addition(lhs.Value(), (T)rhs));
		}

		//rhs is negative
		if((T)(-rhs) > lhs.Value()) //this will never work
			throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

		return SafeInt<T>(lhs.Value() - (T)(-rhs));
	}

	template <typename U> 
	static SafeInt<T> MixedSizeSubtraction(SafeInt<T> lhs, U rhs)
	{
		if(SafeInt<T>::IsSigned() == SafeInt<U>::IsSigned())
		{
			//easy case - just upcast U to T
			if(sizeof(T) >= sizeof(U))
				return SafeInt<T>(subtraction(lhs.Value(), (T)rhs));

			//otherwise, U > T
			if(sizeof(U) == 2)
			{
				//an int can hold any possible range of _int16 - char
				//do bounds checking in constructor
				if(IsSigned())
					return SafeInt<T>((_int32)lhs.Value() - (_int32)rhs);
				//else unsigned

				return SafeInt<T>((unsigned _int32)lhs.Value() - (unsigned _int32)rhs);
			}

			if(sizeof(U) == 4)
			{
				//it is possibly overkill to go to 64-bit, but other alternatives
				//involve more conditionals. This is likely cheaper overall
				if(IsSigned())
					return SafeInt<T>((_int64)lhs.Value() - (_int64)rhs);

				return SafeInt<T>((unsigned _int64)lhs.Value() - (unsigned _int64)rhs);
			}

			//else U is a signed or unsigned _int64
			//this will have to be done the expensive way
			{
				return SafeInt<T>(SafeInt<U>(lhs.Value()) - rhs); //more checking here
			}
		}

		//else mixed sign
		//first consider the case of signed T, unsigned U
		if(IsSigned())
		{
			if(sizeof(T) > sizeof(U))
			{
				//piece of cake, U fits in T
				return SafeInt<T>(subtraction(lhs.Value(), (T)rhs));
			}

			//else sizeof(T) <= sizeof(U)
			if(sizeof(U) < 4)
			{
				//upcast to int, which is what the compiler normally does
				//catch overflows in the constructor
				return SafeInt<T>((_int32)lhs.Value() - (_int32)rhs);
			}

			if(sizeof(U) == 4)
			{
				//upcast to _int64, which is what the compiler should do
				return SafeInt<T>((_int64)lhs.Value() - (_int64)rhs);
			}

			//else U is unsigned 64-bit
			//this will have to be done the expensive way
			//this particular part of the problem is especially
			//difficult - the result has to fit correctly into a signed int[8|16|32]
			//lhs input could be negative, rhs input is always positive
			if(sizeof(T) < 4)
			{
				//test if rhs is even possibly legal
				if(rhs > (U)SafeInt<unsigned _int16>::MaxInt())
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

				//stuff them all into an int, return
				return SafeInt<T>((_int32)lhs.Value() - (_int32)rhs);
			}
			else
			if(sizeof(T) == 4)
			{
				//you can't possibly subtract more than 0xFFFFFFFF
				//from a signed int and have it work, so
				if(rhs > (U)SafeInt<unsigned _int32>::MaxInt())
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

				return SafeInt<T>((_int64)lhs.Value() - (_int64)rhs);
			}
			else
			{
				//T = signed _int64
				//U = unsigned _int64

				//need the size of the range between lhs.MinInt and lhs
				//this is the maximum value that can be subtracted from lhs
				//we're actually going to take advantage of rollover
				if(rhs > (U)(lhs.Value() - SafeInt<T>::MinInt()))
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

				//else it has to work
				return SafeInt<T>((T)(lhs.Value() - rhs));
			}
		}

		//now we have the case of unsigned T, signed U
		
		//easily deals with 1/4 of the problem space
		if(sizeof(T) < 4 && sizeof(U) < 4)
		{
			//stuff them all into an int, return
			return SafeInt<T>((_int32)lhs.Value() - (_int32)rhs);
		}
		else
		if(sizeof(T) < 8 && sizeof(U) < 8)
		{
			//handles another 5 of 16 cases
			return SafeInt<T>((_int64)lhs.Value() - (_int64)rhs);
		}

		//case of T = unsigned _int64
		//handles 4 more cases
		if(sizeof(T) == 8)
		{
			//has to work - rhs has to fit into a T
			if(rhs >= 0)
				return SafeInt<T>(subtraction(lhs.Value(), (T)rhs));

			//rhs negative - turn into an addition
			//take care to do an intermediate cast because the
			//unary negation operator returns an int
			//corner case of MinInt still works
			return SafeInt<T>(addition(lhs.Value(), (T)((U)-rhs)));
		}

		//sizeof T < 8, sizeof U == 8
		//maximum possible range for rhs is lhs - MinInt(T)
		//because sizeof T smaller than _int64, upcast
		if(rhs > (U)((_int64)lhs.Value() - (_int64)lhs.MinInt()))
			throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

		return SafeInt<T>((_int64)lhs.Value() - (_int64)rhs);
	}

	template <typename U> 
	static SafeInt<T> MixedSizeMultiply(SafeInt<T> lhs, U rhs)
	{
		//what is U?
		//if T is unsigned, and sizeof(T) >= sizeof(U)
		//T can hold all values of U for rhs > 0
		//if T is unsigned and sizeof(T) < sizeof(U)
		//declare an unsigned SafeInt of same size as U

		//if T is signed and sizeof(T) > sizeof(U)
		//T can hold all values of U
		//if T is signed and sizeof(T) <= sizeof(U)
		//declare a signed SafeInt of same size as U

		if(SafeInt<T>::IsSigned() == SafeInt<U>::IsSigned())
		{
			//simple case - same signedness and U always fits in T
			if(sizeof(T) >= sizeof(U))
			{
				return SafeInt<T>(multiply(lhs.Value(), (T)rhs));
			}

			//simple case - same signedness and U bigger than T
			//looks like a lot of code, but is compile-time constants
			//must handle signed and unsigned in different cases - unless sizeof(U) == 8

			if(sizeof(U) == 8)
			{
				SafeInt<U> u(rhs);
				u *= lhs.m_int;
				return SafeInt<T>(u.Value());
			}

			if(SafeInt<T>::IsSigned())
			{
				if(sizeof(U) < 4)
				{
					//the result must always fit into an int
					return SafeInt<T>((_int32)((_int32)lhs.m_int * (_int32)rhs));
				}

				//result must fit into an _int64
				return SafeInt<T>((_int64)((_int64)lhs.m_int * (_int64)rhs));
			}

			//else unsigned
			if(sizeof(U) < 4)
			{
				//the result must always fit into an int
				return SafeInt<T>((unsigned _int32)((unsigned _int32)lhs.m_int * (unsigned _int32)rhs));
			}

			//result must fit into an _int64
			return SafeInt<T>((unsigned _int64)((unsigned _int64)lhs.m_int * (unsigned _int64)rhs));
		}

		//mixed sign - consider T is signed, U unsigned
		if(SafeInt<T>::IsSigned() && !SafeInt<U>::IsSigned())
		{
			//if T > U, we're OK
			if(sizeof(T) > sizeof(U))
			{
				return SafeInt<T>(multiply(lhs.Value(), (T)rhs));
			}

			//else sizeof(T) <= sizeof(U) - upcast T to signed U
			//otherwise, we have to make an signed next size up from U
			if(sizeof(U) < 4)
			{
				//T is a signed char or _int16, U is unsigned _int16 or char
				//this must also always fit into an int
				return SafeInt<T>((_int32)((_int32)lhs.m_int * (_int32)rhs));
			}
			else if(sizeof(U) == 4)
			{
				//T is signed int or smaller, U is unsigned int
				//result must fit into an _int64
				return SafeInt<T>((_int64)((_int64)lhs.m_int * (_int64)rhs));
			}
			else
			{
				//U is unsigned 64-bit

				//now if rhs > MaxInt(T), overflow unless lhs == 0
				//or the corner case
				if(rhs > (U)SafeInt<T>::MaxInt())
				{
					//corner case -1 * (MaxInt + 1) = MinInt
					//do lhs comparison first, since is cheaper than 64-bit test
					if(lhs.Value() == -1 && ((U)SafeInt<T>::MaxInt()+1) == rhs)
						return SafeInt<T>(SafeInt<T>::MinInt());

					if(lhs.Value() == 0)
						return SafeInt<T>(0);

					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
				}

				//now rhs must fit into T
				return SafeInt<T>(multiply(lhs.Value(), (T)rhs));
			}
		}

		//now mixed sign where T is unsigned, U signed
			
		//negative numbers are always bad
		//test here to avoid having to test in constructors below
		//also allows some simplifying assumptions
		if(rhs < 0)
			throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

		if(sizeof(T) >= sizeof(U))
		{
			return SafeInt<T>(multiply(lhs.Value(), (T)rhs));
		}

		//U > T
		//else there is no corner case, but we do have to check overflow
		if(sizeof(U) < 4)
		{
			//both U and T can be contained in an unsigned int
			return SafeInt<T>((unsigned _int32)((unsigned _int32)lhs.m_int * (unsigned _int32)rhs));
		}
		else if(sizeof(U) == 4)
		{
			//now go up to 64-bit
			return SafeInt<T>((unsigned _int64)((unsigned _int64)lhs.m_int * (unsigned _int64)rhs));
		}
		else
		{
			//U = signed _int64, T = unsigned [char|_int16|int]
			SafeInt<U> u(rhs);
			u *= lhs.Value();

			//now bounds check
			if(u.Value() > (U)SafeInt<T>::MaxInt())
				throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

			//it has to be safe, since rhs is non-negative
			return SafeInt<T>((T)u.Value());
		}
	}
	
	//Note - the standard is arguably broken in the case of some integer
	//conversion operations
	//For example, signed char a = -1 = 0xff
	//             unsigned int b = 0xffffffff
	//if you then test if a < b, a value-preserving cast
	//is made, and you're essentially testing
	// (unsigned int)a < b == false
	//
	// I do not think this makes sense - if you perform
	// a cast to an _int64, which can clearly preserve both value and signedness
	// then you get a different and intuitively correct answer
	// IMHO, -1 should be less than 4 billion
	// If you prefer to retain the ANSI standard behavior
	// insert #define ANSI_CONVERSIONS into your source
	// Behavior differences occur in the following cases:
	// 8, 16, and 32-bit signed int, unsigned 32-bit int
	// any signed int, unsigned 64-bit int
	// Note - the signed int must be negative to show the problem

	template <typename U>
	static bool LessThan(T lhs, U rhs)
	{
#ifdef ANSI_CONVERSIONS
		return lhs < rhs;
#else
		return (SafeInt<U>(rhs) > lhs);
#endif
	}

	template <typename U>
	static bool GreaterThan(T lhs, U rhs)
	{
#ifdef ANSI_CONVERSIONS
		return lhs > rhs;
#else
		if(SafeInt<T>::IsSigned(lhs) == SafeInt<T>::IsSigned(rhs))
		{
			if(sizeof(T) > sizeof(U))
                return (T)lhs > (T)rhs;

			return (U)lhs > (U)rhs;
		}
		
		//all remaining cases are mixed sign
		if((sizeof(T) < 4 && sizeof(U) < 4) ||
		    (SafeInt<T>::IsSigned(lhs) && sizeof(T) == 4 && sizeof(U) < 4) ||
			(SafeInt<T>::IsSigned(rhs) && sizeof(U) == 4 && sizeof(T) < 4))
		{
			//all of these fit into an int
			return (_int32)lhs > (_int32)rhs;
		}

		if((SafeInt<T>::IsSigned(rhs) && sizeof(U) == 8 && sizeof(T) < 8) ||
		   (SafeInt<T>::IsSigned(lhs) && sizeof(T) == 8 && sizeof(U) < 8))
		{
			//these cases all fit into an _int64
			return (_int64)lhs > (_int64)rhs;
		}

		//for all remaining cases unsigned value is 64-bit

		//corner cases - signed value is negative
		if(SafeInt<T>::IsSigned(lhs) && lhs < 0) 
		{
			//if lhs < 0, rhs unsigned
			return false;
		}
			
		//2nd corner case
		if(SafeInt<T>::IsSigned(rhs) && rhs < 0)
		{
			//rhs < 0, lhs unsigned
			return true;
		}

		//now the signed value is positive, and must fit into a 64-bit unsigned
		return (unsigned _int64)lhs > (unsigned _int64)rhs;
#endif
	}

	template <typename U>
	static bool Equals(T lhs, U rhs)
	{
#ifdef ANSI_CONVERSIONS
		return lhs == rhs;
#else
		if(SafeInt<T>::IsSigned(lhs) == SafeInt<T>::IsSigned(rhs))
		{
			if(sizeof(T) > sizeof(U))
                return (T)lhs == (T)rhs;

			return (U)lhs == (U)rhs;
		}
		
		//all remaining cases are mixed sign
		if((sizeof(T) < 4 && sizeof(U) < 4) ||
		    (SafeInt<T>::IsSigned(lhs) && sizeof(T) == 4 && sizeof(U) < 4) ||
			(SafeInt<T>::IsSigned(rhs) && sizeof(U) == 4 && sizeof(T) < 4))
		{
			//all of these fit into an int
			return (_int32)lhs == (_int32)rhs;
		}

		if((SafeInt<T>::IsSigned(rhs) && sizeof(U) == 8 && sizeof(T) < 8) ||
		   (SafeInt<T>::IsSigned(lhs) && sizeof(T) == 8 && sizeof(U) < 8))
		{
			//these cases all fit into an _int64
			return (_int64)lhs == (_int64)rhs;
		}

		//for all remaining cases unsigned value is 64-bit
		if((SafeInt<T>::IsSigned(lhs) && lhs < 0) ||
			(SafeInt<T>::IsSigned(rhs) && rhs < 0))
		{
			//corner case - signed value is negative
			//cannot possibly be equal to the unsigned value
			return false;
		}

		//now the signed value is positive, and must fit into a 64-bit unsigned
		return (unsigned _int64)lhs == (unsigned _int64)rhs;
#endif
	}

	//this is almost certainly not the best optimized version of atoi,
	//but it does not display a typical bug where it isn't possible to set MinInt
	//and it won't allow you to overflow your integer
	//This is here because it is useful, and it is an example of what
	//can be done easily with SafeInt
	template <typename U>
	static SafeInt<T> SafeTtoI(U* input)
	{
		U* tmp  = input;
		SafeInt<T> s;
		bool negative = false;
		
		if(input == NULL || input[0] == 0)
			throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

		switch(*tmp)
		{
		case '-':
			tmp++;
			negative = true;
			break;
		case '+':
			tmp++;
			break;
		}

		while(*tmp != 0)
		{
			if(*tmp < '0' || *tmp > '9')
				break;
			
			if(s.Value() != 0)
				s *= (T)10;

			if(!negative)
				s += (T)(*tmp - '0');
			else
                s -= (T)(*tmp - '0');

			tmp++;
		}

		return s;
	}

	//internal helper functions

	//explanation - consider 2 8-bit ints:
	//  00001010
	// ^10001000
	// =10000010
	// if result is < 0, high bit is set
	//so we now have an efficient test to see if two integers
	//are the same sign or opposite signs

	static bool IsMixedSign(T lhs, T rhs)
	{
		return ((lhs ^ rhs) < 0);
	}

	T m_int;
};

//externally defined functions for the case of U op SafeInt<T>
template <typename T, typename U>
bool operator <(U lhs, SafeInt<T> rhs)
{
	return rhs > lhs;
}

template <typename T, typename U>
bool operator <(SafeInt<U> lhs, SafeInt<T> rhs)
{
	return lhs < rhs.Value();
}

//greater than
template <typename T, typename U>
bool operator >(U lhs, SafeInt<T> rhs)
{
	return rhs < lhs;
}

template <typename T, typename U>
bool operator >(SafeInt<T> lhs, SafeInt<U> rhs)
{
	return lhs > rhs.Value();
}


//greater than or equal
template <typename T, typename U>
bool operator >=(U lhs, SafeInt<T> rhs)
{
	return rhs < lhs;
}

template <typename T, typename U>
bool operator >=(SafeInt<T> lhs, SafeInt<U> rhs)
{
	return lhs >= rhs.Value();
}

//less than or equal
template <typename T, typename U>
bool operator <=(U lhs, SafeInt<T> rhs)
{
	return rhs > lhs;
}

template <typename T, typename U>
bool operator <=(SafeInt<T> lhs, SafeInt<U> rhs)
{
	return lhs <= rhs.Value();
}

//equality
//explicit overload for bool
template <typename T>
bool operator ==(bool lhs, SafeInt<T> rhs)
{
	return lhs == (rhs.Value() == 0 ? false : true);
}

template <typename T, typename U>
bool operator ==(U lhs, SafeInt<T> rhs)
{
	return rhs == lhs;
}

template <typename T, typename U>
bool operator ==(SafeInt<T> lhs, SafeInt<U> rhs)
{
	return lhs == rhs.Value();
}

//not equals
template <typename T, typename U>
bool operator !=(U lhs, SafeInt<T> rhs)
{
	return rhs != lhs;
}

template <typename T>
bool operator !=(bool lhs, SafeInt<T> rhs)
{
	return (rhs.Value() == 0 ? false : true) != lhs;
}

template <typename T, typename U>
bool operator !=(SafeInt<T> lhs, SafeInt<U> rhs)
{
	return lhs != rhs.Value();
}

//modulus
template <typename T, typename U>
SafeInt<T> operator %(U lhs, SafeInt<T> rhs)
{
	//value of return depends on sign of lhs
	//this one may not be safe - bounds check in constructor
	//if lhs is negative and rhs is unsigned, this will throw an exception

	//fast-track the simple case
	if(sizeof(T) == sizeof(U) &&
		SafeInt<T>::IsSigned() == SafeInt<U>::IsSigned())
	{
		if(rhs == 0)
			throw SafeIntException(EXCEPTION_INT_DIVIDE_BY_ZERO);

		return SafeInt<T>((T)(lhs % rhs.Value()));
	}

	return SafeInt<T>( (SafeInt<U>(lhs) % rhs.Value()) );
}

//multiplication
template <typename T, typename U> 
SafeInt<T> operator *(U lhs, SafeInt<T> rhs)
{
	return rhs * lhs;
}

//division
template <typename T, typename U> SafeInt<T> operator /(U lhs, SafeInt<T> rhs)
{
	//no easy way out - cannot make lhs into a SafeInt, then convert
	//or cases of lhs unsigned, rhs < 0 become illegal which is wrong
	//first test corner cases

	if(rhs.Value() == 0)
		throw SafeIntException(EXCEPTION_INT_DIVIDE_BY_ZERO);

	//only if both are signed, check corner case
	if(SafeInt<U>::IsSigned() && SafeInt<T>::IsSigned())
	{
		//corner case where lhs = MinInt and rhs = -1
		if(lhs == SafeInt<U>::MinInt() && rhs == -1)
			throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);
	}

	//it is safe to divide lhs by an arbitrary larger number
	//unless T is unsigned and U is signed

	//if both have the same sign, there is no problem
	//though there could be an overflow - do not cast result to T
	if(SafeInt<U>::IsSigned() == SafeInt<T>::IsSigned())
	{
		return SafeInt<T>(lhs/rhs.Value());
	}
	
	//now we have mixed sign case, which can lead to problems
	//first consider T signed, U unsigned

	if(SafeInt<T>::IsSigned())
	{
		if(sizeof(U) < sizeof(T))
		{
			//simply upcast to T - lhs always fits into T
			return SafeInt<T>(((T)lhs)/rhs.Value());
		}

		if(sizeof(U) < 4 && sizeof(T) < 4)
		{
			//even if U is bigger, upcast to int
			return SafeInt<T>((_int32)lhs/(_int32)rhs.Value());
		}

		//now U is either 32 or 64-bit, T same size or smaller
		if(sizeof(U) == 4)
		{
			//upcast to 64-bit
			return SafeInt<T>((_int64)lhs/(_int64)rhs.Value());
		}

		//U is unsigned _int64
		//now it matters whether rhs < 0
		if(rhs.Value() < 0)
		{
			U tmp = lhs/((U)-rhs.Value());
			if(tmp == (U)SafeInt<T>::MaxInt()+1)
				return SafeInt<T>(SafeInt<T>::MinInt());

			//else tmp is too big, or can be negated
			return SafeInt<T>(-(SafeInt<T>(tmp)));
		}

		//rhs >= 0
		//T now has to fit into U
		return SafeInt<T>((U)lhs/(U)rhs.Value());
	}

	//now lhs is signed, rhs unsigned - return must be unsigned
	if(lhs < 0)
		throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

	if(sizeof(T) >= sizeof(U))
	{
		//all U fits into T
		return SafeInt<T>((T)lhs/rhs.Value());
	}
		
	//now sizeof(U) > sizeof(T) and rhs >= 0
	//all T fits into U
	return SafeInt<T>(lhs/(U)rhs.Value());
}

//addition
template <typename T, typename U>
SafeInt<T> operator +(U lhs, SafeInt<T> rhs)
{
	return rhs + lhs;
}

//subtraction
template <typename T, typename U>
SafeInt<T> operator -(U lhs, SafeInt<T> rhs)
{
	if(rhs.IsSigned())
	{
		if(SafeInt<U>::IsSigned())
		{
			//both are signed
			if(sizeof(T) >= sizeof(U))
				return SafeInt<T>(lhs) - rhs;
			//else
			return SafeInt<T>(SafeInt<U>(lhs) - rhs.Value());
		}

		//lhs is unsigned, rhs is signed
		if(sizeof(T) > sizeof(U))
			return SafeInt<T>(lhs) - rhs;

		//U is >= T - not all values of U fit into T
		if(sizeof(U) < 4)
		{
			//upcast to int
			return SafeInt<T>((_int32)lhs - (_int32)rhs.Value());
		}

		if(sizeof(U) == 4)
		{
			//upcast to _int64
			return SafeInt<T>((_int64)lhs - (_int64)rhs.Value());
		}

		if(sizeof(U) == 8)
		{
			//lhs - unsigned _int64
			//rhs - signed int - any size

			if(sizeof(T) < 8)
			{
				//if this is true, the result can never fit into T
				if(lhs > ((U)2 * (U)SafeInt<T>::MaxInt()))
					throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

				//everything has to be able to fit into an _int64
				return SafeInt<T>((_int64)lhs - (_int64)rhs.Value());
			}

			//now rhs is _int64

			//if rhs is > 0, upcast to U
			if(rhs.Value() >= 0)
				return SafeInt<T>(SafeInt<U>(lhs) - rhs.Value());

			//if rhs < 0, treat as addition
			//unboxing everything maintains correctness, even if rhs = MinInt
			return SafeInt<T>(SafeInt<U>(lhs) + SafeInt<U>((U)((T)-rhs.Value())) );
		}
	}

	//T is unsigned
	//this means that if lhs < 0, result is an error
	if(SafeInt<T>::IsSigned(lhs) && lhs < 0)
		throw SafeIntException(ERROR_ARITHMETIC_OVERFLOW);

	//whether T is signed or not, it is non-negative
	//if U fits in T, no problem
	if(sizeof(T) >= sizeof(U))
	{
		return SafeInt<T>(lhs) - rhs;
	}

	//sizeof(T) < sizeof(U)
	//T fits in U, whether U is signed or unsigned
	return SafeInt<T>(SafeInt<U>(lhs) - rhs.Value());
}

//shift operators
//NOTE - shift operators always return the type of the lhs argument

//left shift
template <typename T, typename U>
SafeInt<U> operator <<(U lhs, SafeInt<T> bits)
{
	if(bits.IsSigned())
        assert(bits.Value() >= 0);

	assert(bits.Value() < SafeInt<U>::BitCount());

	return SafeInt<U>((U)(lhs << bits.Value()));
}

//right shift
template <typename T, typename U>
SafeInt<U> operator >>(U lhs, SafeInt<T> bits)
{
	if(bits.IsSigned())
        assert(bits.Value() >= 0);

	assert(bits.Value() < SafeInt<U>::BitCount());

	return SafeInt<U>((U)(lhs >> bits.Value()));
}

//bitwise operators
//this only makes sense if we're dealing with the same type and size
//demand a type T, or something that fits into a type T

//bitwise &
template <typename T, typename U>
SafeInt<T> operator &(U lhs, SafeInt<T> rhs)
{
	//if U can fit into T without truncating, force U to T
	return SafeInt<T>(SafeInt<T>(lhs) & rhs.Value());
}

//bitwise XOR
template <typename T, typename U>
SafeInt<T> operator ^(U lhs, SafeInt<T> rhs)
{
	return SafeInt<T>(SafeInt<T>(lhs) ^ rhs.Value());
}

//bitwise OR
template <typename T, typename U>
SafeInt<T> operator |(U lhs, SafeInt<T> rhs)
{
	return SafeInt<T>(SafeInt<T>(lhs) | rhs.Value());
}

//logical operators
//logical OR
template <typename T>
bool operator ||(bool lhs, SafeInt<T> rhs) 
{
	return lhs || rhs.Value();
}

template <typename T, typename U>
bool operator ||(U lhs, SafeInt<T> rhs) 
{
	return lhs || rhs.Value();
}

template <typename T, typename U>
bool operator ||(SafeInt<U> lhs, SafeInt<T> rhs) 
{
	return lhs.Value() || rhs.Value();
}

//logical AND
template <typename T>
bool operator &&(bool lhs, SafeInt<T> rhs) 
{
	return lhs && rhs.Value();
}

template <typename T, typename U>
bool operator &&(U lhs, SafeInt<T> rhs) 
{
	return lhs && rhs.Value();
}

template <typename T, typename U>
bool operator &&(SafeInt<U> lhs, SafeInt<T> rhs) 
{
	return lhs.Value() && rhs.Value();
}

#pragma warning(pop)


#endif //SAFEINT_HPP
