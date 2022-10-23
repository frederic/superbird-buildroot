<?php
require "../select_lang.php";
require "./get_props.php";
require "./create_elem.php";
check_session("../login/login.html");

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
    <title><?php echo gettext("Rtsp Setting"); ?></title>
    <meta http-equiv="content-type" content="text/html;charset=utf-8"/>
    <meta name="generator" content="Geany 1.34.1"/>
    <script src="/scripts/jquery.min.js"></script>
    <link rel="stylesheet" href="/layui/css/layui.css" media="all">
</head>

<body>
<h1><?php echo gettext("Rtsp Setting"); ?></h1>
<hr class="layui-bg-green">
<br>
<div class="layui-collapse" lay-filter="rtsp">
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Authenticate"); ?></h2>
        <div class="layui-colla-content">
            <form target="auth" class="layui-form layui-form-pane" action="./rtsp/auth.php" method="post"
                  onreset="reset_auth()">
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("username"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="a_username" class="layui-input" autocomplete="off" id="a_username">
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("password"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="a_pwd" class="layui-input" autocomplete="off" id="a_pwd">
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-input-inline">
                        <button class="layui-btn" lay-submit lay-filter="auth"><?php echo gettext("Submit"); ?></button>
                        <button type="reset"
                                class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                    </div>
                </div>
            </form>
            <iframe name="auth" style="display:none"></iframe>
        </div>
    </div>
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Network"); ?></h2>
        <div class="layui-colla-content">
            <form target="network" class="layui-form layui-form-pane" action="./rtsp/network.php" method="post"
                  onreset="reset_rtsp_network()">
                <div class="layui-form-item" hidden>
                    <label class="layui-form-label"><?php echo gettext("address"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="n_address" class="layui-input" autocomplete="off" id="n_addr">
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("port"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="n_port" class="layui-input" autocomplete="off" id="n_port">
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("route"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="n_route" class="layui-input" autocomplete="off" id="n_route">
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("subroute"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="n_subroute" class="layui-input" autocomplete="off" id="n_subroute">
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-input-inline">
                        <button class="layui-btn" lay-filter="demo1"><?php echo gettext("Submit"); ?></button>
                        <button type="reset"
                                class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                    </div>
                </div>
            </form>
            <iframe name="network" style="display:none"></iframe>
        </div>
    </div>
</div>
<script src="/layui/layui.js"></script>
<script>
    function reset_auth() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "auth"},
            success: function (data) {
                $("#a_username").val(data["username"]);
                $("#a_pwd").val(data["password"]);
                <?php re_render()?>
            }
        });
    }

    function reset_rtsp_network() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "rtsp_network"},
            success: function (data) {
                $("#n_addr").val(data["address"]);
                $("#n_port").val(data["port"]);
                $("#n_route").val(data["route"]);
                $("#n_subroute").val(data["subroute"]);
                <?php re_render()?>
            }
        });
    }

    layui.use(['element', 'layer', 'form', 'jquery'], function () {
        var element = layui.element,
            layer = layui.layer,
            form = layui.form,
            $ = layui.$;
        element.on('collapse(rtsp)', function (data) {
            var index = $(data.title).parents().index();
            switch (index) {
                case 0:
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "auth"},
                        success: function (data) {
                            $("#a_username").val(data["username"]);
                            $("#a_pwd").val(data["password"]);
                            form.render();
                        }
                    });
                    break;
                case 1:
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "rtsp_network"},
                        success: function (data) {
                            $("#n_addr").val(data["address"]);
                            $("#n_port").val(data["port"]);
                            $("#n_route").val(data["route"]);
                            $("#n_subroute").val(data["subroute"]);
                            form.render();
                        }
                    });
                    break;
            }
        })
    });
</script>
</body>

</html>
