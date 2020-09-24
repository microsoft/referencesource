#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Error.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Error.__typeName = 'Error';
Error.__class = true;

Error.create = function(message, errorInfo) {
    /// <summary>Use this method to create a new error.</summary>
    /// <param name="message" type="String" optional="true" mayBeNull="true">The error message.</param>
    /// <param name="errorInfo" optional="true" mayBeNull="true">
    ///     A plain JavaScript object that contains extended information about the error.
    ///     The object should have a 'name' field that contains a string that identifies the error
    ///     and any additional fields that are necessary to fully describe the error.
    /// </param>
    /// <returns type="Error">An Error object.</returns>

    // If message string can be converted to a number, IE sets e.message to the number, not the string.
    // Workaround this issue by explicitly setting e.message to the string.
    var err = new Error(message);
    err.message = message;

    if (errorInfo) {
        for (var v in errorInfo) {
            err[v] = errorInfo[v];
        }
    }

    err.popStackFrame();
    return err;
}

// The ArgumentException ctor in .NET has the message *before* paramName.  This
// is inconsistent with all the other Argument*Exception ctors in .NET.
// We feel the paramName is more important than the message, and we want all our
// argument errors to be consistent, so our Error.argument() takes the paramName
// before the message.  This is inconsistent with .NET, but overall we feel
// it is the better design.
Error.argument = function(paramName, message) {
    /// <summary>
    ///     Creates an ArgumentException with a specified error message
    ///     and the name of the parameter that caused this exception.
    /// </summary>
    /// <param name="paramName" type="String" optional="true" mayBeNull="true">
    ///     The name of the parameter that caused the exception.
    /// </param>
    /// <param name="message" type="String" optional="true" mayBeNull="true">
    ///     A message that describes the error.
    /// </param>
    /// <returns>An Error instance that represents an ArgumentException.</returns>

    var displayMessage = "Sys.ArgumentException: " + (message ? message : Sys.Res.argument);
    if (paramName) {
        displayMessage += "\n" + String.format(Sys.Res.paramName, paramName);
    }

    var err = Error.create(displayMessage, { name: "Sys.ArgumentException", paramName: paramName });
    err.popStackFrame();
    return err;
}

Error.argumentNull = function(paramName, message) {
    /// <summary>
    ///     Creates an ArgumentNullException with a specified error message
    ///     and the name of the parameter that caused this exception.
    /// </summary>
    /// <param name="paramName" type="String" optional="true" mayBeNull="true">
    ///     The name of the parameter that caused the exception.
    /// </param>
    /// <param name="message" type="String" optional="true" mayBeNull="true">
    ///     A message that describes the error.
    /// </param>
    /// <returns>An Error instance that represents an ArgumentNullException.</returns>

    var displayMessage = "Sys.ArgumentNullException: " + (message ? message : Sys.Res.argumentNull);
    if (paramName) {
        displayMessage += "\n" + String.format(Sys.Res.paramName, paramName);
    }

    var err = Error.create(displayMessage, { name: "Sys.ArgumentNullException", paramName: paramName });
    err.popStackFrame();
    return err;
}

Error.argumentOutOfRange = function(paramName, actualValue, message) {
    /// <summary>
    ///     Creates an ArgumentOutOfRangeException with a specified error message
    ///     and the name and actual value of the parameter that caused this exception.
    /// </summary>
    /// <param name="paramName" type="String" optional="true" mayBeNull="true">
    ///     The name of the parameter that caused the exception.
    /// </param>
    /// <param name="actualValue" optional="true" mayBeNull="true">
    ///     The actual value of the parameter.
    /// </param>
    /// <param name="message" type="String" optional="true" mayBeNull="true">
    ///     A message that describes the error.
    /// </param>
    /// <returns>An Error instance that represents an ArgumentOutOfRangeException.</returns>

    var displayMessage = "Sys.ArgumentOutOfRangeException: " + (message ? message : Sys.Res.argumentOutOfRange);
    if (paramName) {
        displayMessage += "\n" + String.format(Sys.Res.paramName, paramName);
    }

    // .NET implementation of ArgumentOutOfRangeException does not display actualValue if it is null.
    // For parity with .NET, we do not display if actualValue is null or undefined.  This is OK,
    // since more specific exceptions exist for null and undefined.
    if (typeof(actualValue) !== "undefined" && actualValue !== null) {
        displayMessage += "\n" + String.format(Sys.Res.actualValue, actualValue);
    }

    var err = Error.create(displayMessage, {
        name: "Sys.ArgumentOutOfRangeException",
        paramName: paramName,
        actualValue: actualValue
    });
    err.popStackFrame();
    return err;
}

