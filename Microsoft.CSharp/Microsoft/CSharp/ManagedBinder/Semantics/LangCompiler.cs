// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;
using Microsoft.CSharp.RuntimeBinder.Errors;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class LangCompiler :
         CSemanticChecker,
         IErrorSink
    {
        private SymbolLoader m_symbolLoader;
        private CController pController;   // This is our parent "controller"
        private ErrorHandling m_errorContext;
        private GlobalSymbolContext globalSymbolContext;
        private UserStringBuilder m_userStringBuilder;

        ////////////////////////////////////////////////////////////////////////////////
        // Construct a compiler. All the real work is done in the Init() routine. This 
        // primary initializes all the sub-components.

        public LangCompiler(CController pCtrl, NameManager pNameMgr)
        {
            Debug.Assert(pCtrl != null);

            pController = pCtrl;
            globalSymbolContext = new GlobalSymbolContext(pNameMgr);
            m_userStringBuilder = new UserStringBuilder(globalSymbolContext);
            m_errorContext = new ErrorHandling(m_userStringBuilder, this, pCtrl.GetErrorFactory());
            m_symbolLoader = new SymbolLoader(globalSymbolContext, null, m_errorContext);
        }

        public new ErrorHandling GetErrorContext()
        {
            return m_errorContext;
        }

        public override SymbolLoader SymbolLoader { get { return m_symbolLoader; } }
        public override SymbolLoader GetSymbolLoader() { return m_symbolLoader; }

        ////////////////////////////////////////////////////////////////////////////////
        // Searches the class [atsSearch] to see if it contains a method which is 
        // sufficient to implement [mwt]. Does not search base classes. [mwt] is 
        // typically a method in some interface.  We may be implementing this interface
        // at some particular type, e.g. IList<String>, and so the required signature is
        // the instantiation (i.e. substitution) of [mwt] for that instance. Similarly, 
        // the implementation may be provided by some base class that exists via
        // polymorphic inheritance, e.g. Foo : List<String>, and so we must instantiate
        // the parameters for each potential implementation. [atsSearch] may thus be an
        // instantiated type.
        //
        // If fOverride is true, this checks for a method with swtSlot set to the 
        // particular method.
        public void SubmitError(CParameterizedError error)
        {
            CError pError = GetErrorContext().RealizeError(error);

            if (pError == null)
            {
                return;
            }
            pController.SubmitError(pError);
        }

        public int ErrorCount()
        {
            return 0;
        }
    }
}
