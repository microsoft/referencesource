//--------------------------------------------------------------------------------
//
// File: SystemResourceHost.cs
//
// Description: Object supplied as the source when the resource is fetched from the SystemResources
//
// Created:     06/23/2005 
//
// Copyright (C) 2004 by Microsoft Corporation.  All rights reserved.
// 
//  History:
//      fmunoz  06/23/2005       Created 
//--------------------------------------------------------------------------------      

using System;

namespace System.Windows
{
    ///<summary/>
    internal sealed class SystemResourceHost
    {
        //prevent creation
        private SystemResourceHost()
        {
        }
        
        static internal SystemResourceHost Instance
        {
            get
            {
                if (_instance == null)
                {
                    _instance = new SystemResourceHost();
                }
                return _instance;
            }
        }

        static private SystemResourceHost _instance;
    }
}

