/**
* The main module of the 'Send to Jolla' Add-on.
* 
* Author: Michal Kosciesza <michal@mkiol.net>
* version: 1.0
*/

var tabs = require("sdk/tabs");
var self = require("sdk/self");

var buttons = require('sdk/ui/button/action');
var button = buttons.ActionButton({
  id: "jolla-link",
  label: "Open in Jolla",
  icon: {
    "16": "./icon-16.png",
    "32": "./icon-32.png",
    "64": "./icon-64.png"
  },
  onClick: invokeOpenUrlAction
});

var cm = require("sdk/context-menu");
cm.Item({
  label: "Send text to Jolla",
  image: self.data.url("icon-16.png"),
  context: cm.SelectionContext(),
  contentScript: 'self.on("click", function (node, data) {' +
                 '  var text = window.getSelection().toString();' +
                 '  self.postMessage(text);' +
                 '});'+
		 'self.on("context", function () {' +
                 '  var text = window.getSelection().toString();' +
                 '  if (text.length > 20)' +
                 '    text = text.substr(0, 20) + "...";' +
                 '  return "Send \'" + text + "\' to Jolla";' +
                 '});',
  onMessage: function (text) {
    invokeSetClipboardAction(text);
  }
});

// Source: http://dzone.com/snippets/validate-url-regexp
function isUrl(s) {
   var re = /(http|https):\/\/(\w+:{0,1}\w*@)?(\S+)(:[0-9]+)?(\/|\/([\w#!:.?+=&%@!\-\/]))?/;
   return re.test(s);
}

function getServerUrl() {
  var sps = require("sdk/simple-prefs").prefs;
  var url = sps['jollaUrl'];
  
  if (!isUrl(url)) {
      var notifications = require("sdk/notifications");
      notifications.notify({
	title: "Send to Jolla",
	text: "Server URL is invalid or not yet configured!",
	iconURL: self.data.url("icon64.png")
      });
      return null;
  }
  
  if (url[url.length-1]=="/")
    url = url.substring(0, url.length-1);

  return url;
}

function invokeSetClipboardAction(text) {
  var url = getServerUrl();
  if (!url)
    return;
  
  url = url+"/set-clipboard";
  sendRequest(url,text);
}

function invokeOpenUrlAction(state) {
  var url = getServerUrl();
  if (!url)
    return;
  
  url = url+"/open-url/" + encodeURIComponent(tabs.activeTab.url);
  //url = url+"/open-url/" + tabs.activeTab.url;
  sendRequest(url);
}

function sendRequest(url,data) {
  console.log("url:",url);
  console.log("data:",data);
  var Request = require("sdk/request").Request;
  
  if (data && data != "") {
    // POST request
    var query = Request({
      url: url,
      content: data,
      onComplete: function (response) {
	console.log("response:",response.status);
	if (response.status==0 || response.status > 399)
          showErrorNotification(response.status);
      }
    });
    query.post();
  } else {
    // GET request
    var query = Request({
      url: url,
      onComplete: function (response) {
	console.log("response:",response.status);
	if (response.status==0 || response.status > 399)
	  showErrorNotification(response.status);
      }
    });
    query.get();
  }
}

function showErrorNotification(status) {
  var notifications = require("sdk/notifications");
    notifications.notify({
      title: "Send to Jolla",
      text: status==0 ? "Server is unreachable!" : "Server reponses with error!",
      iconURL: self.data.url("icon64.png")
    });
}