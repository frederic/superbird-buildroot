"use strict";

layui.use(['layer', 'element', 'form', 'slider'], function(){
    var layer = layui.layer
        ,element = layui.element
        ,form = layui.form
        ,slider = layui.slider
    ;

    let extra_post_data = [];

    let sliders = {
        recognition_threshold: {min: 0, max: 1500, step: 1, input: false, divisor: 1000},
        detection_limits: {min: 10, max: 200, step: 1, input: true, divisor: 1},
    };
    let slider_controls = [];
    for (let id in sliders) {
        slider_controls[id] = slider.render({
            elem: '#' + id
            , value: sliders[id].min
            , min: sliders[id].min
            , max: sliders[id].max
            , step: sliders[id].step
            , input: sliders[id].input
            , setTips:
                function (value) {
                    extra_post_data[id] = value/sliders[id].divisor;
                    return value/sliders[id].divisor;
                },
            change: function (value) {
                extra_post_data[id] = value;
            }
        });
    }

    let options_list = [
        {
            id: "detection_model",
            options: [
                {val: 'aml_face_detection', text: 'Amlogic Detection'},
                {val: 'mtcnn-v1', text: 'MTCNN'},
                {val: 'yoloface-v2', text: 'YOLOFACE'},
                {val: 'disable', text: gettext('Disabled')},
            ],
            key: "detection/model"
        },
        {
            id: "recognition_model",
            options: [
                {val: 'aml_face_recognition', text: 'Amlogic Recognition'},
                {val: 'facenet', text: 'FaceNet'},
                {val: 'disable', text: gettext('Disabled')},
            ],
            key: "recognition/model"
        },
    ];
    options_list.forEach(function(l) {
        let e = document.getElementById(l.id);
        option_set_values(e, l.options);
    });
    set_form_value();

    function set_form_value() {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'nn.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get"}
            ,success: function (data) {
                for (let k in data) {
                    if (k === 'enabled') {
                        extra_post_data['enabled'] = data[k];
                        $('#' + k).prop("checked", data[k] === 'true');
                    } else if (Object.keys(sliders).includes(k)) {
                        let v = data[k] - sliders[k].min;
                        slider_controls[k].setValue(v * sliders[k].divisor);
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

    form.on('select(recognition_model)', function (data) {
        if (data.value === "facenet") {
            slider_controls['recognition_threshold'].setValue(0.6 * 1000);
        } else {
            slider_controls['recognition_threshold'].setValue(1.0 * 1000);
        }
    });

    $(document).ready(function () {
        $("#reset").click(function (e) {
            e.preventDefault();
            set_form_value();
        });
        $("#submit").click(function(e) {
            e.preventDefault();
            let layer_id = layer.load(0, {time: 10*1000});
            let post_data = getFormData($("#form_ipc_nn"));
            post_data['action'] = 'set';
            post_data = $.extend(post_data, extra_post_data);
            $.ajax({
                url:"nn.action.php"
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
