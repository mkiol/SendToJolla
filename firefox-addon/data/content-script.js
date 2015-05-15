/*
Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

This file is part of SendToPhone add-on.

This Source Code Form is subject to the terms of
the Mozilla Public License, v.2.0. If a copy of
the MPL was not distributed with this file, You can
obtain one at http://mozilla.org/MPL/2.0/.
*/

self.on("click", function (node, data) {
  var text = window.getSelection().toString();
  self.postMessage(text);
});

self.on("context", function () {
  var text = window.getSelection().toString();
  if (text.length > 20)
    text = text.substr(0, 20) + "...";
  return "Send “" + text + "” to Phone";
}); 
