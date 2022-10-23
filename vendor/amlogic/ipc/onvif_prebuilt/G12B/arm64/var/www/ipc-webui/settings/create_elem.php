<?php
function create_slider($elem_name, $min, $max, $step, $cookie_name)
{
    echo sprintf('
        slider.render({
            elem: "#%1$s"
            ,value: 0
            ,min: %2$s
            ,max: %3$s
            ,step: %4$s
            ,setTips: function(value){
                document.cookie="%5$s"+"="+value;
                return value;
            }
            ,change: function(value){
                document.cookie="%5$s"+"="+value;
            }
        })
        ', $elem_name, $min, $max, $step, $cookie_name);
}

function create_select($sel_list, $cur_val, $sel_id, $sel_display_list=null)
{
    echo sprintf('
        $("#%3$s").empty();
        for(var i=0; i<%1$s.length; i++){
            if(%1$s[i] === %2$s){
                $("#%3$s").append(\'<option value=\\"\'+%1$s[i]+\'\\" selected>\'+%4$s[i]+\'</option>\');
            }else{
                $("#%3$s").append(\'<option value=\\"\'+%1$s[i]+\'\\">\'+%4$s[i]+\'</option>\');
            }
        }
        ', $sel_list, $cur_val, $sel_id, is_null($sel_display_list) ? $sel_list : $sel_display_list);
}

function create_switch($en_filter, $div_id)
{
    if ($en_filter == 'i_exp' || $en_filter == 'i_whau') {
        echo sprintf('
            form.on(\'switch(%1$s)\', function(data){
                if(data.elem.checked){
                    document.cookie = "%1$s"+"=true";
                    $("#%2$s").hide();
                    form.render();
                }else{
                    document.cookie = "%1$s"+"=false";
                    $("#%2$s").show();
                    form.render();
                }
             });
             ', $en_filter, $div_id);
    } else {
        echo sprintf('
            form.on(\'switch(%1$s)\', function(data){
                if(data.elem.checked){
                    document.cookie = "%1$s"+"=true";
                    $("#%2$s").show();
                    form.render();
                }else{
                    document.cookie = "%1$s"+"=false";
                    $("#%2$s").hide();
                    form.render();
                }
            });
            ', $en_filter, $div_id);
    }
}

function create_judge_switch($index, $div_en, $lay_filter, $div_dis, $sw_id)
{
    echo sprintf('
        document.cookie = "%1$s" + "=" + %2$s;
        ', $lay_filter, $index);
    if ($div_dis == "none") {
        echo sprintf('
            if(%1$s === "true"){
                $("#%2$s").empty().append("<input type=\'checkbox\' lay-skin=\'switch\' checked lay-filter=\'%3$s\' value=\'true\' name=\'%4$s\'>");
            }else{
                $("#%2$s").empty().append("<input type=\'checkbox\' lay-skin=\'switch\' lay-filter=\'%3$s\' value=\'false\' name=\'%4$s\'>");
            }
            ', $index, $div_en, $lay_filter, $sw_id);
    } elseif ($div_en == "i_exau" || $div_en == "i_whau") {
        echo sprintf('
            if(%1$s === "true"){
                $("#%2$s").empty().append("<input type=\'checkbox\' lay-skin=\'switch\' checked lay-filter=\'%4$s\' value=\'true\' name=\'%5$s\'>");
                $("#%3$s").hide();
            }else{
                $("#%2$s").empty().append("<input type=\'checkbox\' lay-skin=\'switch\' lay-filter=\'%4$s\' value=\'false\' name=\'%5$s\'>");
                $("#%3$s").show();
            }
            ', $index, $div_en, $div_dis, $lay_filter, $sw_id);
    } else {
        echo sprintf('
            if(%1$s === "true"){
                $("#%2$s").empty().append("<input type=\'checkbox\' lay-skin=\'switch\' checked lay-filter=\'%4$s\' value=\'true\' name=\'%5$s\'>");
                $("#%3$s").show();
            }else{
                $("#%2$s").empty().append("<input type=\'checkbox\' lay-skin=\'switch\' lay-filter=\'%4$s\' value=\'false\' name=\'%5$s\'>");
                $("#%3$s").hide();
            }
            ', $index, $div_en, $div_dis, $lay_filter, $sw_id);
    }
}

function create_colorpickers($elem, $val)
{
    echo sprintf('
        colorpicker.render({
            elem: "#%1$s-form"
            ,color: %2$s
            ,format: "rgb"
            ,alpha: true
            ,done: function(color){
                $("#%1$s-form-input").val(rgba2hex(color));
            }
        });
        ', $elem, $val);
}

function create_upload($elem, $sel_id, $btn_id)
{
    echo sprintf('
        upload.render({
            elem: "#%1$s"
            ,url: "./ipc/up_ttf.php"
            ,accept: "file"
            ,exts: "ttf"
            ,auto: false
            ,bindAction: "#%2$s"
            ,choose: function(obj){
                var files = obj.pushFile();
                obj.preview(function(index, file, result){
                    fileName = file.name;
                    $("#%3$s").prepend(\'<option value=\\"\'+file.name+\'\\" selected>\'+file.name+\'</option>\');
                    form.render();
                });
            }
            ,done: function(res, index, upload){
                layer.closeAll("loading");
            }
        });
        ', $elem, $btn_id, $sel_id);
}

function re_create_switch($data, $cookie_name, $div_en, $div_prop, $sw_filter, $sw_id, $rm_hid)
{
    echo sprintf('
        if(%1$s === "true") {
            document.cookie = "%2$s"+"=true";
            $("#%3$s").empty().append("<input type=\'checkbox\' lay-skin=\'switch\' checked lay-filter=\'%4$s\' value=\'true\' name=\'%5$s\'>");', $data, $cookie_name, $div_en, $sw_filter, $sw_id);
    if ($rm_hid == "yes") {
        echo sprintf('
            $("#%1$s").removeAttr("hidden");', $div_prop);
    }
    if ($div_prop != "none") {
        if (($data == 'data[0]["exposure/auto"]') || ($data == 'data[0]["whitebalance/auto"]')) {
            echo sprintf('
            document.getElementById("%s").style.display="none";', $div_prop);
        } else {
            echo sprintf('
            document.getElementById("%s").style.display="";', $div_prop);
        }
    }
    echo sprintf('
        }else{
            document.cookie = "%1$s"+"=false";
            $("#%2$s").empty().append("<input type=\'checkbox\' lay-skin=\'switch\' lay-filter=\'%3$s\' value=\'false\' name=\'%4$s\'>");', $cookie_name, $div_en, $sw_filter, $sw_id);
    if ($div_prop != "none") {
        if (($data == 'data[0]["exposure/auto"]') || ($data == 'data[0]["whitebalance/auto"]')) {
            echo sprintf('
            document.getElementById("%s").style.display="";', $div_prop);
        } else {
            echo sprintf('
            document.getElementById("%s").style.display="none";', $div_prop);
        }
    }
    echo '
        }
        ';
}

function re_render()
{
    echo '
        layui.use("form", function(){
            var form = layui.form;
            form.render();
        });
        ';
}

function create_Vgdc($target, $action, $part)
{
    if ($part != "v_vr_g") {
        echo sprintf('
        <form target="%1$s" class="layui-form layui-form-pane" action="./ipc/%2$s" method="post" enctype="multipart/form-data" onreset="reset_%3$s()">', $target, $action, $part);
    }
    echo sprintf('
            <div class="layui-form-item">
                <label class="layui-form-label">%2$s</label>
                <div class="layui-input-inline" id="%1$s_en"></div>
            </div>
            <div id="%1$s_prop" hidden>
                <div class="layui-form-item">
                   <label class="layui-form-label">%3$s</label>
                   <div class="layui-input-inline">
                        <select name="%1$s_config_file" lay-filter="%1$s_cong" id="%1$s_conf"></select>
                   </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label" style="width: auto">%4$s</label>
                    <div class="layui-input-inline">
                        <input type="text" name="%1$s_in_reso" class="layui-input layui-disabled" autocomplete="off" id="%1$s_in_reso">
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label" style="width: auto">%5$s</label>
                    <div class="layui-input-inline">
                        <input type="text" name="%1$s_out_reso" class="layui-input layui-disabled" autocomplete="off" id="%1$s_out_reso">
                    </div>
                </div>
            </div>', $part, gettext("enabled"), gettext("config file"), gettext("input resolution"), gettext("output resolution"));
    if ($part != "v_vr_g") {
        echo sprintf('<div class="layui-form-item">
                <div class="layui-input-inline">
                    <button class="layui-btn" lay-submit lay-filter="demo1" name="gdc_set">%s</button>
                    <button type="reset" class="layui-btn layui-btn-primary">%s</button>
                </div>
            </div>
            <iframe name="%s" style="display:none"></iframe>
        </form>', gettext("Submit"), gettext("Reset"), $target);
    }
}

function create_Vform($target, $action, $part)
{
    echo sprintf('
        <form target="%1$s" class="layui-form layui-form-pane" action="./ipc/%2$s" method="post" onreset="reset_%3$s()" lay-filter="%3$s_form" id="%3$s_form">', $target, $action, $part);
    if ($part == "v_ts_s") {
        echo sprintf('
                <div class="layui-form-item">
                    <label class="layui-form-label">%s</label>
                    <div class="layui-input-inline" id="v_ts_s_en"></div>
                </div>
             <div id="v_ts_s_prop" hidden>', gettext("enabled"));
    }
    echo sprintf('
            <div class="layui-form-item">
                <label class="layui-form-label" style="margin-right:15px;">%2$s</label>
                <div class="layui-input-inline">
                    <select name="%1$s_device" lay-filter="%1$s_dev" id="%1$s_device"></select>
                </div>
            </div>
            <div class="layui-form-item">
                <label class="layui-form-label" style="margin-right:15px;">%3$s</label>
                <div class="layui-input-inline">
                    <br>
                    <div id="slide_%1$s_bitrate" class="demo-slider"></div>
                </div>
            </div>
            <div class="layui-form-item">
                <label class="layui-form-label">%4$s</label>
                <div class="layui-input-inline">
                    <select name="%1$s_codec" lay-filter="%1$s_codec" id="%1$s_codec"></select>
                </div>
            </div>
            <div class="layui-form-item">
                <label class="layui-form-label">%5$s</label>
                <div class="layui-input-inline">
                    <select name="%1$s_framerate" lay-filter="%1$s_frate" id="%1$s_framerate"></select>
                </div>
            </div>
            <div class="layui-form-item">
                <label class="layui-form-label" style="margin-right:15px;">%6$s</label>
                <div class="layui-input-inline">
                    <br>
                    <div id="slide_%1$s_gop" class="demo-slider"></div>
                </div>
            </div>
            <div class="layui-form-item">
                <label class="layui-form-label">%7$s</label>
                <div class="layui-input-inline">
                    <select name="%1$s_reso" lay-filter="%1$s_reso" id="%1$s_reso"></select>
                </div>
            </div>
            ', $part, gettext("device"), gettext("bitrate"), gettext("codec"), gettext("framerate"), gettext("GOP"), gettext("resolution"));

    if ($part == "v_vr") {
        echo sprintf('
            <div class="layui-form-item">
                <div class="layui-collapse" lay-filter="v_vr_gdc">
                    <h2 class="layui-colla-title">%s</h2>
                    <div class="layui-colla-content">
                        ', gettext("GDC"));
        create_Vgdc("", "", "v_vr_g");
        echo '
                    </div>
                </div>
            </div>';
    }

    if ($part == "v_ts_s") {
        echo '</div>';
    }
    echo sprintf('
            <div class="layui-form-item">
                <div class="layui-input-inline">
                    <button class="layui-btn" id="%1$s_btn" lay-submit lay-filter="%1$s_submit">%3$s</button>
                    <button type="reset" class="layui-btn layui-btn-primary">%4$s</button>
                </div>
            </div>
            <iframe name="%2$s" style="display:none"></iframe>
        </form>
        ', $part, $target, gettext("Submit"), gettext("Reset"));
}

function create_Vreset($part, $prop, $bit_id, $gop_id)
{
    echo sprintf('
        function reset_%s(){
            $.ajax({
                url:"./load.php",
                type:"post",
                dataType:"json",
                data:{prop:"%s"},
                success:function (data){
                    ', $part, $prop);
    if ($part == "v_ts_s") {
        re_create_switch('data[0]["enabled"]', 'v_ts_s_en', 'v_ts_s_en', 'v_ts_s_prop', 'v_ts_s_en', 'v_ts_s_btn', 'yes');
    }
    echo sprintf('
                    var vb_val = data[0]["bitrate"];
                    var vg_val = data[0]["gop"];
                    %1$s.setValue((data[0]["bitrate"]-500)/500);
                    %2$s.setValue(data[0]["gop"]-0+1);
                    $("#%3$s_codec").val(data[0]["codec"]);
                    $("#%3$s_device").val(data[0]["device"]);
                    $("#%3$s_framerate").val(data[0]["framerate"]);
                    $("#%3$s_reso").val(data[0]["resolution"]);', $bit_id, $gop_id, $part);
    if ($part == "v_vr") {
        create_Vreset_gdc("v_vr", "none");
    }
    re_render();
    echo '
                }
            })
        }';

}

function create_Vreset_gdc($part, $prop)
{
    if ($part != "v_vr") {
        echo sprintf('
            function reset_%s(){
                $.ajax({
                    url:"./load.php",
                    type:"post",
                    dataType:"json",
                    data:{prop:"%s"},
                    success:function (data){
                        ', $part, $prop);
        re_create_switch('data[0]["enabled"]', 'v_ts_g_en', 'v_ts_g_en', 'v_ts_g_prop', 'v_ts_g_en', "v_ts_g_btn", "yes");
        echo sprintf('
                        $("#%1$s_in_reso").val(data[0]["input-resolution"]);
                        $("#%1$s_out_reso").val(data[0]["output-resolution"]);
                        var conf = "";
                        if(data[0]["config-file"] != null) {
                            conf = data[0]["config-file"].split("/");
                        }
                        $("#%1$s_conf").val(conf[3]);
                     ', $part);
        re_render();
        echo '
                    }
                });
            }';
    } else {
        re_create_switch('data[0]["gdc/enabled"]', 'v_vr_g_en', 'v_vr_g_en', 'v_vr_g_prop', 'v_vr_g_en', "v_vr_g_btn", "yes");
        echo sprintf('
                $("#v_vr_g_in_reso").val(data[0]["gdc/input-resolution"]);
                $("#v_vr_g_out_reso").val(data[0]["gdc/output-resolution"]);
                var conf = "";
                if(data[0]["gdc/config-file"] != null) {
                    conf = data[0]["gdc/config-file"].split("/");
                }
                $("#%s_conf").val(conf[3]);
                ', $part);
    }

}

function create_ol_wm_text()
{
    echo sprintf('
                            <div class="layui-colla-item">
                                <h2 class="layui-colla-title">%2$s</h2>
                                <div class="layui-colla-content">
                                    <form target="o_wm_txt" class="layui-form layui-form-pane" action="./ipc/ol_wm_txt.php" method="post" enctype="multipart/form-data">
                                        <div class="layui-form-item">
                                            <label class="layui-form-label">%1$s</label>
                                            <div class="layui-input-inline">
                                                <select name="ol_wm_txt_cur_index" lay-filter="ol_wm_txt_cur_index" id="ol_wm_txt_cur_index"></select>
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <label class="layui-form-label">%2$s</label>
                                            <div class="layui-input-inline">
                                                <input type="text" name="o_w_txt_txt" id="ol_wm_txt" class="layui-input" autocomplete="off" value="">
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <label class="layui-form-label" style="width: auto">%3$s</label>
                                            <div class="layui-input-inline">
                                                <input type="text" name="o_w_txt_font_color" id="owtfc-form-input" class="layui-input" autocomplete="off"
                                                       value="">
                                            </div>
                                            <div class="layui-inline" style="left: -11px;">
                                                <div id="owtfc-form"></div>
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <label class="layui-form-label">%4$s</label>
                                            <div class="layui-input-inline">
                                                <select name="o_w_txt_font_file" lay-filter="owm_txt" id="o_txt"></select>
                                            </div>
                                            <div class="layui-input-inline">
                                                <button type="button" class="layui-btn" id="w_txt" name="file" value="watermark/text"><i class="layui-icon">&#xe67c;</i>%5$s</button>
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <label class="layui-form-label" style="margin-right:15px;" >%6$s</label>
                                            <div class="layui-input-inline">
                                                &nbsp
                                                <div class="layui-input-inline">
                                                    <div id="slideWfs" class="demo-slider"></div>
                                                </div>
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <label class="layui-form-label">%7$s</label>
                                            <div class="layui-input-inline">
                                                <input type="text" name="o_w_txt_left" class="layui-input" autocomplete="off" id="ol_wm_txt_l" value="">
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <label class="layui-form-label">%8$s</label>
                                            <div class="layui-input-inline">
                                                <input type="text" name="o_w_txt_top" class="layui-input" autocomplete="off" id="ol_wm_txt_t" value="">
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <div class="layui-input-inline">
                                                <button class="layui-btn" lay-submit="" lay-filter="demo1" id="ol_wm_txt_set">%9$s</button>
                                                <button type="reset" class="layui-btn layui-btn-primary" id="ol_wm_txt_reset">%10$s</button>
                                            </div>
                                        </div>
                                    </form>
                                    <iframe name="o_wm_txt" style="display:none"></iframe>
                                </div>
                            </div>
        ', gettext("index"), gettext("text"), gettext("font color"), gettext("font file"), gettext("upload file"), gettext("font size"), gettext("x"), gettext("y"), gettext("Submit"), gettext("Reset"));
}

function create_ol_wm_image() {
    echo sprintf('
                            <div class="layui-colla-item">
                                <h2 class="layui-colla-title">%2$s</h2>
                                <div class="layui-colla-content">
                                    <form target="o_wm_img" class="layui-form layui-form-pane"
                                          action="./ipc/ol_wm_img.php" method="post" enctype="multipart/form-data">
                                        <div class="layui-form-item">
                                            <label class="layui-form-label">%1$s</label>
                                            <div class="layui-input-inline">
                                                <select name="ol_wm_img_cur_index" lay-filter="ol_wm_img_cur_index" id="ol_wm_img_cur_index"></select>
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <label class="layui-form-label">%3$s</label>
                                            <div class="layui-input-inline">
                                                <select name="o_w_img_file" lay-filter="owm_img" id="o_img"></select>
                                            </div>
                                            <div class="layui-input-inline">
                                                <button type="button" class="layui-btn" id="w_img" name="file"><i
                                                            class="layui-icon">&#xe67c;</i><?php echo gettext("upload image") ?>
                                                </button>
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <label class="layui-form-label">%4$s</label>
                                            <div class="layui-input-inline">
                                                <input type="text" name="o_w_img_height" class="layui-input"
                                                       autocomplete="off" id="ol_wm_img_h" value="">
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <label class="layui-form-label">%5$s</label>
                                            <div class="layui-input-inline">
                                                <input type="text" name="o_w_img_width" class="layui-input"
                                                       autocomplete="off" id="ol_wm_img_w" value="">
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <label class="layui-form-label">%6$s</label>
                                            <div class="layui-input-inline">
                                                <input type="text" name="o_w_img_left" class="layui-input"
                                                       autocomplete="off" id="ol_wm_img_l" value="">
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <label class="layui-form-label">%7$s</label>
                                            <div class="layui-input-inline">
                                                <input type="text" name="o_w_img_top" class="layui-input"
                                                       autocomplete="off" id="ol_wm_img_t" value="">
                                            </div>
                                        </div>
                                        <div class="layui-form-item">
                                            <div class="layui-input-inline">
                                                <button class="layui-btn" lay-submit="" lay-filter="demo1"
                                                        id="ol_wm_img_set">%8$s</button>
                                                <button type="reset"
                                                        class="layui-btn layui-btn-primary" id="ol_wm_img_reset">%9$s</button>
                                            </div>
                                        </div>
                                    </form>
                                    <iframe name="o_wm_img" style="display:none"></iframe>
                                </div>
                            </div>
   ', gettext("index"), gettext("image"), gettext("file"), gettext("height"), gettext("width"), gettext("x"), gettext("y"), gettext("Submit"), gettext("Reset"));
}

?>

