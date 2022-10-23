<?php
require "../../common.php";

$wm_img = $_FILES["file"];
$data_out = '{"code": 0, "msg": "", "data": {"count":';
if ($wm_img["error"] == 0) {
    $query_path = $query_path = $GLOBALS["data_dir"] . "/image";
    if (($ret = make_dir($query_path)) == -1) {
        die("make dir error");
    }
    $query_path = $query_path . "/" . $_FILES["file"]["name"];
    if (move_uploaded_file($_FILES["file"]["tmp_name"], $query_path)) {
        $data_out = $data_out . '1';
    } else {
        $data_out = $data_out . '-1';
    }
} else {
    $data_out = $data_out . '0';
}
$data_out = $data_out . '}}';
echo $data_out;

