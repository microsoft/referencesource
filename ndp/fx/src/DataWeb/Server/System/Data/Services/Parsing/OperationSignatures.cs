//---------------------------------------------------------------------
// <copyright file="OperationSignatures.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides types with operation signatures.
// </summary>
//
// @owner  mruiz
//---------------------------------------------------------------------

namespace System.Data.Services.Parsing
{
    using System;

    /// <summary>This class provides inner types with operation signatures.</summary>
    internal static class OperationSignatures
    {
        /// <summary>Signatures for logical operations.</summary>
        internal interface ILogicalSignatures
        {
            /// <summary>Logical signatures for bool arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(bool x, bool y);

            /// <summary>Logical signatures for bool? arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(bool? x, bool? y);

            /// <summary>Logical signatures for object arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(object x, object y);
        }

        /// <summary>Signatures for arithmetic operations.</summary>
        internal interface IArithmeticSignatures
        {
            /// <summary>Arithmetic signature for int arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(int x, int y);

            /// <summary>Arithmetic signature for long arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(long x, long y);

            /// <summary>Arithmetic signature for float arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(float x, float y);

            /// <summary>Arithmetic signature for double arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(double x, double y);

            /// <summary>Arithmetic signature for decimal arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(decimal x, decimal y);

            /// <summary>Arithmetic signature for int? arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(int? x, int? y);

            /// <summary>Arithmetic signature for long? arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(long? x, long? y);

            /// <summary>Arithmetic signature for float? arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(float? x, float? y);

            /// <summary>Arithmetic signature for double? arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(double? x, double? y);

            /// <summary>Arithmetic signature for decimal? arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(decimal? x, decimal? y);

            /// <summary>Arithmetic signature for object arguments.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(object x, object y);
        }

        /// <summary>Signatures for relational operations.</summary>
        internal interface IRelationalSignatures : IArithmeticSignatures
        {
            /// <summary>Relational operation signature.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(string x, string y);

            /// <summary>Relational operation signature.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(bool x, bool y);

            /// <summary>Relational operation signature.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(bool? x, bool? y);

            /// <summary>Relational operation signature.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(Guid x, Guid y);

            /// <summary>Relational operation signature.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(Guid? x, Guid? y);

            /// <summary>Relational operation signature.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(char x, char y);

            /// <summary>Relational operation signature.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(DateTime x, DateTime y);

            /// <summary>Relational operation signature.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(char? x, char? y);

            /// <summary>Relational operation signature.</summary>
            /// <param name="x">First argument.</param><param name="y">Second argument.</param>
            void F(DateTime? x, DateTime? y);
        }

        /// <summary>Signatures for equality operations.</summary>
        internal interface IEqualitySignatures : IRelationalSignatures
        {
        }

        /// <summary>Signatures for addition operations.</summary>
        internal interface IAddSignatures : IArithmeticSignatures
        {
        }

        /// <summary>Signatures for subtraction operations.</summary>
        internal interface ISubtractSignatures : IAddSignatures
        {
        }

        /// <summary>Signatures for negation operations.</summary>
        internal interface INegationSignatures
        {
            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(int x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(long x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(float x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(double x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(decimal x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(int? x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(long? x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(float? x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(double? x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(decimal? x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(object x);
        }

        /// <summary>Signatures for logical negation operations.</summary>
        internal interface INotSignatures
        {
            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(bool x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(bool? x);

            /// <summary>Negation operation signature.</summary>
            /// <param name="x">Argument.</param>
            void F(object x);
        }

        /// <summary>Signatures for enumerable operations.</summary>
        internal interface IEnumerableSignatures
        {
            /// <summary>Enumerable operation signature.</summary>
            /// <param name="predicate">Predicate.</param>
            void Where(bool predicate);

            /// <summary>Enumerable operation signature.</summary>
            void Any();

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="predicate">Predicate.</param>
            void Any(bool predicate);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="predicate">Predicate.</param>
            void All(bool predicate);

            /// <summary>Enumerable operation signature.</summary>
            void Count();

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="predicate">Predicate.</param>
            void Count(bool predicate);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Min(object selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Max(object selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Sum(int selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Sum(int? selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Sum(long selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Sum(long? selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Sum(float selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Sum(float? selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Sum(double selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Sum(double? selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Sum(decimal selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Sum(decimal? selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Average(int selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Average(int? selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Average(long selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Average(long? selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Average(float selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Average(float? selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Average(double selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Average(double? selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Average(decimal selector);

            /// <summary>Enumerable operation signature.</summary>
            /// <param name="selector">Selector.</param>
            void Average(decimal? selector);
        }
    }
}
