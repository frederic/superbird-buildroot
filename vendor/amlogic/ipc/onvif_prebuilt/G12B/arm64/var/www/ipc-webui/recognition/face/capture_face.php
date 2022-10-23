<?php
include "../../common.php";

function face_cap_enable()
{
    return ipc_property("set", "/ipc/nn/store-face", "true");
}

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

$res_array = array();
$res_array["count"] = 0;

/*
 * 此处调用外部命令: ipc-property set "/ipc/nn/store-face" true
 * 抓取人脸
 */
do {
    $ret = face_cap_enable();
    if ($ret != 0) {
        die("face capture enable error");
    }

    usleep(1000000);    // 1s

    $current_total_count = $db->querySingle($sql_item_count);
    if ($current_total_count != $front_item_count) {
        break;
    }
} while ($retry_count-- > 0);

$res_array["count"] = $current_total_count;
$res_array["front_item_count"] = $front_item_count;

ob_clean();
echo json_encode($res_array);
$db->close();

?>
