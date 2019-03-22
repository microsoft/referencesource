// <copyright>
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace Microsoft.VisualStudio.Activities.CSharp
{
    using System.Threading;

    internal static class CSharpEditorConstants
    {
        public static readonly string BeginMarker = "start of expression";
        public static readonly string EndMarker = "end of expression";
        public static readonly string MethodName = "CSharpExpressionPlaceHolder";

        public static readonly string TemporaryFileNameForEditing = "TemporaryGeneratedFile_E7A71F73-0F8D-4B9B-B56E-8E70B10BC5D3.cs";
        public static readonly string TemporaryFileNameForValidation = "TemporaryGeneratedFile_036C0B5B-1481-4323-8D20-8F5ADCB23D92.cs";
        public static readonly string TemporaryFileNameForTypeInfer = "TemporaryGeneratedFile_5937a670-0e60-4077-877b-f7221da3dda1.cs";
        
        public static readonly Mutex CSharpExpressionValidationFileMutex = new Mutex(false, csharpExpressionValidationFileMutexName);        
        public static readonly int ValidationFileTimeOut = 200;
        private static string csharpExpressionValidationFileMutexName = @"Global\Microsoft_VisualStudio_Workflow_Designer_" + "19eb3bbc11ae4ad89d1e6217eb80a4e5"; // prefix by "Global\" indicating this is a global mutex across terminal sessions
    }
}