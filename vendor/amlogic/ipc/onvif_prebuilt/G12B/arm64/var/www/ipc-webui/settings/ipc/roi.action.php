<?php

require "../../common.php";
$parent_key = '/ipc/video/roi/sections';
$keys = [
    "quality" => '0',
    "position" => [
        "left" => '0',
        "top" => '0',
        "width" => '0',
        "height" => '0',
    ],
];

$action = $_POST["action"];

if ($action == 'get') {
    $items = [];
    $size = ipc_property('arrsz', $parent_key);
    for ($i = 0; $i < $size; $i++) {
        foreach ($keys as $k => $v) {
            if ($k == 'position') {
                foreach ($v as $kk => $vv) {
                    $items[$i][$k][$kk] = ipc_property('get',
                        $parent_key . '/' . $i . '/' . $k . '/' . $kk, $vv);
                }
            } else {
                $items[$i][$k] = ipc_property('get', $parent_key . '/' . $i . '/' . $k, $v);
            }
        }
    }
    echo json_encode($items);
}

if ($action == 'set') {
    $size = $_POST['size'];
    $sections = $_POST['sections'];
    // clean up rects first
    ipc_property('del', $parent_key);
    for ($i = 0; $i < $size; $i++) {
        foreach ($keys as $k => $v) {
            if ($k == 'position') {
                foreach ($v as $kk => $vv) {
                    $vv = $sections[$i][$k][$kk];
                    ipc_property('set',
                        $parent_key . '/' . $i . '/' . $k . '/' . $kk, $vv);
                }
            } else {
                $v = $sections[$i][$k];
                ipc_property('set', $parent_key . '/' . $i . '/' . $k, $v);
            }
        }
    }

    $ret = ["ret" => 0];
    echo json_encode($ret);
}

