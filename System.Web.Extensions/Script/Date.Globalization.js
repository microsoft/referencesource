#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="Date.Globalization.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif

Date._appendPreOrPostMatch = function(preMatch, strBuilder) {
    // appends pre- and post- token match strings while removing escaped characters.
    // Returns a single quote count which is used to determine if the token occurs
    // in a string literal.
    var quoteCount = 0;
    var escaped = false;
    for (var i = 0, il = preMatch.length; i < il; i++) {
        var c = preMatch.charAt(i);
        switch (c) {
        case '\'':
            if (escaped) strBuilder.append("'");
            else quoteCount++;
            escaped = false;
            break;
        case '\\':
            if (escaped) strBuilder.append("\\");
            escaped = !escaped;
            break;
        default:
            strBuilder.append(c);
            escaped = false;
            break;
        }
    }
    return quoteCount;
}

Date._expandFormat = function(dtf, format) {
    // expands unspecified or single character date formats into the full pattern.
    if (!format) {
        format = "F";
    }
    var len = format.length;
    if (len === 1) {
        switch (format) {
        case "d":
            return dtf.ShortDatePattern;
        case "D":
            return dtf.LongDatePattern;
        case "t":
            return dtf.ShortTimePattern;
        case "T":
            return dtf.LongTimePattern;
        case "f":
            return dtf.LongDatePattern + " " + dtf.ShortTimePattern;
        case "F":
            return dtf.FullDateTimePattern;
        case "M": case "m":
            return dtf.MonthDayPattern;
        case "s":
            return dtf.SortableDateTimePattern;
        case "Y": case "y":
            return dtf.YearMonthPattern;
        default:
            throw Error.format(Sys.Res.formatInvalidString);
        }
    }
    else if ((len === 2) && (format.charAt(0) === "%")) {
        // Dev10 580024: %X escape format -- intended as a custom format string that
        // is only one character, not a built-in format.
        format = format.charAt(1);
    }
    return format;
}

Date._expandYear = function(dtf, year) {
    // expands 2-digit year into 4 digits.
    var now = new Date(),
        era = Date._getEra(now);
    if (year < 100) {
        var curr = Date._getEraYear(now, dtf, era);
        year += curr - (curr % 100);
        if (year > dtf.Calendar.TwoDigitYearMax) {
            year -= 100;
        }
    }
    return year;
}

Date._getEra = function(date, eras) {
    if (!eras) return 0;
    var start, ticks = date.getTime();
    for (var i = 0, l = eras.length; i < l; i += 4) {
        start = eras[i+2];
        if ((start === null) || (ticks >= start)) {
            return i;
        }
    }
    return 0;
}
Date._getEraYear = function(date, dtf, era, sortable) {
    var year = date.getFullYear();
    if (!sortable && dtf.eras) {
        // convert normal gregorian year to era-shifted gregorian
        // year by subtracting the era offset
        year -= dtf.eras[era + 3];
    }    
    return year;
}

