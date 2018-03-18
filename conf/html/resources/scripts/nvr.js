/**
 * nvr javascript libary v 0.1
 *
 * Copyright (c) 2012-2013 xiaowei chai
 * http://www.gentek.com.cn
 *
 * Dual licensed under the MIT and GPL licenses:
 *
 * 
 */


function load()
{
		get_systeminfo();
}
function restart()
{
	if(confirm("您确定要重启服务器"))
	{
		var url='/cmd?action=restart&callback=?';
		$.getJSON(url, function(data){});
	}
	else
	{
		alert("不重启了");
	}
}
function change_div_show(tab_id)
{
		obj=document.getElementById(tab_id);
		obj.setAttribute("className","tab-content default-tab");
		obj.setAttribute("class","tab-content default-tab");
		$("#"+tab_id).show().siblings().hide();
		if(tab_id=='tab1')
		{
			get_systeminfo();
		}
		if(tab_id=='tab2')
		{
			get_netinfo();
		}
		if(tab_id=='tab3')
		{
			get_devlist();
		}
		if(tab_id=='tab4')
		{
			get_devlist2();
		}
		if(tab_id=='tab5')
		{
		    get_devsip();
		}

}
 var getHost = function(url) { 
        var host = "null";
        if(typeof url == "undefined"
                        || null == url)
                url = window.location.href;
        var regex = /.*\:\/\/([^\/]*).*/;
        var match = url.match(regex);
        if(typeof match != "undefined"
                        && null != match)
                host = match[1];
        return host;
}
function sqlserver_connectted()
{
	var bconn = $.get(getHost("undefined"),null,null,text);
	if(bconn=="0")
		return false;
	else
		return true;
}

function get_devlist()
{
	var html="";
	var url='/get?name=devlist&callback=?';
	$.getJSON(url, function(data){
	if(data)
	{
		var devs = data[0].devlist;
		$("#dev_panel_list").empty();
		html = '<table><thead><tr><th><input class="check-all" type="checkbox" /></th><th>前端设备编号</th><th>前端设备名称</th><th>前端设备地址</th> <th>前端设备状态</th><th>设备操作</th></tr></thead>';
		html +='<tfoot><tr><td colspan="6"><div class="bulk-actions align-left"><select name="dropdown"><option value="option1">批量操作...</option><option value="option2">编辑</option><option value="option3">删除</option></select><a class="button" href="#">应用</a></div>';
		html +='<div class="pagination"><a href="#" title="First Page">&laquo; First</a><a href="#" title="Previous Page">&laquo; Previous</a><a href="#" class="number current" title="1">1</a><a href="#" title="Next Page">Next &raquo;</a><a href="#" title="Last Page">Last &raquo;</a></div> <!-- End .pagination --><div class="clear"></div></td></tr></tfoot><tbody>';
			for(var i = 0; i<devs.length; i++){
					if(i%2)
					{
						html+='<tr>';
					}
					else
					{
						html+='<tr class="alt-row">';
					}
					html+='<td><input type="checkbox" /></td><td>'+devs[i].id+'</td><td><a href="#" title="title">'+devs[i].name+'</a></td>';
					html+='<td>'+devs[i].ip+':'+devs[i].channel+'</td><td>';
					if(devs[i].status==1)
					{
						html+='正常';
					}
					else if(devs[i].status==0)
					{
						html+='未启用';
					}
					else if(devs[i].status==2)
					{
						html+='正在录像';
					}
					else if(devs[i].status==3)
					{
						html+='被删除';
					}
					else if(devs[i].status==4)
					{
						html+='故障';
					}
					
					html+='</td><td>';
					//html+='<a href="#" title="启用"><img src="resources/images/icons/pencil.png" alt="Edit" /></a>';
					//html+='<a href="#" title="停用"><img src="resources/images/icons/pencil.png" alt="Edit" /></a>';
					//html+='<a href="#" title="编辑"><img src="resources/images/icons/pencil.png" alt="Edit" /></a>';
					html+='<a href="#" title="删除" onclick="del_devbyid('+devs[i].id+')"><img src="resources/images/icons/cross.png" alt="Delete" /></a>&nbsp;&nbsp;' ;
					html+='<a href="#showpic" rel="modal" title="预览"  onclick="show_devpicbyid('+devs[i].id+')">预览</a></td></tr>';
			}	
		html+='</tbody></table>';
		document.getElementById("dev_panel_list").innerHTML=html;
		//document.getElementById("dev_panel_list").innerText = html;
	}
	});
	 
}
function get_devlist2()
{
	
	var html="";
	var url='/get?name=devlist&callback=?';
	$.getJSON(url, function(data){
	if(data)
	{
		var devs = data[0].devlist;
		$("#sub_dev_list_div_1").empty();
		html = '<select name="sub_dev_list" id="sub_dev_list">';
		for(var i = 0; i<devs.length; i++){
					html+='<option value="'+devs[i].id+'">&nbsp;&nbsp;'+devs[i].id+'&nbsp;'+devs[i].name+'</option>';
			}	
		html+='</select><a class="button" href="#" onclick="get_videotrack(0);">查询</a>';
		document.getElementById("sub_dev_list_div_1").innerHTML=html;
	}
	});
	 
}

