"use strict";

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
            , url: 'load.php' //数据接口
            , page: {layout: ['prev', 'page', 'next']}
            , id: 'idFace'
            , limit: 20
            , response: {
                statusName: 'code'
                , statusCode: 0
                , msgName: 'msg'
                , countName: 'count'
                , dataName: 'data'
            }
            , text: {none: gettext("No Data")}
            , toolbar: '#toolbarFace'
            , defaultToolbar: []
            , cols: [[ //表头
                {type: 'checkbox'}
                , {field: 'index', title: gettext("Index"), width: 200, sort: true}
                , {field: 'uid', title: gettext("UID"), width: 200/*, hide: true*/ /* 隐藏列 */}
                , {field: 'name', title: gettext("Name"), width: 200}
                , {
                    field: 'faceimg', title: gettext("Image"), width: 200,
                    templet: '<div><img src="data:image/jpeg;base64,{{ d.faceimg }}" style="height:40px;width:40px"></div>'
                }
                , {
                    fixed: 'right',
                    title: gettext("Operation"),
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
                layer.msg('ID：' + data.id + gettext("Check Operation"));
            } else if (obj.event === 'del') {
                layer.confirm(
                    gettext("Please confirm to delete this item")
                    , {
                        title: gettext("Message"),
                        btn: [gettext("OK"), gettext("Cancel")]
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
                        layer.msg(gettext("Please select the item to be deleted"));
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
            , url: 'load.php' //数据接口
            , id: 'idFace'
            , page: {
                curr: page
                , limit: limit
                , layout: ['prev', 'page', 'next']
                , item: gettext("/Page")
            }
            , response: {
                statusName: 'code'
                , statusCode: 0
                , msgName: 'msg'
                , countName: 'count'
                , dataName: 'data'
            }
            , text: {none: gettext("No Data")}
            , toolbar: '#toolbarFace'
            , defaultToolbar: []
            , cols: [[ //表头
                {type: 'checkbox'}
                , {field: 'index', title: gettext("Index"), width: 200, sort: true}
                , {field: 'uid', title: gettext("UID"), width: 200}
                , {field: 'name', title: gettext("Name"), width: 200}
                , {
                    field: 'faceimg', title: gettext("Image"), width: 200,
                    templet: '<div><img src="data:image/jpeg;base64,{{ d.faceimg }}" style="height:40px;width:40px"></div>'
                }
                , {
                    fixed: 'right',
                    title: gettext("Operation"),
                    width: 168,
                    align: 'center',
                    toolbar: '#barFace'
                }
            ]]
            , done: function (res, curr, count) {
                _cur_page = curr;
                _front_item_count = count;
                if (res.data.length > 0) {
                    _cur_limit = res.data[0]["limit"];
                    if (res.data.length === 1 && count > 1) {
                        _cur_page--;
                    }
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
            url: 'capture_face.php',
            type: 'GET',
            dataType: 'json',
            data: {'front_item_count': param},
            beforeSend: function () {
                layer.load(); //上传loading
            },

            success: function (res) {
                layer.closeAll('loading');
                if (_front_item_count !== res["count"]) {
                    _front_item_count = res["count"];
                    faceinfo_reload(1, _cur_limit);
                } else {
                    layer.msg(gettext("Could not capture face or face already exists"));
                }
            },

            // 请求超时
            error: function (res) {
                layer.msg(gettext("Request timeout ..."));
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
            , url: 'upload_face_image.php'
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
                if (res.data["count"] !== -1) {
                    if (_front_item_count !== res.data["count"]) {
                        _front_item_count = res.data["count"];
                        faceinfo_reload(1, _cur_limit);
                    } else {
                        layer.msg(gettext("Could not capture face from image or face already exists"));
                    }
                } else {
                    layer.msg(gettext("Upload image failed"));
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
            , url: 'upload_database.php'
            , accept: 'file' //普通文件
            , auto: true
            , size: 102400 //限制文件大小，单位 KB
            , exts: 'db'
            , before: function (input) {
                //返回的参数item，即为当前的input DOM对象
                layer.load(); //上传loading
            }
            , done: function (res, index, upload) {
                if (res.data["res"] !== -1) {
                    layer.msg(gettext("Database uploaded"));
                    layer.closeAll('loading');
                    faceinfo_reload(1, _cur_limit);
                } else {
                    layer.msg(gettext("Database upload failed"));
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
            url: "fetch_userinfo.php",
            type: "GET",
            cache: false,
            async: true,
            success: function (res) {
                var selection = document.getElementById("name");
                selection.innerHTML = "";
                var data = JSON.parse(res);
                var proHtml = "";
                if (indata.name === "unknow") {
                    proHtml += '<option value="' + "" + '">' + "" + '</option>';
                }
                for (var i = 0; i < data.length; i++) {
                    if (data[i].name === indata.name) {
                        proHtml += '<option value="' + data[i].name + '" selected="selected">' + data[i].name + '</option>';
                    } else {
                        proHtml += '<option value="' + data[i].name + '">' + data[i].name + '</option>';
                    }
                }
                $("#name").append(proHtml);

                layer.open({
                    type: 1,
                    title: gettext("Modify"),
                    skin: 'layui-layer-rim',
                    area: ['600px', '450px'],
                    offset: ['225px', '225px'],
                    content: $("#layer_modify").html(),
                    btn: [gettext("OK"), gettext("Cancel")],
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
            if (input_name === "") {
                layer.msg(gettext("Empty name is not allowed"));
                return false;
            }
            for (var i = 0; i < data.length; i++) {
                if (data[i].name === input_name) {
                    var uid = data[i].uid;
                }
            }

            var post_str = ("index=" + indata.index + "&uid=" + uid);
            var url_set = "modify.php";
            var xhr_set = new XMLHttpRequest();
            xhr_set.open("POST", url_set, true);
            xhr_set.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
            xhr_set.onload = function () {
                if (xhr_set.status === 200) {
                    var response = xhr_set.response;
                    if (response === "1") {
                        layer.msg(gettext("Modify successfully"));
                    } else {
                        layer.msg(gettext("Modify failed"));
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
        var post_str = ("data=" + indata.index);
        var url_delete = "delete.php";
        var xhr_delete = new XMLHttpRequest();
        xhr_delete.open("POST", url_delete, true);
        xhr_delete.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
        xhr_delete.onload = function () {
            if (xhr_delete.status === 200) {
                let res = JSON.parse(xhr_delete.response);
                if (res['res'] === "0") {
                    layer.msg(gettext("Deleted"));
                } else {
                    layer.msg(gettext("Delete failed"));
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
            if (i !== indata.length - 1) {
                send_data += indata[i].index + ",";
            } else {
                send_data += indata[i].index;
            }
        }
        //console.log(send_data);
        $.ajax({
            url: 'delete.php',
            type: 'POST',
            dataType: 'json',
            data: {'data': send_data},

            success: function (res) {
                if ("0" === res["res"]) {
                    layer.msg(gettext("Deleted"));
                } else {
                    layer.msg(gettext("Delete failed"));
                }
                faceinfo_reload(1, _cur_limit);
            },

            // 请求超时
            error: function (res) {
                layer.msg(gettext("Request timeout ..."));
            }
        });

    });
}

