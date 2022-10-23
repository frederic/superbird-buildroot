<?php
include "../common.php";
check_session("../login/login.html");
?>
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <script src="/scripts/jquery.min.js"></script>
    <title>userinfo</title>
    <style type="text/css">
        .layui-table-cell {
            height: auto !important;
            white-space: normal;
        }
    </style>
    <link rel="stylesheet" href="../layui/css/layui.css" media="all">
</head>
<body>
<?php include "../select_lang.php"; ?>
<h1><?php echo gettext("UserInfo"); ?></h1>
<table class="layui-hide" id="user" lay-filter="usertable" lay-data="{id:'idUser'}"></table>
<script type="text/html" id="toolbarUser">
    <div class="layui-btn-container">
        <button class="layui-btn layui-btn-sm" lay-event="add"><?php echo gettext("add"); ?></button>
        <button class="layui-btn layui-btn-sm" lay-event="refresh"><?php echo gettext("refresh"); ?></button>
        <ul class="layui-layout-right" style="margin-top:10px">
            <button class="layui-btn layui-btn-sm layui-btn-danger" lay-event="batch_delete"><i class="layui-icon">&#xe640</i><?php echo gettext("batch delete"); ?>
            </button>
        </ul>
    </div>
</script>
<script type="text/html" id="barUser">
    <a class="layui-btn layui-btn-primary layui-btn-xs" lay-event="detail"
       style="display:none"><?php echo gettext("check"); ?></a>
    <a class="layui-btn layui-btn-xs" lay-event="edit"><?php echo gettext("edit"); ?></a>
    <a class="layui-btn layui-btn-danger layui-btn-xs" lay-event="del"><?php echo gettext("delete"); ?></a>
</script>

<div class="layui-row" id="layer_add" style="display:none;">
    <div class="layui-col-md10" style="margin-top:25px">
        <form class="layui-form" id="add">
            <div class="layui-form-item">
                <label class="layui-form-label"><?php echo gettext("Name:"); ?></label>
                <div class="layui-input-block">
                    <input type="text" id="input_name" name="name" class="layui-input"
                           placeholder='<?php echo gettext("please input name"); ?>'>
                </div>
            </div>
        </form>
    </div>
</div>

<div class="layui-row" id="layer_edit" style="display:none;">
    <div class="layui-col-md10" style="margin-top:25px">
        <form class="layui-form" id="modify">
            <div class="layui-form-item">
                <label class="layui-form-label"><?php echo gettext("Name:"); ?></label>
                <div class="layui-input-block">
                    <input type="text" id="edit_name" name="name" class="layui-input" placeholder="">
                </div>
            </div>
        </form>
    </div>
</div>

