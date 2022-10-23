"use strict";

layui.use(['layer', 'element', 'form', 'slider'], function(){
    let layer = layui.layer
        ,element = layui.element
        ,form = layui.form
        ,slider = layui.slider;

    let extra_post_data = [];
    let current_tab_id = {
        'multi-channel': '',
        ts: 'main',
        vr: 'recording',
    };

    let device_elements_id = ['ts_main', 'ts_sub', 'vr_recording'];
    let gdc_config_element_id = ['ts_gdc', 'vr_gdc'];

    let video_dev_options = call_remote_function('video.php', 'list_video_devices');
    let gdc_cfg_options = call_remote_function('video.php', 'list_gdc_config_files');
    let video_codec_options = ['h264', 'h265'];
    let framerate_options = call_remote_function('video.php', 'list_video_framerates');

    let bitrate_type_options = [
        {val: "CBR", text: "CBR"},
        {val: "VBR", text: "VBR"},
    ];

    let isp_resolutions = call_remote_function('video.php', 'get_isp_resolution')['ret'];
    let resolution_options = isp_resolutions.split(",");

    let sub_resolution_options = [
        {val: "352x288", text: "352x288 (CIF)"},
        {val: "640x480", text: "640x480 (VGA)"},
        {val: "704x576", text: "704x576 (D1)"},
    ];
    let sliders = {
        bitrate: {min: 200, max: 12000, step: 100},
        gop: {min: 1, max: 100, step: 1},
        "video-quality-level": {min: 1, max: 6, step: 1},
    };

    let slider_controls = [];

    device_elements_id.forEach(function(id) {
        for (let n in sliders) {
            slider_controls[id + '_' + n] = slider.render({
                elem: '#' + id + '_' + n
                , value: sliders[n].min
                , min: sliders[n].min
                , max: sliders[n].max
                , step: sliders[n].step
                , input: true
                , setTips:
                    function (value) {
                        extra_post_data[id + '_' + n] = value;
                        return value;
                    },
                change: function (value) {
                    extra_post_data[id + '_' + n] = value;
                }
            });
        }
    });

    device_elements_id.forEach(function(did) {
        let e = document.getElementById(did + '_device');
        option_set_values(e, video_dev_options, true);
        e = document.getElementById(did + '_framerate');
        option_set_values(e, framerate_options, true);
        e = document.getElementById(did + '_codec');
        option_set_values(e, video_codec_options, true);
        e = document.getElementById(did + '_resolution');
        if (did === 'ts_sub') {
            option_set_values(e, sub_resolution_options, false);
        } else {
            option_set_values(e, resolution_options, true);
        }
        //bitrate type
        e = document.getElementById(did + '_bitrate-type');
        option_set_values(e, bitrate_type_options);
    });
    gdc_config_element_id.forEach(function(did) {
        let e = document.getElementById(did + '_config-file');
        option_set_values(e, gdc_cfg_options, true);
    });

    set_form_value('multi-channel', '');
    set_form_value('ts', 'main');

    function set_form_value(ch_id, tab_id, index = 0) {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'video.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get", ch: ch_id, tab: tab_id, index: index}
            ,success: function (data) {
                for (let k in data) {
                    let eid = '';
                    if (ch_id !== 'multi-channel') {
                        eid = ch_id + '_' + tab_id + '_';
                    } else {
                        refresh_vr_settings(data[k] === 'true');
                    }
                    eid += k;
                    if (Object.keys(sliders).includes(k)) {
                        let v = data[k] - sliders[k].min;
                        if (k === 'bitrate') {
                            v /= 100;
                        }
                        slider_controls[eid].setValue(v);
                    } else if (k === 'config-file') {
                        if (data[k]) $('#' + eid).val(basename(data[k]));
                    } else if (k === 'enabled' || k === 'multi-channel') {
                        if (k === 'enabled') {
                            extra_post_data[eid] = data[k];
                        } else {
                            extra_post_data[k] = data[k];
                        }
                        if (data[k] === "true") {
                            $("#" + eid).prop("checked", true);
                        } else {
                            $("#" + eid).prop("checked", false);
                        }
                    } else if (k === 'index' && tab_id == 'sub') {
                        let element = document.getElementById(eid);
                        option_set_values(element, data[k], true);
                        option_set_selected(element, index);
                    } else if (k === "bitrate-type") {
                        $('#' + eid).val(data[k]);
                        refresh_bitrate_type(data[k], ch_id + "_" + tab_id);
                    } else {
                        $('#' + eid).val(data[k]);
                    }
                }
                form.render();
            }
            ,complete: function () {
                layer.close(layer_id);
            }
        });
    }

    element.on('tab(tab)', function(elem){
        let id = $(this).attr('lay-id');
        let ch = $(this).closest('div.layui-tab').attr('ch');
        if (current_tab_id[ch] !== id) {
            current_tab_id[ch] = id;
            set_form_value(ch, id);
        }
    });

    function refresh_vr_settings(enabled) {
        extra_post_data['multi-channel'] = enabled;
        if (enabled) {
            set_form_value('vr', 'recording');
            $('#div_vr').show();
        } else {
            $('#div_vr').hide();
        }
    }

    function refresh_bitrate_type(action, channel) {
        if (action === "VBR") {
            $("#div_" + channel + "_video-quality-level").show();
        } else {
            $("#div_" + channel + "_video-quality-level").hide();
        }
    }

    form.on('switch(enabled)', function(data){
        extra_post_data[data.elem.name] = data.elem.checked;
        if (data.elem.name === 'multi-channel') {
            refresh_vr_settings(data.elem.checked);
            form.render();
        }
    });

    form.on('select(config-file)', function(data) {
        let ch = $(this).closest('div.layui-tab').attr('ch');
        let tab = current_tab_id[ch];
        let res = data.value.split('-');
        let in_res = res[1].split('_');
        var out_res = res[2].split('_');
        $('#' + ch + '_' + tab + '_input-resolution').val(in_res[0] + 'x' + in_res[1]);
        $('#' + ch + '_' + tab + '_output-resolution').val(out_res[2] + 'x' + out_res[3]);
        form.render();
    });


    form.on('select(ts_sub_index)', function(data) {
        set_form_value('ts', 'sub', data.value - 1);
    });

    function bitrate_type_form_on(channel) {
        form.on('select(' + channel + '_bitrate-type)' , function(data) {
            refresh_bitrate_type(data.value, channel);
            form.render();
        });
    }
    bitrate_type_form_on("ts_main");
    bitrate_type_form_on("ts_sub");
    bitrate_type_form_on("vr_recording");

    $(document).ready(function () {
        $(".layui-btn").click(function (e) {
            e.preventDefault();
            let name = $(this).attr('name');
            let ch = $(this).closest('div.layui-tab').attr('ch');
            let tab = current_tab_id[ch];
            let index = $('#ts_sub_index option:selected').val() - 1;
            if (name === 'reset') {
                set_form_value(ch, tab, index);
            } else {
                let post_data = getFormData($("#form_ipc_video"));
                post_data['action'] = 'set';
                post_data['ch'] = ch;
                post_data['tab'] = tab;
                post_data['index'] = index;
                if (tab === 'gdc') {
                    post_data[ch + '_' + tab + '_input-resolution'] = $('#' + ch + '_' + tab + '_input-resolution').val();
                    post_data[ch + '_' + tab + '_output-resolution'] = $('#' + ch + '_' + tab + '_output-resolution').val();
                }
                post_data = $.extend(post_data, extra_post_data);
                let layer_id = layer.load(0, {time: 10 * 1000});
                $.ajax({
                    url: "video.action.php"
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
