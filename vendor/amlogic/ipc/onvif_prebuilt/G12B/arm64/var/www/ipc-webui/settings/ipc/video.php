<?php
require '../../common.php';

function list_video_devices() {
    $ret = [];
    exec("ls /dev/video*", $ret, $rc);
    echo json_encode($ret);
}

function list_gdc_config_files() {
    $ret = [];
    exec("ls /etc/gdc_config/", $ret, $rc);
    echo json_encode($ret);
}

function list_video_framerates() {
    $ret = [24, 25];
    $vsys = ipc_property('get', '/ipc/isp/anti-banding');
    if ($vsys == '60') {
        array_push($ret, '30');
    }
    echo json_encode($ret);
}

function get_isp_resolution() {
    $ret['ret'] = ipc_property('get', '/ipc/isp/sensor/resolution/list');
    echo json_encode($ret);
}

function get_ts_main_video_framerate() {
    $ret['ret'] = ipc_property('get', '/ipc/video/ts/main/framerate');
    echo json_encode($ret);
}

function get_ts_main_video_resolution() {
    $val = ipc_property('get', '/ipc/video/ts/main/resolution');
    $resolution = explode('x', $val);
    $ret['width'] = $resolution[0];
    $ret['height'] = $resolution[1];
    echo json_encode($ret);
}

if (isset($_GET['function'])) {
    $func = $_GET['function'];
    $func();
}
