<?php
require "../../common.php";
$parent_key = '/ipc/';
$keys = [
    'image-capture_quality' => '80',
    'image-capture_use-hw-encoder' => 'false',
];

$action = $_POST["action"];

if ($action == 'get') {
    $items = [];
    foreach ($keys as $k => $t) {
        $items[$k] = ipc_property('get', $parent_key . str_replace('_', '/', $k));
    }
    echo json_encode($items);
}

if ($action == 'set') {
    foreach ($keys as $k => $t) {
        $v = $_POST[$k];
        ipc_property('set', $parent_key . str_replace('_', '/', $k), $v);
    }

    $ret = ["ret" => 0];
    echo json_encode($ret);
}

