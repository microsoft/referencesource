using System;

namespace System.Transactions
{
    // FXCop doesn't like the way we spelled Promotable.
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
    public interface IPromotableSinglePhaseNotification : ITransactionPromoter
    {
        void Initialize();

        void SinglePhaseCommit(
            SinglePhaseEnlistment singlePhaseEnlistment
            );

        void Rollback(
            SinglePhaseEnlistment singlePhaseEnlistment
            );
    }
}
