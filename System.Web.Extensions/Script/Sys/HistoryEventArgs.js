Sys.HistoryEventArgs = function(state) {
    /// <param name="state" type="Object"/>
    Sys.HistoryEventArgs.initializeBase(this);
    this._state = state;
}
Sys.HistoryEventArgs.prototype = {
    get_state: function() {
        /// <value type="Object"/>
        return this._state;
    }
}
Sys.HistoryEventArgs.registerClass('Sys.HistoryEventArgs', Sys.EventArgs);
