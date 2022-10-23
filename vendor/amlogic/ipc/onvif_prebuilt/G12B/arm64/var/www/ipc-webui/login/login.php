<?php
require "../common.php";
session_start();

//获取post的数据
$account = $_POST['account'];
$passwd = $_POST['password'];
$flag = 0;


// 对已存在用户进行计数
function get_onvif_user($index, $key)
{
    return ipc_property("get", "/onvif/users/" . $index . "/" . $key);
}

if ($account == "admin") {
    $num_user = ipc_property("arrsz", "/onvif/users");
    for ($i = 0; $i < $num_user; $i++) {
        if (($account == get_onvif_user($i, "name")) &&
            ($passwd == get_onvif_user($i, "password"))) {
            $flag = 1;
            break;
        }
    }
}

if ($flag == 1) {
    $_SESSION['user'] = $account;
    echo '1';
} else {
    echo '0';
}

?>
