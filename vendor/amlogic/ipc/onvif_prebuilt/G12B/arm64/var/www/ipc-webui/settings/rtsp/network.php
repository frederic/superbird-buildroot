<?php
require "../../common.php";

function network_set($address, $port, $route, $subroute)
{
    $kvs = array('address' => $address, 'port' => $port, 'route' => $route, 'subroute' => $subroute);
    $vkey = '/rtsp/network/';

    foreach ($kvs as $k => $v) {
        ipc_property('set', $vkey . $k, $v);
    }
}

$address = $_POST["n_address"];
$port = $_POST["n_port"];
$route = $_POST["n_route"];
$subroute = $_POST["n_subroute"];

network_set($address, $port, $route, $subroute);

?>

