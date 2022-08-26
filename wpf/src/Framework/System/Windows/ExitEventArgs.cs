//---------------------------------------------------------------------------
// File: ExitEventArgs.cs
//
// Copyright (C) 2004 by Microsoft Corporation.  All rights reserved.
// 
// Description:
//          This event will be called after Shutdown has been called. 
//
//          The developer will hook this event if they want to take action 
//          when the application exits.
//
// History:
//  08/10/04: kusumav   Moved out of Application.cs to its own separate file.
//  05/09/05: hamidm    Created ExitEventArgs.cs and renamed ShuttingDownEventArgs 
//                      to ExitEventArgs
//
//---------------------------------------------------------------------------

namespace System.Windows
{
    /// <summary>
    /// Event args for the Exit event
    /// </summary>
    public class ExitEventArgs : EventArgs
    {
        internal int _exitCode;

        /// <summary>
        /// constructor
        /// </summary>
        internal ExitEventArgs(int exitCode)
        {
            _exitCode = exitCode;
        }

        /// <summary>
        /// Get and set the exit code to be returned by this application
        /// </summary>
        public int ApplicationExitCode
        {
            get { return _exitCode; }
            set { _exitCode = value; }
        }
    }
}
