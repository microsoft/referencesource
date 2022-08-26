using System;
using System.Security.Permissions;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace System.Transactions
{
    internal interface IEnlistmentNotificationInternal
    {
        void Prepare(
            IPromotedEnlistment preparingEnlistment
            );
            
        void Commit(
            IPromotedEnlistment enlistment
            );
        
        void Rollback(
            IPromotedEnlistment enlistment
            );

        void InDoubt(
            IPromotedEnlistment enlistment
            );
    }


    public interface IEnlistmentNotification
    {
        void Prepare(
            PreparingEnlistment preparingEnlistment
            );
            
        void Commit(
            Enlistment enlistment
            );
        
        void Rollback(
            Enlistment enlistment
            );

        void InDoubt(
            Enlistment enlistment
            );
    }
}
