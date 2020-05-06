#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="DomElement.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif
 
Sys.UI.DomElement = function() {
    /// <summary>This static class provides helpers to work with DOM elements.</summary>
    ##DEBUG throw Error.notImplemented();
}
Sys.UI.DomElement.registerClass('Sys.UI.DomElement');

Sys.UI.DomElement.addCssClass = function(element, className) {
    /// <summary>Adds a CSS class to an element if it doesn't already have it.</summary>
    /// <param name="element" domElement="true"/>
    /// <param name="className" type="String">The name of the CSS class to add.</param>
    if (!Sys.UI.DomElement.containsCssClass(element, className)) {
        if (element.className === '') {
            element.className = className;
        }
        else {
            element.className += ' ' + className;
        }
    }
}

Sys.UI.DomElement.containsCssClass = function(element, className) {
    /// <summary>Determines if an element has the specified CSS class.</summary>
    /// <param name="element" domElement="true"/>
    /// <param name="className" type="String">The name of the CSS class to test.</param>
    /// <returns type="Boolean">True if the CSS class was found on the element.</returns>
    return Array.contains(element.className.split(' '), className);
}

Sys.UI.DomElement.getBounds = function(element) {
    /// <summary>Gets the coordinates, width and height of an element.</summary>
    /// <param name="element" domElement="true"/>
    /// <returns type="Sys.UI.Bounds">
    ///   A Bounds object with four fields, x, y, width and height, which contain the pixel coordinates,
    ///   width and height of the element.
    /// </returns>
    var offset = Sys.UI.DomElement.getLocation(element);

    return new Sys.UI.Bounds(offset.x, offset.y, element.offsetWidth || 0, element.offsetHeight || 0);
}

var $get = Sys.UI.DomElement.getElementById = function(id, element) {
    /// <summary>Finds an element by id.</summary>
    /// <param name="id" type="String">The id of the element to find.</param>
    /// <param name="element" domElement="true" optional="true" mayBeNull="true"/>
    /// <returns domElement="true" mayBeNull="true">The element, or null if it was not found.</returns>
    if (!element) return document.getElementById(id);
    if (element.getElementById) return element.getElementById(id);

    // Implementation for browsers that don't have getElementById on elements:
    var nodeQueue = [];
    var childNodes = element.childNodes;
    for (var i = 0; i < childNodes.length; i++) {
        var node = childNodes[i];
        if (node.nodeType == 1) {
            nodeQueue[nodeQueue.length] = node;
        }
    }

    while (nodeQueue.length) {
        node = nodeQueue.shift();
        if (node.id == id) {
            return node;
        }
        childNodes = node.childNodes;
        for (i = 0; i < childNodes.length; i++) {
            node = childNodes[i];
            if (node.nodeType == 1) {
                nodeQueue[nodeQueue.length] = node;
            }
        }
    }

    return null;
}


