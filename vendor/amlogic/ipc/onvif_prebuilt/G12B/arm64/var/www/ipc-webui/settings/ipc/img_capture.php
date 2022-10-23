<?php
require "../../common.php";

function img_cap_set($quality)
{
    ipc_property('set', '/ipc/image-capture/quality', $quality);
}

$quality = $_COOKIE["quality"];
img_cap_set($quality);

?>
