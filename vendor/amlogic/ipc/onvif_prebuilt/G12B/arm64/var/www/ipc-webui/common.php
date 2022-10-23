<?php
global $data_dir;
$GLOBALS["data_dir"] = "/etc/ipc-webui";

function ipc_property($act, $key, $val = "")
{
    $cmd = "ipc-property " . $act . " '" . $key . "'";
    if ($act == "set") {
        $cmd .= " '" . $val . "'";
        exec($cmd, $out, $ret);
        return $ret;
    } else {
        exec($cmd, $out);
        $v = explode(": ", $out[0]);
        return $v[1];
    }
    return 0;
}

function get_database_path()
{
    return ipc_property("get", "/ipc/nn/recognition/db-path");
}

function set_database_path($path)
{
    return ipc_property("set", "/ipc/nn/recognition/db-path", $path);
}

function face_cap_enable()
{
    return ipc_property("set", "/ipc/nn/store-face", "true");
}

function face_cap_with_img($img_path)
{
    return ipc_property("set", "/ipc/nn/custom-image", $img_path);
}

function get_capture_image_location()
{
    return ipc_property("get", "/ipc/image-capture/location");
}


function set_capture_image_location($path)
{
    return ipc_property("set", "/ipc/image-capture/location", $path);
}

function dump_image_enable()
{
    return ipc_property("set", "/ipc/image-capture/trigger", "true");
}

function get_dump_image_path()
{
    return ipc_property("get", "/ipc/image-capture/imagefile");
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
        header('Location: ' . $locate_path);
    }
}

function make_dir($dir_path)
{
    $command_mkdir = "mkdir -p " . $dir_path;
    exec($command_mkdir, $res, $rc);
    if ($rc != 0) {
        return -1;
    } else {
        return 0;
    }
}

function rename_file($filename)
{
    return str_replace(" ", "_", $filename);
}

function get_gdc_resolution_from_filename($file, $flag)
{
    $anal = explode('-', $file);
    if ($flag == "in") {
        $input = explode('_', $anal[1]);
        return $input[0] . 'x' . $input[1];
    } else {
        $output = explode('_', $anal[2]);
        return $output[2] . 'x' . $output[3];
    }
}

function get_ol_wm_max_item($type)
{
    return ipc_property("get", "/ipc/overlay/max-" . $type . "-item");
}

function get_ol_wm_items($type)
{
    return ipc_property("arrsz", "ipc/overlay/watermark/" . $type);
}

?>
