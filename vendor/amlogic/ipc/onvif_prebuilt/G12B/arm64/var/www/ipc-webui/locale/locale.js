"use strict";

var getUrlParameterByName = function (name, url) {
    if (!url) url = window.location.href;
    name = name.replace(/[\[\]]/g, '\\$&');
    var regex = new RegExp('[?&]' + name + '(=([^&#]*)|&|#|$)'),
        results = regex.exec(url);
    if (!results) return null;
    if (!results[2]) return '';
    return decodeURIComponent(results[2].replace(/\+/g, ' '));
};


var load_translation = function(lang) {
    let ret = null;
    $.ajax({
        async: false
        , url: "/locale/language." + lang + ".js"
        , dataType: "script"
        , cache: true
        , statusCode: {404: function() {
            console.log("language file not found");
        }}
        , success: function(data) {
            ret = load_translation();
        }
    });
    return ret;
};

var gettext = function(text) {
    text = text.trim();
    if (translation) {
        return translation[text] || text;
    } else {
        return text;
    }
};

var translate_html = function() {
  let elements = document.getElementsByClassName('locale');
  for (let i = 0; i < elements.length; i++) {
      elements[i].textContent = gettext(elements[i].textContent);
  }
};

var translate_inner_html = function(elementId) {
    let reg = new RegExp("\\[([^\\[\\]]*?)\\]", 'igm');
    let e = document.getElementById(elementId);
    let html = e.innerHTML;
    e.innerHTML = html.replace(reg, function (node, key) {
        return gettext(key);
    });
};

var get_browser_language = function() {
    let lang = getUrlParameterByName('lang');

    if (!lang) {
        lang = (navigator.language || navigator.userLanguage || 'en_US').replace('-', '_');
        if (lang === 'en') lang = 'en_US';
    }

    return lang;
};

var language = get_browser_language();
var translation = load_translation(language);
