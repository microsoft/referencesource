//------------------------------------------------------------------------------
/// <copyright from='2004' to='2005' company='Microsoft Corporation'>
///    Copyright (c) Microsoft Corporation. All Rights Reserved.
///    Information Contained Herein is Proprietary and Confidential.
/// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Diagnostics.CodeAnalysis;

namespace System.AddIn.Contract
{
    /// <summary>
    /// Empty == System.Reflection.Missing
    /// Intrinsic == All Primitive types, System.String
    /// </summary>
    public enum RemoteArgumentKind
    {
        Missing,
        Intrinsic,
        IntrinsicArray,
        Contract
    }

    /// <summary>
    /// Similar to the COM Variant type, this class can be used to safely
    /// pass Contract compliant types through the IRemoteTypeContract
    /// RemoteArgument passes, by value, the "Convertible" types.
    /// These are: int, uint, long, ulong, short, ushort, sbyte, byte, char
    /// decimal, float, double, bool, DateTime, DBNull, Empty and String
    /// </summary>
    [Serializable]
    [SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes")]
    public struct RemoteArgument
    {
        #region Private Fields
        //this ("value") clearly breaks the rule of "tranistive closure" by
        //having a field typed as System.Object. We re-enforce the rule
        //by ensuring the only things that can be assigned here are either
        //intrinsic types or IContracts, and *never* exposing the object.
        //This should be the *ONLY* exception, ever, and allows the rule
        //to be enforced everywhere else
        private object _value;

        //byref is changeable, can be set and unset....
        private bool isByRef;

        //the following two fields are "readonly" because once they
        //are set they are not allowed to change.
        private readonly RemoteArgumentKind remoteArgKind;
        private readonly TypeCode intrinsicTypeCode;
        #endregion

        #region Static Creation Methods
        /// <summary>
        /// With all the various intrinsic types, in order to avoid each consumer
        /// of this method having to write the large switch statement to decide which constructor
        /// to call, we wrote it for you in this helper method. All primitive types
        /// implement IConvertible, as do enum types, which fit into integral primitives
        /// String *is* included here though it is not strictly a primitive type.
        /// Anything with its own type code is included. Anything with TypeCode.Object throws
        /// </summary>
        /// <param name="intrinsicValue"></param>
        /// <returns></returns>
        public static RemoteArgument CreateRemoteArgument(object value)
        {
            return CreateRemoteArgument(value, false);
        }

        /// <summary>
        /// This overload allows the caller to create a byref remote argument
        /// </summary>
        /// <param name="intrinsicValue"></param>
        /// <param name="isByRef"></param>
        /// <returns></returns>
        public static RemoteArgument CreateRemoteArgument(object value, bool isByRef)
        {
            if (value == null)
                throw new ArgumentNullException("value");

            System.Array arrayValue = value as System.Array;
            TypeCode typeCode;
            if (arrayValue != null)
            {
                typeCode = Type.GetTypeCode(arrayValue.GetType().GetElementType());
            }
            else
            {
                typeCode = Type.GetTypeCode(value.GetType());
            }

            //else its System.Reflection.Missing, so leave as TypeCode.Empty
            return CreateRemoteArgument(value, isByRef, typeCode);
        }

        /// <summary>
        /// This overload allows the caller to convert between intrinsic types. The returned
        /// remote argument will contain the type code specified or this will throw.
        /// </summary>
        /// <param name="intrinsicValue"></param>
        /// <param name="isByRef"></param>
        /// <param name="typeCodeToUse"></param>
        /// <returns></returns>
        [SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity")]
        public static RemoteArgument CreateRemoteArgument(object value, bool isByRef, TypeCode typeCodeToUse)
        {
            IConvertible convertibleValue = value as IConvertible;
            IContract contractValue = value as IContract;
            System.Array arrayValue = value as System.Array;

            if ((typeCodeToUse == TypeCode.Object) && (contractValue == null) && (value != null))
            {
                // If the TypeCode is object, we better have a Contract unless the value itself
                // was null to begin with.  In this case, we just want a RemoteArgument for a
                // null contract.
                throw new ArgumentException(null, "value");
            }
            else if ((typeCodeToUse == TypeCode.Empty) && (value != System.Reflection.Missing.Value))
            {
                // If the TypeCode is Empty the only valid value is Missing.Value.
                throw new ArgumentException(null, "value");
            }
            else if ((convertibleValue == null) && (value != null) && (arrayValue == null) && (typeCodeToUse != TypeCode.Object) && (typeCodeToUse != TypeCode.Empty))
            {
                // All the rest of the TypeCodes are for Convertible values, so we better
                // have one unless of course the value itself was null (i.e. a null string).
                // In this case, null is completely valid and we should create a null
                // string RemoteArgument.
                throw new ArgumentException(null, "value");
            }

            if (arrayValue != null)
            {
                TypeCode arrayTypeCode = Type.GetTypeCode(arrayValue.GetType().GetElementType());
                if ((arrayTypeCode == TypeCode.Object) || (arrayTypeCode != typeCodeToUse))
                    throw new ArgumentException(null, "value");

                return new RemoteArgument(arrayValue, isByRef);
            }

            bool needsConversion = false;
            if (convertibleValue != null)
                needsConversion = (convertibleValue.GetTypeCode() != typeCodeToUse);

            //all of this seemingly duplicated code
            //is necessary to unbox the value and call the
            //correct constructor on RemoteArgument
            switch (typeCodeToUse)
            {
                case TypeCode.Boolean:
                    {
                        bool boolToUse;
                        if (needsConversion)
                            boolToUse = convertibleValue.ToBoolean(null);
                        else
                            boolToUse = (bool)convertibleValue;
                        return new RemoteArgument(boolToUse, isByRef);
                    }
                case TypeCode.Byte:
                    {
                        byte byteToUse;
                        if (needsConversion)
                            byteToUse = convertibleValue.ToByte(null);
                        else
                            byteToUse = (byte)convertibleValue;
                        return new RemoteArgument(byteToUse, isByRef);
                    }
                case TypeCode.Char:
                    {
                        char charToUse;
                        if (needsConversion)
                            charToUse = convertibleValue.ToChar(null);
                        else
                            charToUse = (char)convertibleValue;
                        return new RemoteArgument(charToUse, isByRef);
                    }
                case TypeCode.DateTime:
                    {
                        DateTime dateTimeToUse;
                        if (needsConversion)
                            dateTimeToUse = convertibleValue.ToDateTime(null);
                        else
                            dateTimeToUse = (DateTime)convertibleValue;
                        return new RemoteArgument(dateTimeToUse, isByRef);
                    }
                case TypeCode.DBNull:
                    {
                        //there is no conversion to DBNull
                        //its either DBNull or not, so
                        //throw if needs conversion is true
                        if (needsConversion)
                            throw new NotSupportedException();
                        return new RemoteArgument((DBNull)convertibleValue);
                    }
                case TypeCode.Decimal:
                    {
                        decimal decimalToUse;
                        if (needsConversion)
                            decimalToUse = convertibleValue.ToDecimal(null);
                        else
                            decimalToUse = (decimal)convertibleValue;
                        return new RemoteArgument(decimalToUse, isByRef);
                    }
                case TypeCode.Double:
                    {
                        double doubleToUse;
                        if (needsConversion)
                            doubleToUse = convertibleValue.ToDouble(null);
                        else
                            doubleToUse = (double)convertibleValue;
                        return new RemoteArgument(doubleToUse, isByRef);
                    }
                case TypeCode.Int16:
                    {
                        short shortToUse;
                        if (needsConversion)
                            shortToUse = convertibleValue.ToInt16(null);
                        else
                            shortToUse = (short)convertibleValue;
                        return new RemoteArgument(shortToUse, isByRef);
                    }
                case TypeCode.Int32:
                    {
                        int intToUse;
                        if (needsConversion)
                            intToUse = convertibleValue.ToInt32(null);
                        else
                            intToUse = (int)convertibleValue;
                        return new RemoteArgument(intToUse, isByRef);
                    }
                case TypeCode.Int64:
                    {
                        long longToUse;
                        if (needsConversion)
                            longToUse = convertibleValue.ToInt64(null);
                        else
                            longToUse = (long)convertibleValue;
                        return new RemoteArgument(longToUse, isByRef);
                    }
                case TypeCode.SByte:
                    {
                        sbyte sbyteToUse;
                        if (needsConversion)
                            sbyteToUse = convertibleValue.ToSByte(null);
                        else
                            sbyteToUse = (sbyte)convertibleValue;
                        return new RemoteArgument(sbyteToUse, isByRef);
                    }
                case TypeCode.Single:
                    {
                        float floatToUse;
                        if (needsConversion)
                            floatToUse = convertibleValue.ToSingle(null);
                        else
                            floatToUse = (float)convertibleValue;
                        return new RemoteArgument(floatToUse, isByRef);
                    }
                case TypeCode.String:
                    {
                        string stringToUse;
                        if (needsConversion)
                            stringToUse = convertibleValue.ToString(null);
                        else
                            stringToUse = (string)convertibleValue;
                        return new RemoteArgument(stringToUse, isByRef);
                    }
                case TypeCode.UInt16:
                    {
                        ushort ushortToUse;
                        if (needsConversion)
                            ushortToUse = convertibleValue.ToUInt16(null);
                        else
                            ushortToUse = (ushort)convertibleValue;
                        return new RemoteArgument(ushortToUse, isByRef);
                    }
                case TypeCode.UInt32:
                    {
                        uint uintToUse;
                        if (needsConversion)
                            uintToUse = convertibleValue.ToUInt32(null);
                        else
                            uintToUse = (uint)convertibleValue;
                        return new RemoteArgument(uintToUse, isByRef);
                    }
                case TypeCode.UInt64:
                    {
                        ulong ulongToUse;
                        if (needsConversion)
                            ulongToUse = convertibleValue.ToUInt64(null);
                        else
                            ulongToUse = (ulong)convertibleValue;
                        return new RemoteArgument(ulongToUse, isByRef);
                    }
                case TypeCode.Empty:
                    {
                        // Just Missing.Value
                        return new RemoteArgument(RemoteArgumentKind.Missing, TypeCode.Empty, isByRef);
                    }
                case TypeCode.Object:
                    {
                        return new RemoteArgument(contractValue, isByRef);
                    }
                default:
                    throw new InvalidOperationException();
            }
        }
        #endregion

        #region Initializers
        //the "default constructor" of RemoteArgument is
        //built-in because this is a struct.
        //it creates the "Null" RemoteArgument:
        //value = null;
        //remoteArgKing = RemoteArgumentKind.Missing
        //intrinsicTypeCode = TypeCode.Empty

        #region Contract
        //Creates the Contract RemoteArgument
        public RemoteArgument(IContract value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Contract;
            this.intrinsicTypeCode = TypeCode.Object;
            this.isByRef = false;
        }

        public RemoteArgument(IContract value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Contract;
            this.intrinsicTypeCode = TypeCode.Object;
            this.isByRef = isByRef;
        }
        #endregion

        #region Special case for null/"out" params and/or default value intrinsics
        //isByRef == true means "out". Must be true if Kind == Contract
        public RemoteArgument(RemoteArgumentKind remoteArgKind, TypeCode typeCode) : this(remoteArgKind, typeCode, false)
        {
        }

        public RemoteArgument(RemoteArgumentKind remoteArgKind, TypeCode typeCode, bool isByRef)
        {
            this._value = null;
            this.isByRef = isByRef;
            this.remoteArgKind = remoteArgKind;
            switch (remoteArgKind)
            {
                case RemoteArgumentKind.Missing:
                    if (typeCode != TypeCode.Empty)
                        throw new ArgumentException(null, "typeCode");
                    this.intrinsicTypeCode = typeCode;
                    break;
                case RemoteArgumentKind.Intrinsic:
                case RemoteArgumentKind.IntrinsicArray:
                    if (typeCode == TypeCode.Object ||
                        typeCode == TypeCode.Empty)
                        throw new ArgumentException(null, "typeCode");
                    this.intrinsicTypeCode = typeCode;
                    break;

                case RemoteArgumentKind.Contract:
                    if (typeCode != TypeCode.Object)
                        throw new ArgumentException(null, "typeCode");
                    this.intrinsicTypeCode = typeCode;
                    break;

                default:
                    throw new InvalidOperationException();
            }
        }
        #endregion

        #region Primitive Types -- CLS Compliant
        //two for each of the primitive types
        //value, by ref

        #region System.Boolean
        public RemoteArgument(System.Boolean value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Boolean;
            this.isByRef = false;
        }

        public RemoteArgument(System.Boolean value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Boolean;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.Byte
        public RemoteArgument(System.Byte value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Byte;
            this.isByRef = false;
        }

        public RemoteArgument(System.Byte value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Byte;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.Char
        public RemoteArgument(System.Char value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Char;
            this.isByRef = false;
        }

        public RemoteArgument(System.Char value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Char;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.DateTime
        public RemoteArgument(System.DateTime value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.DateTime;
            this.isByRef = false;
        }

        public RemoteArgument(System.DateTime value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.DateTime;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.DBNull
        public RemoteArgument(System.DBNull value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.DBNull;
            this.isByRef = false;
        }

        public RemoteArgument(System.DBNull value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.DBNull;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.Decimal
        public RemoteArgument(System.Decimal value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Decimal;
            this.isByRef = false;
        }

        public RemoteArgument(System.Decimal value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Decimal;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.Double
        public RemoteArgument(System.Double value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Double;
            this.isByRef = false;
        }

        public RemoteArgument(System.Double value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Double;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.Int16
        public RemoteArgument(System.Int16 value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Int16;
            this.isByRef = false;
        }

        public RemoteArgument(System.Int16 value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Int16;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.Int32
        public RemoteArgument(System.Int32 value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Int32;
            this.isByRef = false;
        }

        public RemoteArgument(System.Int32 value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Int32;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.Int64
        public RemoteArgument(System.Int64 value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Int64;
            this.isByRef = false;
        }

        public RemoteArgument(System.Int64 value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Int64;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.Single
        public RemoteArgument(System.Single value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Single;
            this.isByRef = false;
        }

        public RemoteArgument(System.Single value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.Single;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.String
        public RemoteArgument(System.String value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.String;
            this.isByRef = false;
        }

        public RemoteArgument(System.String value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.String;
            this.isByRef = isByRef;
        }
        #endregion
        #endregion

        #region Intrinsic Types Non-CLS Compliant
        #region System.SByte
        [CLSCompliant(false)]
        public RemoteArgument(System.SByte value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.SByte;
            this.isByRef = false;
        }

        [CLSCompliant(false)]
        public RemoteArgument(System.SByte value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.SByte;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.UInt16
        [CLSCompliant(false)]
        public RemoteArgument(System.UInt16 value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.UInt16;
            this.isByRef = false;
        }

        [CLSCompliant(false)]
        public RemoteArgument(System.UInt16 value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.UInt16;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.UInt32
        [CLSCompliant(false)]
        public RemoteArgument(System.UInt32 value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.UInt32;
            this.isByRef = false;
        }

        [CLSCompliant(false)]
        public RemoteArgument(System.UInt32 value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.UInt32;
            this.isByRef = isByRef;
        }
        #endregion

        #region System.UInt64
        [CLSCompliant(false)]
        public RemoteArgument(System.UInt64 value)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.UInt64;
            this.isByRef = false;
        }

        [CLSCompliant(false)]
        public RemoteArgument(System.UInt64 value, bool isByRef)
        {
            this._value = value;
            this.remoteArgKind = RemoteArgumentKind.Intrinsic;
            this.intrinsicTypeCode = TypeCode.UInt64;
            this.isByRef = isByRef;
        }
        #endregion
        #endregion

        #region IntrinsicArray
        //we could have overloads for each of the primitive
        //type arrays, but arrays can be multidimensional
        //so we'd need this as a catchall anyway.
        //So this throws if the array element type isn't intrinsic
        //NOTE we don't support System.Reflection.Missing[]
        //makes no sense....
        public RemoteArgument(System.Array array)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            //this handles multi-dimensional intrinsic arrays, too.
            //Jagged arrays are handled recursively as arrays of arrays
            //we can't just ask the type if it IsPrimitive because string says "false"
            if (Type.GetTypeCode(array.GetType().GetElementType()) == TypeCode.Object)
            {
                throw new ArgumentException(null, "array");
            }

            this._value = array;
            this.remoteArgKind = RemoteArgumentKind.IntrinsicArray;
            //for IntrinsicArray this holds the element type code
            this.intrinsicTypeCode = Type.GetTypeCode(array.GetType().GetElementType());
            this.isByRef = false;
        }

        public RemoteArgument(System.Array array, bool isByRef)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            //this handles multi-dimensional intrinsic arrays, too.
            //Jagged arrays are handled recursively as arrays of arrays
            //we can't just ask the type if it IsPrimitive because string says "false"
            if (Type.GetTypeCode(array.GetType().GetElementType()) == TypeCode.Object)
            {
                throw new ArgumentException(null, "array");
            }

            this._value = array;
            this.remoteArgKind = RemoteArgumentKind.IntrinsicArray;
            //for IntrinsicArray this holds the element type code
            this.intrinsicTypeCode = Type.GetTypeCode(array.GetType().GetElementType());
            this.isByRef = isByRef;
        }
        #endregion
        #endregion

        #region Public Accessors
        public RemoteArgumentKind RemoteArgumentKind
        {
            get
            {
                return this.remoteArgKind;
            }
        }

        public TypeCode TypeCode
        {
            get
            {
                return this.intrinsicTypeCode;
            }
        }

        public bool IsByRef
        {
            get
            {
                return this.isByRef;
            }
            set
            {
                this.isByRef = value;
            }
        }

        #region Type Specific Value Properties

        /// <summary>
        /// All of these properties do the same thing:
        /// On get, validate return and typecode, return if
        /// appropriate, else throw InvalidOperationException
        /// On set, isByRef MUST be true, then validate kind
        /// and typecode and set value if appropriate, else
        /// throw InvalidOperationException
        /// </summary>
        #region Contract
        public IContract ContractValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Contract &&
                   this.intrinsicTypeCode == TypeCode.Object)
                {
                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (IContract)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Contract &&
                   this.intrinsicTypeCode == TypeCode.Object)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.Reflection.Missing
        public System.Reflection.Missing MissingValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Missing &&
                    this.intrinsicTypeCode == TypeCode.Empty)
                {
                    //only one possible return value...
                    return System.Reflection.Missing.Value;
                }

                throw new InvalidOperationException();
            }
        }
        #endregion

        #region Primitive Types -- CLS Compliant
        /// <remarks>
        /// A Note on the implementation: We could have used
        /// a generic helper function to eliminate so much duplicated
        /// code. We chose not to to avoid misuse of the generic argument
        /// Explicitly coding all of these leaves nothing to interpretation
        /// on the use of a function.
        /// </remarks>

        #region System.Boolean
        public System.Boolean BooleanValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Boolean)
                {
                    if (this._value == null)
                        return default(System.Boolean);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.Boolean)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Boolean)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.Byte
        public System.Byte ByteValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Byte)
                {
                    if (this._value == null)
                        return default(System.Byte);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.Byte)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Byte)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.Char
        public System.Char CharValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Char)
                {
                    if (this._value == null)
                        return default(System.Char);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.Char)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Char)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.DateTime
        public System.DateTime DateTimeValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.DateTime)
                {
                    if (this._value == null)
                        return default(System.DateTime);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.DateTime)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.DateTime)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.DBNull
        public System.DBNull DBNullValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.DBNull)
                {
                    if (this._value == null)
                        return default(System.DBNull);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.DBNull)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.DBNull)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.Decimal
        public System.Decimal DecimalValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Decimal)
                {
                    if (this._value == null)
                        return default(System.Decimal);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.Decimal)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Decimal)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.Double
        public System.Double DoubleValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Double)
                {
                    if (this._value == null)
                        return default(System.Double);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.Double)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Double)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.Int16
        public System.Int16 Int16Value
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Int16)
                {
                    if (this._value == null)
                        return default(System.Int16);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.Int16)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Int16)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.Int32
        public System.Int32 Int32Value
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Int32)
                {
                    if (this._value == null)
                        return default(System.Int32);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.Int32)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Int32)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.Int64
        public System.Int64 Int64Value
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Int64)
                {
                    if (this._value == null)
                        return default(System.Int64);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.Int64)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Int64)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.Single
        public System.Single SingleValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Single)
                {
                    if (this._value == null)
                        return default(System.Single);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.Single)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.Single)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.String
        public System.String StringValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.String)
                {
                    if (this._value == null)
                        return default(System.String);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.String)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.String)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion
        #endregion

        #region Intrinsic Types -- Non-CLS Compliant
        #region System.SByte
        [CLSCompliant(false)]
        public System.SByte SByteValue
        {
            get
            {
                if(this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.SByte)
                {
                    if (this._value == null)
                        return default(System.SByte);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.SByte)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if(this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.SByte)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.UInt16
        [CLSCompliant(false)]
        public System.UInt16 UInt16Value
        {
            get
            {
                if(this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.UInt16)
                {
                    if (this._value == null)
                        return default(System.UInt16);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.UInt16)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if(this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.UInt16)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.UInt32
        [CLSCompliant(false)]
        public System.UInt32 UInt32Value
        {
            get
            {
                if(this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.UInt32)
                {
                    if (this._value == null)
                        return default(System.UInt32);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.UInt32)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if(this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.UInt32)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion

        #region System.UInt64
        [CLSCompliant(false)]
        public System.UInt64 UInt64Value
        {
            get
            {
                if(this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.UInt64)
                {
                    if (this._value == null)
                        return default(System.UInt64);

                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.UInt64)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if(this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.Intrinsic &&
                   this.intrinsicTypeCode == TypeCode.UInt64)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion
        #endregion

        #region IntrinsicArray
        [SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly")]
        public System.Array ArrayValue
        {
            get
            {
                if (this.remoteArgKind == RemoteArgumentKind.IntrinsicArray &&
                   this.intrinsicTypeCode != TypeCode.Object)
                {
                    //do explicit cast here instead of "as"
                    //because we *want* it to throw if it is a
                    //bad cast. It would mean an invalid state
                    //of the struct.
                    return (System.Array)this._value;
                }

                throw new InvalidOperationException();
            }
            set
            {
                if (this.isByRef == true &&
                   this.remoteArgKind == RemoteArgumentKind.IntrinsicArray &&
                   this.intrinsicTypeCode != TypeCode.Object)
                {
                    this._value = value;
                }
                else
                {
                    throw new InvalidOperationException();
                }
            }
        }
        #endregion
        #endregion
        #endregion
    }
}
