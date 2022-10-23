<?php
require "../../common.php";

function recording_set($chunk_duration, $enabled, $location, $size)
{
    $kvs = array('chunk-duration' => $chunk_duration, 'location' => $location, 'reserved-space-size' => $size);
    if ($enabled == "false") {
        $kvs = array('enabled' => $enabled) + $kvs;
    } else {
        $kvs['enabled'] = $enabled;
    }

    $vkey = '/ipc/recording/';

    foreach ($kvs as $k => $v) {
        ipc_property('set', $vkey . $k, $v);
    }
}

$chunk_duration = $_POST["s_chunk_duration"];
$enabled = $_COOKIE["s_en"];
$location = $_POST["s_location"];
$size = $_POST["s_size"];

recording_set($chunk_duration, $enabled, $location, $size);
?>
