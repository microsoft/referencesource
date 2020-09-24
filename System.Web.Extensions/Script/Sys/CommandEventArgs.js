Sys.CommandEventArgs = function(commandName, commandArgument, commandSource) {
    /// <param name="commandName" type="String">The command name.</param>
    /// <param name="commandArgument" mayBeNull="true">The command arguments.</param>
    /// <param name="commandSource" mayBeNull="true">The command source.</param>
    Sys.CommandEventArgs.initializeBase(this);
    this._commandName = commandName;
    this._commandArgument = commandArgument;
    this._commandSource = commandSource;
}
Sys.CommandEventArgs.prototype = {
    _commandName: null,
    _commandArgument: null,
    _commandSource: null,
    get_commandName: function() {
        /// <value type="String">The command name.</value>
        return this._commandName;
    },
    get_commandArgument: function() {
        /// <value mayBeNull="true">The command arguments.</value>
        return this._commandArgument;
    },
    get_commandSource: function() {
        /// <value mayBeNull="true">The command source.</value>
        return this._commandSource;
    }
}
Sys.CommandEventArgs.registerClass("Sys.CommandEventArgs", Sys.CancelEventArgs);