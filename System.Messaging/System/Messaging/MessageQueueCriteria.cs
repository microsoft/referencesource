//------------------------------------------------------------------------------
// <copyright file="MessageQueueCriteria.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    using System.Diagnostics;
    using System;
    using System.Messaging.Interop;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Security.Permissions;
    using System.Globalization; //for CultureInfo

    /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria"]/*' />
    /// <devdoc>
    ///    <para>
    ///       This class
    ///       is used to filter MessageQueues when performing a
    ///       query in the network, through MessageQueue.GetPublicQueues method.
    ///    </para>
    /// </devdoc>
    public class MessageQueueCriteria
    {
        private DateTime createdAfter;
        private DateTime createdBefore;
        private string label;
        private string machine;
        private DateTime modifiedAfter;
        private DateTime modifiedBefore;
        private Guid category;
        private CriteriaPropertyFilter filter = new CriteriaPropertyFilter();
        private Restrictions restrictions;
        private Guid machineId;
        private static DateTime minDate = new DateTime(1970, 1, 1);
        private static DateTime maxDate = new DateTime(2038, 1, 19);

        /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria.CreatedAfter"]/*' />
        /// <devdoc>
        ///    Specifies the lower bound of the interval 
        ///    that will be used as  the queue creation time
        ///    search criteria.
        /// </devdoc>
        public DateTime CreatedAfter
        {
            get
            {
                if (!this.filter.CreatedAfter)
                    throw new InvalidOperationException(Res.GetString(Res.CriteriaNotDefined));

                return this.createdAfter;
            }

            set
            {
                if (value < MessageQueueCriteria.minDate || value > MessageQueueCriteria.maxDate)
                    throw new ArgumentException(Res.GetString(Res.InvalidDateValue, MessageQueueCriteria.minDate.ToString(CultureInfo.CurrentCulture), MessageQueueCriteria.maxDate.ToString(CultureInfo.CurrentCulture)));

                this.createdAfter = value;
                if (this.filter.CreatedBefore && this.createdAfter > this.createdBefore)
                    this.createdBefore = this.createdAfter;

                this.filter.CreatedAfter = true;
            }
        }

        /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria.CreatedBefore"]/*' />
        /// <devdoc>
        ///    Specifies the upper bound of the interval 
        ///    that will be used as  the queue creation time
        ///    search criteria.
        /// </devdoc>
        public DateTime CreatedBefore
        {
            get
            {
                if (!this.filter.CreatedBefore)
                    throw new InvalidOperationException(Res.GetString(Res.CriteriaNotDefined));

                return this.createdBefore;
            }

            set
            {
                if (value < MessageQueueCriteria.minDate || value > MessageQueueCriteria.maxDate)
                    throw new ArgumentException(Res.GetString(Res.InvalidDateValue, MessageQueueCriteria.minDate.ToString(CultureInfo.CurrentCulture), MessageQueueCriteria.maxDate.ToString(CultureInfo.CurrentCulture)));

                this.createdBefore = value;
                if (this.filter.CreatedAfter && this.createdAfter > this.createdBefore)
                    this.createdAfter = this.createdBefore;

                this.filter.CreatedBefore = true;
            }
        }

        internal bool FilterMachine
        {
            get
            {
                return this.filter.MachineName;
            }
        }

        /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria.Label"]/*' />
        /// <devdoc>
        ///    Specifies the label that that will be used as 
        ///    the criteria to search queues in the network.        
        /// </devdoc>
        public string Label
        {
            get
            {
                if (!this.filter.Label)
                    throw new InvalidOperationException(Res.GetString(Res.CriteriaNotDefined));

                return this.label;
            }

            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                this.label = value;
                this.filter.Label = true;
            }
        }

        /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria.MachineName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the machine name that will be used
        ///       as the criteria to search queues in the network.
        ///    </para>
        /// </devdoc>
        public string MachineName
        {
            get
            {
                if (!this.filter.MachineName)
                    throw new InvalidOperationException(Res.GetString(Res.CriteriaNotDefined));

                return this.machine;
            }

            set
            {
                if (!SyntaxCheck.CheckMachineName(value))
                    throw new ArgumentException(Res.GetString(Res.InvalidProperty, "MachineName", value));

                //SECREVIEW: Setting this property shouldn't demmand any permissions,
                //                    the machine id will only be used internally.
                MessageQueuePermission permission = new MessageQueuePermission(PermissionState.Unrestricted);
                permission.Assert();
                try
                {
                    this.machineId = MessageQueue.GetMachineId(value);
                }
                finally
                {
                    MessageQueuePermission.RevertAssert();
                }

                this.machine = value;
                this.filter.MachineName = true;
            }
        }

        /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria.ModifiedAfter"]/*' />
        /// <devdoc>
        ///    Specifies the lower bound of the interval 
        ///    that will be used as  the queue modified time
        ///    search criteria.
        /// </devdoc>
        public DateTime ModifiedAfter
        {
            get
            {
                if (!this.filter.ModifiedAfter)
                    throw new InvalidOperationException(Res.GetString(Res.CriteriaNotDefined));

                return this.modifiedAfter;
            }

            set
            {
                if (value < MessageQueueCriteria.minDate || value > MessageQueueCriteria.maxDate)
                    throw new ArgumentException(Res.GetString(Res.InvalidDateValue, MessageQueueCriteria.minDate.ToString(CultureInfo.CurrentCulture), MessageQueueCriteria.maxDate.ToString(CultureInfo.CurrentCulture)));

                this.modifiedAfter = value;

                if (this.filter.ModifiedBefore && this.modifiedAfter > this.modifiedBefore)
                    this.modifiedBefore = this.modifiedAfter;

                this.filter.ModifiedAfter = true;
            }
        }

        /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria.ModifiedBefore"]/*' />
        /// <devdoc>
        ///    Specifies the upper bound of the interval 
        ///    that will be used as  the queue modified time
        ///    search criteria.
        /// </devdoc>
        public DateTime ModifiedBefore
        {
            get
            {
                if (!this.filter.ModifiedBefore)
                    throw new InvalidOperationException(Res.GetString(Res.CriteriaNotDefined));

                return this.modifiedBefore;
            }

            set
            {
                if (value < MessageQueueCriteria.minDate || value > MessageQueueCriteria.maxDate)
                    throw new ArgumentException(Res.GetString(Res.InvalidDateValue, MessageQueueCriteria.minDate.ToString(CultureInfo.CurrentCulture), MessageQueueCriteria.maxDate.ToString(CultureInfo.CurrentCulture)));

                this.modifiedBefore = value;

                if (this.filter.ModifiedAfter && this.modifiedAfter > this.modifiedBefore)
                    this.modifiedAfter = this.modifiedBefore;

                this.filter.ModifiedBefore = true;
            }
        }

        /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria.Reference"]/*' />
        /// <internalonly/>
        internal Restrictions.MQRESTRICTION Reference
        {
            get
            {
                int size = 0;
                if (this.filter.CreatedAfter)
                    ++size;
                if (this.filter.CreatedBefore)
                    ++size;
                if (this.filter.Label)
                    ++size;
                if (this.filter.ModifiedAfter)
                    ++size;
                if (this.filter.ModifiedBefore)
                    ++size;
                if (this.filter.Category)
                    ++size;

                restrictions = new Restrictions(size);
                if (this.filter.CreatedAfter)
                    restrictions.AddI4(NativeMethods.QUEUE_PROPID_CREATE_TIME, Restrictions.PRGT, ConvertTime(this.createdAfter));
                if (this.filter.CreatedBefore)
                    restrictions.AddI4(NativeMethods.QUEUE_PROPID_CREATE_TIME, Restrictions.PRLE, ConvertTime(this.createdBefore));
                if (this.filter.Label)
                    restrictions.AddString(NativeMethods.QUEUE_PROPID_LABEL, Restrictions.PREQ, this.label);
                if (this.filter.ModifiedAfter)
                    restrictions.AddI4(NativeMethods.QUEUE_PROPID_MODIFY_TIME, Restrictions.PRGT, ConvertTime(this.modifiedAfter));
                if (this.filter.ModifiedBefore)
                    restrictions.AddI4(NativeMethods.QUEUE_PROPID_MODIFY_TIME, Restrictions.PRLE, ConvertTime(this.modifiedBefore));
                if (this.filter.Category)
                    restrictions.AddGuid(NativeMethods.QUEUE_PROPID_TYPE, Restrictions.PREQ, this.category);

                return this.restrictions.GetRestrictionsRef();
            }
        }

        /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria.Category"]/*' />
        /// <devdoc>
        ///    Specifies the Category that will be used 
        ///    as the criteria to search queues in the network.        
        /// </devdoc>
        public Guid Category
        {
            get
            {
                if (!this.filter.Category)
                    throw new InvalidOperationException(Res.GetString(Res.CriteriaNotDefined));

                return this.category;
            }

            set
            {
                this.category = value;
                this.filter.Category = true;
            }
        }

        /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria.ClearAll"]/*' />
        /// <devdoc>
        ///    Resets all the current instance settings.
        /// </devdoc>
        public void ClearAll()
        {
            this.filter.ClearAll();
        }

        /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria.ConvertTime"]/*' />
        /// <internalonly/>
        private int ConvertTime(DateTime time)
        {
            time = time.ToUniversalTime();
            return (int)(time - MessageQueueCriteria.minDate).TotalSeconds;
        }

        /// <include file='doc\MessageQueueCriteria.uex' path='docs/doc[@for="MessageQueueCriteria.CriteriaPropertyFilter"]/*' />
        /// <internalonly/>
        private class CriteriaPropertyFilter
        {
            public bool CreatedAfter;
            public bool CreatedBefore;
            public bool Label;
            public bool MachineName;
            public bool ModifiedAfter;
            public bool ModifiedBefore;
            public bool Category;

            public void ClearAll()
            {
                this.CreatedAfter = false;
                this.CreatedBefore = false;
                this.Label = false;
                this.MachineName = false;
                this.ModifiedAfter = false;
                this.ModifiedBefore = false;
                this.Category = false;
            }
        }
    }
}
