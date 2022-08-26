//---------------------------------------------------------------------------
//
// File: RootBrowserWindowProxy.cs
//
// Description: 
//
// Created: 07/01/2004
//
// Copyright (C) 2001 by Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------   

using System;
using System.Security;
using MS.Internal;
using MS.Internal.PresentationFramework;

namespace MS.Internal.AppModel
{
    class RootBrowserWindowProxy : MarshalByRefObject
    {
        #region Constructors

        internal RootBrowserWindowProxy(RootBrowserWindow rbw)
        {
            _rbw = rbw;
        }

        #endregion Constructors

        #region Internal Properties

        internal RootBrowserWindow RootBrowserWindow
        {
            get
            {
                return _rbw;
            }
        }

        #endregion Internal Properties

        #region Internal methods

        internal void TabInto(bool forward)
        {
            _rbw.TabInto(forward);
        }

        #endregion

        #region Private Data

        private RootBrowserWindow _rbw;

        #endregion Private Data
    }
}
