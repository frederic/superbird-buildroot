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
$sql = <<<EOF
SELECT * FROM userinfo;
EOF;
$result = $db->query($sql);
if (!$result) {
    die($db->lastErrorMsg());
}

$data = array();

while ($row = $result->fetchArray(SQLITE3_ASSOC)) {
    array_push($data, $row);
}

ob_clean();
echo json_encode($data);
?>
