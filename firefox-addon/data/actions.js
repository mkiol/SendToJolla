/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone add-on.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

var setButton = document.getElementById("setButton");
var openButton = document.getElementById("openButton");
var getButton = document.getElementById("getButton");
var cleanButton = document.getElementById("cleanButton");
var textBox = document.getElementById("textBox");
var statusLabel = document.getElementById("statusLabel");

setButton.addEventListener('click', function (event) {
  addon.port.emit("set-clipboard",textBox.value);
}, false);

openButton.addEventListener('click', function (event) {
  addon.port.emit("open-url");
}, false);

getButton.addEventListener('click', function (event) {
  addon.port.emit("get-clipboard");
}, false);

cleanButton.addEventListener('click', function (event) {
  textBox.value = "";
  cleanButton.disabled = true;
  setButton.disabled = true;
}, false);

textBox.addEventListener('keyup', function onkeyup(event) {
  if (textBox.value != "") {
    setButton.disabled = false;
    cleanButton.disabled = false;
  } else {
    setButton.disabled = true;
    cleanButton.disabled = true;
  }
}, false);

addon.port.on("show", function () {
  if (textBox.value != "") {
    setButton.disabled = false;
    cleanButton.disabled = false;
  } else {
    setButton.disabled = true;
    cleanButton.disabled = true;
  }
});

addon.port.on("show-open-button", function () {
  openButton.disabled = false;
});

addon.port.on("hide-open-button", function () {
  openButton.disabled = true;
});

addon.port.on("status-changed", function (status) {
  statusLabel.textContent = status;
});

addon.port.on("get-clipboard-response", function (text) {
  textBox.value = text;
  if (textBox.value != "") {
    setButton.disabled = false;
    cleanButton.disabled = false;
  } else {
    setButton.disabled = true;
    cleanButton.disabled = true;
  }
});
