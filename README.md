# Send To Jolla

Sailfish app and Firefox add-on that allows you to send
URLs and text between Firefox and your Jolla phone via WiFi.

## Downloads

* Sailfish app (server): [OpenRepos repository](https://openrepos.net/content/mkiol/send-jolla)
* Firefox add-on (client): [addons.mozilla.org page](https://addons.mozilla.org/firefox/addon/send-to-phone-jolla/)

## API description

*Send To Jolla* implements HTTP server that expose an API for client applications.

### Server URL

*Send To Jolla* API is accessible at server URL. URL has the following form:

`http://[IP address]:[port]/[cookie]`

where:

* `IP address` is phone'e IP address on WLAN interface,
* `port` is server listening port (by default is 9090),
* `cookie` is 5 character random string generated when *Send to Jolla* app is installed on phone.

Example:

`http://192.168.1.1:9090/cRKTI`

### Methods

#### Open URL
Opens URL in default phone's browser.

Has the following form:

|Method|URL|Content|
|:--:|:--:|:--:|
|GET|`[server URL]/open-url/[url]`|-|

where:

* `server URL` is server URL,
* `url` is percent encoded URL of page to open.

Returns `204 No Content` response upon success.

URL example:

`http://192.168.1.1:9090/cRKTI/open-url/https%3A%2F%2Fjolla.com%2`

#### Set clipboard text
Sets text in phone's clipboard.

Has the following form:

|Method|URL|Content|
|:--:|:--:|:--:|
|POST|`[server URL]/set-clipboard`|`[text]`|

where:

* `server URL` is server URL,
* `text` text to be inserted into phone's clipboard.

Returns `204 No Content` response upon success.

URL example:

`http://192.168.1.1:9090/cRKTI/set-clipboard`

#### Get clipboard text
Gets text from phone's clipboard.

Has the following form:

|Method|URL|Content|
|:--:|:--:|:--:|
|GET|`[server URL]/get-clipboard`|-|

where:

* `server URL` is server URL.

Returns `200 OK` response upon success. Response contains phone's clipboard text.

URL example:

`http://192.168.1.1:9090/cRKTI/get-clipboard`

---------------

This software is distributed under
[Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/).
