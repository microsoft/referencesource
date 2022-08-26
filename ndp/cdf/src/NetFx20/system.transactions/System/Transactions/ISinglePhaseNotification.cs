using System;
using System.Security.Permissions;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace System.Transactions
{
    internal interface ISinglePhaseNotificationInternal : IEnlistmentNotificationInternal
    {
        void SinglePhaseCommit(
            IPromotedEnlistment singlePhaseEnlistment
            );
    }

    public interface ISinglePhaseNotification : IEnlistmentNotification
    {
        void SinglePhaseCommit(
            SinglePhaseEnlistment singlePhaseEnlistment
            );
        
    }
}
