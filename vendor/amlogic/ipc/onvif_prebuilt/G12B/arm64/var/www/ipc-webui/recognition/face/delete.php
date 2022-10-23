<?php
include "../../common.php";

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

// 从web前端接收到的参数
$data = $_POST["data"];
$index_set = '(' . $data . ')';
$res_array = array();
$res_array["res"] = "0";

$sql = <<<EOF
DELETE FROM faceinfo WHERE `index` in $index_set;
EOF;

$result = $db->exec($sql);
if (!$result) {
    $res_array["res"] = "-1";
}

ob_clean();
echo json_encode($res_array);
$db->close();

?>