if (document.documentElement.getBoundingClientRect) {
    Sys.UI.DomElement.getLocation = function(element) {
        /// <summary>Gets the coordinates of a DOM element.</summary>
        /// <param name="element" domElement="true"/>
        /// <returns type="Sys.UI.Point">
        ///   A Point object with two fields, x and y, which contain the pixel coordinates of the element.
        /// </returns>
        // For a document element, body, or window, return zero.
        // In IE8, the boundingClientRect for body is influenced by the bounding rect of its content, and so may not be 0,0.
        // But for positioning purposes, elements positioned at 0,0 will be at the top even if the content has margins, etc, so
        // getlocation should return 0,0 for body.
        // In all browsers, detecting the body works by seeing if the element's parent ndoe is the element's own document's documentElement node.
        if (element.self || element.nodeType === 9 || // window?
            (element === document.documentElement) || // documentElement?
            (element.parentNode === element.ownerDocument.documentElement)) { // body?
            return new Sys.UI.Point(0, 0);
        }        
        
        // Here there is a small inconsistency with what other browsers would give for wrapping elements:
        // the bounding rect can be different from the first rectangle. getBoundingRect is used here
        // because it's more consistent and because clientRects need to be offset by the coordinates
        // of the frame in the parent window, which is not always accessible to script (if it's in a different
        // domain in particular).
        var clientRect = element.getBoundingClientRect();
        if (!clientRect) {
            return new Sys.UI.Point(0,0);
        }
        // Dev11 360629 - Different browsers can fill in the scroll position on either the documentElement or the body
        // element.  Safari and quirks-mode IE use the body element from what I can tell, and standards-mode IE along
        // with most other browsers use documentElement.  The other value will always exist, but be 0.  Choose the
        // correct one carefully, giving preference to the more standard documentElement.
        var documentElement = element.ownerDocument.documentElement;
        var bodyElement = element.ownerDocument.body;
        // Firefox 3 can return decimals here, so round them.
        // This appears to be consistent with how the display engine actually places the element when there is a decimal.
        var ex,
            offsetX = Math.round(clientRect.left) + (documentElement.scrollLeft || bodyElement.scrollLeft),
            offsetY = Math.round(clientRect.top) + (documentElement.scrollTop || bodyElement.scrollTop);
        if (Sys.Browser.agent === Sys.Browser.InternetExplorer) {
            // When the window is an iframe, the frameborder needs to be added. This is only available from
            // script when the parent window is in the same domain as the frame, hence the try/catch.
            try {
                var f = element.ownerDocument.parentWindow.frameElement || null;
                if (f) {
                    // frameBorder has a default of "1" so undefined must map to 0, and "0" and "no" to 2.
                    var offset = (f.frameBorder === "0" || f.frameBorder === "no") ? 2 : 0;
                    offsetX += offset;
                    offsetY += offset;
                }
            }
            catch(ex) {
            }
            if (Sys.Browser.version === 7 && !document.documentMode) {
                // IE7 reapplies the page zoom level when using the returned coordinates.
                // therefore we must divide by the zoom level to compensate. This is not perfect, but close.
                // NOTE: IE8 with document.documentMode === 7 does NOT emulate IE7 behavior, by design.
                // Also, this zoom detection does not work perfectly in IE8 compat mode, where we would want
                // it to be 100% always, so it is necessary that we ensure this only happens in ACTUAL IE7.
                // IE6 does not support zoom.
                var body = document.body,
                    rect = body.getBoundingClientRect(),
                    zoom = (rect.right-rect.left) / body.clientWidth;
                // zoom is not completely accurate, so snap to the previous 5% by multiplying by 100, rounding,
                // then subtracting zoom % 5, then dividing by 100 to get back to a multiplier.
                // It's not likely someone is zooming at 154%, for example, so that probably means it is actually 150%, whereas
                // 156% probably means 155% (the estimate tends to over-estimate).
                zoom = Math.round(zoom * 100);
                zoom = (zoom - zoom % 5) / 100;
                if (!isNaN(zoom) && (zoom !== 1)) {
                    offsetX = Math.round(offsetX / zoom);
                    offsetY = Math.round(offsetY / zoom);
                }
            }        
            if ((document.documentMode || 0) < 8) {
                offsetX -= documentElement.clientLeft;
                offsetY -= documentElement.clientTop;
            }
        }
        return new Sys.UI.Point(offsetX, offsetY);
    }
}
else if (Sys.Browser.agent === Sys.Browser.Safari) {
    Sys.UI.DomElement.getLocation = function(element) {
        /// <summary>Gets the coordinates of a DOM element.</summary>
        /// <param name="element" domElement="true"/>
        /// <returns type="Sys.UI.Point">
        ///   A Point object with two fields, x and y, which contain the pixel coordinates of the element.
        /// </returns>
        // For a document element, return zero.
        if ((element.window && (element.window === element)) || element.nodeType === 9) return new Sys.UI.Point(0,0);

        var offsetX = 0, offsetY = 0,
            parent,
            previous = null,
            previousStyle = null,
            currentStyle;
        for (parent = element; parent; previous = parent, previousStyle = currentStyle, parent = parent.offsetParent) {
            currentStyle = Sys.UI.DomElement._getCurrentStyle(parent);
            // DevDiv Bugs 146697: tagName needs to be case insensitive to work with xhtml content type
            var tagName = parent.tagName ? parent.tagName.toUpperCase() : null;

            // Safari has a bug that double-counts the body offset for absolutely positioned elements
            // that are direct children of body.
            // Firefox has its own quirk, which is that non-absolutely positioned elements that are
            // direct children of body get the body offset counted twice.
            if ((parent.offsetLeft || parent.offsetTop) &&
                ((tagName !== "BODY") || (!previousStyle || previousStyle.position !== "absolute"))) {
                offsetX += parent.offsetLeft;
                offsetY += parent.offsetTop;
            }

            // safari 3 does not count border width. But don't count the element's own border.
            // previous would be set if parent is not the target element.
            if (previous && Sys.Browser.version >= 3) {
                offsetX += parseInt(currentStyle.borderLeftWidth);
                offsetY += parseInt(currentStyle.borderTopWidth);
            }
        }

        currentStyle = Sys.UI.DomElement._getCurrentStyle(element);
        var elementPosition = currentStyle ? currentStyle.position : null;
        // If an element is absolutely positioned, its parent's scroll should not be subtracted
        if (!elementPosition || (elementPosition !== "absolute")) {
            // In Firefox and Safari, all parent's scroll values must be taken into account.
            for (parent = element.parentNode; parent; parent = parent.parentNode) {
                // DevDiv Bugs 146697: tagName needs to be case insensitive to work with xhtml content type
                tagName = parent.tagName ? parent.tagName.toUpperCase() : null;

                if ((tagName !== "BODY") && (tagName !== "HTML") && (parent.scrollLeft || parent.scrollTop)) {
                    offsetX -= (parent.scrollLeft || 0);
                    offsetY -= (parent.scrollTop || 0);
                }
                currentStyle = Sys.UI.DomElement._getCurrentStyle(parent);
                var parentPosition = currentStyle ? currentStyle.position : null;

                // If an element is absolutely positioned, its parent's scroll should not be subtracted
                if (parentPosition && (parentPosition === "absolute")) break;
            }
        }
        return new Sys.UI.Point(offsetX, offsetY);
    }
}
else {
    Sys.UI.DomElement.getLocation = function(element) {
        /// <summary>Gets the coordinates of a DOM element.</summary>
        /// <param name="element" domElement="true"/>
        /// <returns type="Sys.UI.Point">
        ///   A Point object with two fields, x and y, which contain the pixel coordinates of the element.
        /// </returns>
        // For a document element, return zero.
        if ((element.window && (element.window === element)) || element.nodeType === 9) return new Sys.UI.Point(0,0);

        var offsetX = 0, offsetY = 0,
            parent,
            previous = null,
            previousStyle = null,
            currentStyle = null;
        for (parent = element; parent; previous = parent, previousStyle = currentStyle, parent = parent.offsetParent) {
            // DevDiv Bugs 146697: tagName needs to be case insensitive to work with xhtml content type
            var tagName = parent.tagName ? parent.tagName.toUpperCase() : null;
            currentStyle = Sys.UI.DomElement._getCurrentStyle(parent);

            // Firefox has its own quirk, which is that non-absolutely positioned elements that are
            // direct children of body get the body offset counted twice.
            if ((parent.offsetLeft || parent.offsetTop) &&
                !((tagName === "BODY") &&
                (!previousStyle || previousStyle.position !== "absolute"))) {

                offsetX += parent.offsetLeft;
                offsetY += parent.offsetTop;
            }

            // This code works around a difference in behavior in Opera and Safari which includes
            // clientLeft and clientTop in the computedstyle offset.
            if (previous !== null && currentStyle) {
                // This is to workaround a known bug in IE and Firefox:
                // <table> and <td> have strange behavior with offsetLeft/offsetTop and clientLeft/clientTop.
                // Say you have the following html: <table style="border-width:25px"><tr><td></table>
                // The offsetLeft and offsetTop for the <td> will be 25, but the client/borderLeft and
                // client/borderTop for the <table> will also be 25.  So if you count the client/borderLeft and
                // client/borderTop for the <table>, you will be double-counting the table border.
                if ((tagName !== "TABLE") && (tagName !== "TD") && (tagName !== "HTML")) {
                    offsetX += parseInt(currentStyle.borderLeftWidth) || 0;
                    offsetY += parseInt(currentStyle.borderTopWidth) || 0;
                }
                if (tagName === "TABLE" &&
                    (currentStyle.position === "relative" || currentStyle.position === "absolute")) {
                    offsetX += parseInt(currentStyle.marginLeft) || 0;
                    offsetY += parseInt(currentStyle.marginTop) || 0;
                }
            }
        }

        currentStyle = Sys.UI.DomElement._getCurrentStyle(element);
        var elementPosition = currentStyle ? currentStyle.position : null;
        // If an element is absolutely positioned, its parent's scroll should not be subtracted, except on Opera.
        if (!elementPosition || (elementPosition !== "absolute")) {
            // In Firefox and Safari, all parent's scroll values must be taken into account.
            // In IE, only the offset parent's because positioned elements are offset-parented to BODY and
            // don't need scroll substraction. Non-positioned elements are offset-parented to their parent,
            // which may be scrolled.
            for (parent = element.parentNode; parent; parent = parent.parentNode) {
                // In IE quirks mode, the <body> element has bogus values for scrollLeft and scrollTop.
                // So we do not use the scrollLeft and scrollTop for the <body> element.  This does not
                // break the standards mode behavior. (VSWhidbey 426176)
                // DevDiv Bugs 146697: tagName needs to be case insensitive to work with xhtml content type
                tagName = parent.tagName ? parent.tagName.toUpperCase() : null;

                if ((tagName !== "BODY") && (tagName !== "HTML") && (parent.scrollLeft || parent.scrollTop)) {

                    offsetX -= (parent.scrollLeft || 0);
                    offsetY -= (parent.scrollTop || 0);

                    currentStyle = Sys.UI.DomElement._getCurrentStyle(parent);
                    if (currentStyle) {
                        offsetX += parseInt(currentStyle.borderLeftWidth) || 0;
                        offsetY += parseInt(currentStyle.borderTopWidth) || 0;
                    }
                }
            }
        }
        return new Sys.UI.Point(offsetX, offsetY);
    }
}

