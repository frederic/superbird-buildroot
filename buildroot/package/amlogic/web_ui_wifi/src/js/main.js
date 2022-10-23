/**************************************************************
 * Function	:main.js for amlogic web_ui
 * Date		:2017-12-26
 * Auther	:haibing.an
 * @		:haibing.an@amlogic.com
 **************************************************************/

/***************************************************
 *HTML url handle
 * ************************************************/
var obj_Data;
function HTMLEnCode(str)
{
	var    s    =    "";
	if (str.length    ==    0)    return    "";
	s = str.replace(/&/g, "&gt;");
	s = s.replace(/</g, "&lt;");
	s = s.replace(/>/g, "&gt;");
	s = s.replace(/ /g, "&nbsp;");
	s = s.replace(/\'/g, "'");
	s = s.replace(/\"/g, "&quot;");
	s = s.replace(/\n/g, "<br>");
	return    s;
}

function handleCommand(command_xmlhttp, callback)
{
	if(command_xmlhttp.readyState==4)
	{
		if(command_xmlhttp.status == 200){
			if(callback == null)
				return;
			callback(command_xmlhttp);
		}
	}
}

function send_commond(command, callback,cgi)
{
  var cgi = cgi || "cgi-bin/main.cgi";
	command_xmlhttp = GetXmlHttpObject();               //create req obj
	command_xmlhttp.onreadystatechange = function () {        //get data from web action
		handleCommand(command_xmlhttp, callback);
	}

	command_xmlhttp.open("POST",cgi,true);
	command_xmlhttp.setRequestHeader("If-Modified-Since","0");
	command_xmlhttp.setRequestHeader("Cache-Control","no-cache");
	command_xmlhttp.setRequestHeader("CONTENT-TYPE","application/x-www-form-urlencoded");
	command_xmlhttp.send(command);
}
//--------------HTML url handle end---------------//

function get_bt_list()
{
	document.getElementById("load_div").style="";
	$(":button").attr("disabled",true);
	var wifi_json = send_commond("",bt_list_init,"cgi-bin/bt.cgi");
}


function bt_list_init(){
	var response_bt_list = command_xmlhttp.responseText;
	response_bt_list =response_bt_list.substring(response_bt_list.indexOf("{"),response_bt_list.lastIndexOf("}")) +"}";
	
	var response_bt_list_json = eval("(" + response_bt_list + ")");
	var len = response_bt_list_json.bt_list.length;
	var htmlStr="";
	for(var i=0; i< len ;i++){
		console.log(response_bt_list_json.bt_list[i].name);
		htmlStr += '<button type="button" name=" '+ i +'" class="btn bt_item btn-default btn-lg btn-block">'+response_bt_list_json.bt_list[i].name +'</button>'
	}
	document.getElementById("load_div").style="display:none";
	$(":button").attr("disabled",false);
	$('#bt_btn').html(htmlStr);
}

$('#bt_btn').on('click', 'button', function(){
	send_commond("", is_bt_ok, "cgi-bin/bt_con.cgi?index="+eval(this.name));
})
function is_bt_ok(){
	var response_status = command_xmlhttp.responseText;
	var stau = eval("(" + response_status +")");
	if (stau.status=="ture")
		alert("BT connecnt success");
	else
		alert("BT connecnt faile");
}

/*************************************************
 * WI-FI handle
 ************************************************/
function get_wifi_list()
{
	var wifi_list = null;
	var wifi_json = send_commond("get_wifi_list", set_wifi_list_to_select);
	show_loading();
	setTimeout(function () { $('#loading').hide(); }, 3000);
}

function show_loading(){
	loadNode = '<img id="loading" src="./images/loading.gif" />';
	$('#loadpoint').html(loadNode);
}

function set_wifi(ssid,pwd)
{
	var wifi_ssid = ssid;
	var wifi_pwd = pwd;
	var wifi_json = send_commond("set_wifi&ssid="+wifi_ssid+"&"+"pwd="+wifi_pwd);
	show_loading();

	setTimeout("check_wifi_connected1()", 10000);
    setTimeout("check_wifi_connected1()", 15000);
    setTimeout("check_wifi_connected1()", 20000);
    setTimeout("check_wifi_connected1()", 25000);
    setTimeout("check_wifi_connected1()", 30000);
    setTimeout(function () { $('#loading').hide(); }, 31000);
}

function set_wifi_list_to_select(command_xmlhttp)
{
	var response_date = command_xmlhttp.responseText;

	obj_Data = eval("("+response_date+")");
	if(obj_Data.length == 0){
		get_wifi_list();
	}
	else{
		var htmlNodes = '';

		for(var i = 0; i < obj_Data.length; i++){
			htmlNodes += '<a class="list-group-item" id="wifi_' + i + '"' + ' role="button" data-toggle="modal" data-target="#myModal">' + obj_Data[i].ssid + '</a>';
		}
		htmlNodes += '</ul>';

		$('#testtext').html(htmlNodes);

		var j;
		var index = 1;
		var wifi_select = document.getElementById("ssid");
		for(j = 0; j < obj_Data.length; j++){
			var wifi = new Object;
			wifi.ssid = obj_Data[j].ssid;
			//wifi_select.Option.add(wifi.id);
		}
	}
	check_wifi_connected();
}

function set_wifi_by_input()
{
	var input_ssid = document.getElementById("selfModal_ssid").value;
	var input_pwd = document.getElementById("selfModal_pwd").value;
	set_wifi(input_ssid,input_pwd);
}

function set_wifi_by_modal_input()
{
	setTimeout(function(){$("#mymodal").modal("hide")},2000);
	var input_ssid = document.getElementById("modal_ssid").value;
	var input_pwd = document.getElementById("modal_pwd").value;

	set_wifi(input_ssid,input_pwd);
}

//-------------------------------------------------------
function check_wifi_connected1() {
    show_loading();
    send_commond("check_wifi_connected", handle_check_wifi1);
}

function handle_check_wifi1(command_xmlhttp)
{
    var response_check_wifi1 = command_xmlhttp.responseText;
    var obj_check_wifi_Data1 = eval("("+response_check_wifi1+")");
    var check_wifi_nc_Nodes1 = '<h3 style="color:red">No Connected</h3>';

	if (obj_check_wifi_Data1.length > 7 && obj_check_wifi_Data1[8].info == "COMPLETED") {
		var check_wifi_oc_Nodes1 = '<h3 style="color:green">' + obj_check_wifi_Data1[2].info + '</br>' + obj_check_wifi_Data1[9].info+ '</h3>';
        $('#check_wifi').html(check_wifi_oc_Nodes1);
        $('#loading').hide();
        console.log("set DONE!");
	}
	if (obj_check_wifi_Data1[0].info == "SCANNING") {
        $('#check_wifi').html(check_wifi_nc_Nodes1);
	}
    if (obj_check_wifi_Data1[0].info == "DISCONNECTED") {
        $('#check_wifi').html(check_wifi_nc_Nodes1);
        $('#loading').hide();
        alert("Unknown error & stop");
    }
}

//-----------------------------------------------

function check_wifi_connected() {
	show_loading();
	setTimeout(function () { $('#loading').hide(); }, 2000);
	send_commond("check_wifi_connected", handle_check_wifi);
}

function handle_check_wifi(command_xmlhttp)
{
	var response_check_wifi = command_xmlhttp.responseText;
	var obj_check_wifi_Data = eval("("+response_check_wifi+")");
	var check_wifi_Nodes = "";

	if (obj_check_wifi_Data.length < 4){
		//if (obj_check_wifi_Data[0].info == "SCANNING")
		check_wifi_Nodes = '<h3 style="color:red">No Connected</h3>';
	}else{
		if (obj_check_wifi_Data[8].info == "COMPLETED")
			check_wifi_Nodes = '<h3 style="color:green">' + obj_check_wifi_Data[2].info + '</br>' +  obj_check_wifi_Data[9].info + '</h3>';
	}

	$('#check_wifi').html(check_wifi_Nodes);
}

function goTop() {
	$('html, body').animate({scrollTop:0}, 'fast');
}

function check_input_is_ok(input_text)
{
	if (input_text=="")
		return false;
	else
		return true;
}

$('#testtext').on('click','a',function(){
	var inner_ssid = this.innerText;
	var my_ssid = document.getElementById("modal_ssid");
	var my_pwd = document.getElementById("modal_pwd");
	my_pwd.value = "";
	my_ssid.value = inner_ssid;
});
//---------------WI-FI handle end---------------//


/**********************************************
 *RECORD handle
 *********************************************/
function get_record_list()
{
	send_commond("get_record_list", handle_record_list);
}

function get_record_dl_list() {
	send_commond("get_record_dl_list", handle_record_dl_list);
}

function handle_record_list(command_xmlhttp)
{
	var response_record_date = command_xmlhttp.responseText;

	var obj_record_Data = eval("("+response_record_date+")");
	if(obj_record_Data.length == 0){
		get_record_list();
	}
	else{
		var html_record_Nodes = '';

		for(var i = 0; i < obj_record_Data.length; i++){
			html_record_Nodes += '<a class="list-group-item" id="record_' + i + '"' + ' role="button" data-toggle="modal" data-target="#recordModal">' + obj_record_Data[i].filename + '</a>';
		}
		html_record_Nodes += '</ul>';

		$('#recordtext').html(html_record_Nodes);

		var j;
		var index = 1;
		var record_select = document.getElementById("filename");
		for(j = 0; j < obj_record_Data.length; j++){
			var record = new Object;
			record.filename = obj_record_Data.recordlist[j].filename;
			record_select.Option.add(record.id);
		}
	}
}

function handle_record_dl_list() {
	var response_record_dl_date = command_xmlhttp.responseText;

	var obj_record_dl_Data = eval("("+response_record_dl_date+")");
	if(obj_record_dl_Data.length == 0){
		get_record_dl_list();
	}
	else{
		var html_record_dl_Nodes = '';

		for(var i = 0; i < obj_record_dl_Data.length; i++){
			html_record_dl_Nodes += '<a class="list-group-item" download="" href="record/' + obj_record_dl_Data[i].filename + '"' + '>' + obj_record_dl_Data[i].filename + '</a>';
		}
		html_record_dl_Nodes += '</ul>';

		$('#record_dl_text').html(html_record_dl_Nodes);
	}
}

function start_record() {

	var input_filename = document.getElementById("modal_filename").value;
	var channal = $('#select-ch option:selected').val();
	var rate = $('#select-rate option:selected').val();
	var bits = $('#select-bits option:selected').val();
	console.log(channal);
	console.log(rate);
	console.log(bits);
	send_commond("start_record&filename="+input_filename+"&ch="+channal+"&rate="+rate+"&bits="+bits);
	//get_record_list();
}

function done_record() {
	send_commond("done_record");
	get_record_list();
}

function delete_record() {
	var input_filename = document.getElementById("modal_filename").value;
	send_commond("delete_record&filename="+input_filename);
	get_record_list();
}

function play_record() {
    var input_filename = document.getElementById("modal_filename").value;
    send_commond("play_record&filename="+input_filename);
}

function play_local_record() {
    var play_filename = document.getElementById("modal_filename").value;
    var player_nodes = "";
    player_nodes += '<audio id ="music" controls="controls" muted="muted"><source  src="record/'+play_filename+'"'+ '/>' + '</audio>';
    $('#ol_player').html(player_nodes);
    var music = document.getElementById('music');

    var input_filename = document.getElementById("modal_filename").value;
    send_commond("play_record&filename="+input_filename);
}

function play_online_record() {
    var play_filename = document.getElementById("modal_filename").value;
    var player_nodes = "";
    var file_info_nodes = "";
    file_info_nodes += play_filename;
    player_nodes += '<audio id ="music" controls="controls"><source  src="record/'+play_filename+'"'+ '/>' + '</audio>';
    $('#file_info').html(file_info_nodes);
    $('#ol_player').html(player_nodes);
    var music = document.getElementById('music');
}

$('#recordtext').on('click','a',function () {
	var inner_filename = this.innerText;
	var my_filename = document.getElementById("modal_filename");
	my_filename.value = inner_filename;
});
//--------------RECORD handle end--------------//


/***********************************************
 * Spotify handle
 **********************************************/
function set_spotify(){
	var spotify_username = document.getElementById("modal_spotify_username").value;
	var spotify_pwd = document.getElementById("modal_spotify_pwd").value;
	var spotify_dname = document.getElementById("modal_spotify_dname").value;

	send_commond("set_spotify&username="+spotify_username+"&"+"spotify_pwd="+spotify_pwd+"&"+"spotify_dname="+spotify_dname);
	var showUserId = '<h3 style="color:green" align="center">Logging...</h3>';
	$('#userinfo').html(showUserId);
	setTimeout("check_spotify()",3000);
}

function TurnOffSpotify(){
	send_commond("kill_spotify");
	var showStopping = '<h3 style="color:red" align="center">stopping...</h3>';
	$('#userinfo').html(showStopping);
	setTimeout("check_spotify()",3000);
}

function check_spotify(){
	send_commond("check_spotify",handle_check);
}

function handle_check(command_xmlhttp){

	var response_check_date = command_xmlhttp.responseText;

	if(response_check_date == "")
		return;

	if(response_check_date.length > 7){
		send_commond("get_spo_info",handle_get_info);
	}
	else{
		var htmlNodes_spo_info = '<h3 style="color:red" align="center">Stopped</h3>';
		$('#userinfo').html(htmlNodes_spo_info);
	}
}

function handle_get_info(){
	var response_spo_info = command_xmlhttp.responseText;
	if(response_spo_info == "")
		return;
	var spo_info = eval("("+response_spo_info+")");
	console.log(spo_info);
	var old_uname = spo_info[0].infos;
	var old_dname = spo_info[2].infos;

	var htmlNodes_spo_info = '<h3 style="color:green" align="center">user: '+old_uname+' device: '+old_dname+'</h3>';
	$('#userinfo').html(htmlNodes_spo_info);
}
//-------------Spotify handle end-------------//


/**********************************************
 * Swupdate handle
 *********************************************/
function system(){
	var swupdate_href = '<li><a href="http://'+window.location.host+':8080" class="list-group-item" role="button">Swupdate</a></li>';
	$('#swupdate').html(swupdate_href);

	send_commond("get_deviceinfo",show_device_info);
}

function show_device_info(command_xmlhttp){

	send_commond("runswupdate");
	var response_dev_info = command_xmlhttp.responseText;
	if(response_dev_info == "")
		return;
	var dev_info = eval("("+response_dev_info+")");
	var htmlNodes_dev_info = '';

	htmlNodes_dev_info += '<li class="list-group-item" align="center">KERNEL :' + dev_info[0].info+'</a></li>'
	htmlNodes_dev_info += '<li class="list-group-item" align="center">ARCH :' + dev_info[8].info+'</a></li>'
	htmlNodes_dev_info += '<li class="list-group-item" align="center">SSID :' + dev_info[1].info+'</a></li>'
	htmlNodes_dev_info += '<li class="list-group-item" align="center">MODE :' + dev_info[2].info+'</a></li>'
	htmlNodes_dev_info += '<li class="list-group-item" align="center">CIPHER :' + dev_info[3].info+'</a></li>'
	htmlNodes_dev_info += '<li class="list-group-item" align="center">KEYMGMT :' + dev_info[4].info+'</a></li>'
	htmlNodes_dev_info += '<li class="list-group-item" align="center">STATE :' + dev_info[5].info+'</a></li>'
	htmlNodes_dev_info += '<li class="list-group-item" align="center">WIREIP :' + dev_info[6].info+'</a></li>'
	htmlNodes_dev_info += '<li class="list-group-item" align="center">MAC :' + dev_info[7].info+'</a></li>'

	$('#dev_info_show').html(htmlNodes_dev_info);
}

function stop_swupdate(){
	send_commond("endswupdate");
}
//-----------Swupdate handle end-----------//

/***********************************************
 * Soundbar handle
 **********************************************/
function send_audio_command(command, callback,cgi)
{
  var cgi = cgi || "cgi-bin/soundbar.cgi";
	command_xmlhttp = GetXmlHttpObject();               //create req obj
	command_xmlhttp.onreadystatechange = function () {        //get data from web action
		handleCommand(command_xmlhttp, callback);
	}
	var url = "";
	url = cgi + "?command=" +command;
	command_xmlhttp.open("GET",url,true);
	command_xmlhttp.setRequestHeader("If-Modified-Since","0");
	command_xmlhttp.setRequestHeader("Cache-Control","no-cache");
	command_xmlhttp.setRequestHeader("CONTENT-TYPE","application/x-www-form-urlencoded");
	command_xmlhttp.send(null);
}

function get_audio_input_list()
{
	var	wifi_json = send_audio_command("showinput&", set_input_list_to_select);
	show_loading();
	setTimeout(function () { $('#loading').hide(); }, 3000);
}

function set_audio_input()
{
	setTimeout(function(){$("#mymodal").modal("hide")},2000);
	var input_ssid = document.getElementById("modal_ssid").value;
	var	wifi_json = send_audio_command("enable-input&value="+input_ssid, set_input_list_to_select);

	//set_wifi(input_ssid,input_pwd);
}

function set_input_list_to_select(command_xmlhttp)
{
	var response_date = command_xmlhttp.responseText;

	obj_Data = eval("("+response_date+")");
	if(obj_Data.length == 0){
		get_audio_input_list();
	}
	else{
		var htmlNodes = '';

		for(var i = 0; i < obj_Data.length; i++){
			if(obj_Data[i].enabled)
				htmlNodes += '<a class="list-group-item" id="input_' + i + '"' + ' role="button" data-toggle="modal" data-target="#myModal">' + obj_Data[i].name + '  *' + '</a>';
			else
				htmlNodes += '<a class="list-group-item" id="input_' + i + '"' + ' role="button" data-toggle="modal" data-target="#myModal">' + obj_Data[i].name + '</a>';
		}
		htmlNodes += '</ul>';

		$('#testtext').html(htmlNodes);

		var j;
		var index = 1;
		var wifi_select = document.getElementById("ssid");
		for(j = 0; j < obj_Data.length; j++){
			var wifi = new Object;
			wifi.ssid = obj_Data[j].name;
			//wifi_select.Option.add(wifi.id);
		}
	}
    get_audio_system_info()
}

$("#audiovolume")[0].oninput = function(){
    change_audio_volume(this.value);
}

function change_audio_volume(audiovolume)
{
	var wifi_json = send_audio_command("set-volume&value="+audiovolume);
}


function update_audio_system_info(command_xmlhttp)
{
	var response_date = command_xmlhttp.responseText;

	obj_Data = eval("("+response_date+")");
	if(obj_Data.length == 0){
		get_audio_system_info();
	}
	else{
       // <input id="audiovolume" type="range" min="0" max="100" value="60" />
		for(var i = 0; i < obj_Data.length; i++){
            if(obj_Data[i].name == "volume"){
                var audiovolumetest = document.getElementById("audiovolume");
                audiovolumetest.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "mute"){
                var audiomute = document.getElementById("audiomute");
                audiomute.checked = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq_status"){
                var eqstatus = document.getElementById("eq_status");
                eqstatus.checked = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq1"){
                var eq = document.getElementById("eqi1");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq2"){
                var eq = document.getElementById("eqi2");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq3"){
                var eq = document.getElementById("eqi3");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq4"){
                var eq = document.getElementById("eqi4");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq5"){
                var eq = document.getElementById("eqi5");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq6"){
                var eq = document.getElementById("eqi6");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq7"){
                var eq = document.getElementById("eqi7");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq7"){
                var eq = document.getElementById("eqi7");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq8"){
                var eq = document.getElementById("eqi8");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq9"){
                var eq = document.getElementById("eqi9");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq10"){
                var eq = document.getElementById("eqi10");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq11"){
                var eq = document.getElementById("eqi11");
                eq.value = obj_Data[i].value;
            } else if(obj_Data[i].name == "eq12"){
                var eq = document.getElementById("eqi12");
                eq.value = obj_Data[i].value;
            }
		}
	}
	//check_wifi_connected();
}

function get_audio_system_info()
{
    var wifi_json = send_audio_command("getsysteminfo&", update_audio_system_info);
}

function set_audio_mute()
{
	var audio_mute = document.getElementById("audiomute").checked;
	var	wifi_json = send_audio_command("set-mute&value="+audio_mute);

	//set_wifi(input_ssid,input_pwd);
}



function audio_reset_eq()
{
    var wifi_json = send_audio_command("reset-EQ&");
    get_audio_system_info();
}

//$("#eqi1")[0].oninput = function(){
//    var wifi_json = send_audio_command("set-eq1&value="+this.value);
//}

function change_audio_equalizer(index, eq)
{
    var wifi_json = send_audio_command("set-eq"+index+"&value="+eq);
}

function audio_dataplay_play()
{
    var playstate = document.getElementById("audioplaystate");
    var format = document.getElementById("audioplayformat");
    if(playstate.value == "Play") {
        send_audio_command("dataplay-start&value="+format.value);
        playstate.value = "Stop"
    } else if(playstate.value == "Stop") {
        send_audio_command("dataplay-stop&");
        playstate.value = "Play"
    }
}

function set_audio_EQEnabled()
{
	var enabled = document.getElementById("eq_status").checked;
	var	wifi_json = send_audio_command("set-EQ&value="+enabled);
	//set_wifi(input_ssid,input_pwd);
}
//-----------Soundbar handle end-----------//
