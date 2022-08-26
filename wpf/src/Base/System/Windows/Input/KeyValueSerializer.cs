//---------------------------------------------------------------------------
//
// File: KeyValueSerializer.cs
//
// Description:
//
//      KeyValueSerializer: Serializes a key string to a string and vice-versa
//
// Features:
//
// History:
//  08/04/2005       created: Chuck Jazdzewski
//
// Copyright (C) 2005 by Microsoft Corporation.  All rights reserved.
// 
//---------------------------------------------------------------------------

using System;
using System.ComponentModel;    // for TypeConverter
using System.Globalization;     // for CultureInfo
using System.Reflection;
using System.Windows;
using System.Windows.Input;
using System.Windows.Markup;
using System.Security.Permissions;
using MS.Utility;

namespace System.Windows.Input
{
    /// <summary>
    /// Key Serializer class for serializing a Key
    /// </summary>
    /// <ExternalAPI/> 
    public class KeyValueSerializer : ValueSerializer
    {
        /// <summary>
        /// CanConvertFromString()
        /// </summary>
        /// <param name="value"></param>
        /// <param name="context"></param>
        /// <returns></returns>
        /// <ExternalAPI/> 
        public override bool CanConvertFromString(string value, IValueSerializerContext context) 
        {
            return true;
        }

        /// <summary>
        /// CanConvertToString()
        /// </summary>
        /// <param name="value"></param>
        /// <param name="context"></param>
        /// <returns></returns>
        /// <ExternalAPI/> 
        public override bool CanConvertToString(object value, IValueSerializerContext context) 
        {
            if (!(value is Key))
                return false;
            Key key = (Key)value;
            return ((int)key >= (int)Key.None && (int)key <= (int)Key.OemClear);
        }

        /// <summary>
        /// ConvertFromString()
        /// </summary>
        /// <param name="value"></param>
        /// <param name="context"></param>
        /// <returns></returns>
        public override object ConvertFromString(string value, IValueSerializerContext context) 
        {
            TypeConverter converter = TypeDescriptor.GetConverter(typeof(Key));
            if (converter != null)
                return converter.ConvertFromString(value);
            else
                return base.ConvertFromString(value, context);
        }

        /// <summary>
        /// ConvertToString()
        /// </summary>
        /// <param name="value"></param>
        /// <param name="context"></param>
        /// <returns></returns>
        public override string ConvertToString(object value, IValueSerializerContext context) 
        {
            TypeConverter converter = TypeDescriptor.GetConverter(typeof(Key));
            if (converter != null)
                return converter.ConvertToInvariantString(value);
            else
                return base.ConvertToString(value, context);
        }
    }
}

