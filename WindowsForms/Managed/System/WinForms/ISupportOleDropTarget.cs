    //------------------------------------------------------------------------------
    // <copyright file="ISupportOleDropTarget.cs" company="Microsoft">
    //     Copyright (c) Microsoft Corporation.  All rights reserved.
    // </copyright>                                                                
    //------------------------------------------------------------------------------


    namespace System.Windows.Forms {
        using System;

        public interface IDropTarget {

            /// <include file='doc\IDropTarget.uex' path='docs/doc[@for=IDropTarget.OnDragEnter]/*'/ />
            /// <devdoc>
            /// Summary of OnDragEnter.
            /// </devdoc>
            /// <param name=e></param>	
            void OnDragEnter(DragEventArgs e);
            /// <include file='doc\IDropTarget.uex' path='docs/doc[@for=IDropTarget.OnDragLeave]/*'/ />
            /// <devdoc>
            /// Summary of OnDragLeave.
            /// </devdoc>
            /// <param name=e></param>	
            void OnDragLeave(System.EventArgs e);
            /// <include file='doc\IDropTarget.uex' path='docs/doc[@for=IDropTarget.OnDragDrop]/*'/ />
            /// <devdoc>
            /// Summary of OnDragDrop.
            /// </devdoc>
            /// <param name=e></param>	
            void OnDragDrop(DragEventArgs e);
            /// <include file='doc\IDropTarget.uex' path='docs/doc[@for=IDropTarget.OnDragOver]/*'/ />
            /// <devdoc>
            /// Summary of OnDragOver.
            /// </devdoc>
            /// <param name=e></param>	
            void OnDragOver(DragEventArgs e);
        }
    }
