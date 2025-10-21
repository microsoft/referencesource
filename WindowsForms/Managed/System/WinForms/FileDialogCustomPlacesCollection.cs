//------------------------------------------------------------------------------
// <copyright file="FileDialogCustomPlacesCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Security;
using System.Security.Permissions;

namespace System.Windows.Forms
{
    public class FileDialogCustomPlacesCollection : Collection<FileDialogCustomPlace>
    {        
        internal void Apply(FileDialogNative.IFileDialog dialog)
        {
            //Walk backwards
            for (int i = this.Items.Count - 1; i >= 0; --i)
            {
                FileDialogCustomPlace customPlace = this.Items[i];

                // Fix for Dev10 bug 536188: we need permission to check whether the specified path exists
                FileIOPermission permission = new FileIOPermission(FileIOPermissionAccess.PathDiscovery, customPlace.Path);
                permission.Demand();

                try
                {
                    FileDialogNative.IShellItem shellItem = customPlace.GetNativePath();
                    if (null != shellItem)
                    {
                        dialog.AddPlace(shellItem, 0);
                    }
                }
                catch (FileNotFoundException)
                {
                }
                //Silently absorb FileNotFound exceptions (these could be caused by a path that disappeared after the place was added to the dialog).
            }
        }

        public void Add(string path)
        {
            Add(new FileDialogCustomPlace(path)); 
        }

        public void Add(Guid knownFolderGuid)
        {
            Add(new FileDialogCustomPlace(knownFolderGuid));
        }
    }
}
