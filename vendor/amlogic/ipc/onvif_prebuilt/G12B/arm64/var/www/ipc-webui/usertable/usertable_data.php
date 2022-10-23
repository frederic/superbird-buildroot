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

$sql_item_count = <<<EOF
SELECT COUNT(*) from userinfo;
EOF;
$result_item_count = $db->querySingle($sql_item_count);

$page = isset($_GET["page"]) ? $_GET["page"] : 1;
$limit = isset($_GET["limit"]) ? $_GET["limit"] : 10;

$sql_userinfo = "SELECT * FROM userinfo limit " . ($page - 1) * $limit . "," . $limit;
$result_userinfo = $db->query($sql_userinfo);
if (!$result_userinfo) {
    die($db->lastErrorMsg());
}

$data_userinfo = array();

$data_out = '{"code":0, "msg":"", "count":';
$data_out = $data_out . $result_item_count . ', "data":[';


while ($row_userinfo = $result_userinfo->fetchArray(SQLITE3_ASSOC)) {
    $data_userinfo["uid"] = $row_userinfo["uid"];
    $data_userinfo["name"] = $row_userinfo["name"];

    $data_userinfo["limit"] = $limit;

    $data_out = $data_out . json_encode($data_userinfo) . ",";
}

if ($result_item_count != 0) {
    $data_out = substr($data_out, 0, -1) . "]}";
} else {
    $data_out = substr($data_out, 0, -1) . "[]}";
}
ob_clean();
echo $data_out;


$db->close();
?>

