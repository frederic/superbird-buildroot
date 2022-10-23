<?php
require "common.php";

function retreive_file($filepath, $unlink = false)
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
        if ($unlink) {
            unlink($filepath);
        }
    }
}

if (isset($_GET['file'])) {
    $fn = $GLOBALS['download_dir'] . $_GET['file'];
    retreive_file($fn, true);
}

