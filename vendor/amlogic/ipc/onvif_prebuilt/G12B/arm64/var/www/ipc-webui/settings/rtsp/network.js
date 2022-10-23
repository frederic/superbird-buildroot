"use strict";

layui.use(['layer', 'element', 'form'], function(){
    var layer = layui.layer
        ,element = layui.element
        ,form = layui.form;

    let extra_post_data = [];

    set_form_value();

    function set_form_value() {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'network.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get"}
            ,success: function (data) {
                for (let k in data) {
                    $('#' + k).val(data[k]);
                    let e = document.getElementById(k);
                    if (e.disabled) {
                        extra_post_data[k] = data[k];
                    }
                }
                form.render();
            }
            ,complete: function () {
                layer.close(layer_id);
            }
        });
    }

    $(document).ready(function () {
        $("#reset").click(function (e) {
            e.preventDefault();
            set_form_value();
        });
        $("#submit").click(function(e) {
            e.preventDefault();
            let layer_id = layer.load(0, {time: 10*1000});
            let post_data = getFormData($("#form_rtsp_network"));
            post_data['action'] = 'set';
            post_data = $.extend(post_data, extra_post_data);
            $.ajax({
                url:"network.action.php"
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
