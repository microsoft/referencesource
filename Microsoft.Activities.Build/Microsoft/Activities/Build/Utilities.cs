// <copyright>
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace Microsoft.Activities.Build
{
    using System;
    using System.Activities;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Runtime;
    using Microsoft.Build.Tasks.Xaml;

    internal static class Utilities
    {
        private const string InitializeComponentMethodName = "InitializeComponent";        

        internal static Activity CreateActivity(Type type, out Exception ctorException)
        {
            try
            {
                ctorException = null;
                Activity result = null;
                if (!type.ContainsGenericParameters)
                {
                    ConstructorInfo defaultConstructor = type.GetConstructor(Type.EmptyTypes);
                    if (defaultConstructor != null)
                    {
                        result = (Activity)defaultConstructor.Invoke(null);
                    }
                }

                return result;
            }
            catch (TargetInvocationException tie)
            {
                Exception ex = tie;
                while (ex != null && ex is TargetInvocationException)
                {
                    ex = ex.InnerException;
                }

                if (ex is BadImageFormatException)
                {
                    // there's an unloadable reference, this will be handled by the Task's Execute method
                    throw Fx.Exception.AsError(ex);
                }

                ctorException = ex;
                return null;
            }
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.AvoidCallingProblematicMethods,
            Justification = "Using LoadFile to avoid loading through Fusion and load the exact local assembly")]
        internal static Assembly GetLocalAssembly(BuildExtensionContext context, string errorMessage)
        {
            try
            {
                string path = Path.GetFullPath(context.LocalAssembly);
                return Assembly.LoadFile(path);
            }
            catch (Exception e)
            {
                if (Fx.IsFatal(e) || e is BadImageFormatException)
                {
                    throw;
                }

                throw FxTrace.Exception.AsError(new FileLoadException(errorMessage));
            }
        }

        internal static Type[] GetTypes(Assembly assembly)
        {
            try
            {
                return assembly.GetTypes();
            }
            catch (ReflectionTypeLoadException rtle)
            {
                foreach (Exception exception in rtle.LoaderExceptions)
                {
                    if (exception is BadImageFormatException)
                    {
                        throw FxTrace.Exception.AsError(exception);
                    }
                }

                throw;
            }
        }

        internal static bool IsTypeAuthoredInXaml(Type type)
        {
            if (type.BaseType != null && (type.BaseType == typeof(Activity) ||
                (type.BaseType.IsGenericType && type.BaseType.GetGenericTypeDefinition() == typeof(Activity<>))))
            {
                if (type.GetMethod(InitializeComponentMethodName, BindingFlags.DeclaredOnly | BindingFlags.Public | BindingFlags.Instance) != null)
                {
                    return true;
                }
            }

            return false;
        }
    }
}
