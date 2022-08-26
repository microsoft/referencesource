// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Enum:  PipelineStoreLocation 
**
** Purpose: Defines (currently one) location where the
** sytem should look for the pipeline components.  Needed
** because a partial trust host may not be able to determine
** the location of the pipeline components.
**
===========================================================*/
using System;

namespace System.AddIn.Hosting
{
    [Serializable]
    public enum PipelineStoreLocation
    {
        ApplicationBase = 0,
    }
}

