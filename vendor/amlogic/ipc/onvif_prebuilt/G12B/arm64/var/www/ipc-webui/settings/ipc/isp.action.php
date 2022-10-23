<?php
require "../../common.php";

$action = $_POST["action"];

$parent_key = '/ipc/isp/';
$keys = [
    "brightness" => "128",
    "contrast" => "128",
    "exposure_absolute" => "1",
    "exposure_auto" => "true",
    "hue" => "180",
    "saturation" => "128",
    "sharpness" => "128",
    "wdr_enabled" => "false",
    "whitebalance_auto" => "true",
    "whitebalance_cbgain" => "128",
    "whitebalance_crgain" => "128",
    "whitebalance_cbgain_default" => "128",
    "whitebalance_crgain_default" => "128",
    "anti-banding" => "60",
    "mirroring" => "NONE",
    "compensation_action" => "disable",
    "compensation_hlc" => "0",
    "compensation_blc" => "0",
    "rotation" => "0",
    "nr_action" => "disable",
    "nr_space-normal" => "0",
    "nr_space-expert" => "0",
    "nr_time" => "0",
    "gain_auto" => "true",
    "gain_manual" => "0",
    "defog_action" => "disable",
    "defog_auto" => "0",
    "defog_manual" => "0"
];

function check_and_reset_video_framerate()
{
    $channels = array("ts/main", "ts/sub", "vr");
    foreach ($channels as $ch) {
        $k = '/ipc/video/' . $ch . '/framerate';
        $fr = ipc_property('get', $k);
        if ($fr == "30") {
            ipc_property('set', $k, '25');
        }
    }
}

if ($action == 'get') {
    $items = [];
    foreach ($keys as $k => $v) {
        $items[$k] = ipc_property('get', $parent_key . str_replace('_', '/', $k), $v);
    }
    echo json_encode($items);
}

if ($action == 'set') {
    foreach ($keys as $k => $d) {
        if (strpos($k, 'default') !== false) continue;
        $v = $_POST[$k];
        if ($k == "anti-banding" && $v == "50") {
            check_and_reset_video_framerate();
        }
        ipc_property('set', $parent_key . str_replace('_', '/', $k), $v);
    }

    $ret = ["ret" => 0];
    echo json_encode($ret);
}
