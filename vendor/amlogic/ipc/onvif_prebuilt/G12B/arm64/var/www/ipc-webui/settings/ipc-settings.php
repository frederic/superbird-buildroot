<?php
require "./ipc/convert_color.php";
require "./get_props.php";
require "./create_elem.php";
require "../select_lang.php";
check_session("../login/login.html");
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
    <title><?php echo gettext("IPC Setting"); ?></title>
    <meta http-equiv="content-type" content="text/html;charset=utf-8"/>
    <script src="/scripts/jquery.min.js"></script>
    <link rel="stylesheet" href="/layui/css/layui.css" media="all">
</head>

<body>
<h1><?php echo gettext("IPC Setting"); ?></h1>
<hr class="layui-bg-green">
<br>
<!--<div class="layui-collapse" hidden>-->
<!--</div>-->
<div class="layui-collapse" lay-filter="ipc" lay-accordion>
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Video"); ?></h2>
        <div class="layui-colla-content">
            <div id="video_multi_area" hidden>
                <form target="video" class="layui-form layui-form-pane" action="./ipc/video.php?p_id=mu_en"
                      method="post" onreset="reset_video()" lay-filter="video">
                    <div class="layui-form-item">
                        <label class="layui-form-label"
                               style="width: auto"><?php echo gettext("multichannel"); ?></label>
                        <div class="layui-input-inline" id="video_mu_en"></div>
                    </div>
                    <div class="layui-form-item">
                        <div class="layui-input-inline">
                            <button class="layui-btn" lay-submit
                                    lay-filter="v_mu_submit"><?php echo gettext("Submit"); ?></button>
                            <button type="reset"
                                    class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                        </div>
                    </div>
                </form>
            </div>
            <iframe name="video" style="display:none"></iframe>
            <div class="layui-collapse layui-show" id="video_ts" lay-accordion>
                <div class="layui-colla-item">
                    <h2 class="layui-colla-title"><?php echo gettext("channel") ?></h2>
                    <div class="layui-colla-content" id="video_channel_con">
                        <div class="layui-collapse" lay-filter="video_channel" lay-accordion>
                            <div class="layui-colla-item">
                                <h2 class="layui-colla-title"><?php echo gettext("main") ?></h2>
                                <div class="layui-colla-content">
                                    <?php create_Vform("v_ts_m", "video.php?p_id=main", "v_ts_m") ?>
                                </div>
                            </div>
                            <div class="layui-colla-item">
                                <h2 class="layui-colla-title"><?php echo gettext("substream") ?></h2>
                                <div class="layui-colla-content">
                                    <?php create_Vform("v_ts_s", "video.php?p_id=sub", "v_ts_s") ?>
                                </div>
                            </div>
                            <div class="layui-colla-item">
                                <h2 class="layui-colla-title"><?php echo gettext("GDC") ?></h2>
                                <div class="layui-colla-content">
                                    <?php create_Vgdc("v_ts_g", "video.php?p_id=ts_gdc", "v_ts_g") ?>
                                </div>
                            </div>
                            <div class="layui-colla-item" id="video_vr">
                                <h2 class="layui-colla-title"><?php echo gettext("video recording") ?></h2>
                                <div class="layui-colla-content">
                                    <?php create_Vform("v_vr", "video.php?p_id=vr", "v_vr") ?>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Audio"); ?></h2>
        <div class="layui-colla-content">
            <form target="audio" class="layui-form layui-form-pane" name="audio" action="./ipc/audio.php" method="post"
                  onreset="reset_audio()">
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("enabled"); ?></label>
                    <div class="layui-input-inline" id="au_en"></div>
                </div>
                <div id="au_prop" hidden>
                    <div class="layui-form-item">
                        <label class="layui-form-label"><?php echo gettext("codec"); ?></label>
                        <div class="layui-input-inline">
                            <select name="a_codec" id="selCodec" lay-filter="acodec"></select>
                        </div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"><?php echo gettext("bitrate"); ?></label>
                        <div class="layui-input-inline">
                            <select name="a_bitrate" id="bitrateId" lay-filter="abitrate"></select>
                        </div>
                        <div class="layui-form-mid layui-word-aux">kbps</div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"><?php echo gettext("channels"); ?></label>
                        <div class="layui-input-inline">
                            <input type="text" name="a_channels" class="layui-input layui-disabled" autocomplete="off"
                                   id="au_channels">
                        </div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"><?php echo gettext("device"); ?></label>
                        <div class="layui-input-inline">
                            <input type="text" name="a_device" class="layui-input" autocomplete="off" id="au_dev"
                                   value="">
                        </div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"
                               style="width: auto"><?php echo gettext("device options"); ?></label>
                        <div class="layui-input-inline">
                            <input type="text" name="a_device_opt" class="layui-input" autocomplete="off"
                                   id="au_dev_opt" value="">
                        </div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"><?php echo gettext("format"); ?></label>
                        <div class="layui-input-inline">
                            <input type="text" name="a_format" class="layui-input layui-disabled" autocomplete="off"
                                   id="au_format">
                        </div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"><?php echo gettext("samplerate"); ?></label>
                        <div class="layui-input-inline">
                            <select name="a_samplerate" lay-filter="asam" id="au_samplerate"></select>
                        </div>
                        <div class="layui-form-mid layui-word-aux">KHz</div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-input-inline">
                        <button class="layui-btn" lay-filter="demo1"><?php echo gettext("Submit"); ?></button>
                        <button type="reset"
                                class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                    </div>
                </div>
            </form>
            <iframe name="audio" style="display:none"></iframe>
        </div>
    </div>
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Backchannel"); ?></h2>
        <div class="layui-colla-content">
            <form target="backchannel" class="layui-form layui-form-pane" action="./ipc/backchannel.php" method="post"
                  onreset="reset_backchannel()">
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("enabled"); ?></label>
                    <div class="layui-input-inline" id="bc_en"></div>
                </div>
                <div id="bc_prop" hidden>
                    <div class="layui-form-item">
                        <label class="layui-form-label"><?php echo gettext("device"); ?></label>
                        <div class="layui-input-inline">
                            <input type="text" name="b_device" class="layui-input" autocomplete="off" id="bc_dev"
                                   value="">
                        </div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"><?php echo gettext("encoding"); ?></label>
                        <div class="layui-input-inline">
                            <input type="text" name="b_encoding" class="layui-input layui-disabled" autocomplete="off"
                                   id="bc_encoding">
                        </div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-input-inline">
                        <button class="layui-btn" lay-submit
                                lay-filter="demo1"><?php echo gettext("Submit"); ?></button>
                        <button type="reset"
                                class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                    </div>
                </div>
            </form>
            <iframe name="backchannel" style="display:none"></iframe>
        </div>
    </div>
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Image Capture"); ?></h2>
        <div class="layui-colla-content">
            <form target="img_cap" class="layui-form layui-form-pane" action="./ipc/img_capture.php" method="post"
                  onreset="reset_img_cap()">
                <div class="layui-form-item">
                    <label class="layui-form-label" style="margin-right:15px;"><?php echo gettext("quality"); ?></label>
                    <div class="layui-input-inline">
                        <br>
                        <div id="slideIcap" class="demo-slider"></div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-input-inline">
                        <button class="layui-btn" lay-filter="demo1"><?php echo gettext("Submit"); ?></button>
                        <button type="reset"
                                class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                    </div>
                </div>
            </form>
            <iframe name="img_cap" style="display:none"></iframe>
        </div>
    </div>
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Isp"); ?></h2>
        <div class="layui-colla-content">
            <form target="isp" class="layui-form layui-form-pane" action="./ipc/isp.php" method="post"
                  onreset="reset_isp()">
                <div class="layui-form-item">
                    <label class="layui-form-label"
                           style="margin-right:15px;"><?php echo gettext("brightness"); ?></label>
                    &nbsp
                    <div class="layui-input-inline">
                        <br>
                        <div id="slideBright" class="demo-slider" name="i_brightness"></div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label"
                           style="margin-right:15px;"><?php echo gettext("contrast"); ?></label>
                    <div class="layui-input-inline">
                        <br>
                        <div id="slideContrast" class="demo-slider"></div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-inline">
                        <label class="layui-form-label" style="width: auto"><?php echo gettext("exposure auto"); ?></label>
                        <div class="layui-input-inline" id="i_exau"></div>
                    </div>
                    <div class="layui-inline" id="i_exab">
                        <label class="layui-form-label"
                               style="width: auto; margin-right:20px;"><?php echo gettext("exposure absolute"); ?></label>
                        <div class="layui-input-inline">
                            <br>
                            <div id="slideExab" class="demo-slider"></div>
                        </div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label" style="margin-right:15px;"><?php echo gettext("hue"); ?></label>
                    <div class="layui-input-inline">
                        <br>
                        <div id="slideHue" class="demo-slider"></div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("ircut"); ?></label>
                    <div class="layui-input-inline" id="i_ircut"></div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label"
                           style="margin-right:15px;"><?php echo gettext("saturation"); ?></label>
                    <div class="layui-input-inline">
                        <br>
                        <div id="slideSat" class="demo-slider"></div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label"
                           style="margin-right:15px;"><?php echo gettext("sharpness"); ?></label>
                    <div class="layui-input-inline">
                        <br>
                        <div id="slideSharp" class="demo-slider"></div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label" style="width: auto"><?php echo gettext("WDR"); ?></label>
                    <div class="layui-input-inline" id="i_wdr"></div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-inline">
                        <label class="layui-form-label"
                           style="width: auto"><?php echo gettext("white balance auto"); ?></label>
                        <div class="layui-input-inline" id="i_whau"></div>
                    </div>
                    <div id="i_wh">
                        <div class="layui-inline">
                            <label class="layui-form-label"
                               style="width: auto; margin-right:15px;"><?php echo gettext("CbGain"); ?></label>
                            <div class="layui-input-inline">
                                <br>
                                <div id="slideWhcb" class="demo-slider"></div>
                            </div>
                        </div>
                        <div class="layui-inline">
                            <label class="layui-form-label"
                               style="width: auto; margin-right:15px;"><?php echo gettext("CrGain"); ?></label>
                            <div class="layui-input-inline">
                                <br>
                                <div id="slideWhcr" class="demo-slider"></div>
                            </div>
                        </div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label" style="width: auto"><?php echo gettext("video standard"); ?></label>
                    <div class="layui-input-inline">
                        <select name="isp_video_mode" lay-filter="ispvmode" id="isp_video_mode_sel"></select>
                    </div>
                    <div class="layui-form-mid layui-word-aux">HZ</div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label" style="width: auto"><?php echo gettext("mirror"); ?></label>
                    <div class="layui-input-inline">
                        <select name="isp_mirror" lay-filter="ispmirror" id="isp_mirror_sel"></select>
                    </div>
                </div>
                <div class="layui-form-item">
                    <label class="layui-form-label" style="width: auto"><?php echo gettext("rotation"); ?></label>
                    <div class="layui-input-inline">
                        <select name="isp_rotation" lay-filter="isprotation" id="isp_rotation_sel"></select>
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-inline">
                        <label class="layui-form-label" style="width: auto"><?php echo gettext("light compensation"); ?></label>
                        <div class="layui-input-inline">
                            <select name="isp_lc" lay-filter="isplc" id="isp_lc_sel" style="width: auto;"></select>
                        </div>
                    </div>
                    <div class="layui-inline" id="i_lc">
                        <label class="layui-form-label" style="width: auto; margin-right:15px;"><?php echo gettext("level"); ?></label>
                        <div class="layui-input-inline">
                            <br>
                            <div id="slideLclev" class="demo-slider"></div>
                        </div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-inline">
                        <label class="layui-form-label" style="width: auto"><?php echo gettext("digital noise reduction"); ?></label>
                        <div class="layui-input-inline">
                            <select name="isp_nr" lay-filter="ispnr" id="isp_nr_sel" style="width: auto;"></select>
                        </div>
                    </div>
                    <div class="layui-inline" id="i_nsl">
                        <label class="layui-form-label" style="width: auto; margin-right:15px;"><?php echo gettext("space level"); ?></label>
                        <div class="layui-input-inline">
                            <br>
                            <div id="slideNrslev" class="demo-slider"></div>
                        </div>
                    </div>
                    <div class="layui-inline" id="i_ntl">
                        <label class="layui-form-label" style="width: auto; margin-right:15px;"><?php echo gettext("time level"); ?></label>
                        <div class="layui-input-inline">
                            <br>
                            <div id="slideNrtlev" class="demo-slider"></div>
                        </div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-input-inline">
                        <button class="layui-btn" lay-submit lay-filter="isp"><?php echo gettext("Submit"); ?></button>
                        <button type="reset"
                                class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                    </div>
                </div>
            </form>
            <iframe name="isp" style="display:none"></iframe>
        </div>
    </div>
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("NN"); ?></h2>
        <div class="layui-colla-content">
            <form target="nn" class="layui-form layui-form-pane" action="./ipc/nn.php" method="post"
                  onreset="reset_nn()">
                <div class="layui-form-item">
                    <label class="layui-form-label" style="width: auto"><?php echo gettext("enabled"); ?></label>
                    <div class="layui-input-inline" id="nn_en"></div>
                </div>
                <div id="nn_con" hidden>
                    <div class="layui-form-item">
                        <label class="layui-form-label"
                               style="width: auto"><?php echo gettext("detection model"); ?></label>
                        <div class="layui-input-inline">
                            <select name="nn_det_model" lay-filter="nndetmd" id="nn_det_mod_sel"></select>
                        </div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"
                               style="width: auto"><?php echo gettext("recognition model"); ?></label>
                        <div class="layui-input-inline">
                            <select name="nn_recog_model" lay-filter="nnrecogmd" id="nn_recog_mod_sel"></select>
                        </div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"
                               style="width: auto"><?php echo gettext("recognition info string"); ?></label>
                        <div class="layui-input-inline">
                            <input type="text" name="nn_recog_info_string" class="layui-input" autocomplete="off"
                                   id="nn_recog_info" value="">
                        </div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"
                               style="margin-right:15px; width: auto"><?php echo gettext("recognition threshold"); ?></label>
                        &nbsp
                        <div class="layui-input-inline">
                            <br>
                            <div id="slideNth" class="demo-slider"></div>
                        </div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-input-inline">
                        <button class="layui-btn" lay-submit
                                lay-filter="demo1"><?php echo gettext("Submit"); ?></button>
                        <button type="reset"
                                class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                    </div>
                </div>
            </form>
            <iframe name="nn" style="display:none"></iframe>
        </div>
    </div>
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Overlay"); ?></h2>
        <div class="layui-colla-content">
            <div class="layui-collapse" lay-filter="ipc_ol" lay-accordion>
                <div class="layui-colla-item">
                    <h2 class="layui-colla-title"><?php echo gettext("NN"); ?></h2>
                    <div class="layui-colla-content">
                        <form target="o_nn" class="layui-form layui-form-pane" action="./ipc/ol_nn.php" method="post"
                              enctype="multipart/form-data" onreset="reset_ol_nn()">
                            <div class="layui-form-item">
                                <label class="layui-form-label"
                                       style="width: auto"><?php echo gettext("show"); ?></label>
                                <div class="layui-input-inline" id="ol_nn_en"></div>
                            </div>
                            <div id="ol_nn_con" hidden>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"
                                           style="width: auto"><?php echo gettext("rect color"); ?></label>
                                    <div class="layui-input-inline">
                                        <input type="text" name="ol_nn_rect_color" id="onrc-form-input"
                                               class="layui-input" autocomplete="off" value="">
                                    </div>
                                    <div class="layui-inline" style="left: -11px;">
                                        <div id="onrc-form"></div>
                                    </div>
                                </div>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"
                                           style="width: auto"><?php echo gettext("font color"); ?></label>
                                    <div class="layui-input-inline">
                                        <input type="text" name="ol_nn_font_color" id="onfc-form-input"
                                               class="layui-input" autocomplete="off" id="onfc-form-input" value="">
                                    </div>
                                    <div class="layui-inline" style="left: -11px;">
                                        <div id="onfc-form"></div>
                                    </div>
                                </div>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"
                                           style="margin-right:15px;"><?php echo gettext("font size"); ?></label>
                                    <div class="layui-input-inline">
                                        <br>
                                        <div id="slideNfs" class="demo-slider"></div>
                                    </div>
                                </div>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"
                                           style="width: auto"><?php echo gettext("font file"); ?></label>
                                    <div class="layui-input-inline">
                                        <select name="ol_nn_font_file" lay-filter="ol_nn_tff" id="n_ttf"></select>
                                    </div>
                                    <div class="layui-input-inline">
                                        <button type="button" class="layui-btn" id="n_ff" name="file" value="nn"><i
                                                    class="layui-icon">&#xe67c;</i><?php echo gettext("upload file") ?>
                                        </button>
                                    </div>
                                </div>
                            </div>
                            <div class="layui-form-item">
                                <div class="layui-input-inline">
                                    <button class="layui-btn" lay-submit lay-filter="demo1"
                                            id="ol_nn_set"><?php echo gettext("Submit"); ?></button>
                                    <button type="reset"
                                            class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                                </div>
                            </div>
                        </form>
                        <iframe name="o_nn" style="display:none"></iframe>
                    </div>
                </div>
                <div class="layui-colla-item">
                    <h2 class="layui-colla-title"><?php echo gettext("datetime"); ?></h2>
                    <div class="layui-colla-content">
                        <form target="o_datetime" class="layui-form layui-form-pane" action="./ipc/ol_time.php?p_id=dt"
                              method="post" enctype="multipart/form-data" onreset="reset_ol_datetime()">
                            <div class="layui-form-item">
                                <label class="layui-form-label"
                                       style="width: auto"><?php echo gettext("enabled"); ?></label>
                                <div class="layui-input-inline" id="ol_datetime_en"></div>
                            </div>
                            <div id="ol_datetime" hidden>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"
                                           style="width: auto"><?php echo gettext("position"); ?></label>
                                    <div class="layui-input-inline">
                                        <select name="ol_pos_datetime" lay-filter="otlc" id="otlc"></select>
                                    </div>
                                </div>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"
                                           style="width: auto"><?php echo gettext("background color"); ?></label>
                                    <div class="layui-input-inline">
                                        <input type="text" name="ol_font_bgc_datetime" id="otfbc-form-input"
                                               class="layui-input" autocomplete="off"
                                               value="">
                                    </div>
                                    <div class="layui-inline" style="left: -11px;">
                                        <div id="otfbc-form"></div>
                                    </div>
                                </div>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"><?php echo gettext("font color"); ?></label>
                                    <div class="layui-input-inline">
                                        <input type="text" name="ol_font_color_datetime" id="otfc-form-input"
                                               class="layui-input" autocomplete="off"
                                               value="">
                                    </div>
                                    <div class="layui-inline" style="left: -11px;">
                                        <div id="otfc-form"></div>
                                    </div>
                                </div>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"><?php echo gettext("font file"); ?></label>
                                    <div class="layui-input-inline">
                                        <select name="ol_font_file_datetime" lay-filter="ol_datetime_tff"
                                                id="t_ttf"></select>
                                    </div>
                                    <div class="layui-input-inline">
                                        <button type="button" class="layui-btn" id="t_f" name="file" value="datetime"><i
                                                    class="layui-icon">&#xe67c;</i><?php echo gettext("upload file") ?>
                                        </button>
                                    </div>
                                </div>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"
                                           style="margin-right:15px;"><?php echo gettext("font size"); ?></label>
                                    <div class="layui-input-inline">
                                        &nbsp
                                        <div class="layui-input-inline">
                                            <div id="slideTfs" class="demo-slider"></div>
                                        </div>
                                    </div>
                                </div>
                            </div>
                            <div class="layui-form-item">
                                <div class="layui-input-inline">
                                    <button class="layui-btn" lay-submit="" lay-filter="demo1"
                                            id="ol_datetime_set"><?php echo gettext("Submit"); ?></button>
                                    <button type="reset"
                                            class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                                </div>
                            </div>
                        </form>
                        <iframe name="o_datetime" style="display:none"></iframe>
                    </div>
                </div>
                <div class="layui-colla-item">
                    <h2 class="layui-colla-title"><?php echo gettext("PTS"); ?></h2>
                    <div class="layui-colla-content">
                        <form target="o_pts" class="layui-form layui-form-pane" action="./ipc/ol_time.php?p_id=pts"
                              method="post" enctype="multipart/form-data" onreset="reset_ol_pts()">
                            <div class="layui-form-item">
                                <label class="layui-form-label"
                                       style="width: auto"><?php echo gettext("enabled"); ?></label>
                                <div class="layui-input-inline" id="ol_pts_en"></div>
                            </div>
                            <div id="ol_pts" hidden>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"
                                           style="width: auto"><?php echo gettext("position"); ?></label>
                                    <div class="layui-input-inline">
                                        <select name="ol_pos_pts" lay-filter="oplc" id="oplc"></select>
                                    </div>
                                </div>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"
                                           style="width: auto"><?php echo gettext("background color"); ?></label>
                                    <div class="layui-input-inline">
                                        <input type="text" name="ol_font_bgc_pts" id="opfbc-form-input"
                                               class="layui-input" autocomplete="off"
                                               value="">
                                    </div>
                                    <div class="layui-inline" style="left: -11px;">
                                        <div id="opfbc-form"></div>
                                    </div>
                                </div>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"><?php echo gettext("font color"); ?></label>
                                    <div class="layui-input-inline">
                                        <input type="text" name="ol_font_color_pts" id="opfc-form-input"
                                               class="layui-input" autocomplete="off"
                                               value="">
                                    </div>
                                    <div class="layui-inline" style="left: -11px;">
                                        <div id="opfc-form"></div>
                                    </div>
                                </div>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"><?php echo gettext("font file"); ?></label>
                                    <div class="layui-input-inline">
                                        <select name="ol_font_file_pts" lay-filter="ol_pts_tff" id="t_ptf"></select>
                                    </div>
                                    <div class="layui-input-inline">
                                        <button type="button" class="layui-btn" id="p_f" name="file" value="pts"><i
                                                    class="layui-icon">&#xe67c;</i><?php echo gettext("upload file") ?>
                                        </button>
                                    </div>
                                </div>
                                <div class="layui-form-item">
                                    <label class="layui-form-label"
                                           style="margin-right:15px;"><?php echo gettext("font size"); ?></label>
                                    <div class="layui-input-inline">
                                        &nbsp
                                        <div class="layui-input-inline">
                                            <div id="slidePfs" class="demo-slider"></div>
                                        </div>
                                    </div>
                                </div>
                            </div>
                            <div class="layui-form-item">
                                <div class="layui-input-inline">
                                    <button class="layui-btn" lay-submit="" lay-filter="demo1"
                                            id="ol_pts_set"><?php echo gettext("Submit"); ?></button>
                                    <button type="reset"
                                            class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                                </div>
                            </div>
                        </form>
                        <iframe name="o_pts" style="display:none"></iframe>
                    </div>
                </div>
                <div class="layui-colla-item">
                    <h2 class="layui-colla-title"><?php echo gettext("watermark"); ?></h2>
                    <div class="layui-colla-content">
                        <div class="layui-collapse" lay-filter="ipc_ol_wm" lay-accordion>
                            <?php create_ol_wm_text(); ?>
                            <?php create_ol_wm_image(); ?>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
    <div class="layui-colla-item">
        <h2 class="layui-colla-title"><?php echo gettext("Recording"); ?></h2>
        <div class="layui-colla-content">
            <form target="recording" class="layui-form layui-form-pane" action="./ipc/recording.php" method="post"
                  onreset="reset_vr()">
                <div class="layui-form-item">
                    <label class="layui-form-label"><?php echo gettext("enabled"); ?></label>
                    <div class="layui-input-inline" id="vr_en"></div>
                </div>
                <div id="vr" hidden>
                    <div class="layui-form-item">
                        <label class="layui-form-label"
                               style="width: auto"><?php echo gettext("chunk duration"); ?></label>
                        <div class="layui-input-inline">
                            <input type="text" name="s_chunk_duration" class="layui-input" autocomplete="off"
                                   lay-verify="number" id="vr_cd" value="">
                        </div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"><?php echo gettext("location"); ?></label>
                        <div class="layui-input-inline">
                            <input type="text" name="s_location" class="layui-input" autocomplete="off" id="vr_loc"
                                   value="">
                        </div>
                    </div>
                    <div class="layui-form-item">
                        <label class="layui-form-label"
                               style="width: auto"><?php echo gettext("reserved space size"); ?></label>
                        <div class="layui-input-inline">
                            <input type="text" name="s_size" class="layui-input" autocomplete="off" lay-verify="number"
                                   id="vr_rs" value="">
                        </div>
                        <div class="layui-form-mid layui-word-aux">MB</div>
                    </div>
                </div>
                <div class="layui-form-item">
                    <div class="layui-input-inline">
                        <button class="layui-btn" lay-submit=""
                                lay-filter="demo1"><?php echo gettext("Submit"); ?></button>
                        <button type="reset"
                                class="layui-btn layui-btn-primary"><?php echo gettext("Reset"); ?></button>
                    </div>
                </div>
            </form>
            <iframe name="recording" style="display:none"></iframe>
        </div>
    </div>
