//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    public sealed class LogPolicy
    {
        [Flags]
        enum ModifiedFields
        {
            None = 0x00,
            MinimumExtentCount = 0x01,
            MaximumExtentCount = 0x02,
            GrowthRate = 0x04,
            PinnedTailThreshold = 0x08,
            AutoShrinkPercentage = 0x10,
            NewExtentPrefix = 0x20,
            NextExtentSuffix = 0x40,
            AutoGrow = 0x80,
        }

        LogStore store;
        ModifiedFields modifiedFields;

        int minimumExtentCount;
        int maximumExtentCount;
        PolicyUnit growthRate;
        PolicyUnit pinnedTailThreshold;
        int autoShrinkPercentage;
        string newExtentPrefix;
        long nextExtentSuffix;
        bool autoGrow;

        internal LogPolicy(LogStore logStore)
        {
            this.store = logStore;
            Refresh();
        }

        public bool AutoGrow
        {
            get { return this.autoGrow; }
            set
            {
                this.autoGrow = value;
                this.modifiedFields |= ModifiedFields.AutoGrow;
            }
        }

        public int MinimumExtentCount
        {
            get { return this.minimumExtentCount; }
            set
            {
                if (value < 2)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("value"));

                this.minimumExtentCount = value;
                this.modifiedFields |= ModifiedFields.MinimumExtentCount;
            }
        }

        public int MaximumExtentCount
        {
            get { return this.maximumExtentCount; }
            set
            {
                if (value < 2)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("value"));

                this.maximumExtentCount = value;
                this.modifiedFields |= ModifiedFields.MaximumExtentCount;
            }
        }

        public PolicyUnit GrowthRate
        {
            get { return this.growthRate; }
            set
            {
                this.growthRate = value;
                this.modifiedFields |= ModifiedFields.GrowthRate;
            }
        }

        public PolicyUnit PinnedTailThreshold
        {
            get { return this.pinnedTailThreshold; }
            set
            {
                this.pinnedTailThreshold = value;
                this.modifiedFields |= ModifiedFields.PinnedTailThreshold;
            }
        }

        public int AutoShrinkPercentage
        {
            get { return this.autoShrinkPercentage; }
            set
            {
                if ((value < 0) || (value > 100))
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("value"));

                this.autoShrinkPercentage = value;
                this.modifiedFields |= ModifiedFields.AutoShrinkPercentage;
            }
        }

        public string NewExtentPrefix
        {
            get { return this.newExtentPrefix; }
            set
            {
#pragma warning suppress 56507
                if (value == null)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("value"));
#pragma warning suppress 56507
                if ((value.Length == 0) || (value.Length > Const.MAX_PATH))
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("value"));

                this.newExtentPrefix = value;
                this.modifiedFields |= ModifiedFields.NewExtentPrefix;
            }
        }

        public long NextExtentSuffix
        {
            get { return this.nextExtentSuffix; }
            set
            {
                if (value < 0)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentOutOfRange("value"));

                this.nextExtentSuffix = value;
                this.modifiedFields |= ModifiedFields.NextExtentSuffix;
            }
        }

        public void Commit()
        {
            CommitMaximumSize();
            CommitMinimumSize();
            CommitGrowthRate();
            CommitPinnedTailThreshold();
            CommitAutoShrinkPercentage();
            CommitAutoGrow();
            CommitNewExtentPrefix();
            CommitNextExtentSuffix();
        }

        public void Refresh()
        {
            RefreshMaximumSize();
            RefreshMinimumSize();
            RefreshGrowthRate();
            RefreshPinnedTailThreshold();
            RefreshAutoShrinkPercentage();
            RefreshAutoGrow();
            RefreshNewExtentPrefix();
            RefreshNextExtentSuffix();
        }

        void CommitAutoGrow()
        {
            if (0 != (this.modifiedFields & ModifiedFields.AutoGrow))
            {
                CLFS_MGMT_POLICY_AUTOGROW policy;
                policy = new CLFS_MGMT_POLICY_AUTOGROW(
                    Const.LOG_POLICY_OVERWRITE | Const.LOG_POLICY_PERSIST);
                policy.Enabled = (uint)(this.autoGrow ? 1 : 0);

                UnsafeNativeMethods.InstallLogPolicy(this.store.Handle,
                                                     ref policy);
                this.modifiedFields &= ~ModifiedFields.AutoGrow;
            }
        }

        void CommitMaximumSize()
        {
            if (0 != (this.modifiedFields & ModifiedFields.MaximumExtentCount))
            {
                CLFS_MGMT_POLICY_MAXIMUMSIZE policy;
                policy = new CLFS_MGMT_POLICY_MAXIMUMSIZE(
                    Const.LOG_POLICY_OVERWRITE | Const.LOG_POLICY_PERSIST);
                policy.Containers = (uint)this.maximumExtentCount;

                UnsafeNativeMethods.InstallLogPolicy(this.store.Handle,
                                                     ref policy);
                this.modifiedFields &= ~ModifiedFields.MaximumExtentCount;
            }
        }

        void CommitMinimumSize()
        {
            if (0 != (this.modifiedFields & ModifiedFields.MinimumExtentCount))
            {
                CLFS_MGMT_POLICY_MINIMUMSIZE policy;
                policy = new CLFS_MGMT_POLICY_MINIMUMSIZE(
                    Const.LOG_POLICY_OVERWRITE | Const.LOG_POLICY_PERSIST);
                policy.Containers = (uint)this.minimumExtentCount;

                UnsafeNativeMethods.InstallLogPolicy(this.store.Handle,
                                                     ref policy);
                this.modifiedFields &= ~ModifiedFields.MinimumExtentCount;
            }
        }

        void CommitGrowthRate()
        {
            if (0 != (this.modifiedFields & ModifiedFields.GrowthRate))
            {
                CLFS_MGMT_POLICY_GROWTHRATE policy;
                policy = new CLFS_MGMT_POLICY_GROWTHRATE(
                    Const.LOG_POLICY_OVERWRITE | Const.LOG_POLICY_PERSIST);

                if (this.growthRate.Type == PolicyUnitType.Extents)
                {
                    policy.AbsoluteGrowthInContainers = (uint)this.growthRate.Value;
                    policy.RelativeGrowthPercentage = 0;
                }
                else
                {
                    policy.RelativeGrowthPercentage = (uint)this.growthRate.Value;
                    policy.AbsoluteGrowthInContainers = 0;
                }

                UnsafeNativeMethods.InstallLogPolicy(this.store.Handle,
                                                     ref policy);
                this.modifiedFields &= ~ModifiedFields.GrowthRate;
            }
        }

        void CommitPinnedTailThreshold()
        {
            if (0 != (this.modifiedFields & ModifiedFields.PinnedTailThreshold))
            {
                CLFS_MGMT_POLICY_LOGTAIL policy;
                policy = new CLFS_MGMT_POLICY_LOGTAIL(
                    Const.LOG_POLICY_OVERWRITE | Const.LOG_POLICY_PERSIST);

                if (this.pinnedTailThreshold.Type == PolicyUnitType.Extents)
                {
                    policy.MinimumAvailableContainers = (uint)this.pinnedTailThreshold.Value;
                    policy.MinimumAvailablePercentage = 0;
                }
                else
                {
                    policy.MinimumAvailablePercentage = (uint)this.pinnedTailThreshold.Value;
                    policy.MinimumAvailableContainers = 0;
                }

                UnsafeNativeMethods.InstallLogPolicy(this.store.Handle,
                                                     ref policy);
                this.modifiedFields &= ~ModifiedFields.PinnedTailThreshold;
            }
        }

        void CommitAutoShrinkPercentage()
        {
            if (0 != (this.modifiedFields & ModifiedFields.AutoShrinkPercentage))
            {
                CLFS_MGMT_POLICY_AUTOSHRINK policy;
                policy = new CLFS_MGMT_POLICY_AUTOSHRINK(
                    Const.LOG_POLICY_OVERWRITE | Const.LOG_POLICY_PERSIST);
                policy.Percentage = (uint)this.autoShrinkPercentage;

                UnsafeNativeMethods.InstallLogPolicy(this.store.Handle,
                                                     ref policy);
                this.modifiedFields &= ~ModifiedFields.AutoShrinkPercentage;
            }
        }

        void CommitNewExtentPrefix()
        {
            if (0 != (this.modifiedFields & ModifiedFields.NewExtentPrefix))
            {
                CLFS_MGMT_POLICY_NEWCONTAINERPREFIX policy;
                policy = new CLFS_MGMT_POLICY_NEWCONTAINERPREFIX(
                    Const.LOG_POLICY_OVERWRITE | Const.LOG_POLICY_PERSIST);

                // The prefix length should not include the NULL terminator. 
                // Including NULL terminator will cause the CLFS container 
                // name to have embedded NULL and container creation will fail. 
                policy.PrefixLengthInBytes = (ushort)((this.newExtentPrefix.Length) * 2);
                policy.PrefixString = this.newExtentPrefix;

                UnsafeNativeMethods.InstallLogPolicy(this.store.Handle,
                                                     ref policy);
                this.modifiedFields &= ~ModifiedFields.NewExtentPrefix;
            }
        }

        void CommitNextExtentSuffix()
        {
            if (0 != (this.modifiedFields & ModifiedFields.NextExtentSuffix))
            {
                CLFS_MGMT_POLICY_NEXTCONTAINERSUFFIX policy;
                policy = new CLFS_MGMT_POLICY_NEXTCONTAINERSUFFIX(
                    Const.LOG_POLICY_OVERWRITE | Const.LOG_POLICY_PERSIST);
                policy.NextContainerSuffix = (ulong)this.nextExtentSuffix;

                UnsafeNativeMethods.InstallLogPolicy(this.store.Handle,
                                                     ref policy);
                this.modifiedFields &= ~ModifiedFields.NextExtentSuffix;
            }
        }

        void RefreshAutoGrow()
        {
            CLFS_MGMT_POLICY_AUTOGROW policy;
            UnsafeNativeMethods.QueryLogPolicy(this.store.Handle, out policy);

            this.autoGrow = (policy.Enabled != 0);
            this.modifiedFields &= ~ModifiedFields.AutoGrow;
        }

        void RefreshMaximumSize()
        {
            CLFS_MGMT_POLICY_MAXIMUMSIZE policy;
            UnsafeNativeMethods.QueryLogPolicy(this.store.Handle, out policy);

            this.maximumExtentCount = (int)policy.Containers;
            this.modifiedFields &= ~ModifiedFields.MaximumExtentCount;
        }

        void RefreshMinimumSize()
        {
            CLFS_MGMT_POLICY_MINIMUMSIZE policy;
            UnsafeNativeMethods.QueryLogPolicy(this.store.Handle, out policy);

            this.minimumExtentCount = (int)policy.Containers;
            this.modifiedFields &= ~ModifiedFields.MinimumExtentCount;
        }

        void RefreshGrowthRate()
        {
            CLFS_MGMT_POLICY_GROWTHRATE policy;
            UnsafeNativeMethods.QueryLogPolicy(this.store.Handle, out policy);

            if (policy.AbsoluteGrowthInContainers != 0)
            {
                this.growthRate = PolicyUnit.Extents(
                    (int)policy.AbsoluteGrowthInContainers);
            }
            else
            {
                this.growthRate = PolicyUnit.Percentage(
                    (int)policy.RelativeGrowthPercentage);
            }
            this.modifiedFields &= ~ModifiedFields.GrowthRate;
        }

        void RefreshPinnedTailThreshold()
        {
            CLFS_MGMT_POLICY_LOGTAIL policy;
            UnsafeNativeMethods.QueryLogPolicy(this.store.Handle, out policy);

            if (policy.MinimumAvailablePercentage != 0)
            {
                this.pinnedTailThreshold = PolicyUnit.Percentage(
                    (int)policy.MinimumAvailablePercentage);
            }
            else
            {
                this.pinnedTailThreshold = PolicyUnit.Extents(
                    (int)policy.MinimumAvailableContainers);
            }
            this.modifiedFields &= ~ModifiedFields.PinnedTailThreshold;
        }

        void RefreshAutoShrinkPercentage()
        {
            CLFS_MGMT_POLICY_AUTOSHRINK policy;
            UnsafeNativeMethods.QueryLogPolicy(this.store.Handle, out policy);

            this.autoShrinkPercentage = (int)policy.Percentage;
            this.modifiedFields &= ~ModifiedFields.AutoShrinkPercentage;
        }

        void RefreshNewExtentPrefix()
        {
            CLFS_MGMT_POLICY_NEWCONTAINERPREFIX policy;

            UnsafeNativeMethods.QueryLogPolicy(this.store.Handle, out policy);

            this.newExtentPrefix = policy.PrefixString;
            this.modifiedFields &= ~ModifiedFields.NewExtentPrefix;
        }

        void RefreshNextExtentSuffix()
        {
            CLFS_MGMT_POLICY_NEXTCONTAINERSUFFIX policy;
            UnsafeNativeMethods.QueryLogPolicy(this.store.Handle, out policy);

            this.nextExtentSuffix = (long)policy.NextContainerSuffix;
            this.modifiedFields &= ~ModifiedFields.NextExtentSuffix;
        }
    }
}
