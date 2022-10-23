<?php
require "../../common.php";

function framerate_reset()
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

function isp_set($isp_vals)
{
    foreach ($isp_vals as $item => $val) {
        if ($item == "anti-banding" && $val == "50") {
            framerate_reset();
        }
        ipc_property('set', '/ipc/isp/' . $item, $isp_vals[$item]);
    }
}

$isp_vals = array();
$isp_vals["brightness"] = $_COOKIE['brightness'];
$isp_vals["contrast"] = $_COOKIE['contrast'];
$isp_vals["exposure/auto"] = $_COOKIE['i_exp'];
if ($isp_vals["exposure/auto"] == "true") {
    unset($isp_vals["exposure/absolute"]);
} else {
    $isp_vals["exposure/absolute"] = $_COOKIE["exab"];
}
$isp_vals["hue"] = $_COOKIE['hue'];
$isp_vals["ircut"] = $_COOKIE['i_ircut'];
$isp_vals["saturation"] = $_COOKIE['saturation'];
$isp_vals["sharpness"] = $_COOKIE['sharpness'];
$isp_vals["wdr/enabled"] = $_COOKIE['i_wdr'];
$isp_vals["whitebalance/auto"] = $_COOKIE['i_whau'];
$isp_vals["whitebalance/cbgain"] = $_COOKIE['whitebalance/cbgain'];
$isp_vals["whitebalance/crgain"] = $_COOKIE['whitebalance/crgain'];
$isp_vals["anti-banding"] = $_POST["isp_video_mode"];
$isp_vals["mirroring"] = $_POST["isp_mirror"];
$isp_vals["rotation"] = $_POST["isp_rotation"];
$isp_vals["compensation/action"] = $_POST["isp_lc"];
if ($isp_vals["compensation/action"] != "disable") {
    $isp_vals["compensation/".$isp_vals["compensation/action"]] = $_COOKIE["compensation"];
}
$isp_vals["nr/action"] = $_POST["isp_nr"];
if ($isp_vals["nr/action"] == "normal") {
    $isp_vals["nr/space-normal"] = $_COOKIE["nr/space"];
} elseif ($isp_vals["nr/action"] == "expert") {
    $isp_vals["nr/space-expert"] = $_COOKIE["nr/space"];
    $isp_vals["nr/time"] = $_COOKIE["nr/time"];
}

isp_set($isp_vals);
?>
