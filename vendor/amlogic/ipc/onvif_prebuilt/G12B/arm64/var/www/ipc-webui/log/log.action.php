<?php
require '../download.php';

$action = $_POST["action"];
$log_path = '/var/log/';
$prop_key = '/ipc/log/';

if ($action == 'get-modules') {
    $module_str = ipc_property('get', $prop_key . 'modules');

    $modules = explode(',', $module_str);

    $debug_level = ipc_property('get', $prop_key . 'debug/level', ".*:1");
    $trace_level = ipc_property('get', $prop_key . 'trace/level', ".*:1");
    echo json_encode(array('modules' => $modules, 'level' => [ 'debug' => $debug_level, 'trace' => $trace_level]));
}

if ($action == 'get-logsize') {
    $response = [
        'debug' => 0,
        'trace' => 0,
    ];
    $logfile = $log_path . 'ipc-debug.log';
    if (file_exists($logfile)) {
        $response['debug'] = filesize($logfile);
    }
    $logfile = $log_path . 'ipc-trace.json';
    if (file_exists($logfile)) {
        $response['trace'] = filesize($logfile);
    }
    echo json_encode($response);
}

if ($action == 'get-log') {
    $logfile = $log_path . 'ipc-debug.log';

    $contents = '';
    if (file_exists($logfile)) {
        $offset = -1;
        if (isset($_POST['offset'])) $offset = $_POST['offset'];

        if(!$fp = fopen($logfile, 'r')) {
            $response['error'] = 'file open failed';
            die(json_encode($response));
        }
        $filesize = filesize($logfile);
        if ($offset <= 0) {
            if (fseek($fp, $filesize > 8192 ?  -8192 : -$filesize, SEEK_END)) {
                $response['error'] = 'file seek failed';
                fclose($fp);
                die(json_encode($response));
            }
            $offset = $filesize;
        } else {
            if ($offset > $filesize) $offset = $filesize;
            if (fseek($fp, $offset, SEEK_SET)) {
                $response['error'] = 'file seek failed';
                fclose($fp);
                die(json_encode($response));
            }
        }
        while (!feof($fp)) {
            $contents .= fread($fp, 8192);
        }
        fclose($fp);
        $response['data'] = $contents;
        $response['length'] = strlen($contents);
        $response['offset'] = $offset + strlen($contents);

        echo json_encode($response);
        return;
    }

    $response['error'] = 'file not exists';
    die(json_encode($response));
}

if ($action == 'start-log' || $action == 'stop-log') {
    $type = $_POST['type'];
    $level = $_POST['level'];
    $file = $log_path;
    if ($type == 'debug') {
        $file .= 'ipc-debug.log';
    } else {
        $file .= 'ipc-trace.json';
    }
    if ($action == 'start-log') {
        ipc_property('set', $prop_key . $type . '/file', '/dev/null');
        ipc_property('set', $prop_key . $type . '/level', '.*:1');
        ipc_property('set', $prop_key . $type . '/file', $file);
        ipc_property('set', $prop_key . $type . '/level', $level);
    } else {
        ipc_property('set', $prop_key . $type . '/level', '.*:1');
        ipc_property('set', $prop_key . $type . '/file', 'stdout');
    }
    $response['ret'] = 'ok';
    echo json_encode($response);
}

if ($action == 'download-debug' || $action == 'download-trace') {
    if ($action == 'download-debug') {
       $logfile = $log_path . 'ipc-debug.log';
    } else {
       $logfile = $log_path . 'ipc-trace.json';
    }
    retreive_file($logfile);
}

if ($action == 'gen-graph') {
    $dir = $GLOBALS["download_dir"] ;
    if(!file_exists($dir)) {
        mkdir($dir, 0700, true);
    }
    chmod($dir, 02710);
    $file = $dir . 'ipc-pipeline.md';
    ipc_property('set', $prop_key . 'graph/file', $dir . 'ipc-pipeline.md');
    ipc_property('set', $prop_key . 'graph/trigger', 'true');
    echo json_encode(array('ret' => 'ok'));
}

if ($action == 'download-graph') {
    $file = $GLOBALS["download_dir"] . 'ipc-pipeline.md';
    $retry = 3;
    while ($retry > 0 && !file_exists($file)) {
        $retry --;
        sleep(1);
    }
    retreive_file($file);
}

