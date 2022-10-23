<?php
require "./get_props.php";

$all_device = get_video_device();
$img_files = get_img_file();
$font_files = get_font_file();

$prop = $_POST["prop"];
if ($prop == "video") {
    foreach ($ipc["video"] as $item => $value) {
        $ipc["video"][$item] = ipc_property('get', $value);
    }
    $main_reso = ipc_property('get', "/ipc/video/ts/main/resolution");
    $ipc_video = array($ipc["video"], $main_reso);
    echo json_encode($ipc_video);
} elseif ($prop == "video/ts/main") {
    foreach ($ipc["video/ts/main"] as $item => $value) {
        $ipc["video/ts/main"][$item] = ipc_property('get', $value);
    }
    $anti_banding = ipc_property('get', "/ipc/isp/anti-banding");
    $isp_support_4k = ipc_property('get', "/ipc/isp/4k");
    $v_ts_m = array($ipc["video/ts/main"], $all_device, $anti_banding, $isp_support_4k);
    echo json_encode($v_ts_m);
} elseif ($prop == "video/ts/sub") {
    foreach ($ipc["video/ts/sub"] as $item => $value) {
        $ipc["video/ts/sub"][$item] = ipc_property('get', $value);
    }
    $anti_banding = ipc_property('get', "/ipc/isp/anti-banding");
    $v_ts_s = array($ipc["video/ts/sub"], $all_device, $anti_banding);
    echo json_encode($v_ts_s);
} elseif ($prop == "video/ts/gdc") {
    foreach ($ipc["video/ts/gdc"] as $item => $value) {
        $ipc["video/ts/gdc"][$item] = ipc_property('get', $value);
    }
    $all_cong = get_gdc_config_file();
    $g_array = array($ipc["video/ts/gdc"], $all_cong);
    echo json_encode($g_array);
} elseif ($prop == "video/vr") {
    foreach ($ipc["video/vr"] as $item => $value) {
        $ipc["video/vr"][$item] = ipc_property('get', $value);
    }
    $all_cong = get_gdc_config_file();
    $anti_banding = ipc_property('get', "/ipc/isp/anti-banding");
    $isp_support_4k = ipc_property('get', "/ipc/isp/4k");
    $v_vr = array($ipc["video/vr"], $all_device, $all_cong, $anti_banding, $isp_support_4k);
    echo json_encode($v_vr);
} elseif ($prop == "audio") {
    foreach ($ipc["audio"] as $item => $value) {
        $ipc["audio"][$item] = ipc_property('get', $value);
    }
    echo json_encode($ipc["audio"]);
} elseif ($prop == "backchannel") {
    foreach ($ipc["backchannel"] as $item => $value) {
        $ipc["backchannel"][$item] = ipc_property('get', $value);
    }
    echo json_encode($ipc["backchannel"]);
} elseif ($prop == "img_cap") {
    foreach ($ipc["image-capture"] as $item => $value) {
        $ipc["image-capture"][$item] = ipc_property('get', $value);
    }
    echo json_encode($ipc["image-capture"]);
} elseif ($prop == "isp") {
    foreach ($ipc["isp"] as $item => $value) {
        $ipc["isp"][$item] = ipc_property('get', $value);
    }
    $isp_vals = array($ipc["isp"]);
    echo json_encode($isp_vals);
} elseif ($prop == "nn") {
    foreach ($ipc["nn"] as $item => $value) {
        $ipc["nn"][$item] = ipc_property('get', $value);
    }
    echo json_encode($ipc["nn"]);
} elseif ($prop == "ol_nn") {
    foreach ($ipc["overlay/nn"] as $item => $value) {
        $ipc["overlay/nn"][$item] = ipc_property('get', $value);
    }
    $ipc["overlay/nn"]["font/font-file"] = basename($ipc["overlay/nn"]["font/font-file"]);
    $ol_nn_vals = array($ipc["overlay/nn"], $font_files);
    echo json_encode($ol_nn_vals);
} elseif ($prop == "ol_datetime") {
    foreach ($ipc["overlay/datetime"] as $item => $value) {
        $ipc["overlay/datetime"][$item] = ipc_property('get', $value);
    }
    $ipc["overlay/datetime"]["font/font-file"] = basename($ipc["overlay/datetime"]["font/font-file"]);
    $ol_datetime_vals = array($ipc["overlay/datetime"], $font_files);
    echo json_encode($ol_datetime_vals);
} elseif ($prop == "ol_pts") {
    foreach ($ipc["overlay/pts"] as $item => $value) {
        $ipc["overlay/pts"][$item] = ipc_property('get', $value);
    }
    $ipc["overlay/pts"]["font/font-file"] = basename($ipc["overlay/pts"]["font/font-file"]);
    $ol_pts_vals = array($ipc["overlay/pts"], $font_files);
    echo json_encode($ol_pts_vals);
} elseif ($prop == "ol_wm_img") {
    $index = $_POST["index"];
    $max_items = ipc_property('get', '/ipc/overlay/max-image-item');
    if ($max_items == 0) $max_items = 2;
    $prop_items = ipc_property('arrsz', '/ipc/overlay/watermark/image');

    $keys = array("file" => "disable", "position/height" => "0", "position/left" => "0", "position/top" => "0",
        "position/width" => "0",
    );

    foreach ($keys as $k => $v) {
        if ($index < $prop_items) {
            $val = ipc_property('get', '/ipc/overlay/watermark/image/' . $index . '/' . $k);
            if ($val == "") $val = $v;
            if ($k == 'file') {
                $val = $val ? basename($val) : "disable";
            }
        } else {
            $val = $v;
        }
        $ipc_ol_wm_image[$k] = $val;
    }
    $ol_wm_img_vals = array($ipc_ol_wm_image, $img_files, range(1, $max_items));
    echo json_encode($ol_wm_img_vals);
} elseif ($prop == "ol_wm_txt") {
    $index = $_POST["index"];
    $max_items = ipc_property('get', '/ipc/overlay/max-text-item');
    if ($max_items == 0) $max_items = 5;
    $prop_items = ipc_property('arrsz', '/ipc/overlay/watermark/text');

    $keys = array("font/color" => "0xffffffff", "font/font-file" => "decker.ttf",
        "font/size" => "30", "position/left" => "0", "position/top" => "0", "text" => "",
    );
    foreach ($keys as $k => $v) {
        if ($index < $prop_items) {
            $val = ipc_property('get', '/ipc/overlay/watermark/text/' . $index . '/' . $k);
            if ($val == "") $val = $v;
            if ($k == 'font/font-file') {
                $val = basename($val);
            }
        } else {
            $val = $v;
        }
        $ipc_ol_wm_text[$k] = $val;
    }
    $ol_wm_txt_vals = array($ipc_ol_wm_text, $font_files, range(1, $max_items));
    echo json_encode($ol_wm_txt_vals);
} elseif ($prop == "recording") {
    foreach ($ipc["recording"] as $item => $value) {
        $ipc["recording"][$item] = ipc_property('get', $value);
    }
    echo json_encode($ipc["recording"]);
} elseif ($prop == "auth") {
    foreach ($rtsp["auth"] as $item => $value) {
        $rtsp["auth"][$item] = ipc_property('get', $value);
    }
    echo json_encode($rtsp["auth"]);
} elseif ($prop == "rtsp_network") {
    foreach ($rtsp["network"] as $item => $value) {
        $rtsp["network"][$item] = ipc_property('get', $value);
    }
    echo json_encode($rtsp["network"]);
} elseif ($prop == "discover") {
    foreach ($onvif["discover"] as $item => $value) {
        $onvif["discover"][$item] = ipc_property('get', $value);
    }
    echo json_encode($onvif["discover"]);
} elseif ($prop == "device") {
    foreach ($onvif["device"] as $item => $value) {
        $onvif["device"][$item] = ipc_property('get', $value);
    }
    echo json_encode($onvif["device"]);
} elseif ($prop == "network") {
    foreach ($onvif["network"] as $item => $value) {
        $onvif["network"][$item] = ipc_property('get', $value);
    }
    echo json_encode($onvif["network"]);
}

?>
