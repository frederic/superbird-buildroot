<?php
require "../../common.php";

$action = $_POST["action"];

$keys = [
    "dev" => "",
    "fs" => "",
    "size" => "",
    "used" => "",
    "available" => "",
    "mp" => ""
];

function connect_server()
{
    $client = stream_socket_client("unix:///tmp/ipc.system.cmd.socket", $errno, $errstr);
    if (!$client) {
        die("connect to server fail: $errno - $errstr");
    }
    return $client;
}


function remote_exec_cmd($client, $cmd) {
    $cmdlen = pack("C", strlen($cmd));
    fwrite($client, $cmdlen);
    fwrite($client, $cmd);
    $id = fread($client, 1024);
    if ($id==null) {
        die("failed to get thread id");
    }
    return $id;
}

function query_status($client, $id) {
    $cmd = "query-status ".$id;
    $status = remote_exec_cmd($client, $cmd);
    if ($status == "-1") {
        die("no corresponding thread");
    }
    return $status;
}

function query_retcode($client, $id) {
    $cmd = "query-recode ".$id;
    $recode = remote_exec_cmd($client, $cmd);
    if ($recode == "-1") {
        die("no corresponding thread");
    }
    return $recode;
}

function query_output($client, $id) {
    $cmd = "query-result ".$id;
    $result = remote_exec_cmd($client, $cmd);
    if ($result == "-1") {
        die("no corresponding thread");
    }
    return $result;
}

function query_result($client, $id, $timeout) {
    do {
        usleep($timeout);
        $status = query_status($client, $id);
    } while($status == "1");
    $recode = query_retcode($client, $id);
    $result = query_output($client, $id);
    if ($recode == "1") {
        die($result);
    } else {
        return $result;
    }
}

$cmd = "storage_device_op.sh ";
$client = connect_server();

if ($action == 'recording') {
    $cmd .= "device ".$action;
    $id = remote_exec_cmd($client, $cmd);
    $result = query_result($client, $id, 200);
    $devs = explode(" ", substr($result, 0, strlen($result)-1));
    echo json_encode($devs);
}elseif ($action == 'device') {
    $cmd .= $action." storage";
    $id = remote_exec_cmd($client, $cmd);
    $result = query_result($client, $id, 200);
    $devs = explode(" ", substr($result, 0, strlen($result)-1));
    echo json_encode($devs);
} elseif ($action == "info") {
    $dev = $_POST["prop"];
    $cmd .= $action." ".$dev;
    $id = remote_exec_cmd($client, $cmd);
    $result = query_result($client, $id, 200);
    $info = explode(" ", substr($result, 0, strlen($result)-1));
    $i=0;
    foreach ($keys as $x => $y) {
        $keys[$x] = $info[$i++];
    }
    echo json_encode($keys);
} elseif ($action == "format") {
    $dev = $_POST["dev"];
    $fs = $_POST["fs"];
    $mp = $_POST["mp"];
    $cmd .= $action." ".$dev." ".$fs." ".$mp;
    $id = remote_exec_cmd($client, $cmd);
    $result = query_result($client, $id, 500);
    $res = substr($result, 0, strlen($result)-1);
    echo json_encode($res);
}

fclose($client);
