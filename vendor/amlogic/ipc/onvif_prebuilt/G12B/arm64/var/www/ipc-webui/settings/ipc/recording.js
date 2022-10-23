"use strict";

layui.use(['layer', 'element', 'form'], function(){
    var layer = layui.layer
        ,element = layui.element
        ,form = layui.form;

    let extra_post_data = [];
    let dirs = [];

    document.getElementById('chunk-duration').onkeyup = check_input_range;
    document.getElementById('reserved-space-size').onkeyup = check_input_range;

    get_dir();

    function get_dir() {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: '../storage/storage.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "recording"}
            ,success: function (data) {
                let e = document.getElementById('dir');
                let k = 0;
                for (k; k<data.length; k++) {
                    if (k%2 === 0) {
                        if (data[k] != "/dev/system") {
                           dirs.push(data[k+1]);
                        }
                    }
                }
                option_set_values(e, dirs, true);
                option_set_selected(e, 0);
                set_form_value(document.getElementById('dir').value);
                form.render();
            }
            ,complete: function () {
                 layer.close(layer_id);
            }
        })
    }

    function set_form_value(dir) {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'recording.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get"}
            ,success: function (data) {
                for (let k in data) {
                    if (k === 'enabled') {
                        extra_post_data['enabled'] = data['enabled'];
                        $('#' + k).prop("checked", data[k] === 'true');
                    } else if(k === 'location') {
                        for (let l in dirs) {
                            if (data[k].indexOf(dirs[l]) >= 0) {
                               $('#dir').val(dirs[l]);
                               $('#sub_dir').val(data[k].substring(dirs[l].length+1, data[k].length));
                            }
                        }
                    } else {
                        $('#' + k).val(data[k]);
                    }
                }
                form.render();
            }
            ,complete: function () {
                layer.close(layer_id);
            }
        });
    }

    form.on('switch(enabled)', function(data){
        extra_post_data['enabled'] = data.elem.checked;
    });

    $(document).ready(function () {
        $("#reset").click(function (e) {
            e.preventDefault();
            set_form_value();
        });
        $("#submit").click(function(e) {
            e.preventDefault();
            let layer_id = layer.load(0, {time: 10*1000});
            let post_data = getFormData($("#form_recording"));
            post_data['action'] = 'set';
            if (!post_data['dir']) {
                layer.msg(gettext("please input 'location'"));
                layer.close(layer_id);
                return;
            }
            if (post_data['sub_dir']) {
                post_data['location'] = post_data['dir'] + '/' + post_data['sub_dir'];
            } else {
                post_data['location'] = post_data['dir'];
            }
            post_data = $.extend(post_data, extra_post_data);
            $.ajax({
                url:"recording.action.php"
                ,type:"post"
                ,dataType: "json"
                ,data: post_data
                ,complete: function () {
                    layer.close(layer_id);
                }
            })
        });
    });

});
