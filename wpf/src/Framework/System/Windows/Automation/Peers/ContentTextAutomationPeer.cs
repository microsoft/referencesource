//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// File: ContentTextElementAutomationPeer.cs
//
// Description: Base class for AutomationPeers associated with TextPattern.
//
//---------------------------------------------------------------------------

using System.Collections.Generic;           // List<T>
using System.Windows.Automation.Provider;   // IRawElementProviderSimple
using System.Windows.Documents;             // ITextPointer

namespace System.Windows.Automation.Peers
{
    /// <summary>
    /// Base class for AutomationPeers associated with TextPattern.
    /// </summary>
    public abstract class ContentTextAutomationPeer : FrameworkContentElementAutomationPeer
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        protected ContentTextAutomationPeer(FrameworkContentElement owner)
            : base(owner)
        { }

        /// <summary>
        /// Maps AutomationPeer to provider object.
        /// </summary>
        internal new IRawElementProviderSimple ProviderFromPeer(AutomationPeer peer)
        {
            return base.ProviderFromPeer(peer);
        }

        /// <summary>
        /// Maps automation provider to DependencyObject.
        /// </summary>
        internal DependencyObject ElementFromProvider(IRawElementProviderSimple provider)
        {
            DependencyObject element = null;
            AutomationPeer peer = PeerFromProvider(provider);
            if (peer is UIElementAutomationPeer)
            {
                element = ((UIElementAutomationPeer)peer).Owner;
            }
            else if (peer is ContentElementAutomationPeer)
            {
                element = ((ContentElementAutomationPeer)peer).Owner;
            }
            return element;
        }

        /// <summary>
        /// Gets collection of AutomationPeers for given text range.
        /// </summary>
        internal abstract List<AutomationPeer> GetAutomationPeersFromRange(ITextPointer start, ITextPointer end);
    }
}
