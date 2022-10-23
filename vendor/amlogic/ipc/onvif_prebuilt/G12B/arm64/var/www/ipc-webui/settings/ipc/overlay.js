"use strict";

layui.use(['layer', 'element', 'form','colorpicker', 'upload', 'slider'], function(){
    let layer = layui.layer
        ,element = layui.element
        ,colorpicker = layui.colorpicker
        ,upload = layui.upload
        ,form = layui.form
        ,slider = layui.slider
    ;

    let font_files = [];
    let image_files = [];
    let extra_post_data = [];
    let current_tab_id = 'datetime';
    let upload_info = {
        font: {
            accept: 'file',
            exts: 'ttf',
            mime: 'font/opentype',
            dir: 'font',
        },
        image: {
            accept: 'images',
            exts: 'jpg|jpeg|JPG|JPEG|png|PNG',
            mime: 'image/*',
            dir: 'image',
        }
    };
    let uploaders = {
        'datetime_font_font-file': 'font',
        'watermark_text_font_font-file': 'font',
        'watermark_image_file': 'image',
        'nn_font_font-file': 'font',
    };
    function create_uploader(eid) {
        let info = upload_info[uploaders[eid]];
        $('#' + eid).off('click');
        $('#' + eid + '_upload').off('click');
        $('#' + eid + '_upload').off('change');
        $('#' + eid + '_upload').data('haveEvents', false);
        let layer_id = 0;
        let uploader = upload.render({
            elem: '#' + eid + '_upload'
            , url: 'overlay.upload.php'
            , accept: info.accept
            , exts: info.exts
            , acceptMime: info.mime
            , auto: true
            , drag: false
            , data: {dir: info.dir, eid: eid}
            , choose: function (obj) {
                layer_id = layer.load(0, {time: 10*1000});
                obj.preview(function (index, file, result) {
                    let eid = uploader.config.data.eid;
                    let element = document.getElementById(eid);
                    let idx = option_value_exists(element, file.name);
                    if (idx > 0) {
                        option_set_selected(element, idx);
                    } else {
                        $('#' + eid).prepend(
                            new Option(file.name, file.name, true, true)
                        );
                    }
                    form.render();
                });
            }
            , before: function (obj) {

            }
            , done: function (res, index, upload) {
                layer.close(layer_id);
                if (res.code === 0) {
                    update_file_list();
                }
            }
        });
    }

    let resolution = call_remote_function('video.php', 'get_ts_main_video_resolution');

    let position_elements = {
        watermark_text_position_left : 'width',
        watermark_text_position_top : 'height',
        watermark_image_position_left : 'width',
        watermark_image_position_top : 'height',
        watermark_image_position_width : 'width',
        watermark_image_position_height : 'height',
    };
    for (let e in position_elements) {
        $('#' + e).attr({
            'min': 0,
            'max' : resolution[position_elements[e]]
        });
        document.getElementById(e).onkeyup = check_input_range;
    }

    let sliders = {
        datetime_font_size: { min: 24, max: 64, },
        watermark_text_font_size: { min: 24, max: 64, },
        nn_font_size: { min: 10, max: 48, },
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
    let select_infos = {
        position: {
            options: [
                {val: 'top-left', text: gettext('Top Left')},
                {val: 'top-mid', text: gettext('Top Middle')},
                {val: 'top-right', text: gettext('Top Right')},
                {val: 'mid-left', text: gettext('Middle Left')},
                {val: 'center', text: gettext('Center')},
                {val: 'mid-right', text: gettext('Middle Right')},
                {val: 'bot-left', text: gettext('Bottom Left')},
                {val: 'bot-mid', text: gettext('Bottom Middle')},
                {val: 'bot-right', text: gettext('Bottom Right')},
            ],
            value_as_text: false,
        },
    };
    let selects = [
        {
            tab: 'datetime',
            elements: [
                {id: 'position', type: 'position'},
            ],
        },
    ];
    selects.forEach(function(s){
        s.elements.forEach(function(e){
            let id = s.tab + '_' + e.id;
            let element = document.getElementById(id);
            option_set_values(element, select_infos[e.type].options, select_infos[e.type].value_as_text);
        });
    });

    update_file_list();
    set_form_value();
    form.render();

    function update_file_list() {
        $.ajax({
            url: 'overlay.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "file", tab: current_tab_id}
            ,success: function(data) {
                font_files = data['font'];
                image_files = data['image'];
            }
        });
    }

    function set_form_value(index = 0) {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'overlay.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get", tab: current_tab_id, index: index}
            ,success: function (data) {
                for (let k in data) {
                    let eid = current_tab_id + '_' + k;
                    if (Object.keys(uploaders).includes(eid)) {
                        create_uploader(eid);
                        let element = document.getElementById(eid);
                        option_set_values(element, uploaders[eid] === 'font' ? font_files : image_files, true);
                        $('#' + eid).val(data[k]);
                    } else if (Object.keys(sliders).includes(eid)) {
                        let v = data[k] - sliders[eid].min;
                        slider_controls[eid].setValue(v);
                    } else if (k.indexOf('color') > -1) {
                        colorpicker.render({
                            elem: '#' + eid + '_picker'
                            , color: hex2rgba(data[k])
                            , format: "rgb"
                            , alpha: true
                            , done: function (color) {
                                $('#' + eid).val(rgba2hex(color));
                            }
                        });
                        $('#' + eid).val(data[k]);
                    } else if (k === 'index') {
                        let element = document.getElementById(eid);
                        option_set_values(element, data[k], true);
                        option_set_selected(element, index);
                    } else if (k === 'enabled' || k === 'show') {
                        $('#' + eid).prop("checked", data[k] === 'true');
                        extra_post_data[eid] = data[k];
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

    form.on('select(index)', function (data) {
        set_form_value(data.value - 1);
    });

    element.on('tab(tab)', function(elem){
        let id = $(this).attr('lay-id');
        if (current_tab_id !== id) {
            current_tab_id = id;
            set_form_value();
        }
    });

    form.on('switch(enabled)', function(data){
        extra_post_data[data.elem.name] = data.elem.checked;
    });

    $(document).ready(function () {
        $("#reset").click(function (e) {
            e.preventDefault();
            let tab = current_tab_id;
            let index = 0;
            if (tab.startsWith('watermark')) {
                index = $('#' + tab + '_index').val() - 1;
            }
            set_form_value(index);
        });

        $("#submit").click(function (e) {
            e.preventDefault();
            let tab = current_tab_id;
            let index = 0;
            if (tab.startsWith('watermark')) {
                index = $('#' + tab + '_index').val() - 1;
            }

            let layer_id = layer.load(0, {time: 10 * 1000});
            let post_data = getFormData($("#form_ipc_overlay"));
            post_data['action'] = 'set';
            post_data['tab'] = tab;
            post_data['index'] = index;
            post_data = $.extend(post_data, extra_post_data);
            $.ajax({
                url: "overlay.action.php"
                , type: "post"
                , dataType: "json"
                , data: post_data
                , complete: function () {
                    layer.close(layer_id);
                }
            });
        });
    });

});
