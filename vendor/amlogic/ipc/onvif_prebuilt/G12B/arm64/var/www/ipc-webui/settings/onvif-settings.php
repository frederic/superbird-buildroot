<?php
require "./get_props.php";
require "./create_elem.php";
require "../select_lang.php";
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
    <title><?php echo gettext("Onvif Setting") ?></title>
    <meta http-equiv="content-type" content="text/html;charset=utf-8"/>
    <script src="/scripts/jquery.min.js"></script>
    <link rel="stylesheet" href="/layui/css/layui.css" media="all">
</head>

<body>
<h1><?php echo gettext("Onvif Setting") ?></h1>
<hr class="layui-bg-green">
<br>
<div class="layui-collapse" lay-filter="onvif">
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Discover") ?></h2>
        <div class="layui-colla-content">
            <form target="discover" class="layui-form layui-form-pane" action="./onvif/discover.php"
                  onreset="reset_discover()" method="post">
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("enabled") ?></label>
                    <div class="layui-input-inline" id="discover_en"></div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-input-inline">
                        <button class="layui-btn" lay-submit lay-filter="demo1"><?php echo gettext("Submit") ?></button>
                        <button type="reset" class="layui-btn layui-btn-primary"><?php echo gettext("Reset") ?></button>
                    </div>
                </div>
            </form>
            <iframe name="discover" style="display:none"></iframe>
        </div>
    </div>
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Device") ?></h2>
        <div class="layui-colla-content">
            <form target="device" class="layui-form layui-form-pane" action="./onvif/device.php"
                  onreset="reset_device()" method="post">
                <div class="layui-form-item">
                    <label class="layui-form-label" style="width: auto"><?php echo gettext("firmware ver"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="dev_fv" class="layui-input layui-disabled" autocomplete="off"
                               id="dev_fv">
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label" style="width: auto"><?php echo gettext("hardware id"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="dev_hd_id" class="layui-input layui-disabled" autocomplete="off"
                               id="dev_hd_id">
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label" style="width: auto"><?php echo gettext("manufacturer"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="dev_manfacturer" class="layui-input" autocomplete="off"
                               id="dev_manfacturer">
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("model"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="dev_mod" class="layui-input layui-disabled" autocomplete="off"
                               id="dev_mod">
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label" style="width: auto"><?php echo gettext("serial number"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="dev_num" class="layui-input layui-disabled" autocomplete="off"
                               id="dev_num">
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-input-inline">
                        <button class="layui-btn" lay-submit lay-filter="demo1"><?php echo gettext("Submit") ?></button>
                        <button type="reset" class="layui-btn layui-btn-primary"><?php echo gettext("Reset") ?></button>
                    </div>
                </div>
            </form>
            <iframe name="device" style="display:none"></iframe>
        </div>
    </div>
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Network") ?></h2>
        <div class="layui-colla-content">
            <form target="network" class="layui-form layui-form-pane" action="./onvif/network.php"
                  onreset="reset_network()" method="post">
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("interface"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="net_interface" class="layui-input" autocomplete="off"
                               id="net_interface">
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("port"); ?></label>
                    <div class="layui-input-inline">
                        <input type="text" name="net_port" class="layui-input" autocomplete="off" id="net_port">
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-input-inline">
                        <button class="layui-btn" lay-submit lay-filter="demo1"><?php echo gettext("Submit") ?></button>
                        <button type="reset" class="layui-btn layui-btn-primary"><?php echo gettext("Reset") ?></button>
                    </div>
                </div>
            </form>
            <iframe name="network" style="display:none"></iframe>
        </div>
    </div>
</div>
<script src="/layui/layui.js"></script>
<script>
    //reset按钮
    function reset_discover() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "discover"},
            success: function (data) {
                <?php re_create_switch('data["enabled"]', "discover_en", "discover_en", "none", "discover_en", "discover", "no")?>
                <?php re_render()?>
            }
        });
    }

    function reset_device() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "device"},
            success: function (data) {
                $("#dev_fv").val(data["firmware-ver"]);
                $("#dev_hd_id").val(data["hardware-id"]);
                $("#dev_manfacturer").val(data["manufacturer"]);
                $("#dev_mod").val(data["model"]);
                $("#dev_num").val(data["serial-number"]);
                <?php re_render()?>
            }
        });
    }

    function reset_network() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "network"},
            success: function (data) {
                $("#net_interface").val(data["interface"]);
                $("#net_port").val(data["port"]);
                <?php re_render()?>
            }
        });
    }

    layui.use(['element', 'layer', 'form', 'jquery'], function () {
        var element = layui.element,
            layer = layui.layer,
            form = layui.form,
            $ = layui.$;
        element.on('collapse(onvif)', function (data) {
            var index = $(data.title).parents().index();
            switch (index) {
                case 0:
                    //discover的按钮创建
                <?php create_switch("discover_en", "none");?>
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "discover"},
                        success: function (data) {
                            <?php create_judge_switch('data["enabled"]', "discover_en", "discover_en", "none", "discover")?>
                            form.render();
                        }
                    });
                    break;
                case 1:
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "device"},
                        success: function (data) {
                            $("#dev_fv").val(data["firmware-ver"]);
                            $("#dev_hd_id").val(data["hardware-id"]);
                            $("#dev_manfacturer").val(data["manufacturer"]);
                            $("#dev_mod").val(data["model"]);
                            $("#dev_num").val(data["serial-number"]);
                            form.render();
                        }
                    });
                    break;
                case 2:
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "network"},
                        success: function (data) {
                            $("#net_interface").val(data["interface"]);
                            $("#net_port").val(data["port"]);
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
