<?php
require "../../common.php";

$action = $_POST["action"];
$parent_key = '/onvif/discover/';
$keys = [
    'enabled' => 'true',
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
        $items[$k] = $_POST[$k];
    }

    foreach ($items as $k => $v) {
        ipc_property('set', $parent_key . str_replace('_', '/', $k), $v);
    }

    $ret = ["ret" => 0];
    echo json_encode($ret);
}
