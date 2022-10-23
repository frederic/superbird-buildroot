"use strict";

layui.use(['element', 'layer', 'jquery', 'slider', 'form', 'laydate'], function () {
    let element = layui.element
        , layer = layui.layer
        , $ = layui.$
        , slider = layui.slider
        , form = layui.form
        , laydate = layui.laydate;

    let extra_post_data = [];
    let sliders = {
        delay: { min: 5, max: 30, step: 1},
    };
    let slider_controls = [];
    for (let id in sliders) {
        slider_controls[id] = slider.render({
            elem: '#' + id
            , value: sliders[id].min
            , min: sliders[id].min
            , max: sliders[id].max
            , step: 1
            , input: true
            , setTips:
                function (value) {
                    extra_post_data[id] = value;
                    return value;
                },
            change: function (value) {
                extra_post_data[id] = value;
            }
        });
    }
    let options_list = [
        {
            id: "mode",
            options: [
                {val: 'day', text:gettext('Day')},
                {val: 'night', text:gettext('Night')},
                {val: 'auto', text:gettext('Auto')},
                {val: 'schedule', text:gettext('Schedule')}
            ],
        },
    ];
    options_list.forEach(function(l) {
        let e = document.getElementById(l.id);
        option_set_values(e, l.options);
    });

    laydate.render({
        elem: '#schedule_begin'
        ,type: 'time'
    });
    laydate.render({
        elem: '#schedule_end'
        ,type: 'time'
    });
    set_form_value();

    function update_schedule(mode) {
        if (mode === "schedule") {
            $("#div_schedule").show();
        } else {
            $("#div_schedule").hide();
        }
        if (mode === "auto") {
            $("#div_delay").show();
        } else {
            $("#div_delay").hide();
        }
    }

    function set_form_value() {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'daynight.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get"}
            ,success: function (data) {
                for (let k in data) {
                    if (k === 'mode') {
                        $('#' + k).val(data[k]);
                        update_schedule(data[k]);
                    } else if (Object.keys(sliders).includes(k)) {
                        let v = data[k] - sliders[k].min;
                        slider_controls[k].setValue(v);
                    } else {
                        $('#' + k).val(data[k]);
                    }
                }
                form.render();
            }
            ,complete: function() {
                layer.close(layer_id);
            }
        });
    }
    form.on('select(mode)', function (data) {
        update_schedule(data.value);
        form.render();
    });

    $(document).ready(function () {
        $("#reset").click(function (e) {
            e.preventDefault();
            set_form_value();
        });
        $("#submit").click(function(e) {
            e.preventDefault();
            let layer_id = layer.load(0, {time: 10*1000});
            let post_data = getFormData($("#form_daynight"));
            post_data['action'] = 'set';
            post_data = $.extend(post_data, extra_post_data);
            $.ajax({
                url:"daynight.action.php"
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

