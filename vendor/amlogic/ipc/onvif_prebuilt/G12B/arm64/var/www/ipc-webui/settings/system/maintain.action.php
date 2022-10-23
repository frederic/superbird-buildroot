<?php

require "../../common.php";

$action = $_POST["action"];

$client = stream_socket_client("unix:///tmp/ipc.system.cmd.socket", $errno, $errstr);
if (!$client) {
    die("connect to server fail: $errno - $errstr");
}

$cmd = "system_maintain_op.sh ".$action;
$cmdlen = pack('C', strlen($cmd));
fwrite($client, $cmdlen);
fwrite($client, $cmd);
$id = fread($client, 1024);
if ($id == null) {
    die("failed to get thread id");
}
echo json_encode($id);

fclose($client);

