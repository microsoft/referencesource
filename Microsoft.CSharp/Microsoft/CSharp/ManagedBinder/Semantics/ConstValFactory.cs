// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ConstValFactory owns the construction of CONSTVALs
    // This should be the only place in the code which dynamically
    // allocates memory for CONSTVALs.
    //
    // One important client of ConstValFactory is the lexer/parser,
    // so ConstValFactory should not take a dependancy on TYPESYMs
    // or predefined types.
    //
    // REFACTOR: Update the Parser to use this class.
    // PERFORMANCE: Consider inlining some of these methods.
    // CONSIDER:  Pointer members of CONSTVAL should point to const values.
    // PERFORMANCE: Optimize copying of zero values to not alloc memory.
    // PERFORMANCE: Optimize copying of values allocated on same allocator.
    // CONSIDER: Consider uniquefying constants based on value.

    internal sealed class ConstValFactory
    {
        public ConstValFactory()
        { }

        public CONSTVAL Copy(ConstValKind kind, CONSTVAL value)
        {
            return new CONSTVAL(value.objectVal);
        }

        public static CONSTVAL GetDefaultValue(ConstValKind kind)
        {
            CONSTVAL result = new CONSTVAL();

            switch (kind)
            {
                case ConstValKind.Int:
                    result.iVal = 0;
                    break;

                case ConstValKind.Double:
                    result.doubleVal = 0;
                    break;

                case ConstValKind.Long:
                    result.longVal = 0;
                    break;

                case ConstValKind.Decimal:
                    result.decVal = 0;
                    break;

                case ConstValKind.Float:
                    result.floatVal = 0;
                    break;

                case ConstValKind.Boolean:
                    result.boolVal = false;
                    break;
            }

            return result;
        }

        public static CONSTVAL GetNullRef()
        {
            return new CONSTVAL();
        }

        public static CONSTVAL GetBool(bool value)
        {
            CONSTVAL result = new CONSTVAL();
            result.boolVal = value;
            return result;
        }

        public static CONSTVAL GetInt(int value)
        {
            CONSTVAL result = new CONSTVAL(); ;
            result.iVal = value;
            return result;
        }

        public static CONSTVAL GetUInt(uint value)
        {
            CONSTVAL result = new CONSTVAL();
            result.uiVal = value;
            return result;
        }

        public CONSTVAL Create(decimal value)
        {
            CONSTVAL result = new CONSTVAL();
            result.decVal = value;
            return result;
        }

        public CONSTVAL Create(string value)
        {
            CONSTVAL result = new CONSTVAL();
            result.strVal = value;
            return result;
        }

        public CONSTVAL Create(float value)
        {
            CONSTVAL result = new CONSTVAL();
            result.floatVal = value;
            return result;
        }

        public CONSTVAL Create(double value)
        {
            CONSTVAL result = new CONSTVAL();
            result.doubleVal = value;
            return result;
        }

        public CONSTVAL Create(long value)
        {
            CONSTVAL result = new CONSTVAL();
            result.longVal = value;
            return result;
        }

        public CONSTVAL Create(ulong value)
        {
            CONSTVAL result = new CONSTVAL();
            result.ulongVal = value;
            return result;
        }

        internal CONSTVAL Create(bool value)
        {
            CONSTVAL result = new CONSTVAL();
            result.boolVal = value;
            return result;
        }

        internal CONSTVAL Create(object p)
        {
            CONSTVAL result = new CONSTVAL();
            result.objectVal = p;
            return result;
        }
    }
}