function get_videotrack(st)
{
	var html="";
	var et = 0;
	if(st==0)
	{
		st= 0;
	}
	if(et==0)
	{
		et = parseInt(new Date().getTime()/1000);
	}
	var dev_id = document.getElementById("sub_dev_list").value;
	var url='/get?name=videotrack&id='+dev_id+'&st='+st+'&et='+et+'&callback=?';
	$.getJSON(url, function(data){
	if(data)
	{
		var tracks = data[0].videotrack;
		$("#dev_plan_panel_list").empty();
		html = '<table><thead><tr><th><input class="check-all" type="checkbox" /></th><th>录像段编号</th><th>录像段状态</th><th>录像开始时间</th> <th>录像结束时间</th></tr></thead>';
		html +='<tfoot><tr><td colspan="6">';
		html +='<div class="pagination"><a href="#" title="First Page" onclick="get_videotrack(0);">&laquo; First</a><a href="#" title="Previous Page" onclick="get_videotrack('+st+');">&laquo; Previous</a><a href="#" title="Next Page" onclick="get_videotrack('+st+');">Next &raquo;</a></div> <!-- End .pagination --><div class="clear"></div></td></tr></tfoot><tbody>';
		for(var i = 0; i<tracks.length; i++){
					if(i%2)
					{
						html+='<tr>';
					}
					else
					{
						html+='<tr class="alt-row">';
					}
					html+='<td><input type="checkbox" /></td><td>'+tracks[i].id+'</td><td>';
					if(tracks[i].status==0)
					{
						html+='<red>正在录像<red>';
					}
					else if(tracks[i].status==1)
					{
						html+='<green>录像完成<green>';
					}
					else if(tracks[i].status==2)
					{
						html+='<blue>录像已锁定<blue>';
					}
					
					html+='</td><td>'+tracks[i].st+'</td><td>'+tracks[i].et+'</td></tr>';
			}	
		html+='</tbody></table>';
		document.getElementById("dev_plan_panel_list").innerHTML=html;
		}
	});
}
function get_devsip()
{
    var url='/get?name=sipconfig&callback=?';
	$.getJSON(url, function(data){
	    var sipconfig = data[0].sipconfig;
	     $($("#sip_server"),$("#sip_pro")).empty();
	     $($("#sip_server"),$("#sip_pro")).append(sipconfig[0].sip_server_ip);
	     console.log(sipconfig[0].sip_server_ip);
	      $($("#sip_server_port"),$("#sip_pro")).empty();
	     $($("#sip_server_port"),$("#sip_pro")).append(sipconfig[0].sip_server_port);
	      $($("#sip_server_sip_id"),$("#sip_pro")).empty();
	     $($("#sip_server_sip_id"),$("#sip_pro")).append(sipconfig[0].sip_server_id);
	     $($("#dev_sip_id"),$("#sip_pro")).empty();
	     $($("#dev_sip_id"),$("#sip_pro")).append(sipconfig[0].media_server_id);
	      $($("#dev_sip_port"),$("#sip_pro")).empty();
	     $($("#dev_sip_port"),$("#sip_pro")).append(sipconfig[0].media_server_port);
	})
}

function add_dev()
{

  //alert(document.getElementById("sip_id").value);	
  var add_subdev_form = $("#add_subdev_form");
  var type = $($("#type"),add_subdev_form).val();
  console.log(type);
  var name = $($("#name"),add_subdev_form).val();
  console.log(name);
  var chnn = $($("#chnn"),add_subdev_form).val();
  var addr = $($("#addr"),add_subdev_form).val();
  var port = $($("#port"),add_subdev_form).val();
  var user = $($("#user"),add_subdev_form).val();
  var pass = $($("#pass"),add_subdev_form).val();
  var sip_id = $($("#sip_id"),add_subdev_form).val();
  var url="/cmd?action=adddev&type="+escape(type)+"&channel="+chnn+"&name="+name+"&addr="+addr+"&port="+port+"&user="+user+"&pass="+pass+"&sip_id="+sip_id+"&callback=?";
	//alert(url);
	console.log(url);
	
	$.getJSON(url, function(data){
	        console.log(data);
					if(data[0].ret==1)
					{
						alert("添加成功");
					}
					else
					{
						alert("添加失败");
					}
	});
	change_div_show("tab3");
}

function del_devbyid(dev_id)
{
	var url="/cmd?action=deldev&id="+dev_id+"&callback=?";
	$.getJSON(url, function(data){
	      console.log(data);
		    if(data[0].ret==1)
		    {
			    alert("删除成功");
		    }
		    else
		    {
			    alert("删除失败");
		    }
	    });
	change_div_show("tab3");
}


