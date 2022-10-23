"use strict";

layui.use(['layer', 'element', 'form', 'slider'], function(){
    let layer = layui.layer
        ,element = layui.element
        ,slider = layui.slider
        ,form = layui.form
    ;

    let extra_post_data = [];

    let slides_info = {
        'image-capture_quality': {min: 10, max: 100, step: 5},
    };

    let slides_node = ['image-capture_quality'];
    let slider_controls = [];

    slides_node.forEach(function(n) {
        slider_controls[n] = slider.render({
            elem: '#' + n
            , value: slides_info[n].min
            , min: slides_info[n].min
            , max: slides_info[n].max
            , step: slides_info[n].step
            , input: true
            , setTips: function (value) {
                extra_post_data[n] = value;
                return value;
            },
            change: function (value) {
                extra_post_data[n] = value;
            }
        });
    });

    set_form_value();

    function set_form_value() {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'other.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get"}
            ,success: function (data) {
                for (let k in data) {
                    if (slides_node.includes(k)) {
                        let v = data[k] - slides_info[k].min;
                        if (k === 'image-capture_quality') {
                            v /= 5;
                        }
                        slider_controls[k].setValue(v);
                    } else if (k === 'image-capture_use-hw-encoder') {
                        extra_post_data[k] = data[k];
                        $('#' + k).prop("checked", data[k] === 'true');
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

    form.on('switch(image-capture_use-hw-encoder)', function(data){
        extra_post_data[data.elem.id] = data.elem.checked;
    });

    $(document).ready(function () {
        $(".layui-btn").click(function (e) {
            e.preventDefault();
            let name = $(this).attr('name');
            if (name === 'reset') {
                set_form_value();
            } else {
                let layer_id = layer.load(0, {time: 10 * 1000});
                let post_data = getFormData($("#form_ipc_other"));
                post_data['action'] = 'set';
                post_data = $.extend(post_data, extra_post_data);
                $.ajax({
                    url: "other.action.php"
                    , type: "post"
                    , dataType: "json"
                    , data: post_data
                    , complete: function () {
                        layer.close(layer_id);
                    }
                });
            }
        });
    });

});
