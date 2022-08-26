//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation, 2005
//
//  File:      RoutedEventValueSerializer.cs
//
//  Contents:  Value serializer for the RoutedEvent class
//
//  Created:   04/28/2005 Microsoft
//
//------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Text;

namespace System.Windows.Markup
{
    internal class RoutedEventValueSerializer: ValueSerializer
    {
        public override bool CanConvertToString(object value, IValueSerializerContext context)
        {
            return ValueSerializer.GetSerializerFor(typeof(Type), context) != null;
        }

        public override bool CanConvertFromString(string value, IValueSerializerContext context)
        {
            return ValueSerializer.GetSerializerFor(typeof(Type), context) != null;
        }

        public override string ConvertToString(object value, IValueSerializerContext context)
        {
            RoutedEvent routedEvent = value as RoutedEvent;
            if (routedEvent != null)
            {
                ValueSerializer typeSerializer = ValueSerializer.GetSerializerFor(typeof(Type), context);
                if (typeSerializer != null)
                {
                    return typeSerializer.ConvertToString(routedEvent.OwnerType, context) + "." + routedEvent.Name;
                }
            }
            return base.ConvertToString(value, context);
        }

        static Dictionary<Type, Type> initializedTypes = new Dictionary<Type, Type>();

        static void ForceTypeConstructors(Type currentType)
        {
            // Force load the Statics by walking up the hierarchy and running class constructors
            while (currentType != null && !initializedTypes.ContainsKey(currentType))
            {
                MS.Internal.WindowsBase.SecurityHelper.RunClassConstructor(currentType);
                initializedTypes[currentType] = currentType;
                currentType = currentType.BaseType;
            }
        }

        public override object ConvertFromString(string value, IValueSerializerContext context)
        {
            ValueSerializer typeSerializer = ValueSerializer.GetSerializerFor(typeof(Type), context);
            if (typeSerializer != null)
            {
                int index = value.IndexOf('.');
                if (index > 0)
                {
                    Type type = typeSerializer.ConvertFromString(value.Substring(0, index), context) as Type;
                    string name = value.Substring(index + 1).Trim();
                    ForceTypeConstructors(type);
                    return EventManager.GetRoutedEventFromName(name, type);
                }
            }
            return base.ConvertFromString(value, context);
        }
    }
}
