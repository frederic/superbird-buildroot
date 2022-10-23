<?php
require "../../common.php";

function video_set($part, $part_id, $bit_id, $gop_id)
{
    $bitrate = $_COOKIE[$bit_id];
    $codec = $_POST[$part_id . "_codec"];
    $device = $_POST[$part_id . "_device"];
    $framerate = $_POST[$part_id . "_framerate"];
    $gop = $_COOKIE[$gop_id];
    $reso = $_POST[$part_id . "_reso"];
    $kvs = array('bitrate' => $bitrate, 'codec' => $codec, 'device' => $device, 'framerate' => $framerate, 'gop' => $gop, 'resolution' => $reso);
    if ($part == "ts/sub") {
        $enabled = $_COOKIE["v_ts_s_en"];
        if ($enabled == "false") {
            $kvs = array('enabled' => $enabled) + $kvs;
        } else {
            $kvs['enabled'] = $enabled;
        }
    }
    $vkey = '/ipc/video/' . $part . '/';
    foreach ($kvs as $k => $v) {
        ipc_property('set', $vkey . $k, $v);
    }
}

function gdc_set($part, $part_id)
{
    $config_file = $_POST[$part_id . "_config_file"];
    $enabled = $_COOKIE[$part_id . "_en"];
    $in_reso = $_POST[$part_id . "_in_reso"];
    $out_reso = $_POST[$part_id . "_out_reso"];
    $kvs = array();
    if ($enabled == "false") {
        $kvs['enabled'] = $enabled;
    } else {
        $kvs['config-file'] = '/etc/gdc_config/' . $config_file;
        $kvs['input-resolution'] = $in_reso;
        $kvs['output-resolution'] = $out_reso;
        $kvs['enabled'] = $enabled;
    }
    $vkey = '/ipc/video/' . $part . '/gdc/';
    foreach ($kvs as $k => $v) {
        ipc_property('set', $vkey . $k, $v);
    }
}

$p_id = $_REQUEST["p_id"];

if ($p_id == "mu_en") {
    $mu_en = $_COOKIE["v_mu_en"];
    ipc_property('set', '/ipc/video/multi-channel', $mu_en);
} elseif ($p_id == "sub") {
    video_set("ts/sub", "v_ts_s", "ts_sb", "ts_sg");
} elseif ($p_id == "ts_gdc") {
    gdc_set("ts", "v_ts_g");
} elseif ($p_id == "vr") {
    video_set("vr", "v_vr", "vr_b", "vr_g");
    gdc_set("vr", "v_vr_g");
}

$id = $_POST["id"];
if ($id == "main") {
    if ($_POST["v_ts_m_reso"] == "3840x2160") {
        ipc_property('set', '/ipc/video/multi-channel', 'false');
    }
    video_set("ts/main", "v_ts_m", "ts_mb", "ts_mg");
}

$main_reso = ipc_property('get', '/ipc/video/ts/main/resolution');
$multi_en = ipc_property('get', '/ipc/video/multi-channel');
$val = array($main_reso, $multi_en);
echo json_encode($val);

?>
