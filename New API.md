# *Send to Phone* API description

*Send To Phone* implements HTTP server that expose an RESTful API.

Following methods are supported:
* [Web client](#web-client)
* [Options](#options)
* [Open URL](#open-url)
* [Set clipboard text](#set-clipboard-text)
* [Get clipboard text](#get-clipboard-text)

## Server URL

*Send To Phone* API is accessible at server URL. URL has the following form:

`http://[IP address]:[port]/?app=[cookie]`

* `IP address` is a device IP address on WLAN interface,
* `port` is a server listening port (by default is 9090),
* `cookie` is 5 character random string generated when *Send to Phone* app is installed

#### Example

`http://192.168.1.1:9090/?app=cRKTI`

## Methods

### Web client
Opens web application.

|Method|URL|Content|
|:--:|:--:|:--:|
|GET|`[server URL]&api=web-client`|-|

#### Example

Request:

`http://192.168.1.1:9090/?app=cRKTI&api=web-client`

### Options

Gets server parameters.

|Method|URL|Content|
|:--:|:--:|:--:|
|GET|`[server URL]&api=options`|-|

* `server URL` is server URL

Returns `200 OK` response upon success. Response contains JSON object with following parameters:

|Parameter         |Type   |Description                                                 |
|:----------------:|:-----:|:----------------------------------------------------------:|
|platform          |String |Name of the platform that runs server (`sailfish` or `bb10`)|
|encryption_enabled|Boolean|Encryption enabled/disabled on the server side              |
|api_version       |String |API version supported by server                             |
|api_list          |Array  |List of methods supported by server                         |

#### Example

Request:

`http://192.168.1.1:9090/?app=cRKTI&api=options`

Response:
```
{
  "platform": "sailfish",
  "encryption_enabled": false,
  "api_version": "2.1",
  "api_list":[
    "options",
    "open-url",
    "get-clipboard",
    "set-clipboard",
    "get-bookmarks",
    "update-bookmark",
    "create-bookmark",
    "delete-bookmark",
    "get-bookmarks-file",
    "set-bookmarks-file",
    "get-notes-list",
    "get-note",
    "delete-note",
    "update-note",
    "create-note",
    "get-contacts",
    "get-contact",
    "create-contact",
    "update-contact",
    "delete-contact",
    "web-client"
    ]
}
```

### Open URL
Opens URL in default phone's browser.

|Method|URL|Content|
|:--:|:--:|:--:|
|GET|`[server URL]&api=open-url&data=[url]`|-|

* `server URL` is server URL,
* `url` is percent encoded URL of page to open.

Returns `204 No Content` response upon success.

#### Example

`http://192.168.1.1:9090/?app=cRKTI&api=open-url&data=https%3A%2F%2Fjolla.com%2`

### Set clipboard text
Sets text in phone's clipboard.

Has the following form:

|Method|URL|Content|
|:--:|:--:|:--:|
|POST|`[server URL]&api=set-clipboard`|`[text]`|

* `server URL` is server URL,
* `text` text to be inserted into phone's clipboard.

Returns `204 No Content` response upon success.

#### Example

`http://192.168.1.1:9090/?app=cRKTI&api=set-clipboard`

### Get clipboard text
Gets text from phone's clipboard.

Has the following form:

|Method|URL|Content|
|:--:|:--:|:--:|
|GET|`[server URL]&api=get-clipboard`|-|

* `server URL` is server URL.

Returns `200 OK` response upon success. Response contains phone's clipboard text.

#### Example

`http://192.168.1.1:9090/?app=cRKTI&api=get-clipboard`

---------------

This software is distributed under
[Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/).
