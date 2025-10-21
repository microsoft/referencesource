//------------------------------------------------------------------------------
// <copyright file="DataGridViewEditMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    /// <include file='doc\DataGridViewEditMode.uex' path='docs/doc[@for="DataGridViewEditMode.DataGridViewEditMode"]/*' />
    public enum DataGridViewEditMode
    {
        /// <include file='doc\DataGridViewEditMode.uex' path='docs/doc[@for="DataGridViewEditMode.EditOnEnter"]/*' />
        EditOnEnter = 0,

        /// <include file='doc\DataGridViewEditMode.uex' path='docs/doc[@for="DataGridViewEditMode.EditOnKeystroke"]/*' />
        EditOnKeystroke,

        /// <include file='doc\DataGridViewEditMode.uex' path='docs/doc[@for="DataGridViewEditMode.EditOnKeystrokeOrF2"]/*' />
        EditOnKeystrokeOrF2,

        /// <include file='doc\DataGridViewEditMode.uex' path='docs/doc[@for="DataGridViewEditMode.EditOnF2"]/*' />
        EditOnF2,

        /// <include file='doc\DataGridViewEditMode.uex' path='docs/doc[@for="DataGridViewEditMode.EditProgrammatically"]/*' />
        EditProgrammatically
    }
}
