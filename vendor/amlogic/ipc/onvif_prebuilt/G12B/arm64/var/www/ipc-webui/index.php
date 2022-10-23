<?php
//index页面，先检查是否已通过ipc-property工具设置了database path
include "check_database.php";
check_session("login/login.html");
?>
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <script src="/scripts/jquery.min.js"></script>
    <title>IPC WebUI</title>
    <style type="text/css">
    </style>
    <style>
        .footer {
            position: fixed;
            right: 10px;
            bottom: 10px;
            color: white;
            text-align: center;
        }
    </style>
    <link rel="stylesheet" href="layui/css/layui.css" media="all">
    <link rel="shortcut icon" href="amlogic.ico" type="image/x-icon">
</head>
<body>
<?php include "select_lang.php"; ?>

<div class="layui-layout layui-layout-admin">
    <div class="layui-header">
        <div class="layui-logo"><img src="logo-amlogic.png"/></div>
        <ul class="layui-nav layui-layout-left" lay-filter="treenav">
            <li class="layui-nav-item">
                <a id="face" href="javascript:" class="site-url" data-url="facetable/facetable.php">
                    <?php echo gettext("Face Management"); ?>
                </a>
            </li>
            <li class="layui-nav-item">
                <a id="user" href="javascript:" class="site-url" data-url="usertable/usertable.php">
                    <?php echo gettext("User Management"); ?>
                </a>
            </li>
            <li class="layui-nav-item">
                <a href="javascript:"><?php echo gettext("Setting"); ?></a>
                <dl class="layui-nav-child">
                    <dd>
                        <a id="ipc" href="javascript:" class="site-url" data-url="settings/ipc-settings.php">IPC</a>
                    </dd>
                    <dd>
                        <a id="rtsp" href="javascript:" class="site-url" data-url="settings/rtsp-settings.php">RTSP</a>
                    </dd>
                    <dd>
                        <a id="onvif" href="javascript:" class="site-url"
                           data-url="settings/onvif-settings.php">ONVIF</a>
                    </dd>
                </dl>
            </li>
        </ul>
        <ul class="layui-nav layui-layout-right">
            <li class="layui-nav-item">
                <div>
                    <button id="showPreview" data-method="showPreview" class="layui-btn">
                        <?php echo gettext("Video Preview") ?>
                    </button>
                </div>
            </li>
            <li class="layui-nav-item">
                <a href="javascript:"><img src="login/user.png" class="layui-nav-img"><?php echo $_SESSION['user']; ?>
                </a>
                <dl class="layui-nav-child">
                    <dd><a href="login/logout.php" style="text-align: center;"><?php echo gettext("logout"); ?></a></dd>
                </dl>
            </li>
            <li class="layui-nav-item">
                <div>
                    <select id="lang" name="lang" style="margin-left: 20px; width:100px;height:27px">
                        <option value="zh_CN" selected="selected">简体中文</option>
                        <option value="en_US">English</option>
                    </select>
                    <div>
            </li>
        </ul>
    </div>
</div>

<iframe id="iframe" src="" name="iframe" frameborder="0"
        style="float:left;width:calc(100vw - 30px);height:calc(100vh - 30px);margin-left:15px;margin-top:15px"></iframe>

<script src="layui/layui.js"></script>
<script type="text/javascript">
    layui.use('element', function () {
        var element = layui.element;

        _id = location.search.replace(/\?page=(\w+)($|&.*)/, "$1");
        _lang = location.search.replace(/.*lang=(\w+)($|&.*)/, "$1");
        if (_id.length == 0 || _lang.length == 0) {
            _id = "face";
            _lang = get_browser_lang();
            var new_url = "http://" + location.host + location.pathname + "?page=" + _id + "&lang=" + _lang;
            history.pushState(null, null, new_url);
        }
        show_select_option(_lang);
        var src = getURLById(_id);
        if (src != null) {
            src = src + "?lang=" + _lang;
            $("#iframe").attr("src", src);
        }

        element.on('nav(treenav)', function (elem) {
            if (typeof (elem.attr("id")) != 'undefined') {
                _id = elem.attr("id");
                var new_url = "http://" + location.host + location.pathname + "?page=" + _id + "&lang=" + _lang;
                history.pushState(null, null, new_url);
                var src = elem.attr("data-url") + "?lang=" + _lang;
                $("#iframe").attr("src", src);
                if (_id == "ipc" || _id == "rtsp") {
                    var param = '#' + _id;
                    $(param).parent().parent().parent().addClass("layui-this");
                }
            }
        });

        /* 选择语言, 刷新页面 */
        $("#lang").change(function () {
            var lang_value = $(this).children("option:selected").val();
            var new_url = "http://" + location.host + location.pathname + "?page=" + _id + "&lang=" + lang_value;
            history.pushState(null, null, new_url);
            window.location.reload();
        });

        /* 获取浏览器当前的语言 */
        function get_browser_lang() {
            var lang = navigator.language || navigator.userLanguage; // 常规浏览器语言和IE浏览器
            lang = lang.substr(0, 2);
            if (lang == "zh") {
                return "zh_CN";
            } else {
                return "en_US";
            }
        }

        /* 根据lang的值, 设置option标签的selected属性 */
        function show_select_option(lang) {
            var options = document.getElementById('lang').children;
            if (lang == "zh_CN") {
                options[0].selected = true;
            } else if (lang == "en_US") {
                options[1].selected = true;
            }
        }

        function getURLById(id) {
            var param = '#' + id;
            var url = $(param).data("url");
            if (id == "face" || id == "user") {
                $(param).parent().addClass("layui-this");
            } else {
                $(param).parent().parent().parent().addClass("layui-this");
            }

            return url;
        }
    });
</script>
<script>
    layui.use('layer', function () {
        var $ = layui.jquery, layer = layui.layer;

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
                , title: "<?php echo gettext("Video Preview") ?>"
                , shade: 0
                , id: "VideoPreview"
                , maxmin: true
                , area: ['640px', '410px']
                , offset: 'rb'
                , content: ['view-stream.php', 'no']

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
</script>


</body>
</html>
