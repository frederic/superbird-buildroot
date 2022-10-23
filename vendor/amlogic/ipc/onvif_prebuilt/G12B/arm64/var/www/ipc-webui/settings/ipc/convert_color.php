<?php
function convert_to_rgba($color)
{
    $color_hex = explode("0x", $color);
    $color = array("red" => "", "green" => "", "blue" => "", "alpha" => "");
    $i = 0;
    foreach ($color as $x => $x_value) {
        if ($i > 6)
            break;
        $color[$x] = hexdec(substr($color_hex[1], $i, 2));
        $i += 2;
    }
    return "rgba(" . $color['red'] . ", " . $color['green'] . ", " . $color['blue'] . ", " . $color['alpha'] . ")";
}

?>

