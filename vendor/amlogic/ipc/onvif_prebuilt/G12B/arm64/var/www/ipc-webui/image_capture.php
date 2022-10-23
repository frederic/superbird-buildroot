<?php
include "common.php";

function get_capture_image_location()
{
    return ipc_property("get", "/ipc/image-capture/location");
}


function set_capture_image_location($path)
{
    return ipc_property("set", "/ipc/image-capture/location", $path);
}

function dump_image_enable()
{
    return ipc_property("set", "/ipc/image-capture/trigger", "true");
}

function get_dump_image_path()
{
    return ipc_property("get", "/ipc/image-capture/imagefile");
}

$retry_count = 5;
$ret = ['status' => 'error'];
$dir = $GLOBALS['download_dir'];

if(!file_exists($dir)) {
    mkdir($dir, 0700, true);
}
chmod($dir, 02710);

// set to download location
$capture_image_location = get_capture_image_location();
if ($capture_image_location != $dir) {
    set_capture_image_location($dir);
}

dump_image_enable();

do {
    sleep(1);    // 1s
    $dump_image_path = get_dump_image_path();
    if ($dump_image_path != "") {
        sleep(1);
        $fn = basename($dump_image_path);
        $ret['status'] = 'success';
        $ret['filename'] = $fn;
        break;
    }
} while ($retry_count-- > 0);

echo json_encode($ret);
