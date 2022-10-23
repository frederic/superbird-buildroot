<?php
require "../../common.php";
require "./convert_color.php";

function ol_nn_set($set_para, $file_dir)
{
    $kvs = array();

    foreach ($set_para as $x => $x_value) {
        if ($x == "font/font-file") {
            if ($x_value == "" || $x_value == "decker.ttf") {
                $x_value = "/usr/share/directfb-1.7.7/decker.ttf";
            } else {
                $x_value = $file_dir . '/' . $x_value;
            }
        }
        $kvs[$x] = $x_value;
    }

    $vkey = '/ipc/overlay/nn/';

    foreach ($kvs as $k => $v) {
        ipc_property('set', $vkey . $k, $v);
    }
}

$det_show = $_COOKIE["ol_nn_en"];
$rect_color = $_POST["ol_nn_rect_color"];
$font_color = $_POST["ol_nn_font_color"];
$face_fs = $_COOKIE["font/size"];
$face_ff = $_POST["ol_nn_font_file"];

if ($det_show == "false") {
    $set_para = array("show" => $det_show, "rect-color" => $rect_color, "font/color" => $font_color,
        "font/size" => $face_fs, "font/font-file" => $face_ff);
} else {
    $set_para = array("rect-color" => $rect_color, "font/color" => $font_color,
        "font/size" => $face_fs, "font/font-file" => $face_ff, "show" => $det_show);
}
$file_dir = $GLOBALS["data_dir"] . "/ttf";
ol_nn_set($set_para, $file_dir);
?>
