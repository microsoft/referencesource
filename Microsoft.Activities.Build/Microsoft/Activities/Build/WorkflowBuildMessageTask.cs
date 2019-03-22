//-----------------------------------------------------------------------
//     Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------

namespace Microsoft.Activities.Build
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime;
    using Microsoft.Build.Framework;
    using Microsoft.Build.Utilities;

    public sealed class WorkflowBuildMessageTask : Task
    {
        public WorkflowBuildMessageTask()
            : base(new System.Resources.ResourceManager("Microsoft.Activities.Build.SR", System.Reflection.Assembly.GetExecutingAssembly()))
        {
        }

        [Required()]
        public string ResourceName
        {
            get;
            set;
        }

        public string MessageType
        {
            get;
            set;
        }
        
        public override bool Execute()
        {
            if (string.IsNullOrWhiteSpace(this.MessageType))
            {
                this.MessageType = "Message";
            }

            try
            {
                if (string.Equals(this.MessageType, "Error", StringComparison.OrdinalIgnoreCase))
                {
                    Log.LogErrorFromResources(this.ResourceName, null);
                    return false;
                }
                else if (string.Equals(this.MessageType, "Warning", StringComparison.OrdinalIgnoreCase))
                {
                    Log.LogWarningFromResources(this.ResourceName, null);
                    return true;
                }
                else if (string.Equals(this.MessageType, "Message", StringComparison.OrdinalIgnoreCase))
                {
                    Log.LogMessageFromResources(this.ResourceName, null);
                    return true;
                }
                else
                {
                    Log.LogError(SR.InvalidType(this.MessageType));
                    return false;
                }
            }
            catch (ArgumentException)
            {
                Log.LogError(SR.InvalidCode(this.ResourceName));
                return false;
            }
        }
    }
}
