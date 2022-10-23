"use strict";

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
            , url: 'load.php' //数据接口
            , page: {layout: ['prev', 'page', 'next']}
            , id: 'idUser'
            , limit: 20
            , response: {
                statusName: 'code'
                , statusCode: 0
                , msgName: 'msg'
                , countName: 'count'
                , dataName: 'data'
            }
            , text: {none: gettext('No Data')}
            , toolbar: '#toolbarUser'
            , defaultToolbar: []
            , cols: [[ //表头
                {type: 'checkbox'}
                , {field: 'uid', title: gettext('UID'), width: 250, display: "none"}
                , {field: 'name', title: gettext('Name'), width: 452}
                , {
                    fixed: 'right',
                    title: gettext('Operation'),
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
                        layer.msg(gettext("Please select the item to be deleted"));
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
                layer.msg('ID：' + data.id + ' check operation');
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
            , url: 'load.php' //数据接口
            , id: 'idUser'
            //,page: true
            , page: {
                curr: page
                , limit: limit
                , layout: ['prev', 'page', 'next']
            }
            , response: {
                statusName: 'code'
                , statusCode: 0
                , msgName: 'msg'
                , countName: 'count'
                , dataName: 'data'
            }
            , text: {none: gettext('No Data')}
            , toolbar: '#toolbarUser'
            , defaultToolbar: []
            , cols: [[ //表头
                {type: 'checkbox'}
                , {field: 'uid', title: gettext('UID'), width: 250}
                , {field: 'name', title: gettext('Name'), width: 452}
                , {
                    fixed: 'right',
                    title: gettext('Operation'),
                    width: 268,
                    align: 'center',
                    toolbar: '#barUser'
                }
            ]]
            , done: function (res, curr, count) {
                _cur_page = curr;
                if(res.data.length > 0) {
                    _cur_limit = res.data[0]["limit"];
                    if (res.data.length === 1 && count > 1) {
                        _cur_page--;
                    }
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
            title: gettext('Add'),
            skin: 'layui-layer-rim',
            area: ['500px', '200px'],
            offset: ['250px', '250px'],
            content: $("#layer_add").html(),
            btn: [gettext("OK"), gettext("Cancel")],
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
            if (input_name === "") {
                layer.msg(gettext('Empty name is not allowed'));
                return false;
            }

            var post_str = ("name=" + input_name);
            var url_add = "insert.php";
            var xhr_add = new XMLHttpRequest();
            xhr_add.open("POST", url_add, true);
            xhr_add.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
            xhr_add.onload = function () {
                if (xhr_add.status === 200) {
                    var response = xhr_add.response;
                    if (response === "1") {
                        layer.msg(gettext('User inserted successfully, Please check the user information on the last page'));
                    } else {
                        layer.msg(gettext('Failed to insert user'));
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
            title: gettext('Modify'),
            skin: 'layui-layer-rim',
            area: ['500px', '200px'],
            offset: ['250px', '250px'],
            content: $("#layer_edit").html(),
            btn: [gettext('OK'), gettext('Cancel')],
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
            if (input_name === "") {
                layer.msg(gettext("Empty name is not allowed"));
                return false;
            }
            var post_str = ("uid=" + indata.uid + "&name=" + input_name);
            var url_edit = "modify.php";
            var xhr_edit = new XMLHttpRequest();
            xhr_edit.open("POST", url_edit, true);
            xhr_edit.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
            xhr_edit.onload = function () {
                if (xhr_edit.status === 200) {
                    var response = xhr_edit.response;
                    if (response === "1") {
                        layer.msg(gettext('User modified successfully'));
                    } else {
                        layer.msg(gettext('Failed to modify the user'));
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
        var post_str = ("data=" + indata.uid);
        var url_delete = "delete.php";
        var xhr_delete = new XMLHttpRequest();
        xhr_delete.open("POST", url_delete, true);
        xhr_delete.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
        xhr_delete.onload = function () {
            if (xhr_delete.status === 200) {
                let res = JSON.parse(xhr_delete.response);
                if (res['res'] === "0") {
                    layer.msg(gettext('Deleted'));
                } else {
                    layer.msg(gettext('Delete failed'));
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
            if (i !== indata.length - 1) {
                send_data += indata[i].uid + ",";
            } else {
                send_data += indata[i].uid;
            }
        }
        $.ajax({
            url: 'delete.php',
            type: 'POST',
            dataType: 'json',
            data: {'data': send_data},

            success: function (res) {
                if ("0" === res["res"]) {
                    layer.msg(gettext('Deleted'));
                } else {
                    layer.msg(gettext('Delete failed'));
                }
                userinfo_reload(1, _cur_limit);
            },

            // 请求超时
            error: function (res) {
                layer.msg(gettext('Request timeout ...'));
            }
        });

    });
}

