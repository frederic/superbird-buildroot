<?php
require '../common.php';

$file = $GLOBALS["download_dir"] . 'ipc-pipeline.md';

if (file_exists($file)) {
    $content = file_get_contents($file);
    $content = str_replace('```mermaid', '<div class="mermaid" style="text-align: center;">', $content);
    $content = str_replace('```', '</div>', $content);
    $content = preg_replace('/([ \S]+)[\n\r]==[\n\r]<div/s', '<h2 style="text-align: center;">\1</h2><div', $content);
    echo <<<'HEAD'
<html>
  <head>
    <script src="/scripts/mermaid.min.js"></script>
  </head>
  <body>
HEAD;
    echo $content;
    echo <<< 'TAIL'
    <script>
      mermaid.initialize({startOnLoad:true, theme: "default"});
	</script>
  </body>
</html>
TAIL;
} else {
    echo <<<'HTML'
<html>
  <body>
    <div>
      <h2>Pipeline Graph Not Exists, use `Generate` button to create it first.</h2>
    </div>
  </body>
</html>
HTML;
}
