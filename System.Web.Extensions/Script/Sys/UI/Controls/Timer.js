#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Timer.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif

Sys.UI._Timer = function(element) {
    Sys.UI._Timer.initializeBase(this,[element]);
    this._interval = 60000;
    this._enabled = true;
    this._postbackPending = false;
    this._raiseTickDelegate = null;
    this._endRequestHandlerDelegate = null;
    this._timer = null;
    this._pageRequestManager = null;
    this._uniqueID = null;
}
Sys.UI._Timer.prototype = {
    get_enabled: function() {
        /// <value type="Boolean"/>
        return this._enabled;
    },
    set_enabled: function(value) {
        /// <value type="Boolean"/>
        this._enabled = value;
    },
    get_interval: function() {
        /// <value type="Number"/>
        return this._interval;
    },
    set_interval: function(value) {
        /// <value type="Number"/>
        this._interval = value;
    },
    get_uniqueID: function(){
        /// <value type="String"/>
        return this._uniqueID;
    },
    set_uniqueID: function(value){
        /// <value type="String"/>
        this._uniqueID = value;
    },
    dispose: function(){
       this._stopTimer();
       if(this._pageRequestManager !== null){
           this._pageRequestManager.remove_endRequest(this._endRequestHandlerDelegate);
       }
       Sys.UI._Timer.callBaseMethod(this,"dispose");
    },
    _doPostback: function(){
        __doPostBack(this.get_uniqueID(),'');
    },
    _handleEndRequest: function(sender, arg){
        var dataItem = arg.get_dataItems()[this.get_id()];
	    if (dataItem){
            this._update(dataItem[0],dataItem[1]);
	  	}
	  
         // if postback is pending, verify that another asyncPostBack hasn't already yet	
	    if ((this._postbackPending === true) && (this._pageRequestManager !== null)&&(this._pageRequestManager.get_isInAsyncPostBack() === false)){
    	   	this._postbackPending = false;
            this._doPostback();
        }
	   
    },
    initialize: function(){
        Sys.UI._Timer.callBaseMethod(this, 'initialize');
    	this._raiseTickDelegate = Function.createDelegate(this,this._raiseTick);
    	this._endRequestHandlerDelegate = Function.createDelegate(this,this._handleEndRequest);
    	if (Sys.WebForms && Sys.WebForms.PageRequestManager){
           this._pageRequestManager = Sys.WebForms.PageRequestManager.getInstance();  
    	}
    	if (this._pageRequestManager !== null ){
    	    this._pageRequestManager.add_endRequest(this._endRequestHandlerDelegate);
    	}
        if(this.get_enabled()) {
            this._startTimer();
        }
    },
    _raiseTick: function() {
        // window.setTimeout only ticks ones, so need to restart it on every tick
        this._startTimer();
        if ((this._pageRequestManager === null) || (!this._pageRequestManager.get_isInAsyncPostBack())){
            this._doPostback();
            this._postbackPending = false;
        } 
        else {
            this._postbackPending = true;
        }
    },
    _startTimer: function(){
        // using window.setTimout instead of window.setInterval because window.setInterval doesn't
        // follow proper timing when multiple setInterval calls are made like in case of multiple 
        // Timers on the page
        this._timer = window.setTimeout(Function.createDelegate(this,this._raiseTick),this.get_interval());
    },
    _stopTimer: function(){
	    if (this._timer !== null){
	 	    window.clearTimeout(this._timer);
		    this._timer = null;
       } 	
    },
    _update: function(enabled,interval) {
        var stopped = !this.get_enabled();
        var intervalChanged= (this.get_interval() !== interval);
	    if ((!stopped) && ((!enabled)||(intervalChanged))){
    	  	this._stopTimer();
    		stopped = true;
       	} 
    	this.set_enabled(enabled);
    	this.set_interval(interval);
    	if ((this.get_enabled()) && (stopped)){
    	    this._startTimer();
    	}
    }
}
Sys.UI._Timer.registerClass('Sys.UI._Timer', Sys.UI.Control);