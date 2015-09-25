/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

// Global vars
var serverURL;
var VERSION = "2.0";

// Checks if URL syntax is valid
// Source: http://dzone.com/snippets/validate-url-regexp
function isUrl(s) {
  var re = /(http|https):\/\/(\w+:{0,1}\w*@)?(\S+)(:[0-9]+)?(\/|\/([\w#!:.?+=&%@!\-\/]))?/;
  return re.test(s);
}

function getServerUrl() {
  return window.location.protocol +
    "//" + window.location.host +
    "/" + window.location.pathname.split('/')[1];
}

$(document).ready(function() {

  serverURL = getServerUrl();

  // Initial reset
  $(".loader").hide();
  $(".info").hide();
  $("div.page").hide();
  $(".version").text(VERSION);

  // --- Menu

  var page = window.location.hash == "" ? "#browser" : window.location.hash;
  $(page).show();
  $(page + "MenuItem").addClass("active");

  $("#menu ul li a").click(function() {
    $("div#menu ul li a").removeClass("active");
    $(this).addClass("active");
    $("div.page").hide();
    $($(this).attr("href")).show();
	  $("html, body").animate({scrollTop:0},"fast");
  });

  // --- Bookmarks

  var bookmarksList;

  $("#updateBookmark").hide();
  $("#deleteBookmark").hide();

  function clearDeleteBookmark() {
	setTimeout(function() {
  		var button = $("#deleteBookmark");
  		button.data("firstclickdone", false);
  		button.text("Delete bookmark");
  	}, 1000);
  }

  var getBookmarksAPICall = function() {
    var info = $("#bookmarks span.info");
    var loader = $("#bookmarks img.loader");
    var table = $("#bookmarks table");

    $("#bookmarks table tr").remove();
    info.removeClass("error");
    info.hide();

    loader.show();
    $.get(serverURL + "/get-bookmarks/", function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) info.addClass("error");
      info.text(ok ? data == "" ? "OK, You don't have any bookmarks." : "OK" : "Something goes wrong!");
      info.show();

      bookmarksList = data;
      if (ok) {
        if (data == "") {
      		$("#updateBookmark").hide();
      		$("#deleteBookmark").hide();
      		$("#bookmarkId").val("");
          table.append('<tr><td class="empty">Empty</td></tr>');
        } else {
            if (data.length > 0) {
              for (var i in data) {
                var row = $("<tr></tr>");
    			      row.append($("<td></td>").text(data[i].id));
                row.append($("<td></td>").append($("<img/>")
                .attr("src",data[i].icon == undefined || data[i].icon == "" ?
                            "icon-launcher-bookmark.png" : data[i].icon)
                .addClass("icon")));
                row.append($("<td></td>").append($('<a href="#"></a>')
                .data("bookmarkId",data[i].id)
                .text(data[i].title == "" ?
                      data[i].url.length > 50 ? data[i].url.substring(0,50) : data[i].url :
                      data[i].title.length > 50 ? data[i].title.substring(0,50) : data[i].title)
                .click(function (){
      				    var id = $(this).data("bookmarkId") - 1;
          				$("#bookmarkId").val(bookmarksList[id].id);
          				$("#bookmarkName").val(bookmarksList[id].title);
          				$("#bookmarkUrl").val(bookmarksList[id].url);
          				$("#updateBookmark").show();
          				$("#deleteBookmark").show();
    			      })));
                table.append(row);
              }
            } else {
        			$("#updateBookmark").hide();
        			$("#deleteBookmark").hide();
        			$("#bookmarkId").val("");
    			    table.append('<tr><td class="empty">Empty</td></tr>');
    		    }
        }
      }
    });
  };

  var openUrlAPICall = function() {
    var info = $("#openurl span.info");
    var loader = $("#openurl img.loader");
    var url = $("#openurl input").val();

    info.removeClass("error");
    info.hide();

    if (url == "") {
      info.addClass("error");
      info.text("URL is not set!");
	    info.show();
      return;
    }
    if (!isUrl(url)) {
      info.addClass("error");
      info.text("URL is invalid!");
	    info.show();
      return;
    }

    loader.show();
    $.get(serverURL + "/open-url/" + encodeURIComponent(url), function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) info.addClass("error");
      info.text(ok ? "OK" : "Something goes wrong!");
	    info.show();
    });

  }

  $("#openurl input").focus(function() {
    var info = $("#browser span.info");
	  info.hide();
  });

  $("#openurl input").bind('keyup', function(e) {
    if (e.keyCode == 13) {
      openUrlAPICall();
    }
  });

  $("#createBookmark").click(function() {
    var info = $("#bookmarks span.info");
    var loader = $("#bookmarks img.loader");
    var title = $("#bookmarkName").val();
    var url = $("#bookmarkUrl").val();

    $("#updateBookmark").hide();
    $("#deleteBookmark").hide();
    $("#bookmarkId").val("");

    info.removeClass("error");
    info.hide();

    if (title == "") {
      info.addClass("error");
      info.text("Bookmark title is not set!");
      info.show();
      return;
    }

    if (url == "") {
      info.addClass("error");
      info.text("Bookmark URL is not set!");
      info.show();
      return;
    }

    if (!isUrl(url)) {
      info.addClass("error");
      info.text("Bookmark URL is invalid!");
      info.show();
      return;
    }

    var content = {"title": title, "url": url};

    loader.show();
    $.ajax({
      type: 'POST',
      url: serverURL + "/create-bookmark",
      data: JSON.stringify(content),
      complete: function() {
        loader.hide();
      },
      success: function(data) {
        info.text("OK");
        info.show();
        getBookmarksAPICall();
      },
      error: function(xhr, status, error) {
        info.text("Something goes wrong!");
        info.addClass("error");
      },
      contentType: "application/json; charset=utf-8",
      dataType: 'json'
    });
  });

  $("#updateBookmark").click(function() {
    var info = $("#bookmarks span.info");
    var loader = $("#bookmarks img.loader");
    var title = $("#bookmarkName").val();
    var url = $("#bookmarkUrl").val();
    var id = $("#bookmarkId").val();

    info.removeClass("error");
    info.hide();

    if (id < 0) {
      info.addClass("error");
      info.text("Bookmark ID is unknown!");
      info.show();
      return;
    }

    if (title == "") {
      info.addClass("error");
      info.text("Bookmark title is not set!");
      info.show();
      return;
    }

    if (url == "") {
      info.addClass("error");
      info.text("Bookmark URL is not set!");
      info.show();
      return;
    }

    if (!isUrl(url)) {
      info.addClass("error");
      info.text("Bookmark URL is invalid!");
      info.show();
      return;
    }

    var content = {"title": title, "url": url};

    loader.show();
    $.ajax({
      type: 'POST',
      url: serverURL + "/update-bookmark/" + id,
      data: JSON.stringify(content),
      complete: function() {
        loader.hide();
      },
      success: function(data) {
        info.text("OK");
        info.show();
        getBookmarksAPICall();
      },
      error: function(xhr, status, error) {
        info.text("Something goes wrong!");
        info.addClass("error");
      },
      contentType: "application/json; charset=utf-8",
      dataType: 'json'
    });
  });

  $("#deleteBookmark").data("firstclickdone",false).click(function() {
    var info = $("#bookmarks span.info");
    var loader = $("#bookmarks img.loader");
	  var id = $("#bookmarkId").val();

    info.removeClass("error");
    info.hide();

  	if (id == "" || id < 1) {
  	  info.addClass("error");
  	  info.text("Bookmark ID is unknown!");
  	  info.show();
  	  return;
  	}

  	if (!$(this).data("firstclickdone")) {
  		$(this).data("firstclickdone",true);
  		$(this).text("Confirm bookmark deletion");
  		clearDeleteBookmark();
  		return;
  	}

    $("#bookmarkId").val("");
    $("#bookmarkName").val("");
    $("#bookmarkUrl").val("");
    $("#updateBookmark").hide();
    $(this).hide();

    loader.show();
    $.get(serverURL + "/delete-bookmark/" + id, function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (ok) {
        getBookmarksAPICall();
      } else {
        info.addClass("error");
        info.text("Something goes wrong!");
        info.show();
      }
    });

  });

  $("#bookmarkName").focus(function() {
    $(".info").removeClass("error").hide();
  });

  $("#bookmarkUrl").focus(function() {
    $(".info").removeClass("error").hide();
  });

  $("#openUrl").click(openUrlAPICall);
  $("#getBookmarks").click(getBookmarksAPICall);

  // --- Clipboard actions

  $("button#getClip").click(function() {
    var info = $("#clipboard span.info");
    var loader = $("#clipboard img.loader");

    info.removeClass("error");
    info.hide();

    loader.show();
    $.get(serverURL + "/get-clipboard/", function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) info.addClass("error");
      info.text(ok ? data == "" ? "OK, Clipboard is empty." : "OK" : "Something goes wrong!");
	  info.show();
      $("#clipboard textarea").val(data);
    });
  });

  $("button#setClip").click(function() {
    var info = $("#clipboard span.info");
    var loader = $("#clipboard img.loader");
    var content = $("#clipboard textarea").val();

	info.removeClass("error");
    info.hide();

	if (content == "") {
      info.addClass("error");
      info.text("Clipboard content is empty!");
	  info.show();
      return;
    }

    loader.show();
    $.post(serverURL + "/set-clipboard/", content, function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) info.addClass("error");
      info.text(ok ? "OK" : "Something goes wrong!");
	  info.show();
    });
  });

  $("#clipboard textarea").focus(function() {
    $(".info").removeClass("error").hide();
  });

  // --- Notes actions

  var pickedColor = 0;
  var availableColors = [
    "#cc0000", "#cc7700", "#ccbb00",
    "#88cc00", "#00b315", "#00bf9f",
    "#005fcc", "#0016de", "#bb00cc"];
  setPickedColor(0);

  $("#noteId").val("");
  $("#updateNote").hide();
  $("#deleteNote").hide();

  function clearDeleteNote() {
  setTimeout(function() {
      var button = $("#deleteNote");
      button.data("firstclickdone", false);
      button.text("Delete note");
    }, 1000);
  }

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
    info.hide();

    loader.show();
    $.get(serverURL + "/get-notes-list/", function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) info.addClass("error");
      info.text(ok ? data == "" ? "OK, You don't have any notes." : "OK" : "Something goes wrong!");
      info.show();
      if (data == "") {
        $("#updateNote").hide();
        $("#noteId").val("");
        table.append('<tr><td class="empty">Empty</td></tr>');
      } else {
        if (data.length > 0) {
          for (var i in data) {
            var row = $("<tr></tr>");
            row.append($("<td></td>").text(data[i].id));
            row.append($("<td></td>").addClass("colorRow")
              .css("background-color", data[i].color));
            row.append($("<td></td>").addClass("textRow")
              .append($('<a href="#"></a>').data("noteid", data[i].id)
                .text(data[i].text == "" ? "Empty note" : data[i].text)
                .click(openNoteAPICall)));
            table.append(row);
          }
        } else {
          $("#updateNote").hide();
          $("#noteId").val("");
          table.append('<tr><td class="empty">Empty</td></tr>');
        }
      }

    });
  };

  var openNoteAPICall = function() {
    var info = $("#notes span.info");
    var loader = $("#notes img.loader");
    var id = $(this).data("noteid");

    info.removeClass("error");
    info.hide();

    loader.show();
    $.get(serverURL + "/get-note/" + id, function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) info.addClass("error");
      info.text(ok ? "OK" : "Something goes wrong!");
      info.show();
      $("#notes textarea").val(data.text);
      $("#notes #noteId").val(data.id);
      var colorId = getColorId(data.color);
      if (colorId >= 0 && colorId < 10)
        setPickedColor(colorId);
      $("#updateNote").show();
      $("#deleteNote").show();
    })
  };

  $("button#getNotes").click(getNotesAPICall);

  $("button#updateNote").click(function() {
    var info = $("#notes span.info");
    var loader = $("#notes img.loader");
    var content = $("#notes textarea").val();
    var id = $("#noteId").val();

    info.removeClass("error");
    info.hide();

    if (id < 1) {
      info.addClass("error");
      info.text("Note ID is unknown!");
      info.show();
      return;
    }

    if (pickedColor < 0) {
      info.addClass("error");
      info.text("Note color is not set!");
      info.show();
      return;
    }

    if (content == "") {
      info.addClass("error");
      info.text("Note content is empty!");
      info.show();
      return;
    }

    loader.show();
    $.post(serverURL + "/update-note/" + id + "/" + encodeURIComponent(availableColors[pickedColor]), content, function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) {
        info.text("Something goes wrong!");
        info.addClass("error");
        info.show();
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

    $("#updateNote").hide();
    $("#deleteNote").hide();
    $("#noteId").val("");

    info.removeClass("error");
    info.hide();

    if (pickedColor < 0) {
      info.addClass("error");
      info.text("Note color is not set!");
      info.show();
      return;
    }

    if (content == "") {
      info.addClass("error");
      info.text("Note content is empty!");
      info.show();
      return;
    }

    loader.show();
    $.post(serverURL + "/create-note/" + encodeURIComponent(availableColors[pickedColor]), content, function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (!ok) {
        info.text("Something goes wrong!");
        info.addClass("error");
        info.show();
      } else {
        // Refresh notes list
        getNotesAPICall();
      }
    });
  });

  $("#deleteNote").data("firstclickdone",false).click(function() {
    var info = $("#notes span.info");
    var loader = $("#notes img.loader");
    var id = $("#noteId").val();

    info.removeClass("error");
    info.hide();

    if (id == "" || id < 1) {
      info.addClass("error");
      info.text("Note ID is unknown!");
      info.show();
      return;
    }

    if (!$(this).data("firstclickdone")) {
      $(this).data("firstclickdone",true);
      $(this).text("Confirm note deletion");
      clearDeleteNote();
      return;
    }

    $("#noteId").val("");
    $("#notes textarea").val("");
    $("#updateNote").hide();
    $(this).hide();

    loader.show();
    $.get(serverURL + "/delete-note/" + id, function(data, status) {
      loader.hide();
      var ok = status == "success" || status == "nocontent" || status == "notmodified";
      if (ok) {
        getNotesAPICall();
      } else {
        info.addClass("error");
        info.text("Something goes wrong!");
        info.show();
      }
    });

  });

  $("#noteId").focus(function() {
    $(".info").removeClass("error").hide();
  });

  $("#notes textarea").focus(function() {
    $(".info").removeClass("error").hide();
  });

});
