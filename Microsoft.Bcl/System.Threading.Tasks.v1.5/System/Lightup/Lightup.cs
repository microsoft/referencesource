using System;
using System.Linq;
using System.Reflection;
using System.Threading;

namespace System
{
    internal abstract class Lightup
    {
        private readonly static Type[] EmptyTypes = new Type[0];
        private readonly Type _type;

        protected Lightup(Type type)
        {
            _type = type;
        }

        protected bool TryGet<T>(ref Delegate storage, string propertyName, out T value)
        {
            return TryCall(ref storage, "get_" + propertyName, out value);
        }

        protected T Get<T>(ref Delegate storage, string propertyName)
        {
            return Call<T>(ref storage, "get_" + propertyName);
        }

        protected void Set<T>(ref Delegate storage, string propertyName, T value)
        {
            Call(ref storage, "set_" + propertyName, value);
        }

        protected void Set<TI, TV>(ref Delegate storage, TI instance, string propertyName, TV value)
        {
            Call(ref storage, instance, "set_" + propertyName, value);
        }

        protected bool TrySet<TI, TV>(ref Delegate storage, TI instance, string propertyName, TV value)
        {
            return TryCall(ref storage, instance, "set_" + propertyName, value);
        }

        protected bool TryCall<T>(ref Delegate storage, string methodName, out T returnValue)
        {
            Func<T> method = GetMethodAccessor<Func<T>>(ref storage, methodName);
            if (method == null)
            {
                returnValue = default(T);
                return false;
            }

            returnValue = method();
            return true;
        }

        protected T Call<T>(ref Delegate storage, string methodName)
        {
            Func<T> method = GetMethodAccessor<Func<T>>(ref storage, methodName);
            if (method == null)
                throw new InvalidOperationException();

            return method();
        }

        protected void Call(ref Delegate storage, string methodName)
        {
            Action method = GetMethodAccessor<Action>(ref storage, methodName);
            if (method == null)
                throw new InvalidOperationException();

            method();
        }

        protected bool TryCall<TI, TV>(ref Delegate storage, TI instance, string methodName, TV parameter)
        {
            Action<TI, TV> method = GetMethodAccessor<Action<TI, TV>>(ref storage, methodName, bindInstance:false);
            if (method == null)
                return false;

            method(instance, parameter);
            return true;
        }

        protected bool TryCall<TI, TV1, TV2>(ref Delegate storage, TI instance, string methodName, TV1 parameter1, TV2 parameter2)
        {
            Action<TI, TV1, TV2> method = GetMethodAccessor<Action<TI, TV1, TV2>>(ref storage, methodName, bindInstance: false);
            if (method == null)
                return false;

            method(instance, parameter1, parameter2);
            return true;
        }

        protected void Call<TI, TV>(ref Delegate storage, TI instance, string methodName, TV parameter)
        {
            Action<TI, TV> method = GetMethodAccessor<Action<TI, TV>>(ref storage, methodName, bindInstance:false);
            if (method == null)
                throw new InvalidOperationException();

            method(instance, parameter);
        }

        protected void Call<T>(ref Delegate storage, string methodName, T parameter)
        {
            Action<T> method = GetMethodAccessor<Action<T>>(ref storage, methodName);
            if (method == null)
                throw new InvalidOperationException();

            method(parameter);
        }

        protected static T Create<T>(params object[] parameters)
        {
            Type[] argumentTypes = parameters.Select(p => p.GetType())
                                             .ToArray();

            Func<object[], T> activator = CreateActivator<T>(argumentTypes);

            return activator(parameters);
        }

        protected abstract object GetInstance();

        private static Func<object[], T> CreateActivator<T>(Type[] argumentTypes)
        {
            ConstructorInfo info = typeof(T).GetConstructor(argumentTypes);
            if (info == null)
                return null;

            Func<object[], T> activator = (arguments) => (T)Activator.CreateInstance(typeof(T), arguments);

            return activator;
        }

        private Delegate CreateMethodAccessor(Type type, string name, bool bindInstance = true)
        {
            if (_type == null)
                return null;

            Type[] argumentTypes = LightupServices.GetMethodArgumentTypes(type, bindInstance);

            MethodInfo method = _type.GetMethod(name, argumentTypes);
            if (method == null)
                return null;
            
            return LightupServices.CreateDelegate(type, bindInstance ? GetInstance() : null, method);
        }

        protected T GetMethodAccessor<T>(ref Delegate storage, string name, bool bindInstance = true)
        {
            return (T)(object)GetMethodAccessor(ref storage, typeof(T), name, bindInstance);
        }

        protected Delegate GetMethodAccessor(ref Delegate storage, Type type, string name, bool bindInstance = true)
        {
            if (storage == null)
            {
                Delegate accessor = CreateMethodAccessor(type, name, bindInstance);

                Interlocked.CompareExchange(ref storage, accessor, null);
            }

            return storage == LightupServices.NotFound ? null : storage;
        }
    }

    internal static class LightupServices
    {
        public static Delegate NotFound = new Action(() => { });

        public static Delegate ReplaceWith(Delegate d, Type delegateType)
        {
            return Delegate.CreateDelegate(delegateType, d.Target, d.Method);
        }
        
        public static Type[] GetMethodArgumentTypes(Type actionOrFuncType, bool bindInstance = true)
        {
            Type[] arguments = actionOrFuncType.GetGenericArguments();

            if (!bindInstance)
            {
                // We aren't binding the instance, remove "this" argument
                arguments = arguments.Skip(1).ToArray();
            }

            if (IsActionType(actionOrFuncType))
                return arguments;

            // We have a Func, remove it's trailing return type
            return arguments.Take(arguments.Length -1).ToArray();
        }

        public static bool IsActionType(Type type)
        {
            if (type.IsGenericType)
                type = type.GetGenericTypeDefinition();

            return type == typeof(Action) || type == typeof(Action<>) || type == typeof(Action<,>) || type == typeof(Action<,,>) || type == typeof(Action<,,,>);
        }

        public static Delegate CreateDelegate(Type type, object instance, MethodInfo method)
        {
            if (method.IsStatic)
                instance = null;

            try
            {
                return Delegate.CreateDelegate(type, instance, method);
            }
            catch (InvalidOperationException)
            {   // Exists, but not callable
            }
            catch (MemberAccessException)
            {   // Exists, but don't have required access
            }

            return NotFound;
        }
    }
}
