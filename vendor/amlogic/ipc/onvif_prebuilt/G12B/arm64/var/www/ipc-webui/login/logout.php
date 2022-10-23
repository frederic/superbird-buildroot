<?php
#启动session
session_start();

#清除session
unset($_SESSION["user"]);

#重定向
header("location: login.html");
?>
