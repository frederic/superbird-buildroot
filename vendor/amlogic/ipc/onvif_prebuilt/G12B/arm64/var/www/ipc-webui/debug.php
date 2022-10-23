<?php
function console_log( $data ){
    echo '<script>';
    echo 'console.log('. json_encode( $data ) .')';
    echo '</script>';
}
function wtf()
{
    error_reporting(E_ALL);
    $args = func_get_args();
    $backtrace = debug_backtrace();
    $file = file($backtrace[0]['file']);
    $src = $file[$backtrace[0]['line'] - 1];  // select debug($varname) line where it has been called
    $pat = '#(.*)' . __FUNCTION__ . ' *?\( *?\$(.*) *?\)(.*)#i';  // search pattern for wtf(parameter)
    $arguments = trim(preg_replace($pat, '$2', $src));  // extract arguments pattern
    $args_arr = array_map('trim', explode(',', $arguments));

    print '<style>
  div.debug {visible; clear: both; display: table; width: 100%; font-family: Courier,monospace; border: medium solid red; background-color: yellow; border-spacing: 5px; z-index: 999;}
  div.debug > div {display: unset; margin: 5px; border-spacing: 5px; padding: 5px;}
  div.debug .cell {display: inline-flex; padding: 5px; white-space: pre-wrap;}
  div.debug .left-cell {float: left; background-color: Violet;}
  div.debug .array {color: RebeccaPurple; background-color: Violet;}
  div.debug .object pre {color: DodgerBlue; background-color: PowderBlue;}
  div.debug .variable pre {color: RebeccaPurple; background-color: LightGoldenRodYellow;}
  div.debug pre {white-space: pre-wrap;}
  </style>' . PHP_EOL;
    print '<div class="debug">' . PHP_EOL;
    foreach ($args as $key => $arg) {
        print '<div><div class="left-cell cell"><b>';
        array_walk(debug_backtrace(), function ($a, $b) {
            print "{$a['function']}()(" . basename($a['file']) . ":{$a['line']})<br> ";
        });
        print '</b></div>' . PHP_EOL;
        if (is_array($arg)) {
            print '<div class="cell array"><b>' . $args_arr[$key] . ' = </b>';
            print_r(htmlspecialchars(print_r($arg)), ENT_COMPAT, 'UTF-8');
            print '</div>' . PHP_EOL;
        } elseif (is_object($arg)) {
            print '<div class="cell object"><pre><b>' . $args_arr[$key] . ' = </b>';
            print_r(htmlspecialchars(print_r(var_dump($arg))), ENT_COMPAT, 'UTF-8');
            print '</pre></div>' . PHP_EOL;
        } else {
            print '<div class="cell variable"><pre><b>' . $args_arr[$key] . ' = </b>&gt;';
            print_r(htmlspecialchars($arg, ENT_COMPAT, 'UTF-8') . '&lt;');
            print '</pre></div>' . PHP_EOL;
        }
        print '</div>' . PHP_EOL;
    }
    print '</div>' . PHP_EOL;
}

?>
