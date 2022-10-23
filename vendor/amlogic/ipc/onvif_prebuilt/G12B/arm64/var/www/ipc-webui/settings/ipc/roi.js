"use strict";

layui.use(['element', 'layer', 'jquery', 'form', 'slider'], function () {
    let layer = layui.layer
        , form = layui.form
        , slider = layui.slider
        , $ = layui.$
    ;

    let qp_list = [41, 39, 37, 35, 33, 29];
    let quality_level = 1;

    function get_quality_level(qp) {
        let min_diff = 51;
        let level = 0;
        for (let i in qp_list) {
            let diff = Math.abs(qp - qp_list[i]);
            if (diff < min_diff) {
                min_diff = diff;
                level = i;
            }
        }
        return 1 + parseInt(level);
    }

    let slider_quality = slider.render({
        elem: '#quality'
        , value: 1
        , min: 1
        , max: 6
        , step: 1
        , input: true
        , setTips: function (value) {
            quality_level = value;
            return value;
        },
        change: function (value) {
            quality_level = value;
            let e = document.getElementById("moving_box");
            if (e !== null) {
                e.innerHTML = '<p>' + value + '</p>';
            }
        },
    });

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

    function create_rect(position, quality) {
        if (position.left > 1.0 || position.top > 1.0 ||
            position.left + position.width > 1.0 || position.top + position.height > 1.0) {
            console.log('rect out of range: ' + position.left + 'x' + position.top + '{' + position.width + 'x' + position.height + '}');
            return;
        }
        let left = container.rect.left + container.rect.width * position.left;
        let top = container.rect.top + container.rect.height * position.top;
        let w = container.rect.width * position.width;
        let h = container.rect.height * position.height;
        let box = document.createElement("div");
        box.className = "box";
        box.style.top = top + 'px';
        box.style.left = left + 'px';
        box.style.width = w + 'px';
        box.style.height = h + 'px';
        let level = get_quality_level(quality);
        if (level === 0) level = 1;
        box.innerHTML = '<p>' + level + '</p>';
        container.element.appendChild(box);
        box = null;
    }

    function load_rects() {
        let layer_id = layer.load(0, {time: 10*1000});
        $.ajax({
            url: 'roi.action.php'
            ,type: 'post'
            ,dataType: 'json'
            ,data: {action: "get"}
            ,success: function (data) {
                // clear all of the child elements
                clear_rects();
                // load rects
                for (let k in data) {
                    create_rect(data[k].position, data[k].quality);
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
                post_data.sections = [];
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
                    let section = {
                        position: rect,
                        quality: qp_list[boxes[i].textContent - 1],
                    };
                    post_data.sections[i] = section;
                }
                $.ajax({
                    url: "roi.action.php"
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
            startX = e.pageX;
            startY = e.pageY;
            let target_element = e.target;
            if (target_element.className === '') {
                target_element = target_element.parentElement;
                if (!target_element) return;
            }

            if (e.button === 2) {
                // right click
                if (target_element.className.match(/box/)) {
                    removing = true;
                    target_element.id = "removing_box";
                }
                return;
            }

            // prevent other mouse click event
            if (e.button !== 0) return;

            if (target_element.className.match(/box/)) {
                dragging = true;

                if (document.getElementById("moving_box") !== null) {
                    document.getElementById("moving_box").removeAttribute("id");
                }
                target_element.id = "moving_box";
                slider_quality.setValue(target_element.innerText - 1);

                diffX = startX - target_element.offsetLeft;
                diffY = startY - target_element.offsetTop;
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
                active_box.innerHTML = '<p>' + quality_level + '</p>';
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
                if (document.getElementById("moving_box") !== null) {
                    document.getElementById("moving_box").removeAttribute("id");
                }
                let ab = document.getElementById("active_box");
                ab.id = "moving_box";
                // remove box if rects too small
                if (ab.offsetWidth < 20 || ab.offsetHeight < 20) {
                    container.element.removeChild(ab);
                }
            }
        };
    });
});
