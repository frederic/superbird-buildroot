"use strict";

layui.use(['element', 'layer', 'jquery', 'form', 'slider'], function () {
    let layer = layui.layer
        , form = layui.form
        , slider = layui.slider
        , $ = layui.$
    ;
    $(document).ready(function () {
        let offset = 0, timer = 0;
        let log_updating = false;
        let interval = getCookie("log-refresh-interval") || 1000;
        let slider_interval = slider.render({
            elem: '#interval'
            , value: interval / 1000
            , min: 1
            , max: 60
            , step: 1
            , input: true
            , setTips: function (value) {
                interval = value * 1000;
                setCookie("log-refresh-interval", interval, 7);
                return value;
            },
            change: function (value) {
                interval = value * 1000;
                stopInterval();
                startInterval();
                setCookie("log-refresh-interval", interval, 7);
            },
        });
        let intervalHandler = function () {
            if (log_updating) {
                return;
            }
            log_updating = true;
            $.ajax({
                url: '/log/log.action.php',
                type: 'POST',
                dataType: 'json',
                data: {action: 'get-log', offset: offset},
                async: true,
                success: function (ret) {
                    if (ret === 500) {
                        stopInterval();
                    }
                    if (ret.length > 0) {
                        let format_data = ret.data.replace(/\n/g, '<br />');
                        format_data = format_data.replace(/ERR/g, '<error>ERR</error>');
                        format_data = format_data.replace(/WARN/g, '<warn>WARN</warn>');
                        format_data = format_data.replace(/INFO/g, '<info>INFO</info>');
                        format_data = format_data.replace(/DEBUG/g, '<debug>DEBUG</debug>');
                        format_data = format_data.replace(/VERBOSE/g, '<verbose>VERBOSE</verbose>');
                        $('#log').append(format_data);
                        offset = ret.offset;
                        let e = document.getElementById('div_log');
                        e.scrollTop = e.scrollHeight;
                    }
                }, error: function (ret) {
                    $('#log').append(ret.error);
                    return false;
                }, complete: function () {
                    log_updating = false;
                }
            });
        };
        let startInterval = function() {
            $('#loading').show();
            intervalHandler(); // load on startup
            if (timer === 0) {
                timer = setInterval(intervalHandler, interval);
            }
        };
        let stopInterval = function() {
            $('#loading').hide();
            if (timer) {
                clearInterval(timer);
                timer = 0;
            }
        };

        startInterval();

        function clear_click (e, id) {
            let element = document.getElementById('log');
            element.innerHTML = '';
            startInterval();
        }

        function reload_click (e, id){
            stopInterval();
            let element = document.getElementById('log');
            element.innerHTML = '';
            offset = 0;
            startInterval();
        }

        function pause_click (e, id){
            let element = document.getElementById(id);
            let value = element.getAttribute('value');
            if (value === 'resume') {
                element.setAttribute('value', 'pause');
                element.innerText = gettext("Pause");
                startInterval();
            } else {
                element.setAttribute('value', 'resume');
                element.innerText = gettext("Resume");
                stopInterval();
            }
        }

        update_log_size();

        function update_log_size() {
            $.ajax({
                url: '/log/log.action.php',
                type: 'POST',
                dataType: 'json',
                data: {action: 'get-logsize'},
                async: true,
                success: function (ret) {
                    if (ret.debug > 1) {
                        document.getElementById('debug-log-filesize').innerText = formatSizeUnits(ret.debug);
                    } else {
                        document.getElementById('debug-log-filesize').innerText = "";
                    }
                    if (ret.trace > 1) {
                        document.getElementById('trace-log-filesize').innerText = formatSizeUnits(ret.trace);
                    } else {
                        document.getElementById('trace-log-filesize').innerText = "";
                    }
                }
            });
            setTimeout(update_log_size, 1);
        }

        function get_log_level(type) {
            let chk = document.getElementById(type + '-cust-check'),
                level = '';
            if (chk.checked) {
                level = document.getElementById(type + '-cust-level').value;
                if (level === '') {
                    level = '.*:0';
                }
                return level;
            }
            let l = log_levels[type],
                default_level = '',
                ld = l['*DEFAULT*'];
            ld = ld == 8 ? 1 : ld; // set default to ERROR
            for (let id in l) {
                if (id === '*DEFAULT*') {
                    default_level += '.*:' + ld;
                } else if (l[id] > 0 && l[id] < 8 && l[id] != ld) {
                    level += '^' + id + '$:' + l[id] + ',';
                }
            }
            if (default_level !== '') {
                level += default_level;
            }
            return level;
        }

        function start_stop_log (id) {
            let s = id.split('-'),
                level = '.*:0';
            let action = s[0],
                type = s[1];
            if (action === 'start') {
                level = get_log_level(type);
            }
            $.ajax({
                url: '/log/log.action.php',
                type: 'POST',
                dataType: 'json',
                data: {action: action + '-log', type: type, level: level},
                async: true,
                success: function (ret) {
                    if (id === 'start-debug') {
                        reload_click();
                    }
                }, error: function (ret) {
                }, complete: function () {
                }
            });
        }

        function download_log (id) {
            ajax_download('/log/log.action.php', 'action', 'download-' + id);
        }

        function generate_graph(e, id){
            $.ajax({
                url: '/log/log.action.php',
                type: 'POST',
                dataType: 'json',
                data: {action: 'gen-graph'},
                async: true,
                success: function (ret) {
                    layer.msg(gettext("Pipeline graph generated."));
                }, error: function (ret) {
                    layer.msg(gettext("Failed to generate pipeline graph!"));
                }, complete: function () {
                }
            });
        }

        function download_graph(e, id){
            ajax_download('/log/log.action.php', 'action', 'download-graph');
        }

        function view_graph(e, id) {
            window.open('/log/pipeline.graph.view.php', '_blank');
        }

        $(".layui-btn").click(function (e){
            e.preventDefault();
            let id = $(this).attr('id');
            let funcs = {
                clear: clear_click,
                reload: reload_click,
                pause: pause_click,
                'gen-graph': generate_graph,
                'download-graph': download_graph,
                'view-graph': view_graph,
            };

            if (id in funcs) {
                funcs[id](e, id);
            } else {
                if (id.indexOf('start') > -1 || id.indexOf('stop') > -1) {
                    start_stop_log(id);
                    return;
                }
                if (id.indexOf('download') > -1) {
                    download_log(id.replace(/download-/,''));
                }
            }
        });

        let log_levels = {};
        let loglevel_map = [
            {text: 'Off', val: 0},
            {text: 'Error', val: 1},
            {text: 'Warning', val: 2},
            {text: 'Info', val: 3},
            {text: 'Debug', val: 4},
            {text: 'Verbose', val: 5},
            {text: 'VV', val: 6},
            {text: 'VVV', val: 7},
            {text: 'Default', val: 8},
        ];
        load_modules();

        function on_change_level(data) {
            let id = data.elem.id;
            let t = id.split('-');
            if (t[1] === 'module') {
                let v = log_levels[t[0]][data.value];
                $('#' + t[0] + '-level').val(v);
                form.render();
            } else {
                let e = document.getElementById(t[0] + '-module');
                log_levels[t[0]][e.value] = data.value;
                show_log_level();
            }
        }

        function show_log_level() {
            ['debug', 'trace'].forEach(function(t) {
                let chk = document.getElementById(t + '-cust-check'),
                    l = log_levels[t],
                    ld = l['*DEFAULT*'],
                    lbl_level = document.getElementById(t + '-log-level'),
                    text = '';
                if (!chk.checked) {
                    ld = ld == 8 ? 1 : ld;
                    for (let id in l) {
                        let t = '';
                        if (id === '*DEFAULT*' || (l[id] > 0 && l[id] < 8 && l[id] != ld)) {
                            let lv = l[id];
                            if (id === '*DEFAULT*') {
                                t = 'default'
                                lv = ld;
                            } else {
                                t = id;
                            }
                            t = ' [' + t + ': ' + loglevel_map[lv].text + '] ';
                            text += t;
                        }
                    }
                    lbl_level.innerText = text;
                } else {
                    lbl_level.innerText = "";
                }
            });
        }

        function save_cust_level(){
            setCookie($(this).attr('id'), $(this).val(), 7);
        }

        function load_modules() {
            $.ajax({
                url: '/log/log.action.php',
                type: 'POST',
                dataType: 'json',
                data: {action: 'get-modules'},
                async: true,
                success: function (data) {
                    let modules = data.modules,
                        levels = data.level;
                    modules.unshift('*DEFAULT*');
                    ['debug', 'trace'].forEach(function(e){
                        let sel_module = document.getElementById(e + '-module'),
                            sel_level = document.getElementById(e + '-level'),
                            level_array = levels[e].split(',');
                        sel_module.setAttribute('lay-filter', sel_module.id);
                        option_set_values(sel_module, modules, true);
                        sel_level.setAttribute('lay-filter', sel_level.id);
                        option_set_values(sel_level, loglevel_map, false);
                        // init debug levels
                        log_levels[e] = {};
                        modules.forEach(function(d) {
                            log_levels[e][d] = 8;
                            level_array.some(function(l) {
                                let v = l.split(':'),
                                    m = v[0],
                                    n = v[1];
                                    n = parseInt(n),
                                    n = n > 7 ? 7 : n;
                                if (d === '*DEFAULT*' && m === '.*') {
                                    log_levels[e][d] = n;
                                    return true;
                                } else {
                                    if (d.match(m) == d) {
                                        if (m === '.*') {
                                            log_levels[e][d] = 8;
                                        } else {
                                            log_levels[e][d] = n;
                                        }
                                        return true;
                                    }
                                }
                            });
                        });
                        if (log_levels[e]['*DEFAULT*'] === 8) {
                            // set the default to ERROR
                            log_levels[e]['*DEFAULT*'] = 1;
                        }
                        $('#' + e + '-level').val(log_levels[e]['*DEFAULT*']);
                        let chk = document.getElementById(e + '-cust-check');
                        chk.setAttribute('title', gettext('Customize'));
                        chk.checked = getCookie(e + '-cust-check') === 'true';
                        let cust_level = document.getElementById(e + '-cust-level');
                        cust_level.value = getCookie(e + '-cust-level');
                        cust_level.oninput = save_cust_level;
                        update_cust_level(cust_level, chk.checked);
                    });
                    show_log_level();
                    form.render();
                }, error: function (ret) {
                    return false;
                }, complete: function () {
                }
            });
        }

        function update_cust_level(elem, enabled) {
            elem.disabled = !enabled;
            if (enabled) {
                elem.classList.remove('layui-disabled');
            } else {
                elem.classList.add('layui-disabled');
            }
        }

        function on_change_custom_level(data) {
            let id = data.elem.id,
                t = id.split('-'),
                elem = document.getElementById(t[0] + '-cust-level');

            update_cust_level(elem, data.elem.checked);
            setCookie(id, data.elem.checked, 7);
            show_log_level();
        }

        form.on('select()', function(data){
            on_change_level(data);
        });

        form.on('checkbox()', function(data){
            on_change_custom_level(data);
        });
    });
});