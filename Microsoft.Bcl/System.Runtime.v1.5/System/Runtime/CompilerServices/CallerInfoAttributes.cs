using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace System.Runtime.CompilerServices
{
    /// <summary>
    /// Allows you to obtain the method or property name of the caller to the method.
    /// </summary>
    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public sealed class CallerMemberNameAttribute : Attribute { }

    /// <summary>
    /// Allows you to obtain the line number in the source file at which the method is called.
    /// </summary>
    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public sealed class CallerLineNumberAttribute : Attribute { }

    /// <summary>
    /// Allows you to obtain the full path of the source file that contains the caller.
    /// This is the file path at the time of compile.
    /// </summary>
    [AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
    public sealed class CallerFilePathAttribute : Attribute { }
}
