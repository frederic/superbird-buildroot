<?php
require "../../common.php";

function audio_set($bitrate, $codec, $device, $devopt, $enabled, $samprate)
{
    $kvs = array('bitrate' => $bitrate, 'codec' => $codec, 'device' => $device, 'device-options' => $devopt, 'samplerate' => $samprate);
    if ($enabled == "false") {
        $kvs = array('enabled' => $enabled) + $kvs;
    } else {
        $kvs['enabled'] = $enabled;
    }

    $vkey = '/ipc/audio/';

    foreach ($kvs as $k => $v) {
        ipc_property('set', $vkey . $k, $v);
    }
}

$bitrate = $_POST["a_bitrate"];
$codec = $_POST["a_codec"];
$device = $_POST["a_device"];
$devopt = $_POST["a_device_opt"];
$enabled = $_COOKIE["a_en"];
$samprate = $_POST["a_samplerate"];

audio_set($bitrate, $codec, $device, $devopt, $enabled, $samprate);
?>