Sys.UI.DomElement.isDomElement = function(obj) {
    /// <summary>Determines if the given argument is a DOM element.</summary>
    /// <param name="obj"></param>
    /// <returns type="Boolean">True if the object is a DOM element, otherwise false.</returns>
    return Sys._isDomElement(obj);
}

Sys.UI.DomElement.removeCssClass = function(element, className) {
    /// <summary>Removes a CSS class from an element.</summary>
    /// <param name="element" domElement="true"/>
    /// <param name="className" type="String">The name of the CSS class to remove.</param>
    var currentClassName = ' ' + element.className + ' ';
    var index = currentClassName.indexOf(' ' + className + ' ');
    if (index >= 0) {
        element.className = (currentClassName.substr(0, index) + ' ' +
            currentClassName.substring(index + className.length + 1, currentClassName.length)).trim();
    }
}

Sys.UI.DomElement.resolveElement = function(elementOrElementId, containerElement) {
    /// <summary>Returns the element with the specified Id in the specified container, or the element if it is already an element.</summary>
    /// <param name="elementOrElementId" mayBeNull="true" />
    /// <param name="containerElement" domElement="true" optional="true" mayBeNull="true"/>
    /// <returns domElement="true"/>
    var el = elementOrElementId;
    if (!el) return null;
    if (typeof(el) === "string") {
        el = Sys.UI.DomElement.getElementById(el, containerElement);
        #if DEBUG
        if (!el) {
            throw Error.argument("elementOrElementId", String.format(Sys.Res.elementNotFound, elementOrElementId));
        }
        #endif
    }
    #if DEBUG
    else if(!Sys.UI.DomElement.isDomElement(el)) {
        throw Error.argument("elementOrElementId", Sys.Res.expectedElementOrId);
    }
    #endif
    return el;
}

