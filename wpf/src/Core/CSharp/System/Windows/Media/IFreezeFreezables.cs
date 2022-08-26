//---------------------------------------------------------------------------
//
// <copyright file="IFreezeFreezables.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description: TODO
//
//---------------------------------------------------------------------------

using System.Windows;

namespace System.Windows.Media
{
    internal interface IFreezeFreezables
    {
        bool FreezeFreezables
        {
            get;
        }
        
        bool TryFreeze(string value, Freezable freezable);

        Freezable TryGetFreezable(string value);
    }
}
