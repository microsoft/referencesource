//----------------------------------------------------------------
// <copyright company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------

namespace Microsoft.Activities.Build.Validation
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Runtime;
    using Microsoft.Build.Framework;
    using Microsoft.Build.Utilities;

    /// <summary>
    /// Wait until Compile target runs successfully, 
    /// and then if validation errors's been reported, simply return failure
    /// so that the entire build process can terminate.
    /// </summary>
    public class DeferredValidationTask : Task
    {
        /// <summary>
        /// Gets or sets the DeferredValidationErrorsFilePath property.
        /// </summary>
        [Required]
        public string DeferredValidationErrorsFilePath { get; set; }

        /// <summary>
        /// Executes to check to see if validation errors' been already reported,
        /// and if yes, fail the entire build now.
        /// </summary>
        /// <returns>Returns true if validation errors's already been reported. Returns false otherwise.</returns>
        public override bool Execute()
        {
            if (File.Exists(this.DeferredValidationErrorsFilePath))
            {
                List<ValidationBuildExtension.Violation> violations = ReportDeferredValidationErrorsTask.LoadDeferredValidationErrors(this.DeferredValidationErrorsFilePath);

                if (ErrorExists(violations))
                {
                    // the validation errors must have already been reported/emitted in ReportDeferredValidationErrorsTask.
                    // the goal of this task is to simply fail the entire build process after CoreCompile target has succeeded.
                    return false;
                }
            }

            return true;
        }

        private static bool ErrorExists(List<ValidationBuildExtension.Violation> violations)
        {
            if (violations != null && violations.Count > 0)
            {
                foreach (ValidationBuildExtension.Violation violation in violations)
                {
                    if (!violation.IsWarning)
                    {
                        return true;
                    }
                }
            }            

            return false;
        }
    }
}
