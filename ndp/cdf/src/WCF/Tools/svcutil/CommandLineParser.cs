//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.SvcUtil
{
    using System;
    using System.Globalization;
    using System.Collections.Generic;
    using System.Text.RegularExpressions;

    internal enum SwitchType
    {
        Flag,
        SingletonValue,
        ValueList
    }

    internal class CommandSwitch
    {
        readonly string name;
        readonly string abbreviation;
        readonly SwitchType switchType;

        internal CommandSwitch(string name, string abbreviation, SwitchType switchType)
        {
            //ensure that either name doesn't start with '/' or '-'
            //also convert to lower-case
            if ((name[0] == '/') || (name[0] == '-'))
                this.name = (name.Substring(1)).ToLower(CultureInfo.InvariantCulture);
            else
                this.name = name.ToLower(CultureInfo.InvariantCulture);
            if ((abbreviation[0] == '/') || (abbreviation[0] == '-'))
                this.abbreviation = (abbreviation.Substring(1)).ToLower(CultureInfo.InvariantCulture);
            else
                this.abbreviation = abbreviation.ToLower(CultureInfo.InvariantCulture);

            this.switchType = switchType;
        }

        internal string Name
        {
            get { return name; }
        }

#if NotUsed
        internal string Abbreviation
        {
            get { return abbreviation; }
        }
#endif

        internal SwitchType SwitchType
        {
            get { return switchType; }
        }

        internal bool Equals(string other)
        {
            string temp;

            //ensure that compare doesn't start with '/' or '-'
            //also convert to lower-case
            if ((other[0] == '/') || (other[0] == '-'))
                temp = (other.Substring(1)).ToLower(CultureInfo.InvariantCulture);
            else
                temp = other.ToLower(CultureInfo.InvariantCulture);

            //if equal to name, then return the OK
            if (name.Equals(temp))
                return true;
            //now check abbreviation
            return abbreviation.Equals(temp);
        }

        internal static CommandSwitch FindSwitch(string name, CommandSwitch[] switches)
        {
            foreach (CommandSwitch cs in switches)
                if (cs.Equals(name))
                    return cs;
            //if no match found, then return null
            return null;
        }
    }

    internal class ArgumentDictionary
    {
        Dictionary<string, IList<string>> contents;

        internal ArgumentDictionary(int capacity)
        {
            contents = new Dictionary<string, IList<string>>(capacity);
        }

        internal void Add(string key, string value)
        {
            IList<string> values;
            if (!ContainsArgument(key))
            {
                values = new List<string>();
                Add(key, values);
            }
            else
                values = GetArguments(key);

            values.Add(value);
        }

        internal string GetArgument(string key)
        {
            IList<string> values;
            if (contents.TryGetValue(key.ToLower(CultureInfo.InvariantCulture), out values))
            {
#if SM_TOOL
                Tool.Assert((values.Count == 1), "contains more than one argument please call GetArguments");
#endif
                return values[0];
            }
#if SM_TOOL
            Tool.Assert(false, "argument was not specified please call ContainsArgument to check this");
#endif

            return null; // unreachable code but the compiler doesn't know this.
        }

        internal IList<string> GetArguments(string key)
        {
            IList<string> result;
            if (!contents.TryGetValue(key.ToLower(CultureInfo.InvariantCulture), out result))
                result = new List<string>();
            return result;
        }

        internal bool ContainsArgument(string key)
        {
            return contents.ContainsKey(key.ToLower(CultureInfo.InvariantCulture));
        }

        internal void Add(string key, IList<string> values)
        {
            contents.Add(key.ToLower(CultureInfo.InvariantCulture), values);
        }

        internal int Count
        {
            get { return contents.Count; }
        }

    }

    internal static class CommandParser
    {


        internal static ArgumentDictionary ParseCommand(string[] cmd, CommandSwitch[] switches)
        {
            ArgumentDictionary arguments;   //switches/values from cmd line

            string arg;                     //argument to test next
            CommandSwitch argSwitch;        //switch corresponding to that argument
            string argValue;                //value corresponding to that argument
            int delim;                      //location of value delimiter (':' or '=')

            arguments = new ArgumentDictionary(cmd.Length);
            foreach (string s in cmd)
            {
                arg = s;
                bool argIsFlag = true;

                //if argument does not start with switch indicator, place into "default" arguments
                if ((arg[0] != '/') && (arg[0] != '-'))
                {
                    arguments.Add(String.Empty, arg);
                    continue;
                }

                //if we have something which begins with '/' or '-', throw if nothing after it
                if (arg.Length == 1)
                    throw new ArgumentException(SR.GetString(SR.ErrSwitchMissing, arg));

                //yank switch indicator ('/' or '-') off of command argument
                arg = arg.Substring(1);

                //check to make sure delimiter does not start off switch
                delim = arg.IndexOfAny(new char[] { ':', '=' });
                if (delim == 0)
                    throw new ArgumentException(SR.GetString(SR.ErrUnexpectedDelimiter));

                //if there is no value, than create a null string
                if (delim == (-1))
                    argValue = String.Empty;
                else
                {
                    //assume valid argument now; must remove value attached to it
                    //must avoid copying delimeter into either arguments
                    argValue = arg.Substring(delim + 1);
                    arg = arg.Substring(0, delim);
                    argIsFlag = false;
                }

                //check if this switch exists in the list of possible switches
                //if no match found, then throw an exception
                argSwitch = CommandSwitch.FindSwitch(arg.ToLower(CultureInfo.InvariantCulture), switches);
                if (argSwitch == null)
                    throw new ArgumentException(SR.GetString(SR.ErrUnknownSwitch, arg.ToLower(CultureInfo.InvariantCulture)));

                //check if switch is allowed to have a value
                // if not and a value has been specified, then thrown an exception
                if (argSwitch.SwitchType == SwitchType.Flag)
                {
                    if (!argIsFlag)
                        throw new ArgumentException(SR.GetString(SR.ErrUnexpectedValue, arg.ToLower(CultureInfo.InvariantCulture)));
                }
                else
                {
                    if (argIsFlag)
                        throw new ArgumentException(SR.GetString(SR.ErrExpectedValue, arg.ToLower(CultureInfo.InvariantCulture)));
                }

                //check if switch is allowed to be specified multiple times
                // if not and it has already been specified and a new value has been paresd, throw an exception
                if (argSwitch.SwitchType != SwitchType.ValueList && arguments.ContainsArgument(argSwitch.Name))
                {
                    throw new ArgumentException(SR.GetString(SR.ErrSingleUseSwitch, arg.ToLower(CultureInfo.InvariantCulture)));
                }
                else
                {
                    arguments.Add(argSwitch.Name, argValue);
                }
            }

            return arguments;
        }

    }

}