<script src="../layui/layui.js"></script>
<script>
    var _cur_page = 0;
    var _cur_limit = 0;

    worker();

    function worker() {
        layui.use(['layer', 'table'], function () {
            var layer = layui.layer;
            var table = layui.table;

            // userinfo table
            table.render({
                elem: '#user'
                , height: "auto"
                , width: 1024
                , url: 'usertable_data.php' //数据接口
                , page: {layout: ['prev', 'page', 'next', 'limit'], item: '<?php echo gettext("/page"); ?>'}
                , id: 'idUser'
                , limits: [10]
                , limit: 10
                , response: {
                    statusName: 'code'
                    , statusCode: 0
                    , msgName: 'msg'
                    , countName: 'count'
                    , dataName: 'data'
                }
                , text: {none: '<?php echo gettext("No Data"); ?>'}
                , toolbar: '#toolbarUser'
                , defaultToolbar: []
                , cols: [[ //表头
                    {type: 'checkbox'}
                    , {field: 'uid', title: '<?php echo gettext("uid"); ?>', width: 250, display: "none"}
                    , {field: 'name', title: '<?php echo gettext("name"); ?>', width: 452}
                    , {
                        fixed: 'right',
                        title: '<?php echo gettext("operation"); ?>',
                        width: 268,
                        align: 'center',
                        toolbar: '#barUser'
                    }
                ]]
                , done: function (res, curr, count) {
                    _cur_page = curr;
                }
            });

            //监听头工具栏事件
            table.on('toolbar(usertable)', function (obj) {
                var checkStatus = table.checkStatus(obj.config.id)
                    , data = checkStatus.data; //获取选中的数据
                switch (obj.event) {
                    case 'add':
                        add_row();
                        break;
                    case 'refresh':
                        userinfo_reload(_cur_page, _cur_limit);
                        break;
                    case 'batch_delete':
                        if (data.length === 0) {
                            layer.msg('<?php echo gettext("please select one item at least") ?>');
                        } else {
                            batch_delete(data);
                        }
                        break;
                }
            });

            //监听工具条
            table.on('tool(usertable)', function (obj) {
                var data = obj.data;
                if (obj.event === 'detail') {
                    layer.msg('ID：' + data.id + ' <?php echo gettext("check operation"); ?>');
                } else if (obj.event === 'del') {
                    layer.confirm(
                        '<?php echo gettext("really want to delete this line?") ?>'
                        , {
                            title: '<?php echo gettext("Message"); ?>',
                            btn: ['<?php echo gettext("OK"); ?>', '<?php echo gettext("Cancel"); ?>']
                        }
                        , function (index) {
                            obj.del();
                            layer.close(index);
                            delete_row(data);
                        });
                } else if (obj.event === 'edit') {
                    edit_row(data);
                }
            });
        });
    }

    // 用户表重载
    function userinfo_reload(page, limit) {
        layui.use('table', function () {
            var table = layui.table;
            table.reload('idUser', {
                elem: '#user'
                , height: "auto"
                , width: 1024
                , url: 'usertable_data.php' //数据接口
                , id: 'idUser'
                //,page: true
                , page: {
                    curr: page
                    , limits: [10]
                    , limit: limit
                    , layout: ['prev', 'page', 'next', 'limit']
                    , item: '<?php echo gettext("/page"); ?>'
                }
                , response: {
                    statusName: 'code'
                    , statusCode: 0
                    , msgName: 'msg'
                    , countName: 'count'
                    , dataName: 'data'
                }
                , text: {none: '<?php echo gettext("No Data"); ?>'}
                , toolbar: '#toolbarUser'
                , defaultToolbar: []
                , cols: [[ //表头
                    {type: 'checkbox'}
                    , {field: 'uid', title: '<?php echo gettext("uid"); ?>', width: 250}
                    , {field: 'name', title: '<?php echo gettext("name"); ?>', width: 452}
                    , {
                        fixed: 'right',
                        title: '<?php echo gettext("operation"); ?>',
                        width: 268,
                        align: 'center',
                        toolbar: '#barUser'
                    }
                ]]
                , done: function (res, curr, count) {
                    _cur_page = curr;
                    _cur_limit = res.data[0]["limit"];
                    if (res.data.length == 1 && count > 1) {
                        _cur_page--;
                    }
                }
            }); //只重载数据
        });
    }


    // 添加操作 - 添加一行
    function add_row() {
        layui.use(['layer', 'table'], function () {
            var layer = layui.layer;
            var table = layui.table;

            layer.open({
                type: 1,
                title: '<?php echo gettext("Add"); ?>',
                skin: 'layui-layer-rim',
                area: ['500px', '200px'],
                offset: ['250px', '250px'],
                content: $("#layer_add").html(),
                btn: ['<?php echo gettext("OK"); ?>', '<?php echo gettext("Cancel"); ?>'],
                btn1: function (index, layero) {
                    add_event_call(index, layero);
                },
                btn2: function (index, layero) {
                    layer.close(index);
                }
                , success: function (layero, index) {
                    this.enterEsc = function (event) {
                        if (event.keyCode === 13) {	// 绑定 Enter 按键
                            add_event_call(index, layero);
                            return false;
                        } else if (event.keyCode === 27) {
                            layer.close(index);
                        }
                    };
                    $(document).on('keydown', this.enterEsc);	//监听键盘事件，关闭层
                }
                , end: function () {
                    $(document).off('keydown', this.enterEsc);	//解除键盘关闭事件
                }
            });

            function add_event_call(index, layero) {
                var input_name = $(layero).find("#input_name").val();
                if (input_name == "") {
                    layer.msg('<?php echo gettext("empty name is not allowed"); ?>');
                    return false;
                }

                var post_str = ("name=" + input_name);
                var url_add = "usertable_add_row.php";
                var xhr_add = new XMLHttpRequest();
                xhr_add.open("POST", url_add, true);
                xhr_add.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
                xhr_add.onload = function () {
                    if (xhr_add.status == 200) {
                        var response = xhr_add.response;
                        if (response == "1") {
                            layer.msg('<?php echo gettext("add successfully, result is on the last page"); ?>');
                        } else {
                            layer.msg('<?php echo gettext("add failed"); ?>');
                        }

                        userinfo_reload(_cur_page, _cur_limit);
                    }
                };
                xhr_add.send(post_str);
                layer.close(index);
            }
        });
    }


    // 编辑操作 - 编辑一行
    function edit_row(indata) {
        layui.use(['layer', 'table'], function () {
            var layer = layui.layer;
            var table = layui.table;

            $("#edit_name").attr("placeholder", indata.name);

            layer.open({
                type: 1,
                title: '<?php echo gettext("Modify"); ?>',
                skin: 'layui-layer-rim',
                area: ['500px', '200px'],
                offset: ['250px', '250px'],
                content: $("#layer_edit").html(),
                btn: ['<?php echo gettext("OK"); ?>', '<?php echo gettext("Cancel"); ?>'],
                btn1: function (index, layero) {
                    edit_event_call(index, layero);
                },
                btn2: function (index, layero) {
                    layer.close(index);
                }
                , success: function (layero, index) {
                    this.enterEsc = function (event) {
                        if (event.keyCode === 13) {
                            edit_event_call(index, layero);
                            return false;
                        } else if (event.keyCode === 27) {
                            layer.close(index);
                        }
                    };
                    $(document).on('keydown', this.enterEsc);	//监听键盘事件，关闭层
                }
                , end: function () {
                    $(document).off('keydown', this.enterEsc);	//解除键盘关闭事件
                }
            });

            function edit_event_call(index, layero) {
                var input_name = $(layero).find("#edit_name").val();
                if (input_name == "") {
                    layer.msg("empty name is not allowed");
                    return false;
                }
                var post_str = ("uid=" + indata.uid + "&name=" + input_name);
                var url_edit = "usertable_edit_row.php";
                var xhr_edit = new XMLHttpRequest();
                xhr_edit.open("POST", url_edit, true);
                xhr_edit.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
                xhr_edit.onload = function () {
                    if (xhr_edit.status == 200) {
                        var response = xhr_edit.response;
                        if (response == "1") {
                            layer.msg('<?php echo gettext("modify successfully"); ?>');
                        } else {
                            layer.msg('<?php echo gettext("modify failed"); ?>');
                        }

                        userinfo_reload(_cur_page, _cur_limit);
                    }
                };
                xhr_edit.send(post_str);
                layer.close(index);
            }
        });
    }


    // 删除操作 - 删除一行数据
    function delete_row(indata) {
        layui.use(['layer', 'table'], function () {
            var layer = layui.layer;
            var table = layui.table;
            var post_str = ("uid=" + indata.uid);
            var url_delete = "usertable_delete_row.php";
            var xhr_delete = new XMLHttpRequest();
            xhr_delete.open("POST", url_delete, true);
            xhr_delete.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
            xhr_delete.onload = function () {
                if (xhr_delete.status == 200) {
                    var response = xhr_delete.response;
                    if (response == "1") {
                        layer.msg('<?php echo gettext("delete successfully"); ?>');
                    } else {
                        layer.msg('<?php echo gettext("delete failed"); ?>');
                    }

                    userinfo_reload(_cur_page, _cur_limit);
                }
            };
            xhr_delete.send(post_str);
        });
    }

    // 批量删除
    function batch_delete(indata) {
        layui.use(['layer'], function () {
            var layer = layui.layer;

            var send_data = "";
            for (var i = 0; i < indata.length; i++) {
                if (i != indata.length - 1) {
                    send_data += indata[i].uid + ",";
                } else {
                    send_data += indata[i].uid;
                }
            }
            //console.log(send_data);
            $.ajax({
                url: 'usertable_delete_batch.php',
                type: 'GET',
                dataType: 'json',
                data: {'data': send_data},

                success: function (res) {
                    if ("0" == res["res"]) {
                        layer.msg('<?php echo gettext("delete batch successfully"); ?>');
                    } else {
                        layer.msg('<?php echo gettext("delete batch failed"); ?>');
                    }
                    userinfo_reload(1, _cur_limit);
                },

                // 请求超时
                error: function (res) {
                    layer.msg('<?php echo gettext("request time out ..."); ?>');
                }
            });

        });
    }

</script>

</body>

</html>