Error.argumentType = function(paramName, actualType, expectedType, message) {
    /// <summary>
    ///     Creates an ArgumentTypeException with a specified error message
    ///     and the name, actual type, and expected type of the parameter that
    ///     caused this exception.
    /// </summary>
    /// <param name="paramName" type="String" optional="true" mayBeNull="true">
    ///     The name of the parameter that caused the exception.
    /// </param>
    /// <param name="actualType" type="Type" optional="true" mayBeNull="true">
    ///     The actual type of the parameter value.
    /// </param>
    /// <param name="expectedType" type="Type" optional="true" mayBeNull="true">
    ///     The expected type of the parameter value.
    /// </param>
    /// <param name="message" type="String" optional="true" mayBeNull="true">
    ///     A message that describes the error.
    /// </param>
    /// <returns>An Error instance that represents an ArgumentTypeException.</returns>

    var displayMessage = "Sys.ArgumentTypeException: ";
    if (message) {
        displayMessage += message;
    }
    else if (actualType && expectedType) {
        displayMessage +=
            String.format(Sys.Res.argumentTypeWithTypes, actualType.getName(), expectedType.getName());
    }
    else {
        displayMessage += Sys.Res.argumentType;
    }

    if (paramName) {
        displayMessage += "\n" + String.format(Sys.Res.paramName, paramName);
    }

    var err = Error.create(displayMessage, {
        name: "Sys.ArgumentTypeException",
        paramName: paramName,
        actualType: actualType,
        expectedType: expectedType
    });
    err.popStackFrame();
    return err;
}

Error.argumentUndefined = function(paramName, message) {
    /// <summary>
    ///     Creates an ArgumentUndefinedException with a specified error message
    ///     and the name of the parameter that caused this exception.
    /// </summary>
    /// <param name="paramName" type="String" optional="true" mayBeNull="true">
    ///     The name of the parameter that caused the exception.
    /// </param>
    /// <param name="message" type="String" optional="true" mayBeNull="true">
    ///     A message that describes the error.
    /// </param>
    /// <returns>An Error instance that represents an ArgumentUndefinedException.</returns>

    var displayMessage = "Sys.ArgumentUndefinedException: " + (message ? message : Sys.Res.argumentUndefined);
    if (paramName) {
        displayMessage += "\n" + String.format(Sys.Res.paramName, paramName);
    }

    var err = Error.create(displayMessage, { name: "Sys.ArgumentUndefinedException", paramName: paramName });
    err.popStackFrame();
    return err;
}

Error.format = function(message) {
    /// <summary>
    ///     Creates a format error.
    /// </summary>
    /// <param name="message" type="String" optional="true" mayBeNull="true">The error message.</param>
    /// <returns>An Error object that represents a FormatException.</returns>
    var displayMessage = "Sys.FormatException: " + (message ? message : Sys.Res.format);
    var err = Error.create(displayMessage, {name: 'Sys.FormatException'});
    err.popStackFrame();
    return err;
}

