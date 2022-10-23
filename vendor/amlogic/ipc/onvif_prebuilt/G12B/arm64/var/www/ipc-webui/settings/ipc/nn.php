<?php
require "../../common.php";

function nn_set($det_enabled, $det_model, $recog_model, $face_info, $face_th)
{
    $kvs = array('detection/model' => $det_model, 'recognition/model' => $recog_model, 'recognition/info-string' => $face_info, 'recognition/threshold' => $face_th);
    if ($det_enabled == "false") {
        $kvs = array('enabled' => $det_enabled) + $kvs;
    } else {
        $kvs['enabled'] = $det_enabled;
    }

    $vkey = '/ipc/nn/';

    foreach ($kvs as $k => $v) {
        ipc_property('set', $vkey . $k, $v);
    }
}

$det_enabled = $_COOKIE["nn_en"];
$det_model = $_POST["nn_det_model"];
$recog_model = $_POST["nn_recog_model"];
$recog_info = $_POST["nn_recog_info_string"];
$recog_th = $_COOKIE["n_th"];

nn_set($det_enabled, $det_model, $recog_model, $recog_info, $recog_th);
?>
