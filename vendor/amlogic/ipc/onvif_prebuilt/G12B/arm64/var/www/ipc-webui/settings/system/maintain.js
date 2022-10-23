"use strict";

layui.use([ 'layer', 'element', 'form' ], function() {
    var layer = layui.layer, element = layui.element, form = layui.form,
        $ = layui.jquery;

    $(document).ready(function() {
      $("#reboot").click(function(e) {
          e.preventDefault();
          layer.open({
              title: gettext('Tip'),
              content: gettext('Are you sure to reboot?')
              ,btn: [gettext('OK'), gettext('Cancel')]
              ,yes: function(index, layero){
                  $.ajax({
                      url : 'maintain.action.php',
                      type : 'post',
                      dataType : 'json',
                      data : {action : "reboot"},
                      success: function (data) {
                          if (data == null) {
                              layer.msg(data.responseText, {icon: 2});
                          }
                      },
                      error: function (data) {
                          layer.msg(data.responseText, {icon: 2});
                      }
                  });
                  layer.close(index);
              }
          });
      });
      $("#factory_reset").click(function (e) {
          e.preventDefault();
          layer.open({
              title: gettext('Tip'),
              content: gettext('Are you sure to factory reset?')
              ,btn: [gettext('OK'), gettext('Cancel')]
              ,yes: function(index, layero){
                  $.ajax({
                      url : 'maintain.action.php',
                      type : 'post',
                      dataType : 'json',
                      data : {action : "factory_reset"},
                      success: function (data) {
                          if (data == null) {
                              layer.msg(data.responseText, {icon: 2});
                          }
                      },
                      error: function (data) {
                          layer.msg(data.responseText, {icon: 2});
                      }
                  });
                  layer.close(index);
              }
          });
      });
    });
});
