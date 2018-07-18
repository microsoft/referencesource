// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    /*
     * The kind of allocation used in a constant value.
     * Since CONSTVALs don't store a discriminant for the union
     * this must be managed by clients.
     */
    enum ConstValKind
    {
        Int,
        Double,
        Long,
        String,
        Decimal,
        IntPtr,
        Float,
        Boolean,
        Lim
    };


    internal sealed class CONSTVAL
    {
        private object value;

        internal CONSTVAL()
            : this(null)
        {
        }

        internal CONSTVAL(object value)
        {
            this.value = value;
        }

        public object objectVal
        {
            get { return this.value; }
            set { this.value = value; }
        }

        public bool boolVal
        {
            get { return SpecialUnbox<bool>(this.value); }
            set { this.value = SpecialBox(value); }
        }

        public sbyte sbyteVal
        {
            get { return SpecialUnbox<sbyte>(this.value); }
            //set { this.value = SpecialBox(value); }
        }

        public byte byteVal
        {
            get { return SpecialUnbox<byte>(this.value); }
            //set { this.value = SpecialBox(value); }
        }

        public short shortVal
        {
            get { return SpecialUnbox<short>(this.value); }
            //set { this.value = SpecialBox(value); }
        }

        public ushort ushortVal
        {
            get { return SpecialUnbox<ushort>(this.value); }
            //set { this.value = SpecialBox(value); }
        }

        public int iVal
        {
            get { return SpecialUnbox<int>(this.value); }
            set { this.value = SpecialBox(value); }
        }

        public uint uiVal
        {
            get { return SpecialUnbox<uint>(this.value); }
            set { this.value = SpecialBox(value); }
        }

        public long longVal
        {
            get { return SpecialUnbox<long>(this.value); }
            set { this.value = SpecialBox(value); }
        }

        public ulong ulongVal
        {
            get { return SpecialUnbox<ulong>(this.value); }
            set { this.value = SpecialBox(value); }
        }

        public float floatVal
        {
            get { return SpecialUnbox<float>(this.value); }
            set { this.value = SpecialBox(value); }
        }

        public double doubleVal
        {
            get { return SpecialUnbox<double>(this.value); }
            set { this.value = SpecialBox(value); }
        }

        public decimal decVal
        {
            get { return SpecialUnbox<decimal>(this.value); }
            set { this.value = SpecialBox(value); }
        }

        public char cVal
        {
            get { return SpecialUnbox<char>(this.value); }
            //set { this.value = SpecialBox(value); }
        }

        public string strVal
        {
            get { return SpecialUnbox<string>(this.value); }
            set { this.value = SpecialBox(value); }
        }

        public bool IsNullRef()
        {
            return this.value == null;
        }

        public bool IsZero(ConstValKind kind)
        {
            switch (kind)
            {
                case ConstValKind.Decimal:
                    return decVal == 0;
                case ConstValKind.String:
                    return false;
                default:
                    return IsDefault(this.value);
            }
        }

        private T SpecialUnbox<T>(object o)
        {
            if (IsDefault(o))
            {
                return default(T);
            }

            return (T)Convert.ChangeType(o, typeof(T), System.Globalization.CultureInfo.InvariantCulture);
        }

        private object SpecialBox<T>(T x)
        {
            return x;
        }

        private bool IsDefault(object o)
        {
            if (o == null)
                return true;

            TypeCode code = Type.GetTypeCode(o.GetType());
            switch (code)
            {
                case TypeCode.Boolean:
                    return default(bool).Equals(o);
                case TypeCode.SByte:
                    return default(sbyte).Equals(o);
                case TypeCode.Byte:
                    return default(byte).Equals(o);
                case TypeCode.Int16:
                    return default(short).Equals(o);
                case TypeCode.UInt16:
                    return default(ushort).Equals(o);
                case TypeCode.Int32:
                    return default(int).Equals(o);
                case TypeCode.UInt32:
                    return default(uint).Equals(o);
                case TypeCode.Int64:
                    return default(long).Equals(o);
                case TypeCode.UInt64:
                    return default(ulong).Equals(o);
                case TypeCode.Single:
                    return default(float).Equals(o);
                case TypeCode.Double:
                    return default(double).Equals(o);
                case TypeCode.Decimal:
                    return default(decimal).Equals(o);
                case TypeCode.Char:
                    return default(char).Equals(o);
            }

            return false;
        }
    }
}