Date._getParseRegExp = function(dtf, format) {
    // converts a format string into a regular expression with groups that
    // can be used to extract date fields from a date string.
    // check for a cached parse regex.
    if (!dtf._parseRegExp) {
        dtf._parseRegExp = {};
    }
    else if (dtf._parseRegExp[format]) {
        return dtf._parseRegExp[format];
    }

    // expand single digit formats, then escape regular expression characters.
    var expFormat = Date._expandFormat(dtf, format);
    expFormat = expFormat.replace(/([\^\$\.\*\+\?\|\[\]\(\)\{\}])/g, "\\\\$1");

    var regexp = new Sys.StringBuilder("^");
    var groups = [];
    var index = 0;
    var quoteCount = 0;
    var tokenRegExp = Date._getTokenRegExp();
    var match;

    // iterate through each date token found.
    while ((match = tokenRegExp.exec(expFormat)) !== null) {
        var preMatch = expFormat.slice(index, match.index);
        index = tokenRegExp.lastIndex;

        // don't replace any matches that occur inside a string literal.
        quoteCount += Date._appendPreOrPostMatch(preMatch, regexp);
        if ((quoteCount%2) === 1) {
            regexp.append(match[0]);
            continue;
        }

        // add a regex group for the token.
        switch (match[0]) {
            case 'dddd': case 'ddd':
            case 'MMMM': case 'MMM':
            case 'gg': case 'g':
                regexp.append("(\\D+)");
                break;
            case 'tt': case 't':
                regexp.append("(\\D*)");
                break;
            case 'yyyy':
                regexp.append("(\\d{4})");
                break;
            case 'fff':
                regexp.append("(\\d{3})");
                break;
            case 'ff':
                regexp.append("(\\d{2})");
                break;
            case 'f':
                regexp.append("(\\d)");
                break;
            case 'dd': case 'd':
            case 'MM': case 'M':
            case 'yy': case 'y':
            case 'HH': case 'H':
            case 'hh': case 'h':
            case 'mm': case 'm':
            case 'ss': case 's':
                regexp.append("(\\d\\d?)");
                break;
            case 'zzz':
                regexp.append("([+-]?\\d\\d?:\\d{2})");
                break;
            case 'zz': case 'z':
                regexp.append("([+-]?\\d\\d?)");
                break;
            case '/':
                regexp.append("(\\" + dtf.DateSeparator + ")");
                break;
            #if DEBUGINTERNAL
            default:
                Sys.Debug.fail("Invalid date format pattern");
            #endif
        }
        Array.add(groups, match[0]);
    }
    Date._appendPreOrPostMatch(expFormat.slice(index), regexp);
    regexp.append("$");
    // allow whitespace to differ when matching formats.
    var regexpStr = regexp.toString().replace(/\s+/g, "\\s+");
    var parseRegExp = {'regExp': regexpStr, 'groups': groups};
    // cache the regex for this format.
    dtf._parseRegExp[format] = parseRegExp;
    return parseRegExp;
}

Date._getTokenRegExp = function() {
    // regular expression for matching dateTime tokens in format strings.
    return /\/|dddd|ddd|dd|d|MMMM|MMM|MM|M|yyyy|yy|y|hh|h|HH|H|mm|m|ss|s|tt|t|fff|ff|f|zzz|zz|z|gg|g/g;
}

Date.parseLocale = function(value, formats) {
    /// <summary>Creates a date from a locale-specific string representation.</summary>
    /// <param name="value" type="String">A locale-specific string that can parse to a date.</param>
    /// <param name="formats" parameterArray="true" optional="true" mayBeNull="true">Custom formats to match.</param>
    /// <returns type="Date"/>
    return Date._parse(value, Sys.CultureInfo.CurrentCulture, arguments);
}

Date.parseInvariant = function(value, formats) {
    /// <summary>Creates a date from its string representation.</summary>
    /// <param name="value" type="String">A string that can parse to a date.</param>
    /// <param name="formats" parameterArray="true" optional="true" mayBeNull="true">Custom formats to match.</param>
    /// <returns type="Date"/>
    return Date._parse(value, Sys.CultureInfo.InvariantCulture, arguments);
}

Date._parse = function(value, cultureInfo, args) {
    // args is a params array with value as the first item, followed by custom formats.
    // try parse with custom formats.
    var i, l, date, format, formats, custom = false;
    for (i = 1, l = args.length; i < l; i++) {
        format = args[i];
        if (format) {
            custom = true;
            date = Date._parseExact(value, format, cultureInfo);
            if (date) return date;
        }
    }
    // try parse with culture formats.
    if (! custom) {
        formats = cultureInfo._getDateTimeFormats();
        for (i = 0, l = formats.length; i < l; i++) {
            date = Date._parseExact(value, formats[i], cultureInfo);
            if (date) return date;
        }
    }
    return null;
}

