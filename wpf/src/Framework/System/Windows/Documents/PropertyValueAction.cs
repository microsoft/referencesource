/////////////////////////////////////////////////////////////////////////////
//
// File: PropertyValueAction.cs
//
// Copyright (C) 2003 by Microsoft Corporation.  All rights reserved.
//
// Description: PropertyValueAction is used in TextRange.ApplyPopertyValue method
//              for defining a way how to apply the value of a property.
//
/////////////////////////////////////////////////////////////////////////////

namespace System.Windows.Documents
{
    /// <summary>
    /// PropertyValueAction is used in TextRange.ApplyPropertyValue method
    /// for defining a way how to apply the value passed as a parameter:
    /// set it as is, use it for increasing the existing values or
    /// use for decreasing.
    /// </summary>
    internal enum PropertyValueAction
    {
        /// <summary>
        /// This option indicates that the value passed as a parameter
        /// must be set as is.
        /// </summary>
        SetValue,

        /// <summary>
        /// This option indicates that the value passed as a parameter
        /// must be used as an absolute increment for existing values.
        /// </summary>
        IncreaseByAbsoluteValue,

        /// <summary>
        /// This option indicates that the value passed as a parameter
        /// must be used as an absolute decrement for existing values.
        /// </summary>
        DecreaseByAbsoluteValue,

        /// <summary>
        /// This options indicates that the value passed as a parameter
        /// must be used as a percentage increment for existing values.
        /// </summary>
        IncreaseByPercentageValue,

        /// <summary>
        /// This options indicates that the value passed as a parameter
        /// must be used as a percentage decrement for existing values.
        /// </summary>
        DecreaseByPercentageValue,
    }
}
