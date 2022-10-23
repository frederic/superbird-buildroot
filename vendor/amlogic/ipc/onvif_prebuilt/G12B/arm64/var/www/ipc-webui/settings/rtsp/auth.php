<?php
function auth_set($username, $password)
{
    $kvs = array('username' => $username, 'password' => $password);
    $vkey = '/rtsp/auth/';

    foreach ($kvs as $k => $v) {
        ipc_property('set', $vkey . $k, $v);
    }
}

$username = $_POST["a_username"];
$password = $_POST["a_pwd"];

auth_set($username, $password);

?>



