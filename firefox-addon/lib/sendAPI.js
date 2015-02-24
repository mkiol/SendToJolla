/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone add-on.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

module.exports = sendAPI = {};

var self = require("sdk/self");

// Source: http://dzone.com/snippets/validate-url-regexp
sendAPI.isUrl = function (s) {
   var re = /(http|https):\/\/(\w+:{0,1}\w*@)?(\S+)(:[0-9]+)?(\/|\/([\w#!:.?+=&%@!\-\/]))?/;
   return re.test(s);
}

function getServerUrl() {
  var sps = require("sdk/simple-prefs").prefs;
  var url = sps['serverUrl'];

  if (!sendAPI.isUrl(url)) {
      var notifications = require("sdk/notifications");
      notifications.notify({
  title: "Send to Phone",
  text: "Server URL is invalid or not yet configured!",
  iconURL: self.data.url("icon64.png")
      });
      return null;
  }

  if (url[url.length-1]=="/")
    url = url.substring(0, url.length-1);

  return url;
}

function sendRequest(url,data,resposeHandler) {
  console.log("url:",url);
  console.log("data:",data);
  var Request = require("sdk/request").Request;

  if (data && data != "") {
    // POST request
    var query = Request({
      url: url,
      content: data,
      onComplete: function (response) {
        if (resposeHandler)
          resposeHandler(response);
        //console.log("response:",response.status);
        //if (response.status==0 || response.status > 299)
        //  showErrorNotification(response.status);
      }
    });
    query.post();
  } else {
    // GET request
    var query = Request({
      url: url,
      onComplete: function (response) {
        if (resposeHandler)
          resposeHandler(response);
        //console.log("response:",response.status);
        //if (response.status==0 || response.status > 299)
        //  showErrorNotification(response.status);
      }
    });
    query.get();
  }
}

function showErrorNotification(status) {
  var notifications = require("sdk/notifications");
    notifications.notify({
      title: "Send to Phone",
      text: status==0 ? "Server is unreachable!" : "Server reponses with error!",
      iconURL: self.data.url("icon64.png")
    });
}

sendAPI.invokeSetClipboardAction = function (text,resposeHandler) {
  var url = getServerUrl();
  if (!url)
    return false;

  url = url+"/set-clipboard";
  sendRequest(url,text,resposeHandler);
  return true;
}

sendAPI.invokeOpenUrlAction = function (pageUrl,resposeHandler) {
  if (!sendAPI.isUrl(pageUrl))
    return false;

  var url = getServerUrl();
  if (!url)
    return false;

  url = url+"/open-url/" + encodeURIComponent(pageUrl);
  sendRequest(url,null,resposeHandler);

  return true;
}

sendAPI.invokeGetClipboardAction = function (resposeHandler) {
  var url = getServerUrl();
  if (!url)
    return false;

  url = url+"/get-clipboard";
  sendRequest(url,null,resposeHandler);
  return true;
}