function sip_config()
{
  var config_sip_form = $("#config_sip_form");
  var sip_server_ip = $($("#sip_server_ip"),config_sip_form).val();
  var sip_server_port = $($("#sip_server_port"),config_sip_form).val();
  var sip_server_sip_id = $($("#sip_server_sip_id"),config_sip_form).val();
  var dev_sip_id = $($("#dev_sip_id"),config_sip_form).val();
  var dev_sip_port = $($("#dev_sip_port"),config_sip_form).val();
  var dev_sip_pass = $($("#dev_sip_pass"),config_sip_form).val();
  var url="/cmd?action=sipconfig&sip_server_ip="+sip_server_ip+"&sip_server_port="+sip_server_port+"&sip_server_id="+sip_server_sip_id+"&media_server_id="+dev_sip_id+"&media_server_port="+dev_sip_port+"&media_server_pass="+dev_sip_pass+"&callback=?";
  $.getJSON(url, function(data){
	      console.log(data);
		    if(data[0].ret==1)
		    {
			    alert("配置成功,重启后生效");
		    }
		    else
		    {
			    alert("配置失败");
		    }
	    });
	change_div_show("tab5");
}
function get_systeminfo()
{
	var url="/get?name=system&callback=?";
	$.getJSON(url, function(data){
	      console.log(data);
		    if(data[0].ret==1)
		    {
			    	var systeminfo = data[0];
	     			$($("#soft_name"),$("#base_info")).empty();
	     			$($("#soft_name"),$("#base_info")).append(systeminfo.nvr_type);
	     			
	     			$($("#soft_version"),$("#base_info")).empty();
	     			$($("#soft_version"),$("#base_info")).append(systeminfo.nvr_version);
	     			$($("#system_version"),$("#base_info")).empty();
	     			$($("#system_version"),$("#base_info")).append(systeminfo.system);
	     			$($("#cpu_version"),$("#base_info")).empty();
	     			$($("#cpu_version"),$("#base_info")).append(systeminfo.cpu);
		    }
		    else
		    {
			    alert("配置失败");
		    }
	    });
}
function get_netinfo()
{
	var url="/get?name=system&callback=?";
	$.getJSON(url, function(data){
	      console.log(data);
		    if(data[0].ret==1)
		    {
			    	var netinfo = data[0];
	     			$($("#ip_addr"),$("#net_info")).empty();
	     			$($("#ip_addr"),$("#net_info")).append(netinfo.ip);
	     			$($("#mask_addr"),$("#net_info")).empty();
	     			$($("#mask_addr"),$("#net_info")).append(netinfo.mask);
	     			$($("#gate_addr"),$("#net_info")).empty();
	     			$($("#gate_addr"),$("#net_info")).append(netinfo.gateway);
	     			$($("#dns_addr"),$("#net_info")).empty();
	     			$($("#dns_addr"),$("#net_info")).append(netinfo.dns);
	     			$($("#mac_addr"),$("#net_info")).empty();
	     			$($("#mac_addr"),$("#net_info")).append(netinfo.mac);
		    }
		    else
		    {
			    alert("配置失败");
		    }
	    });
}


function show_devpicbyid(dev_id)
{
	var url = "/images/"+dev_id+".jpg";
	$("#picshow").empty();
	showobj=document.getElementById("picshow");
	
/*
  var clientWidth, clientHeight;
  
  if(navigator.userAgent.indexOf(' MSIE ') > -1)
  {
      clientWidth = document.documentElement.clientWidth;
      clientHeight = document.documentElement.clientHeight;
  }
  else if (navigator.userAgent.indexOf(' Safari/') > -1)
  {
      clientWidth = window.innerWidth;
      clientHeight = window.innerHeight;
  }
  else if (navigator.userAgent.indexOf('Opera/') > -1)
  {
      clientWidth = Math.min(window.innerWidth, document.body.clientWidth);
      clientHeight = Math.min(window.innerHeight, document.body.clientHeight);
  }
  else
  {
      clientWidth = Math.min(window.innerWidth, document.documentElement.clientWidth);
      clientHeight = Math.min(window.innerHeight, document.documentElement.clientHeight);
  }
	
	alert(document.documentElement.scrollWidth);
	alert(document.documentElement.scrollHeigh);
	alert(document.body.scrollWidth);
	alert(document.body.scrollHeight);
 	showobj.style.width = Math.max(Math.max(document.documentElement.scrollWidth, document.body.scrollWidth), clientWidth)+'px';
  showobj.style.height = Math.max(Math.max(document.documentElement.scrollHeight, document.body.scrollHeight), clientHeight)+'px';
  showobj.style.display = '';
 */ 
  showobj.setAttribute("style","z-index: 99; right: 200px; position: absolute; top:200px; background-color: Gray;text-align: center;");
  $("#picshow").append("<img id='pic' src="+url+" alt='video show' onclick='hide_devpic()' />");
  showobj.style.display = '';
  toggleSelect('hidden');
}
function hide_devpic()
{
	/*
    $("#picshow").style.display = 'none';
    toggleSelect('visible');
    */
     $("#picshow").empty();
     $("#picshow").hide();
}
function toggleSelect(visibility)
{
    var ss = document.getElementsByTagName('select');

    for(var i = 0; i < ss.length; i++)
    {
        ss[i].style.visibility = visibility;
    }
}