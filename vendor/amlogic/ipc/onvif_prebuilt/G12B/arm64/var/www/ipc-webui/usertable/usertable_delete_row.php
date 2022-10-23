<?php
include "../common.php";

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
$uid = $_POST["uid"];

$sql_userinfo = <<<EOF
DELETE FROM userinfo WHERE `uid`=$uid;
EOF;

$sql_faceinfo = <<<EOF
UPDATE faceinfo SET `uid`=0 WHERE `uid`=$uid;
EOF;

$result_userinfo = $db->exec($sql_userinfo);
$result_faceinfo = $db->exec($sql_faceinfo);
if ($result_userinfo && $result_faceinfo) {
    echo "1";
} else {
    echo "0";
}
$db->close();

?>
