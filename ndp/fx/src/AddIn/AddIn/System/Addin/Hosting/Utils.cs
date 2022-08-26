// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Utils
**
** Purpose: Random functionality that is useful for the 
**    Add-In model
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections;
using System.Collections.ObjectModel;
using System.IO;
using System.Reflection;
using System.Security;
using System.Security.Permissions;
using System.Security.Policy;
using System.Text;
using System.Diagnostics;
using System.AddIn.MiniReflection;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{
    [Serializable]
    internal static class Utils
    {
        // This method requires that the types passed in are from the same CLR 
        // loader context.  The V2 loader defines 4 loader contexts, and only 2 of
        // them will exist in the CLR V3.  A type from an assembly loaded in one 
        // loader context is not assignable to the same type in a different loader
        // context.  This applies for normal assignment, but also shows up in 
        // Reflection, as getting different System.Type instances.  This method
        // wants to be relatively fast, so instead of doing string comparisons on
        // type names, we leverage Reflection's guarantees about singleton Type
        // instances for assemblies loaded in the same loader context.  That's why
        // the loop below can do a reference equality test.
        internal static bool HasCustomAttribute(Type attributeType, Type inspectType)
        {
            return GetCustomAttributeData(attributeType, inspectType) != null;
        }

        
        internal static CustomAttributeData GetCustomAttributeData(Type attributeType, Type inspectType)
        {
            // Spec#: Should this be a DebugRequires or RequiresExpensive?
            System.Diagnostics.Contracts.Contract.Requires(typeof(Attribute).IsAssignableFrom(attributeType));
            // The following precondition isn't strictly sufficient for the V2 CLR
            // because we define 4 loader contexts and expose only one boolean 
            // predicate for testing which loader context an assembly is loaded in.
            // But it is still very useful - our types must be in the same loader context.
            // 
            // Removed because sometimes we look for attributes on a type's Base type which may be Object
            // System.Diagnostics.Contracts.Contract.Requires(attributeType.Assembly.ReflectionOnly == inspectType.Assembly.ReflectionOnly);

            foreach (CustomAttributeData ca in CustomAttributeData.GetCustomAttributes(inspectType))
            {
                if (Object.ReferenceEquals(ca.Constructor.DeclaringType, attributeType))
                    return ca;
            }
            return null;
        }

        /*
        internal static bool PublicKeyTokensEqual(String token1, byte[] token2)
        {
            if (token2 == null || token2.Length == 0)
                return token1 == "null";
            if (token1 == "null")
                return false;
            Contract.Assert(token1.Length == 2 * token2.Length, "Lengths didn't match");
            for (int i = 0; i < token2.Length; i++)
            {
                int firstPart = (token1[2 * i] <= '9') ? token1[2 * i] - '0' : token1[2 * i] - 'a' + 10;
                int secondPart = (token1[2 * i + 1] <= '9') ? token1[2 * i + 1] - '0' : token1[2 * i + 1] - 'a' + 10;
                byte b1 = (byte)((firstPart << 4) + secondPart);
                if (b1 != token2[i])
                    return false;
            }
            return true;
        }
         */

        /*
        internal static bool PublicKeyTokensEqual(byte[] token1, byte[] token2)
        {
            if (token2 == null || token2.Length == 0)
                return token1 == null || token1.Length == 0;
            if (token1 == null)
                return false;
            System.Diagnostics.Contracts.Contract.Assert(token1.Length == token2.Length, "Lengths didn't match");
            for (int i = 0; i < token1.Length; i++)
                if (token1[i] != token2[i])
                    return false;
            return true;
        }
        */

        internal static bool PublicKeyMatches(AssemblyName a1, AssemblyName a2)
        {
            byte[] key = a2.GetPublicKey();
            return PublicKeyMatches(a1, key);
        }

        internal static bool PublicKeyMatches(System.Reflection.AssemblyName a1, byte[] publicKeyOrToken)
        {
            if (publicKeyOrToken == null)
                return a1.GetPublicKey() == null;
            byte[] publicKey = a1.GetPublicKey();
            if (publicKey != null && publicKeyOrToken.Length == publicKey.Length)
            {
                for (int i = 0; i < publicKey.Length; i++)
                    if (publicKey[i] != publicKeyOrToken[i])
                        return false;
                return true;
            }
            byte[] publicKeyToken = a1.GetPublicKeyToken();
            if (publicKeyOrToken.Length == publicKeyToken.Length)
            {
                for (int i = 0; i < publicKeyToken.Length; i++)
                    if (publicKeyToken[i] != publicKeyOrToken[i])
                        return false;
                return true;
            }
            return false;
        }

        internal static String PublicKeyToString(byte[] key)
        {
            if (key == null || key.Length == 0)
                return "null";

            StringBuilder sb = new StringBuilder(key.Length);
            foreach (byte b in key)
            {
                sb.Append(b.ToString("x2", System.Globalization.CultureInfo.InvariantCulture));
            }
            return sb.ToString();
        }

        // You must have already normalized the paths!
        internal static String MakeRelativePath(String path, String root)
        {
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(path));
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(root));
            if (!path.StartsWith(root, StringComparison.OrdinalIgnoreCase))
                throw new ArgumentException(Res.MakeRelativePathArgs);
            System.Diagnostics.Contracts.Contract.Requires(String.Equals(path, Path.GetFullPath(path)));
            System.Diagnostics.Contracts.Contract.Requires(String.Equals(root, Path.GetFullPath(root)));
            System.Diagnostics.Contracts.Contract.Ensures(!Path.IsPathRooted(System.Diagnostics.Contracts.Contract.Result<String>()));
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            int skip = 0;
            char lastChar = root[root.Length - 1];
            if (lastChar != Path.DirectorySeparatorChar && lastChar != Path.AltDirectorySeparatorChar)
                skip++;

            String relPath = path.Substring(root.Length + skip);
            return relPath;
        }

        // Pass in fully qualified names.  We've factored out our assembly names
        // comparisons so that we can handle policy if necessary, and also deal with
        // potentially mal-formed assembly refs, or assembly refs that include
        // processor architecture, etc.  (I haven't implemented that, but it could be done.)
        internal static bool AssemblyRefEqualsDef(String assemblyRef, String assemblyDef)
        {
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(assemblyRef));
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(assemblyDef));

            return String.Equals(assemblyRef, assemblyDef);
        }

        // Pass in fully qualified names.  We've factored out our assembly names
        // comparisons so that we can handle policy if necessary.
        internal static bool AssemblyDefEqualsDef(String assemblyDef1, String assemblyDef2)
        {
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(assemblyDef1));
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(assemblyDef2));

            return String.Equals(assemblyDef1, assemblyDef2);
        }

        // Pass in fully qualified type names.  We've factored out our assembly names
        // comparisons so that we can handle policy if necessary.
        internal static bool FullTypeNameDefEqualsDef(String typeAndAssemblyName1, String typeAndAssemblyName2)
        {
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(typeAndAssemblyName1));
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(typeAndAssemblyName2));

            return String.Equals(typeAndAssemblyName1, typeAndAssemblyName2);
        }

        // If you're calling this method, you suspect that you've already loaded this
        // assembly, but you need to upgrade the assembly from the LoadFrom loader
        // context to the default loader context.  However, we'll have to call it
        // for an unbound set of assemblies.
        internal static Assembly FindLoadedAssemblyRef(String assemblyRef)
        {
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(assemblyRef));

            foreach (Assembly a in AppDomain.CurrentDomain.GetAssemblies()) {
                if (Utils.AssemblyRefEqualsDef(assemblyRef, a.FullName)) {
                    //Console.WriteLine("FindLoadedAssemblyRef found its target (probably in the LoadFrom context).  Returning, in hopes of upgrading to default loader context.  Code base: {0}", a.CodeBase);
                    return a;
                }
            }
            return null;
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", MessageId = "System.Reflection.Assembly.LoadFrom", Justification="LoadFrom was designed for addins")]
        internal static Assembly LoadAssemblyFrom(List<String> dirsToLookIn, String assemblyRef)
        {
            int firstComma = assemblyRef.IndexOf(',');
            if (firstComma == -1)
                return null;
            String simpleName = assemblyRef.Substring(0, firstComma);

            List<String> potentialFileNames = new List<string>(dirsToLookIn.Count * 2);
            foreach (String path in dirsToLookIn)
            {
                String simpleFileName = Path.Combine(path, simpleName);
                String dllName = simpleFileName + ".dll";
                if (File.Exists(dllName))
                    potentialFileNames.Add(dllName);
                else if (File.Exists(simpleFileName + ".exe"))
                    potentialFileNames.Add(simpleFileName + ".exe");
            }

            foreach (String fileName in potentialFileNames)
            {
                try
                {
                    Assembly assembly = Assembly.LoadFrom(fileName);
                    // We should at least be comparing the public key token
                    // for the two assemblies here.  The version numbers may 
                    // potentially be different, dependent on publisher policy.
                    if (Utils.AssemblyRefEqualsDef(assemblyRef, assembly.FullName))
                    {
                        return assembly;
                    }
                }
                catch (BadImageFormatException)
                {
                }
            }
            return null;
        }
 
        // If they have full trust, give a good error message.  Otherwise, prevent information disclosure.
        [SecuritySafeCritical]
        internal static bool HasFullTrust()
        {
            try
            {
                new PermissionSet(PermissionState.Unrestricted).Demand();
                return true;
            }
            catch(SecurityException)
            {
                return false;
            }
        }

        //Utility method to assert permission and unload the appdomain
        [System.Security.SecurityCritical]
        [SecurityPermission(SecurityAction.Assert, ControlAppDomain = true)]
        internal static void UnloadAppDomain(AppDomain domain)
        {
            AppDomain.Unload(domain);
        }
    }
}

