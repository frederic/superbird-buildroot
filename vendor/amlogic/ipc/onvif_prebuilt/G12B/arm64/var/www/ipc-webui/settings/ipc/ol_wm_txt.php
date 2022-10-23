<?php
require "./convert_color.php";
require "../../common.php";

function ol_wm_txt_set($current_index, $set_para, $file_dir)
{
    $kvs = array();
    foreach ($set_para as $x => $x_value) {
        $k = '/ipc/overlay/watermark/text/' . $current_index . '/' . $x;
        if ($x == "font/font-file") {
            if ($x_value == "" || $x_value == "decker.ttf") {
                $x_value = '/usr/share/directfb-1.7.7/decker.ttf';
            } else {
                $x_value = $file_dir . '/' . $x_value;
            }
        }
        $kvs[$k] = $x_value;
    }
    foreach ($kvs as $k => $v) {
        ipc_property('set', $k, $v);
    }
}

$current_index = $_POST["ol_wm_txt_cur_index"] - 1;
$txt_f_c = $_POST["o_w_txt_font_color"];
$txt_f_file = $_POST["o_w_txt_font_file"];
$txt_f_size = $_COOKIE["watermark/text/font/size"];
$txt_left = $_POST["o_w_txt_left"];
$txt_top = $_POST["o_w_txt_top"];
$txt = $_POST["o_w_txt_txt"];
$set_para = array("font/color" => $txt_f_c, "font/font-file" => $txt_f_file,
    "font/size" => $txt_f_size, "position/left" => $txt_left, "position/top" => $txt_top, "text" => $txt);
$file_dir = $GLOBALS["data_dir"] . "/ttf";

ol_wm_txt_set($current_index, $set_para, $file_dir);

?>



