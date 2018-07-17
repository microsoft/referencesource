// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    /*
    // ===========================================================================
       Defines structs that package an aggregate member together with
       generic type argument information.
    // ===========================================================================*/
    /******************************************************************************
        SymWithType and its cousins. These package an aggregate member (field,
        prop, event, or meth) together with the particular instantiation of the
        aggregate (the AggregateType).
     
        The default constructor does nothing so these are not safe to use
        uninitialized. Note that when they are used as member of an EXPR they
        are automatically zero filled by newExpr.
    ******************************************************************************/
    internal class SymWithType
    {
        AggregateType ats;
        Symbol sym;

        public SymWithType()
        {
        }

        public SymWithType(Symbol sym, AggregateType ats)
        {
            Set(sym, ats);
        }

        public virtual void Clear()
        {
            this.sym = null;
            this.ats = null;
        }

        public AggregateType Ats
        {
            get { return this.ats; }
        }

        public Symbol Sym
        {
            get { return this.sym; }
        }

        public new AggregateType GetType()
        {
            // This conflicts with object.GetType.  Turn every usage of this
            // into a get on Ats.
            return Ats;
        }

        public static bool operator ==(SymWithType swt1, SymWithType swt2)
        {
            if (object.ReferenceEquals(swt1, swt2))
            {
                return true;
            }
            else if (object.ReferenceEquals(swt1, null))
            {
                return swt2.sym == null;
            }
            else if (object.ReferenceEquals(swt2, null))
            {
                return swt1.sym == null;
            }
            return swt1.Sym == swt2.Sym && swt1.Ats == swt2.Ats;
        }

        public static bool operator !=(SymWithType swt1, SymWithType swt2)
        {
            if (object.ReferenceEquals(swt1, swt2))
            {
                return false;
            }
            else if (object.ReferenceEquals(swt1, null))
            {
                return swt2.sym != null;
            }
            else if (object.ReferenceEquals(swt2, null))
            {
                return swt1.sym != null;
            }
            return swt1.Sym != swt2.Sym || swt1.Ats != swt2.Ats;
        }

        public override bool Equals(object obj)
        {
            SymWithType other = obj as SymWithType;
            if (other == null) return false;
            return this.Sym == other.Sym && this.Ats == other.Ats;
        }

        public override int GetHashCode()
        {
            return (this.Sym != null ? this.Sym.GetHashCode() : 0) +
                (this.Ats != null ? this.Ats.GetHashCode() : 0);
        }

        // The SymWithType is considered NULL iff the Symbol is NULL.
        public static implicit operator bool(SymWithType swt)
        {
            return swt != null;
        }

        // These assert that the Symbol is of the correct type.
        public MethodOrPropertySymbol MethProp()
        {
            return this.Sym as MethodOrPropertySymbol;
        }

        public MethodSymbol Meth()
        {
            return this.Sym as MethodSymbol;
        }

        public PropertySymbol Prop()
        {
            return this.Sym as PropertySymbol;
        }

        public FieldSymbol Field()
        {
            return this.Sym as FieldSymbol;
        }

        public EventSymbol Event()
        {
            return this.Sym as EventSymbol;
        }

        public void Set(Symbol sym, AggregateType ats)
        {
            if (sym == null)
                ats = null;
            Debug.Assert(ats == null || sym.parent == ats.getAggregate());
            this.sym = sym;
            this.ats = ats;
        }
    }

    internal class MethPropWithType : SymWithType
    {
        public MethPropWithType()
        {
        }

        public MethPropWithType(MethodOrPropertySymbol mps, AggregateType ats)
        {
            Set(mps, ats);
        }
    }

    internal class MethWithType : MethPropWithType
    {
        public MethWithType()
        {
        }

        public MethWithType(MethodSymbol meth, AggregateType ats)
        {
            Set(meth, ats);
        }
    }

    internal class PropWithType : MethPropWithType
    {
        public PropWithType()
        { }

        public PropWithType(PropertySymbol prop, AggregateType ats)
        {
            Set(prop, ats);
        }

        public PropWithType(SymWithType swt)
        {
            Set(swt.Sym as PropertySymbol, swt.Ats);
        }
    }

    internal class EventWithType : SymWithType
    {
        public EventWithType()
        {
        }

        public EventWithType(EventSymbol @event, AggregateType ats)
        {
            Set(@event, ats);
        }
    }

    internal class FieldWithType : SymWithType
    {
        public FieldWithType()
        {
        }

        public FieldWithType(FieldSymbol field, AggregateType ats)
        {
            Set(field, ats);
        }
    }

    /******************************************************************************
        MethPropWithInst and MethWithInst. These extend MethPropWithType with
        the method type arguments. Properties will never have type args, but
        methods and properties share a lot of code so it's convenient to allow
        both here.
     
        The default constructor does nothing so these are not safe to use
        uninitialized. Note that when they are used as member of an EXPR they
        are automatically zero filled by newExpr.
    ******************************************************************************/

    internal class MethPropWithInst : MethPropWithType
    {
        public TypeArray TypeArgs { get; private set; }

        public MethPropWithInst()
        {
            Set(null, null, null);
        }

        public MethPropWithInst(MethodOrPropertySymbol mps, AggregateType ats)
            : this(mps, ats, null)
        {
        }

        public MethPropWithInst(MethodOrPropertySymbol mps, AggregateType ats, TypeArray typeArgs)
        {
            Set(mps, ats, typeArgs);
        }

        public override void Clear()
        {
            base.Clear();
            TypeArgs = null;
        }
#if false    
        bool operator ==(const MethPropWithInst & mpwi) const
        {
            return sym == mpwi.sym && ats == mpwi.ats && typeArgs == mpwi.typeArgs;
        }
        bool operator !=(const MethPropWithInst & mpwi) const
        {
            return sym != mpwi.sym || ats != mpwi.ats || typeArgs != mpwi.typeArgs;
        }
#endif

        public void Set(MethodOrPropertySymbol mps, AggregateType ats, TypeArray typeArgs)
        {
            if (mps == null)
            {
                ats = null;
                typeArgs = null;
            }
            Debug.Assert(ats == null || mps != null && mps.getClass() == ats.getAggregate());
#if false
            Debug.Assert(typeArgs == null || typeArgs.Size == 0 || mps != null && mps.IsMethodSymbol());
            Debug.Assert(typeArgs == null|| !mps.IsMethodSymbol() || mps.AsMethodSymbol().typeVars.Size == typeArgs.Size);
#endif
            base.Set(mps, ats);
            this.TypeArgs = typeArgs;
        }
    }

    internal class MethWithInst : MethPropWithInst
    {
        public MethWithInst()
        {
        }
        public MethWithInst(MethodSymbol meth, AggregateType ats)
            : this(meth, ats, null)
        {
        }
        public MethWithInst(MethodSymbol meth, AggregateType ats, TypeArray typeArgs)
        {
            Set(meth, ats, typeArgs);
        }
        public MethWithInst(MethPropWithInst mpwi)
        {
            Set(mpwi.Sym.AsMethodSymbol(), mpwi.Ats, mpwi.TypeArgs);
        }
    }
}
