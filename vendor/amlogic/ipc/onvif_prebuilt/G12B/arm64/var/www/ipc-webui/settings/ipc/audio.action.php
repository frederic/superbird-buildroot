<?php
require "../../common.php";

$action = $_POST["action"];
$tab = $_POST["tab"];
$keys = [
    'audio' => [
        'enabled' => 'false',
        'codec' => 'g711',
        'bitrate' => '24',
        'samplerate' => '11025',
        'channels' => '2',
        'device' => '',
        'device-options' => '',
        'format' => 'S16LE',
    ],
    'backchannel' => [
        'enabled' => 'false',
        'device' => '',
        'encoding' => 'PCMU',
        //'clock-rate' => 'post',
    ]
];

if ($action == 'get') {
    $items = [];
    $parent_key = '/ipc/' . $tab . '/';
    foreach ($keys[$tab] as $k => $t) {
        $items[$k] = ipc_property('get', $parent_key . $k);
    }
    echo json_encode($items);
}

if ($action == 'set') {
    $items = [];

    foreach ($keys[$tab] as $k => $v) {
        $items[$k] = $_POST[str_replace('/', '_', $tab . '_' . $k)];
    }

    $enabled = $items['enabled'];

    if ($enabled == 'true') {
        unset($items['enabled']);
        $items['enabled'] = $enabled;
    }

    $parent_key = '/ipc/' . $tab . '/';
    foreach ($items as $k => $v) {
        ipc_property('set', $parent_key . $k, $v);
    }

    $ret = ["ret" => 0];
    echo json_encode($ret);
}

