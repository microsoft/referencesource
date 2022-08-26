//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation, 2005
//
//  File:      ElementItem.cs
//
//  Contents:  Implements a converter to an instance descriptor for 
//             DynamicResourceExtension
//
//  Created:   04/28/2005 Microsoft
//
//------------------------------------------------------------------------

using System;
using System.ComponentModel;
using System.ComponentModel.Design.Serialization;
using System.Collections;
using System.Security;
using System.Text;

namespace System.Windows
{
    /// <summary>
    /// Type converter to inform the serialization system how to construct a DynamicResourceExtension from
    /// an instance. It reports that ResourceKey should be used as the first parameter to the constructor.
    /// </summary>
    public class DynamicResourceExtensionConverter: TypeConverter
    {
        /// <summary>
        /// True if converting to an instance descriptor
        /// </summary>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(InstanceDescriptor))
            {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        /// <summary>
        /// Converts to an instance descriptor
        /// </summary>
        ///<SecurityNote>
        ///     Critical: calls InstanceDescriptor ctor which LinkDemands
        ///     PublicOK: can only make an InstanceDescriptor for DynamicResourceExtension, not an arbitrary class
        ///</SecurityNote> 
        [SecurityCritical]
        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType == typeof(InstanceDescriptor))
            {
                if(value == null)
                    throw new ArgumentNullException("value");

                DynamicResourceExtension dynamicResource = value as DynamicResourceExtension;

                if (dynamicResource == null)
                    throw new ArgumentException(SR.Get(SRID.MustBeOfType, "value", "DynamicResourceExtension"), "value"); 

                return new InstanceDescriptor(typeof(DynamicResourceExtension).GetConstructor(new Type[] { typeof(object) }), 
                    new object[] { dynamicResource.ResourceKey } );
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
