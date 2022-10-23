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
            url: 'discover.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get"}
            ,success: function (data) {
                extra_post_data['enabled'] = data['enabled'];
                $('#enabled').prop("checked", data['enabled'] === 'true');
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
            let post_data = getFormData($("#form_onvif_discover"));
            post_data['action'] = 'set';
            post_data = $.extend(post_data, extra_post_data);
            $.ajax({
                url:"discover.action.php"
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
