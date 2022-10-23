function WSFrameToMP4(url, proto) {
    this.url = url;
    this.proto = proto;
    this.caps = null;
    this.trackInfo = {
        id: 1,
        timescale: 90000,
//                duration:-1,
        type: 'video',
        dts: 0,
        pixelRatio: [1, 1],
        seq: 0,
        width: 1920,
        height: 1080,
        timeFirstFrame: 0,
        timeLastFrame: 0,
        samples: [{
            duration: 0,
            size: 0,
            cts: 0,
            flags: {
                isLeading: 0,
                dependsOn: 0,
                isDependedOn: 0,
                hasRedundancy: 0,
                paddingValue: 0,
                isNonSync: 0,
                degradPrio: 0
            },
        }]
    };

    this.onSegment = function (data) {
    };
    this.onInitSegment = function (data) {
    };

    this.close = function () {
        this.ws.close();
    };

    this.open = function () {
        this.caps = null;
        this.trackInfo.seq = 0;
        this.trackInfo.firstDTS = null;
        this.trackInfo.timeStart = this.trackInfo.timeFirstFrame = this.trackInfo.timeLastFrame = window.performance.now();
        if (this.ws) this.ws.close();
        this.ws = new WebSocket(this.url, this.proto);
        this.ws.binaryType = 'arraybuffer';
        this.ws.onopen = function (e) {
            e.target.send("start-live-streaming");
        };
        this.ws.onmessage = this.onmessage.bind(this);
    };

    this.onmessage = function (msg) {
        var dv = new DataView(msg.data);
        var headerlen = dv.getUint32(0);
        var data = new Uint8Array(msg.data, headerlen);
        if (!this.caps) {
            this.trackInfo.timeFirstFrame = window.performance.now();
            this.initCaps(data);
            return;
        }
        var dts = dv.getUint32(4) * 90;
        var pts = dv.getUint32(8) * 90;
        var dur = dv.getUint32(12) * 90;
        var cts = pts - dts;
        var flags = dv.getUint32(16);   // GstBufferFlags
        this.trackInfo.samples[0].flags.isNonSync = (flags & 8192) ? 1 : 0;
        if (this.trackInfo.firstDTS === null) {
            if (this.trackInfo.samples[0].flags.isNonSync)
                return;
            this.trackInfo.firstDTS = dts;
            pts -= dts;
            dts = 0;
            this.trackInfo.dts = 0;
        } else {
            dts -= this.trackInfo.firstDTS;
            pts -= this.trackInfo.firstDTS;
        }
        var dtserror = dts - this.trackInfo.dts;
        if (dtserror < -dur) {
            console.log("frame:" + this.trackInfo.seq + " dts:" + dts + " expect:" + this.trackInfo.dts + " diff:" + dtserror);
        } else if (dtserror > dur) {
            //console.log("frame:" + this.trackInfo.seq + " dts:" + dts + " expect:"+this.trackInfo.dts+" diff:"+dtserror);
            dur += dtserror;
        }
        this.trackInfo.samples[0].duration = dur;
        this.trackInfo.samples[0].dts = dts;
        this.trackInfo.samples[0].pts = pts;
        this.trackInfo.samples[0].size = data.byteLength;
        this.trackInfo.samples[0].cts = cts;
        //console.log("frame:" + this.trackInfo.seq + " pts:" + pts + " dts:" + dts + " dur:" + dur + " size:" + this.trackInfo.samples[0].size + " flags:0x" + flags.toString(16));
        this.onSegment(MP4.moof(this.trackInfo.seq, this.trackInfo.dts, this.trackInfo));
        this.onSegment(MP4.mdat(data));
        this.trackInfo.timeLastFrame = window.performance.now();
        this.trackInfo.seq++;
        this.trackInfo.dts += dur;
    };

    this.initCaps = function (data) {
        var caps_arr = String.fromCharCode.apply(null, data).split(", ");
        this.caps = {
            name: caps_arr[0]
        };
        for (var i = caps_arr.length; --i;) {
            var pair = caps_arr[i].match(/(.+?)\s*=\s*\((.*)\)\s*(.*)/);
            this.caps[pair[1]] = pair[3].trim();
        }
        if (this.caps.name == "video/x-h264") {
            if (this.caps.codec_data) {
                this.codecString = "avc1." + this.caps.codec_data.substr(2, 6);
                var str_len = this.caps.codec_data.length / 2;
                var u8 = new Uint8Array(str_len);
                for (var i = 0; i < str_len; ++i) {
                    u8[i] = parseInt(this.caps.codec_data.substr(i * 2, 2), 16);
                }
                var pos = 0;
                this.trackInfo.headlen = (u8[4] & 3) + 1;
                this.trackInfo.sps = [];
                this.trackInfo.pps = [];
                var count = u8[5] & 31;
                pos += 6;
                for (var n = 0; n < count; ++n) {
                    var len = u8[pos] << 8 | u8[pos + 1];
                    pos += 2;
                    this.trackInfo.sps.push(u8.subarray(pos, pos + len));
                    pos += len;
                }
                count = u8[pos++] & 31;
                for (var n = 0; n < count; ++n) {
                    var len = u8[pos] << 8 | u8[pos + 1];
                    pos += 2;
                    this.trackInfo.pps.push(u8.subarray(pos, pos + len));
                    pos += len;
                }
            }
            if (this.caps.width) this.trackInfo.width = parseInt(this.caps.width);
            if (this.caps.height) this.trackInfo.height = parseInt(this.caps.height);
        }
        this.onInitSegment(MP4.initSegment([this.trackInfo], 0xffffffff, this.trackInfo.timescale));
    }
}

