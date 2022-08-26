//---------------------------------------------------------------------
// <copyright file="PathBox.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner  Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Text;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Reflection;
    using System.Linq;
    using System.Linq.Expressions;
    
    #endregion Namespaces.

    /// <summary>
    /// Holds state (Path, lambda parameter stack, etc) for projection analysis.
    /// </summary>
    internal class PathBox
    {
        #region Private fields.

        /// <summary>This class is used as a marker for an entity projected in its entirety.</summary>
        private const char EntireEntityMarker = UriHelper.ASTERISK;

        private readonly List<StringBuilder> projectionPaths = new List<StringBuilder>();

        private readonly List<StringBuilder> expandPaths = new List<StringBuilder>();

        private readonly Stack<ParameterExpression> parameterExpressions = new Stack<ParameterExpression>();

        private readonly Dictionary<ParameterExpression, string> basePaths = new Dictionary<ParameterExpression, string>(ReferenceEqualityComparer<ParameterExpression>.Instance);

        #endregion Private fields.

        /// <summary>Initializes a new <see cref="PathBox"/> instance.</summary>
        internal PathBox()
        {
            // add a default empty path.
            projectionPaths.Add(new StringBuilder());
        }

        internal IEnumerable<string> ProjectionPaths
        {
            get
            {
                return projectionPaths.Where(s => s.Length > 0).Select(s => s.ToString()).Distinct();
            }
        }

        internal IEnumerable<string> ExpandPaths
        {
            get
            {
                return expandPaths.Where(s => s.Length > 0).Select(s => s.ToString()).Distinct();
            }
        }

        internal void PushParamExpression(ParameterExpression pe)
        {
            StringBuilder basePath = projectionPaths.Last();
            basePaths.Add(pe, basePath.ToString());
            projectionPaths.Remove(basePath);
            parameterExpressions.Push(pe);
        }

        internal void PopParamExpression()
        {
            parameterExpressions.Pop();
        }

        internal ParameterExpression ParamExpressionInScope
        {
            get
            {
                Debug.Assert(parameterExpressions.Count > 0);
                return parameterExpressions.Peek();
            }
        }

        /// <summary>Starts a new path.</summary>
        internal void StartNewPath()
        {
            Debug.Assert(this.ParamExpressionInScope != null, "this.ParamExpressionInScope != null -- should not be starting new path with no lambda parameter in scope.");

            StringBuilder sb = new StringBuilder(basePaths[this.ParamExpressionInScope]);
            RemoveEntireEntityMarkerIfPresent(sb);
            expandPaths.Add(new StringBuilder(sb.ToString()));
            AddEntireEntityMarker(sb);
            projectionPaths.Add(sb);
        }

        internal void AppendToPath(PropertyInfo pi)
        {
            Debug.Assert(pi != null, "pi != null");

            StringBuilder sb;
            Type t = TypeSystem.GetElementType(pi.PropertyType);

            if (ClientType.CheckElementTypeIsEntity(t))
            {
                // an entity, so need to append to expand path also
                sb = expandPaths.Last();
                Debug.Assert(sb != null);  // there should always be an expand path because must call StartNewPath first.
                if (sb.Length > 0)
                {
                    sb.Append(UriHelper.FORWARDSLASH);
                }

                sb.Append(pi.Name);
            }

            sb = projectionPaths.Last();
            Debug.Assert(sb != null, "sb != null -- we are always building paths in the context of a parameter");

            RemoveEntireEntityMarkerIfPresent(sb);

            if (sb.Length > 0)
            {
                sb.Append(UriHelper.FORWARDSLASH);
            }

            sb.Append(pi.Name);

            if (ClientType.CheckElementTypeIsEntity(t))
            {
                AddEntireEntityMarker(sb);
            }
        }

        private static void RemoveEntireEntityMarkerIfPresent(StringBuilder sb)
        {
            if (sb.Length > 0 && sb[sb.Length - 1] == EntireEntityMarker)
            {
                sb.Remove(sb.Length - 1, 1);
            }

            if (sb.Length > 0 && sb[sb.Length - 1] == UriHelper.FORWARDSLASH)
            {
                sb.Remove(sb.Length - 1, 1);
            }
        }

        private static void AddEntireEntityMarker(StringBuilder sb)
        {
            if (sb.Length > 0)
            {
                sb.Append(UriHelper.FORWARDSLASH);
            }

            sb.Append(EntireEntityMarker);
        }
    }
}
