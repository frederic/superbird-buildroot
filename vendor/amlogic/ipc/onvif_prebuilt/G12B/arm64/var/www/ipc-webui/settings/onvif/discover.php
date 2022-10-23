<?php
require "../../common.php";

$discover = $_COOKIE["discover_en"];
ipc_property("set", "/onvif/discover/enabled", $discover);
?>