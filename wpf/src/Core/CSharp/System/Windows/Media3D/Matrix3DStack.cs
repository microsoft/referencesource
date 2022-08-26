//---------------------------------------------------------------------------
//
// <copyright file=Matrix3DStack.cs company=Microsoft>
//    Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: This is a super simple Matrix3DStack implementation.
//              MatrixStack (2D) is optimized to avoid boxig and copying
//              of structs.  This was written as a stop-gap to address
//              a bug until we can use CodeGen here.
//              
// History:  
//  1/19/2004 : Microsoft - Created
//
//---------------------------------------------------------------------------

using System;
using System.Collections;
using System.Collections.Generic;

namespace System.Windows.Media.Media3D
{
    // 




    internal class Matrix3DStack
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------
        
        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        public void Clear()
        {
            _stack.Clear();
        }

        public Matrix3D Pop()
        {
            Matrix3D top = Top;
            _stack.RemoveAt(_stack.Count - 1);
            return top;
        }

        /// <summary>
        /// Empty => [matrix]
        /// tail | [top] => tail | [top] | [matrix * top]
        /// </summary>
        public void Push(Matrix3D matrix)
        {
            if (_stack.Count > 0)
            {
                matrix.Append(Top);
            }
            
            _stack.Add(matrix);
        }
        
        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------

        public int Count
        {
            get
            {
                return _stack.Count;
            }
        }

        public bool IsEmpty
        {
            get
            {
                return (_stack.Count == 0);
            }
        }

        public Matrix3D Top
        {
            get
            {
                return _stack[_stack.Count - 1];
            }
        }
        
        //------------------------------------------------------
        //
        //  Public Events
        //
        //------------------------------------------------------

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------
        
        #region Private Fields
        
        private readonly List<Matrix3D> _stack = new List<Matrix3D>();
        
        #endregion Private Fields
        
    }
}

