//------------------------------------------------------------------------------
// <copyright file="ADP.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// @owner       Microsoft
// @backupOwner Microsoft
//------------------------------------------------------------------------------

namespace System.Data.Services.Design.Common {

    using System;


    internal static class EDesignUtil
    {

        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Helper Functions
        //
        static internal T CheckArgumentNull<T>(T value, string parameterName) where T : class
        {
            if (null == value)
            {
                throw Error.ArgumentNull(parameterName);
            }
            return value;
        }

        static internal void CheckStringArgument(string value, string parameterName)
        {
            // Throw ArgumentNullException when string is null
            CheckArgumentNull(value, parameterName);

            // Throw ArgumentException when string is empty
            if (value.Length == 0)
            {
                throw InvalidStringArgument(parameterName);
            }
        }

        static internal LanguageOption CheckLanguageOptionArgument(LanguageOption value, string paramName)
        {
            if (value == LanguageOption.GenerateCSharpCode ||
                value == LanguageOption.GenerateVBCode)
            {
                return value;
            }
            throw Error.ArgumentOutOfRange(paramName);
        }

        static internal DataServiceCodeVersion CheckDataServiceCodeVersionArgument(DataServiceCodeVersion value, string paramName)
        {
            if (value == DataServiceCodeVersion.V1 ||
                value == DataServiceCodeVersion.V2)
            {
                return value;
            }
            throw Error.ArgumentOutOfRange(paramName);
        }

        static internal ArgumentException InvalidStringArgument(string parameterName) {
            ArgumentException e = new ArgumentException(Strings.InvalidStringArgument(parameterName));
            return e;
        }

        static internal InvalidOperationException InvalidOperation(string error)
        {
            InvalidOperationException e = new InvalidOperationException(error);
            return e;
        }
    }
}