</div>
<script src="/layui/layui.js"></script>
<script>
    //create global slider variable
    var sli_V_ts_mb, sli_V_ts_mg, sli_V_ts_sb, sli_V_ts_sg, sli_V_vr_b, sli_V_vr_g, sli_Icq,
        sli_Ib, sli_Ic, sli_Ish, sli_Isa, sli_Ih, sli_Iwb, sli_Iwr, sli_Iex, sli_Ilc, sli_Inrsl,
        sli_Inrtl, sli_Nft, sli_Onfs, sli_Otfs, sli_Opfs, sli_Owtfs;

    //set reset_btn click envent
    function reset_video() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "video"},
            success: function (data) {
                if (data[0]["multi-channel"] === "true") {
                    $("#video_mu_en").empty().append('<input type="checkbox"  lay-skin="switch" checked lay-filter="v_mu_en" value="true" id="v_mu_en">');
                    $("#video_ts").show();
                    $("#video_vr").show();
                } else {
                    $("#video_mu_en").empty().append('<input type="checkbox"  lay-skin="switch" lay-filter="v_mu_en" value="false" id="v_mu_en">');
                    $("#video_ts").show();
                    $("#video_vr").hide();
                }
                <?php re_render();?>
            }
        });
    }
    <?php create_Vreset("v_ts_m", "video/ts/main", "sli_V_ts_mb", "sli_V_ts_mg")?>
    <?php create_Vreset("v_ts_s", "video/ts/sub", "sli_V_ts_sb", "sli_V_ts_sg")?>
    <?php create_Vreset_gdc("v_ts_g", "video/ts/gdc")?>
    <?php create_Vreset("v_vr", "video/vr", "sli_V_vr_b", "sli_V_vr_g")?>
    function reset_audio() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "audio"},
            success: function (data) {
                <?php re_create_switch('data["enabled"]', "a_en", "au_en", "au_prop", "a_en", "au_btn", "yes");?>
                var codec_vals = ["g711", "g726", "aac"];
                var bit_vals = [["64"], ["16", "24", "32", "40"], ["16", "24", "32", "40", "64", "96", "128", "160"]];
                var codec_val = data["codec"];
                var bit_val = data["bitrate"];
                $("#selCodec").empty();
                $("#bitrateId").empty();
                for (var i = 0; i < codec_vals.length; i++) {
                    if (codec_vals[i] === codec_val) {
                        $("#selCodec").append('<option selected value="' + codec_vals[i] + '">' + codec_vals[i] + '</option>');
                    } else {
                        $("#selCodec").append('<option value="' + codec_vals[i] + '">' + codec_vals[i] + '</option>');
                    }
                }
                if (codec_val === "g711") {
                    $("#bitrateId").append('<option selected value="64">64</option>');
                } else if (codec_val === "g726") {
                    for (i = 0; i < bit_vals[1].length; i++) {
                        if (bit_vals[1][i] === bit_val) {
                            $("#bitrateId").append('<option selected value="' + bit_vals[1][i] + '">' + bit_vals[1][i] + '</option>');
                        } else {
                            $("#bitrateId").append('<option value="' + bit_vals[1][i] + '">' + bit_vals[1][i] + '</option>');
                        }
                    }
                } else if (codec_val === "aac") {
                    for (i = 0; i < bit_vals[2].length; i++) {
                        if (bit_vals[2][i] === bit_val) {
                            $("#bitrateId").append('<option selected value="' + bit_vals[2][i] + '">' + bit_vals[2][i] + '</option>');
                        } else {
                            $("#bitrateId").append('<option value="' + bit_vals[2][i] + '">' + bit_vals[2][i] + '</option>');
                        }
                    }
                }
                $("#au_dev").val(data["device"]);
                $("#au_dev_opt").val(data["device-options"]);
                $("#au_samplerate").val(data["samplerate"]);
                <?php re_render();?>
            }
        });

    }

    function reset_backchannel() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "backchannel"},
            success: function (data) {
                <?php re_create_switch('data["enabled"]', "b_en", "bc_en", "bc_prop", "b_en", "nc_btn", "yes");?>
                $("#bc_dev").val(data["device"]);
                $("#bc_encoding").val(data["encoding"]);
                <?php re_render();?>
            }
        })
    }

    function reset_img_cap() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "img_cap"},
            success: function (data) {
                sli_Icq.setValue((data["quality"] - 10) / 5);
                <?php re_render();?>
            }
        });
    }

    function reset_isp() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "isp"},
            success: function (data) {
                <?php re_create_switch('data[0]["exposure/auto"]', "i_exp", "i_exau", "i_exab", "i_exp", "exau_btn", "no");?>
                <?php re_create_switch('data[0]["whitebalance/auto"]', "i_whau", "i_whau", "i_wh", "i_whau", "wh_btn", "no");?>
                <?php re_create_switch('data[0]["ircut"]', "i_ircut", "i_ircut", "none", "i_ircut", "ir_btn", "no");?>
                <?php re_create_switch('data[0]["wdr/enabled"]', "i_wdr", "i_wdr", "none", "i_wdr", "wdr_btn", "no");?>
                $("#isp_video_mode_sel").val(data[0]["anti-banding"]);
                $("#isp_mirror_sel").val(data[0]["mirroring"]);
                $("#isp_rotation_sel").val(data[0]["rotation"]);
                $("#isp_lc_sel").val(data[0]["compensation/action"]);
                $("#isp_nr_sel").val(data[0]["nr/action"]);
                if (data[0]["compensation/action"] === "disable") {
                    $("#i_lc").hide();
                } else if (data[0]["compensation/action"] === "hlc") {
                    sli_Ilc.setValue(data[0]["compensation/hlc"]);
                    $("#i_lc").show();
                } else {
                    sli_Ilc.setValue(data[0]["compensation/blc"]);
                    $("#i_lc").show();
                }
                isp_set_value(data);
                <?php re_render();?>
            }
        });
    }

    function reset_nn() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "nn"},
            success: function (data) {
                <?php re_create_switch('data["enabled"]', "nn_en", "nn_en", "nn_con", "nn_en", "nn_btn", "yes");?>
                $("#nn_det_mod_sel").val(data["detection/model"]);
                $("#nn_recog_mod_sel").val(data["recognition/model"]);
                $("#nn_recog_info").val(data["recognition/info-string"]);
                sli_Nft.setValue(data["recognition/threshold"] * 100000);
                <?php re_render();?>
            }
        });
    }

    function reset_ol_nn() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "ol_nn"},
            success: function (data) {
                <?php re_create_switch('data[0]["show"]', "ol_nn_en", "ol_nn_en", "ol_nn_con", "ol_nn_en", "ol_nn_btn", "yes");?>
                sli_Onfs.setValue(data[0]["font/size"] - 10);
                var rect_color = hex2rgba(data[0]["rect-color"]);
                var font_color = hex2rgba(data[0]["font/color"]);
                layui.use('colorpicker', function () {
                    var colorpicker = layui.colorpicker;
                    <?php create_colorpickers('onrc', 'rect_color')?>
                    <?php create_colorpickers('onfc', 'font_color')?>
                });
                $("#onrc-form-input").val(data[0]["rect-color"]);
                $("#onfc-form-input").val(data[0]["font/color"]);
                $("#n_ttf").val(data[0]["font/font-file"]);
                <?php re_render();?>
            },
        });
    }

    function reset_ol_datetime() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "ol_datetime"},
            success: function (data) {
                <?php re_create_switch('data[0]["enabled"]', "ol_datetime", "ol_datetime_en", "ol_datetime", "ol_datetime", "ol_datetime_btn", "yes");?>
                $("#otlc").val(data[0]["position"]);
                sli_Otfs.setValue(data[0]["font/size"] - 24);
                var bg_color = hex2rgba(data[0]["font/background-color"]);
                var font_color = hex2rgba(data[0]["font/color"]);
                layui.use('colorpicker', function () {
                    var colorpicker = layui.colorpicker;
                    <?php create_colorpickers('otfbc', 'bg_color')?>
                    <?php create_colorpickers('otfc', 'font_color')?>
                });
                $("#otfbc-form-input").val(data[0]["font/background-color"]);
                $("#otfc-form-input").val(data[0]["font/color"]);
                if (!data[0]["font/font-file"]) {
                    $("#t_ttf").val("decker.ttf");
                } else {
                    $("#t_ttf").val(data[0]["font/font-file"]);
                }
                <?php re_render();?>
            }
        });
    }

    function reset_ol_pts() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "ol_pts"},
            success: function (data) {
                <?php re_create_switch('data[0]["enabled"]', "ol_pts", "ol_pts_en", "ol_pts", "ol_pts", "ol_pts_btn", "yes");?>
                $("#oplc").val(data[0]["position"]);
                sli_Opfs.setValue(data[0]["font/size"] - 24);
                var bg_color = hex2rgba(data[0]["font/background-color"]);
                var font_color = hex2rgba(data[0]["font/color"]);
                layui.use('colorpicker', function () {
                    var colorpicker = layui.colorpicker;
                    <?php create_colorpickers('opfbc', 'bg_color')?>
                    <?php create_colorpickers('opfc', 'font_color')?>
                });
                $("#opfbc-form-input").val(data[0]["font/background-color"]);
                $("#opfc-form-input").val(data[0]["font/color"]);
                if (!data[0]["font/font-file"]) {
                    $("#t_ptf").val("decker.ttf");
                } else {
                    $("#t_ptf").val(data[0]["font/font-file"]);
                }
                <?php re_render();?>
            }
        });
    }

    $(document).ready(function () {
        $("#ol_wm_img_reset").click(function (e) {
            e.preventDefault();
            $.ajax({
                url: './load.php',
                type: 'post',
                dataType: 'json',
                data: {prop: "ol_wm_img", index: $("#ol_wm_img_cur_index").val() - 1},
                success: function (data) {
                    var prop = data[0];
                    $("#ol_wm_img_h").val(prop["position/height"]);
                    $("#ol_wm_img_w").val(prop["position/width"]);
                    $("#ol_wm_img_l").val(prop["position/left"]);
                    $("#ol_wm_img_t").val(prop["position/top"]);
                    $("#o_img").val(prop["file"]);
                    <?php re_render();?>
                }
            });
        });

        $("#ol_wm_txt_reset").click(function (e) {
            e.preventDefault();
            $.ajax({
                url: './load.php',
                type: 'post',
                dataType: 'json',
                data: {prop: "ol_wm_txt", index: $("#ol_wm_txt_cur_index").val() - 1},
                success: function (data) {
                    var prop = data[0];
                    $("#ol_wm_txt").val(prop["text"]);
                    $("#ol_wm_txt_t").val(prop["position/top"]);
                    $("#ol_wm_txt_l").val(prop["position/left"]);
                    sli_Owtfs.setValue(prop["font/size"] - 24);
                    var font_color = hex2rgba(prop["font/color"]);
                    layui.use('colorpicker', function () {
                        var colorpicker = layui.colorpicker;
                        <?php create_colorpickers('owtfc', 'font_color')?>
                    });
                    $("#owtfc-form-input").val(prop["font/color"]);
                    $("#o_txt").val(prop["font/font-file"]);
                    <?php re_render();?>
                }
            });
        });
    });

    function reset_vr() {
        $.ajax({
            url: './load.php',
            type: 'post',
            dataType: 'json',
            data: {prop: "recording"},
            success: function (data) {
                <?php re_create_switch('data["enabled"]', "s_en", "vr_en", "vr", "s_en", "vr_btn", "yes");?>
                $("#vr_cd").val(data["chunk-duration"]);
                $("#vr_loc").val(data["location"]);
                $("#vr_rs").val(data["reserved-space-size"]);
                <?php re_render();?>
            }
        });
    }

    //change color between hex and rgba
    var rgbToHex = function (rgb) {
        var hex = Number(rgb).toString(16);
        if (hex.length < 2) {
            hex = "0" + hex;
        }
        return hex;
    };
    var colorhex = function (r, g, b, a) {
        var red = rgbToHex(r);
        var green = rgbToHex(g);
        var blue = rgbToHex(b);
        var alpha = rgbToHex(a);
        return "0x" + red + green + blue + alpha;
    };
    var rgba2hex = function (color) {
        let rgba = color.split(new RegExp('[,()]', 'g'));
        var r = rgba[1];
        var g = rgba[2];
        var b = rgba[3];
        var a = Math.round(rgba[4] * 255);
        return colorhex(r, g, b, a);
    };
    var hex2rgba = function (hex) {
        if (hex === '') return 'rgba(0,0,0,0)';
        return 'rgba(' + parseInt('0x' + hex.slice(2, 4)) + ',' + parseInt('0x' + hex.slice(4, 6)) + ','
            + parseInt('0x' + hex.slice(6, 8)) + ',' + parseInt('0x' + hex.slice(8, 10)) + ')';
    };

    function isp_set_lc(prop, action=null) {
        var lc_val;
        if (action) {
            lc_val = action;
        } else {
            lc_val = prop[0]["compensation/action"];
        }
        if (lc_val == "disable") {
            $("#i_lc").hide();
        }else if (lc_val == "hlc") {
            sli_Ilc.setValue(prop[0]["compensation/hlc"]);
            $("#i_lc").show();
        } else if (lc_val == "blc") {
            sli_Ilc.setValue(prop[0]["compensation/blc"]);
            $("#i_lc").show();
        }
    }
    function isp_set_nr(prop, action=null) {
        var nr_val;
        if (action) {
            nr_val = action;
        } else {
            nr_val = prop[0]["nr/action"];
        }
        if (nr_val == "disable") {
            $("#i_nsl").hide();
            $("#i_ntl").hide();
        } else if (nr_val == "normal") {
            sli_Inrsl.setValue(prop[0]["nr/space-normal"]);
            $("#i_ntl").hide();
            $("#i_nsl").show();
        } else if (nr_val == "expert") {
            sli_Inrsl.setValue(prop[0]["nr/space-expert"]);
            sli_Inrtl.setValue(prop[0]["nr/time"]);
            $("#i_nsl").show();
            $("#i_ntl").show();
        }
    }
    function isp_set_value(prop) {
        sli_Ib.setValue(prop[0]["brightness"] - 0);
        sli_Ic.setValue(prop[0]["contrast"] - 0);
        sli_Ish.setValue(prop[0]["sharpness"] - 0);
        sli_Isa.setValue(prop[0]["saturation"] - 0);
        sli_Ih.setValue(prop[0]["hue"] - 0);
        sli_Iwb.setValue(prop[0]["whitebalance/cbgain"] - 28);
        sli_Iwr.setValue(prop[0]["whitebalance/crgain"] - 28);
        if (prop[0]["exposure/auto"] === "true") {
            sli_Iex.setValue(0);
        } else {
            sli_Iex.setValue(prop[0]["exposure/absolute"] - 1);
        }
        isp_set_lc(prop);
        isp_set_nr(prop);
    }

    layui.use(['element', 'layer', 'jquery', 'slider', 'form', 'colorpicker', 'upload'], function () {
        var element = layui.element
            , layer = layui.layer
            , $ = layui.$
            , slider = layui.slider
            , form = layui.form
            , colorpicker = layui.colorpicker
            , upload = layui.upload;


        element.on('collapse(ipc)', function (data) {
            var index = $(data.title).parents().index();
            var collapse = data.title.parents().parents();
            var colla_items = collapse.find('.layui-colla-item');
            var current_colla_item = data.title.parents();
            colla_items.each(function () {
                var othis = $(this);
                if (!othis.is(current_colla_item)) {
                    var icon = othis.find('.layui-colla-icon');
                    icon.html('&#xe602;');
                    icon.removeAttr('style');
                    var colla_contents = othis.find('.layui-colla-content');
                    colla_contents.each(function () {
                        $(this).removeClass('layui-show');
                    });
                }
            });
            switch (index) {
                case 0:
                    $("#video_channel_con").addClass("layui-show");

                    var codecList = ["h264", "h265"];
                    var framerateList1 = ["24", "25", "30"];
                    var framerateList2 = ["24", "25"];
                    var resoList = ["1280x720", "1920x1080"];
                    var resoList_4k = ["1280x720", "1920x1080", "3840x2160"];
                    var sub_resoList = ["704x576", "640x480", "352x288"];
                    var sub_resoList_dis = ["704x576(D1)", "640x480(VGA)", "352x288(CIF)"];

                    sli_V_ts_mb = <?php create_slider("slide_v_ts_m_bitrate", 500, 12000, 500, "ts_mb");?>;
                    sli_V_ts_mg = <?php create_slider("slide_v_ts_m_gop", 1, 100, 1, "ts_mg");?>;
                    sli_V_ts_sb = <?php create_slider("slide_v_ts_s_bitrate", 500, 12000, 500, "ts_sb");?>;
                    sli_V_ts_sg = <?php create_slider("slide_v_ts_s_gop", 1, 100, 1, "ts_sg");?>;
                    sli_V_vr_b = <?php create_slider("slide_v_vr_bitrate", 500, 12000, 500, "vr_b");?>;
                    sli_V_vr_g = <?php create_slider("slide_v_vr_gop", 1, 100, 1, "vr_g");?>;

                    var multi_en_show = function () {
                        $("#video_mu_en").empty().append('<input type="checkbox"  lay-skin="switch" checked lay-filter="v_mu_en" value="true" name="v_mu_en" id="v_mu_en">');
                        $("#video_vr").show();
                    };
                    var multi_en_hide = function () {
                        $("#video_mu_en").empty().append('<input type="checkbox"  lay-skin="switch" lay-filter="v_mu_en" value="false" name="v_mu_en" id="v_mu_en">');
                        $("#video_vr").hide();
                    };

                    var multi_dis = function (flag) {
                        if (flag === "true") {
                            multi_en_show();
                        } else {
                            multi_en_hide();
                        }
                        $("#video_multi_area").show();
                    };

                    var multi_4k_dis = function () {
                        $("#video_multi_area").hide();
                        $("#video_vr").hide();
                    };

                    //show multi-channel-btn
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "video"},
                        success: function (data) {
                            document.cookie = "v_mu_en" + "=" + data[0]["multi-channel"];
                            if (data[1] === "3840x2160") {
                                multi_4k_dis();
                            } else {
                                multi_dis(data[0]["multi-channel"]);
                            }
                            form.render();
                        },
                    });

                    element.on('collapse(video_channel)', function (data) {
                        var index = $(data.title).parents().index();
                        switch (index) {
                            case 0:
                                $.ajax({
                                    url: './load.php',
                                    type: 'post',
                                    dataType: 'json',
                                    data: {prop: "video/ts/main"},
                                    success: function (data) {
                                        sli_V_ts_mb.setValue((data[0]["bitrate"] - 500) / 500);
                                        sli_V_ts_mg.setValue((data[0]["gop"] - 0) + 1);
                                        <?php create_select("data[1]", 'data[0]["device"]', "v_ts_m_device")?>
                                        if (data[2] == "60") {
                                            <?php create_select("framerateList1", 'data[0]["framerate"]', "v_ts_m_framerate")?>
                                        } else {
                                            <?php create_select("framerateList2", 'data[0]["framerate"]', "v_ts_m_framerate")?>
                                        }
                                        <?php create_select("codecList", 'data[0]["codec"]', "v_ts_m_codec")?>
                                        if (data[3] == "true") {
                                            <?php create_select("resoList_4k", 'data[0]["resolution"]', "v_ts_m_reso")?>
                                        } else {
                                            <?php create_select("resoList", 'data[0]["resolution"]', "v_ts_m_reso")?>
                                        }
                                        form.render();
                                    }
                                });
                                break;
                            case 1:
                            <?php create_switch("v_ts_s_en", "v_ts_s_prop")?>
                                $.ajax({
                                    url: './load.php',
                                    type: 'post',
                                    dataType: 'json',
                                    data: {prop: "video/ts/sub"},
                                    success: function (data) {
                                        <?php create_judge_switch('data[0]["enabled"]', "v_ts_s_en", "v_ts_s_en", "v_ts_s_prop", "v_ts_s_btn")?>
                                        sli_V_ts_sb.setValue((data[0]["bitrate"] - 500) / 500);
                                        sli_V_ts_sg.setValue((data[0]["gop"] - 0) + 1);
                                        <?php create_select("data[1]", 'data[0]["device"]', "v_ts_s_device")?>
                                        <?php create_select("codecList", 'data[0]["codec"]', "v_ts_s_codec")?>
                                        if (data[2] == "60") {
                                            <?php create_select("framerateList1", 'data[0]["framerate"]', "v_ts_s_framerate")?>
                                        } else {
                                            <?php create_select("framerateList2", 'data[0]["framerate"]', "v_ts_s_framerate")?>
                                        }
                                        <?php create_select("sub_resoList", 'data[0]["resolution"]', "v_ts_s_reso", "sub_resoList_dis")?>
                                        form.render();
                                    }
                                });
                                break;
                            case 2:
                            <?php create_switch("v_ts_g_en", "v_ts_g_prop")?>
                                $.ajax({
                                    url: './load.php',
                                    type: 'post',
                                    dataType: 'json',
                                    data: {prop: "video/ts/gdc"},
                                    success: function (data) {
                                        var conf = "";
                                        if (data[0]["config-file"] != null) {
                                            conf = data[0]["config-file"].split("/");
                                        }
                                        <?php create_select("data[1]", 'conf[3]', "v_ts_g_conf")?>
                                        $("#v_ts_g_in_reso").prop("value", data[0]["input-resolution"]);
                                        $("#v_ts_g_out_reso").prop("value", data[0]["output-resolution"]);
                                        form.on('select(v_ts_g_cong)', function (data) {
                                            var conf = data.value;
                                            var anal_in = conf.split('-');
                                            var in_reso = anal_in[1].split('_')[0] + "x" + anal_in[1].split('_')[1];
                                            var out_reso = anal_in[2].split('_')[2] + "x" + anal_in[2].split('_')[3];
                                            $("#v_ts_g_in_reso").val(in_reso);
                                            $("#v_ts_g_out_reso").val(out_reso);
                                        });
                                        form.render('select');
                                        <?php create_judge_switch('data[0]["enabled"]', "v_ts_g_en", "v_ts_g_en", "v_ts_g_prop", "v_ts_g_btn")?>
                                        form.render();
                                    }
                                });
                                break;
                            case 3:
                            <?php create_switch("v_vr_g_en", "v_vr_g_prop")?>
                                $.ajax({
                                    url: './load.php',
                                    type: 'post',
                                    dataType: 'json',
                                    data: {prop: "video/vr"},
                                    success: function (data) {
                                        sli_V_vr_b.setValue((data[0]["bitrate"] - 500) / 500);
                                        sli_V_vr_g.setValue((data[0]["gop"] - 0) + 1);
                                        <?php create_select("data[1]", 'data[0]["device"]', "v_vr_device")?>
                                        <?php create_select("codecList", 'data[0]["codec"]', "v_vr_codec")?>
                                        if (data[3] == "60") {
                                            <?php create_select("framerateList1", 'data[0]["framerate"]', "v_vr_framerate")?>
                                        } else {
                                            <?php create_select("framerateList2", 'data[0]["framerate"]', "v_vr_framerate")?>
                                        }
                                        if (data[4] == "true") {
                                            <?php create_select("resoList_4k", 'data[0]["resolution"]', "v_vr_reso")?>
                                        } else {
                                            <?php create_select("resoList", 'data[0]["resolution"]', "v_vr_reso")?>
                                        }
                                        var conf = "";
                                        if (data[0]["gdc/config-file"] != null) {
                                            conf = data[0]["gdc/config-file"].split("/");
                                        }
                                        <?php create_select("data[2]", 'conf[3]', "v_vr_g_conf")?>
                                        $("#v_vr_g_in_reso").prop("value", data[0]["gdc/input-resolution"]);
                                        $("#v_vr_g_out_reso").prop("value", data[0]["gdc/output-resolution"]);
                                        form.on('select(v_vr_g_cong)', function (data) {
                                            var conf = data.value;
                                            var anal_in = conf.split('-');
                                            var in_reso = anal_in[1].split('_')[0] + "x" + anal_in[1].split('_')[1];
                                            var out_reso = anal_in[2].split('_')[2] + "x" + anal_in[2].split('_')[3];
                                            $("#v_vr_g_in_reso").val(in_reso);
                                            $("#v_vr_g_out_reso").val(out_reso);
                                        });
                                        form.render('select');
                                        <?php create_judge_switch('data[0]["gdc/enabled"]', "v_vr_g_en", "v_vr_g_en", "v_vr_g_prop", "v_vr_g_btn")?>
                                        form.render();
                                    }
                                });
                                break;
                        }
                    });

                    form.on('switch(v_mu_en)', function (data) {
                        if (data.elem.checked) {
                            document.cookie = "v_mu_en" + "=true";
                            $("#video_vr").show();
                        } else {
                            document.cookie = "v_mu_en" + "=false";
                            $("#video_vr").hide();
                        }
                        form.render();
                    });

                    $(document).ready(function () {
                        $("#v_ts_m_btn").click(function (e) {
                            e.preventDefault();
                            $.ajax({
                                type: "post",
                                dataType: "json",
                                url: "ipc/video.php",
                                data: {
                                    id: "main",
                                    v_ts_m_device: $("#v_ts_m_device").val(),
                                    v_ts_m_codec: $("#v_ts_m_codec").val(),
                                    v_ts_m_framerate: $("#v_ts_m_framerate").val(),
                                    v_ts_m_reso: $("#v_ts_m_reso").val()
                                },
                                beforeSend: function (xhr) {
                                    xhr.withCredentials = true;
                                },
                                success: function (result) {
                                    if (result[0] === "3840x2160") {
                                        multi_4k_dis();
                                    } else {
                                        multi_dis(result[1]);
                                    }
                                    form.render(null, "video");
                                },
                                error: function (result) {
                                    alert('error');
                                }
                            });
                        });
                    });
                    break;
                case 1:
                    var codecList = ["g711", "g726", "aac"];
                    var bitrateList = [['64'], ["16", "24", "32", "40"], ["16", "24", "32", "40", "64", "96", "128", "160", "192", "256", "320"]];
                    var codecSel = document.querySelector("#selCodec");
                    var bitSel = document.querySelector("#bitrateId");
                    var samplerate;
                    var samplerateList = ["7.35", "8", "11.025", "12", "16", "22.05", "24", "32", "44.1", "48", "64", "88.2", "96",];
                <?php create_switch("a_en", "au_prop")?>
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "audio"},
                        success: function (data) {
                            <?php create_judge_switch('data["enabled"]', "au_en", "a_en", "au_prop", "au_btn")?>
                            $("#au_dev").val(data["device"]);
                            $("#au_dev_opt").val(data["device-options"]);
                            form.render();
                            if (data["codec"] == 'mulaw') data["codec"] = 'g711';
                            $("#selCodec").empty();
                            $("#bitrateId").empty();
                            for (var i = 0; i < codecList.length; i++) {
                                var codecOpt = new Option(codecList[i]);
                                codecSel.options.add(codecOpt);
                                if (codecList[i] == data["codec"]) {
                                    codecSel.options.selectedIndex = i;
                                    for (var j = 0; j < bitrateList[i].length; j++) {
                                        var b_opt = new Option(bitrateList[i][j]);
                                        bitSel.options.add(b_opt);
                                        if (bitrateList[i][j] == data["bitrate"]) {
                                            bitSel.options.selectedIndex = j;
                                        }
                                    }
                                }
                            }
                            form.render('select');
                            form.on('select(acodec)', function (data) {
                                var codec = data.value;
                                if (codec == "g711") {
                                    bitSel.options.length = 0;
                                    var bitrateOpt = new Option(bitrateList[0][0]);
                                    bitSel.options.add(bitrateOpt);
                                } else if (codec == "g726") {
                                    bitSel.options.length = 0;
                                    for (var i = 0; i < bitrateList[1].length; i++) {
                                        var bitrateOpt = new Option(bitrateList[1][i]);
                                        bitSel.options.add(bitrateOpt);
                                    }
                                } else if (codec == "aac") {
                                    bitSel.options.length = 0;
                                    for (var j = 0; j < bitrateList[2].length; j++) {
                                        var bitrateOpt = new Option(bitrateList[2][j]);
                                        bitSel.options.add(bitrateOpt);
                                    }
                                }
                                form.render('select');
                            });
                            $("#au_channels").prop("placeholder", data["channels"]);
                            $("#au_format").prop("placeholder", data["format"]);
                            $("#au_samplerate").empty();
                            for (var i = 0; i < samplerateList.length; i++) {
                                samplerate = (samplerateList[i] * 1000).toString();
                                if (samplerate === data["samplerate"]) {
                                    $("#au_samplerate").append('<option value="' + samplerateList[i] * 1000 + '" selected>' + samplerateList[i] + '</option>');
                                } else {
                                    $("#au_samplerate").append('<option value="' + samplerateList[i] * 1000 + '">' + samplerateList[i] + '</option>');
                                }
                            }
                            form.render('select');
                        },
                    });
                    break;
                case 2:
                <?php create_switch("b_en", "bc_prop")?>
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "backchannel"},
                        success: function (data) {
                            <?php create_judge_switch('data["enabled"]', "bc_en", "b_en", "bc_prop", "bc_btn")?>
                            $("#bc_dev").val(data["device"]);
                            $("#bc_encoding").val(data["encoding"]);
                            form.render();
                        },
                    });
                    break;
                case 3:
                    sli_Icq = <?php create_slider("slideIcap", 10, 100, 5, "quality")?>;
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "img_cap"},
                        success: function (data) {
                            sli_Icq.setValue((data["quality"] - 10) / 5);
                            form.render();
                        },
                    });
                    break;
                case 4:
                    sli_Ib = <?php create_slider("slideBright", 0, 255, 1, "brightness")?>;
                    sli_Ic = <?php create_slider("slideContrast", 0, 255, 1, "contrast")?>;
                    sli_Ish = <?php create_slider("slideSharp", 0, 255, 1, "sharpness")?>;
                    sli_Isa = <?php create_slider("slideSat", 0, 255, 1, "saturation")?>;
                    sli_Ih = <?php create_slider("slideHue", 0, 360, 1, "hue")?>;
                    sli_Iwb = <?php create_slider("slideWhcb", 28, 228, 1, "whitebalance/cbgain")?>;
                    sli_Iwr = <?php create_slider("slideWhcr", 28, 228, 1, "whitebalance/crgain")?>;
                    sli_Ilc = <?php create_slider("slideLclev", 0, 128, 1, "compensation")?>;
                    sli_Inrsl = <?php create_slider("slideNrslev", 0, 255, 1, "nr/space")?>;
                    sli_Inrtl = <?php create_slider("slideNrtlev", 0, 255, 1, "nr/time")?>;

                <?php create_switch("i_exp", "i_exab")?>
                <?php create_switch("i_ircut", "none")?>
                <?php create_switch("i_wdr", "none")?>
                <?php create_switch("i_whau", "i_wh")?>

                    var v_fr = <?php echo ipc_property('get', '/ipc/video/ts/main/framerate')?>;

                    var vmList = ["50", "60"];
                    var vmList_dis = ["50(PAL)", "60(NTSC)"];
                    var mirList = ["NONE", "H", "V", "HV"];
                    var mirList_dis = ['<?php echo gettext("disable")?>', '<?php echo gettext("left and right")?>', '<?php echo gettext("up and down")?>', '<?php echo gettext("central")?>'];
                    var lcList = ["disable", "hlc", "blc"];
                    var lcList_dis = ['<?php echo gettext("disable")?>', '<?php echo gettext("high light")?>', '<?php echo gettext("back light")?>'];
                    var rotateList = ["0", "90", "180", "270"];
                    var rotateList_display = [
                        '<?php echo gettext("disable") ?>',
                        '90 <?php echo gettext("degrees") ?>',
                        '180 <?php echo gettext("degrees") ?>',
                        '270 <?php echo gettext("degrees") ?>',
                    ];

                    var nrList = ['disable', 'normal', 'expert'];
                    var nrList_dis = ['<?php echo gettext("disable")?>', '<?php echo gettext("normal")?>', '<?php echo gettext("expert")?>'];
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "isp"},
                        success: function (data) {
                            <?php create_judge_switch('data[0]["exposure/auto"]', "i_exau", "i_exp", "i_exab", "exau_btn")?>
                            <?php create_judge_switch('data[0]["ircut"]', "i_ircut", "i_ircut", "none", "ir_btn")?>
                            <?php create_judge_switch('data[0]["wdr/enabled"]', "i_wdr", "i_wdr", "none", "wdr_btn")?>
                            <?php create_judge_switch('data[0]["whitebalance/auto"]', "i_whau", "i_whau", "i_wh", "wh_btn")?>
                            <?php create_select("vmList", 'data[0]["anti-banding"]', "isp_video_mode_sel", "vmList_dis")?>
                            <?php create_select("mirList", 'data[0]["mirroring"]', "isp_mirror_sel", "mirList_dis")?>
                            <?php create_select("lcList", 'data[0]["compensation/action"]', "isp_lc_sel", "lcList_dis")?>
                            <?php create_select("rotateList", 'data[0]["rotation"]', "isp_rotation_sel", "rotateList_display")?>
                            <?php create_select("nrList", 'data[0]["nr/action"]', "isp_nr_sel", "nrList_dis")?>

                            sli_Iex = slider.render({
                                elem: '#slideExab'
                                , min: 1
                                , max: 1000 / v_fr
                                , step: 1
                                , setTips: function (value) {
                                    document.cookie = "exab" + "=" + value;
                                    return value;
                                }
                                , change: function (value) {
                                    document.cookie = "exab" + "=" + value;
                                }
                            });

                            if (data[0]["exposure/auto"] === "true") {
                                sli_Iex.setValue(0);
                            } else {
                                sli_Iex.setValue(data[0]["exposure/absolute"] - 1);
                            }
                            var exab = 0;
                            var cur_val = data[0]['exposure/absolute'];
                            form.on('select(vfrate)', function (data) {
                                var frate = data.value;
                                exab = 1000 / frate;
                                slider.render({
                                    elem: '#slideExab'
                                    , value: cur_val
                                    , min: 1
                                    , max: exab
                                    , step: 1
                                    , change: function (value) {
                                        document.cookie = "exab" + "=" + value;
                                    }
                                });
                            });
                            isp_set_value(data);

                            form.render();
                        },
                    });

                    form.on('select(isplc)', function(data){
                        var sel_val = data.value;
                        $.ajax({
                            url: './load.php',
                            type: 'post',
                            dataType: 'json',
                            data: {prop: "isp"},
                            success: function (data) {
                                sli_Ilc.setValue(0);
                                isp_set_lc(data, sel_val);
                            }
                        });
                        form.render();
                    });
                    form.on('select(ispnr)', function(data){
                        var sel_val = data.value;
                        $.ajax({
                            url: './load.php',
                            type: 'post',
                            dataType: 'json',
                            data: {prop: "isp"},
                            success: function (data) {
                                sli_Inrsl.setValue(0);
                                sli_Inrtl.setValue(data[0]["nr/time"]);
                                isp_set_nr(data, sel_val);
                            }
                        });
                        form.render();
                    });
                    break;
                case 5:
                <?php create_switch("nn_en", "nn_con")?>
                    var modList = ["mtcnn-v1", "yoloface-v3", "aml_face_detection", "disable"];
                    var modList_dis = ["mtcnn", "yoloface", "aml_face_detection", "disable"];
                    var recogList = ["facenet", "aml_face_recognition", "disable"];
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "nn"},
                        success: function (data) {
                            <?php create_select("modList", 'data["detection/model"]', "nn_det_mod_sel", "modList_dis")?>
                            <?php create_select("recogList", 'data["recognition/model"]', "nn_recog_mod_sel")?>
                            <?php create_judge_switch('data["enabled"]', "nn_en", "nn_en", "nn_con", "nn_btn")?>
                            $("#nn_recog_info").val(data["recognition/info-string"]);
                            sli_Nft = slider.render({
                                elem: '#slideNth'
                                , value: data["recognition/threshold"].slice(0, 7) * 100000
                                , min: 0
                                , max: 150000
                                , step: 1
                                , setTips: function (value) {
                                    document.cookie = "n_th" + "=" + value / 100000;
                                    return (value / 100000);
                                }
                                , change: function (value) {
                                    document.cookie = "n_th" + "=" + value;
                                }
                            });
                            form.on('select(nnrecogmd)', function (data) {
                                var recog_mod = data.value;
                                if (recog_mod === "facenet") {
                                    sli_Nft.setValue(0.6 * 100000);
                                } else {
                                    sli_Nft.setValue(1.0 * 100000);
                                }
                            });
                            form.render();
                        },
                    });
                    break;
                case 6:
                    element.on('collapse(ipc_ol)', function (data) {
                        var index = $(data.title).parents().index();
                        var posList = ["top-left", "top-mid", "top-right", "mid-left", "center", "mid-right",  "bot-left", "bot-mid", "bot-right"];
                        var posList_dis = ['<?php echo gettext("top left")?>', '<?php echo gettext("top middle")?>', '<?php echo gettext("top right")?>',
                                            '<?php echo gettext("middle left")?>', '<?php echo gettext("center")?>', '<?php echo gettext("middle right")?>',
                                            '<?php echo gettext("bottom left")?>', '<?php echo gettext("bottom middle")?>', '<?php echo gettext("bottom right")?>'];
                        switch (index) {
                            case 0:
                            <?php create_switch("ol_nn_en", "ol_nn_con")?>
                                sli_Onfs = <?php create_slider("slideNfs", 10, 48, 1, "font/size")?>;
                                $("#ol_nn_set").off('click');
                                $("#n_ff").off('click');
                                $("#n_ff").off('change');
                                $("#n_ff").data('haveEvents', false);
                            <?php create_upload("n_ff", "n_ttf", "ol_nn_set")?>
                                $.ajax({
                                    url: './load.php',
                                    type: 'post',
                                    dataType: 'json',
                                    data: {prop: "ol_nn"},
                                    success: function (data) {
                                        <?php create_judge_switch('data[0]["show"]', "ol_nn_en", "ol_nn_en", "ol_nn_con", "ol_nn_btn")?>
                                        <?php create_select('data[1]', 'data[0]["font/font-file"]', 'n_ttf')?>
                                        if (!data[0]["font/font-file"]) {
                                            $("#n_ttf").val("decker.ttf");
                                        }
                                        sli_Onfs.setValue(data[0]["font/size"] - 10);
                                        var rect_color = hex2rgba(data[0]["rect-color"]);
                                        var font_color = hex2rgba(data[0]["font/color"]);
                                        <?php create_colorpickers('onrc', 'rect_color')?>
                                        <?php create_colorpickers('onfc', 'font_color')?>
                                        $("#onrc-form-input").val(data[0]["rect-color"]);
                                        $("#onfc-form-input").val(data[0]["font/color"]);
                                        form.render();
                                    },
                                });
                                break;
                            case 1:
                            <?php create_switch("ol_datetime", "ol_datetime")?>
                                sli_Otfs = <?php create_slider("slideTfs", 24, 64, 1, "datetime/font/size")?>;
                                $("#ol_datetime_set").off('click');
                                $("#t_f").off('click');
                                $("#t_f").off('change');
                                $("#t_f").data('haveEvents', false);
                            <?php create_upload("t_f", "t_ttf", "ol_datetime_set")?>

                                $.ajax({
                                    url: './load.php',
                                    type: 'post',
                                    dataType: 'json',
                                    data: {prop: "ol_datetime"},
                                    success: function (data) {
                                        <?php create_judge_switch('data[0]["enabled"]', "ol_datetime_en", "ol_datetime", "ol_datetime", "ol_datetime_btn")?>
                                        <?php create_select("posList", 'data[0]["position"]', "otlc", "posList_dis")?>
                                        <?php create_select('data[1]', 'data[0]["font/font-file"]', 't_ttf')?>
                                        if (!data[0]["font/font-file"]) {
                                            $("#t_ttf").val("decker.ttf");
                                        }
                                        sli_Otfs.setValue(data[0]["font/size"] - 24);
                                        var ol_fbc = hex2rgba(data[0]["font/background-color"]);
                                        var ol_fc = hex2rgba(data[0]["font/color"]);
                                        <?php create_colorpickers('otfbc', 'ol_fbc')?>
                                        <?php create_colorpickers('otfc', 'ol_fc')?>
                                        $("#otfbc-form-input").val(data[0]["font/background-color"]);
                                        $("#otfc-form-input").val(data[0]["font/color"]);
                                        form.render();
                                    }
                                });
                                break;
                            case 2:
                            <?php create_switch("ol_pts", "ol_pts")?>
                                sli_Opfs = <?php create_slider("slidePfs", 24, 64, 1, "pts/font/size")?>;
                                $("#ol_pts_set").off('click');
                                $("#p_f").off('click');
                                $("#p_f").off('change');
                                $("#p_f").data('haveEvents', false);
                            <?php create_upload("p_f", "t_ptf", "ol_pts_set")?>

                                $.ajax({
                                    url: './load.php',
                                    type: 'post',
                                    dataType: 'json',
                                    data: {prop: "ol_pts"},
                                    success: function (data) {
                                        <?php create_judge_switch('data[0]["enabled"]', "ol_pts_en", "ol_pts", "ol_pts", "ol_pts_btn")?>
                                        <?php create_select("posList", 'data[0]["position"]', "oplc", "posList_dis")?>
                                        <?php create_select('data[1]', 'data[0]["font/font-file"]', 't_ptf')?>
                                        if (!data[0]["font/font-file"]) {
                                            $("#t_ptf").val("decker.ttf");
                                        }
                                        sli_Opfs.setValue(data[0]["font/size"] - 24);
                                        var ol_fbc = hex2rgba(data[0]["font/background-color"]);
                                        var ol_fc = hex2rgba(data[0]["font/color"]);
                                        <?php create_colorpickers('opfbc', 'ol_fbc')?>
                                        <?php create_colorpickers('opfc', 'ol_fc')?>
                                        $("#opfbc-form-input").val(data[0]["font/background-color"]);
                                        $("#opfc-form-input").val(data[0]["font/color"]);
                                        form.render();
                                    }
                                });
                                break;
                            case 3:
                                element.on('collapse(ipc_ol_wm)', function (data) {
                                    var index = $(data.title).parents().index();
                                    switch (index) {
                                        case 1:
                                            $("#ol_wm_img_set").off('click');
                                            $("#w_img").off('click');
                                            $("#w_img").off('change');
                                            $("#w_img").data('haveEvents', false);
                                            var fileName;
                                            upload.render({
                                                elem: '#w_img'
                                                , url: './ipc/up_img.php'
                                                , accept: 'images'
                                                , exts: 'jpg|jpeg|JPG|JPEG|png|PNG'
                                                , auto: false
                                                , bindAction: '#ol_wm_img_set'
                                                , choose: function (obj) {
                                                    var files = obj.pushFile();
                                                    obj.preview(function (index, file, result) {
                                                        fileName = file.name;
                                                        $("#o_img").prepend('<option value=\"' + file.name + '\" selected>' + file.name + '</option>');
                                                        form.render();
                                                    });
                                                }
                                            });
                                            $.ajax({
                                                url: './load.php',
                                                type: 'post',
                                                dataType: 'json',
                                                data: {prop: "ol_wm_img", index: 0},
                                                success: function (data) {
                                                    var prop = data[0];
                                                    <?php create_select('data[1]', 'prop["file"]', 'o_img')?>
                                                    <?php create_select('data[2]', 0, 'ol_wm_img_cur_index')?>
                                                    ol_wm_img_set_value(prop);
                                                },
                                            });

                                            function ol_wm_img_set_value(prop) {
                                                $("#o_img").val(prop["file"]);
                                                $("#ol_wm_img_h").val(prop["position/height"]);
                                                $("#ol_wm_img_w").val(prop["position/width"]);
                                                $("#ol_wm_img_l").val(prop["position/left"]);
                                                $("#ol_wm_img_t").val(prop["position/top"]);
                                                form.render();
                                            }

                                            form.on('select(ol_wm_img_cur_index)', function (data) {
                                                $.ajax({
                                                    url: './load.php',
                                                    type: 'post',
                                                    dataType: 'json',
                                                    data: {prop: "ol_wm_img", index: data.value - 1},
                                                    success: function (data) {
                                                        ol_wm_img_set_value(data[0]);
                                                    },
                                                });
                                            });
                                            break;
                                        case 0:
                                            sli_Owtfs = <?php create_slider("slideWfs", 24, 64, 1, "watermark/text/font/size")?>;
                                            $("#ol_wm_txt_set").off('click');
                                            $("#w_txt").off('click');
                                            $("#w_txt").off('change');
                                            $("#w_txt").data('haveEvents', false);
                                            <?php create_upload("w_txt", "o_txt", "ol_wm_txt_set")?>
                                            $.ajax({
                                                url: './load.php',
                                                type: 'post',
                                                dataType: 'json',
                                                data: {prop: "ol_wm_txt", index: 0},
                                                success: function (data) {
                                                    var prop = data[0];
                                                    <?php create_select('data[1]', 'prop["font/font-file"]', 'o_txt')?>
                                                    <?php create_select('data[2]', 0, 'ol_wm_txt_cur_index')?>
                                                    if (!prop["font/font-file"]) {
                                                        $("#o_txt").val("decker.ttf");
                                                    }
                                                    ol_wm_txt_set_value(prop);
                                                }
                                            });

                                            function ol_wm_txt_set_value(prop) {
                                                sli_Owtfs.setValue(prop["font/size"] - 24);
                                                $("#ol_wm_txt_l").val(prop["position/left"]);
                                                $("#ol_wm_txt_t").val(prop["position/top"]);
                                                var olwt_fc = hex2rgba(prop["font/color"]);
                                                <?php create_colorpickers("owtfc", 'olwt_fc')?>
                                                $("#owtfc-form-input").val(prop["font/color"]);
                                                $("#ol_wm_txt").val(prop["text"]);
                                                $("#o_txt").val(prop["font/font-file"]);
                                                form.render();
                                            }

                                            form.on('select(ol_wm_txt_cur_index)', function (data) {
                                                $.ajax({
                                                    url: './load.php',
                                                    type: 'post',
                                                    dataType: 'json',
                                                    data: {prop: "ol_wm_txt", index: data.value - 1},
                                                    success: function (data) {
                                                        ol_wm_txt_set_value(data[0]);
                                                    }
                                                });
                                            });
                                            break;
                                    }
                                });
                                break;
                        }
                    });
                    break;
                case 7:
                <?php create_switch("s_en", "vr")?>
                    $.ajax({
                        url: './load.php',
                        type: 'post',
                        dataType: 'json',
                        data: {prop: "recording"},
                        success: function (data) {
                            <?php create_judge_switch('data["enabled"]', "vr_en", "s_en", "vr", "vr_btn")?>
                            $("#vr_cd").val(data["chunk-duration"]);
                            $("#vr_loc").val(data["location"]);
                            $("#vr_rs").val(data["reserved-space-size"]);
                            form.render();
                        }
                    });
                    break;
            }
        });

    });

</script>

</body>

</html>
