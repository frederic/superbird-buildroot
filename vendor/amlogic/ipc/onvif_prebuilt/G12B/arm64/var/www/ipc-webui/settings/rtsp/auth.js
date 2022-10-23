"use strict";

layui.use(['layer', 'element', 'form'], function(){
    var layer = layui.layer
        ,element = layui.element
        ,form = layui.form;

    set_form_value();

    function set_form_value() {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'auth.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get"}
            ,success: function (data) {

                $('#username').val(data['username']);
                $('#password').val(data['password']);
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
            let post_data = getFormData($("#form_rtsp_auth"));
            post_data['action'] = 'set';
            $.ajax({
                url:"auth.action.php"
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