Date._parseExact = function(value, format, cultureInfo) {
    // try to parse the date string value by matching against the format string
    // while using the specified culture for date field names.
    value = value.trim();
    var dtf = cultureInfo.dateTimeFormat,
    // convert date formats into regular expressions with groupings.
    // use the regexp to determine the input format and extract the date fields.
        parseInfo = Date._getParseRegExp(dtf, format),
        match = new RegExp(parseInfo.regExp).exec(value);
    // DevDiv 124696: Return null to avoid Firefox warning "does not always return a value"
    if (match === null) return null;
    
    // found a date format that matches the input.
    var groups = parseInfo.groups,
        era = null, year = null, month = null, date = null, weekDay = null,
        hour = 0, hourOffset, min = 0, sec = 0, msec = 0, tzMinOffset = null,
        pmHour = false;
    // iterate the format groups to extract and set the date fields.
    for (var j = 0, jl = groups.length; j < jl; j++) {
        var matchGroup = match[j+1];
        if (matchGroup) {
            switch (groups[j]) {
                case 'dd': case 'd':
                    // Day of month.
                    date = parseInt(matchGroup, 10);
                    // check that date is generally in valid range, also checking overflow below.
                    if ((date < 1) || (date > 31)) return null;
                    break;
                case 'MMMM':
                    // Month, long name.
                    month = cultureInfo._getMonthIndex(matchGroup);
                    if ((month < 0) || (month > 11)) return null;
                    break;
                case 'MMM':
                    // Month, short name.
                    month = cultureInfo._getAbbrMonthIndex(matchGroup);
                    if ((month < 0) || (month > 11)) return null;
                    break;
                case 'M': case 'MM':
                    // Month.
                    month = parseInt(matchGroup, 10) - 1;
                    if ((month < 0) || (month > 11)) return null;
                    break;
                case 'y': case 'yy':
                    // 2-Digit Year.
                    year = Date._expandYear(dtf,parseInt(matchGroup, 10));
                    if ((year < 0) || (year > 9999)) return null;
                    break;
                case 'yyyy':
                    // 4-Digit Year.
                    year = parseInt(matchGroup, 10);
                    if ((year < 0) || (year > 9999)) return null;
                    break;
                case 'h': case 'hh':
                    // Hours (12-hour clock).
                    hour = parseInt(matchGroup, 10);
                    if (hour === 12) hour = 0;
                    if ((hour < 0) || (hour > 11)) return null;
                    break;
                case 'H': case 'HH':
                    // Hours (24-hour clock).
                    hour = parseInt(matchGroup, 10);
                    if ((hour < 0) || (hour > 23)) return null;
                    break;
                case 'm': case 'mm':
                    // Minutes.
                    min = parseInt(matchGroup, 10);
                    if ((min < 0) || (min > 59)) return null;
                    break;
                case 's': case 'ss':
                    // Seconds.
                    sec = parseInt(matchGroup, 10);
                    if ((sec < 0) || (sec > 59)) return null;
                    break;
                case 'tt': case 't':
                    // AM/PM designator.
                    var upperToken = matchGroup.toUpperCase();
                    pmHour = (upperToken === dtf.PMDesignator.toUpperCase());
                    if (!pmHour && (upperToken !== dtf.AMDesignator.toUpperCase())) return null;
                    break;
                case 'f':
                    // Deciseconds.
                    msec = parseInt(matchGroup, 10) * 100;
                    if ((msec < 0) || (msec > 999)) return null;
                    break;
                case 'ff':
                    // Centiseconds.
                    msec = parseInt(matchGroup, 10) * 10;
                    if ((msec < 0) || (msec > 999)) return null;
                    break;
                case 'fff':
                    // Milliseconds.
                    msec = parseInt(matchGroup, 10);
                    if ((msec < 0) || (msec > 999)) return null;
                    break;
                case 'dddd':
                    // Day of week.
                    weekDay = cultureInfo._getDayIndex(matchGroup);
                    if ((weekDay < 0) || (weekDay > 6)) return null;
                    break;
                case 'ddd':
                    // Day of week.
                    weekDay = cultureInfo._getAbbrDayIndex(matchGroup);
                    if ((weekDay < 0) || (weekDay > 6)) return null;
                    break;
                case 'zzz':
                    // Time zone offset in +/- hours:min.
                    var offsets = matchGroup.split(/:/);
                    if (offsets.length !== 2) return null;
                    hourOffset = parseInt(offsets[0], 10);
                    if ((hourOffset < -12) || (hourOffset > 13)) return null;
                    var minOffset = parseInt(offsets[1], 10);
                    if ((minOffset < 0) || (minOffset > 59)) return null;
                    tzMinOffset = (hourOffset * 60) + (matchGroup.startsWith('-')? -minOffset : minOffset);
                    break;
                case 'z': case 'zz':
                    // Time zone offset in +/- hours.
                    hourOffset = parseInt(matchGroup, 10);
                    if ((hourOffset < -12) || (hourOffset > 13)) return null;
                    tzMinOffset = hourOffset * 60;
                    break;
                case 'g': case 'gg':
                    var eraName = matchGroup;
                    if (!eraName || !dtf.eras) return null;
                    eraName = eraName.toLowerCase().trim();
                    for (var i = 0, l = dtf.eras.length; i < l; i += 4) {
                        if (eraName === dtf.eras[i + 1].toLowerCase()) {
                            era = i;
                            break;
                        }
                    }
                    // could not find an era with that name
                    if (era === null) return null;
                    break;
            }
        }
    }
    var result = new Date(), defaultYear, convert = dtf.Calendar.convert;
    if (convert) {
        defaultYear = convert.fromGregorian(result)[0];
    }
    else {
        defaultYear = result.getFullYear();
    }
    if (year === null) {
        year = defaultYear;
    }
    else if (dtf.eras) {
        // year must be shifted to normal gregorian year
        // but not if year was not specified, its already normal gregorian
        // per the main if clause above.
        year += dtf.eras[(era || 0) + 3];
    }
    // Dev10 747832: set default day and month to 1 and January to be consistent with .Net behavior.
    if (month === null) {
        month = 0;
    }
    if (date === null) {
        date = 1;
    }
    // now have year, month, and date, but in the culture's calendar.
    // convert to gregorian if necessary
    if (convert) {
        result = convert.toGregorian(year, month, date);
        // conversion failed, must be an invalid match
        if (result === null) return null;
    }
    else {
        // have to set year, month and date together to avoid overflow based on current date.
        result.setFullYear(year, month, date);
        // check to see if date overflowed for specified month (only checked 1-31 above).
        if (result.getDate() !== date) return null;
        // invalid day of week.
        if ((weekDay !== null) && (result.getDay() !== weekDay)) {
            return null;
        }
    }
    // if pm designator token was found make sure the hours fit the 24-hour clock.
    if (pmHour && (hour < 12)) {
        hour += 12;
    }
    result.setHours(hour, min, sec, msec);
    if (tzMinOffset !== null) {
        // adjust timezone to utc before applying local offset.
        var adjustedMin = result.getMinutes() - (tzMinOffset + result.getTimezoneOffset());
        // Safari limits hours and minutes to the range of -127 to 127.  We need to use setHours
        // to ensure both these fields will not exceed this range.  adjustedMin will range
        // somewhere between -1440 and 1500, so we only need to split this into hours.
        result.setHours(result.getHours() + parseInt(adjustedMin/60, 10), adjustedMin%60);
    }
    return result;
}

