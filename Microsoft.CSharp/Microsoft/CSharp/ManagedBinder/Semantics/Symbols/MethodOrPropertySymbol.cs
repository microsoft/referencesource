// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // MethodOrPropertySymbol
    //
    // MethodOrPropertySymbol - abstract class representing a method or a property. There
    // are a bunch of algorithms in the compiler (e.g., override and overload 
    // resolution) that want to treat methods and properties the same. This 
    // abstract base class has the common parts. 
    //
    // Changed to a ParentSymbol to allow generic methods to parent their type
    // variables.
    // ----------------------------------------------------------------------------

    class MethodOrPropertySymbol : ParentSymbol
    {
        public uint modOptCount;              // number of CMOD_OPTs in signature and return type

        public new bool isStatic;               // Static member?
        public bool isOverride;             // Overrides an inherited member. Only valid if isVirtual is set.
        // false implies that a new vtable slot is required for this method.
        public bool useMethInstead;         // Only valid iff isBogus == TRUE && IsPropertySymbol().
        // If this is true then tell the user to call the accessors directly.
        public bool isOperator;             // a user defined operator (or default indexed property)
        public bool isParamArray;           // new style varargs
        public bool isHideByName;           // this property hides all below it regardless of signature
        public List<Name> ParameterNames { get; private set; }
        private bool[] optionalParameterIndex;
        private bool[] defaultParameterIndex;
        private CONSTVAL[] defaultParameters;
        private CType[] defaultParameterConstValTypes;
        private bool[] dispatchConstantParameterIndex;
        private bool[] unknownConstantParameterIndex;
        private bool[] marshalAsIndex;
        private UnmanagedType[] marshalAsBuffer;
        
        // This indicates the base member that this member overrides or implements.
        // For an explicit interface member implementation, this is the interface member (and type)
        // that the member implements. For an override member, this is the base member that is
        // being overridden. This is not affected by implicit interface member implementation.
        // If this symbol is a property and an explicit interface member implementation, the swtSlot
        // may be an event. This is filled in during prepare.
        public SymWithType swtSlot;
        public ErrorType errExpImpl;          // If name == NULL but swtExpImpl couldn't be resolved, this contains error information.
        public CType RetType;            // Return type.

        private TypeArray _Params;
        public TypeArray Params
        {
            get
            {
                return _Params;
            }
            set
            {
                // Should only be set once!
                _Params = value;
                optionalParameterIndex = new bool[_Params.size];
                defaultParameterIndex = new bool[_Params.size];
                defaultParameters = new CONSTVAL[_Params.size];
                defaultParameterConstValTypes = new CType[_Params.size];
                dispatchConstantParameterIndex = new bool[_Params.size];
                unknownConstantParameterIndex = new bool[_Params.size];
                marshalAsIndex = new bool[_Params.size];
                marshalAsBuffer = new UnmanagedType[_Params.size];
            }
        }             // array of cParams parameter types.
        public AggregateDeclaration declaration;       // containing declaration
        public int MetadataToken;

        public MethodOrPropertySymbol()
        {
            ParameterNames = new List<Name>();
        }

        /////////////////////////////////////////////////////////////////////////////////

        public bool IsParameterOptional(int index)
        {
            Debug.Assert(index < Params.size);

            if (optionalParameterIndex == null)
            {
                return false;
            }
            return optionalParameterIndex[index];
        }

        public void SetOptionalParameter(int index)
        {
            Debug.Assert(optionalParameterIndex != null);
            optionalParameterIndex[index] = true;
        }

        public bool HasOptionalParameters()
        {
            if (optionalParameterIndex == null)
            {
                return false;
            }
            foreach (bool b in optionalParameterIndex)
            {
                if (b)
                {
                    return true;
                }
            }
            return false;
        }

        public bool HasDefaultParameterValue(int index)
        {
            Debug.Assert(index < Params.size);
            Debug.Assert(defaultParameterIndex != null);
            return defaultParameterIndex[index];
        }

        public void SetDefaultParameterValue(int index, CType type, CONSTVAL cv)
        {
            Debug.Assert(defaultParameterIndex != null);
            ConstValFactory factory = new ConstValFactory();
            defaultParameterIndex[index] = true;
            defaultParameters[index] = factory.Copy(type.constValKind(), cv);
            defaultParameterConstValTypes[index] = type;
        }

        public CONSTVAL GetDefaultParameterValue(int index)
        {
            Debug.Assert(HasDefaultParameterValue(index));
            Debug.Assert(defaultParameterIndex != null);
            return defaultParameters[index];
        }

        public CType GetDefaultParameterValueConstValType(int index)
        {
            Debug.Assert(HasDefaultParameterValue(index));
            return defaultParameterConstValTypes[index];
        }

        public bool IsMarshalAsParameter(int index)
        {
            return marshalAsIndex[index];
        }

        public void SetMarshalAsParameter(int index, UnmanagedType umt)
        {
            marshalAsIndex[index] = true;
            marshalAsBuffer[index] = umt;
        }

        public UnmanagedType GetMarshalAsParameterValue(int index)
        {
            Debug.Assert(IsMarshalAsParameter(index));
            return marshalAsBuffer[index];
        }

        public bool MarshalAsObject(int index)
        {
            UnmanagedType marshalAsType = default(UnmanagedType);

            if (IsMarshalAsParameter(index))
            {
                marshalAsType = GetMarshalAsParameterValue(index);
            }

#if SILVERLIGHT && ! FEATURE_NETCORE
            return marshalAsType == UnmanagedType.IUnknown;

#else
            return marshalAsType == UnmanagedType.Interface
                || marshalAsType == UnmanagedType.IUnknown
                || marshalAsType == UnmanagedType.IDispatch;
#endif
        }

        public bool IsDispatchConstantParameter(int index)
        {
            return dispatchConstantParameterIndex[index];
        }

        public void SetDispatchConstantParameter(int index)
        {
            dispatchConstantParameterIndex[index] = true;
        }

        public bool IsUnknownConstantParameter(int index)
        {
            return unknownConstantParameterIndex[index];
        }

        public void SetUnknownConstantParameter(int index)
        {
            unknownConstantParameterIndex[index] = true;
        }

        public AggregateSymbol getClass()
        {
            return parent.AsAggregateSymbol();
        }

        public bool IsExpImpl()
        {
            return name == null;
        }

        public AggregateDeclaration containingDeclaration()
        {
            return declaration;
        }
    }
}
