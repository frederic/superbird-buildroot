// JavaScript Document

$(function(){
	var i = 0;
	var process1 = false;
	var process2 = false;

	$(".list li").last().css({"border-bottom-left-radius":"20px","border-bottom-right-radius":"20px"});
	$(".list li").first().css({"border-top-left-radius":"20px","border-top-right-radius":"20px"});

	if($.browser.msie&&($.browser.version < "9.0"))
{$(".button").addClass("buttonIE");}

$(".button").click(function(){
	if(process1){}
	else{
		process1 = true;
		$(this).toggleClass("hover");
		if(i==0){
			$(".load").fadeTo(400,1,slideDown);
		}
		else{
			$(".load").fadeTo(100,0,slideUp);
		}
}
function slideDown(){
	$(".list").slideToggle(100);
	$(".load img").hide();
	i = 1;process1 = false;
}
function slideUp(){
	$(".list").slideToggle(100);
	$(".load img").show();
	i = 0;process1 = false;
}

$(".list li").hover(function(){
	$(this).find(".signal").addClass("hover");
},function(){
	$(this).find(".signal").removeClass("hover");
})

$(".floatBox").center();
$(".list li").click(function(){
	if($.browser.msie&&($.browser.version < "9.0")){}
	else if(process2){}
	else{
		process2 = true;
		$(".mask").show();
		$(".floatBox").fadeTo(300,1);
	}
});

$(".ok,.cancel").click(function(){
	$(".mask").hide();
	$(".floatBox").hide(0,finish);
});

function finish(){
	process2 = false;
}
})
jQuery.fn.center = function () {
	this.css('position','absolute');
	this.css('top', ( $(window).height() - this.height() ) /2 +$(window).scrollTop() + 'px');
	this.css('left', ( $(window).width() - this.width() ) / 2+$(window).scrollLeft() + 'px');
	return this;
}
})
