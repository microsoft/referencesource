//-----------------------------------------------------------------------------
// <copyright file="AppSettings.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions.Configuration
{
    using System;
    using System.Collections.Specialized;
    using System.Configuration;

    internal static class AppSettings
    {
        // Make the default value negative so the fix is opt-in.
        private const int DefaultContextKeyRemotingLeaseLifetimeInMinutes = -1;

        private static volatile bool settingsInitalized = false;
        private static object appSettingsLock = new object();

        // Bug 954268 Extend Default TransactionException to include the Distributed Transaction ID in error message
        // When the appsetting "Transactions:IncludeDistributedTransactionIdInExceptionMessage" is "true", we display the distributed transaction ID in TransactionException message if the distributed transaction ID is availble.
        private static bool includeDistributedTxIdInExceptionMessage;

        // Bug 669153 and 516454 Remoting calls made within a transaction can lead to Transaction.Current incorrectly returning null
        // The value from the "Transactions:ContextKeyRemotingLeaseLifetimeInMinutes" app setting.
        // Values include:
        // < 0 - Don't modify the lease lifetime of ContextKey, just use what is done for the AppDomain.
        // 0 - use the default value of DefaultContextKeyRemotingLeaseLifetimeInMinutes, as if not specified.
        // > 0 - specifies the remoting Lease Lifetime value, in minutes, to use for the ContextKey objects.
        private static int contextKeyRemotingLeaseLifetimeInMinutes;

        // Bug 954268 Extend Default TransactionException to include the Distributed Transaction ID in error message
        // When the appsetting "Transactions:IncludeDistributedTransactionIdInExceptionMessage" is "true", we display the distributed transaction ID in TransactionException message if the distributed transaction ID is availble.
        internal static bool IncludeDistributedTxIdInExceptionMessage
        {
            get
            {
                EnsureSettingsLoaded();
                return includeDistributedTxIdInExceptionMessage;
            }
        }

        // Bug 669153 and 516454 Remoting calls made within a transaction can lead to Transaction.Current incorrectly returning null
        // When the appsetting "Transactions:ContextKeyRemotingLeaseLifetimeInMinutes" is specified and greater than 0, it specifies the remoting Lease Lifetime value, in minutes, for the ContextKey objects.
        internal static int ContextKeyRemotingLeaseLifetimeInMinutes
        {
            get
            {
                EnsureSettingsLoaded();
                return contextKeyRemotingLeaseLifetimeInMinutes;
            }
        }

        private static void EnsureSettingsLoaded()
        {
            if (!settingsInitalized)
            {
                lock (appSettingsLock)
                {
                    if (!settingsInitalized)
                    {
                        NameValueCollection settings = null;
                        try
                        {
                            settings = ConfigurationManager.AppSettings;
                        }
                        catch (ConfigurationErrorsException)
                        {
                        }
                        finally
                        {
                            if (settings == null || !bool.TryParse(settings["Transactions:IncludeDistributedTransactionIdInExceptionMessage"], out includeDistributedTxIdInExceptionMessage))
                            {
                                includeDistributedTxIdInExceptionMessage = false;
                            }

                            if (settings == null || !int.TryParse(settings["Transactions:ContextKeyRemotingLeaseLifetimeInMinutes"], out contextKeyRemotingLeaseLifetimeInMinutes))
                            {
                                contextKeyRemotingLeaseLifetimeInMinutes = DefaultContextKeyRemotingLeaseLifetimeInMinutes;
                            }
                            else if (contextKeyRemotingLeaseLifetimeInMinutes == 0)
                            {
                                // If 0 was specified, revert to using the default, as if the setting wasn't specified.
                                // Note that a value < 0 is handled in the code where this app setting is consumed and it is treated as
                                // an opt-out and the lease lifetime of the ContextKey will not be modified away from the AppDomain value.
                                contextKeyRemotingLeaseLifetimeInMinutes = DefaultContextKeyRemotingLeaseLifetimeInMinutes;
                            }

                            settingsInitalized = true;
                        }
                    }
               }
            }
        }
    }
}
