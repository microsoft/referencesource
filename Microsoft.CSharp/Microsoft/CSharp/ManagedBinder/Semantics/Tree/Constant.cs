// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;
using System.Text;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRCONSTANT : EXPR
    {
        public EXPR OptionalConstructorCall;
        public EXPR GetOptionalConstructorCall() { return OptionalConstructorCall; }
        public void SetOptionalConstructorCall(EXPR value) { OptionalConstructorCall = value; }

        private CONSTVAL val;

        public bool IsZero
        {
            get
            {
                return Val.IsZero(this.type.constValKind());
            }
        }
        public bool isZero() { return IsZero; }
        public CONSTVAL getVal() { return Val; }
        public void setVal(CONSTVAL newValue) { Val = newValue; }
        public CONSTVAL Val
        {
            get
            {
                return val;
            }
            set
            {
                val = value;
            }
        }

        public ulong getU64Value() { return val.ulongVal; }
        public long getI64Value() { return I64Value; }
        public long I64Value
        {
            get
            {
                FUNDTYPE ft = type.fundType();
                switch (ft)
                {
                    case FUNDTYPE.FT_I8:
                    case FUNDTYPE.FT_U8:
                        return val.longVal;
                    case FUNDTYPE.FT_U4:
                        return val.uiVal;
                    case FUNDTYPE.FT_I1:
                    case FUNDTYPE.FT_I2:
                    case FUNDTYPE.FT_I4:
                    case FUNDTYPE.FT_U1:
                    case FUNDTYPE.FT_U2:
                        return val.iVal;
                    default:
                        Debug.Assert(false, "Bad fundType in getI64Value");
                        return 0;
                }
            }
        }
    }

}