Error.invalidOperation = function(message) {
    /// <summary>
    ///     Creates an invalid operation error.
    /// </summary>
    /// <param name="message" type="String" optional="true" mayBeNull="true">The error message.</param>
    /// <returns>An Error instance that represents an InvalidOperationException.</returns>
    var displayMessage = "Sys.InvalidOperationException: " + (message ? message : Sys.Res.invalidOperation);

    var err = Error.create(displayMessage, {name: 'Sys.InvalidOperationException'});
    err.popStackFrame();
    return err;
}

Error.notImplemented = function(message) {
    /// <summary>
    ///     Creates a not implemented error.
    /// </summary>
    /// <param name="message" type="String" optional="true" mayBeNull="true">The error message.</param>
    /// <returns>An Error instance that represents a NotImplementedException.</returns>
    var displayMessage = "Sys.NotImplementedException: " + (message ? message : Sys.Res.notImplemented);

    var err = Error.create(displayMessage, {name: 'Sys.NotImplementedException'});
    err.popStackFrame();
    return err;
}

Error.parameterCount = function(message) {
    /// <summary>
    ///     Creates a ParameterCountException with a specified error message.
    /// </summary>
    /// <param name="message" type="String" optional="true" mayBeNull="true">
    ///     A message that describes the error.
    /// </param>
    /// <returns>An Error instance that represents a ParameterCountException.</returns>

    var displayMessage = "Sys.ParameterCountException: " + (message ? message : Sys.Res.parameterCount);
    var err = Error.create(displayMessage, {name: 'Sys.ParameterCountException'});
    err.popStackFrame();
    return err;
}

Error.prototype.popStackFrame = function() {
    /// <summary>
    ///     Updates the fileName and lineNumber fields based on the next frame in the
    ///     stack trace.  Call this method whenever an instance of Error is returned
    ///     from a function.  This makes the fileName and lineNumber reported in the
    ///     FireFox console point to the location where the exception was thrown, not
    ///     the location where the instance of Error was created.
    /// </summary>
    /// <example>
    ///     function checkParam(param, expectedType) {
    ///         if (!expectedType.isInstanceOfType(param)) {
    ///             var e = new Error("invalid type");
    ///             e.popStackFrame();
    ///             return e;
    ///         }
    ///     }
    /// </example>

    // Example stack frame
    // ===================
    // Error("test error")@:0
    // createError()@http://localhost/app/Error.js:2
    // throwError()@http://localhost/app/Error.js:6
    // callThrowError()@http://localhost/app/Error.js:10
    // @http://localhost/app/Error:js:14

    if (typeof(this.stack) === "undefined" || this.stack === null ||
        typeof(this.fileName) === "undefined" || this.fileName === null ||
        typeof(this.lineNumber) === "undefined" || this.lineNumber === null) {
        return;
    }

    var stackFrames = this.stack.split("\n");

    // Find current stack frame.  It may not be the first stack frame, since the very
    // first frame when the Error is constructed does not correspond to any actual file
    // or line number.  See example stack frame above.
    var currentFrame = stackFrames[0];
    var pattern = this.fileName + ":" + this.lineNumber;
    while(typeof(currentFrame) !== "undefined" &&
          currentFrame !== null &&
          currentFrame.indexOf(pattern) === -1) {
        stackFrames.shift();
        currentFrame = stackFrames[0];
    }

    var nextFrame = stackFrames[1];

    // Special-case last stack frame, to stop shifting frames off the stack.
    if (typeof(nextFrame) === "undefined" || nextFrame === null) {
        return;
    }

    // Update fields to correspond with next stack frame
    var nextFrameParts = nextFrame.match(/@(.*):(\d+)$/);
    if (typeof(nextFrameParts) === "undefined" || nextFrameParts === null) {
        return;
    }

    this.fileName = nextFrameParts[1];

    // This should always succeed, since the regex matches "\d+"
    this.lineNumber = parseInt(nextFrameParts[2]);

    stackFrames.shift();
    this.stack = stackFrames.join("\n");
}
