// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  MiniParameterInfo
**
** Purpose: Represents a method parameter.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;
using System.AddIn.MiniReflection.MetadataReader;
using System.Diagnostics.Contracts;

namespace System.AddIn.MiniReflection
{
    [Serializable]
    internal sealed class MiniParameterInfo
    {
        private CorElementType _elementType;
        private TypeInfo _paramType;

        public MiniParameterInfo(CorElementType elementType)
        {
            System.Diagnostics.Contracts.Contract.Requires(elementType <= CorElementType.String || 
                elementType == CorElementType.I || elementType == CorElementType.U ||
                elementType == CorElementType.Object);

            _elementType = elementType;
        }

        public MiniParameterInfo(CorElementType elementType, TypeInfo parameterType)
        {
            System.Diagnostics.Contracts.Contract.Requires(parameterType != null);
            System.Diagnostics.Contracts.Contract.Requires(IsValid(elementType));

            _elementType = elementType;
            _paramType = parameterType;
        }

        // To avoid reflection with Enum.IsDefined
        [Pure]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Justification="Needed for Contracts conditional compilation")]
        public static bool IsValid(CorElementType elementType)
        {
            return (CorElementType.End <= elementType && elementType <= CorElementType.Max) ||
                elementType == CorElementType.Modifier ||
                elementType == CorElementType.Sentinel ||
                elementType == CorElementType.Pinned;
        }

        /*
        // This lets Reflection Type instances sneak into our MiniParameterInfo's.
        internal MiniParameterInfo(TypeInfo parameterType)
        {
            Contract.Requires(parameterType != null);

            // This isn't technically correct, but we don't need it to be 
            // perfectly accurate because the CorElementType never leaks 
            // out of this class.
            _elementType = CorElementType.Class;
            _paramType = parameterType;
        }
         */

        public TypeInfo ParameterType
        {
            get
            {
                if (_elementType == CorElementType.Class || _elementType == CorElementType.ValueType)
                    return _paramType;

                switch (_elementType)
                {
                    case CorElementType.Boolean:
                        return new TypeInfo(typeof(Boolean));
                    case CorElementType.I1:
                        return new TypeInfo(typeof(SByte));
                    case CorElementType.U1:
                        return new TypeInfo(typeof(Byte));
                    case CorElementType.I2:
                        return new TypeInfo(typeof(Int16));
                    case CorElementType.U2:
                        return new TypeInfo(typeof(UInt16));
                    case CorElementType.Char:
                        return new TypeInfo(typeof(Char));
                    case CorElementType.I4:
                        return new TypeInfo(typeof(Int32));
                    case CorElementType.U4:
                        return new TypeInfo(typeof(UInt32));
                    case CorElementType.I8:
                        return new TypeInfo(typeof(Int64));
                    case CorElementType.U8:
                        return new TypeInfo(typeof(UInt64));
                    case CorElementType.R4:
                        return new TypeInfo(typeof(Single));
                    case CorElementType.R8:
                        return new TypeInfo(typeof(Double));
                    case CorElementType.I:
                        return new TypeInfo(typeof(IntPtr));
                    case CorElementType.U:
                        return new TypeInfo(typeof(UIntPtr));                    

                    case CorElementType.String:
                        return new TypeInfo(typeof(String));

                    case CorElementType.Object:
                        return new TypeInfo(typeof(Object));

                    default:
                        System.Diagnostics.Contracts.Contract.Assert(false, "Unrecognized CorElementType.");
                        throw new NotImplementedException();
                }
            }
        }

        public override string ToString()
        {
            if (_elementType == CorElementType.Class || _elementType == CorElementType.ValueType)
                return _paramType.ToString();
            else
                return _elementType.ToString();
        }
    }
}
