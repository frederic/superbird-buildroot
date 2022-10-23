<?php
require "./convert_color.php";
require "../../common.php";

function ol_time_set($part, $file_dir)
{
    $enabled = $_COOKIE["ol_" . $part];
    $pos = $_POST["ol_pos_" . $part];
    $font_bg = $_POST["ol_font_bgc_" . $part];
    $font_c = $_POST["ol_font_color_" . $part];
    $font_file = $_POST["ol_font_file_" . $part];
    $font_size = $_COOKIE[$part . "/font/size"];

    if ($font_file == "" || $font_file == 'decker.ttf') {
        $font_file = "/usr/share/directfb-1.7.7/decker.ttf";
    } else {
        $font_file = $file_dir . '/' . $font_file;
    }

    $kvs = array("position" => $pos, "font/background-color" => $font_bg,
        "font/color" => $font_c, "font/font-file" => $font_file, "font/size" => $font_size);

    if ($enabled == "false") {
        $kvs = array('enabled' => $enabled) + $kvs;
    } else {
        $kvs['enabled'] = $enabled;
    }

    $vkey = '/ipc/overlay/' . $part . '/';

    foreach ($kvs as $k => $v) {
        ipc_property('set', $vkey . $k, $v);
    }
}

$p_id = $_REQUEST["p_id"];
$file_dir = $GLOBALS["data_dir"] . "/ttf";
if ($p_id == "dt") {
    ol_time_set("datetime", $file_dir);
} elseif ($p_id == "pts") {
    ol_time_set("pts", $file_dir);
}


?>
