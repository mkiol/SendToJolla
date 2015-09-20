/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

// API server URL
var serverURL = document.URL;
if (serverURL.charAt(serverURL.length - 1) == '/')
  serverURL = serverURL.substr(0, serverURL.length - 1);

// Checks if URL syntax is valid
// Source: http://dzone.com/snippets/validate-url-regexp
function isUrl(s) {
  var re = /(http|https):\/\/(\w+:{0,1}\w*@)?(\S+)(:[0-9]+)?(\/|\/([\w#!:.?+=&%@!\-\/]))?/;
  return re.test(s);
}

$(document).ready(function() {

  // Initial reset
  $("img.loader").hide();
  $("div.page").hide();
  $("div#browser").show();
  $("a#browserMenuItem").addClass("active");

  // --- Menu

  $("a#browserMenuItem").click(function() {
    $("div#menu ul li a").removeClass("active");
    $(this).addClass("active");
    $("div.page").hide();
    $("div#browser").show();
  });
  $("a#clipboardMenuItem").click(function() {
    $("div#menu ul li a").removeClass("active");
    $(this).addClass("active");
    $("div.page").hide();
    $("div#clipboard").show();
  });
  $("a#notesMenuItem").click(function() {
    $("div#menu ul li a").removeClass("active");
    $(this).addClass("active");
    $("div.page").hide();
    $("div#notes").show();
  });
  $("a#aboutMenuItem").click(function() {
    $("div#menu ul li a").removeClass("active");
    $(this).addClass("active");
    $("div.page").hide();
    $("div#about").show();
  });

  // --- Open URL action

  // Calls Open URL API
  var openUrlAPICall = function() {
    var info = $("#browser span.info");
    var loader = $("#browser img.loader");
    var url = $("#browser input").val();

    info.removeClass("error");
    info.text("");

    if (url == "") {
      info.addClass("error");
      info.text("URL is not specified.");
      return;
    }
    if (!isUrl(url)) {
      info.addClass("error");
      info.text("URL is invalid.");
      return;
    }

    loader.show();
    $.get(serverURL + "/open-url/" + encodeURIComponent(url), function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) info.addClass("error");
      info.text(ok ? "OK" : "Something goes wrong!");
    });

  }

  $("#browser input").focus(function() {
    var info = $("#browser span.info");
    info.text("");
  });

  $("#browser input").bind('keyup', function(e) {
    if (e.keyCode == 13) {
      openUrlAPICall();
    }
  });

  $("button#openUrl").click(openUrlAPICall);

  // --- Clipboard actions

  $("button#getClip").click(function() {
    var info = $("#clipboard span.info");
    var loader = $("#clipboard img.loader");

    info.removeClass("error");
    info.text("");

    loader.show();
    $.get(serverURL + "/get-clipboard/", function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) info.addClass("error");
      info.text(ok ? data == "" ? "OK, Clipboard is empty." : "OK" : "Something goes wrong!");
      $("#clipboard textarea").val(data);
    });
  });

  $("button#setClip").click(function() {
    var info = $("#clipboard span.info");
    var loader = $("#clipboard img.loader");
    var content = $("#clipboard textarea").val();

    info.removeClass("error");
    info.text("");

    loader.show();
    $.post(serverURL + "/set-clipboard/", content, function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) info.addClass("error");
      info.text(ok ? "OK" : "Something goes wrong!");
    });
  });

  $("#clipboard textarea").focus(function() {
    var info = $("#clipboard span.info");
    info.text("");
  });

  // --- Notes actions

  var pickedColor = 0;
  var availableColors = [
    "#cc0000", "#cc7700", "#ccbb00",
    "#88cc00", "#00b315", "#00bf9f",
    "#005fcc", "#0016de", "#bb00cc"];
  setPickedColor(0);
  $("button#updateNote").hide();
  $("#noteId").val("");

  function setPickedColor(colorId) {
    if (colorId < 0 && colorId > 8) {
      colorId = 0;
    }
    var colorPicker = $("#colorPicker");
    $("#colorPicker div").remove();
    pickedColor = colorId;
    for (var i in availableColors) {
      var dot = $("<div></div>").addClass("colorRow").css("background-color",availableColors[i]).append($("<a href='#'></a>").data("colorid",i).click(function() {
        setPickedColor($(this).data("colorid"));
      }));
      if (i == pickedColor)
        dot.addClass("pickedColor");
      colorPicker.append(dot);
    }
  }

  function getColorId(color) {
    for (var i in availableColors) {
      if (availableColors[i] == color)
        return i;
    }
    return -1;
  }

  var getNotesAPICall = function() {
    var info = $("#notes span.info");
    var loader = $("#notes img.loader");
    var table = $("#notes table");

    $("#notes table tr").remove();
    info.removeClass("error");
    info.text("");

    loader.show();
    $.get(serverURL + "/get-notes-list/", function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) info.addClass("error");
      info.text(ok ? data == "" ? "OK, You don't have any notes." : "OK" : "Something goes wrong!");

      if (data == "") {
        $("button#updateNote").hide();
        $("#noteId").val("");
        table.append('<tr><td class="empty">Empty</td></tr>');
      } else {
        if (data.length > 0) {
          for (var i in data) {
            var row = $("<tr></tr>");
            row.append($("<td></td>").append($('<a href="#"></a>').data("noteid",data[i].id).text(data[i].id).click(openNoteAPICall)));
            row.append($("<td></td>").addClass("colorRow").css("background-color",data[i].color));
            row.append($("<td></td>").addClass("textRow").append($('<a href="#"></a>').data("noteid",data[i].id).text(data[i].text).click(openNoteAPICall)));
            table.append(row);
          }
        }
      }

    });
  };

  var openNoteAPICall = function() {
    var info = $("#notes span.info");
    var loader = $("#notes img.loader");
    var id = $(this).data("noteid");

    info.removeClass("error");
    info.text("");

    loader.show();
    $.get(serverURL + "/get-note/" + id, function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) info.addClass("error");
      info.text(ok ? "OK" : "Something goes wrong!");

      $("#notes textarea").val(data.text);
      $("#notes #noteId").val(data.id);
      var colorId = getColorId(data.color);
      if (colorId >= 0 && colorId < 10)
        setPickedColor(colorId);
      $("button#updateNote").show();
    })

  }

  $("button#getNotes").click(getNotesAPICall);

    $("button#updateNote").click(function() {
      var info = $("#notes span.info");
      var loader = $("#notes img.loader");
      var content = $("#notes textarea").val();
      var id = $("#noteId").val();

      info.removeClass("error");
      info.text("");

      if (id < 1) {
        info.addClass("error");
        info.text("Note ID is unknown!");
        return;
      }

      if (pickedColor < 0) {
        info.addClass("error");
        info.text("Note color is not set!");
        return;
      }

      if (content == "") {
        info.addClass("error");
        info.text("Note content is empty!");
        return;
      }

      loader.show();
      $.post(serverURL + "/update-note/" + id + "/" + encodeURIComponent(availableColors[pickedColor]), content, function(data, status) {
        loader.hide();
        var ok = status == "success" || status == "nocontent" || status == "notmodified";
        if (!ok) {
          info.text("Something goes wrong!");
          info.addClass("error");
        } else {
          // Refresh notes list
          getNotesAPICall();
        }
      });
});

  $("button#createNote").click(function() {
    var info = $("#notes span.info");
    var loader = $("#notes img.loader");
    var content = $("#notes textarea").val();

    $("button#updateNote").hide();
    $("#noteId").val("");

    info.removeClass("error");
    info.text("");

    if (pickedColor < 0) {
      info.addClass("error");
      info.text("Note color is not set!");
      return;
    }

    if (content == "") {
      info.addClass("error");
      info.text("Note content is empty!");
      return;
    }

    loader.show();
    $.post(serverURL + "/create-note/" + encodeURIComponent(availableColors[pickedColor]), content, function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) {
        info.text("Something goes wrong!");
        info.addClass("error");
      } else {
        // Refresh notes list
        getNotesAPICall();
      }
    });
});

});
