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
$uid_set = '(' . $data . ')';
$res_array = array();
$res_array["res"] = "0";

$sql_userinfo = <<<EOF
DELETE FROM userinfo WHERE `uid` in $uid_set;
EOF;

$sql_faceinfo = <<<EOF
UPDATE faceinfo SET `uid`=0 WHERE `uid` in $uid_set;
EOF;

$result_userinfo = $db->exec($sql_userinfo);
$result_faceinfo = $db->exec($sql_faceinfo);
if (!$result_userinfo || !$result_faceinfo) {
    $res_array["res"] = "-1";
}

ob_clean();
echo json_encode($res_array);
$db->close();

?>
