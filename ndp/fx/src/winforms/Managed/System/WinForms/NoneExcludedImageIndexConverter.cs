//------------------------------------------------------------------------------
// <copyright file="NoneExcludedImageIndexConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {


    /// <include file='doc\NoneExcludedImageIndexConverter.uex' path='docs/doc[@for="NoneExcludedImageIndexConverter"]/*' />
    /// <devdoc>
    ///      Just returns false for IncludeNoneAsStandardValue
    /// </devdoc>
    internal sealed class NoneExcludedImageIndexConverter : ImageIndexConverter {

        /// <include file='doc\NoneExcludedImageIndexConverter.uex' path='docs/doc[@for="NoneExcludedImageIndexConverter.IncludeNoneAsStandardValue"]/*' />
        protected override bool IncludeNoneAsStandardValue {
            get {
                return false;
            }
        }   
    }
}

