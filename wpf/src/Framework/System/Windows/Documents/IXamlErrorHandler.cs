//---------------------------------------------------------------------------
// 
// File: IXamlErrorHandler.cs
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// Description: Interface Xaml error handler.
//
//---------------------------------------------------------------------------

namespace System.Windows.Documents
{
    internal interface IXamlErrorHandler
    {
        void Error(string message, XamlToRtfError xamlToRtfError);

        void FatalError(string message, XamlToRtfError xamlToRtfError);

        void IgnorableWarning(string message, XamlToRtfError xamlToRtfError);
    }
}
