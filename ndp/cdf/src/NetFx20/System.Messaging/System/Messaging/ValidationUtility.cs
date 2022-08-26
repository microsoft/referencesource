namespace System.Messaging
{

    using System;

    internal static class ValidationUtility
    {


        public static bool ValidateAccessControlEntryType(AccessControlEntryType value)
        {
            return (value >= AccessControlEntryType.Allow) && (value <= AccessControlEntryType.Revoke);
        }


        public static bool ValidateCryptographicProviderType(CryptographicProviderType value)
        {
            return (value >= CryptographicProviderType.None) && (value <= CryptographicProviderType.SttIss);
        }


        public static bool ValidateEncryptionAlgorithm(EncryptionAlgorithm value)
        {
            //
            // note that EncryptionAlgorithm has disjoined values
            //
            return (value == EncryptionAlgorithm.None) ||
                   (value == EncryptionAlgorithm.Rc2) ||
                   (value == EncryptionAlgorithm.Rc4);
        }


        public static bool ValidateEncryptionRequired(EncryptionRequired value)
        {
            return (value >= EncryptionRequired.None) && (value <= EncryptionRequired.Body);
        }


        public static bool ValidateHashAlgorithm(HashAlgorithm value)
        {
            //
            // note that HashAlgorithm has disjoined values
            //
            return (value == HashAlgorithm.None) ||
                   (value == HashAlgorithm.Md2) ||
                   (value == HashAlgorithm.Md4) ||
                   (value == HashAlgorithm.Md5) ||
                   (value == HashAlgorithm.Sha) ||
                   (value == HashAlgorithm.Sha256) ||
                   (value == HashAlgorithm.Sha384) ||
                   (value == HashAlgorithm.Sha512) ||
                   (value == HashAlgorithm.Mac);
        }



        public static bool ValidateMessageLookupAction(MessageLookupAction value)
        {
            //
            // note that MessageLookupAction has disjoined values
            //
            return (value == MessageLookupAction.Current) ||
                   (value == MessageLookupAction.Next) ||
                   (value == MessageLookupAction.Previous) ||
                   (value == MessageLookupAction.First) ||
                   (value == MessageLookupAction.Last);
        }


        public static bool ValidateMessagePriority(MessagePriority value)
        {
            return (value >= MessagePriority.Lowest) && (value <= MessagePriority.Highest);

        }


        public static bool ValidateMessageQueueTransactionType(MessageQueueTransactionType value)
        {
            //
            // note that MessageQueueTransactionType has disjoined values
            //
            return (value == MessageQueueTransactionType.None) ||
                   (value == MessageQueueTransactionType.Automatic) ||
                   (value == MessageQueueTransactionType.Single);
        }


        public static bool ValidateQueueAccessMode(QueueAccessMode value)
        {
            //
            // note that QueueAccessMode has disjoined values
            //
            return (value == QueueAccessMode.Send) ||
                   (value == QueueAccessMode.Peek) ||
                   (value == QueueAccessMode.Receive) ||
                   (value == QueueAccessMode.PeekAndAdmin) ||
                   (value == QueueAccessMode.ReceiveAndAdmin) ||
                   (value == QueueAccessMode.SendAndReceive);
        }


        public static bool ValidateTrusteeType(TrusteeType trustee)
        {
            return (trustee >= TrusteeType.Unknown) && (trustee <= TrusteeType.Computer);
        }



    } //class ValidationUtility


}

