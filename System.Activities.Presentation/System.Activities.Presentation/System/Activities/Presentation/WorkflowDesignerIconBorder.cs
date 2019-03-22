//----------------------------------------------------------------
// <copyright company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------

namespace System.Activities.Presentation
{
    using System;
    using System.Windows.Automation.Peers;
    using System.Windows.Controls;
    using System.Windows.Media;

    /// <summary>
    /// A Border wrapper used for presenting icons in workflow designer.
    /// </summary>
    internal class WorkflowDesignerIconBorder : Border
    {
        /// <summary>
        /// Override to return a FrameworkElementAutomationPeer so that the decorator border control could be referred to by UI automation tools.
        /// </summary>
        /// <returns>A FrameworkElementAutomationPeer.</returns>
        protected override AutomationPeer OnCreateAutomationPeer()
        {
            if (!LocalAppContextSwitches.UseLegacyAccessibilityFeatures)
            {
                return new FrameworkElementAutomationPeer(this);
            }

            return base.OnCreateAutomationPeer();
        }
    }
}