Sys.UI.DomElement.raiseBubbleEvent = function(source, args) {
    /// <summary>Raises a bubble event.</summary>
    /// <param name="source" domElement="true">The DOM element that triggers the event.</param>
    /// <param name="args" type="Sys.EventArgs">The event arguments.</param>
    var target = source;
    while (target) {
        var control = target.control;
        if (control && control.onBubbleEvent && control.raiseBubbleEvent) {
            Sys.UI.DomElement._raiseBubbleEventFromControl(control, source, args);
            return;
        }
        target = target.parentNode;
    }
}

Sys.UI.DomElement._raiseBubbleEventFromControl = function(control, source, args) {
    if (!control.onBubbleEvent(source, args)) {
        control._raiseBubbleEvent(source, args);
    }
}

Sys.UI.DomElement.setLocation = function(element, x, y) {
    /// <summary>Sets the position of an element.</summary>
    /// <param name="element" domElement="true"/>
    /// <param name="x" type="Number" integer="true"/>
    /// <param name="y" type="Number" integer="true"/>
    var style = element.style;
    style.position = 'absolute';
    style.left = x + "px";
    style.top = y + "px";
}

Sys.UI.DomElement.toggleCssClass = function(element, className) {
    /// <summary>Toggles a CSS class on and off o an element.</summary>
    /// <param name="element" domElement="true"/>
    /// <param name="className" type="String">The name of the CSS class to toggle.</param>
    if (Sys.UI.DomElement.containsCssClass(element, className)) {
        Sys.UI.DomElement.removeCssClass(element, className);
    }
    else {
        Sys.UI.DomElement.addCssClass(element, className);
    }
}

