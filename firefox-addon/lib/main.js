/*
Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

This file is part of SendToPhone add-on.

This Source Code Form is subject to the terms of
the Mozilla Public License, v.2.0. If a copy of
the MPL was not distributed with this file, You can
obtain one at http://mozilla.org/MPL/2.0/.
*/

var sendAPI = require("sendAPI");

// Button

var { ToggleButton } = require('sdk/ui/button/toggle');
var panels = require("sdk/panel");
var self = require("sdk/self");

var button = ToggleButton({
  id: "send-to-phone",
  label: "Send to Phone",
  icon: {
    "16": "./icon-16.png",
    "32": "./icon-32.png",
    "64": "./icon-64.png"
  },
  onChange: handleChange
});

var panel = panels.Panel({
  contentURL: self.data.url("panel.html"),
  height: 250,
  onHide: handleHide,
  onShow: handleShow
});

function handleChange(state) {
  if (state.checked) {
    panel.show({
      position: button
    });
  }
}

function handleShow() {
  var tabs = require("sdk/tabs");
  panel.port.emit("show");
  if (sendAPI.isUrl(tabs.activeTab.url))
    panel.port.emit("show-open-button");
  else
    panel.port.emit("hide-open-button");
}

function handleHide() {
  button.state('window', {checked: false});
}

function responseHandle(response) {
  if (response.status == 0) {
    panel.port.emit("status-changed","Server is unreachable!");
    return;
  }

  if (response.status <= 299) {
    panel.port.emit("status-changed","Ok!");
    return;
  }

  panel.port.emit("status-changed","Server reponses with error!");
}

function getClipboardResponseHandle(response) {
  if (response.status == 0) {
    panel.port.emit("status-changed","Server is unreachable!");
    return;
  }

  if (response.status <= 299) {
    panel.port.emit("status-changed","Ok!");
    panel.port.emit("get-clipboard-response",response.text);
    return;
  }

  panel.port.emit("status-changed","Server reponses with error!");
}

panel.port.on("open-url", function () {
  var tabs = require("sdk/tabs");
  if (sendAPI.invokeOpenUrlAction(tabs.activeTab.url,responseHandle))
    panel.port.emit("status-changed","Requesting...");
  else
    panel.port.emit("status-changed","Server URL is invalid or not yet configured!");
  //panel.hide();
});

panel.port.on("set-clipboard", function (text) {
  if (sendAPI.invokeSetClipboardAction(text,responseHandle))
    panel.port.emit("status-changed","Requesting...");
  else
    panel.port.emit("status-changed","Server URL is invalid or not yet configured!");
  //panel.hide();
});

panel.port.on("get-clipboard", function () {
  if (sendAPI.invokeGetClipboardAction(getClipboardResponseHandle))
    panel.port.emit("status-changed","Requesting...");
  else
    panel.port.emit("status-changed","Server URL is invalid or not yet configured!");
  //panel.hide();
});

// Context menu

var cm = require("sdk/context-menu");
cm.Item({
  label: "Send text to Phone",
  image: self.data.url("icon-16.png"),
  context: cm.SelectionContext(),
  contentScriptFile: self.data.url("content-script.js"),
  onMessage: function (text) {
    sendAPI.invokeSetClipboardAction(text);
  }
});
