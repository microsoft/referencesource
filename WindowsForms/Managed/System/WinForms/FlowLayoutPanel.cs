//------------------------------------------------------------------------------
// <copyright file="Panel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Windows.Forms.Layout;
    using System.Runtime.InteropServices;

    /// <include file='doc\FlowPanel.uex' path='docs/doc[@for="FlowPanel"]/*' />
    [ComVisible(true)]
    [ClassInterface(ClassInterfaceType.AutoDispatch)]
    [ProvideProperty("FlowBreak", typeof(Control))]
    [DefaultProperty("FlowDirection")]
    [Designer("System.Windows.Forms.Design.FlowLayoutPanelDesigner, " + AssemblyRef.SystemDesign)]
    [Docking(DockingBehavior.Ask)]
    [SRDescription(SR.DescriptionFlowLayoutPanel)]
    public class FlowLayoutPanel : Panel, IExtenderProvider {
        private FlowLayoutSettings _flowLayoutSettings;

        /// <include file='doc\FlowLayoutPanel.uex' path='docs/doc[@for="FlowLayoutPanel.FlowLayoutPanel"]/*' />
        public FlowLayoutPanel() {
            _flowLayoutSettings = FlowLayout.CreateSettings(this);
        }
        
        /// <include file='doc\FlowPanel.uex' path='docs/doc[@for="FlowPanel.LayoutEngine"]/*' />
        public override LayoutEngine LayoutEngine {
            get { return FlowLayout.Instance; }
        }

        /// <include file='doc\FlowPanel.uex' path='docs/doc[@for="FlowPanel.FlowDirection"]/*' />
        [SRDescription(SR.FlowPanelFlowDirectionDescr)]
        [DefaultValue(FlowDirection.LeftToRight)]
        [SRCategory(SR.CatLayout)]
        [Localizable(true)]
        public FlowDirection FlowDirection {
            get { return _flowLayoutSettings.FlowDirection; }
            set { 
                _flowLayoutSettings.FlowDirection = value; 
                Debug.Assert(FlowDirection == value, "FlowDirection should be the same as we set it");
            }
        }

        /// <include file='doc\FlowPanel.uex' path='docs/doc[@for="FlowPanel.WrapContents"]/*' />
        [SRDescription(SR.FlowPanelWrapContentsDescr)]
        [DefaultValue(true)]
        [SRCategory(SR.CatLayout)]
        [Localizable(true)]
        public bool WrapContents {
            get { return _flowLayoutSettings.WrapContents; }
            set { 
                _flowLayoutSettings.WrapContents = value;
                Debug.Assert(WrapContents == value, "WrapContents should be the same as we set it");
            }
        }

        #region Provided properties
        /// <include file='doc\FlowPanel.uex' path='docs/doc[@for="FlowPanel.IExtenderProvider.CanExtend"]/*' />
        /// <internalonly/>
        bool IExtenderProvider.CanExtend(object obj) {
            Control control = obj as Control;
            return control != null && control.Parent == this;
        }

        [DefaultValue(false)]
        [DisplayName("FlowBreak")]
        public bool GetFlowBreak(Control control) {
            return _flowLayoutSettings.GetFlowBreak(control);
        }

        [DisplayName("FlowBreak")]
        public void SetFlowBreak(Control control, bool value) {
            _flowLayoutSettings.SetFlowBreak(control, value);
        }
		
        #endregion
    }
}

