using System;

namespace System.AddIn.MiniReflection.MetadataReader
{
    [Serializable]
    internal enum CorElementType : byte
    {
        End = 0x00,
        Void = 0x01,
        Boolean = 0x02,
        Char = 0x03,
        I1 = 0x04,
        U1 = 0x05,
        I2 = 0x06,
        U2 = 0x07,
        I4 = 0x08,
        U4 = 0x09,
        I8 = 0x0A,
        U8 = 0x0B,
        R4 = 0x0C,
        R8 = 0x0D,
        String = 0x0E,
        Ptr = 0x0F,
        ByRef = 0x10,
        ValueType = 0x11,
        Class = 0x12,
        Var = 0x13,   // Generic class's type parameter.  class C<T> { M(T t) }
        Array = 0x14,
        GenericInst = 0x15,
        TypedByRef = 0x16,
        I = 0x18,
        U = 0x19,
        FnPtr = 0x1B,
        Object = 0x1C,
        SzArray = 0x1D,
        MVar = 0x1E, // Generic method's type parameter.  class C { M<T>(T t) }
        CModReqd = 0x1F,
        CModOpt = 0x20,
        Internal = 0x21,
        Max = 0x22,
        Modifier = 0x40,
        Sentinel = 0x41,
        Pinned = 0x45,
    }

    [Serializable, Flags()]
    internal enum MdSigCallingConvention : byte
    {
        CallConvMask = 0x0f,  // Calling convention is bottom 4 bits 

        Default = 0x00,
        C = 0x01,
        StdCall = 0x02,
        ThisCall = 0x03,
        FastCall = 0x04,
        Vararg = 0x05,
        Field = 0x06,
        LocalSig = 0x07,
        Property = 0x08,
        Unmgd = 0x09,
        GenericInst = 0x0a,  // generic method instantiation

        Generic = 0x10,  // Generic method sig with explicit number of type arguments (precedes ordinary parameter count)
        HasThis = 0x20,  // Top bit indicates a 'this' parameter    
        ExplicitThis = 0x40,  // This parameter is explicitly in the signature
    }

    // Attributes for metadata's File table (ECMA Spec section 23.1.6)
    // This is primarily for manifest resources & linked .netmodules.
    internal enum MDFileAttributes
    {
        ContainsMetaData = 0,
        ContainsNoMetaData = 1
    }
}
