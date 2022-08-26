using System;

namespace System.Transactions
{
    public interface ITransactionPromoter
    {
        Byte[] Promote();
    }
}
