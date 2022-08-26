//---------------------------------------------------------------------
// <copyright file="Util.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// static utility functions
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System.Collections;
    using System.Diagnostics;
    using System.Xml;
    using System.Reflection;
    using System.Linq.Expressions;

    #endregion Namespaces.

    /// <summary>
    /// static utility function
    /// </summary>
    internal static class Util
    {
        /// <summary>
        /// String Suffix on outgoing version numbers
        /// </summary>
        internal const string VersionSuffix = ";NetFx";

        /// <summary>Tool name for the GeneratedCode attribute used by Astoria CodeGen</summary>
        internal const string CodeGeneratorToolName = "System.Data.Services.Design";

        /// <summary>
        /// Empty Data Service Version - represents a blank DSV header
        /// </summary>
        internal static readonly Version DataServiceVersionEmpty = new Version(0, 0);

        /// <summary>
        /// Data Service Version 1
        /// </summary>
        internal static readonly Version DataServiceVersion1 = new Version(1, 0);

        /// <summary>
        /// Data Service Version 2
        /// </summary>
        internal static readonly Version DataServiceVersion2 = new Version(2, 0);

        /// <summary>
        /// Default maximum data service version this client can handle
        /// </summary>
        internal static readonly Version MaxResponseVersion = DataServiceVersion2;

        /// <summary>
        /// Data service versions supported on the client
        /// </summary>
        internal static readonly Version[] SupportedResponseVersions = 
        { 
            DataServiceVersion1,
            DataServiceVersion2
        };

        /// <summary>forward slash char array for triming uris</summary>
        internal static readonly char[] ForwardSlash = new char[1] { '/' };

        /// <summary>
        /// static char[] for indenting whitespace when tracing xml
        /// </summary>
        private static char[] whitespaceForTracing = new char[] { '\r', '\n', ' ', ' ', ' ', ' ', ' ' };

#if DEBUG
        /// <summary>
        /// DebugFaultInjector is a test hook to inject faults in specific locations. The string is the ID for the location
        /// </summary>
        private static Action<string> DebugFaultInjector = new Action<string>((s) => { });

        /// <summary>
        /// ReferenceIdentity is a test hook to help verify we dont' use identity instead of editLink
        /// </summary>
        private static Func<String, String> referenceIdentity = delegate(String identity)
        {
            return identity;
        };

        /// <summary>
        /// DereferenceIdentity is a test hook to help verify we dont' use identity instead of editLink
        /// </summary>
        private static Func<String, String> dereferenceIdentity = delegate(String identity)
        {
            return identity;
        };
#endif

        /// <summary>
        /// DebugInjectFault is a test hook to inject faults in specific locations. The string is the ID for the location
        /// </summary>
        /// <param name="state">The injector state parameter</param>
        [Conditional("DEBUG")]
        internal static void DebugInjectFault(string state)
        {
#if DEBUG
            DebugFaultInjector(state);
#endif
        }

        /// <summary>
        /// ReferenceIdentity is a test hook to help verify we dont' use identity instead of editLink
        /// </summary>
        /// <param name="uri">The uri argument</param>
        /// <returns>Returned value by the referenceIdentity hook</returns>
        internal static String ReferenceIdentity(String uri)
        {
#if DEBUG
            return referenceIdentity(uri);
#else
            return uri;
#endif
        }

        /// <summary>
        /// DereferenceIdentity is a test hook to help verify we dont' use identity instead of editLink
        /// </summary>
        /// <param name="uri">The uri argument</param>
        /// <returns>returned value by the dereferenceIdentity hook</returns>
        internal static String DereferenceIdentity(String uri)
        {
#if DEBUG
            return dereferenceIdentity(uri);
#else
            return uri;
#endif
        }

        /// <summary>
        /// Checks the argument value for null and throw ArgumentNullException if it is null
        /// </summary>
        /// <typeparam name="T">type of the argument to prevent accidental boxing of value types</typeparam>
        /// <param name="value">argument whose value needs to be checked</param>
        /// <param name="parameterName">name of the argument</param>
        /// <exception cref="System.ArgumentNullException">if value is null</exception>
        /// <returns>value</returns>
        internal static T CheckArgumentNull<T>(T value, string parameterName) where T : class
        {
            if (null == value)
            {
                throw Error.ArgumentNull(parameterName);
            }

            return value;
        }

        /// <summary>
        /// Checks the string value is not empty
        /// </summary>
        /// <param name="value">value to check </param>
        /// <param name="parameterName">parameterName of public function</param>
        /// <exception cref="System.ArgumentNullException">if value is null</exception>
        /// <exception cref="System.ArgumentException">if value is empty</exception>
        internal static void CheckArgumentNotEmpty(string value, string parameterName)
        {
            CheckArgumentNull(value, parameterName);
            if (0 == value.Length)
            {
                throw Error.Argument(Strings.Util_EmptyString, parameterName);
            }
        }

        /// <summary>
        /// Checks the array value is not empty
        /// </summary>
        /// <typeparam name="T">type of the argument to prevent accidental boxing of value types</typeparam>
        /// <param name="value">value to check </param>
        /// <param name="parameterName">parameterName of public function</param>
        /// <exception cref="System.ArgumentNullException">if value is null</exception>
        /// <exception cref="System.ArgumentException">if value is empty or contains null elements</exception>
        internal static void CheckArgumentNotEmpty<T>(T[] value, string parameterName) where T : class
        {
            CheckArgumentNull(value, parameterName);
            if (0 == value.Length)
            {
                throw Error.Argument(Strings.Util_EmptyArray, parameterName);
            }

            for (int i = 0; i < value.Length; ++i)
            {
                if (Object.ReferenceEquals(value[i], null))
                {
                    throw Error.Argument(Strings.Util_NullArrayElement, parameterName);
                }
            }
        }

        /// <summary>
        /// Validate MergeOption
        /// </summary>
        /// <param name="value">option to validate</param>
        /// <param name="parameterName">name of the parameter being validated</param>
        /// <exception cref="System.ArgumentOutOfRangeException">if option is not valid</exception>
        /// <returns>option</returns>
        internal static MergeOption CheckEnumerationValue(MergeOption value, string parameterName)
        {
            switch (value)
            {
                case MergeOption.AppendOnly:
                case MergeOption.OverwriteChanges:
                case MergeOption.PreserveChanges:
                case MergeOption.NoTracking:
                    return value;
                default:
                    throw Error.ArgumentOutOfRange(parameterName);
            }
        }

#if ASTORIA_LIGHT // Multiple HTTP stacks in Silverlight
        /// <summary>
        /// Validate HttpStack
        /// </summary>
        /// <param name="value">option to validate</param>
        /// <param name="parameterName">name of the parameter being validated</param>
        /// <exception cref="System.ArgumentOutOfRangeException">if option is not valid</exception>
        /// <returns>option</returns>
        internal static HttpStack CheckEnumerationValue(HttpStack value, string parameterName)
        {
            switch (value)
            {
                case HttpStack.Auto:
                case HttpStack.ClientHttp:
                case HttpStack.XmlHttp:
                    return value;
                default:
                    throw Error.ArgumentOutOfRange(parameterName);
            }
        }
#endif

        /// <summary>
        /// get char[] for indenting whitespace when tracing xml
        /// </summary>
        /// <param name="depth">how many characters to trace</param>
        /// <returns>char[]</returns>
        internal static char[] GetWhitespaceForTracing(int depth)
        {
            char[] whitespace = Util.whitespaceForTracing;
            while (whitespace.Length <= depth)
            {
                char[] tmp = new char[2 * whitespace.Length];
                tmp[0] = '\r';
                tmp[1] = '\n';
                for (int i = 2; i < tmp.Length; ++i)
                {
                    tmp[i] = ' ';
                }

                System.Threading.Interlocked.CompareExchange(ref Util.whitespaceForTracing, tmp, whitespace);
                whitespace = tmp;
            }

            return whitespace;
        }

        /// <summary>new Uri(string uriString, UriKind uriKind)</summary>
        /// <param name="value">value</param>
        /// <param name="kind">kind</param>
        /// <returns>new Uri(value, kind)</returns>
        internal static Uri CreateUri(string value, UriKind kind)
        {
            return value == null ? null : new Uri(value, kind);
        }

        /// <summary>new Uri(Uri baseUri, Uri requestUri)</summary>
        /// <param name="baseUri">baseUri</param>
        /// <param name="requestUri">relativeUri</param>
        /// <returns>new Uri(baseUri, requestUri)</returns>
        internal static Uri CreateUri(Uri baseUri, Uri requestUri)
        {
            Debug.Assert((null != baseUri) && baseUri.IsAbsoluteUri, "baseUri !IsAbsoluteUri");
            Debug.Assert(String.IsNullOrEmpty(baseUri.Query) && String.IsNullOrEmpty(baseUri.Fragment), "baseUri has query or fragment");
            Util.CheckArgumentNull(requestUri, "requestUri");

            // there is a bug in (new Uri(Uri,Uri)) which corrupts the port of the result if out relativeUri is also absolute
            if (!requestUri.IsAbsoluteUri)
            {
                if (baseUri.OriginalString.EndsWith("/", StringComparison.Ordinal))
                {
                    if (requestUri.OriginalString.StartsWith("/", StringComparison.Ordinal))
                    {
                        requestUri = new Uri(baseUri, Util.CreateUri(requestUri.OriginalString.TrimStart(Util.ForwardSlash), UriKind.Relative));
                    }
                    else
                    {
                        requestUri = new Uri(baseUri, requestUri);
                    }
                }
                else
                {
                    requestUri = Util.CreateUri(baseUri.OriginalString + "/" + requestUri.OriginalString.TrimStart(Util.ForwardSlash), UriKind.Absolute);
                }
            }

            return requestUri;
        }

        /// <summary>
        /// does the array contain the value reference
        /// </summary>
        /// <typeparam name="T">generic type</typeparam>
        /// <param name="array">array to search</param>
        /// <param name="value">value being looked for</param>
        /// <returns>true if value reference was found in array</returns>
        internal static bool ContainsReference<T>(T[] array, T value) where T : class
        {
            return (0 <= IndexOfReference<T>(array, value));
        }

        /// <summary>dispose of the object and set the reference to null</summary>
        /// <typeparam name="T">type that implements IDisposable</typeparam>
        /// <param name="disposable">object to dispose</param>
        internal static void Dispose<T>(ref T disposable) where T : class, IDisposable
        {
            Dispose(disposable);
            disposable = null;
        }

        /// <summary>dispose of the object</summary>
        /// <typeparam name="T">type that implements IDisposable</typeparam>
        /// <param name="disposable">object to dispose</param>
        internal static void Dispose<T>(T disposable) where T : class, IDisposable
        {
            if (null != disposable)
            {
                disposable.Dispose();
            }
        }

        /// <summary>
        /// index of value reference in the array
        /// </summary>
        /// <typeparam name="T">generic type</typeparam>
        /// <param name="array">array to search</param>
        /// <param name="value">value being looked for</param>
        /// <returns>index of value reference in the array else (-1)</returns>
        internal static int IndexOfReference<T>(T[] array, T value) where T : class
        {
            Debug.Assert(null != array, "null array");
            for (int i = 0; i < array.Length; ++i)
            {
                if (object.ReferenceEquals(array[i], value))
                {
                    return i;
                }
            }

            return -1;
        }

        /// <summary>Checks whether the exception should not be handled.</summary>
        /// <param name="ex">exception to test</param>
        /// <returns>true if the exception should not be handled</returns>
        internal static bool DoNotHandleException(Exception ex)
        {
            return ((null != ex) &&
                    ((ex is System.StackOverflowException) ||
                     (ex is System.OutOfMemoryException) ||
                     (ex is System.Threading.ThreadAbortException)));
        }

        /// <summary>
        /// Checks whether the exception type is one of the DataService*Exception
        /// </summary>
        /// <param name="ex">exception to test</param>
        /// <returns>true if the exception type is one of the DataService*Exception</returns>
        internal static bool IsKnownClientExcption(Exception ex)
        {
            return (ex is DataServiceClientException) || (ex is DataServiceQueryException) || (ex is DataServiceRequestException);
        }

        /// <summary>validate value is non-null</summary>
        /// <typeparam name="T">type of value</typeparam>
        /// <param name="value">value</param>
        /// <param name="errorcode">error code to throw if null</param>
        /// <returns>the non-null value</returns>
        internal static T NullCheck<T>(T value, InternalError errorcode) where T : class
        {
            if (Object.ReferenceEquals(value, null))
            {
                Error.ThrowInternalError(errorcode);
            }

            return value;
        }

        /// <summary>are the two values the same reference</summary>
        /// <param name="value1">value1</param>
        /// <param name="value2">value2</param>
        /// <returns>true if they are the same reference</returns>
        internal static bool AreSame(string value1, string value2)
        {
            // bool result = Object.ReferenceEquals(value1, value2);
            // Debug.Assert(result == (value1 == value2), "!NameTable - unable to do reference comparison on '" + value1 + "'");
            // XElement uses a global name table which may have encountered
            // our strings first and return a different instance than what was expected
            bool result = (value1 == value2);
            return result;
        }

        /// <summary>is the reader on contentNode where the localName and namespaceUri match</summary>
        /// <param name="reader">reader</param>
        /// <param name="localName">localName</param>
        /// <param name="namespaceUri">namespaceUri</param>
        /// <returns>true if localName and namespaceUri match reader current element</returns>
        internal static bool AreSame(XmlReader reader, string localName, string namespaceUri)
        {
            Debug.Assert((null != reader) && (null != localName) && (null != namespaceUri), "null");
            return ((XmlNodeType.Element == reader.NodeType) || (XmlNodeType.EndElement == reader.NodeType)) &&
                    AreSame(reader.LocalName, localName) && AreSame(reader.NamespaceURI, namespaceUri);
        }

        /// <summary>
        /// check the atom:null="true" attribute
        /// </summary>
        /// <param name="reader">XmlReader</param>
        /// <returns>true of null is true</returns>
        internal static bool DoesNullAttributeSayTrue(XmlReader reader)
        {
            string attributeValue = reader.GetAttribute(XmlConstants.AtomNullAttributeName, XmlConstants.DataWebMetadataNamespace);
            return ((null != attributeValue) && XmlConvert.ToBoolean(attributeValue));
        }

        /// <summary>Checks whether the specified <paramref name='type' /> can be assigned null.</summary>
        /// <param name='type'>Type to check.</param>
        /// <returns>true if type is a reference type or a Nullable type; false otherwise.</returns>
        internal static bool TypeAllowsNull(Type type)
        {
            Debug.Assert(type != null, "type != null");
            return !type.IsValueType || IsNullableType(type);
        }

        /// <summary>Gets a type for <paramref name="type"/> that allows null values.</summary>
        /// <param name="type">Type to base resulting type on.</param>
        /// <returns>
        /// <paramref name="type"/> if it's a reference or Nullable&lt;&gt; type;
        /// Nullable&lt;<paramref name="type"/>&gt; otherwise.
        /// </returns>
        internal static Type GetTypeAllowingNull(Type type)
        {
            Debug.Assert(type != null, "type != null");
            return TypeAllowsNull(type) ? type : typeof(Nullable<>).MakeGenericType(type);
        }

        /// <summary>Set the continuation for the following results for a collection.</summary>
        /// <param name="collection">The collection to set the links to</param>
        /// <param name="continuation">The continuation for the collection.</param>
        internal static void SetNextLinkForCollection(object collection, DataServiceQueryContinuation continuation)
        {
            Debug.Assert(collection != null, "collection != null");

            // We do a convention call for setting Continuation. We'll invoke this
            // for all properties named 'Continuation' that is a DataServiceQueryContinuation
            // (assigning to a single one would make it inconsistent if reflection
            // order is changed).
            foreach (var property in collection.GetType().GetProperties(System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public))
            {
                if (property.Name != "Continuation" || !property.CanWrite)
                {
                    continue;
                }

                if (typeof(DataServiceQueryContinuation).IsAssignableFrom(property.PropertyType))
                {
                    property.SetValue(collection, continuation, null);
                }
            }
        }

        /// <summary>
        /// Similar to Activator.CreateInstance, but uses LCG to avoid 
        /// more stringent Reflection security constraints.in Silverlight
        /// </summary>
        /// <param name="type">Type to create.</param>
        /// <param name="arguments">Arguments.</param>
        /// <returns>The newly instantiated object.</returns>
        internal static object ActivatorCreateInstance(Type type, params object[] arguments)
        {
            Debug.Assert(type != null, "type != null");
#if ASTORIA_LIGHT
            // Look up the constructor
            // We don't do overload resolution, make sure that the specific type has only one constructor with specified number of arguments
            int argumentCount = (arguments == null) ? 0 : arguments.Length;
            ConstructorInfo[] constructors = type.GetConstructors();
            ConstructorInfo constructor = null;
            for (int i = 0; i < constructors.Length; i++)
            {
                if (constructors[i].GetParameters().Length == argumentCount)
                {
                    Debug.Assert( constructor == null, "Make sure that the specific type has only one constructor with specified argument count");
                    constructor = constructors[i];
#if !DEBUG
                    break;
#endif
                }
            }

            // A constructor should have been found, but we could run into a case with
            // complex types where there is no default constructor. A better error
            // message can be provided after localization freeze.
            if (constructor == null)
            {
                throw new MissingMethodException();
            }

            return ConstructorInvoke(constructor, arguments);
#else // ASTORIA_LIGHT
            return Activator.CreateInstance(type, arguments);
#endif
        }

        /// <summary>
        /// Similar to ConstructorInfo.Invoke, but uses LCG to avoid 
        /// more stringent Reflection security constraints in Silverlight
        /// </summary>
        /// <param name="constructor">Constructor to invoke.</param>
        /// <param name="arguments">Arguments.</param>
        /// <returns>The newly instantiated object.</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        internal static object ConstructorInvoke(ConstructorInfo constructor, object[] arguments)
        {
            if (constructor == null)
            {
                throw new MissingMethodException();
            }
#if ASTORIA_LIGHT
            int argumentCount = (arguments == null) ? 0 : arguments.Length;
            ParameterExpression argumentsExpression = Expression.Parameter(typeof(object[]), "arguments");
            Expression[] argumentExpressions = new Expression[argumentCount];
            ParameterInfo[] parameters = constructor.GetParameters();
            for (int i = 0; i < argumentExpressions.Length; i++)
            {
                argumentExpressions[i] = Expression.Constant(arguments[i], parameters[i].ParameterType);
            }

            Expression newExpression = Expression.New(constructor, argumentExpressions);
            Expression<Func<object[], object>> lambda = Expression.Lambda<Func<object[], object>>(
                Expression.Convert(newExpression, typeof(object)),
                argumentsExpression);
            object result = lambda.Compile()(arguments);
            return result;
#else
            return constructor.Invoke(arguments);
#endif
        }

        #region Tracing

        /// <summary>
        /// trace Element node
        /// </summary>
        /// <param name="reader">XmlReader</param>
        /// <param name="writer">TextWriter</param>
        [Conditional("TRACE")]
        internal static void TraceElement(XmlReader reader, System.IO.TextWriter writer)
        {
            Debug.Assert(XmlNodeType.Element == reader.NodeType, "not positioned on Element");

            if (null != writer)
            {
                writer.Write(Util.GetWhitespaceForTracing(2 + reader.Depth), 0, 2 + reader.Depth);
                writer.Write("<{0}", reader.Name);

                if (reader.MoveToFirstAttribute())
                {
                    do
                    {
                        writer.Write(" {0}=\"{1}\"", reader.Name, reader.Value);
                    }
                    while (reader.MoveToNextAttribute());

                    reader.MoveToElement();
                }

                writer.Write(reader.IsEmptyElement ? " />" : ">");
            }
        }

        /// <summary>
        /// trace EndElement node
        /// </summary>
        /// <param name="reader">XmlReader</param>
        /// <param name="writer">TextWriter</param>
        /// <param name="indent">indent or not</param>
        [Conditional("TRACE")]
        internal static void TraceEndElement(XmlReader reader, System.IO.TextWriter writer, bool indent)
        {
            if (null != writer)
            {
                if (indent)
                {
                    writer.Write(Util.GetWhitespaceForTracing(2 + reader.Depth), 0, 2 + reader.Depth);
                }

                writer.Write("</{0}>", reader.Name);
            }
        }

        /// <summary>
        /// trace string value
        /// </summary>
        /// <param name="writer">TextWriter</param>
        /// <param name="value">value</param>
        [Conditional("TRACE")]
        internal static void TraceText(System.IO.TextWriter writer, string value)
        {
            if (null != writer)
            {
                writer.Write(value);
            }
        }
        
        #endregion        

        /// <summary>Checks whether the specified type is a generic nullable type.</summary>
        /// <param name="type">Type to check.</param>
        /// <returns>true if <paramref name="type"/> is nullable; false otherwise.</returns>
        private static bool IsNullableType(Type type)
        {
            return type.IsGenericType && type.GetGenericTypeDefinition() == typeof(Nullable<>);
        }
    }
}
