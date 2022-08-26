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
    internal sealed class MiniCustomAttributeInfo
    {
        private String _typeName;

        private MiniCustomAttributeFixedArgInfo[] _fixedArgs;
        private MiniCustomAttributeNamedArgInfo[] _namedArgs;

        public MiniCustomAttributeInfo(String typeName, MiniCustomAttributeFixedArgInfo[] fixedArgs, 
                MiniCustomAttributeNamedArgInfo[] namedArgs)
        {
            _typeName = typeName;
            _fixedArgs = fixedArgs;
            _namedArgs = namedArgs;
        }

        /*
        public String TypeName
        {
            get { return _typeName; }
        }
        */

        public MiniCustomAttributeFixedArgInfo[] FixedArgs {
            get { return _fixedArgs; }
        }

        public MiniCustomAttributeNamedArgInfo[] NamedArgs {
            get { return _namedArgs; }
        }
    }

    [Serializable]
    internal sealed class MiniCustomAttributeNamedArgInfo
    {
        private String _argName;
        private CorElementType _type;
        private Object _value;

        public MiniCustomAttributeNamedArgInfo(CorElementType type, String name, Object value)
        {
            _argName = name;
            _type = type;
            _value = value;
        }

        public Object Value {
            get { return _value; }
        }

        public String Name
        {
            get {return _argName; }
        }

        /*
        public CorElementType CorElementType
        {
            get { return _type; }
        }
        */
    }

    [Serializable]
    internal sealed class MiniCustomAttributeFixedArgInfo
    {
        private Object _value;

        public MiniCustomAttributeFixedArgInfo(Object value)
        {
            _value = value;
        }

        public Object Value {
            get { return _value; }
        }
    }
}


