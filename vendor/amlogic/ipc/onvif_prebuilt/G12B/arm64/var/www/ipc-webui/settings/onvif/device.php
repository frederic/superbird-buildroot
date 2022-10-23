<?php
require "../../common.php";

$manufacturer = $_POST["dev_manfacturer"];
ipc_property("set", "/onvif/device/manufacturer", $manufacturer);
?>
