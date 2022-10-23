<?php
include "../common.php";
check_session("../login/login.html");
?>
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <script src="/scripts/jquery.min.js"></script>
    <title>faceinfo</title>
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
<h1><?php echo gettext("FaceInfo"); ?></h1>
<table class="layui-hide" id="face" lay-filter="facetable" lay-data="{id:'idFace'}"></table>
<script type="text/html" id="toolbarFace">
    <div class="layui-btn-container">
        <button class="layui-btn layui-btn-sm" lay-event="refresh"><?php echo gettext("refresh"); ?></button>
        <button class="layui-btn layui-btn-sm"
                lay-event="camera_face_capture"><?php echo gettext("face capture"); ?></button>
        <button class="layui-btn layui-btn-sm" lay-event="upload"
                id="up_img"><?php echo gettext("face capture from image"); ?></button>
        <button class="layui-btn layui-btn-sm" lay-event="upload"
                id="up"><?php echo gettext("customized database"); ?></button>
        <button class="layui-btn layui-btn-sm" lay-event="download" id="dl_db"><a href="../download/save_database.php"
                                                                                  style="color:white;"><?php echo gettext("save database"); ?></a>
        </button>
        <button class="layui-btn layui-btn-sm" lay-event="download" id="dl_img"><a
                    href="../download/save_capture_image.php"
                    style="color:white;"><?php echo gettext("capture and save image"); ?></a></button>
        <ul class="layui-layout-right" style="margin-top:10px">
            <button class="layui-btn layui-btn-sm layui-btn-danger" lay-event="batch_delete"><i class="layui-icon">&#xe640</i><?php echo gettext("batch delete"); ?>
            </button>
        </ul>
</script>
<script type="text/html" id="barFace">
    <a class="layui-btn layui-btn-primary layui-btn-xs" lay-event="detail"
       style="display:none"><?php echo gettext("check"); ?></a>
    <a class="layui-btn layui-btn-xs" lay-event="edit"><?php echo gettext("edit"); ?></a>
    <a class="layui-btn layui-btn-danger layui-btn-xs" lay-event="del"><?php echo gettext("delete"); ?></a>
</script>

<div class="layui-row" id="layer_modify" style="display:none;">
    <div class="layui-col-md10" style="margin-top:25px">
        <form class="layui-form" id="modify">
            <div class="layui-form-item">
                <label class="layui-form-label"><?php echo gettext("Index:"); ?></label>
                <div class="layui-input-block">
                    <input type="text" id="input_index" name="index" class="layui-input" placeholder="0"
                           readonly="true">
                </div>
            </div>
            <div class="layui-form-item">
                <label class="layui-form-label"><?php echo gettext("Name:"); ?></label>
                <div class="layui-input-block">
                    <select class="layui-input" name="name" id="name" lay-filter="nameTest" lay-search="">
                        <option value="0"></option>
                    </select>
                </div>
            </div>
        </form>
    </div>
</div>

