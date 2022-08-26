//---------------------------------------------------------------------
// <copyright file="RequestQueryParser.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a type to parse expressions in request queries.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Parsing
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;

    #endregion Namespaces.

    /// <summary>
    /// This class provides static methods to parse query options and compose 
    /// them on an existing query.
    /// </summary>
    internal static class RequestQueryParser
    {
        #region Internal methods.

        /// <summary>Sorts a query like a SQL ORDER BY clause does.</summary>
        /// <param name="service">Service with data and configuration.</param>
        /// <param name="source">Original source for query.</param>
        /// <param name="orderingInfo">Ordering definition to compose.</param>
        /// <returns>The composed query.</returns>
        internal static IQueryable OrderBy(IDataService service, IQueryable source, OrderingInfo orderingInfo)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(source != null, "source != null");
            Debug.Assert(orderingInfo != null, "orderingInfo != null");

            Expression queryExpr = source.Expression;
            string methodAsc = "OrderBy";
            string methodDesc = "OrderByDescending";
            foreach (OrderingExpression o in orderingInfo.OrderingExpressions)
            {
                LambdaExpression selectorLambda = (LambdaExpression)o.Expression;
                Type selectorType = selectorLambda.Body.Type;
                service.Provider.CheckIfOrderedType(selectorType);

                queryExpr = Expression.Call(
                    typeof(Queryable),
                    o.IsAscending ? methodAsc : methodDesc,
                    new Type[] { source.ElementType, selectorType },
                    queryExpr,
                    Expression.Quote(selectorLambda));

                methodAsc = "ThenBy";
                methodDesc = "ThenByDescending";
            }

            return source.Provider.CreateQuery(queryExpr);
        }

        /// <summary>Filters a query like a SQL WHERE clause does.</summary>
        /// <param name="service">Service with data and configuration.</param>
        /// <param name="setForIt">Set for each item in the query if they are entities</param>
        /// <param name="typeForIt">Type for each item in the query in Astoria metadata terms</param>
        /// <param name="source">Original source for query.</param>
        /// <param name="predicate">Predicate to compose.</param>
        /// <returns>The composed query.</returns>
        internal static IQueryable Where(IDataService service, ResourceSetWrapper setForIt, ResourceType typeForIt, IQueryable source, string predicate)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(typeForIt != null, "typeForIt != null");
            Debug.Assert(typeForIt.ResourceTypeKind != ResourceTypeKind.EntityType || setForIt != null, "setForIt cannot be null if typeForIt is an entity type.");
            Debug.Assert(source != null, "source != null");
            Debug.Assert(predicate != null, "predicate != null");

            LambdaExpression lambda = ParseLambdaForWhere(service, setForIt, typeForIt, source.ElementType, predicate);
            return source.Provider.CreateQuery(
                Expression.Call(typeof(Queryable), "Where", new Type[] { source.ElementType }, source.Expression, Expression.Quote(lambda)));
        }

        #endregion Internal methods.

        #region Private methods.

        /// <summary>Parses a lambda expression.</summary>
        /// <param name="service">Service with data and configuration.</param>
        /// <param name="setForIt">Resource set for "it" contextual variable.</param>
        /// <param name="typeForIt">Type for "it" contextual variable.</param>
        /// <param name="queryElementType">Actual (clr) element type for the sequence</param>
        /// <param name="expression">Expression to parse.</param>
        /// <returns>The parsed expression.</returns>
        private static LambdaExpression ParseLambdaForWhere(IDataService service, ResourceSetWrapper setForIt, ResourceType typeForIt, Type queryElementType, string expression)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(typeForIt != null, "typeForIt != null");
            Debug.Assert(typeForIt.ResourceTypeKind != ResourceTypeKind.EntityType || setForIt != null, "setForIt cannot be null if typeForIt is an entity type.");
            Debug.Assert(queryElementType != null, "queryElementType != null");
            Debug.Assert(expression != null, "expression != null");
            Debug.Assert(typeForIt.InstanceType == queryElementType, "typeForIt.InstanceType == queryElementType");

            ParameterExpression parameterForIt = Expression.Parameter(queryElementType, "it");
            ExpressionParser parser = new ExpressionParser(service, setForIt, typeForIt, parameterForIt, expression);
            return Expression.Lambda(parser.ParseWhere(), parameterForIt);
        }

        #endregion Private methods.

        /// <summary>Use this class to parse an expression in the Astoria URI format.</summary>
        [DebuggerDisplay("ExpressionParser ({lexer.text})")]
        internal class ExpressionParser
        {
            #region Fields.

            /// <summary>Maximum recursion limit on deserializer.</summary>
            private const int RecursionLimit = 100;

            /// <summary>A type that is not numeric.</summary>
            private const int NumericTypeNotNumeric = 0;

            /// <summary>A type that is a char, single, double or decimal.</summary>
            private const int NumericTypeNotIntegral = 1;

            /// <summary>A type that is a signed integral.</summary>
            private const int NumericTypeSignedIntegral = 2;

            /// <summary>A type that is an unsigned integral.</summary>
            private const int NumericTypeUnsignedIntegral = 3;

            /// <summary>Empty Expressions array.</summary>
            private static readonly Expression[] emptyExpressions = new Expression[0];

            /// <summary>Constant for "true" literal.</summary>
            private static readonly ConstantExpression trueLiteral = Expression.Constant(true);

            /// <summary>Constant for "false" literal.</summary>
            private static readonly ConstantExpression falseLiteral = Expression.Constant(false);

            /// <summary>Dictionary of system functions.</summary>
            private static readonly Dictionary<string, FunctionDescription[]> functions = FunctionDescription.CreateFunctions();

            /// <summary>Method info for string comparison</summary>
            private static readonly MethodInfo StringCompareMethodInfo = typeof(DataServiceProviderMethods)
                                                                  .GetMethods(BindingFlags.Public | BindingFlags.Static)
                                                                  .Single(m => m.Name == "Compare" && m.GetParameters()[0].ParameterType == typeof(string));

            /// <summary>Method info for Bool comparison</summary>
            private static readonly MethodInfo BoolCompareMethodInfo = typeof(DataServiceProviderMethods)
                                                                  .GetMethods(BindingFlags.Public | BindingFlags.Static)
                                                                  .Single(m => m.Name == "Compare" && m.GetParameters()[0].ParameterType == typeof(bool));

            /// <summary>Method info for Bool? comparison</summary>
            private static readonly MethodInfo BoolCompareMethodInfoNullable = typeof(DataServiceProviderMethods)
                                                                  .GetMethods(BindingFlags.Public | BindingFlags.Static)
                                                                  .Single(m => m.Name == "Compare" && m.GetParameters()[0].ParameterType == typeof(bool?));

            /// <summary>Method info for Guid comparison</summary>
            private static readonly MethodInfo GuidCompareMethodInfo = typeof(DataServiceProviderMethods)
                                                                  .GetMethods(BindingFlags.Public | BindingFlags.Static)
                                                                  .Single(m => m.Name == "Compare" && m.GetParameters()[0].ParameterType == typeof(Guid));

            /// <summary>Method info for Guid? comparison</summary>
            private static readonly MethodInfo GuidCompareMethodInfoNullable = typeof(DataServiceProviderMethods)
                                                                  .GetMethods(BindingFlags.Public | BindingFlags.Static)
                                                                  .Single(m => m.Name == "Compare" && m.GetParameters()[0].ParameterType == typeof(Guid?));

            /// <summary>Method info for byte array comparison.</summary>
            private static readonly MethodInfo ByteArrayEqualMethod = typeof(ExpressionParser)
                                                                  .GetMethod("ByteArraysEqual", BindingFlags.NonPublic | BindingFlags.Static);

            /// <summary>Method info for byte array comparison.</summary>
            private static readonly MethodInfo ByteArrayNotEqualMethod = typeof(ExpressionParser)
                                                                  .GetMethod("ByteArraysNotEqual", BindingFlags.NonPublic | BindingFlags.Static);

            /// <summary>Provider of data and metadata.</summary>
            private readonly DataServiceProviderWrapper provider;

            /// <summary>Service with data and configuration.</summary>
            private readonly IDataService service;

            /// <summary>Resource set for "it"</summary>
            private readonly ResourceSetWrapper setForIt;

            /// <summary>Metadata type for "it"</summary>
            private readonly ResourceType typeForIt;

            /// <summary>Literals.</summary>
            private Dictionary<Expression, string> literals;

            /// <summary>"it" contextual parameter.</summary>
            private ParameterExpression it;

            /// <summary>Expression lexer.</summary>
            private ExpressionLexer lexer;

            /// <summary>Whether the expression tree should propagate nulls explicitly.</summary>
            private bool nullPropagationRequired;

            /// <summary>Depth of recursion.</summary>
            private int recursionDepth;

            /// <summary>ResourceType and ResourceSet information for current segment.</summary>
            private SegmentTypeInfo currentSegmentInfo;

            #endregion Fields.

            #region Constructors.

            /// <summary>Initializes a new <see cref="ExpressionParser"/>.</summary>
            /// <param name="service">Service with data and configuration.</param>
            /// <param name="setForIt">Resource set for "it" contextual variable</param>
            /// <param name="typeForIt">Type for "it" contextual variable</param>
            /// <param name="parameterForIt">Parameters for the current "it" context.</param>
            /// <param name="expression">Expression to parse.</param>
            internal ExpressionParser(IDataService service, ResourceSetWrapper setForIt, ResourceType typeForIt, ParameterExpression parameterForIt, string expression)
            {
                Debug.Assert(service != null, "service != null");

                // For Open types, we could potentially have typeForIt parameter set to null value,
                // However, we have decided not support ordering on Open properties which also implies
                // that we can not have entity typed properties on Open types 
                Debug.Assert(typeForIt != null, "typeForIt != null");

                Debug.Assert(expression != null, "expression != null");
                Debug.Assert(parameterForIt != null, "parameterForIt != null");

                this.service = service;
                this.provider = service.Provider;
                this.nullPropagationRequired = this.provider.NullPropagationRequired;
                this.literals = new Dictionary<Expression, string>(ReferenceEqualityComparer<Expression>.Instance);
                this.setForIt = setForIt;
                this.typeForIt = typeForIt;
                this.it = parameterForIt;
                this.lexer = new ExpressionLexer(expression);
            }

            #endregion Constructors.

            /// <summary>Current token being processed.</summary>
            private Token CurrentToken
            {
                get { return this.lexer.CurrentToken; }
                set { this.lexer.CurrentToken = value; }
            }

            /// <summary>Parses the text expression for a .Where method invocation.</summary>
            /// <returns>The parsed expression.</returns>
            internal Expression ParseWhere()
            {
                int exprPos = this.lexer.Position;
                Expression expr = this.ParseExpression();

                expr = PrepareExpressionForWhere(expr);
                if (expr.Type != typeof(bool))
                {
                    throw ParseError(Strings.RequestQueryParser_ExpressionTypeMismatch(WebUtil.GetTypeName(typeof(bool)), exprPos));
                }

                this.lexer.ValidateToken(TokenId.End);
                return expr;
            }

            /// <summary>Makes the expression that is used as a filter corresponding to skip token.</summary>
            /// <param name="topLevelOrderingInfo">Ordering expression.</param>
            /// <param name="k">The provided skip token.</param>
            /// <returns>LambdaExpression corresponding to the skip token filter.</returns>
            internal LambdaExpression BuildSkipTokenFilter(OrderingInfo topLevelOrderingInfo, KeyInstance k)
            {
                ParameterExpression element = Expression.Parameter(this.typeForIt.InstanceType, "element");
                Expression lastCondition = Expression.Constant(true, typeof(bool));
                Expression lastMatch = Expression.Constant(false, typeof(bool));

                foreach (var v in WebUtil.Zip(topLevelOrderingInfo.OrderingExpressions, k.PositionalValues, (x, y) => new { Order = x, Value = y }))
                {
                    Token comparisonExp = v.Order.IsAscending ? Token.GreaterThan : Token.LessThan;

                    Expression fixedLambda = System.Data.Services.Client.ParameterReplacerVisitor.Replace(
                                                    ((LambdaExpression)v.Order.Expression).Body,
                                                    ((LambdaExpression)v.Order.Expression).Parameters[0],
                                                    element);

                    Expression comparison = GenerateLogicalAnd(
                                                lastCondition,
                                                this.GenerateNullAwareComparison(fixedLambda, (String)v.Value, comparisonExp));

                    lastMatch = GenerateLogicalOr(lastMatch, comparison);

                    lastCondition = GenerateLogicalAnd(
                                        lastCondition,
                                        this.GenerateComparison(fixedLambda, (String)v.Value, Token.EqualsTo));
                }

                lastMatch = PrepareExpressionForWhere(lastMatch);

                Debug.Assert(lastMatch.Type == typeof(bool), "Skip token should generate boolean expression.");

                return Expression.Lambda(lastMatch, element);
            }

            /// <summary>Parses the text expression for ordering.</summary>
            /// <returns>An enumeration of orderings.</returns>
            internal IEnumerable<OrderingExpression> ParseOrdering()
            {
                List<OrderingExpression> orderings = new List<OrderingExpression>();
                while (true)
                {
                    Expression expr = this.ParseExpression();
                    bool ascending = true;
                    if (this.TokenIdentifierIs(ExpressionConstants.KeywordAscending))
                    {
                        this.lexer.NextToken();
                    }
                    else if (this.TokenIdentifierIs(ExpressionConstants.KeywordDescending))
                    {
                        this.lexer.NextToken();
                        ascending = false;
                    }

                    orderings.Add(new OrderingExpression(expr, ascending));
                    if (this.CurrentToken.Id != TokenId.Comma)
                    {
                        break;
                    }

                    this.lexer.NextToken();
                }

                this.ValidateToken(TokenId.End);
                return orderings;
            }

            /// <summary>Parse one of the literals of skip token.</summary>
            /// <param name="literal">Input literal.</param>
            /// <returns>Object resulting from conversion of literal.</returns>
            internal object ParseSkipTokenLiteral(String literal)
            {
                ExpressionLexer l = new ExpressionLexer(literal);
                Expression e = this.ParsePrimaryStart(l);
                if (e.NodeType != ExpressionType.Constant)
                {
                    throw new InvalidOperationException(Strings.RequsetQueryParser_ExpectingLiteralInSkipToken(literal));
                }

                return ((ConstantExpression)e).Value;
            }

            /// <summary>Prepare the given expression for passing as a filter to Queryable.Where.</summary>
            /// <param name="expr">Input expression.</param>
            /// <returns>Expression converted to boolean expression.</returns>
            private static Expression PrepareExpressionForWhere(Expression expr)
            {
                if (IsOpenPropertyExpression(expr))
                {
                    expr = OpenTypeMethods.EqualExpression(expr, Expression.Constant(true, typeof(object)));
                    expr = Expression.Convert(expr, typeof(bool));
                }
                else if (WebUtil.IsNullConstant(expr))
                {
                    expr = falseLiteral;
                }
                else if (expr.Type == typeof(bool?))
                {
                    Expression test = Expression.Equal(expr, Expression.Constant(null, typeof(bool?)));
                    expr = Expression.Condition(test, falseLiteral, Expression.Property(expr, "Value"));
                }

                return expr;
            }

            /// <summary>Compares two byte arrays for equality.</summary>
            /// <param name="b0">First byte array.</param><param name="b1">Second byte array.</param>
            /// <returns>true if the arrays are equal; false otherwise.</returns>
            private static bool ByteArraysEqual(byte[] b0, byte[] b1)
            {
                if (b0 == b1)
                {
                    return true;
                }

                if (b0 == null || b1 == null)
                {
                    return false;
                }

                if (b0.Length != b1.Length)
                {
                    return false;
                }

                for (int i = 0; i < b0.Length; i++)
                {
                    if (b0[i] != b1[i])
                    {
                        return false;
                    }
                }

                return true;
            }

            /// <summary>Compares two byte arrays for equality.</summary>
            /// <param name="b0">First byte array.</param><param name="b1">Second byte array.</param>
            /// <returns>true if the arrays are not equal; false otherwise.</returns>
            private static bool ByteArraysNotEqual(byte[] b0, byte[] b1)
            {
                return !ByteArraysEqual(b0, b1);
            }

            /// <summary>Gets a non-nullable version of the specified type.</summary>
            /// <param name="type">Type to get non-nullable version for.</param>
            /// <returns>
            /// <paramref name="type"/> if type is a reference type or a 
            /// non-nullable type; otherwise, the underlying value type.
            /// </returns>
            private static Type GetNonNullableType(Type type)
            {
                return Nullable.GetUnderlyingType(type) ?? type;
            }

            /// <summary>Checks whether the specified type is a signed integral type.</summary>
            /// <param name="type">Type to check.</param>
            /// <returns>true if <paramref name="type"/> is a signed integral type; false otherwise.</returns>
            private static bool IsSignedIntegralType(Type type)
            {
                return GetNumericTypeKind(type) == NumericTypeSignedIntegral;
            }

            /// <summary>Checks whether the specified type is an unsigned integral type.</summary>
            /// <param name="type">Type to check.</param>
            /// <returns>true if <paramref name="type"/> is an unsigned integral type; false otherwise.</returns>
            private static bool IsUnsignedIntegralType(Type type)
            {
                return GetNumericTypeKind(type) == NumericTypeUnsignedIntegral;
            }

            /// <summary>Gets a flag for the numeric kind of type.</summary>
            /// <param name="type">Type to get numeric kind for.</param>
            /// <returns>
            /// One of NumericTypeNotNumeric; NumericTypeNotIntegral if it's char,
            /// single, double or decimal; NumericTypeSignedIntegral, or NumericTypeUnsignedIntegral.
            /// </returns>
            private static int GetNumericTypeKind(Type type)
            {
                type = GetNonNullableType(type);
                Debug.Assert(!type.IsEnum, "!type.IsEnum");
                switch (Type.GetTypeCode(type))
                {
                    case TypeCode.Char:
                    case TypeCode.Single:
                    case TypeCode.Double:
                    case TypeCode.Decimal:
                        return NumericTypeNotIntegral;
                    case TypeCode.SByte:
                    case TypeCode.Int16:
                    case TypeCode.Int32:
                    case TypeCode.Int64:
                        return NumericTypeSignedIntegral;
                    case TypeCode.Byte:
                        return NumericTypeUnsignedIntegral;
                    default:
                        return NumericTypeNotNumeric;
                }
            }

            /// <summary>Checks whether type is a (possibly nullable) enumeration type.</summary>
            /// <param name="type">Type to check.</param>
            /// <returns>true if type is an enumeration or a nullable enumeration; false otherwise.</returns>
            private static bool IsEnumType(Type type)
            {
                return GetNonNullableType(type).IsEnum;
            }

            /// <summary>Returns an object that can enumerate the specified type and its supertypes.</summary>
            /// <param name="type">Type to based enumeration on.</param>
            /// <returns>An object that can enumerate the specified type and its supertypes.</returns>
            private static IEnumerable<Type> SelfAndBaseTypes(Type type)
            {
                if (type.IsInterface)
                {
                    List<Type> types = new List<Type>();
                    AddInterface(types, type);
                    return types;
                }

                return SelfAndBaseClasses(type);
            }

            /// <summary>Returns an object that can enumerate the specified type and its supertypes.</summary>
            /// <param name="type">Type to based enumeration on.</param>
            /// <returns>An object that can enumerate the specified type and its supertypes.</returns>
            private static IEnumerable<Type> SelfAndBaseClasses(Type type)
            {
                while (type != null)
                {
                    yield return type;
                    type = type.BaseType;
                }
            }

            /// <summary>Adds an interface type to a list of types, including inherited interfaces.</summary>
            /// <param name="types">Types list ot add to.</param>
            /// <param name="type">Interface type to add.</param>
            private static void AddInterface(List<Type> types, Type type)
            {
                if (!types.Contains(type))
                {
                    types.Add(type);
                    foreach (Type t in type.GetInterfaces())
                    {
                        AddInterface(types, t);
                    }
                }
            }

            /// <summary>Finds the best applicable methods from the specified array that match the arguments.</summary>
            /// <param name="applicable">Candidate methods.</param>
            /// <param name="args">Argument expressions.</param>
            /// <returns>Best applicable methods.</returns>
            private static MethodData[] FindBestApplicableMethods(MethodData[] applicable, Expression[] args)
            {
                Debug.Assert(applicable != null, "applicable != null");

                List<MethodData> result = new List<MethodData>();
                foreach (MethodData method in applicable)
                {
                    bool betterThanAllOthers = true;
                    foreach (MethodData otherMethod in applicable)
                    {
                        if (otherMethod != method && IsBetterThan(args, otherMethod, method))
                        {
                            betterThanAllOthers = false;
                            break;
                        }
                    }

                    if (betterThanAllOthers)
                    {
                        result.Add(method);
                    }
                }

                return result.ToArray();
            }

            /// <summary>Parses the specified text into a number.</summary>
            /// <param name="text">Text to parse.</param>
            /// <param name="type">Type to parse into.</param>
            /// <returns>The parsed number.</returns>
            private static object ParseNumber(string text, Type type)
            {
                TypeCode tc = Type.GetTypeCode(GetNonNullableType(type));
                switch (tc)
                {
                    case TypeCode.SByte:
                        sbyte sb;
                        if (sbyte.TryParse(text, out sb))
                        {
                            return sb;
                        }

                        break;
                    case TypeCode.Byte:
                        byte b;
                        if (byte.TryParse(text, out b))
                        {
                            return b;
                        }

                        break;
                    case TypeCode.Int16:
                        short s;
                        if (short.TryParse(text, out s))
                        {
                            return s;
                        }

                        break;
                    case TypeCode.Int32:
                        int i;
                        if (int.TryParse(text, out i))
                        {
                            return i;
                        }

                        break;
                    case TypeCode.Int64:
                        long l;
                        if (long.TryParse(text, out l))
                        {
                            return l;
                        }

                        break;
                    case TypeCode.Single:
                        float f;
                        if (float.TryParse(text, out f))
                        {
                            return f;
                        }

                        break;
                    case TypeCode.Double:
                        double d;
                        if (double.TryParse(text, out d))
                        {
                            return d;
                        }

                        break;
                    case TypeCode.Decimal:
                        decimal e;
                        if (decimal.TryParse(text, out e))
                        {
                            return e;
                        }

                        break;
                }

                return null;
            }

            /// <summary>Checks whether the source type is compatible with the value type.</summary>
            /// <param name="source">Source type.</param>
            /// <param name="target">Target type.</param>
            /// <returns>true if source can be used in place of target; false otherwise.</returns>
            private static bool IsCompatibleWith(Type source, Type target)
            {
                if (source == target)
                {
                    return true;
                }

                if (!target.IsValueType)
                {
                    return target.IsAssignableFrom(source);
                }

                Type sourceType = GetNonNullableType(source);
                Type targetType = GetNonNullableType(target);

                //// This rule stops the parser from considering nullable types as incompatible
                //// with non-nullable types. We have however implemented this rule because we just
                //// access underlying rules. C# requires an explicit .Value access, and EDM has no
                //// nullablity on types and (at the model level) implements null propagation.
                ////
                //// if (sourceType != source && targetType == target)
                //// {
                ////     return false;
                //// }

                TypeCode sourceCode = sourceType.IsEnum ? TypeCode.Object : Type.GetTypeCode(sourceType);
                TypeCode targetCode = targetType.IsEnum ? TypeCode.Object : Type.GetTypeCode(targetType);
                switch (sourceCode)
                {
                    case TypeCode.SByte:
                        switch (targetCode)
                        {
                            case TypeCode.SByte:
                            case TypeCode.Int16:
                            case TypeCode.Int32:
                            case TypeCode.Int64:
                            case TypeCode.Single:
                            case TypeCode.Double:
                            case TypeCode.Decimal:
                                return true;
                        }

                        break;
                    case TypeCode.Byte:
                        switch (targetCode)
                        {
                            case TypeCode.Byte:
                            case TypeCode.Int16:
                            case TypeCode.Int32:
                            case TypeCode.Int64:
                            case TypeCode.Single:
                            case TypeCode.Double:
                            case TypeCode.Decimal:
                                return true;
                        }

                        break;
                    case TypeCode.Int16:
                        switch (targetCode)
                        {
                            case TypeCode.Int16:
                            case TypeCode.Int32:
                            case TypeCode.Int64:
                            case TypeCode.Single:
                            case TypeCode.Double:
                            case TypeCode.Decimal:
                                return true;
                        }

                        break;
                    case TypeCode.Int32:
                        switch (targetCode)
                        {
                            case TypeCode.Int32:
                            case TypeCode.Int64:
                            case TypeCode.Single:
                            case TypeCode.Double:
                            case TypeCode.Decimal:
                                return true;
                        }

                        break;
                    case TypeCode.Int64:
                        switch (targetCode)
                        {
                            case TypeCode.Int64:
                            case TypeCode.Single:
                            case TypeCode.Double:
                            case TypeCode.Decimal:
                                return true;
                        }

                        break;
                    case TypeCode.Single:
                        switch (targetCode)
                        {
                            case TypeCode.Single:
                            case TypeCode.Double:
                                return true;
                        }

                        break;
                    default:
                        if (sourceType == targetType)
                        {
                            return true;
                        }

                        break;
                }

                // Anything can be converted to something that's *exactly* an object.
                if (target == typeof(object))
                {
                    return true;
                }

                return false;
            }

            /// <summary>
            /// Checks whether one type list is a better fit than other for the 
            /// specified expressions.
            /// </summary>
            /// <param name="args">Expressions for arguments.</param>
            /// <param name="firstCandidate">First type list to check.</param>
            /// <param name="secondCandidate">Second type list to check.</param>
            /// <returns>
            /// true if <paramref name="firstCandidate"/> has better parameter matching than <paramref name="secondCandidate"/>.
            /// </returns>
            private static bool IsBetterThan(Expression[] args, IEnumerable<Type> firstCandidate, IEnumerable<Type> secondCandidate)
            {
                bool better = false;

                using (IEnumerator<Type> first = firstCandidate.GetEnumerator())
                using (IEnumerator<Type> second = secondCandidate.GetEnumerator())
                {
                    for (int i = 0; i < args.Length; i++)
                    {
                        first.MoveNext();
                        second.MoveNext();
                        int c = CompareConversions(args[i].Type, first.Current, second.Current);
                        if (c < 0)
                        {
                            return false;
                        }
                        else if (c > 0)
                        {
                            better = true;
                        }
                    }
                }

                return better;
            }

            /// <summary>
            /// Checks whether one method is a better fit than other for the 
            /// specified expressions.
            /// </summary>
            /// <param name="args">Expressions for arguments.</param>
            /// <param name="m1">First method to check.</param>
            /// <param name="m2">Second method to check.</param>
            /// <returns>
            /// true if <paramref name="m1"/> has better parameter matching than <paramref name="m2"/>.
            /// </returns>
            private static bool IsBetterThan(Expression[] args, MethodData m1, MethodData m2)
            {
                Debug.Assert(args != null, "args != null");
                Debug.Assert(m1 != null, "m1 != null");
                Debug.Assert(m2 != null, "m2 != null");
                return IsBetterThan(args, m1.ParameterTypes, m2.ParameterTypes);
            }

            /// <summary>Checks which conversion is better.</summary>
            /// <param name="source">Source type.</param>
            /// <param name="targetA">First candidate type to convert to.</param>
            /// <param name="targetB">Second candidate type to convert to.</param>
            /// <returns>
            /// Return 1 if s -> t1 is a better conversion than s -> t2
            /// Return -1 if s -> t2 is a better conversion than s -> t1
            /// Return 0 if neither conversion is better
            /// </returns>
            private static int CompareConversions(Type source, Type targetA, Type targetB)
            {
                // If both types are exactly the same, there is no preference.
                if (targetA == targetB)
                {
                    return 0;
                }

                // Look for exact matches.
                if (source == targetA)
                {
                    return 1;
                }
                else if (source == targetB)
                {
                    return -1;
                }

                // If one is compatible and the other is not, choose the compatible type.
                bool compatibleT1AndT2 = IsCompatibleWith(targetA, targetB);
                bool compatibleT2AndT1 = IsCompatibleWith(targetB, targetA);
                if (compatibleT1AndT2 && !compatibleT2AndT1)
                {
                    return 1;
                }
                else if (compatibleT2AndT1 && !compatibleT1AndT2)
                {
                    return -1;
                }

                // Prefer to keep the original nullability.
                bool sourceNullable = WebUtil.IsNullableType(source);
                bool typeNullableA = WebUtil.IsNullableType(targetA);
                bool typeNullableB = WebUtil.IsNullableType(targetB);
                if (sourceNullable == typeNullableA && sourceNullable != typeNullableB)
                {
                    return 1;
                }
                else if (sourceNullable != typeNullableA && sourceNullable == typeNullableB)
                {
                    return -1;
                }

                // Prefer signed to unsigned.
                if (IsSignedIntegralType(targetA) && IsUnsignedIntegralType(targetB))
                {
                    return 1;
                }
                else if (IsSignedIntegralType(targetB) && IsUnsignedIntegralType(targetA))
                {
                    return -1;
                }

                // Prefer non-object to object.
                if (targetA != typeof(object) && targetB == typeof(object))
                {
                    return 1;
                }
                else if (targetB != typeof(object) && targetA == typeof(object))
                {
                    return -1;
                }

                return 0;
            }

            /// <summary>Generates an Equal expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateEqual(Expression left, Expression right)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.EqualExpression(left, right);
                }

                if (left.Type == typeof(byte[]))
                {
                    return Expression.Equal(left, right, false, ExpressionParser.ByteArrayEqualMethod);
                }

                return Expression.Equal(left, right);
            }

            /// <summary>Generates a NotEqual expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateNotEqual(Expression left, Expression right)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.NotEqualExpression(left, right);
                }

                if (left.Type == typeof(byte[]))
                {
                    return Expression.NotEqual(left, right, false, ExpressionParser.ByteArrayNotEqualMethod);
                }

                return Expression.NotEqual(left, right);
            }

            /// <summary>Generates a GreaterThan comparison expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <param name="comparisonMethodInfo">MethodInfo for comparison method used for string, bool, guid types</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateGreaterThan(Expression left, Expression right, MethodInfo comparisonMethodInfo)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.GreaterThanExpression(left, right);
                }

                if (comparisonMethodInfo != null)
                {
                    left = Expression.Call(null, comparisonMethodInfo, left, right);
                    right = Expression.Constant(0, typeof(int));
                }

                return Expression.GreaterThan(left, right);
            }

            /// <summary>Generates a GreaterThanOrEqual comparsion expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <param name="comparisonMethodInfo">MethodInfo for comparison method used for string, bool, guid types</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateGreaterThanEqual(Expression left, Expression right, MethodInfo comparisonMethodInfo)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.GreaterThanOrEqualExpression(left, right);
                }

                if (comparisonMethodInfo != null)
                {
                    left = Expression.Call(null, comparisonMethodInfo, left, right);
                    right = Expression.Constant(0, typeof(int));
                }

                return Expression.GreaterThanOrEqual(left, right);
            }

            /// <summary>Generates a LessThan comparsion expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <param name="comparisonMethodInfo">MethodInfo for comparison method used for string, bool, guid types</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateLessThan(Expression left, Expression right, MethodInfo comparisonMethodInfo)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.LessThanExpression(left, right);
                }

                if (comparisonMethodInfo != null)
                {
                    left = Expression.Call(null, comparisonMethodInfo, left, right);
                    right = Expression.Constant(0, typeof(int));
                }

                return Expression.LessThan(left, right);
            }

            /// <summary>Generates a LessThanOrEqual comparsion expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <param name="comparisonMethodInfo">MethodInfo for comparison method used for string, bool, guid types</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateLessThanEqual(Expression left, Expression right, MethodInfo comparisonMethodInfo)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.LessThanOrEqualExpression(left, right);
                }

                if (comparisonMethodInfo != null)
                {
                    left = Expression.Call(null, comparisonMethodInfo, left, right);
                    right = Expression.Constant(0, typeof(int));
                }

                return Expression.LessThanOrEqual(left, right);
            }

            /// <summary>
            /// Compares 2 strings by ordinal, used to obtain MethodInfo for comparison operator expression parameter
            /// </summary>
            /// <param name="left">Left Parameter</param>
            /// <param name="right">Right Parameter</param>
            /// <returns>0 for equality, -1 for left less than right, 1 for left greater than right</returns>
            /// <remarks>
            /// Do not change the name of this function because LINQ to SQL is sensitive about the 
            /// method name, so is EF probably.
            /// </remarks>
            private static int Compare(String left, String right)
            {
                return Comparer<string>.Default.Compare(left, right);
            }

            /// <summary>
            /// Compares 2 booleans with true greater than false, used to obtain MethodInfo for comparison operator expression parameter
            /// </summary>
            /// <param name="left">Left Parameter</param>
            /// <param name="right">Right Parameter</param>
            /// <returns>0 for equality, -1 for left less than right, 1 for left greater than right</returns>
            /// <remarks>
            /// Do not change the name of this function because LINQ to SQL is sensitive about the 
            /// method name, so is EF probably.
            /// </remarks>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA908:AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies", Justification = "Need implementation")]
            private static int Compare(bool left, bool right)
            {
                return Comparer<bool>.Default.Compare(left, right);
            }

            /// <summary>
            /// Compares 2 nullable booleans with true greater than false, used to obtain MethodInfo for comparison operator expression parameter
            /// </summary>
            /// <param name="left">Left Parameter</param>
            /// <param name="right">Right Parameter</param>
            /// <returns>0 for equality, -1 for left less than right, 1 for left greater than right</returns>
            /// <remarks>
            /// Do not change the name of this function because LINQ to SQL is sensitive about the 
            /// method name, so is EF probably.
            /// </remarks>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA908:AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies", Justification = "Need implementation")]
            private static int Compare(bool? left, bool? right)
            {
                return Comparer<bool?>.Default.Compare(left, right);
            }

            /// <summary>
            /// Compares 2 guids by byte order, used to obtain MethodInfo for comparison operator expression parameter
            /// </summary>
            /// <param name="left">Left Parameter</param>
            /// <param name="right">Right Parameter</param>
            /// <returns>0 for equality, -1 for left less than right, 1 for left greater than right</returns>
            /// <remarks>
            /// Do not change the name of this function because LINQ to SQL is sensitive about the 
            /// method name, so is EF probably.
            /// </remarks>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA908:AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies", Justification = "Need implementation")]
            private static int Compare(Guid left, Guid right)
            {
                return Comparer<Guid>.Default.Compare(left, right);
            }

            /// <summary>
            /// Compares 2 nullable guids by byte order, used to obtain MethodInfo for comparison operator expression parameter
            /// </summary>
            /// <param name="left">Left Parameter</param>
            /// <param name="right">Right Parameter</param>
            /// <returns>0 for equality, -1 for left less than right, 1 for left greater than right</returns>
            /// <remarks>
            /// Do not change the name of this function because LINQ to SQL is sensitive about the 
            /// method name, so is EF probably.
            /// </remarks>
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA908:AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies", Justification = "Need implementation")]
            private static int Compare(Guid? left, Guid? right)
            {
                return Comparer<Guid?>.Default.Compare(left, right);
            }

            /// <summary>Generates an addition expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateAdd(Expression left, Expression right)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.AddExpression(left, right);
                }

                return Expression.Add(left, right);
            }

            /// <summary>Generates a subtract expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateSubtract(Expression left, Expression right)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.SubtractExpression(left, right);
                }

                return Expression.Subtract(left, right);
            }

            /// <summary>Generates a multiplication expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateMultiply(Expression left, Expression right)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.MultiplyExpression(left, right);
                }

                return Expression.Multiply(left, right);
            }

            /// <summary>Generates a negative of expression.</summary>
            /// <param name="expr">Input expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateNegate(Expression expr)
            {
                if (IsOpenPropertyExpression(expr))
                {
                    return OpenTypeMethods.NegateExpression(expr);
                }

                return Expression.Negate(expr);
            }

            /// <summary>Generates a not of expression.</summary>
            /// <param name="expr">Input expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateNot(Expression expr)
            {
                if (IsOpenPropertyExpression(expr))
                {
                    return OpenTypeMethods.NotExpression(expr);
                }

                if (expr.Type == typeof(bool) || expr.Type == typeof(Nullable<bool>))
                {
                    // Expression.Not will take numerics and apply '~' to them, thus the extra check here.
                    return Expression.Not(expr);
                }
                else
                {
                    throw ParseError(Strings.RequestQueryParser_NotDoesNotSupportType(expr.Type));
                }
            }

            /// <summary>Generates a divide expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateDivide(Expression left, Expression right)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.DivideExpression(left, right);
                }

                return Expression.Divide(left, right);
            }

            /// <summary>Generates a modulo expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateModulo(Expression left, Expression right)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.ModuloExpression(left, right);
                }

                return Expression.Modulo(left, right);
            }

            /// <summary>Generates a logical And expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateLogicalAnd(Expression left, Expression right)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.AndAlsoExpression(left, right);
                }

                return Expression.AndAlso(left, right);
            }

            /// <summary>Generates a Logical Or expression.</summary>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateLogicalOr(Expression left, Expression right)
            {
                if (IsOpenPropertyExpression(left) || IsOpenPropertyExpression(right))
                {
                    return OpenTypeMethods.OrElseExpression(left, right);
                }

                return Expression.OrElse(left, right);
            }

            /// <summary>
            /// Checks whether the specified <paramref name="expression"/> is part of an open property expression.
            /// </summary>
            /// <param name="expression">Non-null <see cref="Expression"/> to check.</param>
            /// <returns>true if <paramref name="expression"/> is based on an open property; false otherwise.</returns>
            private static bool IsOpenPropertyExpression(Expression expression)
            {
                Debug.Assert(expression != null, "expression != null");
                return expression != WebUtil.NullLiteral && expression.Type == typeof(object) && IsOpenExpression(expression);
            }

            /// <summary>Checks if the given input expression refers to open types.</summary>
            /// <param name="input">Input expression.</param>
            /// <returns>true if the input is an open expression, false otherwise.</returns>
            private static bool IsOpenExpression(Expression input)
            {
                Debug.Assert(input != null, "input != null");
                input = StripObjectConvert(input);

                switch (input.NodeType)
                {
                    case ExpressionType.Call:
                        return ((MethodCallExpression)input).Method.DeclaringType == typeof(OpenTypeMethods);

                    case ExpressionType.Negate:
                    case ExpressionType.Not:
                        return ((UnaryExpression)input).Method != null && ((UnaryExpression)input).Method.DeclaringType == typeof(OpenTypeMethods);

                    case ExpressionType.Add:
                    case ExpressionType.AndAlso:
                    case ExpressionType.Divide:
                    case ExpressionType.Equal:
                    case ExpressionType.GreaterThan:
                    case ExpressionType.GreaterThanOrEqual:
                    case ExpressionType.LessThan:
                    case ExpressionType.LessThanOrEqual:
                    case ExpressionType.Modulo:
                    case ExpressionType.Multiply:
                    case ExpressionType.NotEqual:
                    case ExpressionType.OrElse:
                    case ExpressionType.Subtract:
                        return ((BinaryExpression)input).Method != null && ((BinaryExpression)input).Method.DeclaringType == typeof(OpenTypeMethods);

                    case ExpressionType.Conditional:
                        // This handles that null propagation scenario.
                        return IsOpenExpression(((ConditionalExpression)input).IfFalse);

                    case ExpressionType.Convert:
                    case ExpressionType.Constant:
                    case ExpressionType.MemberAccess:
                    case ExpressionType.TypeIs:
                        return false;

                    default:
                        Debug.Assert(false, "Unexpected expression type found.");
                        break;
                }

                return false;
            }

            /// <summary>Strips all Expression.Convert(object) calls from the input expression.</summary>
            /// <param name="input">Input expression.</param>
            /// <returns>First non-Convert expression inside Converts that converts to non-object type.</returns>
            private static Expression StripObjectConvert(Expression input)
            {
                Debug.Assert(input != null, "input != null");
                while (input.NodeType == ExpressionType.Convert && input.Type == typeof(object))
                {
                    UnaryExpression inner = (UnaryExpression)input;
                    input = inner.Operand;
                }

                return input;
            }

            /// <summary>Gets a static method by name.</summary>
            /// <param name="methodName">Name of method to get.</param>
            /// <param name="left">Left expression to resolve method from and to use as argument.</param>
            /// <param name="right">Right expression.</param>
            /// <returns>The method.</returns>
            private static MethodInfo GetStaticMethod(string methodName, Expression left, Expression right)
            {
                return left.Type.GetMethod(methodName, new Type[] { left.Type, right.Type });
            }

            /// <summary>Generates a static method call.</summary>
            /// <param name="methodName">Method name.</param>
            /// <param name="left">Left expression.</param>
            /// <param name="right">Right expression.</param>
            /// <returns>The generated expression.</returns>
            private static Expression GenerateStaticMethodCall(string methodName, Expression left, Expression right)
            {
                return Expression.Call(null, GetStaticMethod(methodName, left, right), new Expression[] { left, right });
            }

            /// <summary>Creates an exception for a parse error.</summary>
            /// <param name="message">Message text.</param>
            /// <returns>A new Exception.</returns>
            private static Exception ParseError(string message)
            {
                return DataServiceException.CreateSyntaxError(message);
            }

            /// <summary>Checks that the given token has the specified identifier.</summary>
            /// <param name="token">Token to check</param>
            /// <param name="id">Identifier to check.</param>
            /// <returns>true if <paramref name="token"/> is an identifier with the specified text.</returns>
            private static bool TokenIdentifierIs(Token token, string id)
            {
                return token.Id == TokenId.Identifier && String.Equals(id, token.Text, StringComparison.OrdinalIgnoreCase);
            }

            /// <summary>Creates an exception indicated that two operands are incompatible.</summary>
            /// <param name="operationName">Name of operation for operands.</param>
            /// <param name="left">Expression for left-hand operand.</param>
            /// <param name="right">Expression for right-hand operand.</param>
            /// <param name="pos">Position for error.</param>
            /// <returns>A new <see cref="Exception"/>.</returns>
            private static Exception IncompatibleOperandsError(string operationName, Expression left, Expression right, int pos)
            {
                string message = Strings.RequestQueryParser_IncompatibleOperands(
                    operationName,
                    WebUtil.GetTypeName(left.Type),
                    WebUtil.GetTypeName(right.Type),
                    pos);
                return ParseError(message);
            }

            #region Parsing.

            /// <summary>Handles 'null' literals.</summary>
            /// <param name="l">Lexer to use for reading tokens</param>
            /// <returns>The parsed expression.</returns>
            private static Expression ParseNullLiteral(ExpressionLexer l)
            {
                Debug.Assert(l.CurrentToken.Id == TokenId.NullLiteral, "l.CurrentToken.Id == TokenId.NullLiteral");
                l.NextToken();
                return WebUtil.NullLiteral;
            }

            /// <summary>Handles a ?: operator - not supported.</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParseExpression()
            {
                this.RecurseEnter();
                Expression expr = this.ParseLogicalOr();
                this.RecurseLeave();
                return expr;
            }

            /// <summary>Handles or operator.</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParseLogicalOr()
            {
                this.RecurseEnter();
                Expression left = this.ParseLogicalAnd();
                while (this.TokenIdentifierIs(ExpressionConstants.KeywordOr))
                {
                    Token op = this.CurrentToken;
                    this.lexer.NextToken();
                    Expression right = this.ParseLogicalAnd();
                    this.CheckAndPromoteOperands(typeof(OperationSignatures.ILogicalSignatures), op.Text, ref left, ref right, op.Position);
                    left = GenerateLogicalOr(left, right);
                }

                this.RecurseLeave();
                return left;
            }

            /// <summary>Handles and operator.</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParseLogicalAnd()
            {
                this.RecurseEnter();
                Expression left = this.ParseComparison();
                while (this.TokenIdentifierIs(ExpressionConstants.KeywordAnd))
                {
                    Token op = this.CurrentToken;
                    this.lexer.NextToken();
                    Expression right = this.ParseComparison();
                    this.CheckAndPromoteOperands(typeof(OperationSignatures.ILogicalSignatures), op.Text, ref left, ref right, op.Position);
                    left = GenerateLogicalAnd(left, right);
                }

                this.RecurseLeave();
                return left;
            }

            /// <summary>Handles eq, ne, lt, gt, le, ge operators.</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParseComparison()
            {
                this.RecurseEnter();
                Expression left = this.ParseAdditive();
                while (this.CurrentToken.IsComparisonOperator)
                {
                    Token op = this.CurrentToken;
                    this.lexer.NextToken();
                    Expression right = this.ParseAdditive();
                    left = this.GenerateComparisonExpression(left, right, op);
                }

                this.RecurseLeave();
                return left;
            }

            /// <summary>
            /// Generates a comparison expression given a left hand side expression and literal for right hand side
            /// </summary>
            /// <param name="left">Left hand side experssions</param>
            /// <param name="rightLiteral">Literal for right hand side</param>
            /// <param name="op">gt, eq or lt operator token</param>
            /// <returns>Resulting comparison expression</returns>
            private Expression GenerateComparison(Expression left, String rightLiteral, Token op)
            {
                ExpressionLexer l = new ExpressionLexer(rightLiteral);
                return this.GenerateComparisonExpression(left, this.ParsePrimaryStart(l), op);
            }

            /// <summary>
            /// Generates a comparison expression which can handle NULL values for any type.
            /// NULL is always treated as the smallest possible value.
            /// So for example for strings NULL is smaller than any non-NULL string.
            /// For now only GreaterThan and LessThan operators are supported by this method.
            /// </summary>
            /// <param name="left">Left hand side expression</param>
            /// <param name="rightLiteral">Literal for the right hand side</param>
            /// <param name="op">gt or lt operator token</param>
            /// <returns>Resulting comparison expression (has a Boolean value)</returns>
            private Expression GenerateNullAwareComparison(Expression left, String rightLiteral, Token op)
            {
                Debug.Assert(
                    op.Text == ExpressionConstants.KeywordGreaterThan || op.Text == ExpressionConstants.KeywordLessThan,
                    "Only GreaterThan or LessThan operators are supported by the GenerateNullAwateComparison method for now.");

                ExpressionLexer l = new ExpressionLexer(rightLiteral);
                Expression right = this.ParsePrimaryStart(l);

                if (WebUtil.TypeAllowsNull(left.Type))
                {
                    if (!WebUtil.TypeAllowsNull(right.Type))
                    {
                        right = Expression.Convert(right, typeof(Nullable<>).MakeGenericType(right.Type));
                    }
                }
                else if (WebUtil.TypeAllowsNull(right.Type))
                {
                    left = Expression.Convert(left, typeof(Nullable<>).MakeGenericType(left.Type));
                }
                else
                {
                    // Can't perform NULL aware comparison on this type. Just let the normal 
                    // comparison deal with it. Since the type doesn't allow NULL one should 
                    // never appear, so normal comparison is just fine.
                    return this.GenerateComparisonExpression(left, right, op);
                }

                switch (op.Text)
                {
                    case ExpressionConstants.KeywordGreaterThan:
                        // (left != null) && ((right == null) || Compare(left, right) > 0)
                        if (left == WebUtil.NullLiteral)
                        {
                            return Expression.Constant(false, typeof(bool));
                        }
                        else if (right == WebUtil.NullLiteral)
                        {
                            return GenerateNotEqual(left, Expression.Constant(null, left.Type));
                        }
                        else
                        {
                            return GenerateLogicalAnd(
                                        GenerateNotEqual(left, Expression.Constant(null, left.Type)),
                                        GenerateLogicalOr(
                                            GenerateEqual(right, Expression.Constant(null, right.Type)),
                                            this.GenerateComparisonExpression(left, right, op)));
                        }

                    case ExpressionConstants.KeywordLessThan:
                        // (right != null) && ((left == null) || Compare(left, right) < 0)
                        if (right == WebUtil.NullLiteral)
                        {
                            return Expression.Constant(false, typeof(bool));
                        }
                        else if (left == WebUtil.NullLiteral)
                        {
                            return GenerateNotEqual(right, Expression.Constant(null, right.Type));
                        }
                        else
                        {
                            return GenerateLogicalAnd(
                                        GenerateNotEqual(right, Expression.Constant(null, left.Type)),
                                        GenerateLogicalOr(
                                            GenerateEqual(left, Expression.Constant(null, right.Type)),
                                            this.GenerateComparisonExpression(left, right, op)));
                        }

                    default:
                        // For now only < and > are supported as we use this only from $skiptoken
                        throw ParseError(
                            Strings.RequestQueryParser_NullOperatorUnsupported(op.Text, op.Position, this.lexer.ExpressionText));
                }
            }

            /// <summary>
            /// Given left and right hand side expressions, generates a comparison expression based
            /// on the given comparison token
            /// </summary>
            /// <param name="left">Left hand side expression</param>
            /// <param name="right">Right hand side expression</param>
            /// <param name="op">Comparison operator</param>
            /// <returns>Resulting comparison expression</returns>
            private Expression GenerateComparisonExpression(Expression left, Expression right, Token op)
            {
                bool equality = op.IsEqualityOperator;
                if (equality && !left.Type.IsValueType && !right.Type.IsValueType)
                {
                    if (left.Type != right.Type)
                    {
                        if (WebUtil.IsNullConstant(left))
                        {
                            left = Expression.Constant(null, right.Type);
                        }
                        else if (WebUtil.IsNullConstant(right))
                        {
                            right = Expression.Constant(null, left.Type);
                        }
                        else if (left.Type.IsAssignableFrom(right.Type))
                        {
                            right = Expression.Convert(right, left.Type);
                        }
                        else if (right.Type.IsAssignableFrom(left.Type))
                        {
                            left = Expression.Convert(left, right.Type);
                        }
                        else
                        {
                            throw ExpressionParser.IncompatibleOperandsError(op.Text, left, right, op.Position);
                        }
                    }
                }
                else if (left == WebUtil.NullLiteral || right == WebUtil.NullLiteral)
                {
                    if (!equality)
                    {
                        throw ParseError(
                            Strings.RequestQueryParser_NullOperatorUnsupported(op.Text, op.Position, this.lexer.ExpressionText));
                    }

                    // Because we don't have an explicit "is null" check, literal comparisons
                    // to null are special.
                    if (!WebUtil.TypeAllowsNull(left.Type))
                    {
                        left = Expression.Convert(left, typeof(Nullable<>).MakeGenericType(left.Type));
                    }
                    else if (!WebUtil.TypeAllowsNull(right.Type))
                    {
                        right = Expression.Convert(right, typeof(Nullable<>).MakeGenericType(right.Type));
                    }
                }
                else
                {
                    // Enums should be checked here for promotion when supported, but they aren't in this version.
                    Debug.Assert(!IsEnumType(left.Type), "!IsEnumType(left.Type)");
                    Debug.Assert(!IsEnumType(right.Type), "!IsEnumType(right.Type)");

                    Type signatures = equality ? typeof(OperationSignatures.IEqualitySignatures) : typeof(OperationSignatures.IRelationalSignatures);
                    this.CheckAndPromoteOperands(signatures, op.Text, ref left, ref right, op.Position);
                }

                Debug.Assert(op.Id == TokenId.Identifier, "op.id == TokenId.Identifier");
                MethodInfo comparisonMethodInfo = null;
                if (!equality)
                {
                    if (left.Type == typeof(string))
                    {
                        comparisonMethodInfo = StringCompareMethodInfo;
                    }
                    else
                        if (left.Type == typeof(bool))
                        {
                            comparisonMethodInfo = BoolCompareMethodInfo;
                        }
                        else if (left.Type == typeof(bool?))
                        {
                            comparisonMethodInfo = BoolCompareMethodInfoNullable;
                        }
                        else
                            if (left.Type == typeof(Guid))
                            {
                                comparisonMethodInfo = GuidCompareMethodInfo;
                            }
                            else
                                if (left.Type == typeof(Guid?))
                                {
                                    comparisonMethodInfo = GuidCompareMethodInfoNullable;
                                }
                }

                switch (op.Text)
                {
                    case ExpressionConstants.KeywordEqual:
                        left = GenerateEqual(left, right);
                        break;
                    case ExpressionConstants.KeywordNotEqual:
                        left = GenerateNotEqual(left, right);
                        break;
                    case ExpressionConstants.KeywordGreaterThan:
                        left = GenerateGreaterThan(left, right, comparisonMethodInfo);
                        break;
                    case ExpressionConstants.KeywordGreaterThanOrEqual:
                        left = GenerateGreaterThanEqual(left, right, comparisonMethodInfo);
                        break;
                    case ExpressionConstants.KeywordLessThan:
                        left = GenerateLessThan(left, right, comparisonMethodInfo);
                        break;
                    case ExpressionConstants.KeywordLessThanOrEqual:
                        left = GenerateLessThanEqual(left, right, comparisonMethodInfo);
                        break;
                }

                return left;
            }

            /// <summary>Handles +, -, &amp; operators (&amp; for string concat, not supported).</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParseAdditive()
            {
                this.RecurseEnter();
                Expression left = this.ParseMultiplicative();
                while (this.CurrentToken.IdentifierIs(ExpressionConstants.KeywordAdd) ||
                    this.CurrentToken.IdentifierIs(ExpressionConstants.KeywordSub))
                {
                    Token op = this.CurrentToken;
                    this.lexer.NextToken();
                    Expression right = this.ParseMultiplicative();
                    if (op.IdentifierIs(ExpressionConstants.KeywordAdd))
                    {
                        this.CheckAndPromoteOperands(typeof(OperationSignatures.IAddSignatures), op.Text, ref left, ref right, op.Position);
                        left = GenerateAdd(left, right);
                    }
                    else
                    {
                        Debug.Assert(ExpressionParser.TokenIdentifierIs(op, ExpressionConstants.KeywordSub), "ExpressionParser.TokenIdentifierIs(op, ExpressionConstants.KeywordSub)");
                        this.CheckAndPromoteOperands(typeof(OperationSignatures.ISubtractSignatures), op.Text, ref left, ref right, op.Position);
                        left = GenerateSubtract(left, right);
                    }
                }

                this.RecurseLeave();
                return left;
            }

            /// <summary>Handles mul, div, mod operators.</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParseMultiplicative()
            {
                this.RecurseEnter();
                Expression left = this.ParseUnary();
                while (this.CurrentToken.IdentifierIs(ExpressionConstants.KeywordMultiply) ||
                    this.CurrentToken.IdentifierIs(ExpressionConstants.KeywordDivide) ||
                    this.CurrentToken.IdentifierIs(ExpressionConstants.KeywordModulo))
                {
                    Token op = this.CurrentToken;
                    this.lexer.NextToken();
                    Expression right = this.ParseUnary();
                    this.CheckAndPromoteOperands(typeof(OperationSignatures.IArithmeticSignatures), op.Text, ref left, ref right, op.Position);
                    if (op.IdentifierIs(ExpressionConstants.KeywordMultiply))
                    {
                        left = GenerateMultiply(left, right);
                    }
                    else if (op.IdentifierIs(ExpressionConstants.KeywordDivide))
                    {
                        left = GenerateDivide(left, right);
                    }
                    else
                    {
                        Debug.Assert(op.IdentifierIs(ExpressionConstants.KeywordModulo), "op.IdentifierIs(ExpressionConstants.KeywordModulo)");
                        left = GenerateModulo(left, right);
                    }
                }

                this.RecurseLeave();
                return left;
            }

            /// <summary>Handles -, not unary operators.</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParseUnary()
            {
                this.RecurseEnter();
                if (this.CurrentToken.Id == TokenId.Minus || this.CurrentToken.IdentifierIs(ExpressionConstants.KeywordNot))
                {
                    Token op = this.CurrentToken;
                    this.lexer.NextToken();
                    if (op.Id == TokenId.Minus && (ExpressionLexer.IsNumeric(this.CurrentToken.Id)))
                    {
                        Token numberLiteral = this.CurrentToken;
                        numberLiteral.Text = "-" + numberLiteral.Text;
                        numberLiteral.Position = op.Position;
                        this.CurrentToken = numberLiteral;
                        this.RecurseLeave();
                        return this.ParsePrimary();
                    }

                    Expression expr = this.ParseUnary();
                    if (op.Id == TokenId.Minus)
                    {
                        this.CheckAndPromoteOperand(typeof(OperationSignatures.INegationSignatures), op.Text, ref expr, op.Position);
                        expr = GenerateNegate(expr);
                    }
                    else
                    {
                        this.CheckAndPromoteOperand(typeof(OperationSignatures.INotSignatures), op.Text, ref expr, op.Position);
                        expr = GenerateNot(expr);
                    }

                    this.RecurseLeave();
                    return expr;
                }

                this.RecurseLeave();
                return this.ParsePrimary();
            }

            /// <summary>Handles primary expressions.</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParsePrimary()
            {
                this.RecurseEnter();
                Expression expr = this.ParsePrimaryStart(this.lexer);
                while (true)
                {
                    if (this.CurrentToken.Id == TokenId.Slash)
                    {
                        this.lexer.NextToken();
                        expr = this.ParseMemberAccess(expr);
                    }
                    else
                    {
                        break;
                    }
                }

                this.RecurseLeave();
                return expr;
            }

            /// <summary>Handles the start of primary expressions.</summary>
            /// <param name="l">Lexer to use for reading tokens</param>
            /// <returns>The parsed expression.</returns>
            private Expression ParsePrimaryStart(ExpressionLexer l)
            {
                switch (l.CurrentToken.Id)
                {
                    case TokenId.BooleanLiteral:
                        return this.ParseTypedLiteral(typeof(bool), XmlConstants.EdmBooleanTypeName, l);
                    case TokenId.DateTimeLiteral:
                        return this.ParseTypedLiteral(typeof(DateTime), XmlConstants.EdmDateTimeTypeName, l);
                    case TokenId.DecimalLiteral:
                        return this.ParseTypedLiteral(typeof(decimal), XmlConstants.EdmDecimalTypeName, l);
                    case TokenId.NullLiteral:
                        return ExpressionParser.ParseNullLiteral(l);
                    case TokenId.Identifier:
                        Debug.Assert(Object.ReferenceEquals(this.lexer, l), "Lexer should be the member lexer");
                        return this.ParseIdentifier();
                    case TokenId.StringLiteral:
                        return this.ParseTypedLiteral(typeof(string), XmlConstants.EdmStringTypeName, l);
                    case TokenId.Int64Literal:
                        return this.ParseTypedLiteral(typeof(Int64), XmlConstants.EdmInt64TypeName, l);
                    case TokenId.IntegerLiteral:
                        return this.ParseTypedLiteral(typeof(Int32), XmlConstants.EdmInt32TypeName, l);
                    case TokenId.DoubleLiteral:
                        return this.ParseTypedLiteral(typeof(double), XmlConstants.EdmDoubleTypeName, l);
                    case TokenId.SingleLiteral:
                        return this.ParseTypedLiteral(typeof(Single), XmlConstants.EdmSingleTypeName, l);
                    case TokenId.GuidLiteral:
                        return this.ParseTypedLiteral(typeof(Guid), XmlConstants.EdmGuidTypeName, l);
                    case TokenId.BinaryLiteral:
                        return this.ParseTypedLiteral(typeof(byte[]), XmlConstants.EdmBinaryTypeName, l);
                    case TokenId.OpenParen:
                        Debug.Assert(Object.ReferenceEquals(this.lexer, l), "Lexer should be the member lexer");
                        return this.ParseParenExpression();
                    default:
                        throw ParseError(Strings.RequestQueryParser_ExpressionExpected(l.CurrentToken.Position));
                }
            }

            /// <summary>Handles parenthesized expressions.</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParseParenExpression()
            {
                if (this.CurrentToken.Id != TokenId.OpenParen)
                {
                    throw ParseError(Strings.RequestQueryParser_OpenParenExpected(this.CurrentToken.Position));
                }

                this.lexer.NextToken();
                Expression e = this.ParseExpression();
                if (this.CurrentToken.Id != TokenId.CloseParen)
                {
                    throw ParseError(Strings.RequestQueryParser_CloseParenOrOperatorExpected(this.CurrentToken.Position));
                }

                this.lexer.NextToken();
                return e;
            }

            /// <summary>Handles identifiers.</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParseIdentifier()
            {
                this.ValidateToken(TokenId.Identifier);

                // An open paren here would indicate calling a method in regular C# syntax.
                bool identifierIsFunction = this.lexer.PeekNextToken().Id == TokenId.OpenParen;
                if (identifierIsFunction)
                {
                    return this.ParseIdentifierAsFunction();
                }
                else
                {
                    this.currentSegmentInfo = new SegmentTypeInfo(this.typeForIt, this.setForIt, false);
                    return this.ParseMemberAccess(this.it);
                }
            }

            /// <summary>Handles identifiers which have been recognized as functions.</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParseIdentifierAsFunction()
            {
                FunctionDescription[] functionDescriptions;
                Token functionToken = this.CurrentToken;
                if (!functions.TryGetValue(functionToken.Text, out functionDescriptions))
                {
                    throw ParseError(Strings.RequestQueryParser_UnknownFunction(functionToken.Text, functionToken.Position));
                }

                if (!this.service.Configuration.DataServiceBehavior.AcceptReplaceFunctionInQuery && functionDescriptions[0].IsReplace)
                {
                    throw ParseError(Strings.RequestQueryParser_UnknownFunction(functionToken.Text, functionToken.Position));
                }

                this.lexer.NextToken();
                Expression[] originalArguments = this.ParseArgumentList();
                Expression[] arguments = this.nullPropagationRequired ? originalArguments : (Expression[])originalArguments.Clone();
                Expression candidateTypeArgument = arguments.Length > 1 ? arguments[1] :
                                                   arguments.Length > 0 ? arguments[0] :
                                                   null;

                // check if the one of the parameters is of open type. If yes, then we need to invoke
                // LateBoundMethods for this function otherwise not.
                bool openParameters = false;
                if (!functionDescriptions[0].IsTypeCast && !functionDescriptions[0].IsTypeCheck)
                {
                    foreach (Expression argument in arguments)
                    {
                        if (IsOpenPropertyExpression(argument))
                        {
                            openParameters = true;
                            break;
                        }
                    }
                }

                Expression result = null;
                FunctionDescription function = null;
                if (openParameters)
                {
                    foreach (FunctionDescription functionDescription in functionDescriptions)
                    {
                        if (functionDescription.ParameterTypes.Length == arguments.Length)
                        {
                            function = functionDescription;
                            break;
                        }
                    }

                    Expression[] openArguments = new Expression[arguments.Length];
                    for (int i = 0; i < openArguments.Length; i++)
                    {
                        if (IsOpenPropertyExpression(arguments[i]))
                        {
                            openArguments[i] = arguments[i];
                        }
                        else
                        {
                            openArguments[i] = Expression.Convert(arguments[i], typeof(object));
                        }
                    }

                    if (function == null)
                    {
                        string message = Strings.RequestQueryParser_NoApplicableFunction(
                            functionToken.Text,
                            functionToken.Position,
                            FunctionDescription.BuildSignatureList(functionToken.Text, functionDescriptions));
                        throw ParseError(message);
                    }

                    result = function.InvokeOpenTypeMethod(openArguments);
                }
                else
                {
                    function = this.FindBestFunction(functionDescriptions, ref arguments);
                    if (function == null)
                    {
                        string message = Strings.RequestQueryParser_NoApplicableFunction(
                            functionToken.Text,
                            functionToken.Position,
                            FunctionDescription.BuildSignatureList(functionToken.Text, functionDescriptions));
                        throw ParseError(message);
                    }

                    // Special case for null propagation - we never strip nullability from expressions.
                    if (this.nullPropagationRequired && function.IsTypeCast)
                    {
                        Expression typeExpression = arguments[arguments.Length - 1];
                        Debug.Assert(typeExpression != null, "typeExpression != null -- otherwise function finding failed.");
                        if (typeExpression.Type == typeof(Type))
                        {
                            Type castTargetType = (Type)((ConstantExpression)typeExpression).Value;
                            if (!WebUtil.TypeAllowsNull(castTargetType))
                            {
                                arguments[arguments.Length - 1] = Expression.Constant(typeof(Nullable<>).MakeGenericType(castTargetType));
                            }
                        }
                    }

                    Expression[] finalArguments;
                    if (function.ConversionFunction == FunctionDescription.BinaryIsOfResourceType ||
                        function.ConversionFunction == FunctionDescription.BinaryCastResourceType)
                    {
                        finalArguments = new Expression[arguments.Length + 1];
                        for (int i = 0; i < arguments.Length; i++)
                        {
                            finalArguments[i] = arguments[i];
                        }

                        finalArguments[arguments.Length] = Expression.Constant(IsOpenPropertyExpression(arguments[0]), typeof(bool));
                    }
                    else
                    {
                        finalArguments = arguments;
                    }

                    result = function.ConversionFunction(this.it, finalArguments);
                }

                if (this.nullPropagationRequired && !function.IsTypeCheck && !function.IsTypeCast)
                {
                    Debug.Assert(
                        originalArguments.Length == arguments.Length,
                        "originalArguments.Length == arguments.Length -- arguments should not be added/removed");
                    for (int i = 0; i < originalArguments.Length; i++)
                    {
                        result = this.ConsiderNullPropagation(originalArguments[i], result);
                    }
                }

                // type casts change the type of the "current" item, reflect this in parser state
                if (function.IsTypeCast)
                {
                    Debug.Assert(candidateTypeArgument != null, "Should have failed to bind to the function if arguments don't match");
                    Debug.Assert(candidateTypeArgument.Type == typeof(string), "First argument to 'cast' should have been a string");
                    Debug.Assert(candidateTypeArgument.NodeType == ExpressionType.Constant, "Non-constant in type name for a cast?");

                    this.currentSegmentInfo = new SegmentTypeInfo(
                        WebUtil.TryResolveResourceType(this.provider, (string)((ConstantExpression)candidateTypeArgument).Value),
                        this.currentSegmentInfo != null ? this.currentSegmentInfo.ResourceSet : this.setForIt,
                        this.currentSegmentInfo != null ? this.currentSegmentInfo.IsCollection : false);
                }

                return result;
            }

            /// <summary>Handles boolean literals.</summary>
            /// <returns>The parsed expression.</returns>
            private Expression ParseBooleanLiteral()
            {
                Debug.Assert(this.CurrentToken.Id == TokenId.BooleanLiteral, "this.CurrentToken.Id == TokenId.BooleanLiteral");
                string originalText = this.CurrentToken.Text;
                this.lexer.NextToken();
                if (originalText == ExpressionConstants.KeywordTrue)
                {
                    return trueLiteral;
                }
                else
                {
                    Debug.Assert(originalText == ExpressionConstants.KeywordFalse, "originalText == ExpressionConstants.KeywordFalse");
                    return falseLiteral;
                }
            }

            /// <summary>Handles typed literals.</summary>
            /// <param name="targetType">Expected type to be parsed.</param>
            /// <param name="targetTypeName">Expected type name.</param>
            /// <param name="l">Lexer to use for reading tokens</param>
            /// <returns>The constants expression produced by building the given literal.</returns>
            private Expression ParseTypedLiteral(Type targetType, string targetTypeName, ExpressionLexer l)
            {
                object targetValue;
                if (!WebConvert.TryKeyStringToPrimitive(l.CurrentToken.Text, targetType, out targetValue))
                {
                    string message = Strings.RequestQueryParser_UnrecognizedLiteral(targetTypeName, l.CurrentToken.Text, l.CurrentToken.Position);
                    throw ParseError(message);
                }

                Expression result = this.CreateLiteral(targetValue, l.CurrentToken.Text);
                l.NextToken();
                return result;
            }

            /// <summary>Handles member access.</summary>
            /// <param name="instance">Instance being accessed.</param>
            /// <returns>The parsed expression.</returns>
            private Expression ParseMemberAccess(Expression instance)
            {
                Debug.Assert(instance != null, "instance != null");

                Type type = instance.Type;
                int errorPos = this.lexer.Position;
                string id = this.CurrentToken.GetIdentifier();
                this.lexer.NextToken();

                // Disallow the member access for a collection property.
                Debug.Assert(this.currentSegmentInfo != null, "Must have initialized current segment information.");
                if (this.currentSegmentInfo.IsCollection)
                {
                    throw ParseError(Strings.RequestQueryParser_UnknownProperty(id, WebUtil.GetTypeName(type), errorPos));
                }

                // An open paren here would indicate calling a method in regular C# syntax.
                ResourceProperty property = this.currentSegmentInfo.ResourceType == null ?
                                                null : // null means the current segment was already an open type
                                                this.currentSegmentInfo.ResourceType.TryResolvePropertyName(id);

                ResourceSetWrapper container = this.currentSegmentInfo.ResourceSet == null || property == null || property.TypeKind != ResourceTypeKind.EntityType ?
                                    null :
                                    this.provider.GetContainer(this.currentSegmentInfo.ResourceSet, this.currentSegmentInfo.ResourceType, property);

                if (property != null)
                {
                    if (property.TypeKind == ResourceTypeKind.EntityType && container == null)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidPropertyNameSpecified(id, this.currentSegmentInfo.ResourceType.FullName));
                    }

                    // We have a strongly-type property.
                    Expression propertyAccess;
                    if (property.CanReflectOnInstanceTypeProperty)
                    {
                        propertyAccess = Expression.Property(instance, this.currentSegmentInfo.ResourceType.GetPropertyInfo(property));
                    }
                    else
                    {
                        // object DataServiceProviderMethods.GetValue(object, ResourceProperty)
                        propertyAccess = Expression.Call(
                                            null, /*instance*/
                                            DataServiceProviderMethods.GetValueMethodInfo,
                                            instance,
                                            Expression.Constant(property));
                        propertyAccess = Expression.Convert(propertyAccess, property.Type);
                    }

                    Expression result = this.ConsiderNullPropagation(instance, propertyAccess);
                    if (container != null)
                    {
                        bool singleResult = property.Kind == ResourcePropertyKind.ResourceReference;
                        DataServiceConfiguration.CheckResourceRightsForRead(container, singleResult);
                        Expression filter = DataServiceConfiguration.ComposeQueryInterceptors(this.service, container);
                        if (filter != null)
                        {
                            // We did null propagation for accessing the property, but we may also need
                            // to do null propagation on the property value itself (otherwise the interception
                            // lambda needs to check the argument for null, which is probably unexpected).
                            result = RequestQueryProcessor.ComposePropertyNavigation(
                                result, (LambdaExpression)filter, this.provider.NullPropagationRequired);
                        }
                    }

                    this.currentSegmentInfo = new SegmentTypeInfo(
                                    property.ResourceType,
                                    container,
                                    property.Kind == ResourcePropertyKind.ResourceSetReference);
                    return result;
                }
                else
                {
                    ResourceType resourceType = this.currentSegmentInfo.ResourceType;
                    this.currentSegmentInfo = new SegmentTypeInfo(null, null, false);

                    // Open types can have any members inside them
                    if (resourceType == null || resourceType.IsOpenType)
                    {
                        // object OpenTypeMethods.GetValue(object, string)
                        Expression call =
                            Expression.Call(null /* instance */, OpenTypeMethods.GetValueOpenPropertyMethodInfo, instance, Expression.Constant(id));
                        return this.ConsiderNullPropagation(instance, call);
                    }
                    else
                    {
                        throw ParseError(Strings.RequestQueryParser_UnknownProperty(id, WebUtil.GetTypeName(type), errorPos));
                    }
                }
            }

            /// <summary>Handles argument lists.</summary>
            /// <returns>The parsed expressions.</returns>
            private Expression[] ParseArgumentList()
            {
                if (this.CurrentToken.Id != TokenId.OpenParen)
                {
                    throw ParseError(Strings.RequestQueryParser_OpenParenExpected(this.CurrentToken.Position));
                }

                this.lexer.NextToken();
                Expression[] args = this.CurrentToken.Id != TokenId.CloseParen ? this.ParseArguments() : emptyExpressions;
                if (this.CurrentToken.Id != TokenId.CloseParen)
                {
                    throw ParseError(Strings.RequestQueryParser_CloseParenOrCommaExpected(this.CurrentToken.Position));
                }

                this.lexer.NextToken();
                return args;
            }

            /// <summary>Handles comma-separated arguments.</summary>
            /// <returns>The parsed expressions.</returns>
            private Expression[] ParseArguments()
            {
                List<Expression> argList = new List<Expression>();
                while (true)
                {
                    argList.Add(this.ParseExpression());
                    if (this.CurrentToken.Id != TokenId.Comma)
                    {
                        break;
                    }

                    this.lexer.NextToken();
                }

                return argList.ToArray();
            }

            #endregion Parsing.

            /// <summary>Creates a constant expression with the specified literal.</summary>
            /// <param name="value">Constant value.</param>
            /// <param name="text">Value text.</param>
            /// <returns>The created expression.</returns>
            private Expression CreateLiteral(object value, string text)
            {
                // DEVNOTE(Microsoft):
                // The following code rely on Expression.Constant returning UNIQUE instances for every call to it
                // i.e., Expression.Constant(1) does not reference equals to Expression.Constant(1)
                ConstantExpression expr = Expression.Constant(value);
                this.literals.Add(expr, text);
                return expr;
            }

            /// <summary>Finds the best fitting function for the specified arguments.</summary>
            /// <param name="functions">Functions to consider.</param>
            /// <param name="arguments">Arguments; if a best function is found, promoted arguments.</param>
            /// <returns>The best fitting function; null if none found or ambiguous.</returns>
            private FunctionDescription FindBestFunction(FunctionDescription[] functions, ref Expression[] arguments)
            {
                Debug.Assert(functions != null, "functions != null");
                List<FunctionDescription> applicableFunctions = new List<FunctionDescription>(functions.Length);
                List<Expression[]> applicablePromotedArguments = new List<Expression[]>(functions.Length);

                // Build a list of applicable functions (and cache their promoted arguments).
                foreach (FunctionDescription candidate in functions)
                {
                    if (candidate.ParameterTypes.Length != arguments.Length)
                    {
                        continue;
                    }

                    Expression[] promotedArguments = new Expression[arguments.Length];
                    bool argumentsMatch = true;
                    for (int i = 0; i < candidate.ParameterTypes.Length; i++)
                    {
                        promotedArguments[i] = this.PromoteExpression(arguments[i], candidate.ParameterTypes[i], true);
                        if (promotedArguments[i] == null)
                        {
                            argumentsMatch = false;
                            break;
                        }
                    }

                    if (argumentsMatch)
                    {
                        applicableFunctions.Add(candidate);
                        applicablePromotedArguments.Add(promotedArguments);
                    }
                }

                // Return the best applicable function.
                if (applicableFunctions.Count == 0)
                {
                    // No matching function.
                    return null;
                }
                else if (applicableFunctions.Count == 1)
                {
                    arguments = applicablePromotedArguments[0];
                    return applicableFunctions[0];
                }
                else
                {
                    // Find a single function which is better than all others.
                    int bestFunctionIndex = -1;
                    for (int i = 0; i < applicableFunctions.Count; i++)
                    {
                        bool betterThanAllOthers = true;
                        for (int j = 0; j < applicableFunctions.Count; j++)
                        {
                            if (i != j && IsBetterThan(arguments, applicableFunctions[j].ParameterTypes, applicableFunctions[i].ParameterTypes))
                            {
                                betterThanAllOthers = false;
                                break;
                            }
                        }

                        if (betterThanAllOthers)
                        {
                            if (bestFunctionIndex == -1)
                            {
                                bestFunctionIndex = i;
                            }
                            else
                            {
                                // Ambiguous.
                                return null;
                            }
                        }
                    }

                    if (bestFunctionIndex == -1)
                    {
                        return null;
                    }

                    arguments = applicablePromotedArguments[bestFunctionIndex];
                    return applicableFunctions[bestFunctionIndex];
                }
            }

            /// <summary>Rewrites an expression to propagate null values if necessary.</summary>
            /// <param name='element'>Expression to check for null.</param>
            /// <param name='notNullExpression'>Expression to yield if <paramref name='element' /> does not yield null.</param>
            /// <returns>The possibly rewriteen <paramref name='notNullExpression' />.</returns>
            private Expression ConsiderNullPropagation(Expression element, Expression notNullExpression)
            {
                if (!this.nullPropagationRequired || element == this.it || !WebUtil.TypeAllowsNull(element.Type))
                {
                    return notNullExpression;
                }

                // Tiny optimization: remove the check on constants which are known not to be null.
                // Otherwise every string literal propagates out, which is correct but unnecessarily messy.
                if (element is ConstantExpression && element != WebUtil.NullLiteral)
                {
                    return notNullExpression;
                }

                // ifTrue and ifFalse expressions must match exactly. We need to ensure that the 'false'
                // side is nullable, and the 'true' side is a null of the correct type.
                Expression test = Expression.Equal(element, Expression.Constant(null, element.Type));
                Expression falseIf = notNullExpression;
                if (!WebUtil.TypeAllowsNull(falseIf.Type))
                {
                    falseIf = Expression.Convert(falseIf, typeof(Nullable<>).MakeGenericType(falseIf.Type));
                }

                Expression trueIf = Expression.Constant(null, falseIf.Type);
                return Expression.Condition(test, trueIf, falseIf);
            }

            /// <summary>Checks that the operand (possibly promoted) is valid for the specified operation.</summary>
            /// <param name="signatures">Type with signatures to match.</param>
            /// <param name="operationName">Name of operation for error reporting.</param>
            /// <param name="expr">Expression for operand.</param>
            /// <param name="errorPos">Position for error reporting.</param>
            private void CheckAndPromoteOperand(Type signatures, string operationName, ref Expression expr, int errorPos)
            {
                Debug.Assert(signatures != null, "signatures != null");
                Debug.Assert(operationName != null, "operationName != null");
                Expression[] args = new Expression[] { expr };
                MethodBase method;
                if (this.FindMethod(signatures, "F", args, out method) != 1)
                {
                    throw ParseError(Strings.RequestQueryParser_IncompatibleOperand(operationName, WebUtil.GetTypeName(args[0].Type), errorPos));
                }

                expr = args[0];
            }

            /// <summary>Checks that the operands (possibly promoted) are valid for the specified operation.</summary>
            /// <param name="signatures">Type with signatures to match.</param>
            /// <param name="operationName">Name of operation for error reporting.</param>
            /// <param name="left">Expression for left operand.</param>
            /// <param name="right">Expression for right operand.</param>
            /// <param name="errorPos">Position for error reporting.</param>
            private void CheckAndPromoteOperands(Type signatures, string operationName, ref Expression left, ref Expression right, int errorPos)
            {
                Expression[] args = new Expression[] { left, right };
                MethodBase method;
                if (this.FindMethod(signatures, "F", args, out method) != 1)
                {
                    throw ExpressionParser.IncompatibleOperandsError(operationName, left, right, errorPos);
                }

                left = args[0];
                right = args[1];
            }

            /// <summary>Finds the named method in the specifid type.</summary>
            /// <param name="type">Type to look in.</param>
            /// <param name="methodName">Name of method to look for.</param>
            /// <param name="args">Arguments to method.</param>
            /// <param name="method">Best method found.</param>
            /// <returns>Number of matching methods.</returns>
            private int FindMethod(Type type, string methodName, Expression[] args, out MethodBase method)
            {
                BindingFlags flags = BindingFlags.Public | BindingFlags.DeclaredOnly | BindingFlags.Static | BindingFlags.Instance;
                foreach (Type t in SelfAndBaseTypes(type))
                {
                    MemberInfo[] members = t.FindMembers(MemberTypes.Method, flags, Type.FilterName, methodName);
                    int count = this.FindBestMethod(members.Cast<MethodBase>(), args, out method);
                    if (count != 0)
                    {
                        return count;
                    }
                }

                method = null;
                return 0;
            }

            /// <summary>Finds all applicable methods from the specified enumeration that match the arguments.</summary>
            /// <param name="methods">Enumerable object of candidate methods.</param>
            /// <param name="args">Argument expressions.</param>
            /// <returns>Methods that apply to the specified arguments.</returns>
            private MethodData[] FindApplicableMethods(IEnumerable<MethodBase> methods, Expression[] args)
            {
                List<MethodData> result = new List<MethodData>();
                foreach (MethodBase method in methods)
                {
                    MethodData methodData = new MethodData(method, method.GetParameters());
                    if (this.IsApplicable(methodData, args))
                    {
                        result.Add(methodData);
                    }
                }

                return result.ToArray();
            }

            /// <summary>Finds the best methods for the specified arguments given a candidate method enumeration.</summary>
            /// <param name="methods">Enumerable object for candidate methods.</param>
            /// <param name="args">Argument expressions to match.</param>
            /// <param name="method">Best matched method.</param>
            /// <returns>The number of "best match" methods.</returns>
            private int FindBestMethod(IEnumerable<MethodBase> methods, Expression[] args, out MethodBase method)
            {
                MethodData[] applicable = this.FindApplicableMethods(methods, args);
                if (applicable.Length > 1)
                {
                    applicable = FindBestApplicableMethods(applicable, args);
                }

                int result = applicable.Length;
                method = null;
                if (applicable.Length == 1)
                {
                    // If we started off with all non-OpenType expressions and end with all-OpenType
                    // expressions, we've been too aggresive - the transition from non-open-types
                    // to open types should initially happen only as a result of accessing open properties.
                    MethodData md = applicable[0];
                    bool originalArgsDefined = true;
                    bool promotedArgsOpen = true;
                    for (int i = 0; i < args.Length; i++)
                    {
                        originalArgsDefined = originalArgsDefined && !IsOpenPropertyExpression(args[i]);
                        promotedArgsOpen = promotedArgsOpen && md.Parameters[i].ParameterType == typeof(object);
                        args[i] = md.Args[i];
                    }

                    method = (originalArgsDefined && promotedArgsOpen) ? null : md.MethodBase;
                    result = (method == null) ? 0 : 1;
                }
                else if (applicable.Length > 1)
                {
                    // We may have the case for operators (which C# doesn't) in which we have a nullable operand
                    // and a non-nullable operand. We choose to convert the one non-null operand to nullable in that
                    // case (the binary expression will lift to null).
                    if (args.Length == 2 && applicable.Length == 2 &&
                        GetNonNullableType(applicable[0].Parameters[0].ParameterType) ==
                        GetNonNullableType(applicable[1].Parameters[0].ParameterType))
                    {
                        MethodData nullableMethod =
                            WebUtil.TypeAllowsNull(applicable[0].Parameters[0].ParameterType) ?
                            applicable[0] :
                            applicable[1];
                        args[0] = nullableMethod.Args[0];
                        args[1] = nullableMethod.Args[1];
                        return this.FindBestMethod(methods, args, out method);
                    }
                }

                return result;
            }

            /// <summary>Checks whether the specified method is applicable given the argument expressions.</summary>
            /// <param name="method">Method to check.</param>
            /// <param name="args">Argument expressions.</param>
            /// <returns>true if the method is applicable; false otherwise.</returns>
            private bool IsApplicable(MethodData method, Expression[] args)
            {
                if (method.Parameters.Length != args.Length)
                {
                    return false;
                }

                Expression[] promotedArgs = new Expression[args.Length];
                for (int i = 0; i < args.Length; i++)
                {
                    ParameterInfo pi = method.Parameters[i];
                    Debug.Assert(!pi.IsOut, "!pi.IsOut");
                    Expression promoted = this.PromoteExpression(args[i], pi.ParameterType, false);
                    if (promoted == null)
                    {
                        return false;
                    }

                    promotedArgs[i] = promoted;
                }

                method.Args = promotedArgs;
                return true;
            }

            /// <summary>Promotes the specified expression to the given type if necessary.</summary>
            /// <param name="expr">Expression to promote.</param>
            /// <param name="type">Type to change expression to.</param>
            /// <param name="exact">Whether an exact type is required; false implies a compatible type is OK.</param>
            /// <returns>Expression with the promoted type.</returns>
            private Expression PromoteExpression(Expression expr, Type type, bool exact)
            {
                Debug.Assert(expr != null, "expr != null");
                Debug.Assert(type != null, "type != null");
                if (expr.Type == type)
                {
                    return expr;
                }

                ConstantExpression ce = expr as ConstantExpression;
                if (ce != null)
                {
                    if (ce == WebUtil.NullLiteral)
                    {
                        if (WebUtil.TypeAllowsNull(type))
                        {
                            return Expression.Constant(null, type);
                        }
                    }
                    else
                    {
                        string text;
                        if (this.literals.TryGetValue(ce, out text))
                        {
                            Type target = GetNonNullableType(type);
                            object value = null;
                            if (ce.Type == typeof(string) && (target == typeof(Type) || target == typeof(ResourceType)))
                            {
                                if (WebConvert.TryRemoveQuotes(ref text))
                                {
                                    ResourceType resourceType = WebUtil.TryResolveResourceType(this.provider, text);
                                    if (resourceType != null)
                                    {
                                        if (target == typeof(Type))
                                        {
                                            if (resourceType.CanReflectOnInstanceType == true)
                                            {
                                                value = resourceType.InstanceType;
                                            }
                                        }
                                        else
                                        {
                                            if (resourceType.CanReflectOnInstanceType == false)
                                            {
                                                value = resourceType;
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                switch (Type.GetTypeCode(ce.Type))
                                {
                                    case TypeCode.Int32:
                                    case TypeCode.Int64:
                                        value = ParseNumber(text, target);
                                        break;
                                    case TypeCode.Double:
                                        if (target == typeof(decimal))
                                        {
                                            value = ParseNumber(text, target);
                                        }

                                        break;
                                }
                            }

                            if (value != null)
                            {
                                return Expression.Constant(value, type);
                            }
                        }
                    }
                }

                if (IsCompatibleWith(expr.Type, type))
                {
                    if (type.IsValueType || exact)
                    {
                        return Expression.Convert(expr, type);
                    }

                    return expr;
                }

                // Allow promotion from nullable to non-nullable by directly accessing underlying value.
                if (WebUtil.IsNullableType(expr.Type) && type.IsValueType)
                {
                    Expression valueAccessExpression = Expression.Property(expr, "Value");
                    valueAccessExpression = this.PromoteExpression(valueAccessExpression, type, exact);
                    return valueAccessExpression;
                }

                return null;
            }

            /// <summary>Checks that the current token has the specified identifier.</summary>
            /// <param name="id">Identifier to check.</param>
            /// <returns>true if the current token is an identifier with the specified text.</returns>
            private bool TokenIdentifierIs(string id)
            {
                return this.CurrentToken.IdentifierIs(id);
            }

            /// <summary>Validates the current token is of the specified kind.</summary>
            /// <param name="t">Expected token kind.</param>
            private void ValidateToken(TokenId t)
            {
                if (this.CurrentToken.Id != t)
                {
                    throw ParseError(Strings.RequestQueryParser_SyntaxError(this.CurrentToken.Position));
                }
            }

            #region Recursion control.

            /// <summary>Marks the fact that a recursive method was entered, and checks that the depth is allowed.</summary>
            private void RecurseEnter()
            {
                WebUtil.RecurseEnterQueryParser(RecursionLimit, ref this.recursionDepth);
            }

            /// <summary>Marks the fact that a recursive method is leaving.</summary>
            private void RecurseLeave()
            {
                WebUtil.RecurseLeave(ref this.recursionDepth);
            }

            #endregion Recursion control.

            /// <summary>Use this class to encapsulate method information.</summary>
            [DebuggerDisplay("MethodData {methodBase}")]
            private class MethodData
            {
                #region Private fields.

                /// <summary>Described method.</summary>
                private readonly MethodBase methodBase;

                /// <summary>Parameters for method.</summary>
                private readonly ParameterInfo[] parameters;

                /// <summary>Argument expressions.</summary>
                private Expression[] args;

                #endregion Private fields.

                #region Constructors.

                /// <summary>Initializes a new <see cref="MethodData"/> instance.</summary>
                /// <param name="method">Described method</param>
                /// <param name="parameters">Parameters for method.</param>
                public MethodData(MethodBase method, ParameterInfo[] parameters)
                {
                    this.methodBase = method;
                    this.parameters = parameters;
                }

                #endregion Constructors.

                #region Properties.

                /// <summary>Argument expressions.</summary>
                public Expression[] Args
                {
                    get { return this.args; }
                    set { this.args = value; }
                }

                /// <summary>Described method.</summary>
                public MethodBase MethodBase
                {
                    get { return this.methodBase; }
                }

                /// <summary>Parameters for method.</summary>
                public ParameterInfo[] Parameters
                {
                    get { return this.parameters; }
                }

                /// <summary>Enumeration of parameter types.</summary>
                public IEnumerable<Type> ParameterTypes
                {
                    get
                    {
                        foreach (ParameterInfo parameter in this.Parameters)
                        {
                            yield return parameter.ParameterType;
                        }
                    }
                }

                #endregion Properties.
            }

            /// <summary>
            /// ResourceType and ResourceSet information for current segment.
            /// This is used only when parsing member access paths.
            /// </summary>
            private class SegmentTypeInfo
            {
                /// <summary>Constructor.</summary>
                /// <param name="resourceType">Type for current segment.</param>
                /// <param name="resourceSet">Set for current segment.</param>
                /// <param name="isCollection">Does current segment property refer to a collection.</param>
                public SegmentTypeInfo(ResourceType resourceType, ResourceSetWrapper resourceSet, bool isCollection)
                {
                    this.ResourceType = resourceType;
                    this.ResourceSet = resourceSet;
                    this.IsCollection = isCollection;
                }

                /// <summary>Type for the current segment.</summary>
                public ResourceType ResourceType
                {
                    get;
                    private set;
                }

                /// <summary>Resource set for the current segment.</summary>
                public ResourceSetWrapper ResourceSet
                {
                    get;
                    private set;
                }

                /// <summary>Does the current segment property refer to a collection.</summary>
                public bool IsCollection
                {
                    get;
                    private set;
                }
            }
        }
    }
}
