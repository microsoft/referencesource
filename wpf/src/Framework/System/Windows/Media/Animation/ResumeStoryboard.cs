/***************************************************************************\
*
* File: ResumeStoryboard.cs
*
* This object includes a Storyboard reference.  When triggered, the Storyboard
*  resumes.
*
* Copyright (C) by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/
using System.Diagnostics;               // Debug.Assert

namespace System.Windows.Media.Animation
{
/// <summary>
/// ResumeStoryboard will call resume on its Storyboard reference when
///  it is triggered.
/// </summary>
public sealed class ResumeStoryboard : ControllableStoryboardAction
{
    /// <summary>
    ///     Called when it's time to execute this storyboard action
    /// </summary>
    internal override void Invoke( FrameworkElement containingFE, FrameworkContentElement containingFCE, Storyboard storyboard )
    {
        Debug.Assert( containingFE != null || containingFCE != null,
            "Caller of internal function failed to verify that we have a FE or FCE - we have neither." );

        if( containingFE != null )
        {
            storyboard.Resume(containingFE);
        }
        else
        {
            storyboard.Resume(containingFCE);
        }
    }
}
}