<script src="../layui/layui.js"></script>
<script>
    var _cur_page = 0;
    var _cur_limit = 0;
    var _front_item_count = 0;

    worker();

    function worker() {
        layui.use(['layer', 'table'], function () {
            var layer = layui.layer;
            var table = layui.table;

            // faceinfo table
            table.render({
                elem: '#face'
                , height: "auto"
                , width: 1024
                , url: 'facetable_data.php' //数据接口
                , page: {layout: ['prev', 'page', 'next', 'limit'], item: '<?php echo gettext("/page"); ?>'}
                , id: 'idFace'
                , limits: [5, 10, 15]
                , limit: 10
                , response: {
                    statusName: 'code'
                    , statusCode: 0
                    , msgName: 'msg'
                    , countName: 'count'
                    , dataName: 'data'
                }
                , text: {none: '<?php echo gettext("No Data"); ?>'}
                , toolbar: '#toolbarFace'
                , defaultToolbar: []
                , cols: [[ //表头
                    {type: 'checkbox'}
                    , {field: 'index', title: '<?php echo gettext("index");  ?>', width: 200, sort: true}
                    , {field: 'uid', title: '<?php echo gettext("uid"); ?>', width: 200/*, hide: true*/ /* 隐藏列 */}
                    , {field: 'name', title: '<?php echo gettext("name"); ?>', width: 200}
                    , {
                        field: 'faceimg', title: '<?php echo gettext("image"); ?>', width: 200,
                        templet: '<div><img src="data:image/jpeg;base64,{{ d.faceimg }}" style="height:40px;width:40px"></div>'
                    }
                    , {
                        fixed: 'right',
                        title: '<?php echo gettext("operation"); ?>',
                        width: 168,
                        align: 'center',
                        toolbar: '#barFace'
                    }
                ]]
                , done: function (res, curr, count) {
                    _cur_page = curr;
                    _front_item_count = count;
                }
            });
            enable_upload_op();

            //监听行工具条
            table.on('tool(facetable)', function (obj) {
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
                    edit(data);
                }
            });

            //监听table表工具栏事件
            table.on('toolbar(facetable)', function (obj) {
                var checkStatus = table.checkStatus(obj.config.id);
                var data = checkStatus.data;
                switch (obj.event) {
                    case 'refresh':
                        faceinfo_reload(_cur_page, _cur_limit);
                        break;
                    case 'camera_face_capture':
                        face_cap(_front_item_count);
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

        });
    }

    // 使需要进行upload操作的按键生效
    function enable_upload_op() {
        upload_database();
        face_cap_with_img();
    }

    // 人脸表重载
    function faceinfo_reload(page, limit) {
        layui.use('table', function () {
            var table = layui.table;
            table.reload('idFace', {
                elem: '#face'
                , height: "auto"
                , width: 1024
                , url: 'facetable_data.php' //数据接口
                , id: 'idFace'
                , page: {
                    curr: page
                    , limits: [5, 10, 15]
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
                , toolbar: '#toolbarFace'
                , defaultToolbar: []
                , cols: [[ //表头
                    {type: 'checkbox'}
                    , {field: 'index', title: '<?php echo gettext("index"); ?>', width: 200, sort: true}
                    , {field: 'uid', title: '<?php echo gettext("uid"); ?>', width: 200}
                    , {field: 'name', title: '<?php echo gettext("name"); ?>', width: 200}
                    , {
                        field: 'faceimg', title: '<?php echo gettext("image"); ?>', width: 200,
                        templet: '<div><img src="data:image/jpeg;base64,{{ d.faceimg }}" style="height:40px;width:40px"></div>'
                    }
                    , {
                        fixed: 'right',
                        title: '<?php echo gettext("operation"); ?>',
                        width: 168,
                        align: 'center',
                        toolbar: '#barFace'
                    }
                ]]
                , done: function (res, curr, count) {
                    _cur_page = curr;
                    _front_item_count = count;
                    _cur_limit = res.data[0]["limit"];
                    if (res.data.length == 1 && count > 1) {
                        _cur_page--;
                    }
                }
            }); //只重载数据
        });
        enable_upload_op();
    }

    // 捕捉人脸 - 1.通过camera镜头捕捉
    function face_cap(param) {
        layui.use(['table', 'layer'], function () {
            var table = layui.table;
            var layer = layui.layer;
            $.ajax({
                url: 'facetable_face_cap.php',
                type: 'GET',
                dataType: 'json',
                data: {'front_item_count': param},
                beforeSend: function () {
                    layer.load(); //上传loading
                },

                success: function (res) {
                    layer.closeAll('loading');
                    if (_front_item_count != res["count"]) {
                        _front_item_count = res["count"];
                        faceinfo_reload(1, _cur_limit);
                    } else {
                        layer.msg('<?php echo gettext("can not capture face or face already exists"); ?>');
                    }
                },

                // 请求超时
                error: function (res) {
                    layer.msg('<?php echo gettext("request time out ..."); ?>');
                }
            });
        });
    }

    // 捕捉人脸 - 2.通过上传的图片捕捉
    function face_cap_with_img(param) {
        layui.use(['table', 'upload', 'layer'], function () {
            var table = layui.table;
            var upload = layui.upload;
            var layer = layui.layer;

            var uploadInst = upload.render({
                elem: '#up_img'
                , url: '../upload/upload_img_data.php'
                , accept: 'images' //普通文件
                , auto: true
                , size: 5120 //限制文件大小，单位 KB
                , exts: 'jpg|jpeg|JPG|JPEG'
                , data: {'front_item_count': param}
                , before: function (input) {
                    //返回的参数item，即为当前的input DOM对象
                    layer.load(); //上传loading
                }
                , done: function (res, index, upload) {
                    layer.closeAll('loading');
                    if (res.data["count"] != -1) {
                        if (_front_item_count != res.data["count"]) {
                            _front_item_count = res.data["count"];
                            faceinfo_reload(1, _cur_limit);
                        } else {
                            layer.msg('<?php echo gettext("can not capture face from image or face already exists"); ?>');
                        }
                    } else {
                        layer.msg('<?php echo gettext("upload image failed"); ?>');
                    }
                }
            });
        });
    }

    // 数据库上传操作
    function upload_database() {
        layui.use('upload', function () {
            var upload = layui.upload;
            var uploadInst = upload.render({
                elem: '#up'
                , url: '../upload/upload_data.php'
                , accept: 'file' //普通文件
                , auto: true
                , size: 102400 //限制文件大小，单位 KB
                , exts: 'db'
                , before: function (input) {
                    //返回的参数item，即为当前的input DOM对象
                    layer.load(); //上传loading
                }
                , done: function (res, index, upload) {
                    if (res.data["res"] != -1) {
                        layer.msg('<?php echo gettext("upload database successfully"); ?>');
                        layer.closeAll('loading');
                        faceinfo_reload(1, _cur_limit);
                    } else {
                        layer.msg('<?php echo gettext("upload database failed"); ?>');
                    }
                }
            });
        });
    }


    // 编辑操作
    function edit(indata) {
        layui.use(['form', 'layer', 'table'], function () {
            var form = layui.form;
            var layer = layui.layer;
            var table = layui.table;


            $("#input_index").attr("placeholder", indata.index);
            $.ajax({
                url: "facetable_get_userinfo.php",
                type: "GET",
                cache: false,
                async: true,
                success: function (res) {
                    var selection = document.getElementById("name");
                    selection.innerHTML = "";
                    var data = JSON.parse(res);
                    var proHtml = "";
                    if (indata.name == "unknow") {
                        proHtml += '<option value="' + "" + '">' + "" + '</option>';
                    }
                    for (var i = 0; i < data.length; i++) {
                        if (data[i].name == indata.name) {
                            proHtml += '<option value="' + data[i].name + '" selected="selected">' + data[i].name + '</option>';
                        } else {
                            proHtml += '<option value="' + data[i].name + '">' + data[i].name + '</option>';
                        }
                    }
                    $("#name").append(proHtml);

                    layer.open({
                        type: 1,
                        title: '<?php echo gettext("Modify"); ?>',
                        skin: 'layui-layer-rim',
                        area: ['600px', '450px'],
                        offset: ['225px', '225px'],
                        content: $("#layer_modify").html(),
                        btn: ['<?php echo gettext("OK"); ?>', '<?php echo gettext("Cancel"); ?>'],
                        btn1: function (index, layero) {
                            edit_event_call(index, layero, data);
                        },
                        btn2: function (index, layero) {
                            layer.close(index);
                        }
                        , success: function (layero, index) {
                            this.enterEsc = function (event) {
                                if (event.keyCode === 13) {
                                    edit_event_call(index, layero, data);
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
                    form.render();
                }
            });

            function edit_event_call(index, layero, data) {
                var input_name = $(layero).find("#name option:selected").val();
                if (input_name == "") {
                    layer.msg('<?php echo gettext("empty name is not allowed"); ?>');
                    return false;
                }
                for (var i = 0; i < data.length; i++) {
                    if (data[i].name == input_name) {
                        var uid = data[i].uid;
                    }
                }

                var post_str = ("index=" + indata.index + "&uid=" + uid);
                var url_set = "facetable_edit_row.php";
                var xhr_set = new XMLHttpRequest();
                xhr_set.open("POST", url_set, true);
                xhr_set.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
                xhr_set.onload = function () {
                    if (xhr_set.status == 200) {
                        var response = xhr_set.response;
                        if (response == "1") {
                            layer.msg('<?php echo gettext("modify successfully"); ?>');
                        } else {
                            layer.msg('<?php echo gettext("modify failed"); ?>');
                        }

                        faceinfo_reload(_cur_page, _cur_limit);
                    }
                }
                xhr_set.send(post_str);
                layer.close(index);
            }
        });
    }

    // 删除操作 - 删除一行数据
    function delete_row(indata) {
        layui.use(['layer'], function () {
            var layer = layui.layer;
            var table = layui.table;
            var post_str = ("index=" + indata.index);
            var url_delete = "facetable_delete_row.php";
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

                    faceinfo_reload(_cur_page, _cur_limit);
                }
            }
            xhr_delete.send(post_str);
        });
    }

    function batch_delete(indata) {
        layui.use(['layer'], function () {
            var layer = layui.layer;

            var send_data = "";
            for (var i = 0; i < indata.length; i++) {
                if (i != indata.length - 1) {
                    send_data += indata[i].index + ",";
                } else {
                    send_data += indata[i].index;
                }
            }
            //console.log(send_data);
            $.ajax({
                url: 'facetable_delete_batch.php',
                type: 'GET',
                dataType: 'json',
                data: {'data': send_data},

                success: function (res) {
                    if ("0" == res["res"]) {
                        layer.msg('<?php echo gettext("delete batch successfully"); ?>');
                    } else {
                        layer.msg('<?php echo gettext("delete batch failed"); ?>');
                    }
                    faceinfo_reload(1, _cur_limit);
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

