//------------------------------------------------------------------------------
// <copyright file="QuestionEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.Diagnostics;
    using System.ComponentModel;

    /// <include file='doc\QuestionEventArgs.uex' path='docs/doc[@for="QuestionEventArgs"]/*' />
    /// <devdoc>
    /// </devdoc>
    public class QuestionEventArgs : EventArgs
    {
        private bool response;

        /// <include file='doc\QuestionEventArgs.uex' path='docs/doc[@for="QuestionEventArgs.QuestionEventArgs1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public QuestionEventArgs()
        {
            this.response = false;
        }

        /// <include file='doc\QuestionEventArgs.uex' path='docs/doc[@for="QuestionEventArgs.QuestionEventArgs2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public QuestionEventArgs(bool response)
        {
            this.response = response;
        }

        /// <include file='doc\QuestionEventArgs.uex' path='docs/doc[@for="QuestionEventArgs.Reponse"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool Response
        {
            get
            {
                return this.response;
            }
            set
            {
                this.response = value;
            }
        }
    }
}