Date.prototype.format = function(format) {
    /// <summary>Format a date using the invariant culture.</summary>
    /// <param name="format" type="String">Format string.</param>
    /// <returns type="String">Formatted date.</returns>
    return this._toFormattedString(format, Sys.CultureInfo.InvariantCulture);
}

Date.prototype.localeFormat = function(format) {
    /// <summary>Format a date using the current culture.</summary>
    /// <param name="format" type="String">Format string.</param>
    /// <returns type="String">Formatted date.</returns>
    return this._toFormattedString(format, Sys.CultureInfo.CurrentCulture);
}

Date.prototype._toFormattedString = function(format, cultureInfo) {
    var dtf = cultureInfo.dateTimeFormat,
        convert = dtf.Calendar.convert;
    if (!format || !format.length || (format === 'i')) {
        if (cultureInfo && cultureInfo.name.length) {
            if (convert) {
                // non-gregorian calendar, so we cannot use built-in toLocaleString()
                return this._toFormattedString(dtf.FullDateTimePattern, cultureInfo);
            }
            else {
                var eraDate = new Date(this.getTime());
                var era = Date._getEra(this, dtf.eras);
                eraDate.setFullYear(Date._getEraYear(this, dtf, era));
                return eraDate.toLocaleString();
            }
        }
        else {
            return this.toString();
        }
    }

    var eras = dtf.eras,
        sortable = (format === "s");
    format = Date._expandFormat(dtf, format);

    // Start with an empty string
    var ret = new Sys.StringBuilder();
    var hour;

    function addLeadingZero(num) {
        if (num < 10) {
            return '0' + num;
        }
        return num.toString();
    }

    function addLeadingZeros(num) {
        if (num < 10) {
            return '00' + num;
        }
        if (num < 100) {
            return '0' + num;
        }
        return num.toString();
    }
    function padYear(year) {
        if (year < 10) {
            return '000' + year;
        }
        else if (year < 100) {
            return '00' + year;
        }
        else if (year < 1000) {
            return '0' + year;
        }
        return year.toString();
    }
    
    var foundDay, checkedDay, dayPartRegExp = /([^d]|^)(d|dd)([^d]|$)/g;
    function hasDay() {
        if (foundDay || checkedDay) {
            return foundDay;
        }
        foundDay = dayPartRegExp.test(format);
        checkedDay = true;
        return foundDay;
    }
    
    var quoteCount = 0,
        tokenRegExp = Date._getTokenRegExp(),
        converted;
    if (!sortable && convert) {
        converted = convert.fromGregorian(this);
    }
    for (;;) {

        // Save the current index
        var index = tokenRegExp.lastIndex;

        // Look for the next pattern
        var ar = tokenRegExp.exec(format);

        // Append the text before the pattern (or the end of the string if not found)
        var preMatch = format.slice(index, ar ? ar.index : format.length);
        quoteCount += Date._appendPreOrPostMatch(preMatch, ret);

        if (!ar) break;

        // do not replace any matches that occur inside a string literal.
        if ((quoteCount%2) === 1) {
            ret.append(ar[0]);
            continue;
        }
        
        function getPart(date, part) {
            if (converted) {
                return converted[part];
            }
            switch (part) {
                case 0: return date.getFullYear();
                case 1: return date.getMonth();
                case 2: return date.getDate();
            }
        }

        switch (ar[0]) {
        case "dddd":
            // Day of the week, using the full name
            ret.append(dtf.DayNames[this.getDay()]);
            break;
        case "ddd":
            //Day of the week, as a three-letter abbreviation
            ret.append(dtf.AbbreviatedDayNames[this.getDay()]);
            break;
        case "dd":
            foundDay = true;
            // Day of month, with leading zero for single-digit days
            ret.append(addLeadingZero(getPart(this, 2)));
            break;
        case "d":
            foundDay = true;
            // Day of month, without leading zero for single-digit days
            ret.append(getPart(this, 2));
            break;
        case "MMMM":
            // Month, using the full name
            ret.append((dtf.MonthGenitiveNames && hasDay())
                ? dtf.MonthGenitiveNames[getPart(this, 1)]
                : dtf.MonthNames[getPart(this, 1)]);
            break;
        case "MMM":
            // Month, as a three-letter abbreviation
            ret.append((dtf.AbbreviatedMonthGenitiveNames && hasDay())
                ? dtf.AbbreviatedMonthGenitiveNames[getPart(this, 1)]
                : dtf.AbbreviatedMonthNames[getPart(this, 1)]);
            break;
        case "MM":
            // Month, as digits, with leading zero for single-digit months
            ret.append(addLeadingZero(getPart(this, 1) + 1));
            break;
        case "M":
            // Month, as digits, with no leading zero for single-digit months
            ret.append(getPart(this, 1) + 1);
            break;
        case "yyyy":
            // Year represented by four full digits
            ret.append(padYear(converted ? converted[0] : Date._getEraYear(this, dtf, Date._getEra(this, eras), sortable)));
            break;
        case "yy":
            // Year, as two digits, with leading zero for years less than 10
            ret.append(addLeadingZero((converted ? converted[0] : Date._getEraYear(this, dtf, Date._getEra(this, eras), sortable)) % 100));
            break;
        case "y":
            // Year, as two digits, but with no leading zero for years less than 10
            ret.append((converted ? converted[0] : Date._getEraYear(this, dtf, Date._getEra(this, eras), sortable)) % 100);
            break;
        case "hh":
            // Hours with leading zero for single-digit hours, using 12-hour clock
            hour = this.getHours() % 12;
            if (hour === 0) hour = 12;
            ret.append(addLeadingZero(hour));
            break;
        case "h":
            // Hours with no leading zero for single-digit hours, using 12-hour clock
            hour = this.getHours() % 12;
            if (hour === 0) hour = 12;
            ret.append(hour);
            break;
        case "HH":
            // Hours with leading zero for single-digit hours, using 24-hour clock
            ret.append(addLeadingZero(this.getHours()));
            break;
        case "H":
            // Hours with no leading zero for single-digit hours, using 24-hour clock
            ret.append(this.getHours());
            break;
        case "mm":
            // Minutes with leading zero  for single-digit minutes
            ret.append(addLeadingZero(this.getMinutes()));
            break;
        case "m":
            // Minutes with no leading zero  for single-digit minutes
            ret.append(this.getMinutes());
            break;
        case "ss":
            // Seconds with leading zero for single-digit seconds
            ret.append(addLeadingZero(this.getSeconds()));
            break;
        case "s":
            // Seconds with no leading zero for single-digit seconds
            ret.append(this.getSeconds());
            break;
        case "tt":
            // Multicharacter am/pm indicator
            ret.append((this.getHours() < 12) ? dtf.AMDesignator : dtf.PMDesignator);
            break;
        case "t":
            // One character am/pm indicator ("a" or "p")
            ret.append(((this.getHours() < 12) ? dtf.AMDesignator : dtf.PMDesignator).charAt(0));
            break;
        case "f":
            // Deciseconds
            ret.append(addLeadingZeros(this.getMilliseconds()).charAt(0));
            break;
        case "ff":
            // Centiseconds
            ret.append(addLeadingZeros(this.getMilliseconds()).substr(0, 2));
            break;
        case "fff":
            // Milliseconds
            ret.append(addLeadingZeros(this.getMilliseconds()));
            break;
        case "z":
            // Time zone offset, no leading zero
            hour = this.getTimezoneOffset() / 60;
            ret.append(((hour <= 0) ? '+' : '-') + Math.floor(Math.abs(hour)));
            break;
        case "zz":
            // Time zone offset with leading zero
            hour = this.getTimezoneOffset() / 60;
            ret.append(((hour <= 0) ? '+' : '-') + addLeadingZero(Math.floor(Math.abs(hour))));
            break;
        case "zzz":
            // Time zone offset with leading zero
            hour = this.getTimezoneOffset() / 60;
            ret.append(((hour <= 0) ? '+' : '-') + addLeadingZero(Math.floor(Math.abs(hour))) +
                // Dev10 580009: Server has hard coded ":" separator, rather than using dtf.TimeSeparator
                // Repeated here for consistency, plus ":" was already assumed in date parsing.
                ":" + addLeadingZero(Math.abs(this.getTimezoneOffset() % 60)));
            break;
        case "g":
        case "gg":
            if (dtf.eras) {
                ret.append(dtf.eras[Date._getEra(this, eras) + 1]);
            }
            break;
        case "/":
            ret.append(dtf.DateSeparator);
            break;
        #if DEBUGINTERNAL
        default:
            Sys.Debug.fail("Invalid date format pattern");
        #endif
        }
    }
    return ret.toString();
}
