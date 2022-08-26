

/***************************************************************************\
*
* SharedDp
*
* Copyright (C) 2005 by Microsoft Corporation.  All rights reserved.
*
* Structure that holds information about a DependencyProperty that is to
* be shared between multiple instantiations of this template.
* (See OptimizedTemplateContent)
*
\***************************************************************************/


using System;
using System.Windows;

namespace System.Windows
{
    internal class SharedDp
    {
        internal SharedDp(
            DependencyProperty dp,
            object             value,
            string             elementName)
        {
            Dp = dp;
            Value = value;
            ElementName = elementName;
        }
        
        internal DependencyProperty Dp;
        internal object             Value;
        internal string             ElementName;
    }

}         


