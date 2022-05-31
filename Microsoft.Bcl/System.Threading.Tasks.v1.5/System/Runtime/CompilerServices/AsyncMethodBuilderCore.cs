using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Contracts;
using System.Linq;
using System.Security;
using System.Text;
using System.Threading;

namespace System.Runtime.CompilerServices
{
    /// <summary>Holds state related to the builder's IAsyncStateMachine.</summary>
    /// <remarks>This is a mutable struct.  Be very delicate with it.</remarks>
    internal struct AsyncMethodBuilderCore
    {
        /// <summary>A reference to the heap-allocated state machine object associated with this builder.</summary>
        internal IAsyncStateMachine m_stateMachine;

        /// <summary>Initiates the builder's execution with the associated state machine.</summary>
        /// <typeparam name="TStateMachine">Specifies the type of the state machine.</typeparam>
        /// <param name="stateMachine">The state machine instance, passed by reference.</param>
        /// <exception cref="System.ArgumentNullException">The <paramref name="stateMachine"/> argument is null (Nothing in Visual Basic).</exception>
#if !SILVERLIGHT
        // [SecuritySafeCritical]
#endif
        [DebuggerStepThrough]
        internal void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
        {
            if (stateMachine == null) throw new ArgumentNullException("stateMachine");
            Contract.EndContractBlock();

            stateMachine.MoveNext();
        }

        /// <summary>Associates the builder with the state machine it represents.</summary>
        /// <param name="stateMachine">The heap-allocated state machine object.</param>
        /// <exception cref="System.ArgumentNullException">The <paramref name="stateMachine"/> argument was null (Nothing in Visual Basic).</exception>
        /// <exception cref="System.InvalidOperationException">The builder is incorrectly initialized.</exception>
        public void SetStateMachine(IAsyncStateMachine stateMachine)
        {
            if (stateMachine == null) throw new ArgumentNullException("stateMachine");
            Contract.EndContractBlock();
            if (m_stateMachine != null) throw new InvalidOperationException("The builder was not properly initialized.");
            m_stateMachine = stateMachine;
        }

        /// <summary>
        /// Gets the Action to use with an awaiter's OnCompleted or UnsafeOnCompleted method.
        /// On first invocation, the supplied state machine will be boxed.
        /// </summary>
        /// <typeparam name="TMethodBuilder">Specifies the type of the method builder used.</typeparam>
        /// <typeparam name="TStateMachine">Specifies the type of the state machine used.</typeparam>
        /// <param name="builder">The builder.</param>
        /// <param name="stateMachine">The state machine.</param>
        /// <returns>An Action to provide to the awaiter.</returns>
#if !SILVERLIGHT
        // [SecuritySafeCritical]
#endif
        internal Action GetCompletionAction<TMethodBuilder, TStateMachine>(
            ref TMethodBuilder builder, ref TStateMachine stateMachine)
            where TMethodBuilder : IAsyncMethodBuilder
            where TStateMachine : IAsyncStateMachine
        {
            Contract.Requires(builder != null, "Expected valid builder");
            Contract.Requires(stateMachine != null, "Expected valid state machine reference");

            // The builder needs to flow ExecutionContext, so capture it.
            var capturedContext = ExecutionContextLightup.Instance.Capture();

            MoveNextRunner runner = new MoveNextRunner(capturedContext);
            Action action = new Action(runner.Run);

            // If this is our first await, such that we've not yet boxed the state machine, do so now.
            if (m_stateMachine == null)
            {
                // This is our first await, and we're not boxed yet.  First performance any work that
                // must affect both the non-boxed and boxed builders.
                builder.PreBoxInitialization();

                // Box the state machine, then tell the boxed instance to call back into its own builder,
                // so we can cache the boxed reference.
                Contract.Assert(!Object.ReferenceEquals((object)stateMachine, (object)stateMachine), "Expected an unboxed state machine reference");
                m_stateMachine = (IAsyncStateMachine)stateMachine;
                m_stateMachine.SetStateMachine(m_stateMachine);
            }

            Contract.Assert(runner.m_stateMachine == null, "The runner's state machine should not yet have been populated.");
            Contract.Assert(m_stateMachine != null, "The builder's state machine field should have been initialized.");

            // Now that we have the state machine, store it into the runner that the action delegate points to.
            // And return the action.
            runner.m_stateMachine = m_stateMachine; // only after this line is the Action delegate usable
            return action;
        }

       

        /// <summary>Provides the ability to invoke a state machine's MoveNext method under a supplied ExecutionContext.</summary>
        private sealed class MoveNextRunner
        {
            /// <summary>The context with which to run MoveNext.</summary>
            private readonly ExecutionContextLightup m_context;
            /// <summary>The state machine whose MoveNext method should be invoked.</summary>
            internal IAsyncStateMachine m_stateMachine;

            /// <summary>Initializes the runner.</summary>
            /// <param name="context">The context with which to run MoveNext.</param>
            internal MoveNextRunner(ExecutionContextLightup context)
            {
                m_context = context;
            }

            /// <summary>Invokes MoveNext under the provided context.</summary>
            internal void Run()
            {
                Contract.Assert(m_stateMachine != null, "The state machine must have been set before calling Run.");

                if (m_context != null)
                {
                    try
                    {
                        // Get the callback, lazily initializing it as necessary
                        Action<object> callback = s_invokeMoveNext;
                        if (callback == null) { s_invokeMoveNext = callback = InvokeMoveNext; }

                        if (m_context == null)
                        {
                            callback(m_stateMachine);
                        }
                        else
                        {
                            // Use the context and callback to invoke m_stateMachine.MoveNext.
                            ExecutionContextLightup.Instance.Run(m_context, callback, m_stateMachine);
                        }
                    }
                    finally { if (m_context != null) m_context.Dispose(); }
                }
                else
                {
                    m_stateMachine.MoveNext();
                }
            }

            /// <summary>Cached delegate used with ExecutionContext.Run.</summary>
            private static Action<object> s_invokeMoveNext; // lazily-initialized due to SecurityCritical attribution

            /// <summary>Invokes the MoveNext method on the supplied IAsyncStateMachine.</summary>
            /// <param name="stateMachine">The IAsyncStateMachine machine instance.</param>
            private static void InvokeMoveNext(object stateMachine)
            {
                ((IAsyncStateMachine)stateMachine).MoveNext();
            }
        }
    }
}
