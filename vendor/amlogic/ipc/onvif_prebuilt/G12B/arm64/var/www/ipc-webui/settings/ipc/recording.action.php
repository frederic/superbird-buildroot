<?php
require "../../common.php";

$action = $_POST["action"];
$parent_key = '/ipc/recording/';
$keys = [
    "chunk-duration" => "5",
    "enabled" => "false",
    "location" => "",
    "reserved-space-size" => "200"
];

if ($action == 'get') {
    $items = [];
    foreach ($keys as $k => $v) {
        $items[$k] = ipc_property('get', $parent_key . $k, $v);
    }
    echo json_encode($items);
}

if ($action == 'set') {
    $items = [];

    foreach ($keys as $k => $t) {
        $items[$k] = $_POST[str_replace('/', '_', $k)];
    }

    $enabled = $items['enabled'];

    if ($enabled == 'true') {
        unset($items['enabled']);
        $items['enabled'] = $enabled;
    }

    foreach ($items as $k => $v) {
        ipc_property('set', $parent_key . $k, $v);
    }

    $ret = ["ret" => 0];
    echo json_encode($ret);
}

