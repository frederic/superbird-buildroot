<?php
require "../../common.php";

$file = $_FILES['file'];
$dir = $_POST['dir'];

$ret = [
    'code' => 0,
    'msg' => '',
    'data' => [
        'ret' => 0,
    ]
];

if ($file['error'] == 0) {
    $query_path = $GLOBALS['data_dir'] . '/' . $dir;
    if (!file_exists($query_path)) {
        if(false == mkdir($query_path, 0755, true)) {
            die('make dir error');
        }
    }
    $query_path .= '/' . $file['name'];
    if (move_uploaded_file($file['tmp_name'], $query_path)) {
        $rc = 0;
    } else {
        $rc = 1;
    }
} else {
    $rc = 2;
}

$ret["data"]["ret"] = $rc;

echo json_encode($ret);