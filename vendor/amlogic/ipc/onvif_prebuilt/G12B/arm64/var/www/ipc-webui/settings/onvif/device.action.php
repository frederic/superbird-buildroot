<?php
require "../../common.php";

$action = $_POST["action"];
$keys = [
    "firmware-ver" => "",
    "hardware-id" => "",
    "manufacturer" => "",
    "model" => "",
    "serial-number" => "",
];

$parent_key = '/onvif/device/';

if ($action == 'get') {
    $items = [];
    foreach ($keys as $k => $v) {
        $items[$k] = ipc_property('get', $parent_key . $k, $v);
    }
    echo json_encode($items);
}

if ($action == 'set') {
    foreach ($keys as $k => $v) {
        ipc_property('set', $parent_key . $k, $_POST[$k]);
    }
    $ret = ["ret" => 0];
    echo json_encode($ret);
}

