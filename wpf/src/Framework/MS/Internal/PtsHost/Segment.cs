//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// File: Section.cs
//
// Description: Interface representing PTS segment. 
//
// History:  
//  05/05/2003 : grzegorz - moving from Avalon branch.
//
//---------------------------------------------------------------------------

using System;

namespace MS.Internal.PtsHost
{
    /// <summary>
    /// Interface representing PTS segment.
    /// </summary>
    internal interface ISegment
    {
        /// <summary>
        /// Get first para 
        /// </summary>
        /// <param name="successful">
        /// OUT: does segment contain any paragraph?
        /// </param>
        /// <param name="firstParaName">
        /// OUT: name of the first paragraph in segment
        /// </param>
        void GetFirstPara(
            out int successful,                 
            out IntPtr firstParaName);           
        
        /// <summary>
        /// Get next para 
        /// </summary>
        /// <param name="currentParagraph">
        /// IN: current para
        /// </param>
        /// <param name="found">
        /// OUT: is there next paragraph?
        /// </param>
        /// <param name="nextParaName">
        /// OUT: name of the next paragraph in section
        /// </param>
        void GetNextPara(
            BaseParagraph currentParagraph,      
            out int found,                       
            out IntPtr nextParaName);           

        /// <summary>
        /// Get first change in segment - part of update process
        /// </summary>
        /// <param name="fFound">
        /// OUT: anything changed?
        /// </param>
        /// <param name="fChangeFirst">
        /// OUT: first paragraph changed?
        /// </param>
        /// <param name="nmpBeforeChange">
        /// OUT: name of paragraph before the change if !fChangeFirst
        /// </param>
        void UpdGetFirstChangeInSegment(
            out int fFound,                     
            out int fChangeFirst,                
            out IntPtr nmpBeforeChange);         
    }
}
