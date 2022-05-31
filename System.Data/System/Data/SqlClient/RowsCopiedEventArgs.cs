//------------------------------------------------------------------------------
// <copyright file="RowsCopiedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <owner current="true" primary="true">[....]</owner>
// <owner current="true" primary="false">[....]</owner>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    public class SqlRowsCopiedEventArgs : System.EventArgs {
        private bool            _abort;
        private long             _rowsCopied;

        public SqlRowsCopiedEventArgs (long rowsCopied) {
            _rowsCopied = rowsCopied;
        }

        public bool Abort {
            get {
                return _abort;
            }
            set {
                _abort = value;
            }

        }

        public long RowsCopied {
            get {
                return _rowsCopied;
            }
        }
    }
}
