//------------------------------------------------------------------------------
// <copyright file="TextBoxBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    /// <summary>
    ///     Represents the mode characters are entered in a text box.
    /// </summary>
    public enum InsertKeyMode
    {
        /// <devdoc> 
        ///     Honors the Insert key mode. 
        /// </devdoc>
        Default,

        /// <devdoc> 
        ///     Forces insertion mode to be 'on' regardless of the Insert key mode. 
        /// </devdoc>
        Insert,

        /// <devdoc> 
        ///     Forces insertion mode to be 'off' regardless of the Insert key mode. 
        /// </devdoc>
        Overwrite        
    }
}
