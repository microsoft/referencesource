//---------------------------------------------------------------------
// <copyright file="EpmAttributeNameBuilder.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides names for attributes in csdl file
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
    /// <summary>
    /// Build attribute names corresponding to ones in csdl file
    /// </summary>
    internal sealed class EpmAttributeNameBuilder
    {
        /// <summary>Current index</summary>
        private int index;
        
        /// <summary>PostFix for current attribute names</summary>
        private String postFix;

        /// <summary>Constructor</summary>
        internal EpmAttributeNameBuilder()
        {
            this.postFix = String.Empty;
        }

        /// <summary>KeepInContent</summary>
        internal String EpmKeepInContent
        {
            get
            {
                return XmlConstants.MetadataAttributeEpmKeepInContent + this.postFix;
            }
        }

        /// <summary>SourcePath</summary>
        internal String EpmSourcePath
        {
            get
            {
                return XmlConstants.MetadataAttributeEpmSourcePath + this.postFix;
            }
        }

        /// <summary>Target Path</summary>
        internal String EpmTargetPath
        {
            get
            {
                return XmlConstants.MetadataAttributeEpmTargetPath + this.postFix;
            }
        }

        /// <summary>ContentKind</summary>
        internal String EpmContentKind
        {
            get
            {
                return XmlConstants.MetadataAttributeEpmContentKind + this.postFix;
            }
        }

        /// <summary>Namespace Prefix</summary>
        internal String EpmNsPrefix
        {
            get
            {
                return XmlConstants.MetadataAttributeEpmNsPrefix + this.postFix;
            }
        }

        /// <summary>Namespace Uri</summary>
        internal String EpmNsUri
        {
            get
            {
                return XmlConstants.MetadataAttributeEpmNsUri + this.postFix;
            }
        }

        /// <summary>Move to next attribute name generation</summary>
        internal void MoveNext()
        {
            this.index++;
            this.postFix = "_" + this.index.ToString(System.Globalization.CultureInfo.InvariantCulture);
        }
    }
}
