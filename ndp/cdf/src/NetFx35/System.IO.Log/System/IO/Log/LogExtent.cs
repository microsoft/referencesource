//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;

    public sealed class LogExtent
    {
        string path;
        long size;
        LogExtentState state;

        internal LogExtent(string path, long size, LogExtentState state)
        {
            this.path = path;
            this.size = size;
            this.state = state;
        }

        public string Path 
        {
            get 
            {
                return this.path; 
            }
        }

        public long Size 
        {
            get 
            {
                return this.size; 
            }
        }

        public LogExtentState State 
        {
            get 
            {
                return this.state; 
            }
        }
    }
}
