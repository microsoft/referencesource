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
    using System.Runtime.Serialization;
    using Microsoft.Build.Framework;
    using Microsoft.Build.Utilities;    

    /// <summary>
    /// Reports validation errors cached immediately following XamlMarkupCompilePass2, 
    /// where ValidationBuildExtesion ran,
    /// and always return success so the build contineus.
    /// </summary>
    public class ReportDeferredValidationErrorsTask : Task
    {
        /// <summary>
        /// Gets or sets the DeferredValidationErrorsFilePath property.
        /// </summary>
        [Required]
        public string DeferredValidationErrorsFilePath { get; set; }

        /// <summary>
        /// Executes to report validation error.
        /// </summary>
        /// <returns>Always returns true.</returns>
        public override bool Execute()
        {
            if (File.Exists(this.DeferredValidationErrorsFilePath))
            {
                List<ValidationBuildExtension.Violation> violations = LoadDeferredValidationErrors(this.DeferredValidationErrorsFilePath);

                if (violations != null && violations.Count > 0)
                {
                    foreach (ValidationBuildExtension.Violation violation in violations)
                    {
                        violation.Emit(this.Log);
                    }
                }                
            }

            return true;
        }

        internal static List<ValidationBuildExtension.Violation> LoadDeferredValidationErrors(string filePath)
        {
            List<ValidationBuildExtension.Violation> violations = null;
            using (FileStream stream = new FileStream(filePath, FileMode.Open))
            {
                DataContractSerializer serializer = new DataContractSerializer(typeof(List<ValidationBuildExtension.Violation>));
                violations = (List<ValidationBuildExtension.Violation>)serializer.ReadObject(stream);
            }

            return violations;
        }
    }    
}
