<?php
require 'common.php';

function get_web_stream_port() {
    $ret['ret'] = ipc_property('get', '/ipc/video/ts/web/port', '8082');
    echo json_encode($ret);
}

if (isset($_GET['function'])) {
    $func = $_GET['function'];
    $func();
}
