<?php
require "../../common.php";

$interface = $_POST["net_interface"];
$port = $_POST["net_port"];
ipc_property("set", "/onvif/network/interface", $interface);
ipc_property("set", "/onvif/network/port", $port);
?>