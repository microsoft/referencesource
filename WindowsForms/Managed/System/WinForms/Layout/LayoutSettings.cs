//------------------------------------------------------------------------------
// <copyright file="TableLayoutInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Windows.Forms.Layout;
namespace System.Windows.Forms {
    
    /// <include file='doc\LayoutSetting.uex' path='docs/doc[@for="LayoutSetting"]/*' />
    public abstract class LayoutSettings {
        private IArrangedElement _owner;

        protected LayoutSettings() {
        }
        
        internal LayoutSettings(IArrangedElement owner) {
            this._owner = owner;
        }
        
        /// <include file='doc\LayoutSetting.uex' path='docs/doc[@for="LayoutSetting.LayoutEngine"]/*' />
        public virtual LayoutEngine LayoutEngine {
            get { return null;}
        }

        internal IArrangedElement Owner {
            get { return _owner; }
        }
    }
}
