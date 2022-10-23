<?php
include "common.php";

$database_path = get_database_path();
if ($database_path == "") {
    create_database();
} else {
    if (file_exists($database_path)) {
        $copy_path = $GLOBALS["data_dir"] . "/face.db";
        if ($database_path != $copy_path) {
            $command_cp = "cp " . $database_path . " " . $copy_path;
            exec($command_cp, $res, $rc);
            if ($rc != 0) {
                die("copy database error");
            }
            $ret = set_database_path($copy_path);
            if ($ret != 0) {
                die("set $copy_path error");
            }
        }
    } else {
        create_database();
    }
}

function create_database()
{
    $file_sql = "/var/www/ipc-webui/sqlite_create.sql";
    $default_database_path = $GLOBALS["data_dir"] . "/face.db";

    if (!($fp_db = fopen($default_database_path, "w+"))) {
        die("fopen $default_database_path error");
    }
    fclose($fp_db);

    class SQLiteDB extends SQLite3
    {
        function __construct($path)
        {
            $this->open($path);
        }
    }

    $db = new SQLiteDB($default_database_path);
    if (!$db) {
        die($db->lastErrorMsg());
    }
    create_database_table($file_sql, $db);

    $ret = set_database_path($default_database_path);
    if ($ret != 0) {
        die("set $default_database_path error");
    }
}

function create_database_table($filename, $db)
{
    if (!($fp_sql = fopen($filename, "r"))) {
        die("fopen $filename error");
    }
    $ret_sql = fread($fp_sql, filesize($filename));
    $result = $db->exec($ret_sql);
    if (!$result) {
        die($db->lastErrorMsg());
    }

    fclose($fp_sql);
}

?>
