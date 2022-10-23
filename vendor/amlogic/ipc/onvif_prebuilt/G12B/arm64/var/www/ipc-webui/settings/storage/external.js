"use strict";

layui.use([ 'layer', 'element', 'form' ], function() {
  var layer = layui.layer,
      element = layui.element,
      form = layui.form,
      $ = layui.jquery;

  var size_info = [];
  var total_size;
  var sd_Chart = echarts.init(document.getElementById('sd_pie'));
  let extra_post_data = [];
  let fs_list = [
    {id : "fs", options : [ "ext2", "ext4", "exfat"]},
  ];

  get_device();

  function get_device() {
    let layer_id = layer.load(0, {time : 10 * 1000});
    let cnt = 0;
    $.ajax({
      url : 'storage.action.php',
      type : 'post',
      dataType : 'json',
      data : {action : "device"},
      success : function(data) {
        let e = document.getElementById('dev');
        let devs = [];
        for (let k in data) {
          if (data[k] != "/dev/data" && data[k] != "/dev/system") {
            devs.push(data[k]);
            cnt++;
          }
        }
        if (cnt == 0) {
          layer.msg(gettext('There is no mounted SD.'));
          layer.close(layer_id);
          return;
        } else {
          option_set_values(e, devs, true);
          option_set_selected(e, 0);
          fs_list.forEach(function(l) {
            let e = document.getElementById(l.id);
            option_set_values(e, l.options, true);
          });
        }
        get_dev_info(document.getElementById("dev").value);
        form.render();
      },
      error:function (data) {
        layer.msg(data.responseText, {icon: 2});
      },
      complete : function() {
        layer.close(layer_id);
      }
    });
  }

  function format_size(str) {
    var size;
    if (str.search(/M/) > 0) {
      size = parseFloat(str) / 1024;
    } else if (str.search(/K/) > 0) {
      size = parseFloat(str) / 1024 / 1024;
    } else {
      size = parseFloat(str);
    }
    return Math.round(size*100)/100;
  }

  function get_dev_info(dev) {
    let layer_id = layer.load(0, {time : 10 * 1000});
    $.ajax({
      url : "storage.action.php",
      type : 'post',
      dataType : 'json',
      data : {action : "info", prop : dev},
      success : function(data) {
        for (let k in data) {
          if (document.getElementById(k)) {
            $('#' + k).val(data[k]);
            $('#' + k + "_div").show();
          } else if (k == "size") {
            total_size = data["size"];
          } else {
            size_info.push({value : format_size(data[k]), name : gettext(k)+'('+format_size(data[k])+'G'+')'});
          }
        }
        $("#sd_btn").show();
        var option = {
          title : {
            text : gettext('External Usage Analysis'),
            subtext : total_size,
            left : 'center'
          },
          tooltip : {trigger : 'item', formatter : '{a}<br/>{b}:{d}%'},
          series : [ {
            name : gettext('External Size'),
            type : 'pie',
            radius : '55%',
            label : {
              show : true,
            },
            data : size_info
          } ]
        };
        size_info = [];
        total_size = null;
        sd_Chart.setOption(option);
        form.render();
      },
      error:function (data) {
        layer.msg(data.responseText, {icon: 2});
      },
      complete : function() {
        layer.close(layer_id);
      }
    })
  }

  form.on('select(dev)', function(data) {
    get_dev_info(data.value);
  });

  $(document).ready(function() {
    $("#format").click(function(e) {
      e.preventDefault();
      layer.open({
        title: gettext('Tip'),
        content: gettext('Are you sure to format?'),
        btn: [gettext('OK'), gettext('Cancel')],
        yes: function (index, layero) {
          let layer_id = layer.load(0, {time : 20 * 1000});
          let post_data = getFormData($("#form_sd_format"));
          post_data['action'] = 'format';
          post_data = $.extend(post_data, extra_post_data);
          $.ajax({
            url : "storage.action.php",
            type : "post",
            dataType : "json",
            data : post_data,
            success : function(data) {
              if (data === "finish") {
                layer.msg(gettext('Format Success'));
              }
            },
            complete : function() {
              layer.close(layer_id);
            },
            error:function () {
              layer.msg(gettext('Format Fail'), {icon: 2});
            }
          });
          layer.close(index);
        }
      });
    });
  });
});
