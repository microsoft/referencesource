//---------------------------------------------------------------------------
//
// <copyright file="ValidationStep.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: Enum describing the different steps in the validation process.
//
//---------------------------------------------------------------------------

using System;
using System.ComponentModel; // doc comments

namespace System.Windows.Controls
{
    /// <summary>
    /// This enum describes the different steps in the validation process.
    /// </summary>
    public enum ValidationStep
    {
        /// <summary> Obtain the value from the target element </summary>
        RawProposedValue,
        /// <summary> Apply any conversions needed to produce a value suitable
        /// for the source </summary>
        ConvertedProposedValue,
        /// <summary> Update the source with the new value </summary>
        UpdatedValue,
        /// <summary> Commit the source's new values.  This step does nothing
        /// unless the source supports a commit mechanism such as <seealso cref="IEditableObject"/>
        /// </summary>
        CommittedValue,
    }
}
