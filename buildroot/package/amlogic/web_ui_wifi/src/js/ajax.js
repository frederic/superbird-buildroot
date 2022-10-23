function Parameters(){
	this.paramMap = new Object;
	this.keyArray = new Array;
}

Parameters.prototype.setParameter = function(key, val){
	if(key == null || val == null)
		return;
	if(this.paramMap[key] == null){
		this.keyArray.push(key);
	}
	this.paramMap[key] = val;
};

Parameters.prototype.getParameter = function(key){
	if(key == null)
		return null;
	return this.paramMap[key];
};

Parameters.prototype.getKeyArray = function(){
	return this.keyArray;
};

var AjaxSupport = {
	doGet : function(url, params, callback, waiting, extra) {
		if(waiting != true){
			waiting = false;
		}
		var req = GetXmlHttpObject();
		if (req) {
			var sb = "";
			var keyArr = params.getKeyArray();
			for(var i=0; i<keyArr.length; i++){
				sb += keyArr[i] + "=" + params.getParameter(keyArr[i]) + "&";
			}
			if(sb != ""){
				url += "?" + sb.substring(0, sb.length-1);
			}
			req.open("GET", url, true);
			req.onreadystatechange = function() {
				if (req.readyState == 4) {
					if (req.status == 200) {
						if (callback != null)
							callback(req, extra);
						if(waiting)
							hideWaitingDialog();
					}
				}
			};
			if(waiting){
				showWaitingDialog();
			}
			req.send(null);
		}
	}
};
function GetXmlHttpObject()
{
var xmlHttp=null;
try
 {
 // Firefox, Opera 8.0+, Safari
 xmlHttp=new XMLHttpRequest();
 }
catch (e)
 {
 // Internet Explorer
 try
  {
  xmlHttp=new ActiveXObject("Msxml2.XMLHTTP");
  }
 catch (e)
  {
  xmlHttp=new ActiveXObject("Microsoft.XMLHTTP");
  }
 }
return xmlHttp;
}
