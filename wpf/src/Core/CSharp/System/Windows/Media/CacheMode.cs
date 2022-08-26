//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2008
//
//  File:       CacheMode.cs
//------------------------------------------------------------------------------

using System.ComponentModel;

using SR=MS.Internal.PresentationCore.SR;
using SRID=MS.Internal.PresentationCore.SRID;

namespace System.Windows.Media
{
    public abstract partial class CacheMode
    {
        internal CacheMode ()
        {
            
        }
        
        /// <summary>
        /// Parse - this method is called by the type converter to parse a CacheMode's string 
        /// (provided in "value").
        /// </summary>
        /// <returns>
        /// A CacheMode which was created by parsing the "value" argument.
        /// </returns>
        /// <param name="value"> String representation of a CacheMode. Cannot be null/empty. </param>
        internal static CacheMode Parse(string value)
        {
            CacheMode cacheMode = null;
            if (value == "BitmapCache")
            {
                cacheMode = new BitmapCache();
            }
            else
            {
                throw new FormatException(SR.Get(SRID.Parsers_IllegalToken));
            }
    
            return cacheMode;
        }

        /// <summary>
        /// Can serialze "this" to a string
        /// </summary>
        internal virtual bool CanSerializeToString()
        {
            return false;
        }

        internal virtual string ConvertToString(string format, IFormatProvider provider)
        {
            return base.ToString();
        }
    }
}
