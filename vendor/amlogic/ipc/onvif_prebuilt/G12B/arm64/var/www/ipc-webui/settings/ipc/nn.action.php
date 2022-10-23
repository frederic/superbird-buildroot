<?php
require "../../common.php";

$action = $_POST["action"];
$parent_key = '/ipc/nn/';
$keys = [
    'enabled' => 'true',
    'detection_model' => 'disable',
    'detection_limits' => '10',
    'recognition_model' => 'disable',
    'recognition_info-string' => 'name',
    'recognition_threshold' => '1',
];

if ($action == 'get') {
    $items = [];
    foreach ($keys as $k => $v) {
        $items[$k] = ipc_property('get', $parent_key . str_replace('_', '/', $k), $v);
    }
    echo json_encode($items);
}

if ($action == 'set') {
    $items = [];

    foreach ($keys as $k => $t) {
        $items[$k] = $_POST[$k];
    }

    $enabled = $items['enabled'];

    if ($enabled == 'true') {
        unset($items['enabled']);
        $items['enabled'] = $enabled;
    }

    foreach ($items as $k => $v) {
        ipc_property('set', $parent_key . str_replace('_', '/', $k), $v);
    }

    $ret = ["ret" => 0];
    echo json_encode($ret);
}

