//------------------------------------------------------------------------------
// <copyright file="MaskFormat.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    /// <devdoc>
    ///     Enum defining inclusion of special characters.
    /// </devdoc>
    public enum MaskFormat
    {
        IncludePrompt               = 0x0001,
        IncludeLiterals             = 0x0002,

        // both of the above
        IncludePromptAndLiterals    = 0x0003,

        // Never include special characters.
        ExcludePromptAndLiterals    = 0x000
    }
}
