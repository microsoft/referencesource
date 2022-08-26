//---------------------------------------------------------------------
// <copyright file="WebUtil.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides some utility functions for this project
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data.Linq;
    using System.Data.Services.Internal;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;
    using System.Text;
    using System.Xml;
    using System.Xml.Linq;

    #endregion Namespaces.

    /// <summary>Utility methods for this project.</summary>
    internal static class WebUtil
    {
        /// <summary>Bindings Flags for public instance members.</summary>
        internal const BindingFlags PublicInstanceBindingFlags = BindingFlags.Public | BindingFlags.Instance;

        /// <summary>A zero-length object array.</summary>
        internal static readonly object[] EmptyObjectArray = new object[0];

        /// <summary>A zero-length string array.</summary>
        internal static readonly string[] EmptyStringArray = new string[0];

        /// <summary>MethodInfo for object WebUtil.GetNamedPropertyValue(this object value, string propertyName, IDataServiceQueryProvider provider).</summary>
        internal static readonly MethodInfo GetNamedPropertyValueMethodInfo = typeof(WebUtil).GetMethod(
            "GetNamedPropertyValue",
            BindingFlags.Static | BindingFlags.NonPublic);

        /// <summary>Constant for "null" literal.</summary>
        internal static readonly ConstantExpression NullLiteral = Expression.Constant(null);

        // Only StackOverflowException & ThreadAbortException are sealed classes.

        /// <summary>Type of OutOfMemoryException.</summary>
        private static readonly Type OutOfMemoryType = typeof(System.OutOfMemoryException);

        /// <summary>Type of StackOverflowException.</summary>
        private static readonly Type StackOverflowType = typeof(System.StackOverflowException);

        /// <summary>Type of ThreadAbortException.</summary>
        private static readonly Type ThreadAbortType = typeof(System.Threading.ThreadAbortException);

        /// <summary>List of primitive types supported by Astoria and their corresponding edm names.</summary>
        private static readonly KeyValuePair<Type, string>[] PrimitiveTypesEdmNameMapping =
            new KeyValuePair<Type, string>[]
            {
                new KeyValuePair<Type, string>(typeof(string), XmlConstants.EdmStringTypeName),
                new KeyValuePair<Type, string>(typeof(Boolean), XmlConstants.EdmBooleanTypeName),
                new KeyValuePair<Type, string>(typeof(Boolean?), XmlConstants.EdmBooleanTypeName),
                new KeyValuePair<Type, string>(typeof(Byte), XmlConstants.EdmByteTypeName),
                new KeyValuePair<Type, string>(typeof(Byte?), XmlConstants.EdmByteTypeName),
                new KeyValuePair<Type, string>(typeof(DateTime), XmlConstants.EdmDateTimeTypeName),
                new KeyValuePair<Type, string>(typeof(DateTime?), XmlConstants.EdmDateTimeTypeName),
                new KeyValuePair<Type, string>(typeof(Decimal), XmlConstants.EdmDecimalTypeName),
                new KeyValuePair<Type, string>(typeof(Decimal?), XmlConstants.EdmDecimalTypeName),
                new KeyValuePair<Type, string>(typeof(Double), XmlConstants.EdmDoubleTypeName),
                new KeyValuePair<Type, string>(typeof(Double?), XmlConstants.EdmDoubleTypeName),
                new KeyValuePair<Type, string>(typeof(Guid), XmlConstants.EdmGuidTypeName),
                new KeyValuePair<Type, string>(typeof(Guid?), XmlConstants.EdmGuidTypeName),
                new KeyValuePair<Type, string>(typeof(Int16), XmlConstants.EdmInt16TypeName),
                new KeyValuePair<Type, string>(typeof(Int16?), XmlConstants.EdmInt16TypeName),
                new KeyValuePair<Type, string>(typeof(Int32), XmlConstants.EdmInt32TypeName),
                new KeyValuePair<Type, string>(typeof(Int32?), XmlConstants.EdmInt32TypeName),
                new KeyValuePair<Type, string>(typeof(Int64), XmlConstants.EdmInt64TypeName),
                new KeyValuePair<Type, string>(typeof(Int64?), XmlConstants.EdmInt64TypeName),
                new KeyValuePair<Type, string>(typeof(SByte), XmlConstants.EdmSByteTypeName),
                new KeyValuePair<Type, string>(typeof(SByte?), XmlConstants.EdmSByteTypeName),
                new KeyValuePair<Type, string>(typeof(Single), XmlConstants.EdmSingleTypeName),
                new KeyValuePair<Type, string>(typeof(Single?), XmlConstants.EdmSingleTypeName),
                new KeyValuePair<Type, string>(typeof(byte[]), XmlConstants.EdmBinaryTypeName),

                // Keep the Binary and XElement in the end, since there are not the default mappings for Edm.Binary and Edm.String.
                new KeyValuePair<Type, string>(typeof(XElement), XmlConstants.EdmStringTypeName),
                new KeyValuePair<Type, string>(typeof(Binary), XmlConstants.EdmBinaryTypeName),
            };

        /// <summary>List of primitive types supported by Astoria and their corresponding default MIME types.</summary>
        private static readonly KeyValuePair<Type, string>[] PrimitiveTypesMimeTypeMapping =
            new KeyValuePair<Type, string>[]
            {
                new KeyValuePair<Type, string>(typeof(byte[]), XmlConstants.MimeApplicationOctetStream),
                new KeyValuePair<Type, string>(typeof(Binary), XmlConstants.MimeApplicationOctetStream),
            };

        /// <summary>List of primitive types supported by Astoria and their corresponding default content format.</summary>
        private static readonly KeyValuePair<Type, ContentFormat>[] PrimitiveTypesContentFormatMapping =
            new KeyValuePair<Type, ContentFormat>[]
            {
                new KeyValuePair<Type, ContentFormat>(typeof(byte[]), ContentFormat.Binary),
                new KeyValuePair<Type, ContentFormat>(typeof(Binary), ContentFormat.Binary),
            };

        /// <summary>
        /// Collection of ExpandedWrapper types and their corresponding number of parameters
        /// </summary>
        private static readonly ExpandWrapperTypeWithIndex[] GenericExpandedWrapperTypes = new ExpandWrapperTypeWithIndex[]
            {
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,>), Index = 1 },
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,,>), Index = 2 },
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,,,>), Index = 3 },
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,,,,>), Index = 4 },
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,,,,,>), Index = 5 },
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,,,,,,>), Index = 6 },
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,,,,,,,>), Index = 7 },
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,,,,,,,,>), Index = 8 },
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,,,,,,,,,>), Index = 9 },
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,,,,,,,,,,>), Index = 10 },
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,,,,,,,,,,,>), Index = 11 },
                new ExpandWrapperTypeWithIndex { Type = typeof(ExpandedWrapper<,,,,,,,,,,,,>), Index = 12 }
            };

        /// <summary>List of primitive resource types. We don't want to create them again and again, so creating them once and caching them</summary>
        private static ResourceType[] primitiveResourceTypes;

        /// <summary>
        /// Applies the host specified in a request if available to the given <paramref name="baseUri"/>.
        /// </summary>
        /// <param name="baseUri">URI to update with host (and port) information.</param>
        /// <param name="requestHost">Host header (possibly null or empty)</param>
        /// <returns>The updated URI.</returns>
        internal static Uri ApplyHostHeader(Uri baseUri, string requestHost)
        {
            Debug.Assert(baseUri != null, "baseUri");
            if (!String.IsNullOrEmpty(requestHost))
            {
                UriBuilder builder = new UriBuilder(baseUri);
                string host;
                int port;
                if (GetHostAndPort(requestHost, baseUri.Scheme, out host, out port))
                {
                    builder.Host = host;
                    builder.Port = port;
                }
                else
                {
                    builder.Host = requestHost;
                }

                baseUri = builder.Uri;
            }

            return baseUri;
        }

        /// <summary>
        /// Checks the argument value for null and throw ArgumentNullException if it is null
        /// </summary>
        /// <typeparam name="T">type of the argument</typeparam>
        /// <param name="value">argument whose value needs to be checked</param>
        /// <param name="parameterName">name of the argument</param>
        /// <returns>returns the argument back</returns>
        internal static T CheckArgumentNull<T>(T value, string parameterName) where T : class
        {
            if (null == value)
            {
                throw Error.ArgumentNull(parameterName);
            }

            return value;
        }

        /// <summary>
        /// Checks the string argument value for empty or null and throw ArgumentNullException if it is null
        /// </summary>
        /// <param name="value">argument whose value needs to be checked</param>
        /// <param name="parameterName">name of the argument</param>
        /// <returns>returns the argument back</returns>
        internal static string CheckStringArgumentNull(string value, string parameterName)
        {
            if (string.IsNullOrEmpty(value))
            {
                throw Error.ArgumentNull(parameterName);
            }

            return value;
        }

        /// <summary>
        /// Check whether the given value for ServiceOperationResultKind is valid. If not, throw argument exception.
        /// </summary>
        /// <param name="kind">value for ServiceOperationResultKind</param>
        /// <param name="parameterName">name of the parameter</param>
        /// <exception cref="ArgumentException">if the value is not valid.</exception>
        internal static void CheckServiceOperationResultKind(ServiceOperationResultKind kind, string parameterName)
        {
            if (kind < ServiceOperationResultKind.DirectValue ||
                kind > ServiceOperationResultKind.Void)
            {
                throw new ArgumentException(Strings.InvalidEnumValue(kind.GetType().Name), parameterName);
            }
        }

        /// <summary>
        /// Check whether the given value for ResourceTypeKind is valid. If not, throw argument exception.
        /// </summary>
        /// <param name="kind">value for ResourceTypeKind</param>
        /// <param name="parameterName">name of the parameter</param>
        /// <exception cref="ArgumentException">if the value is not valid.</exception>
        internal static void CheckResourceTypeKind(ResourceTypeKind kind, string parameterName)
        {
            if (kind < ResourceTypeKind.EntityType ||
                kind > ResourceTypeKind.Primitive)
            {
                throw new ArgumentException(Strings.InvalidEnumValue(kind.GetType().Name), parameterName);
            }
        }

        /// <summary>Checks that the <paramref name="rights"/> are valid and throws an exception otherwise.</summary>
        /// <param name="rights">Value to check.</param>
        /// <param name="parameterName">Name of parameter for the exception message.</param>
        internal static void CheckResourceContainerRights(EntitySetRights rights, string parameterName)
        {
            if (rights < 0 || rights > EntitySetRights.All)
            {
                throw Error.ArgumentOutOfRange(parameterName);
            }
        }

        /// <summary>Checks that the <paramref name="rights"/> are valid and throws an exception otherwise.</summary>
        /// <param name="rights">Value to check.</param>
        /// <param name="parameterName">Name of parameter for the exception message.</param>
        internal static void CheckServiceOperationRights(ServiceOperationRights rights, string parameterName)
        {
            if (rights < 0 || rights > (ServiceOperationRights.All | ServiceOperationRights.OverrideEntitySetRights))
            {
                throw Error.ArgumentOutOfRange(parameterName);
            }
        }

        /// <summary>Checks the specifid value for syntax validity.</summary>
        /// <param name="resourceExists">Whether syntax is valid.</param>
        /// <param name="identifier">segment indentifier for which the resource was null.</param>
        /// <remarks>This helper method is used to keep syntax check code more terse.</remarks>
        internal static void CheckResourceExists(bool resourceExists, string identifier)
        {
            if (!resourceExists)
            {
                throw DataServiceException.CreateResourceNotFound(identifier);
            }
        }

        /// <summary>Checks the specific value for syntax validity.</summary>
        /// <param name="valid">Whether syntax is valid.</param>
        /// <remarks>This helper method is used to keep syntax check code more terse.</remarks>
        internal static void CheckSyntaxValid(bool valid)
        {
            if (!valid)
            {
                throw DataServiceException.CreateSyntaxError();
            }
        }

        /// <summary>Creates a new instance if the specified value is null.</summary>
        /// <typeparam name="T">Type of variable.</typeparam>
        /// <param name="value">Current value.</param>
        internal static void CreateIfNull<T>(ref T value) where T : new()
        {
            if (value == null)
            {
                value = new T();
            }
        }

        /// <summary>
        /// Debug.Assert(Enum.IsDefined(typeof(T), value))
        /// </summary>
        /// <typeparam name="T">type of enum</typeparam>
        /// <param name="value">enum value</param>
        [Conditional("DEBUG")]
        internal static void DebugEnumIsDefined<T>(T value)
        {
            Debug.Assert(Enum.IsDefined(typeof(T), value), "enum value is not valid");
        }

        /// <summary>Disposes of <paramref name="o"/> if it implements <see cref="IDisposable"/>.</summary>
        /// <param name="o">Object to dispose, possibly null.</param>
        internal static void Dispose(object o)
        {
            IDisposable disposable = o as IDisposable;
            if (disposable != null)
            {
                disposable.Dispose();
            }
        }

        /// <summary>Adds an empty last segment as necessary to the specified <paramref name="absoluteUri"/>.</summary>
        /// <param name="absoluteUri">An absolute URI.</param>
        /// <returns><paramref name="absoluteUri"/> with an empty last segment (ie, "ending with '/'").</returns>
        internal static Uri EnsureLastSegmentEmpty(Uri absoluteUri)
        {
            Debug.Assert(absoluteUri != null, "absoluteUri != null");
            Debug.Assert(absoluteUri.IsAbsoluteUri, "absoluteUri.IsAbsoluteUri");
            string[] segments = absoluteUri.Segments;
            if (segments.Length > 0)
            {
                string lastBaseSegment = segments[segments.Length - 1];
                if (lastBaseSegment.Length > 0 && lastBaseSegment[lastBaseSegment.Length - 1] != '/')
                {
                    absoluteUri = new Uri(absoluteUri, lastBaseSegment + "/");
                }
            }

            return absoluteUri;
        }

        /// <summary>
        /// Get the expected responseFormat and the content type for the given primitive type
        /// </summary>
        /// <param name="valueType">type of the primitive</param>
        /// <param name="contentType">expected content type for the given primitive type</param>
        /// <returns>the expected response format for the given primitive type </returns>
        internal static ContentFormat GetResponseFormatForPrimitiveValue(ResourceType valueType, out string contentType)
        {
            ContentFormat result = ContentFormat.Text;
            contentType = XmlConstants.MimeTextPlain;

            // For open properties, resourceType can be null, since we don't know the property type yet.
            if (valueType != null)
            {
                foreach (KeyValuePair<Type, string> mapping in PrimitiveTypesMimeTypeMapping)
                {
                    if (valueType.InstanceType == mapping.Key)
                    {
                        contentType = mapping.Value;
                        break;
                    }
                }

                foreach (KeyValuePair<Type, ContentFormat> mapping in PrimitiveTypesContentFormatMapping)
                {
                    if (valueType.InstanceType == mapping.Key)
                    {
                        result = mapping.Value;
                        break;
                    }
                }
            }

            return result;
        }

        /// <summary>Gets the public name for the specified <paramref name='type' />.</summary>
        /// <param name='type'>Type to get name for.</param>
        /// <returns>A public name for the specified <paramref name='type' />, empty if it cannot be found.</returns>
        internal static string GetTypeName(Type type)
        {
            Debug.Assert(type != null, "type != null");
            return type.FullName;
        }

        /// <summary>
        /// Determines whether the specified exception can be caught and 
        /// handled, or whether it should be allowed to continue unwinding.
        /// </summary>
        /// <param name="e"><see cref="Exception"/> to test.</param>
        /// <returns>
        /// true if the specified exception can be caught and handled; 
        /// false otherwise.
        /// </returns>
        internal static bool IsCatchableExceptionType(Exception e)
        {
            // a 'catchable' exception is defined by what it is not.
            Debug.Assert(e != null, "Unexpected null exception!");
            Type type = e.GetType();

            return ((type != StackOverflowType) &&
                     (type != OutOfMemoryType) &&
                     (type != ThreadAbortType));
        }

        /// <summary>Marks the fact that a recursive method was entered, and checks that the depth is allowed.</summary>
        /// <param name="recursionLimit">Maximum recursion limit.</param>
        /// <param name="recursionDepth">Depth of recursion.</param>
        internal static void RecurseEnterQueryParser(int recursionLimit, ref int recursionDepth)
        {
            recursionDepth++;
            Debug.Assert(recursionDepth <= recursionLimit, "recursionDepth <= recursionLimit");
            if (recursionDepth == recursionLimit)
            {
                throw DataServiceException.CreateDeepRecursion_General();
            }
        }

        /// <summary>Marks the fact that a recursive method was entered, and checks that the depth is allowed.</summary>
        /// <param name="recursionLimit">Maximum recursion limit.</param>
        /// <param name="recursionDepth">Depth of recursion.</param>
        internal static void RecurseEnter(int recursionLimit, ref int recursionDepth)
        {
            recursionDepth++;
            Debug.Assert(recursionDepth <= recursionLimit, "recursionDepth <= recursionLimit");
            if (recursionDepth == recursionLimit)
            {
                throw DataServiceException.CreateDeepRecursion(recursionLimit);
            }
        }
        
        /// <summary>Marks the fact that a recursive method is leaving.</summary>
        /// <param name="recursionDepth">Depth of recursion.</param>
        internal static void RecurseLeave(ref int recursionDepth)
        {
            recursionDepth--;
            Debug.Assert(0 <= recursionDepth, "0 <= recursionDepth");
        }

        /// <summary>Selects the request format type from the content type.</summary>
        /// <param name="contentType">content type as specified in the request header</param>
        /// <param name="description">request description for which the content type was specified.</param>
        /// <returns>enum representing the response format for the given content type</returns>
        internal static ContentFormat SelectRequestFormat(string contentType, RequestDescription description)
        {
            ContentFormat contentFormat;

            if (WebUtil.CompareMimeType(contentType, XmlConstants.MimeApplicationJson))
            {
                contentFormat = ContentFormat.Json;
            }
            else if (WebUtil.CompareMimeType(contentType, XmlConstants.MimeTextXml) ||
                     WebUtil.CompareMimeType(contentType, XmlConstants.MimeApplicationXml))
            {
                contentFormat = ContentFormat.PlainXml;
            }
            else if (WebUtil.CompareMimeType(contentType, XmlConstants.MimeApplicationAtom) ||
                     WebUtil.CompareMimeType(contentType, XmlConstants.MimeAny))
            {
                contentFormat = ContentFormat.Atom;
            }
            else
            {
                return ContentFormat.Unsupported;
            }

            if (description.LinkUri)
            {
                if (contentFormat == ContentFormat.Atom)
                {
                    throw new DataServiceException(
                        415,
                        Strings.BadRequest_InvalidContentTypeForRequestUri(contentType, String.Format(CultureInfo.InvariantCulture, "'{0}', '{1}', '{2}'", XmlConstants.MimeApplicationJson, XmlConstants.MimeApplicationXml, XmlConstants.MimeTextXml)));
                }
            }
            else if (description.TargetKind == RequestTargetKind.Resource ||
                     description.LastSegmentInfo.HasKeyValues)
            {
                // If the target is a resource (for open properties, it must have key values or its a collection - nav properties)
                // For reference open properties, this check needs to be done in each deserializer
                if (contentFormat == ContentFormat.PlainXml)
                {
                    throw new DataServiceException(
                        415, 
                        Strings.BadRequest_InvalidContentTypeForRequestUri(contentType, String.Format(CultureInfo.InvariantCulture, "'{0}', '{1}', '{2}'", XmlConstants.MimeApplicationJson, XmlConstants.MimeApplicationAtom, XmlConstants.MimeAny)));
                }
            }
            else if (
                description.TargetKind != RequestTargetKind.OpenProperty &&
                contentFormat == ContentFormat.Atom)
            {
                throw new DataServiceException(
                    415,
                    Strings.BadRequest_InvalidContentTypeForRequestUri(contentType, String.Format(CultureInfo.InvariantCulture, "'{0}', '{1}', '{2}'", XmlConstants.MimeApplicationJson, XmlConstants.MimeApplicationXml, XmlConstants.MimeTextXml)));
            }

            return contentFormat;
        }

        /// <summary>Converts comma-separated entries with no quotes into a text array.</summary>
        /// <param name="text">Text to convert.</param>
        /// <returns>A string array that represents the comma-separated values in the text.</returns>
        /// <remarks>This method can be used to provide a simpler API facade instead of identifier arrays.</remarks>
        internal static string[] StringToSimpleArray(string text)
        {
            if (String.IsNullOrEmpty(text))
            {
                return EmptyStringArray;
            }
            else
            {
                return text.Split(new char[] { ',' }, StringSplitOptions.None);
            }
        }

        /// <summary>
        /// Test if any of the types in the hierarchy of <paramref name="baseType"/> is a Media Link Entry.
        /// </summary>
        /// <param name="baseType">base type of the hierarchy</param>
        /// <param name="provider">IDataServiceMetadataProvider interface instance</param>
        /// <returns>Returns true if <paramref name="baseType"/> or at least one of its descendants is a Media Link Entry.</returns>
        internal static bool HasMediaLinkEntryInHierarchy(ResourceType baseType, DataServiceProviderWrapper provider)
        {
            if (baseType.IsMediaLinkEntry)
            {
                return true;
            }
            else
            {
                foreach (ResourceType derivedType in provider.GetDerivedTypes(baseType))
                {
                    if (derivedType.IsMediaLinkEntry)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        /// <summary>copy from one stream to another</summary>
        /// <param name="input">input stream</param>
        /// <param name="output">output stream</param>
        /// <param name="buffer">reusable buffer</param>
        /// <returns>count of copied bytes</returns>
        internal static long CopyStream(Stream input, Stream output, byte[] buffer)
        {
            Debug.Assert(null != input, "null input stream");
            Debug.Assert(input.CanRead, "input.CanRead");
            Debug.Assert(null != output, "null output stream");
            Debug.Assert(output.CanWrite, "output.CanWrite");
            Debug.Assert(buffer != null, "buffer != null");

            long total = 0;
            int count = 0;
            while (0 < (count = input.Read(buffer, 0, buffer.Length)))
            {
                output.Write(buffer, 0, count);
                total += count;
            }

            return total;
        }

        /// <summary>copy from one stream to another</summary>
        /// <param name="input">input stream</param>
        /// <param name="output">output stream</param>
        /// <param name="bufferSize">size of buffer to use during copying. If 0 is specified, the default of 64K will be used.</param>
        /// <returns>count of copied bytes</returns>
        internal static long CopyStream(Stream input, Stream output, int bufferSize)
        {
            // 64K = 65536 bytes.
            const int DefaultBufferSize = 65536;

            byte[] buffer = new byte[bufferSize <= 0 ? DefaultBufferSize : bufferSize];

            return CopyStream(input, output, buffer);
        }

        /// <summary>
        /// Creates a delegate that when called creates a new instance of the specified <paramref name="type" />.
        /// </summary>
        /// <param name="type">Type of the instance.</param>
        /// <param name="fullName">full name of the given clr type.
        /// If the type name is not specified, it takes the full name from the clr type.</param>
        /// <param name="targetType">Type to return from the delegate.</param>
        /// <returns>A delegate that when called creates a new instance of the specified <paramref name="type" />.</returns>
        internal static Delegate CreateNewInstanceConstructor(Type type, string fullName, Type targetType)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(targetType != null, "targetType != null");
            
            // Create the new instance of the type
            ConstructorInfo emptyConstructor = type.GetConstructor(Type.EmptyTypes);
            if (emptyConstructor == null)
            {
                fullName = fullName ?? type.FullName;
                throw new InvalidOperationException(Strings.NoEmptyConstructorFoundForType(fullName));
            }

            System.Reflection.Emit.DynamicMethod method = new System.Reflection.Emit.DynamicMethod("invoke_constructor", targetType, Type.EmptyTypes, false);
            var generator = method.GetILGenerator();
            generator.Emit(System.Reflection.Emit.OpCodes.Newobj, emptyConstructor);
            if (targetType.IsValueType)
            {
                generator.Emit(System.Reflection.Emit.OpCodes.Box);
            }

            generator.Emit(System.Reflection.Emit.OpCodes.Ret);
            return method.CreateDelegate(typeof(Func<>).MakeGenericType(targetType));
        }

        /// <summary>Checks whether the specified type is a known primitive type.</summary>
        /// <param name="type">Type to check.</param>
        /// <returns>true if the specified type is known to be a primitive type; false otherwise.</returns>
        internal static bool IsPrimitiveType(Type type)
        {
            if (type == null)
            {
                return false;
            }

            foreach (KeyValuePair<Type, string> primitiveTypeInfo in PrimitiveTypesEdmNameMapping)
            {
                if (type == primitiveTypeInfo.Key)
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Try to resolve a type by name by first trying primitive types and then provider's types
        /// </summary>
        /// <param name="provider">Provider to resolve non-primitive types against</param>
        /// <param name="typeName">Type name</param>
        /// <returns>ResourceType object for this type, or null if none found</returns>
        internal static ResourceType TryResolveResourceType(DataServiceProviderWrapper provider, string typeName)
        {
            Debug.Assert(provider != null, "provider != null");
            Debug.Assert(typeName != null, "typeName != null");

            ResourceType resourceType = null;
            ResourceType[] types = WebUtil.GetPrimitiveTypes();

            for (int i = 0; i < types.Length; i++)
            {
                if (types[i].FullName == typeName)
                {
                    resourceType = types[i];
                    break;
                }
            }

            if (resourceType == null)
            {
                resourceType = provider.TryResolveResourceType(typeName);
            }

            return resourceType;
        }

        /// <summary>
        /// Get a primitive or EDM type from an instance
        /// </summary>
        /// <param name="provider">Provider to get EDM types from, in case <paramref name="obj"/> is not a primitive.</param>
        /// <param name="obj">Instance to get the type from</param>
        /// <returns>A ResourceType for this instance or null if it is not a known type</returns>
        internal static ResourceType GetResourceType(DataServiceProviderWrapper provider, object obj)
        {
            Debug.Assert(obj != null, "obj != null");
            Debug.Assert(provider != null, "provider != null");

            ResourceType r = ResourceType.GetPrimitiveResourceType(obj.GetType());
            if (r == null)
            {
                // Note that GetNonPrimitiveResourceType() will throw if we fail to get the resource type.
                r = GetNonPrimitiveResourceType(provider, obj);
            }

            return r;
        }

        /// <summary>
        /// Get the non primitive type resource and checks that the given instance represents a single resource.
        /// </summary>
        /// <param name="provider">underlying data source.</param>
        /// <param name="obj">instance of the resource.</param>
        /// <returns>returns the resource type representing the given resource instance.</returns>
        internal static ResourceType GetNonPrimitiveResourceType(DataServiceProviderWrapper provider, object obj)
        {
            Debug.Assert(obj != null, "obj != null");

            ResourceType resourceType;
            IProjectedResult projectedResult = obj as IProjectedResult;
            if (projectedResult != null)
            {
                resourceType = provider.TryResolveResourceType(projectedResult.ResourceTypeName);
            }
            else
            {
                resourceType = provider.GetResourceType(obj);
            }

            if (resourceType == null)
            {
                throw new DataServiceException(500, Strings.BadProvider_InvalidTypeSpecified(obj.GetType().FullName));
            }

            return resourceType;
        }

        /// <summary> Gets the root type of the given resource type.</summary>
        /// <param name="resourceType">ResourceType to get least derived type for.</param>
        /// <returns>The least derived type for the specified <paramref name="resourceType"/>.</returns>
        internal static ResourceType GetRootType(ResourceType resourceType)
        {
            if (resourceType == null)
            {
                return resourceType;
            }

            while (resourceType.BaseType != null)
            {
                resourceType = resourceType.BaseType;
            }

            return resourceType;
        }

        /// <summary>
        /// Checks whether the specified <paramref name="mimeType"/>
        /// is a valid MIME type with no parameters.
        /// </summary>
        /// <param name="mimeType">Simple MIME type.</param>
        /// <returns>
        /// true if the specified <paramref name="mimeType"/> is valid; 
        /// false otherwise.
        /// </returns>
        /// <remarks>
        /// See http://tools.ietf.org/html/rfc2045#section-5.1 for futher details.
        /// </remarks>
        internal static bool IsValidMimeType(string mimeType)
        {
            Debug.Assert(mimeType != null, "mimeType != null");
            const string Tspecials = "()<>@,;:\\\"/[]?=";
            bool partFound = false;
            bool slashFound = false;
            bool subTypeFound = false;
            foreach (char c in mimeType)
            {
                Debug.Assert(partFound || !slashFound, "partFound || !slashFound -- slashFound->partFound");
                Debug.Assert(slashFound || !subTypeFound, "slashFound || !subTypeFound -- subTypeFound->slashFound");

                if (c == '/')
                {
                    if (!partFound || slashFound)
                    {
                        return false;
                    }

                    slashFound = true;
                }
                else if (c < '\x20' || c > '\x7F' || c == ' ' || Tspecials.IndexOf(c) >= 0)
                {
                    return false;
                }
                else
                {
                    if (slashFound)
                    {
                        subTypeFound = true;
                    }
                    else
                    {
                        partFound = true;
                    }
                }
            }

            return subTypeFound;
        }

        /// <summary>
        /// Checks whether the specified element is an <see cref="IEnumerable"/>
        /// of other elements.
        /// </summary>
        /// <param name="element">Element to check (possibly null).</param>
        /// <param name="enumerable"><paramref name="element"/>, or null if <see cref="IEnumerable"/> is not supported.</param>
        /// <returns>
        /// true if <paramref name="element"/> supports IEnumerable and is not
        /// a primitive type (strings and byte arrays are also enumerables, but
        /// they shouldn't be iterated over, so they return false).
        /// </returns>
        internal static bool IsElementIEnumerable(object element, out IEnumerable enumerable)
        {
            enumerable = element as IEnumerable;
            
            if (enumerable == null)
            {
                return false;
            }

            // Primitive types are atomic, not enumerable, even if they implement IEnumerable.
            Type elementType = element.GetType();
            foreach (ResourceType type in GetPrimitiveTypes())
            {
                if (type.InstanceType == elementType)
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Returns false if the given etag value is not valid.
        /// Look in http://www.ietf.org/rfc/rfc2616.txt?number=2616 (Section 14.26) for more information
        /// </summary>
        /// <param name="etag">etag value to be checked.</param>
        /// <param name="allowStrongEtag">true if we allow strong etag values.</param>
        /// <returns>returns true if the etag value is valid, otherwise returns false.</returns>
        internal static bool IsETagValueValid(string etag, bool allowStrongEtag)
        {
            if (String.IsNullOrEmpty(etag) || etag == XmlConstants.HttpAnyETag)
            {
                return true;
            }

            // HTTP RFC 2616, section 3.11:
            //   entity-tag = [ weak ] opaque-tag
            //   weak       = "W/"
            //   opaque-tag = quoted-string
            int etagValueStartIndex = 1;
            if (etag.StartsWith(XmlConstants.HttpWeakETagPrefix, StringComparison.Ordinal) && etag[etag.Length - 1] == '"')
            {
                etagValueStartIndex = 3;
            }
            else if (!allowStrongEtag || etag[0] != '"' || etag[etag.Length - 1] != '"')
            {
                return false;
            }

            for (int i = etagValueStartIndex; i < etag.Length - 1; i++)
            {
                // Format of etag looks something like: W/"etag property values" or "strong etag value"
                // according to HTTP RFC 2616, if someone wants to specify more than 1 etag value,
                // then need to specify something like this: W/"etag values", W/"etag values", ...
                // To make sure only one etag is specified, we need to ensure that if " is part of the
                // key value, it needs to be escaped.
                if (etag[i] == '"')
                {
                    return false;
                }
            }

            return true;
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

        /// <summary>Checks whether the specified type is a generic nullable type.</summary>
        /// <param name="type">Type to check.</param>
        /// <returns>true if <paramref name="type"/> is nullable; false otherwise.</returns>
        internal static bool IsNullableType(Type type)
        {
            return type.IsGenericType && type.GetGenericTypeDefinition() == typeof(Nullable<>);
        }

        /// <summary>Checks whether <paramref name="expression"/> is a null constant.</summary>
        /// <param name="expression">Expression to check.</param>
        /// <returns>true if <paramref name="expression"/> is a null constant; false otherwise.</returns>
        internal static bool IsNullConstant(Expression expression)
        {
            Debug.Assert(expression != null, "expression != null");
            return
                expression == NullLiteral ||
                (expression.NodeType == ExpressionType.Constant && ((ConstantExpression)expression).Value == null);
        }

        /// <summary>Returns the etag for the given resource.</summary>
        /// <param name="resource">Resource for which etag value needs to be returned.</param>
        /// <param name="resourceType">Resource type of the <paramref name="resource"/>.</param>
        /// <param name="etagProperties">list of etag properties for the given resource</param>
        /// <param name="service">Service to which the request was made.</param>
        /// <param name="getMethod">whether the request was a get method or not.</param>
        /// <returns>ETag value for the given resource, with values encoded for use in a URI.</returns>
        internal static string GetETagValue(object resource, ResourceType resourceType, ICollection<ResourceProperty> etagProperties, IDataService service, bool getMethod)
        {
            Debug.Assert(etagProperties.Count != 0, "etagProperties.Count != 0");

            StringBuilder resultBuilder = new StringBuilder();
            bool firstKey = true;
            resultBuilder.Append(XmlConstants.HttpWeakETagPrefix);
            foreach (ResourceProperty property in etagProperties)
            {
                object keyValue;

                // We need to call IUpdatable.GetValue, if we are still trying to get
                // property value as part of the update changes. If the CUD operation
                // is done (i.e. IUpdatable.SaveChanges) have been called, and if we
                // need to compute the etag, we go via the IDSP.GetPropertyValue.
                // This was the V1 behavior and not changing this now.
                // The getMethod variable name is misleading, since this might be true
                // even for CUD operations, but only after SaveChanges is called.
                if (getMethod)
                {
                    keyValue = WebUtil.GetPropertyValue(service.Provider, resource, resourceType, property, null);
                }
                else
                {
                    keyValue = service.Updatable.GetValue(resource, property.Name);
                }

                if (firstKey)
                {
                    firstKey = false;
                }
                else
                {
                    resultBuilder.Append(',');
                }

                string keyValueText;
                if (keyValue == null)
                {
                    keyValueText = XmlConstants.NullLiteralInETag;
                }
                else if (!System.Data.Services.Parsing.WebConvert.TryKeyPrimitiveToString(keyValue, out keyValueText))
                {
                    throw new InvalidOperationException(Strings.Serializer_CannotConvertValue(keyValue));
                }

                Debug.Assert(keyValueText != null, "keyValueText != null - otherwise TryKeyPrimitiveToString returned true and null value");
                resultBuilder.Append(keyValueText);
            }

            resultBuilder.Append('"');
            return resultBuilder.ToString();
        }

        /// <summary>Returns the etag for the given resource.</summary>
        /// <param name="service">Data service to which the request was made.</param>
        /// <param name="resource">Resource for which etag value needs to be returned.</param>
        /// <param name="container">resource set to which the resource belongs to.</param>
        /// <returns>ETag value for the given resource, with values encoded for use in a URI.</returns>
        internal static string GetETagValue(IDataService service, object resource, ResourceSetWrapper container)
        {
            ResourceType resourceType = WebUtil.GetNonPrimitiveResourceType(service.Provider, resource);
            ICollection<ResourceProperty> etagProperties = service.Provider.GetETagProperties(container.Name, resourceType);
            if (etagProperties.Count != 0)
            {
                return WebUtil.GetETagValue(resource, resourceType, etagProperties, service, true /*getMethod*/);
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Gets the type namespace of the specified <paramref name="type"/>,
        /// appropriate as an externally-visible type name.
        /// </summary>
        /// <param name="type">Type to get namespace for.</param>
        /// <returns>The namespace for <paramref name="type"/>.</returns>
        internal static string GetModelTypeNamespace(Type type)
        {
            Debug.Assert(type != null, "type != null");
            return type.Namespace ?? "";
        }

        /// <summary>Returns an array of primitive types supported.</summary>
        /// <returns>An array of primitive types supported</returns>
        /// <remarks>Most of the time ResourceType.GetPrimitiveResourceType should be used instead of 
        /// searching this array directly, as it takes into account nullable types.</remarks>
        internal static ResourceType[] GetPrimitiveTypes()
        {
            if (primitiveResourceTypes == null)
            {
                ResourceType[] types = new ResourceType[PrimitiveTypesEdmNameMapping.Length];
                for (int i = 0; i < types.Length; i++)
                {
                    string fullName = PrimitiveTypesEdmNameMapping[i].Value;
                    Debug.Assert(fullName.StartsWith(XmlConstants.EdmNamespace, StringComparison.Ordinal), "fullName.StartsWith(XmlConstants.EdmNamespace, StringComparison.Ordinal)");
                    string name = fullName.Substring(XmlConstants.EdmNamespace.Length + 1);
                    types[i] = new ResourceType(PrimitiveTypesEdmNameMapping[i].Key, XmlConstants.EdmNamespace, name);
                }

                System.Threading.Interlocked.CompareExchange<ResourceType[]>(ref primitiveResourceTypes, types, null);
            }

            Debug.Assert(primitiveResourceTypes != null, "primitiveResourceTypes != null");
            return primitiveResourceTypes;
        }

        /// <summary>
        /// Gets an <see cref="IEnumerator"/> for the specified <paramref name="enumerable"/>,
        /// mapping well-known exceptions to the appropriate HTTP status code.
        /// </summary>
        /// <param name="enumerable">Request enumerable to get enumerator for.</param>
        /// <returns>An <see cref="IEnumerator"/> for the specified <paramref name="enumerable"/>.</returns>
        internal static IEnumerator GetRequestEnumerator(IEnumerable enumerable)
        {
            try
            {
                return enumerable.GetEnumerator();
            }
            catch (NotImplementedException e)
            {
                // 501: Not Implemented
                throw new DataServiceException(501, null, Strings.DataService_NotImplementedException, null, e);
            }
            catch (NotSupportedException e)
            {
                // 501: Not Implemented
                throw new DataServiceException(501, null, Strings.DataService_NotImplementedException, null, e);
            }
        }

        /// <summary>
        /// Given the request description, query for the parent entity resource
        /// and compare the etag, if specified in the header
        /// </summary>
        /// <param name="parentEntityResource">entity resource for which etag needs to be checked.</param>
        /// <param name="parentEntityToken">token as returned by the IUpdatable interface methods.</param>
        /// <param name="container">container to which the entity resource belongs to.</param>
        /// <param name="service">Underlying service to which the request was made to.</param>
        /// <param name="writeResponseForGetMethods">out bool which indicates whether response needs to be written for GET operations</param>
        /// <returns>current etag value for the given entity resource.</returns>
        internal static string CompareAndGetETag(
            object parentEntityResource,
            object parentEntityToken,
            ResourceSetWrapper container,
            IDataService service,
            out bool writeResponseForGetMethods)
        {
            Debug.Assert(service.OperationContext.Host != null, "service.OperationContext.Host != null");
            DataServiceHostWrapper host = service.OperationContext.Host;

            // If this method is called for Update, we need to pass the token object as well as the actual instance.
            // The actual instance is used to determine the type that's necessary to find out the etag properties.
            // The token is required to pass back to IUpdatable interface, if we need to get the values for etag properties.
            Debug.Assert(host.AstoriaHttpVerb == AstoriaVerbs.GET, "this method must be called for GET operations only");

            writeResponseForGetMethods = true;
            string etag = null;

            // For .e.g when you are querying for /Customers(1)/BestFriend, the value can be null.
            // Hence in this case, if the If-Match header value is specified, we throw.
            if (parentEntityResource == null)
            {
                if (!String.IsNullOrEmpty(host.RequestIfMatch))
                {
                    throw DataServiceException.CreatePreConditionFailedError(Strings.Serializer_ETagValueDoesNotMatch);
                }
            }
            else
            {
                ResourceType resourceType = WebUtil.GetNonPrimitiveResourceType(service.Provider, parentEntityResource);
                ICollection<ResourceProperty> etagProperties = service.Provider.GetETagProperties(container.Name, resourceType);
                if (etagProperties.Count == 0)
                {
                    // Cannot specify etag for types that don't have etag properties.
                    if (!String.IsNullOrEmpty(host.RequestIfMatch))
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.Serializer_NoETagPropertiesForType);
                    }
                }
                else if (String.IsNullOrEmpty(host.RequestIfMatch) && String.IsNullOrEmpty(host.RequestIfNoneMatch))
                {
                    // no need to check anything if no header is specified.
                }
                else if (host.RequestIfMatch == XmlConstants.HttpAnyETag)
                {
                    // just return - for put, perform the operation and get, return the payload
                }
                else if (host.RequestIfNoneMatch == XmlConstants.HttpAnyETag)
                {
                    // If-None-Match is not allowed for PUT. Hence there is no point checking that
                    // For GET, return Not Modified
                    writeResponseForGetMethods = false;
                }
                else
                {
                    etag = WebUtil.GetETagValue(parentEntityToken, resourceType, etagProperties, service, true /*getMethod*/);
                    if (String.IsNullOrEmpty(host.RequestIfMatch))
                    {
                        Debug.Assert(!String.IsNullOrEmpty(host.RequestIfNoneMatch), "Both can't be null, otherwise it should have entered the first condition");
                        if (host.RequestIfNoneMatch == etag)
                        {
                            writeResponseForGetMethods = false;
                        }
                    }
                    else if (etag != host.RequestIfMatch)
                    {
                        throw DataServiceException.CreatePreConditionFailedError(Strings.Serializer_ETagValueDoesNotMatch);
                    }
                }

                if (etag == null && etagProperties.Count != 0)
                {
                    etag = WebUtil.GetETagValue(parentEntityResource, resourceType, etagProperties, service, true /*getMethod*/);
                }
            }

            return etag;
        }

#if DEBUG
        /// <summary>
        /// Write the etag header value in the response
        /// </summary>
        /// <param name="requestDescription">description about the request made</param>
        /// <param name="etagValue">etag value that needs to be written.</param>
        /// <param name="host">Host implementation for this data service.</param>
        internal static void WriteETagValueInResponseHeader(RequestDescription requestDescription, string etagValue, DataServiceHostWrapper host)
#else
        /// <summary>
        /// Write the etag header value in the response
        /// </summary>
        /// <param name="etagValue">etag value that needs to be written.</param>
        /// <param name="host">Host implementation for this data service.</param>
        internal static void WriteETagValueInResponseHeader(string etagValue, DataServiceHostWrapper host)
#endif
        {
            if (!string.IsNullOrEmpty(etagValue))
            {
#if DEBUG
                // asserting that etag response header is written only in cases when the etag request headers are allowed.
                Debug.Assert(requestDescription == null || RequestDescription.IsETagHeaderAllowed(requestDescription), "etag should not be computed before serialization time if etag response header is not allowed");
                Debug.Assert(requestDescription == null || WebUtil.IsETagValueValid(etagValue, requestDescription.TargetKind == RequestTargetKind.MediaResource), "WebUtil.IsETagValueValid(etagValue)");
                Debug.Assert(string.IsNullOrEmpty(host.ResponseETag), "string.IsNullOrEmpty(host.ResponseETag)");
#endif
                host.ResponseETag = etagValue;
            }
        }

        /// <summary>Writes an xml:space='preserve' attribute if the element value would need it.</summary>
        /// <param name='writer'>XmlWriter to write to.</param>
        /// <param name='elementValue'>Value that will be written after this call.</param>
        internal static void WriteSpacePreserveAttributeIfNecessary(XmlWriter writer, string elementValue)
        {
            if (elementValue.Length > 0 &&
                (Char.IsWhiteSpace(elementValue, 0) ||
                 Char.IsWhiteSpace(elementValue, elementValue.Length - 1)))
            {
                WriteSpacePreserveAttribute(writer);
            }
        }

        /// <summary>
        /// If the specified reader is not on an element, advances to one, skipping document declaration
        /// nodes (typically at the beginning of a document), comments, processing instructions and 
        /// whitespace.
        /// </summary>
        /// <param name="reader">Reader to reposition.</param>
        /// <returns>
        /// true if the reader is left on an element; false otherwise.
        /// </returns>
        internal static bool XmlReaderEnsureElement(XmlReader reader)
        {
            Debug.Assert(reader != null, "reader != null");
            do
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        return true;
                    case XmlNodeType.Comment:
                    case XmlNodeType.None:
                    case XmlNodeType.ProcessingInstruction:
                    case XmlNodeType.XmlDeclaration:
                    case XmlNodeType.Whitespace:
                        break;
                    case XmlNodeType.Text:
                        if (WebUtil.IsWhitespace(reader.Value))
                        {
                            break;
                        }
                        else
                        {
                            return false;
                        }

                    default:
                        return false;
                }
            }
            while (reader.Read());
            
            return false;
        }

        /// <summary>Gets the text for a well-known status code.</summary>
        /// <param name="statusCode">Status code to get text for.</param>
        /// <returns>Text for the status code; an empty string if <paramref name="statusCode"/> is unknown.</returns>
        internal static string GetStatusCodeText(int statusCode)
        {
            #region Non-localized messages for status codes.

            switch (statusCode)
            {
                case 100:
                    return "Continue";
                case 101:
                    return "Switching Protocols";
                case 200:
                    return "OK";
                case 201:
                    return "Created";
                case 202:
                    return "Accepted";
                case 203:
                    return "Non-Authoritative Information";
                case 204:
                    return "No Content";
                case 205:
                    return "Reset Content";
                case 206:
                    return "Partial Content";
                case 300:
                    return "Multiple Choices";
                case 301:
                    return "Moved Permanently";
                case 302:
                    return "Found";
                case 303:
                    return "See Other";
                case 304:
                    return "Not Modified";
                case 305:
                    return "Use Proxy";
                case 307:
                    return "Temporary Redirect";
                case 400:
                    return "Bad Request";
                case 401:
                    return "Unauthorized";
                case 402:
                    return "Payment Required";
                case 403:
                    return "Forbidden";
                case 404:
                    return "Not Found";
                case 405:
                    return "Method Not Allowed";
                case 406:
                    return "Not Acceptable";
                case 407:
                    return "Proxy Authentication Required";
                case 408:
                    return "Request Time-out";
                case 409:
                    return "Conflict";
                case 410:
                    return "Gone";
                case 411:
                    return "Length Required";
                case 412:
                    return "Precondition Failed";
                case 413:
                    return "Request Entity Too Large";
                case 414:
                    return "Request-URI Too Large";
                case 415:
                    return "Unsupported Media Type";
                case 416:
                    return "Requested range not satisfiable";
                case 417:
                    return "Expectation Failed";
                case 500:
                    return "Internal Server Error";
                case 501:
                    return "Not Implemented";
                case 502:
                    return "Bad Gateway";
                case 503:
                    return "Service Unavailable";
                case 504:
                    return "Gateway Time-out";
                case 505:
                    return "HTTP Version not supported";
                default:
                    return "Unknown Status Code";
            }

            #endregion Non-localized messages for status codes.
        }

        /// <summary>
        /// Checks whether a given object implements IServiceProvider and if it supports the specified service interface
        /// </summary>
        /// <typeparam name="T">The type representing the requested service</typeparam>
        /// <param name="target">Object that acts as the service provider</param>
        /// <returns>An object implementing the requested service, or null if not available</returns>
        internal static T GetService<T>(object target) where T : class
        {
            IServiceProvider provider = target as IServiceProvider;
            if (provider != null)
            {
                object service = provider.GetService(typeof(T));
                if (service != null)
                {
                    return (T)service;
                }
            }

            return null;
        }

        /// <summary>
        /// Gets the type corresponding to the wrapper based in input parameter types
        /// </summary>
        /// <param name="wrapperParameters">Parameter types</param>
        /// <param name="errorGenerator">Delegate that generates the error</param>
        /// <returns>Closed generic type</returns>
        internal static Type GetWrapperType(Type[] wrapperParameters, Func<object, String> errorGenerator)
        {
            Debug.Assert(wrapperParameters.Length > 1, "Must have 1 element besides the ProjectedType");
            if (wrapperParameters.Length - 1 > 12)
            {
                throw DataServiceException.CreateBadRequestError(errorGenerator(wrapperParameters.Length - 1));
            }

            return WebUtil.GenericExpandedWrapperTypes.Single(x => x.Index == wrapperParameters.Length - 1).Type.MakeGenericType(wrapperParameters);
        }

        /// <summary>
        /// Checks if the given type is an ExpandedWrapper type
        /// </summary>
        /// <param name="inputType">Input closed type</param>
        /// <returns>true if the given type is one of ExpandedWrapper types</returns>
        internal static bool IsExpandedWrapperType(Type inputType)
        {
            return inputType.IsGenericType && WebUtil.GenericExpandedWrapperTypes.SingleOrDefault(x => x.Type == inputType.GetGenericTypeDefinition()) != null;
        }
        
        /// <summary>
        /// Returns an enumeration that picks one element from each enumerable and projects from them.
        /// </summary>
        /// <typeparam name="T1">Type of first enumerable.</typeparam>
        /// <typeparam name="T2">Type of second enumerable.</typeparam>
        /// <typeparam name="TResult">Type of zipped projection.</typeparam>
        /// <param name="left">Left enumerable.</param>
        /// <param name="right">Right enumerable.</param>
        /// <param name="resultSelector">Projecting function.</param>
        /// <returns>An enumeration with the projected results.</returns>
        internal static IEnumerable<TResult> Zip<T1, T2, TResult>(IEnumerable<T1> left, IEnumerable<T2> right, Func<T1, T2, TResult> resultSelector)
        {
            if (null == left || null == right)
            {
                yield break;
            }

            resultSelector = resultSelector ?? ((x, y) => default(TResult));
            using (var l = left.GetEnumerator())
            using (var r = right.GetEnumerator())
            {
                while (l.MoveNext() && r.MoveNext())
                {
                    yield return resultSelector(l.Current, r.Current);
                }
            }
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
        /// Skips the children of the current node.
        /// </summary>
        /// <param name="xmlReader">XmlReader to skip content from</param>
        /// <param name="localName">local name of the node we search to</param>
        /// <param name="namespaceURI">namespace of the node we search to</param>
        internal static void SkipToEnd(XmlReader xmlReader, string localName, string namespaceURI)
        {
            // <ns:name/>
            // <ns:name></ns:name>
            // <ns:name><amhere><ns:name/></ns:name>
            int readerDepth = xmlReader.Depth;

            do
            {
                switch (xmlReader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (xmlReader.IsEmptyElement &&
                            xmlReader.Depth == readerDepth &&
                            AreSame(xmlReader, localName, namespaceURI))
                        {
                            return;
                        }

                        break;

                    case XmlNodeType.EndElement:
                        if (xmlReader.Depth <= readerDepth)
                        {   // new end element
                            readerDepth--;

                            if (AreSame(xmlReader, localName, namespaceURI))
                            {
                                return;
                            }
                        }

                        break;
                }
            }
            while (xmlReader.Read());
        }

        /// <summary>
        /// get attribute value from specified namespace or empty namespace
        /// </summary>
        /// <param name="reader">reader</param>
        /// <param name="attributeName">attributeName</param>
        /// <param name="namespaceUri">namespaceUri</param>
        /// <returns>attribute value</returns>
        internal static string GetAttributeEx(XmlReader reader, string attributeName, string namespaceUri)
        {
            return reader.GetAttribute(attributeName, namespaceUri) ?? reader.GetAttribute(attributeName);
        }

        /// <summary>
        /// Does a ordinal ignore case comparision of the given mime types.
        /// </summary>
        /// <param name="mimeType1">mime type1.</param>
        /// <param name="mimeType2">mime type2.</param>
        /// <returns>returns true if the mime type are the same.</returns>
        internal static bool CompareMimeType(string mimeType1, string mimeType2)
        {
            return String.Equals(mimeType1, mimeType2, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Disposes the stream provider and returns a no-op method for a stream-writing action.
        /// </summary>
        /// <returns>A delegate that can serialize the result.</returns>
        internal static Action<Stream> GetEmptyStreamWriter()
        {
            return stream => { };
        }

        /// <summary>
        /// Get the value of the given property.
        /// </summary>
        /// <param name="provider">underlying data provider.</param>
        /// <param name="resource">instance of the type which contains this property.</param>
        /// <param name="resourceType">resource type instance containing metadata about the declaring type.</param>
        /// <param name="resourceProperty">resource property instance containing metadata about the property.</param>
        /// <param name="propertyName">Name of the property to read if <paramref name="resourceProperty"/> is not given.</param>
        /// <returns>the value of the given property.</returns>
        internal static object GetPropertyValue(DataServiceProviderWrapper provider, object resource, ResourceType resourceType, ResourceProperty resourceProperty, String propertyName)
        {
            Debug.Assert(provider != null, "provider != null");
            Debug.Assert(resource != null, "resource != null");
            Debug.Assert(
                (resourceProperty == null && propertyName != null) || (resourceProperty != null && propertyName == null), 
                "One of resourceProperty or propertyName should be null and other non-null.");

            IProjectedResult projectedResult = resource as IProjectedResult;
            if (projectedResult != null)
            {
                object result = projectedResult.GetProjectedPropertyValue(propertyName ?? resourceProperty.Name);
                if (result == DBNull.Value)
                {
                    result = null;
                }

                return result;
            }

            if (resourceProperty != null)
            {
                return provider.GetPropertyValue(resource, resourceProperty, resourceType);
            }

            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(propertyName != null, "propertyName != null");
            Debug.Assert(resourceType.IsOpenType, "resourceType must be of open type.");
            return provider.GetOpenPropertyValue(resource, propertyName);
        }

        /// <summary>Checks that the specified <paramref name='service' /> has a known version.</summary>
        /// <param name='service'>Service to check.</param>
        /// <param name="requestDescription">The request description object</param>
        internal static void CheckVersion(IDataService service, RequestDescription requestDescription)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(service.OperationContext != null, "service.OperationContext != null");

            // Check that the request/payload version is understood.
            string versionText = service.OperationContext.Host.RequestVersion;
            if (!String.IsNullOrEmpty(versionText))
            {
                KeyValuePair<Version, string> version;
                if (!HttpProcessUtility.TryReadVersion(versionText, out version))
                {
                    throw DataServiceException.CreateBadRequestError(
                        Strings.DataService_VersionCannotBeParsed(versionText));
                }

                // For both Batch and Non-Batch requests we need to verify that the request DSV
                // is a known version number.
                if (!RequestDescription.IsKnownRequestVersion(version.Key))
                {
                    // Resource strings are already frozen, we can't add new ones...  Hijacking an existing one to make it look close.
                    // The error message will look like:
                    // "Request version '99.99' is not supported for the request payload. The only supported version is '1.0' or '2.0'."
                    // For non-english loc the error will have the english word 'or' which is bad, but this gives more detailed info
                    // than a generic error message.
                    string message = Strings.DataService_VersionNotSupported(version.Key.ToString(2), "1", "0' or '2.0");
                    throw DataServiceException.CreateBadRequestError(message);
                }

                // For non-batch requests we also need to verify we meet the minimum request requirement.
                if (requestDescription != null)
                {
                    if (version.Key < requestDescription.RequireMinimumVersion)
                    {
                        string message = Strings.DataService_VersionNotSupported(
                            version.Key.ToString(2),
                            requestDescription.RequireMinimumVersion.Major,
                            requestDescription.RequireMinimumVersion.Minor);
                        throw DataServiceException.CreateBadRequestError(message);
                    }
                }
            }

            // Check that the maximum version for the client will understand our response.
            versionText = service.OperationContext.Host.RequestMaxVersion;
            if (!String.IsNullOrEmpty(versionText))
            {
                KeyValuePair<Version, string> version;
                if (!HttpProcessUtility.TryReadVersion(versionText, out version))
                {
                    throw DataServiceException.CreateBadRequestError(
                        Strings.DataService_VersionCannotBeParsed(versionText));
                }

                if (requestDescription != null)
                {
                    if (version.Key < requestDescription.ResponseVersion)
                    {
                        string message = Strings.DataService_VersionTooLow(
                            version.Key.ToString(2),
                            requestDescription.ResponseVersion.Major,
                            requestDescription.ResponseVersion.Minor);
                        throw DataServiceException.CreateBadRequestError(message);
                    }
                }
                else
                {
                    // For batch requests we also need to make sure the MDSV is at least 1.0. This was the V1 behavior.
                    if (version.Key < RequestDescription.DataServiceDefaultResponseVersion)
                    {
                        string message = Strings.DataService_VersionTooLow(
                            version.Key.ToString(2),
                            RequestDescription.DataServiceDefaultResponseVersion.Major,
                            RequestDescription.DataServiceDefaultResponseVersion.Minor);
                        throw DataServiceException.CreateBadRequestError(message);
                    }
                }
            }
        }

        /// <summary>
        /// Tests if the mime type text from a request header is application/atom+xml
        /// </summary>
        /// <param name="mimeTypeText">mime type text</param>
        /// <returns>returns true if the mime type text is application/atom+xml</returns>
        internal static bool IsAtomMimeType(string mimeTypeText)
        {
            string mime = HttpProcessUtility.SelectMimeType(mimeTypeText, new string[] { XmlConstants.MimeApplicationAtom });
            return WebUtil.CompareMimeType(mime, XmlConstants.MimeApplicationAtom);
        }

        /// <summary>Gets the host and port parts of a Host header if they are both present.</summary>
        /// <param name="hostHeader">Host header value (non-null).</param>
        /// <param name="scheme">Scheme for the host and port values.</param>
        /// <param name="host">If the result is true, the host part of the header.</param>
        /// <param name="port">If the result is false, the port part of the header.</param>
        /// <returns>true if the header has a host and port part, false otherwise.</returns>
        private static bool GetHostAndPort(string hostHeader, string scheme, out string host, out int port)
        {
            Debug.Assert(hostHeader != null, "hostHeader != null");

            if (scheme != null && !scheme.EndsWith("://", StringComparison.Ordinal))
            {
                scheme += "://";
            }

            Uri result;
            if (Uri.TryCreate(scheme + hostHeader, UriKind.Absolute, out result))
            {
                host = result.Host;
                port = result.Port;
                return true;
            }
            else
            {
                host = null;
                port = default(int);
                return false;
            }
        }

        /// <summary>Checks whether the specifies string is null or blank.</summary>
        /// <param name="text">Text to check.</param>
        /// <returns>true if text is null, empty, or all whitespace characters.</returns>
        private static bool IsWhitespace(string text)
        {
            if (text == null)
            {
                return true;
            }
            else
            {
                foreach (char c in text)
                {
                    if (!Char.IsWhiteSpace(c))
                    {
                        return false;
                    }
                }

                return true;
            }
        }

        /// <summary>Writes an xml:space='preserve' attribute to the specified writer.</summary>
        /// <param name='writer'>XmlWriter to write to.</param>
        private static void WriteSpacePreserveAttribute(XmlWriter writer)
        {
            Debug.Assert(writer != null, "writer != null");
            writer.WriteAttributeString(
                XmlConstants.XmlNamespacePrefix,        // prefix
                XmlConstants.XmlSpaceAttributeName,     // localName
                null,                                   // ns
                XmlConstants.XmlSpacePreserveValue);    // value
        }
        
        /// <summary>
        /// Represents a pair of Expanded wrapper with the index in  the array
        /// The only reason to create this class is to avoid CA908 for KeyValuePairs
        /// </summary>
        private sealed class ExpandWrapperTypeWithIndex
        {
            /// <summary>Type</summary>
            internal Type Type
            {
                get;
                set;
            }
            
            /// <summary>Index</summary>
            internal int Index
            {
                get;
                set;
            }
        }
    }
}
