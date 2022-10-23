"use strict";

layui.use(['element', 'layer', 'jquery', 'slider', 'form'], function () {
    let element = layui.element
        , layer = layui.layer
        , $ = layui.$
        , slider = layui.slider
        , form = layui.form;

    let extra_post_data = [];
    let v_fr = call_remote_function('video.php', 'get_ts_main_video_framerate')['ret'];
    let sliders = {
        brightness: { min: 0, max: 255, step: 1},
        contrast: { min: 0, max: 255, step: 1},
        sharpness: { min: 0, max: 255, step: 1},
        saturation: { min: 0, max: 255, step: 1},
        hue: { min: 0, max: 360, step: 1},
        whitebalance_cbgain: { min: 28, max: 228, step: 1},
        whitebalance_crgain: { min: 28, max: 228, step: 1},
        compensation_hlc: { min: 0, max: 128, step: 1},
        compensation_blc: { min: 0, max: 128, step: 1},
        exposure_absolute: { min: 1, max: 1000 / v_fr, step: 1},
        'nr_space-normal': { min: 0, max: 255, step: 1},
        'nr_space-expert': { min: 0, max: 255, step: 1},
        nr_time: { min: 0, max: 160, step: 1},
        gain_manual: { min: 0, max: 127, step: 1},
        defog_auto: { min: 0, max: 4096, step: 1},
        defog_manual: { min: 0, max: 4096, step: 1}
    };
    let slider_controls = [];
    for (let id in sliders) {
        slider_controls[id] = slider.render({
            elem: '#' + id
            , value: sliders[id].min
            , min: sliders[id].min
            , max: sliders[id].max
            , step: sliders[id].step
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
            id: "anti-banding",
            options: [
                {val: '50', text:'50(PAL)'},
                {val: '60', text:'60(NTSC)'}
            ],
        },
        {
            id: "mirroring",
            options: [
                {val: 'NONE', text: gettext("Disabled")},
                {val: 'H', text: gettext("Left Right")},
                {val: 'V', text: gettext("Up Down")},
                {val: 'HV', text: gettext("Central")}
            ],
        },
        {
            id: "rotation",
            options: [
                {val: '0', text: gettext("Disabled")},
                {val: '90', text: '90 ' + gettext("Degrees")},
                {val: '180', text: '180 ' + gettext("Degrees")},
                {val: '270', text: '270 ' + gettext("Degrees")}
            ],
        },
        {
            id: "compensation_action",
            options: [
                {val: 'disable', text: gettext("Disabled")},
                {val: 'hlc', text: gettext("High Light")},
                {val: 'blc', text: gettext("Back Light")}
            ],
        },
        {
            id: "nr_action",
            options: [
                {val: 'disable', text: gettext("Disabled")},
                {val: 'normal', text: gettext("Normal")},
                {val: 'expert', text: gettext("Expert")}
            ],
        },
        {
            id: "defog_action",
            options: [
                {val: 'disable', text: gettext("Disabled")},
                {val: 'auto', text: gettext("Auto")},
                {val: 'manual', text: gettext('Manual')}
            ]
        }
    ];
    options_list.forEach(function(l) {
        let e = document.getElementById(l.id);
        option_set_values(e, l.options);
    });
    set_form_value();

    function refresh_compensation(action) {
        if (action === "hlc") {
            $("#div_light_compensation_hlc").show();
            $("#div_light_compensation_blc").hide();
        }else if (action === "blc") {
            $("#div_light_compensation_hlc").hide();
            $("#div_light_compensation_blc").show();
        } else {
            $("#div_light_compensation_hlc").hide();
            $("#div_light_compensation_blc").hide();
        }
    }

    function refresh_nr(action) {
        if (action === "normal") {
            $("#div_nr_space-normal").show();
            $("#div_nr_space-expert").hide();
        } else if (action === "expert") {
            $("#div_nr_space-normal").hide();
            $("#div_nr_space-expert").show();
        } else {
            $("#div_nr_space-normal").hide();
            $("#div_nr_space-expert").hide();
        }
    }

    function refresh_defog(action) {
        if (action === "auto") {
            $("#div_defog_auto").show();
            $("#div_defog_manual").hide();
        } else if (action === "manual") {
            $("#div_defog_auto").hide();
            $("#div_defog_manual").show();
        } else {
            $("#div_defog_auto").hide();
            $("#div_defog_manual").hide();
        }
    }

    let switch_div_relation = {
        exposure_auto: 'div_exposure_absolute',
        gain_auto: 'div_gain',
        whitebalance_auto: 'div_whitebalance',
    };

    function on_switch_action(element_name, checked) {
        if (Object.keys(switch_div_relation).includes(element_name)) {
            if(checked) {
                $('#' + switch_div_relation[element_name]).hide();
            } else {
                $('#' + switch_div_relation[element_name]).show();
            }
        }
    }


    function set_form_value() {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'isp.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get"}
            ,success: function (data) {
                for (let k in data) {
                    if ((k.indexOf('auto') > -1 || k.indexOf('enabled') > -1) && k != 'defog_auto') {
                        extra_post_data[k] = data[k];
                        $('#' + k).prop("checked", data[k] === 'true');
                        on_switch_action(k, data[k] === 'true');
                    } else if (k === 'compensation_action') {
                        $('#' + k).val(data[k]);
                        refresh_compensation(data[k]);
                    } else if (k === 'nr_action') {
                        $('#' + k).val(data[k]);
                        refresh_nr(data[k]);
                    } else if (k === "defog_action") {
                       $('#' + k).val(data[k]);
                       refresh_defog(data[k]);
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
    form.on('select(compensation)', function (data) {
        refresh_compensation(data.value);
        form.render();
    });
    form.on('select(nr_action)', function (data) {
        refresh_nr(data.value);
        form.render();
    });
    form.on('select(defog_action)', function (data) {
        refresh_defog(data.value);
        form.render();
    });

    form.on('switch(enabled)', function(data){
        extra_post_data[data.elem.name] = data.elem.checked;
        on_switch_action(data.elem.name, data.elem.checked);
    });

    $(document).ready(function () {
        $("#reset").click(function (e) {
            e.preventDefault();
            set_form_value();
        });
        $("#submit").click(function(e) {
            e.preventDefault();
            let layer_id = layer.load(0, {time: 10*1000});
            let post_data = getFormData($("#form_isp"));
            post_data['action'] = 'set';
            post_data = $.extend(post_data, extra_post_data);
            $.ajax({
                url:"isp.action.php"
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