function H264Player(vid, url, proto) {
    this.video = vid;
    this.buffers = [];
    this.playing = false;
    this.mp4 = new WSFrameToMP4(url, proto);

    this.play = function () {
        this.playing = true;
        this.mp4.open();
    };

    this.stop = function () {
        this.playing = false;
        this.mp4.close();
        this.sb = null;
        this.video.pause();
        this.video.removeAttribute('src');
        this.video.load();
    };

    this.append = function () {
        if (this.buffers.length && !this.sb.updating) {
            this.sb.appendBuffer(this.buffers.shift());
        }
    };

    this.mp4.onInitSegment = function (data) {
        this.buffers = [data];
        this.mse = new (window.MediaSource || window.WebKitMediaSource)();
        this.video.src = URL.createObjectURL(this.mse);
        this.mse.addEventListener('sourceopen', function (e) {
            URL.revokeObjectURL(this.video.src);
            this.sb = this.mse.addSourceBuffer('video/mp4; codecs="' + this.mp4.codecString + '"');
            this.sb.addEventListener("update", this.append.bind(this));
            this.append();
        }.bind(this));
    }.bind(this);

    this.mp4.onSegment = function (data) {
        this.buffers.push(data);
        if (this.sb)
            this.append();
    }.bind(this);
}

let wsport = call_remote_function('/view-stream.php', 'get_web_stream_port')['ret'];
var wsurl = 'ws://' + document.location.hostname + ':' + wsport + '/';
var vidElement;
var player;
var show_fps = false;
var use_shortcut = true;

function EnableShortCuts(enabled) {
    use_shortcut = enabled;
}

window.onload = function () {
    vidElement = document.querySelector('video');
    vidElement.addEventListener('dblclick', function (e) {
        if (!use_shortcut) return;
        if (!document.webkitCurrentFullScreenElement) {
            e.target.parentElement.webkitRequestFullScreen();
        } else {
            document.webkitCancelFullScreen();
        }
        vidElement.currentTime = vidElement.buffered.end(0) - 0.2;
    });
    vidElement.addEventListener('contextmenu', function (e) {
        e.preventDefault();
        return false;
    });
    vidElement.addEventListener('playing', function (e) {
        console.log(e.type + " " + vidElement.buffered.end(0).toFixed(3) + "/" + vidElement.currentTime.toFixed(3));
        e.target.play();
    });
    vidElement.addEventListener('keydown', function (e) {
        if (!use_shortcut) return;
        if (e.key == "F1" && e.ctrlKey) {
            show_fps = show_fps ? false : {seq: 0, time: player.mp4.trackInfo.timeFirstFrame};
            document.getElementById('fps').style.display = show_fps ? "block" : "none";
            document.getElementById('resolution').style.display = show_fps ? "block" : "none";
        }
    });
    vidElement.addEventListener('waiting', function (e) {
        console.log(e.type + " " + vidElement.buffered.end(0).toFixed(3) + "/" + vidElement.currentTime.toFixed(3));
    });
    document.addEventListener('visibilitychange', function (e) {
        if (document.hidden) {
            stop_player();
        } else {
            start_player();
        }
    });

    function start_player() {
        if (!player)
            player = new H264Player(vidElement, wsurl, "ipc-webstream");
        player.play();
        check_update();

        function check_update() {
            if (player.update_timer_id) {
                clearTimeout(player.update_timer_id);
                player.update_timer_id = 0;
            }
            if (player.playing) {
                var latency = vidElement.buffered.length ? vidElement.buffered.end(0) - vidElement.currentTime : 0;
                if (show_fps && player.mp4.caps) {
                    var fps = (player.mp4.trackInfo.seq - show_fps.seq) * 1000 / (player.mp4.trackInfo.timeLastFrame - show_fps.time);
                    document.getElementById('fps').textContent = "fps:" + fps.toFixed(3) + " latency:" + latency.toFixed(3) + " num frames:" + player.mp4.trackInfo.seq;
                    document.getElementById('resolution').textContent = "resolution:" + player.mp4.caps.width + 'x' + player.mp4.caps.height;
                    show_fps.seq = player.mp4.trackInfo.seq;
                    show_fps.time = player.mp4.trackInfo.timeLastFrame;
                }
                if (latency > 0.6) {
                    console.log("latency is too big " + latency.toFixed(3) + " drop...");
                    vidElement.currentTime = vidElement.buffered.end(0) - 0.2;
                }
                var timeSinceLast = window.performance.now() - player.mp4.trackInfo.timeLastFrame;
                if (timeSinceLast > 3000) {
                    console.log("no frame received in " + timeSinceLast + " ms, restart...");
                    player.stop();
                    player.play();
                }
                player.update_timer_id = setTimeout(check_update, 1000);
            }
        }
    }

    function stop_player() {
        if (player) player.stop();
    }

    start_player();
}
