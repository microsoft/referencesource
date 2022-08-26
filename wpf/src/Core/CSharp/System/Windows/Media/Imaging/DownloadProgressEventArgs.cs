//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2003
//
//  File: DownloadProgressEventArgs.cs
//
//------------------------------------------------------------------------------

namespace System.Windows.Media.Imaging
{
    #region DownloadProgressEventArgs

    /// <summary>
    /// Event args for the DownloadProgress event.
    /// </summary>
    public class DownloadProgressEventArgs : EventArgs
    {
        // Internal constructor
        internal DownloadProgressEventArgs(int percentComplete)
        {
            _percentComplete = percentComplete;
        }

        /// <summary>
        /// Returns the progress between 1-100
        /// </summary>
        public int Progress
        {
            get
            {
                return _percentComplete;
            }
        }

        int _percentComplete;
    }

    #endregion
}

