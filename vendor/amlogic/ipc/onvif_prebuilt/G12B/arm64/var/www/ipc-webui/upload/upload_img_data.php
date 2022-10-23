<?php
include "../common.php";

$retry_count = 2;

$database_path = get_database_path();
if ($database_path == "") {
    die("can not get the database path");
}

class SQLiteDB extends SQLite3
{
    function __construct($path)
    {
        $this->open($path);
    }
}

$db = new SQLiteDB($database_path);
if (!$db) {
    die($db->lastErrorMsg());
}

$front_item_count = isset($_GET["front_item_count"]) ? $_GET["front_item_count"] : 0;
$sql_item_count = <<<EOF
SELECT COUNT(*) from faceinfo;
EOF;

$upload_img = $_FILES["file"];
$data_out = '{"code":0, "msg":"", "data":{"count":';

if ($upload_img["error"] == 0 && !empty($upload_img)) {
    $query_path = $GLOBALS["data_dir"];
    if (($ret = make_dir($query_path)) == -1) {
        die("make dir error");
    }
    $query_path = $query_path . "/upload_img.jpg";
    if (move_uploaded_file($_FILES["file"]["tmp_name"], $query_path)) {
        do {
            $ret = face_cap_with_img($query_path);
            if ($ret != 0) {
                die("face capture with image error");
            }

            sleep(1);    // 1s

            $current_total_count = $db->querySingle($sql_item_count);
            if ($current_total_count != $front_item_count) {
                break;
            }
        } while ($retry_count-- > 0);

        $data_out = $data_out . $current_total_count;
    } else {
        $data_out = $data_out . '-1';
    }
} else {
    $data_out = $data_out . '-1';
}

$data_out = $data_out . '}}';
ob_clean();
echo $data_out;

?>
