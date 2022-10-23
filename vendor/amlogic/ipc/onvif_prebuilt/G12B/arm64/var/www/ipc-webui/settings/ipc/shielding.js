"use strict";

layui.use(['element', 'layer', 'jquery', 'form'], function () {
    let layer = layui.layer
        , form = layui.form
        , $ = layui.$
    ;

    let container = {
        element: document.getElementById('container'),
    };
    container.rect = {
        top: container.element.offsetTop + container.element.clientTop,
        left: container.element.offsetLeft + container.element.clientLeft,
        width: container.element.clientWidth,
        height: container.element.clientHeight,
    };
    container.rect.right = container.rect.left + container.rect.width;
    container.rect.bottom = container.rect.top + container.rect.height;

    load_rects();

    function clear_rects() {
        let rects = container.element.getElementsByClassName('box');
        for (;rects.length > 0;) {
            container.element.removeChild(rects[0]);
        }
    }

    function create_rect(rel_x, rel_y, rel_w, rel_h) {
        if (rel_x > 1.0 || rel_y > 1.0 || rel_x + rel_w > 1.0 || rel_y + rel_h > 1.0) {
            console.log('rect out of range: ' + rel_x + 'x' + rel_y + '{' + rel_w + 'x' + rel_h + '}');
            return;
        }
        let left = container.rect.left + container.rect.width * rel_x;
        let top = container.rect.top + container.rect.height * rel_y;
        let w = container.rect.width * rel_w;
        let h = container.rect.height * rel_h;
        let box = document.createElement("div");
        box.className = "box";
        box.style.top = top + 'px';
        box.style.left = left + 'px';
        box.style.width = w + 'px';
        box.style.height = h + 'px';
        container.element.appendChild(box);
        box = null;
    }

    function load_rects() {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'shielding.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get"}
            ,success: function (data) {
                // clear all of the child elements
                clear_rects();
                // load rects
                for (let k in data) {
                    create_rect(data[k].left, data[k].top, data[k].width, data[k].height);
                }
                form.render();
            }
            ,complete: function () {
                layer.close(layer_id);
            }
        });
    }

    $(document).ready(function () {
        $(".layui-btn").click(function (e) {
            e.preventDefault();
            let name = $(this).attr('name');
            if (name === 'reset') {
                load_rects();
            } else if (name === 'remove_all') {
                clear_rects();
            } else if (name === 'submit') {
                let layer_id = layer.load(0, {time: 10 * 1000});
                let post_data = {};
                let boxes = $('.box');
                post_data.action = 'set';
                post_data.size = boxes.length;
                post_data.rects = [];
                for (let i = 0; i < boxes.length; i++) {
                    let rect = {
                        top: boxes[i].offsetTop + boxes[i].clientTop,
                        left: boxes[i].offsetLeft + boxes[i].clientLeft,
                        width: boxes[i].clientWidth,
                        height: boxes[i].clientHeight,
                    };

                    rect.left = (rect.left - container.rect.left) / container.rect.width;
                    rect.top = (rect.top - container.rect.top) / container.rect.height;
                    rect.width = rect.width / container.rect.width;
                    rect.height = rect.height / container.rect.height;
                    post_data.rects[i] = rect;
                }
                $.ajax({
                    url: "shielding.action.php"
                    , type: "post"
                    , dataType: "json"
                    , data: post_data
                    , complete: function () {
                        layer.close(layer_id);
                    }
                });
            }
        });
    });

    window.addEventListener('load', function(e) {
        e = e || window.event;
        let startX, startY, diffX, diffY;
        let dragging = false;
        let removing = false;

        function restrict_movement(x, y, w = 0, h = 0) {
            if (x < container.rect.left) x = container.rect.left;
            if (x + w > container.rect.right) x = container.rect.right - w;
            if (y < container.rect.top) y = container.rect.top;
            if (y + h > container.rect.bottom) y = container.rect.bottom - h;

            return {x: x, y: y};
        }

        container.element.addEventListener('contextmenu', function (e) {
            e.preventDefault();
            return false;
        });

        // 鼠标按下
        document.onmousedown = function (e) {
            if (e.button === 2) {
                // right click
                if (e.target.className.match(/box/)) {
                    removing = true;
                    e.target.id = "removing_box";
                }
                return;
            }

            // prevent other mouse click event
            if (e.button !== 0) return;

            startX = e.pageX;
            startY = e.pageY;

            if (e.target.className.match(/box/)) {
                dragging = true;

                if (document.getElementById("moving_box") !== null) {
                    document.getElementById("moving_box").removeAttribute("id");
                }
                e.target.id = "moving_box";

                diffX = startX - e.target.offsetLeft;
                diffY = startY - e.target.offsetTop;
            } else {
                if (!e.target.className.match(/container/)) {
                    return;
                }
                if ($('.box').length > 20) {
                    layer.msg(gettext("Box limitation exceeded"));
                    return;
                }
                // check the duplicated active box to prevent errors
                let old_active_box = document.getElementById("active_box");
                if (old_active_box != null) {
                    old_active_box.removeAttribute('id');
                }
                let active_box = document.createElement("div");
                active_box.id = "active_box";
                active_box.className = "box";
                active_box.style.top = startY + 'px';
                active_box.style.left = startX + 'px';
                container.element.appendChild(active_box);
                active_box = null;
            }
        };

        document.onmousemove = function (e) {
            if (e.button !== 0) return;

            if (document.getElementById("active_box") !== null) {
                let ab = document.getElementById("active_box");
                let pos = restrict_movement(e.pageX, e.pageY);
                ab.style.width = pos.x - startX + 'px';
                ab.style.height = pos.y - startY + 'px';
            }

            if (document.getElementById("moving_box") !== null && dragging) {
                let mb = document.getElementById("moving_box");
                let rect = mb.getBoundingClientRect();
                let pos = restrict_movement(e.pageX - diffX, e.pageY - diffY, rect.width, rect.height);
                mb.style.left = pos.x + 'px';
                mb.style.top = pos.y + 'px';
            }
        };

        document.onmouseup = function (e) {
            if (e.button === 2 && removing) {
                // right click
                removing = false;
                let rb = document.getElementById('removing_box');
                container.element.removeChild(rb);
                return;
            }

            // prevent other mouse click event
            if (e.button !== 0) return;

            dragging = false;
            if (document.getElementById("active_box") !== null) {
                let ab = document.getElementById("active_box");
                ab.removeAttribute("id");
                // remove box if rects too small
                if (ab.offsetWidth < 3 || ab.offsetHeight < 3) {
                    container.element.removeChild(ab);
                }
            }
        };
    });
});
