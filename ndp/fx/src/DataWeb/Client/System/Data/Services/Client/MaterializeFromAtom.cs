//---------------------------------------------------------------------
// <copyright file="MaterializeFromAtom.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// materialize objects from an xml stream
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Xml;
    using System.Xml.Linq;
    using System.Text;
    using System.Linq.Expressions;

    #endregion Namespaces.

    /// <summary>
    /// Use this class to materialize objects from an application/atom+xml stream.
    /// </summary>
    internal class MaterializeAtom : IDisposable, IEnumerable, IEnumerator
    {
        /// <summary>MergeOption captured from context so its static for the entire materialization</summary>
        internal readonly MergeOption MergeOptionValue;

        #region Private fields.

        /// <summary>Row count value representing the initial state (-1)</summary>
        private const long CountStateInitial = -1;

        /// <summary>Row count value representing the failure state (-2)</summary>
        private const long CountStateFailure = -2;

        /// <summary>Options when deserializing properties to the target type.</summary>
        private readonly bool ignoreMissingProperties;

        /// <summary>Backreference to the context to allow type resolution.</summary>
        private readonly DataServiceContext context;

        /// <summary>base type of the object to create from the stream.</summary>
        private readonly Type elementType;

        /// <summary>when materializing a known type (like string)</summary>
        /// <remarks>&lt;property&gt; text-value &lt;/property&gt;</remarks>
        private readonly bool expectingSingleValue;

        /// <summary>Materializer from which instances are read.</summary>
        /// <remarks>
        /// The log for the materializer stores all changes for the 
        /// associated data context.
        /// </remarks>
        private readonly AtomMaterializer materializer;

        /// <summary>Parser used by materializer.</summary>
        /// <remarks>Odd thing to have, only here to force a lookahead for inline elements.</remarks>
        private readonly AtomParser parser;

        /// <summary>source reader (may be running on top of a buffer or be the real reader)</summary>
        private XmlReader reader;

        /// <summary>untyped current object</summary>
        private object current;

        /// <summary>has GetEnumerator been called?</summary>
        private bool calledGetEnumerator;

        /// <summary>The count tag's value, if requested</summary>
        private long countValue;

        /// <summary>Whether anything has been read.</summary>
        private bool moved;

        /// <summary>
        /// output writer, set using reflection
        /// </summary>
#if DEBUG && !ASTORIA_LIGHT
        private System.IO.TextWriter writer = new System.IO.StringWriter(System.Globalization.CultureInfo.InvariantCulture);
#else
#pragma warning disable 649
        private System.IO.TextWriter writer;
#pragma warning restore 649
#endif

        #endregion Private fields.

        /// <summary>
        /// constructor
        /// </summary>
        /// <param name="context">originating context</param>
        /// <param name="reader">reader</param>
        /// <param name="queryComponents">Query components (projection, expected type)</param>
        /// <param name="plan">Projection plan (if compiled in an earlier query).</param>
        /// <param name="mergeOption">merge option to use for this materialization pass</param>
        internal MaterializeAtom(DataServiceContext context, XmlReader reader, QueryComponents queryComponents, ProjectionPlan plan, MergeOption mergeOption)
        {
            Debug.Assert(queryComponents != null, "queryComponents != null");

            this.context = context;
            this.elementType = queryComponents.LastSegmentType;
            this.MergeOptionValue = mergeOption;
            this.ignoreMissingProperties = context.IgnoreMissingProperties;
            this.reader = (reader == null) ? null : new System.Data.Services.Client.Xml.XmlAtomErrorReader(reader);
            this.countValue = CountStateInitial;
            this.expectingSingleValue = ClientConvert.IsKnownNullableType(elementType);

            Debug.Assert(reader != null, "Materializer reader is null! Did you mean to use Materializer.ResultsWrapper/EmptyResults?");

            // NOTE: dataNamespace is used for reference equality, and while it looks like
            // a variable, it appears that it will only get set to XmlConstants.DataWebNamespace
            // at runtime. Therefore we remove string dataNamespace as a field here.
            // this.dataNamespace = reader != null ? reader.Settings.NameTable.Add(context.DataNamespace) : null;
            reader.Settings.NameTable.Add(context.DataNamespace);

            string typeScheme = this.context.TypeScheme.OriginalString;
            this.parser = new AtomParser(this.reader, AtomParser.XElementBuilderCallback, typeScheme, context.DataNamespace);
            AtomMaterializerLog log = new AtomMaterializerLog(this.context, mergeOption);
            Type implementationType;
            Type materializerType = GetTypeForMaterializer(this.expectingSingleValue, this.elementType, out implementationType);
            this.materializer = new AtomMaterializer(parser, context, materializerType, this.ignoreMissingProperties, mergeOption, log, this.MaterializedObjectCallback, queryComponents, plan);
        }

        private void MaterializedObjectCallback(object tag, object entity)
        {
            Debug.Assert(tag != null, "tag != null");
            Debug.Assert(entity != null, "entity != null");

            XElement data = (XElement)tag;
            if (this.context.HasReadingEntityHandlers)
            {
                XmlUtil.RemoveDuplicateNamespaceAttributes(data);
                this.context.FireReadingEntityEvent(entity, data);
            }
        }

        /// <summary>
        /// Private internal constructor used for creating empty wrapper.
        /// </summary>
        private MaterializeAtom()
        {
        }

        /// <summary>
        /// DO NOT USE - this is a private hook for client unit tests to construct materializers
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        private MaterializeAtom(DataServiceContext context, XmlReader reader, Type type, MergeOption mergeOption)
            : this(context, reader, new QueryComponents(null, Util.DataServiceVersionEmpty, type, null, null), null, mergeOption)
        {
        }

        #region Current

        /// <summary>Loosely typed current object property.</summary>
        /// <remarks>
        /// The value should be of the right type, as the materializer takes the 
        /// expected type into account.
        /// </remarks>
        public object Current
        {
            get
            {
                object currentValue = this.current;
                return currentValue;
            }
        }

        #endregion

        /// <summary>
        /// A materializer for empty results
        /// </summary>
        internal static MaterializeAtom EmptyResults
        {
            get
            {
                return new ResultsWrapper(null, null);
            }
        }

        /// <summary>
        /// Returns true, if the materialize atom has empty results, because of
        /// IgnoreResourceNotFoundException flag set in context
        /// </summary>
        internal bool IsEmptyResults
        {
            get { return this.reader == null; }
        }

        /// <summary>
        /// The context object this materializer belongs to
        /// </summary>
        internal DataServiceContext Context
        {
            get { return this.context; }
        }

        #region IDisposable
        /// <summary>
        /// dispose
        /// </summary>
        public void Dispose()
        {
            this.current = null;

            if (null != this.reader)
            {
                ((IDisposable)this.reader).Dispose();
            }

            if (null != this.writer)
            {
                this.writer.Dispose();
            }

            GC.SuppressFinalize(this);
        }

        #endregion

        #region IEnumerable
        /// <summary>
        /// as IEnumerable
        /// </summary>
        /// <returns>this</returns>
        public virtual IEnumerator GetEnumerator()
        {
            this.CheckGetEnumerator();
            return this;
        }
        #endregion

        private static Type GetTypeForMaterializer(bool expectingSingleValue, Type elementType, out Type implementationType)
        {
            if (!expectingSingleValue && typeof(IEnumerable).IsAssignableFrom(elementType))
            {
                implementationType = ClientType.GetImplementationType(elementType, typeof(ICollection<>));
                if (implementationType != null)
                {
                    Type expectedType = implementationType.GetGenericArguments()[0]; // already know its IList<>
                    return expectedType;
                }
            }

            implementationType = null;
            return elementType;
        }

        /// <summary>
        /// Creates the next object from the stream.
        /// </summary>
        /// <returns>false if stream is finished</returns>
        public bool MoveNext()
        {
            bool applying = this.context.ApplyingChanges;
            try
            {
                this.context.ApplyingChanges = true;
                return this.MoveNextInternal();
            }
            finally
            {
                this.context.ApplyingChanges = applying;
            }
        }

        /// <summary>
        /// Creates the next object from the stream.
        /// </summary>
        /// <returns>false if stream is finished</returns>
        private bool MoveNextInternal()
        {
            // For empty results, just return false.
            if (this.reader == null)
            {
                Debug.Assert(this.current == null, "this.current == null -- otherwise this.reader should have some value.");
                return false;
            }

            this.current = null;
            this.materializer.Log.Clear();

            bool result = false;
            Type implementationType;
            GetTypeForMaterializer(this.expectingSingleValue, this.elementType, out implementationType);
            if (implementationType != null)
            {
                if (this.moved)
                {
                    return false;
                }

                Type expectedType = implementationType.GetGenericArguments()[0]; // already know its IList<>
                implementationType = this.elementType;
                if (implementationType.IsInterface)
                {
                    implementationType = typeof(System.Collections.ObjectModel.Collection<>).MakeGenericType(expectedType);
                }

                IList list = (IList)Activator.CreateInstance(implementationType);

                while (this.materializer.Read())
                {
                    this.moved = true;
                    list.Add(this.materializer.CurrentValue);
                }

                this.current = list;
                result = true;
            }

            if (null == this.current)
            {
                if (this.expectingSingleValue && this.moved)
                {
                    result = false;
                }
                else
                {
                    result = this.materializer.Read();
                    if (result)
                    {
                        this.current = this.materializer.CurrentValue;
                    }

                    this.moved = true;
                }
            }

            this.materializer.Log.ApplyToContext();

            return result;
        }

        /// <summary>
        /// Not supported.
        /// </summary>
        /// <exception cref="NotSupportedException">Always thrown</exception>
        void System.Collections.IEnumerator.Reset()
        {
            throw Error.NotSupported();
        }

        /// <summary>
        ///  Creates materializer for results
        /// </summary>
        /// <param name="results">the results to wrap</param>
        /// <returns>a new materializer</returns>
        internal static MaterializeAtom CreateWrapper(IEnumerable results)
        {
            return new ResultsWrapper(results, null);
        }

        /// <summary>Creates a materializer for partial result sets.</summary>
        /// <param name="results">The current page of results</param>
        /// <param name="cotinuation">The continuation for the results.</param>
        /// <returns>A new materializer.</returns>
        internal static MaterializeAtom CreateWrapper(IEnumerable results, DataServiceQueryContinuation continuation)
        {
            return new ResultsWrapper(results, continuation);
        }

        /// <summary>set the inserted object expected in the response</summary>
        /// <param name="addedObject">object being inserted that the response is looking for</param>
        internal void SetInsertingObject(object addedObject)
        {
            this.materializer.TargetInstance = addedObject;
        }

        internal static void SkipToEnd(XmlReader reader)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(reader.NodeType == XmlNodeType.Element, "reader.NodeType == XmlNodeType.Element");

            if (reader.IsEmptyElement)
            {
                return;
            }

            int readerDepth = reader.Depth;
            while (reader.Read())
            {
                if (reader.NodeType == XmlNodeType.EndElement && reader.Depth == readerDepth)
                {
                    return;
                }
            }
        }

        /// <summary>
        /// The count tag's value, if requested
        /// </summary>
        /// <returns>The count value returned from the server</returns>
        internal long CountValue()
        {
            if (this.countValue == CountStateInitial)
            {
                this.ReadCountValue();
            }
            else if (this.countValue == CountStateFailure)
            {
                throw new InvalidOperationException(Strings.MaterializeFromAtom_CountNotPresent);
            }

            return this.countValue;
        }

        /// <summary>
        /// Returns the next link URI for the collection key
        /// </summary>
        /// <param name="Key">The collection for which the Uri is returned, or null, if the top level link is to be returned</param>
        /// <returns>An Uri pointing to the next page for the collection</returns>
        internal virtual DataServiceQueryContinuation GetContinuation(IEnumerable key)
        {
            Debug.Assert(this.materializer != null, "Materializer is null!");

            DataServiceQueryContinuation result;
            if (key == null)
            {
                if ((this.expectingSingleValue && !this.moved) || (!this.expectingSingleValue && !this.materializer.IsEndOfStream))
                {
                    // expectingSingleValue && !moved : haven't started parsing single value (single value should not have next link anyway)
                    // !expectingSingleValue && !IsEndOfStream : collection type feed did not finish parsing yet
                    throw new InvalidOperationException(Strings.MaterializeFromAtom_TopLevelLinkNotAvailable);
                }

                // we have already moved to the end of stream
                // are we singleton or just an entry?
                if (this.expectingSingleValue || this.materializer.CurrentFeed == null)
                {
                    result = null;
                }
                else
                {
                    // DEVNOTE(Microsoft): The next link uri should never be edited by the client, and therefore it must be absolute
                    result = DataServiceQueryContinuation.Create(
                        this.materializer.CurrentFeed.NextLink, 
                        this.materializer.MaterializeEntryPlan);
                }
            }
            else
            {
                if (!this.materializer.NextLinkTable.TryGetValue(key, out result))
                {
                    // someone has asked for a collection that's "out of scope" or doesn't exist
                    throw new ArgumentException(Strings.MaterializeFromAtom_CollectionKeyNotPresentInLinkTable);
                }
            }

            return result;
        }
            
        /// <summary>verify the GetEnumerator can only be called once</summary>
        private void CheckGetEnumerator()
        {
            if (this.calledGetEnumerator)
            {
                throw Error.NotSupported(Strings.Deserialize_GetEnumerator);
            }

            this.calledGetEnumerator = true;
        }

        /// <summary>
        /// read the m2:count tag in the feed
        /// </summary>
        private void ReadCountValue()
        {
            Debug.Assert(this.countValue == CountStateInitial, "Count value is not in the initial state");

            if (this.materializer.CurrentFeed != null &&
                this.materializer.CurrentFeed.Count.HasValue)
            {
                this.countValue = this.materializer.CurrentFeed.Count.Value;
                return;
            }

            // find the first element tag
            while (this.reader.NodeType != XmlNodeType.Element && this.reader.Read())
            {
            }

            if (this.reader.EOF)
            {
                throw new InvalidOperationException(Strings.MaterializeFromAtom_CountNotPresent);
            }

            // the tag Should only be <feed> or <links> tag:
            Debug.Assert(
                (Util.AreSame(XmlConstants.AtomNamespace, this.reader.NamespaceURI) &&
                Util.AreSame(XmlConstants.AtomFeedElementName, this.reader.LocalName)) ||
                (Util.AreSame(XmlConstants.DataWebNamespace, this.reader.NamespaceURI) &&
                Util.AreSame(XmlConstants.LinkCollectionElementName, this.reader.LocalName)),
                "<feed> or <links> tag expected");

            // Create the XElement for look-ahead
            // DEVNOTE(Microsoft):
            // This is not streaming friendly!
            XElement element = XElement.Load(this.reader);
            this.reader.Close();

            // Read the count value from the xelement
            XElement countNode = element.Descendants(XNamespace.Get(XmlConstants.DataWebMetadataNamespace) + XmlConstants.RowCountElement).FirstOrDefault();

            if (countNode == null)
            {
                throw new InvalidOperationException(Strings.MaterializeFromAtom_CountNotPresent);
            }
            else
            {
                if (!long.TryParse(countNode.Value, System.Globalization.NumberStyles.Integer, System.Globalization.CultureInfo.InvariantCulture, out this.countValue))
                {
                    throw new FormatException(Strings.MaterializeFromAtom_CountFormatError);
                }

                if (this.countValue < 0)
                {
                    throw new FormatException(Strings.MaterializeFromAtom_CountFormatError);
                }
            }

            this.reader = new System.Data.Services.Client.Xml.XmlAtomErrorReader(element.CreateReader());
            this.parser.ReplaceReader(this.reader);
        }

        /// <summary>
        /// Gets the <see cref="ClientType"/> for the (potentially 
        /// user-callback-resolved) type name.
        /// </summary>
        /// <param name="typeName">String with type name to check.</param>
        /// <param name="context">Context in which type is being resolved.</param>
        /// <param name="expectedType">Expected type for the name.</param>
        /// <param name="checkAssignable">
        /// Whether to check that a user-overriden type is assignable to a 
        /// variable of type <paramref name="expectedType"/>.
        /// </param>
        /// <returns>The <see cref="ClientType"/> for the given type name.</returns>
        internal static ClientType GetEntryClientType(string typeName, DataServiceContext context, Type expectedType, bool checkAssignable)
        {
            Debug.Assert(context != null, "context != null");
            Type resolvedType = context.ResolveTypeFromName(typeName, expectedType, checkAssignable);
            ClientType result = ClientType.Create(resolvedType);
            Debug.Assert(result != null, "result != null -- otherwise ClientType.Create returned null");
            return result;
        }

        internal static string ReadElementString(XmlReader reader, bool checkNullAttribute)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(
                reader.NodeType == XmlNodeType.Element,
                "reader.NodeType == XmlNodeType.Element -- otherwise caller is confused as to where the reader is");

            string result = null;
            bool empty = checkNullAttribute && !Util.DoesNullAttributeSayTrue(reader);

            if (reader.IsEmptyElement)
            {
                return (empty ? String.Empty : null);
            }

            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.EndElement:
                        return result ?? (empty ? String.Empty : null);
                    case XmlNodeType.CDATA:
                    case XmlNodeType.Text:
                    case XmlNodeType.SignificantWhitespace:
                        if (null != result)
                        {
                            throw Error.InvalidOperation(Strings.Deserialize_MixedTextWithComment);
                        }

                        result = reader.Value;
                        break;
                    case XmlNodeType.Comment:
                    case XmlNodeType.Whitespace:
                        break;

                    #region XmlNodeType error
                    case XmlNodeType.Element:
                        goto default;

                    default:
                        throw Error.InvalidOperation(Strings.Deserialize_ExpectingSimpleValue);
                    #endregion
                }
            }

            // xml ended before EndElement?
            throw Error.InvalidOperation(Strings.Deserialize_ExpectingSimpleValue);
        }

        /// <summary>
        /// Private type to wrap partial (paged) results and make it look like standard materialized results.
        /// </summary>
        private class ResultsWrapper : MaterializeAtom
        {
            #region Private fields.

            /// <summary> The results to wrap </summary>
            private readonly IEnumerable results;

            /// <summary>A continuation to the next page of results.</summary>
            private readonly DataServiceQueryContinuation continuation;

            #endregion Private fields.

            /// <summary>
            /// Creates a wrapper for raw results
            /// </summary>
            /// <param name="results">the results to wrap</param>
            /// <param name="continuation">The continuation for this query.</param>
            internal ResultsWrapper(IEnumerable results, DataServiceQueryContinuation continuation)
            {
                this.results = results ?? new object[0];
                this.continuation = continuation;
            }

            /// <summary>
            /// Get the next link to the result set
            /// </summary>
            /// <param name="key">When equals to null, returns the next link associated with this collection. Otherwise throws InvalidOperationException.</param>
            /// <returns>The continuation for this query.</returns>
            internal override DataServiceQueryContinuation GetContinuation(IEnumerable key)
            {
                if (key == null)
                {
                    return this.continuation;
                }
                else
                {
                    throw new InvalidOperationException(Strings.MaterializeFromAtom_GetNestLinkForFlatCollection);
                }
            }

            /// <summary>
            /// Gets Enumerator for wrapped results.
            /// </summary>
            /// <returns>IEnumerator for results</returns>
            public override IEnumerator GetEnumerator()
            {
                return this.results.GetEnumerator();
            }
        }
    }
}
