<?php

require "../../common.php";
$parent_key = '/ipc/overlay/shielding';
$keys = [
    "left" => '0',
    "top" => '0',
    "width" => '0',
    "height" => '0',
];

$action = $_POST["action"];

if ($action == 'get') {
    $items = [];
    $size = ipc_property('arrsz', $parent_key);
    for ($i = 0; $i < $size; $i++) {
        foreach ($keys as $k => $v) {
            $items[$i][$k] = ipc_property('get', $parent_key . '/' . $i . '/position/' . $k, $v);
        }
    }
    echo json_encode($items);
}

if ($action == 'set') {
    $size = $_POST['size'];
    $rects = $_POST['rects'];
    // clean up rects first
    ipc_property('del', $parent_key);
    for ($i = 0; $i < $size; $i++) {
        foreach ($keys as $k => $v) {
            $v = $rects[$i][$k];
            ipc_property('set', $parent_key . '/' . $i . '/position/' . $k, $v);
        }
    }

    $ret = ["ret" => 0];
    echo json_encode($ret);
}

