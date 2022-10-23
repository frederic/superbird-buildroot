<?php
require "../../common.php";
require "./convert_color.php";

function ol_wm_set($current_index, $set_para, $file_dir)
{
    $kvs = array();
    foreach ($set_para as $x => $x_value) {
        $k = '/ipc/overlay/watermark/image/' . $current_index . '/' . $x;
        if ($x == "file" && $x_value != "") {
            $kvs[$k] = $file_dir . '/' . $x_value;
        } else {
            $kvs[$k] = $x_value;
        }
    }
    foreach ($kvs as $k => $v) {
        ipc_property('set', $k, $v);
    }
}

$current_index = $_POST["ol_wm_img_cur_index"] - 1;
$img_file = ($_POST["o_w_img_file"] == "disable") ? "" : $_POST["o_w_img_file"];
$img_height = $_POST["o_w_img_height"];
$img_left = $_POST["o_w_img_left"];
$img_top = $_POST["o_w_img_top"];
$img_width = $_POST["o_w_img_width"];
$set_para = array("file" => $img_file, "position/height" => $img_height, "position/left" => $img_left,
    "position/top" => $img_top, "position/width" => $img_width);
$file_dir = $GLOBALS["data_dir"] . "/image";
ol_wm_set($current_index, $set_para, $file_dir);

?>
