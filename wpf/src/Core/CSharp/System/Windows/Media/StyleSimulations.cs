//+-----------------------------------------------------------------------
//
//  Microsoft Avalon
//  Copyright (C) Microsoft Corporation
//
//  File:      StyleSimulations.cs
//
//  Contents:  StyleSimulations declaration
//
//  Created:   6-04-2003 Michael Leonov (mleonov) Moved StyleSimulations from GlyphRun.cs
//
//------------------------------------------------------------------------

using System;

namespace System.Windows.Media
{
    
    // Compatibility warning:
    /************************/
    // This enum has a mirror enum Cpp\DWriteWrapper\FontSimulation.h
    // If any changes happens to StyleSimulations they should be reflected
    // in FontSimulation.h

    /// <summary>
    /// Font style simulation
    /// </summary>
    [Flags]
    public enum StyleSimulations
    {
        /// <summary>
        /// No font style simulation
        /// </summary>
        None                 = 0,
        /// <summary>
        /// Bold style simulation
        /// </summary>
        BoldSimulation       = 1,
        /// <summary>
        /// Italic style simulation
        /// </summary>
        ItalicSimulation     = 2,
        /// <summary>
        /// Bold and Italic style simulation.
        /// </summary>
        BoldItalicSimulation = BoldSimulation | ItalicSimulation
    }
}

