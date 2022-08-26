//---------------------------------------------------------------------
// <copyright file="ExpressionNormalizer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner  Microsoft, Microsoft
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq.Expressions;
    using System.Reflection;

    #endregion Namespaces.

    /// <summary>
    /// Replaces expression patterns produced by the compiler with approximations
    /// used in query translation. For instance, the following VB code:
    /// 
    ///     x = y
    ///     
    /// becomes the expression
    /// 
    ///     Equal(MethodCallExpression(Microsoft.VisualBasic.CompilerServices.Operators.CompareString(x, y, False), 0)
    ///     
    /// which is normalized to
    /// 
    ///     Equal(x, y)
    ///     
    /// Comment convention:
    /// 
    ///     CODE(Lang): _VB or C# coding pattern being simplified_
    ///     ORIGINAL: _original LINQ expression_
    ///     NORMALIZED: _normalized LINQ expression_
    /// </summary>
    internal class ExpressionNormalizer : DataServiceALinqExpressionVisitor
    {
        #region Private fields.

        /// <summary>
        /// If we encounter a MethodCallExpression, we never need to lift to lift to null. This capability
        /// exists to translate certain patterns in the language. In this case, the user (or compiler)
        /// has explicitly asked for a method invocation (at which point, lifting can no longer occur).
        /// </summary>
        private const bool LiftToNull = false;

        /// <summary>
        /// Gets a dictionary mapping from LINQ expressions to matched by those expressions. Used
        /// to identify composite expression patterns.
        /// </summary>
        private readonly Dictionary<Expression, Pattern> _patterns = new Dictionary<Expression, Pattern>(ReferenceEqualityComparer<Expression>.Instance);

        /// <summary>Records the generated-to-source rewrites created.</summary>
        private readonly Dictionary<Expression, Expression> normalizerRewrites;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initializes a new <see cref="ExpressionNormalizer"/> instance.</summary>
        /// <param name="normalizerRewrites">Dictionary in which to store rewrites.</param>
        private ExpressionNormalizer(Dictionary<Expression, Expression> normalizerRewrites)
        {
            Debug.Assert(normalizerRewrites != null, "normalizerRewrites != null");
            this.normalizerRewrites = normalizerRewrites;
        }

        #endregion Constructors.

        #region Internal properties.

        /// <summary>Records the generated-to-source rewrites created.</summary>
        internal Dictionary<Expression, Expression> NormalizerRewrites
        {
            get { return this.normalizerRewrites; }
        }

        #endregion Internal properties.

        /// <summary>
        /// Applies normalization rewrites to the specified 
        /// <paramref name="expression"/>, recording them in the 
        /// <paramref name="rewrites"/> dictionary.
        /// </summary>
        /// <param name="expression">Expression to normalize.</param>
        /// <param name="rewrites">Dictionary in which to record rewrites.</param>
        /// <returns>The normalized expression.</returns>
        internal static Expression Normalize(Expression expression, Dictionary<Expression, Expression> rewrites)
        {
            Debug.Assert(expression != null, "expression != null");
            Debug.Assert(rewrites != null, "rewrites != null");

            ExpressionNormalizer normalizer = new ExpressionNormalizer(rewrites);
            Expression result = normalizer.Visit(expression);
            return result;
        }

        /// <summary>
        /// Handle binary patterns:
        /// 
        /// - VB 'Is' operator
        /// - Compare patterns
        /// </summary>
        internal override Expression VisitBinary(BinaryExpression b)
        {
            BinaryExpression visited = (BinaryExpression)base.VisitBinary(b);

            // CODE(VB): x Is y
            // ORIGINAL: Equal(Convert(x, typeof(object)), Convert(y, typeof(object))
            // NORMALIZED: Equal(x, y)
            if (visited.NodeType == ExpressionType.Equal)
            {
                Expression normalizedLeft = UnwrapObjectConvert(visited.Left);
                Expression normalizedRight = UnwrapObjectConvert(visited.Right);
                if (normalizedLeft != visited.Left || normalizedRight != visited.Right)
                {
                    visited = CreateRelationalOperator(ExpressionType.Equal, normalizedLeft, normalizedRight);
                }
            }

            // CODE(VB): x = y
            // ORIGINAL: Equal(Microsoft.VisualBasic.CompilerServices.Operators.CompareString(x, y, False), 0)
            // NORMALIZED: Equal(x, y)
            Pattern pattern;
            if (_patterns.TryGetValue(visited.Left, out pattern) && pattern.Kind == PatternKind.Compare && IsConstantZero(visited.Right))
            {
                ComparePattern comparePattern = (ComparePattern)pattern;
                // handle relational operators
                BinaryExpression relationalExpression;
                if (TryCreateRelationalOperator(visited.NodeType, comparePattern.Left, comparePattern.Right, out relationalExpression))
                {
                    visited = relationalExpression;
                }
            }

            this.RecordRewrite(b, visited);

            return visited;
        }

        /// <summary>
        /// CODE: x
        /// ORIGINAL: Convert(x, t) where t is assignable from typeof(x)
        /// ORIGINAL: x as t, where t is assignable from typeof(x)
        /// ORIGINAL: and typeof(x) or t are not known primitives unless typeof(x) == t
        /// ORIGINAL: and x is not a collection of entity types
        /// NORMALIZED: x
        /// </summary>
        internal override Expression VisitUnary(UnaryExpression u)
        {
            UnaryExpression visited = (UnaryExpression)base.VisitUnary(u);
            Expression result = visited;

            // Note that typically we would record a potential rewrite
            // after extracting the conversion, but we avoid doing this
            // because it breaks undoing the rewrite by making the non-local
            // change circular, ie:
            //   unary [operand = a]
            // becomes 
            //   a <- unary [operand = a]
            // So the undoing visits a, then the original unary, then the 
            // operand and again the unary, the operand, etc.
            this.RecordRewrite(u, result);

            // Convert(x, t) or x as t, where t is assignable from typeof(x)
            if ((visited.NodeType == ExpressionType.Convert || visited.NodeType == ExpressionType.TypeAs) && visited.Type.IsAssignableFrom(visited.Operand.Type))
            {
                // typeof(x) or t are not known primitives unless typeof(x) == t
                if (!ClientConvert.IsKnownNullableType(visited.Operand.Type) && !ClientConvert.IsKnownNullableType(visited.Type) || visited.Operand.Type == visited.Type)
                {
                    // x is not a collection of entity types
                    if(!(ClientType.CheckElementTypeIsEntity(visited.Operand.Type) && ProjectionAnalyzer.IsCollectionProducingExpression(visited.Operand)))
                    {
                        result = visited.Operand;
                    }
                }
            }

            return result;
        }

        /// <summary>
        /// CODE: x
        /// ORIGINAL: Convert(x, typeof(object))
        /// ORIGINAL(Funcletized): Constant(x, typeof(object))
        /// NORMALIZED: x
        /// </summary>
        private static Expression UnwrapObjectConvert(Expression input)
        {
            // recognize funcletized (already evaluated) Converts
            if (input.NodeType == ExpressionType.Constant && input.Type == typeof(object))
            {
                ConstantExpression constant = (ConstantExpression)input;

                // we will handle nulls later, so just bypass those
                if (constant.Value != null &&
                    constant.Value.GetType() != typeof(object))
                {
                    return Expression.Constant(constant.Value, constant.Value.GetType());
                }
            }

            // unwrap object converts
            while (ExpressionType.Convert == input.NodeType && typeof(object) == input.Type)
            {
                input = ((UnaryExpression)input).Operand;
            }

            return input;
        }

        /// <summary>
        /// Returns true if the given expression is a constant '0'.
        /// </summary>
        private static bool IsConstantZero(Expression expression)
        {
            return expression.NodeType == ExpressionType.Constant &&
                ((ConstantExpression)expression).Value.Equals(0);
        }

        /// <summary>
        /// Handles MethodCall patterns:
        /// 
        /// - Operator overloads
        /// - VB operators
        /// </summary>
        internal override Expression VisitMethodCall(MethodCallExpression call)
        {
            Expression visited = this.VisitMethodCallNoRewrite(call);
            this.RecordRewrite(call, visited);
            return visited;
        }

        /// <summary>
        /// Handles MethodCall patterns (without recording rewrites):
        /// 
        /// - Operator overloads
        /// - VB operators
        /// </summary>
        internal Expression VisitMethodCallNoRewrite(MethodCallExpression call)
        {
            MethodCallExpression visited = (MethodCallExpression)base.VisitMethodCall(call);

            // handle operator overloads
            if (visited.Method.IsStatic && visited.Method.Name.StartsWith("op_", StringComparison.Ordinal))
            {
                // handle binary operator overloads
                if (visited.Arguments.Count == 2)
                {
                    // CODE(C#): x == y
                    // ORIGINAL: MethodCallExpression(<op_Equality>, x, y)
                    // NORMALIZED: Equal(x, y)
                    switch (visited.Method.Name)
                    {
                        case "op_Equality":
                            return Expression.Equal(visited.Arguments[0], visited.Arguments[1], LiftToNull, visited.Method);

                        case "op_Inequality":
                            return Expression.NotEqual(visited.Arguments[0], visited.Arguments[1], LiftToNull, visited.Method);

                        case "op_GreaterThan":
                            return Expression.GreaterThan(visited.Arguments[0], visited.Arguments[1], LiftToNull, visited.Method);

                        case "op_GreaterThanOrEqual":
                            return Expression.GreaterThanOrEqual(visited.Arguments[0], visited.Arguments[1], LiftToNull, visited.Method);

                        case "op_LessThan":
                            return Expression.LessThan(visited.Arguments[0], visited.Arguments[1], LiftToNull, visited.Method);

                        case "op_LessThanOrEqual":
                            return Expression.LessThanOrEqual(visited.Arguments[0], visited.Arguments[1], LiftToNull, visited.Method);

                        case "op_Multiply":
                            return Expression.Multiply(visited.Arguments[0], visited.Arguments[1], visited.Method);

                        case "op_Subtraction":
                            return Expression.Subtract(visited.Arguments[0], visited.Arguments[1], visited.Method);

                        case "op_Addition":
                            return Expression.Add(visited.Arguments[0], visited.Arguments[1], visited.Method);

                        case "op_Division":
                            return Expression.Divide(visited.Arguments[0], visited.Arguments[1], visited.Method);

                        case "op_Modulus":
                            return Expression.Modulo(visited.Arguments[0], visited.Arguments[1], visited.Method);

                        case "op_BitwiseAnd":
                            return Expression.And(visited.Arguments[0], visited.Arguments[1], visited.Method);

                        case "op_BitwiseOr":
                            return Expression.Or(visited.Arguments[0], visited.Arguments[1], visited.Method);

                        case "op_ExclusiveOr":
                            return Expression.ExclusiveOr(visited.Arguments[0], visited.Arguments[1], visited.Method);

                        default:
                            break;
                    }
                }

                // handle unary operator overloads
                if (visited.Arguments.Count == 1)
                {
                    // CODE(C#): +x
                    // ORIGINAL: MethodCallExpression(<op_UnaryPlus>, x)
                    // NORMALIZED: UnaryPlus(x)
                    switch (visited.Method.Name)
                    {
                        case "op_UnaryNegation":
                            return Expression.Negate(visited.Arguments[0], visited.Method);

                        case "op_UnaryPlus":
                            return Expression.UnaryPlus(visited.Arguments[0], visited.Method);

                        case "op_Explicit":
                        case "op_Implicit":
                            return Expression.Convert(visited.Arguments[0], visited.Type, visited.Method);

                        case "op_OnesComplement":
                        case "op_False":
                            return Expression.Not(visited.Arguments[0], visited.Method);

                        default:
                            break;
                    }
                }
            }

            // check for static Equals method
            if (visited.Method.IsStatic && visited.Method.Name == "Equals" && visited.Arguments.Count > 1)
            {
                // CODE(C#): Object.Equals(x, y)
                // ORIGINAL: MethodCallExpression(<object.Equals>, x, y)
                // NORMALIZED: Equal(x, y)
                return Expression.Equal(visited.Arguments[0], visited.Arguments[1], false, visited.Method);
            }

            // check for instance Equals method
            if (!visited.Method.IsStatic && visited.Method.Name == "Equals" && visited.Arguments.Count > 0)
            {
                // CODE(C#): x.Equals(y)
                // ORIGINAL: MethodCallExpression(x, <Equals>, y)
                // NORMALIZED: Equal(x, y)
                return CreateRelationalOperator(ExpressionType.Equal, visited.Object, visited.Arguments[0]);
            }

            // check for Microsoft.VisualBasic.CompilerServices.Operators.CompareString method
            if (visited.Method.IsStatic && visited.Method.Name == "CompareString" && visited.Method.DeclaringType.FullName == "Microsoft.VisualBasic.CompilerServices.Operators")
            {
                // CODE(VB): x = y; where x and y are strings, a part of the expression looks like:
                // ORIGINAL: MethodCallExpression(Microsoft.VisualBasic.CompilerServices.Operators.CompareString(x, y, False)
                // NORMALIZED: see CreateCompareExpression method
                return CreateCompareExpression(visited.Arguments[0], visited.Arguments[1]);
            }

            // check for instance CompareTo method
            if (!visited.Method.IsStatic && visited.Method.Name == "CompareTo" && visited.Arguments.Count == 1 && visited.Method.ReturnType == typeof(int))
            {
                // CODE(C#): x.CompareTo(y)
                // ORIGINAL: MethodCallExpression(x.CompareTo(y))
                // NORMALIZED: see CreateCompareExpression method
                return CreateCompareExpression(visited.Object, visited.Arguments[0]);
            }

            // check for static Compare method
            if (visited.Method.IsStatic && visited.Method.Name == "Compare" && visited.Arguments.Count > 1 && visited.Method.ReturnType == typeof(int))
            {
                // CODE(C#): Class.Compare(x, y)
                // ORIGINAL: MethodCallExpression(<Compare>, x, y)
                // NORMALIZED: see CreateCompareExpression method
                return CreateCompareExpression(visited.Arguments[0], visited.Arguments[1]);
            }

            // check for coalesce operators added by the VB compiler to predicate arguments
            MethodCallExpression normalizedResult = NormalizePredicateArgument(visited);

            return normalizedResult;
        }

        /// <summary>
        /// Identifies and normalizes any predicate argument in the given call expression. If no changes
        /// are needed, returns the existing expression. Otherwise, returns a new call expression
        /// with a normalized predicate argument.
        /// </summary>
        private static MethodCallExpression NormalizePredicateArgument(MethodCallExpression callExpression)
        {
            MethodCallExpression result;

            int argumentOrdinal;
            Expression normalizedArgument;
            if (HasPredicateArgument(callExpression, out argumentOrdinal) &&
                TryMatchCoalescePattern(callExpression.Arguments[argumentOrdinal], out normalizedArgument))
            {
                List<Expression> normalizedArguments = new List<Expression>(callExpression.Arguments);

                // replace the predicate argument with the normalized version
                normalizedArguments[argumentOrdinal] = normalizedArgument;

                result = Expression.Call(callExpression.Object, callExpression.Method, normalizedArguments);
            }
            else
            {
                // nothing has changed
                result = callExpression;
            }

            return result;
        }

        /// <summary>
        /// Determines whether the given call expression has a 'predicate' argument (e.g. Where(source, predicate)) 
        /// and returns the ordinal for the predicate.
        /// </summary>
        /// <remarks>
        /// Obviously this method will need to be replaced if we ever encounter a method with multiple predicates.
        /// </remarks>
        private static bool HasPredicateArgument(MethodCallExpression callExpression, out int argumentOrdinal)
        {
            argumentOrdinal = default(int);
            bool result = false;

            // It turns out all supported methods taking a predicate argument have it as the second
            // argument. As a result, we always set argumentOrdinal to 1 when there is a match and
            // we can safely ignore all methods taking fewer than 2 arguments
            SequenceMethod sequenceMethod;
            if (2 <= callExpression.Arguments.Count &&
                ReflectionUtil.TryIdentifySequenceMethod(callExpression.Method, out sequenceMethod))
            {
                switch (sequenceMethod)
                {
                    case SequenceMethod.FirstPredicate:
                    case SequenceMethod.FirstOrDefaultPredicate:
                    case SequenceMethod.SinglePredicate:
                    case SequenceMethod.SingleOrDefaultPredicate:
                    case SequenceMethod.LastPredicate:
                    case SequenceMethod.LastOrDefaultPredicate:
                    case SequenceMethod.Where:
                    case SequenceMethod.WhereOrdinal:
                    case SequenceMethod.CountPredicate:
                    case SequenceMethod.LongCountPredicate:
                    case SequenceMethod.AnyPredicate:
                    case SequenceMethod.All:
                    case SequenceMethod.SkipWhile:
                    case SequenceMethod.SkipWhileOrdinal:
                    case SequenceMethod.TakeWhile:
                    case SequenceMethod.TakeWhileOrdinal:
                        argumentOrdinal = 1; // the second argument is always the one
                        result = true;
                        break;
                }
            }

            return result;
        }

        /// <summary>
        /// Determines whether the given expression of the form Lambda(Coalesce(left, Constant(false)), ...), a pattern
        /// introduced by the VB compiler for predicate arguments. Returns the 'normalized' version of the expression
        /// Lambda((bool)left, ...)
        /// </summary>
        private static bool TryMatchCoalescePattern(Expression expression, out Expression normalized)
        {
            normalized = null;
            bool result = false;

            if (expression.NodeType == ExpressionType.Quote)
            {
                // try to normalize the quoted expression
                UnaryExpression quote = (UnaryExpression)expression;
                if (TryMatchCoalescePattern(quote.Operand, out normalized))
                {
                    result = true;
                    normalized = Expression.Quote(normalized);
                }
            }
            else if (expression.NodeType == ExpressionType.Lambda)
            {
                LambdaExpression lambda = (LambdaExpression)expression;

                // collapse coalesce lambda expressions
                // CODE(VB): where a.NullableInt = 1
                // ORIGINAL: Lambda(Coalesce(expr, Constant(false)), a)
                // NORMALIZED: Lambda(expr, a)
                if (lambda.Body.NodeType == ExpressionType.Coalesce && lambda.Body.Type == typeof(bool))
                {
                    BinaryExpression coalesce = (BinaryExpression)lambda.Body;
                    if (coalesce.Right.NodeType == ExpressionType.Constant && false.Equals(((ConstantExpression)coalesce.Right).Value))
                    {
                        normalized = Expression.Lambda(lambda.Type, Expression.Convert(coalesce.Left, typeof(bool)), lambda.Parameters);
                        result = true;
                    }
                }
            }

            return result;
        }

        private static readonly MethodInfo s_relationalOperatorPlaceholderMethod = typeof(ExpressionNormalizer).GetMethod("RelationalOperatorPlaceholder", BindingFlags.Static | BindingFlags.NonPublic);

        /// <summary>
        /// This method exists solely to support creation of valid relational operator LINQ expressions that are not natively supported
        /// by the CLR (e.g. String > String). This method must not be invoked.
        /// </summary>
        private static bool RelationalOperatorPlaceholder<TLeft, TRight>(TLeft left, TRight right)
        {
            Debug.Assert(false, "This method should never be called. It exists merely to support creation of relational LINQ expressions.");
            return object.ReferenceEquals(left, right);
        }

        /// <summary>
        /// Create an operator relating 'left' and 'right' given a relational operator.
        /// </summary>
        private static BinaryExpression CreateRelationalOperator(ExpressionType op, Expression left, Expression right)
        {
            BinaryExpression result;
            if (!TryCreateRelationalOperator(op, left, right, out result))
            {
                Debug.Assert(false, "CreateRelationalOperator has unknown op " + op);
            }

            return result;
        }

        /// <summary>
        /// Try to create an operator relating 'left' and 'right' using the given operator. If the given operator
        /// does not define a known relation, returns false.
        /// </summary>
        private static bool TryCreateRelationalOperator(ExpressionType op, Expression left, Expression right, out BinaryExpression result)
        {
            MethodInfo relationalOperatorPlaceholderMethod = s_relationalOperatorPlaceholderMethod.MakeGenericMethod(left.Type, right.Type);

            switch (op)
            {
                case ExpressionType.Equal:
                    result = Expression.Equal(left, right, LiftToNull, relationalOperatorPlaceholderMethod);
                    return true;

                case ExpressionType.NotEqual:
                    result = Expression.NotEqual(left, right, LiftToNull, relationalOperatorPlaceholderMethod);
                    return true;

                case ExpressionType.LessThan:
                    result = Expression.LessThan(left, right, LiftToNull, relationalOperatorPlaceholderMethod);
                    return true;

                case ExpressionType.LessThanOrEqual:
                    result = Expression.LessThanOrEqual(left, right, LiftToNull, relationalOperatorPlaceholderMethod);
                    return true;

                case ExpressionType.GreaterThan:
                    result = Expression.GreaterThan(left, right, LiftToNull, relationalOperatorPlaceholderMethod);
                    return true;

                case ExpressionType.GreaterThanOrEqual:
                    result = Expression.GreaterThanOrEqual(left, right, LiftToNull, relationalOperatorPlaceholderMethod);
                    return true;

                default:
                    result = null;
                    return false;
            }
        }

        /// <summary>
        /// CODE(C#): Class.Compare(left, right)
        /// ORIGINAL: MethodCallExpression(Compare, left, right)
        /// NORMALIZED: Condition(Equal(left, right), 0, Condition(left > right, 1, -1))
        /// 
        /// Why is this an improvement? We know how to evaluate Condition in the store, but we don't
        /// know how to evaluate MethodCallExpression... Where the CompareTo appears within a larger expression,
        /// e.g. left.CompareTo(right) > 0, we can further simplify to left > right (we register the "ComparePattern"
        /// to make this possible).
        /// </summary>
        private Expression CreateCompareExpression(Expression left, Expression right)
        {
            Expression result = Expression.Condition(
                CreateRelationalOperator(ExpressionType.Equal, left, right),
                Expression.Constant(0),
                Expression.Condition(
                    CreateRelationalOperator(ExpressionType.GreaterThan, left, right),
                    Expression.Constant(1),
                    Expression.Constant(-1)));

            // Remember that this node matches the pattern
            _patterns[result] = new ComparePattern(left, right);

            return result;
        }

        /// <summary>Records a rewritten expression as necessary.</summary>
        /// <param name="source">Original source expression.</param>
        /// <param name="rewritten">Rewritten expression.</param>
        /// <remarks>
        /// IMPORTANT: if there are higher-level rewrites such as replacing parameter
        /// references, the lower-level rewrites will become un-doable in other
        /// contexts; we will have to change normalization/de-normalization strategy,
        /// record additional mapping information and/or bubble up the rewrite
        /// tracking.
        /// </remarks>
        private void RecordRewrite(Expression source, Expression rewritten)
        {
            Debug.Assert(source != null, "source != null");
            Debug.Assert(rewritten != null, "rewritten != null");
            Debug.Assert(this.NormalizerRewrites != null, "this.NormalizerRewrites != null");

            if (source != rewritten)
            {
                this.NormalizerRewrites.Add(rewritten, source);
            }
        }

        #region Inner types.

        /// <summary>
        /// Encapsulates an expression matching some pattern.
        /// </summary>
        private abstract class Pattern
        {
            /// <summary>
            /// Gets pattern kind.
            /// </summary>
            internal abstract PatternKind Kind { get; }
        }

        /// <summary>
        /// Gets pattern kind.
        /// </summary>
        private enum PatternKind
        {
            Compare,
        }

        /// <summary>
        /// Matches expression of the form x.CompareTo(y) or Class.CompareTo(x, y)
        /// </summary>
        private sealed class ComparePattern : Pattern
        {
            internal ComparePattern(Expression left, Expression right)
            {
                this.Left = left;
                this.Right = right;
            }

            /// <summary>
            /// Gets left-hand argument to Compare operation.
            /// </summary>
            internal readonly Expression Left;

            /// <summary>
            /// Gets right-hand argument to Compare operation.
            /// </summary>
            internal readonly Expression Right;


            internal override PatternKind Kind
            {
                get { return PatternKind.Compare; }
            }
        }

        #endregion Inner types.
    }
}
