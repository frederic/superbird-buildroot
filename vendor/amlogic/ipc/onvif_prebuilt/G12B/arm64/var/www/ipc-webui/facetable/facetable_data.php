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
SELECT COUNT(*) from faceinfo;
EOF;
$result_item_count = $db->querySingle($sql_item_count);

$page = isset($_GET["page"]) ? $_GET["page"] : 1;
$limit = isset($_GET["limit"]) ? $_GET["limit"] : 10;

$sql_userinfo = <<<EOF
SELECT * FROM userinfo;
EOF;
$sql_faceinfo = "SELECT * FROM faceinfo ORDER BY `index` DESC limit " . ($page - 1) * $limit . "," . $limit;
$result_userinfo = $db->query($sql_userinfo);
if (!$result_userinfo) {
    die($db->lastErrorMsg());
}
$result_faceinfo = $db->query($sql_faceinfo);
if (!$result_faceinfo) {
    die($db->lastErrorMsg());
}

$i = 0;
$length_userinfo = 0;
$data_userinfo = array();
$data_faceinfo = array();

while ($row_userinfo = $result_userinfo->fetchArray(SQLITE3_ASSOC)) {
    $data_userinfo[$length_userinfo]["uid"] = $row_userinfo["uid"];
    $data_userinfo[$length_userinfo]["name"] = $row_userinfo["name"];
    $length_userinfo++;
}

$data_out = '{"code":0, "msg":"", "count":';
$data_out = $data_out . $result_item_count . ', "data":[';
while ($row_faceinfo = $result_faceinfo->fetchArray(SQLITE3_ASSOC)) {
    $data_faceinfo["index"] = $row_faceinfo["index"];
    $data_faceinfo["uid"] = $row_faceinfo["uid"];

    for ($i = 0; $i < $length_userinfo; $i++) {
        if ($row_faceinfo["uid"] == $data_userinfo[$i]["uid"]) {
            $data_faceinfo["name"] = $data_userinfo[$i]["name"];
            break;
        }
    }
    if ($i >= $length_userinfo) {
        $data_faceinfo["name"] = "unknow";
    }

    $face_image = base64_encode($row_faceinfo["faceimg"]);
    $data_faceinfo["faceimg"] = $face_image;

    $data_faceinfo["limit"] = $limit;

    $data_out = $data_out . json_encode($data_faceinfo) . ",";
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

