<?php
require "../../common.php";

$action = $_POST["action"];
$tab = $_POST["tab"];

$keys = [
    'datetime' => [
        'enabled' => 'true',
        'font' => [
            'font-file' => 'decker.ttf',
            'size' => '48',
            'color' => '0xffffffff',
            'background-color' => '0x00000000',
        ],
        'position' => 'top-right',
    ],
    'watermark_text' => [
        'font' => [
            'font-file' => 'decker.ttf',
            'size' => '30',
            'color' => '0xffffffff',
        ],
        'position' => [
            'left' => '0',
            'top' => '0',
        ],
        'text' => '',
    ],
    'watermark_image' => [
        'position' => [
            'left' => '0',
            'top' => '0',
            'width' => '0',
            'height' => '0',
        ],
        'file' => 'Disabled',
    ],
    'nn' => [
        'show' => 'true',
        'font' => [
            'font-file' => 'decker.ttf',
            'size' => '24',
            'color' => '0xffff00ff',
        ],
        'rect-color' => '0xf0f0f0ff',
    ],
];

$watermark_max_item_default = [
    'text' => 5,
    'image' => 2,
];

$parent_key = '/ipc/overlay/';

function process_keys_recursive(array &$array, $prefix_prop, callable $callback, &$userdata, $parent = null) {
    foreach($array as $k => $v) {
        if (is_array($v)) {
            if (empty($parent)) {
                $p = $k . '_';
            } else {
                $p = $parent . $k . '_';
            }
            process_keys_recursive($v, $prefix_prop, $callback, $userdata, $p);
        } else {
            $callback ($parent . $k, $prefix_prop, $v, $userdata);
        }
    }
}

function get_props($k, $prop, $val, &$userdata) {
    $userdata[$k] = ipc_property('get', $prop . str_replace('_', '/', $k), $val);
    if(strpos($k, 'file') !== false) {
        $userdata[$k] = basename($userdata[$k]);
    }
    if(strpos($k, 'text') !== false) {
        $userdata[$k] = substr($userdata[$k], 0, 80*3);
    }
}

function set_props($k, $prop, $defval, &$userdata) {
    global $tab;
    $v = $_POST[$tab . '_' . $k];
    if (strpos($k,'font-file')) {
        if ($v == 'decker.ttf') {
            $font_dir = '/usr/share/directfb-1.7.7/';
        } else {
            $font_dir = $GLOBALS["data_dir"] . '/font/';
        }
        $v = $font_dir . $v;
    } else if ($tab == 'watermark_image' && $k == 'file') {
        if ($v == 'Disabled') {
            $v = '';
        } else {
            $v = $GLOBALS['data_dir'] . '/image/' . $v;
        }
    }

    ipc_property('set', $prop . str_replace('_', '/', $k), $v);
}

if ($action == 'get') {
    $items = [];
    $prefix = $parent_key . str_replace('_', '/', $tab) . '/';
    if (startsWith($tab, 'watermark')) {
        $watermark_type = str_replace('watermark_','', $tab);
        $max_item = ipc_property('get', $parent_key . 'max-' . $watermark_type . '-item');
        if ($max_item == '') {
            $max_item = $watermark_max_item_default[$watermark_type];
        }
        $items['index'] = range(1, $max_item);
        $prefix .= $_POST['index'] . '/';
    }

    process_keys_recursive($keys[$tab], $prefix, 'get_props', $items);

    echo json_encode($items);
}

if ($action == 'set') {
    $items = [];
    $prefix = str_replace('_', '/', $parent_key . $tab . '/');
    if (startsWith($tab, 'watermark')) {
        $prefix .= $_POST['index'] . '/';
    }

    process_keys_recursive($keys[$tab], $prefix, 'set_props', $items);

    $ret = ["ret" => 0];
    echo json_encode($ret);

}

if ($action == 'file') {
    $files = [];

    exec("ls " . $GLOBALS["data_dir"] . "/image", $files, $rc);
    array_push($files, "Disabled");
    $ret['image'] = $files;

    unset($files);
    exec("ls " . $GLOBALS["data_dir"] . "/font", $files, $rc);
    exec("ls /usr/share/directfb-1.7.7/*.ttf", $def_font, $rc);
    if (count($def_font) > 0) {
        array_push($files, basename($def_font[0]));
    }
    $ret['font'] = $files;

    echo json_encode($ret);
}
