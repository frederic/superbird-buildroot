<?php
global $data_dir;
$GLOBALS["data_dir"] = "/etc/ipc/webui";
$GLOBALS["download_dir"] = "/tmp/ipc-webui-downloads/";

function ipc_property($act, $key, $val = "")
{
    $cmd = "ipc-property " . $act . " '" . $key . "'";
    if ($act == "set") {
        $cmd .= " '" . $val . "'";
        exec($cmd, $out, $ret);
        return $ret;
    } else {
        exec($cmd, $out);
        if (!isset($out[0])) {
            return '';
        }
        $v = explode(": ", $out[0]);
        if(count($v) < 2) {
            return $val;
        } else {
            return $v[1];
        }
    }
}

function get_database_path()
{
    return ipc_property("get", "/ipc/nn/recognition/db-path");
}

function set_database_path($path)
{
    return ipc_property("set", "/ipc/nn/recognition/db-path", $path);
}


function download_file($filepath)
{
    if (file_exists($filepath)) {
        header('Content-Description: File Transfer');
        header('Content-Type: application/octet-stream');
        header('Content-Disposition: attachment; filename=' . basename($filepath));
        header('Content-Transfer-Encoding: binary');
        header('Expires: 0');
        header('Cache-Control: must-revalidate, post-check=0, pre-check=0');
        header('Pragma: public');
        header('Content-Length: ' . filesize($filepath));
        ob_clean();
        flush();
        readfile($filepath);
        exit;
    } else {
        die("file not exists");
    }
}

function check_session($locate_path)
{
    session_start();
    if (empty($_SESSION['user'])) {
        echo sprintf("<script>top.window.location = '%s'</script>", $locate_path);
        die;
    }
}

function startsWith ($string, $startString)
{
    $len = strlen($startString);
    return (substr($string, 0, $len) === $startString);
}