Sys.UI.DomElement.getVisibilityMode = function(element) {
    /// <param name="element" domElement="true"/>
    /// <returns type="Sys.UI.VisibilityMode"/>
    return (element._visibilityMode === Sys.UI.VisibilityMode.hide) ?
        Sys.UI.VisibilityMode.hide :
        Sys.UI.VisibilityMode.collapse;
}
Sys.UI.DomElement.setVisibilityMode = function(element, value) {
    /// <param name="element" domElement="true"/>
    /// <param name="value" type="Sys.UI.VisibilityMode"/>
    Sys.UI.DomElement._ensureOldDisplayMode(element);
    if (element._visibilityMode !== value) {
        element._visibilityMode = value;
        if (Sys.UI.DomElement.getVisible(element) === false) {
            if (element._visibilityMode === Sys.UI.VisibilityMode.hide) {
                element.style.display = element._oldDisplayMode;
            }
            else {
                element.style.display = 'none';
            }
        }
        element._visibilityMode = value;
    }
}

Sys.UI.DomElement.getVisible = function(element) {
    /// <param name="element" domElement="true"/>
    /// <returns type="Boolean"/>
    var style = element.currentStyle || Sys.UI.DomElement._getCurrentStyle(element);
    if (!style) return true;
    return (style.visibility !== 'hidden') && (style.display !== 'none');
}
Sys.UI.DomElement.setVisible = function(element, value) {
    /// <param name="element" domElement="true"/>
    /// <param name="value" type="Boolean"/>
    if (value !== Sys.UI.DomElement.getVisible(element)) {
        Sys.UI.DomElement._ensureOldDisplayMode(element);
        element.style.visibility = value ? 'visible' : 'hidden';
        if (value || (element._visibilityMode === Sys.UI.VisibilityMode.hide)) {
            element.style.display = element._oldDisplayMode;
        }
        else {
            element.style.display = 'none';
        }
    }
}

Sys.UI.DomElement._ensureOldDisplayMode = function(element) {
    if (!element._oldDisplayMode) {
        var style = element.currentStyle || Sys.UI.DomElement._getCurrentStyle(element);
        element._oldDisplayMode = style ? style.display : null;
        if (!element._oldDisplayMode || element._oldDisplayMode === 'none') {
            // Default is different depending on the tag name (omitting deprecated and non-standard tags)
            switch(element.tagName.toUpperCase()) {
                case 'DIV': case 'P': case 'ADDRESS': case 'BLOCKQUOTE': case 'BODY': case 'COL':
                case 'COLGROUP': case 'DD': case 'DL': case 'DT': case 'FIELDSET': case 'FORM':
                case 'H1': case 'H2': case 'H3': case 'H4': case 'H5': case 'H6': case 'HR':
                case 'IFRAME': case 'LEGEND': case 'OL': case 'PRE': case 'TABLE': case 'TD':
                case 'TH': case 'TR': case 'UL':
                    element._oldDisplayMode = 'block';
                    break;
                case 'LI':
                    element._oldDisplayMode = 'list-item';
                    break;
                default:
                    element._oldDisplayMode = 'inline';
            }
        }
    }
}

Sys.UI.DomElement._getWindow = function(element) {
    var doc = element.ownerDocument || element.document || element;
    return doc.defaultView || doc.parentWindow;
}

Sys.UI.DomElement._getCurrentStyle = function(element) {
    if (element.nodeType === 3) return null;
    var w = Sys.UI.DomElement._getWindow(element);
    if (element.documentElement) element = element.documentElement;
    var computedStyle = (w && (element !== w) && w.getComputedStyle) ?
        w.getComputedStyle(element, null) :
        element.currentStyle || element.style;
    if (!computedStyle && (Sys.Browser.agent === Sys.Browser.Safari) && element.style) {
        // Safari has an interesting bug (fixed in WebKit) where an element with display:none will have a null computed style.
        var oldDisplay = element.style.display;
        var oldPosition = element.style.position;
        element.style.position = 'absolute';
        element.style.display = 'block';
        var style = w.getComputedStyle(element, null);
        element.style.display = oldDisplay;
        element.style.position = oldPosition;
        // Need a clone as the display property may be wrong and can't be fixed on the original object.
        computedStyle = {};
        for (var n in style) {
            computedStyle[n] = style[n];
        }
        computedStyle.display = 'none';
    }
    return computedStyle;
}
