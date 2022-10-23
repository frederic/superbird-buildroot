"use strict";

layui.use(['layer', 'element', 'form'], function(){
    var layer = layui.layer
        ,element = layui.element
        ,form = layui.form;

    let current_tab_id = 'audio';
    let extra_post_data = [];
    let options_list = {
        audio_codec: {
            options: [ 'g711', 'g726', 'aac' ],
        },
        audio_bitrate: {
            groups: {
                g711: ['64'],
                g726: ["16", "24", "32", "40"],
                aac: ["16", "24", "32", "40", "64", "96", "128", "160", "192", "256", "320"],
            },
        },
        audio_samplerate: {
            groups: {
                g711: ['8000'],
                g726: ['8000'],
                aac: ['8000', '11025', '12000', '16000', '22050', '24000', '32000', '44100', '48000', '64000', '88200', '96000']
            },
        }
    };
    for(let eid in options_list) {
        if (eid === 'audio_codec') {
            let e = document.getElementById(eid);
            option_set_values(e, options_list[eid].options, true);
        }
    }

    set_form_value();

    function set_form_value() {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'audio.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get", tab: current_tab_id}
            ,success: function (data) {
                for (let k in data) {
                    let eid = current_tab_id + '_' + k;

                    if (k === 'codec') {
                        let codec = data[k];
                        if (codec === 'mulaw') {
                            codec = data[k] = 'g711';
                        }
                        let e = document.getElementById('audio_bitrate');
                        option_set_values(e, options_list.audio_bitrate.groups[codec], true);
                        e = document.getElementById('audio_samplerate');
                        option_set_values(e, options_list.audio_samplerate.groups[codec], true);
                    }

                    if (k === 'enabled') {
                        $('#' + eid).prop("checked", data[k] === 'true');
                        extra_post_data[eid] = data[k];
                    } else {
                        let e = document.getElementById(eid);
                        if (e.disabled) {
                            extra_post_data[eid] = data[k];
                        }
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

    element.on('tab(audio-tab)', function(elem){
        let id = $(this).attr('lay-id');
        if (current_tab_id !== id) {
            current_tab_id = id;
            set_form_value();
        }
    });
    form.on('select(codec)', function (data) {
        let codec = data.value;
        let e = document.getElementById('audio_bitrate');
        option_set_values(e, options_list.audio_bitrate.groups[codec], true);
        e = document.getElementById('audio_samplerate');
        option_set_values(e, options_list.audio_samplerate.groups[codec], true);
        form.render('select');
    });

    form.on('switch(enabled)', function(data){
        extra_post_data[data.elem.name] = data.elem.checked;
    });

    $(document).ready(function () {
        $("#reset").click(function (e) {
            e.preventDefault();
            set_form_value();
        });
        $("#submit").click(function(e) {
            e.preventDefault();
            let layer_id = layer.load(0, {time: 10*1000});
            let post_data = getFormData($("#form_ipc_audio"));
            post_data['action'] = 'set';
            post_data['tab'] = current_tab_id;
            post_data = $.extend(post_data, extra_post_data);
            $.ajax({
                url:"audio.action.php"
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
