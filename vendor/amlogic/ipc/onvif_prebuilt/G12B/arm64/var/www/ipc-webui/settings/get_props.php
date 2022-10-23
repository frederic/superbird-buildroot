<?php
require "../common.php";

function get_video_device()
{
    exec("ls /dev/video*", $video_device, $rc);
    if ($rc == 0) {
        return $video_device;
    } else {
        return "error";
    }
}

function get_gdc_config_file()
{
    exec("ls /etc/gdc_config/", $config_file, $rc);
    if ($rc == 0) {
        return $config_file;
    } else {
        return "error";
    }
}

function get_img_file()
{
    exec("ls " . $GLOBALS["data_dir"] . "/image", $img_file, $rc);
    array_push($img_file, "disable");
    return $img_file;
}

function get_font_file()
{
    exec("ls " . $GLOBALS["data_dir"] . "/ttf", $font_file, $rc);
    exec("ls /usr/share/directfb-1.7.7/*.ttf", $def_font, $rc);
    array_push($font_file, basename($def_font[0]));
    if ($rc == 0) {
        return $font_file;
    } else {
        return "";
    }

}

$ipc = array("audio" => array("bitrate" => "", "channels" => "", "codec" => "", "device" => "", "device-options" => "",
    "enabled" => "", "format" => "", "samplerate" => ""),
    "backchannel" => array("clock-rate" => "", "device" => "", "enabled" => "", "encoding" => ""),
    "image-capture" => array("quality" => ""),
    "isp" => array("brightness" => "", "contrast" => "", "exposure/absolute" => "", "exposure/auto" => "",
        "hue" => "", "ircut" => "", "saturation" => "", "sharpness" => "", "wdr/enabled" => "",
        "whitebalance/auto" => "", "whitebalance/cbgain" => "", "whitebalance/cbgain_default" => "",
        "whitebalance/crgain" => "", "whitebalance/crgain_default" => "", "anti-banding" => "",
        "mirroring" => "", "compensation/action"=>"", "compensation/hlc"=>"", "compensation/blc"=>"", "rotation" => "",
        "nr/action"=>"", "nr/space-normal"=>"", "nr/space-expert"=>"", "nr/time"=>""),
    "nn" => array("enabled" => "", "detection/model" => "", "recognition/model" => "", "recognition/db-path" => "",
        "recognition/info-string" => "", "recognition/threshold" => ""),
    "overlay/nn" => array("show" => "", "rect-color" => "", "font/color" => "", "font/font-file" => "", "font/size" => ""),
    "overlay/datetime" => array("enabled" => "", "position" => "", "font/background-color" => "", "font/color" => "",
        "font/font-file" => "", "font/size" => ""),
    "overlay/pts" => array("enabled" => "", "position" => "", "font/background-color" => "", "font/color" => "",
        "font/font-file" => "", "font/size" => ""),
    "recording" => array("chunk-duration" => "", "enabled" => "", "location" => "", "reserved-space-size" => ""),
    "video" => array("multi-channel" => ""),
    "video/ts/gdc" => array("config-file" => "", "enabled" => "", "input-resolution" => "", "output-resolution" => ""),
    "video/ts/main" => array("bitrate" => "", "codec" => "", "device" => "", "framerate" => "", "gop" => "", "resolution" => ""),
    "video/ts/sub" => array("enabled" => "", "bitrate" => "", "codec" => "", "device" => "", "framerate" => "", "gop" => "", "resolution" => ""),
    "video/vr" => array("bitrate" => "", "codec" => "", "device" => "", "framerate" => "", "gop" => "", "resolution" => "",
        "gdc/config-file" => "", "gdc/enabled" => "", "gdc/input-resolution" => "", "gdc/output-resolution" => ""),
    "video/vr/gdc" => array("config-file" => "", "enabled" => "", "input-resolution" => "", "output-resolution" => ""),
);

foreach ($ipc as $x => $x_value) {
    foreach ($x_value as $y => $y_value) {
        $key = sprintf("/ipc/%s/%s", $x, $y);
        $ipc[$x][$y] = $key;
    }
}

$rtsp = array("auth" => array("username" => "", "password" => ""),
    "network" => array("address" => "", "port" => "", "route" => "", "subroute" => ""));

foreach ($rtsp as $x => $x_value) {
    foreach ($x_value as $y => $y_value) {
        $key = sprintf("/rtsp/%s/%s", $x, $y);
        $rtsp[$x][$y] = $key;
    }
}


$onvif = array("discover" => array("enabled" => ""),
    "device" => array("firmware-ver" => "", "hardware-id" => "", "manufacturer" => "", "model" => "", "serial-number" => ""),
    "network" => array("interface" => "", "port" => ""));

foreach ($onvif as $x => $x_value) {
    foreach ($x_value as $y => $y_value) {
        $key = sprintf("/onvif/%s/%s", $x, $y);
        $onvif[$x][$y] = $key;
    }
}

?>

