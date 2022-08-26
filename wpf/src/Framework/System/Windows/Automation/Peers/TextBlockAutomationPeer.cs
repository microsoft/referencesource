//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// File: TextBlockAutomationPeer.cs
//
// Description: AutomationPeer associated with TextBlock.
//
//---------------------------------------------------------------------------

using System.Collections.Generic;           // List<T>
using System.Windows.Controls;              // TextBlock
using System.Windows.Documents;             // ITextContainer
using MS.Internal.Documents;                // TextContainerHelper

namespace System.Windows.Automation.Peers
{
    /// <summary>
    /// AutomationPeer associated with TextBlock.
    /// </summary>
    public class TextBlockAutomationPeer : FrameworkElementAutomationPeer
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="owner">Owner of the AutomationPeer.</param>
        public TextBlockAutomationPeer(TextBlock owner)
            : base(owner)
        { }

        /// <summary>
        /// <see cref="AutomationPeer.GetChildrenCore"/>
        /// </summary>
        protected override List<AutomationPeer> GetChildrenCore()
        {
            List<AutomationPeer> peers = null;
            TextBlock owner = (TextBlock)Owner;
            // TextBlock has children only if it has complex content.
            if (owner.HasComplexContent)
            {
                peers = TextContainerHelper.GetAutomationPeersFromRange(owner.TextContainer.Start, owner.TextContainer.End, null);
            }
            return peers;
        }

        /// <summary>
        /// <see cref="AutomationPeer.GetAutomationControlTypeCore"/>
        /// </summary>
        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Text;
        }

        /// <summary>
        /// <see cref="AutomationPeer.GetClassNameCore"/>
        /// </summary>
        /// <returns></returns>
        protected override string GetClassNameCore()
        {
            return "TextBlock";
        }

        /// <summary>
        /// <see cref="AutomationPeer.IsControlElementCore"/>
        /// </summary>
        override protected bool IsControlElementCore()
        {
            // Return false if TextBlock is part of a ControlTemplate, otherwise return the base method
            TextBlock tb = (TextBlock)Owner;
            DependencyObject templatedParent = tb.TemplatedParent;

            // If the templatedParent is a ContentPresenter, this TextBlock is generated from a DataTemplate
            if (templatedParent == null || templatedParent is ContentPresenter)
            {
                return base.IsControlElementCore();
            }

            return false;
        }
    }
}
