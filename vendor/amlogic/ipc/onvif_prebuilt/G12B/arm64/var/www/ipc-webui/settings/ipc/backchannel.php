<?php
require "../../common.php";

function backchannel_set($device, $enabled)
{
    $kvs = array('device' => $device);
    if ($enabled == "false") {
        $kvs = array('enabled' => $enabled) + $kvs;
    } else {
        $kvs['enabled'] = $enabled;
    }

    $vkey = '/ipc/backchannel/';

    foreach ($kvs as $k => $v) {
        ipc_property('set', $vkey . $k, $v);
    }
}

$device = $_POST["b_device"];
$enabled = $_COOKIE["b_en"];

backchannel_set($device, $enabled);

?>
