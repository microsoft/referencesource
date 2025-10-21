//------------------------------------------------------------------------------
// <copyright file="RelatedImageListAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    /// <include file='doc\RelatedImageListAttribute.uex' path='docs/doc[@for="RelatedImageListAttribute"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies which imagelist a property relates to. For example ImageListIndex must relate to a
    ///       specific ImageList property
    ///    </para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Property, AllowMultiple=false, Inherited=true)]
    public sealed class RelatedImageListAttribute : Attribute {
        private string relatedImageList=null;

        public RelatedImageListAttribute(string relatedImageList) {
            this.relatedImageList = relatedImageList;
        }

        public string RelatedImageList {
            get {
                return relatedImageList;
            }
        }
    } // end of RelatedImageListAttribute class
} // end of namespace
