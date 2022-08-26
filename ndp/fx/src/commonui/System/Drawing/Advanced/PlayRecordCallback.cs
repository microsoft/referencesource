//------------------------------------------------------------------------------
// <copyright file="PlayRecordCallback.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
namespace System.Drawing.Imaging {
      	
    /// <include file='doc\PlayRecordCAllback.uex' path='docs/doc[@for="PlayRecodCallBack"]/*' />
    /// <devdoc>
    /// </devdoc>
    public delegate void PlayRecordCallback(EmfPlusRecordType recordType,
                                            int flags,
                                            int dataSize,
                                            IntPtr recordData);
}


