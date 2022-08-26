//------------------------------------------------------------------------------
// <copyright file="WebBrowserUriTypeConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
using System;
using System.ComponentModel;

namespace System.Windows.Forms 
{
    class WebBrowserUriTypeConverter : UriTypeConverter 
    {
        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            //The UriTypeConverter gives back a relative Uri for things like "www.microsoft.com".  If 
            //the Uri is relative, we'll try sticking "http://" on the front to see whether that fixes it up.
            Uri uri = base.ConvertFrom(context, culture, value) as Uri;
            if (uri != null && !string.IsNullOrEmpty(uri.OriginalString) && !uri.IsAbsoluteUri)
            {
                try 
                {
                    uri = new Uri("http://" + uri.OriginalString.Trim());
                }
                catch (UriFormatException) 
                {
                    //We can't throw "http://" on the front: just return the original (relative) Uri,
                    //which will throw an exception with reasonable text later.
                }
            }
            return uri;
        } 
    }
}
