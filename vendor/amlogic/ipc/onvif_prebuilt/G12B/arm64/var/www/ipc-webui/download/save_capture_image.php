<?php
include "../common.php";

$retry_count = 5;

// 检查 image location 是否为空，如果为空，设置为/tmp
$capture_image_location = get_capture_image_location();
if ($capture_image_location == "") {
    $ret = set_capture_image_location("/tmp");
    if ($ret != 0) {
        die("set capture image location error");
    }
}

do {
    $ret = dump_image_enable();
    if ($ret != 0) {
        die("enable image dump error");
    }

    sleep(1);    // 1s

    $dump_image_path = get_dump_image_path();
    if ($dump_image_path != "") {
        sleep(1);
        download_file($dump_image_path);
        break;
    }
} while ($retry_count-- > 0);

?>
