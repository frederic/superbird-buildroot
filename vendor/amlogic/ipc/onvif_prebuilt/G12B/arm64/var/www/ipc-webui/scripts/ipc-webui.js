"use strict";

function adaptIFrame(id, parentid = null, offset_h = 0){
    let ifm = document.getElementById(id);
    if (! parentid === parentid) {
        ifm.height = window.parent.$(parentid).height();
        ifm.width= window.parent.$(parentid).width();
    } else {
        ifm.height = document.documentElement.clientHeight - offset_h;
        ifm.width = document.documentElement.clientWidth;
    }
}

function getFormData($form){
    let unindexed_array = $form.serializeArray();
    let indexed_array = {};

    $.map(unindexed_array, function(n, i){
        indexed_array[n['name']] = n['value'];
    });

    return indexed_array;
}

function option_clear(e) {
    e.options.length = 0;
}
function option_set_value(e, i, val, text) {
    e.options[i] = new Option(text, val, false, false);
}
function option_set_selected(e, i) {
    e.options[i].selected = true;
}
function option_value_exists(e, v) {
    let ret = -1;
    Object.values(e).every(function(o, i) {
        if (v === o.text) {
            ret = i;
            return false;
        }
        return true;
    });
    return ret;
}

function option_set_values(e, list, value_as_text= false) {
    option_clear(e);
    list.forEach(function(l,i) {
        if (value_as_text) {
            option_set_value(e, i, l, l);
        } else {
            option_set_value(e, i, l.val, l.text);
        }
    });
}

function basename(str) {
    var idx = str.lastIndexOf('/')
    idx = idx > -1 ? idx : str.lastIndexOf('\\')
    if (idx < 0) {
        return str
    }
    return str.substring(idx + 1);
}

//convert color between hex and rgba
var rgbToHex = function (rgb) {
    var hex = Number(rgb).toString(16);
    if (hex.length < 2) {
        hex = "0" + hex;
    }
    return hex;
};
var colorhex = function (r, g, b, a) {
    var red = rgbToHex(r);
    var green = rgbToHex(g);
    var blue = rgbToHex(b);
    var alpha = rgbToHex(a);
    return "0x" + red + green + blue + alpha;
};
var rgba2hex = function (color) {
    let rgba = color.split(new RegExp('[,()]', 'g'));
    var r = rgba[1];
    var g = rgba[2];
    var b = rgba[3];
    var a = Math.round(rgba[4] * 255);
    return colorhex(r, g, b, a);
};

var hex2rgba = function (hex) {
    if (hex === '') return 'rgba(0,0,0,0)';
    return 'rgba(' + parseInt('0x' + hex.slice(2, 4)) + ',' + parseInt('0x' + hex.slice(4, 6)) + ','
        + parseInt('0x' + hex.slice(6, 8)) + ',' + parseInt('0x' + hex.slice(8, 10)) + ')';
};

var call_remote_function = function(url, func) {
    let result = null;
    $.ajax({
        type: 'get'
        , url: url
        , async: false
        ,dataType: 'json'
        ,data: {function: func}
        ,success: function(data) {
            result = data;
        }
    });
    return result;
};

function check_input_range(e) {
    let min = $(this).attr('min');
    let max = $(this).attr('max');
    if ($(this).val() === '' || $(this).val() < min * 1) {
        $(this).val(min);
    }
    if (max !== '' && $(this).val() > max * 1) {
        $(this).val(max);
    }
}

function setCookie(cname, cvalue, exdays) {
    let d = new Date();
    d.setTime(d.getTime() + (exdays * 24 * 60 * 60 * 1000));
    let expires = "expires="+d.toUTCString();
    document.cookie = cname + "=" + cvalue + ";" + expires + ";path=/";
}

function getCookie(cname) {
    let name = cname + "=";
    let ca = document.cookie.split(';');
    for(let i = 0; i < ca.length; i++) {
        let c = ca[i];
        while (c.charAt(0) === ' ') {
            c = c.substring(1);
        }
        if (c.indexOf(name) === 0) {
            return c.substring(name.length, c.length);
        }
    }
    return "";
}

function formatSizeUnits(bytes){
    if      (bytes >= 1073741824) { bytes = (bytes / 1073741824).toFixed(2) + " GB"; }
    else if (bytes >= 1048576)    { bytes = (bytes / 1048576).toFixed(2) + " MB"; }
    else if (bytes >= 1024)       { bytes = (bytes / 1024).toFixed(2) + " KB"; }
    else if (bytes > 1)           { bytes = bytes + " bytes"; }
    else if (bytes == 1)          { bytes = bytes + " byte"; }
    else                          { bytes = "0 bytes"; }
    return bytes;
}

function ajax_download(url, input_name, data) {
    var $iframe,
        iframe_doc,
        iframe_html;

    if (($iframe = $('#download_iframe')).length > 0) {
        for (let i = 0; i < $iframe.length; i++) {
            $iframe[i].parentNode.removeChild($iframe[i]);
        }
    }

    $iframe = $("<iframe id='download_iframe'" +
        " style='display: none' src='about:blank'></iframe>"
    ).appendTo("body");

    iframe_doc = $iframe[0].contentWindow || $iframe[0].contentDocument;
    if (iframe_doc.document) {
        iframe_doc = iframe_doc.document;
    }

    iframe_html = "<html><head></head><body><form method='POST' action='" +
        url +"'>" + "<input type=hidden name='" + input_name + "' value='" +
        data +"'/></form>" + "</body></html>";

    iframe_doc.open();
    iframe_doc.write(iframe_html);
    $(iframe_doc).find('form').submit();
}