//------------------------------------------------------------------------------
// <copyright file="InstallerCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Configuration.Install {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;
    
    /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Contains a collection of installers to be run during an
    ///       installation.
    ///    </para>
    /// </devdoc>
    public class InstallerCollection : CollectionBase {
        private Installer owner;

        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.InstallerCollection"]/*' />
        /// <devdoc>
        /// internal so no one can create one of these
        /// </devdoc>
        internal InstallerCollection(Installer owner) {
            this.owner = owner;
        }

        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets an
        ///       installer at the specified index.
        ///    </para>
        /// </devdoc>
        public Installer this[int index] {
            get {
                return ((Installer)(List[index]));
            }
            set {
                List[index] = value;
            }
        }        

        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds the specified installer to this collection of
        ///       installers.
        ///    </para>
        /// </devdoc>
        public int Add(Installer value) {
            return List.Add(value);
        }

        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.AddRange"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds the specified installers to this installer collection.
        ///    </para>
        /// </devdoc>
        public void AddRange(InstallerCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }

        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.AddRange1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddRange(Installer[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
         
        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines whether the specified installer is included in this <see cref='System.Configuration.Install.InstallerCollection'/>.
        ///    </para>
        /// </devdoc>
        public bool Contains(Installer value) {
            return List.Contains(value);
        }

        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(Installer[] array, int index) {
            List.CopyTo(array, index);
        }
        
        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(Installer value) {
            return List.IndexOf(value);
        }
        
        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.Insert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Insert(int index, Installer value) {
            List.Insert(index, value);
        }

        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes the specified <see cref='System.Configuration.Install.Installer'/> from the
        ///       collection.
        ///    </para>
        /// </devdoc>
        public void Remove(Installer value) {
            List.Remove(value);
        }
        
        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.OnInsert"]/*' />
        ///<internalonly/>                          
        protected override void OnInsert(int index, object value) {
            if (value == owner)
                throw new ArgumentException(Res.GetString(Res.CantAddSelf));
            
            if (CompModSwitches.InstallerDesign.TraceVerbose) Debug.WriteLine("Adding installer " + index + " to Installers collection");
            ((Installer)value).parent = this.owner;
        }
        
        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.OnRemove"]/*' />
        ///<internalonly/>                          
        protected override void OnRemove(int index, object value) {
            if (CompModSwitches.InstallerDesign.TraceVerbose) Debug.WriteLine("Removing installer " + index + " from Installers collection");
            ((Installer)value).parent = null;
        }
                 
        /// <include file='doc\InstallerCollection.uex' path='docs/doc[@for="InstallerCollection.OnSet"]/*' />
        ///<internalonly/>                          
        protected override void OnSet(int index, object oldValue, object newValue) {
            if (newValue == owner)
                throw new ArgumentException(Res.GetString(Res.CantAddSelf));

            if (CompModSwitches.InstallerDesign.TraceVerbose) Debug.WriteLine("Setting installer " + index + " in Installers collection");
            ((Installer)oldValue).parent = null;
            ((Installer)newValue).parent = this.owner;    
        }         
    }
}
  
