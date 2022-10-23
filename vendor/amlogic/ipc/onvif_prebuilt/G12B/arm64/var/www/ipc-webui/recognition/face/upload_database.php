<?php
include "../../common.php";

// 判断文件是不是sqlite数据库文件
function check_is_db($tmp_path)
{
    class SQLiteDB extends SQLite3
    {
        function __construct($path)
        {
            $this->open($path);
        }
    }

    $db = new SQLiteDB($tmp_path);
    if (!$db) {
        die($db->lastErrorMsg());
    }
    $sql1 = <<<EOF
	SELECT COUNT(*) FROM userinfo;
EOF;
    $sql2 = <<<EOF
	SELECT COUNT(*) FROM faceinfo;
EOF;
    $result1 = $db->query($sql1);
    $result2 = $db->query($sql2);
    if (!$result1 || !$result2) {
        return -1;
    } else {
        return 0;
    }
}

$upload_file = $_FILES["file"];
$data_out = '{"code":0, "msg":"", "data":{"res":';

if ($upload_file["error"] == 0 && !empty($upload_file)) {
    $query_path = "/tmp/upload.db";
    if (move_uploaded_file($_FILES["file"]["tmp_name"], $query_path)) {
        $ret = check_is_db($query_path);
        if ($ret == -1) {
            // 1. 不是数据库文件或者表结构不符合要求，先删除上传的文件
            $command_del = "rm -rf " . $query_path;
            exec($command_del, $res, $rc);
            if ($rc != 0) {
                die("delete $query_path error");
            }
            // 2. 返回上传失败信息，并提示用户
            $data_out = $data_out . '-1';
        } else {
            // 1. 先将 database path 置空
            $ret = set_database_path("''");
            if ($ret != 0) {
                die("set database path empty error");
            }

            // delay 500ms
            usleep(500000);

            // 2. 文件符合要求，将其拷贝至/etc/ipc/webui/face.db ，
            //    然后删除query_path，并通过ipc-property设置database path
            $copy_path = $GLOBALS["data_dir"] . "/face.db";
            $command_cp = "cp " . $query_path . " " . $copy_path;
            exec($command_cp, $res, $rc);
            if ($rc != 0) {
                die("copy database error");
            }
            $ret = set_database_path($copy_path);
            if ($ret != 0) {
                die("set $copy_path error");
            }
            $command_del = "rm -rf " . $query_path;
            exec($command_del, $res, $rc);
            if ($rc != 0) {
                die("delete $query_path error");
            }
            // 3. 返回上传成功信息
            $data_out = $data_out . '0';
        }
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
