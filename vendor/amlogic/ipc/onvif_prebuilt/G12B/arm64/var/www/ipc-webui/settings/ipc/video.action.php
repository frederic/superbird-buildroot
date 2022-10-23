<?php
require "../../common.php";

$action = $_POST["action"];
$ch = $_POST["ch"];
$tab = $_POST["tab"];
$index = $_POST["index"];

$parent_key_with_tab = [
    'ts' => [
        'main' => true,
        'sub' => true,
        'gdc' => true,
    ],
    'vr' => [
        'recording' => false,
        'gdc' => true,
    ]
];

$keys = [
    'ts' => [
        'main' => [
            'device' => '/dev/video0',
            'resolution' => '1920x1080',
            'framerate' => '30',
            'codec' => 'h264',
            'gop' => '10',
            'bitrate' => '6000',
            'bitrate-type' => 'CBR',
            'video-quality-level' => '6',
        ],
        'sub' => [
            'enabled' => 'false',
            'device' => '/dev/video0',
            'resolution' => '704x576',
            'framerate' => '30',
            'codec' => 'h264',
            'gop' => '10',
            'bitrate' => '2000',
            'bitrate-type' => 'CBR',
            'video-quality-level' => '6',
        ],
        'sub1' => [
            'enabled' => 'false',
            'device' => '/dev/video0',
            'resolution' => '704x576',
            'framerate' => '30',
            'codec' => 'h264',
            'gop' => '10',
            'bitrate' => '2000',
            'bitrate-type' => 'CBR',
            'video-quality-level' => '6',
        ],
        'gdc' => [
            'enabled' => 'false',
            'config-file' => '',
            'input-resolution' => '',
            'output-resolution' => '',
        ],
    ],
    'vr' => [
        'recording' => [
            'device' => '/dev/video0',
            'resolution' => '1920',
            'framerate' => '1080',
            'codec' => 'h264',
            'gop' => '10',
            'bitrate' => '6000',
            'bitrate-type' => 'CBR',
            'video-quality-level' => '6',
        ],
        'gdc' => [
            'enabled' => 'false',
            'config-file' => '',
            'input-resolution' => '',
            'output-resolution' => '',
        ],
    ]
];

if ($action == 'get') {
    $items = [];
    if ($ch == 'multi-channel') {
        $items[$ch] = ipc_property('get', '/ipc/video/' . $ch, 'false');
        echo json_encode($items);
        return;
    }

    $parent_key = '/ipc/video/' . $ch . '/';
    if ($parent_key_with_tab[$ch][$tab]) {
        if ($tab == 'sub' && $index != 0) {
            $tab .= $index;
        }
        $parent_key .= $tab . '/';
    }

    $items['index'] = range(1, 2);

    foreach ($keys[$ch][$tab] as $k => $v) {
        $items[$k] = ipc_property('get', $parent_key . $k, $v);
    }
    echo json_encode($items);
}

if ($action == 'set') {
    if ($ch == 'multi-channel') {
        $v = $_POST[$ch];
        ipc_property('set', '/ipc/video/' . $ch, $v);
        $ret = ["ret" => 0];
        echo json_encode($ret);
        return;
    }

    $items = [];
    $prefix = $ch . '_' . $tab . '_';
    foreach ($keys[$ch][$tab] as $k => $v) {
        $items[$k] = $_POST[$prefix . $k];
        if ($k == 'config-file') {
            $items[$k] = '/etc/gdc_config/' . $items[$k];
        }
    }

    if (isset($items['enabled'])) {
        $enabled = $items['enabled'];
        if ($enabled == 'true') {
            unset($items['enabled']);
            $items['enabled'] = $enabled;
        }
    }

    $parent_key = '/ipc/video/' . $ch . '/';
    if ($parent_key_with_tab[$ch][$tab]) {
        if ($tab == 'sub' && $index != 0) {
            $tab .= $index;
        }
        $parent_key .= $tab . '/';
    }
    foreach ($items as $k => $v) {
        ipc_property('set', $parent_key . $k, $v);
    }

    $ret = ["ret" => 0];
    echo json_encode($ret);

}
