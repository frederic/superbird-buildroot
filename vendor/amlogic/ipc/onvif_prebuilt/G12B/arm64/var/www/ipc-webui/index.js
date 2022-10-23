window.onresize = function(){
    adaptIFrame('iframe_main', null, 64);
};

layui.use('element', function () {
    var element = layui.element;

    function refresh_history_url(page, lang, reload) {
        let new_url = "http://" + location.host + location.pathname + "?page=" + page + "&lang=" + lang;
        history.pushState(null, null, new_url);
        if (reload) {
            window.location.reload();
        }
    }

    function getURLById(id) {
        let param = '#' + id;
        let url = $(param).data("url");
        $(param).parent().addClass("layui-this");
        return url;
    }

    let page_id = getUrlParameterByName("page");
    let browser_lang = get_browser_language();
    if (page_id === null) {
        page_id = "face";
        var new_url = "http://" + location.host + location.pathname + "?page=" + page_id + "&lang=" + browser_lang;
        history.pushState(null, null, new_url);
    }
    $('#lang').val(browser_lang);

    let src = getURLById(page_id);
    if (src !== null) {
        src = src + "?lang=" + browser_lang;
        $("#iframe_main").attr("src", src);
    }

    element.on('nav(treenav)', function (elem) {
        if (typeof (elem.attr("id")) !== 'undefined') {
            page_id = elem.attr("id");
            refresh_history_url(page_id, browser_lang, false);
            let src = elem.attr("data-url") + "?lang=" + browser_lang;
            $("#iframe_main").attr("src", src);
        }
    });

    $("#lang").change(function () {
        let lang = $("#lang").val();
        refresh_history_url(page_id, lang, true);
    });

});

layui.use('layer', function () {
    var $ = layui.jquery, layer = layui.layer;

    $(document).ready(function () {
        $('#captureScreen').click(function (e) {
            e.preventDefault();
            let layer_id = layer.load(0, {time: 10 * 1000});
            $.ajax({
                url: "image_capture.php"
                , type: "post"
                , dataType: "json"
                , complete: function () {
                    layer.close(layer_id);
                }
                , success: function(response) {
                    if (response.status === 'success') {
                        let id = 'iframe-screen-capture-download';
                        let ifrm = document.getElementById(id);
                        if (ifrm === null) {
                            ifrm = document.createElement('iframe');
                            ifrm.id = id;
                            ifrm.hidden = true;
                            document.body.appendChild(ifrm);
                        }
                        ifrm.src = 'download.php?file=' + response.filename;
                    } else {
                        layer.msg(gettext('Failed to capture screen'));
                    }
                }
            });
        });
    });

    var previewButton = $('#showPreview'), showPreview = function () {
        var that = this;

        function resize_iframe(layero) {
            var title_height = layero.find('.layui-layer-title')[0].clientHeight;
            var iframe = layero.find('iframe');
            var cw = layero[0].clientWidth;
            var ch = layero[0].clientHeight - title_height;
            var rw = cw / 640;
            var rh = ch / 368;
            var w, h;
            if (rw > rh) {
                h = ch;
                w = ch * 640 / 368;
            } else {
                w = cw;
                h = cw * 368 / 640;
            }
            var t = (ch - h) / 2;
            var l = (cw - w) / 2;
            iframe.css({position: 'relative', top: t, left: l, width: w, height: h});
        }

        layer.open({
            type: 2
            , title: gettext("Video Preview")
            , shade: 0
            , id: "VideoPreview"
            , maxmin: true
            , area: ['640px', '410px']
            , offset: 'rb'
            , content: ['view-stream.html', 'no']

            , zIndex: layer.zIndex
            , success: function (layero, index) {
                layer.setTop(layero);
                resize_iframe(layero);
            }
            , resizing: function (layero) {
                resize_iframe(layero);
            }
            , full: function (layero) {
                resize_iframe(layero);
            }
            , restore: function (layero) {
                resize_iframe(layero);
            }
        });
    };
    previewButton.on('click', showPreview);
    layer.ready(function () {
        showPreview();
    });
});
