<?php
require "../../common.php";

$action = $_POST["action"];

$parent_key = '/ipc/day-night-switch/';
$keys = [
    "mode" => "day",
    "delay" => "5",
    "schedule_begin" => "06:00:00",
    "schedule_end" => "18:00:00"
];

if ($action == 'get') {
    $items = [];
    foreach ($keys as $k => $v) {
        $items[$k] = ipc_property('get', $parent_key . str_replace('_', '/', $k), $v);
    }
    echo json_encode($items);
}

if ($action == 'set') {
    foreach ($keys as $k => $d) {
        $v = $_POST[$k];
        ipc_property('set', $parent_key . str_replace('_', '/', $k), $v);
    }

    $ret = ["ret" => 0];
    echo json_encode($ret);
}
