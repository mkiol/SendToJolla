# Send To Phone

Mobile app, Web client and Firefox add-on that simplifies clipboard,
notes & bookmarks transfer between the PC and your phone.

This app is for those who want to, in simple, secure and handy way,
send text data between PC and mobile phone.
To make it work, only your private infrastructure is used,
so your data will never be transferred to any third parties.

## Downloads

* Sailfish app: [OpenRepos repository](https://openrepos.net/content/mkiol/send-phone)
* BlackBerry 10 app: [BlackBerry World](https://appworld.blackberry.com/webstore/content/59953449/?countrycode=PL&lang=en)
* Firefox add-on: [addons.mozilla.org page](https://addons.mozilla.org/firefox/addon/send-to-phone-jolla/)

## Connection modes

*Send To Phone* app supports two connection modes: *Local server* and *Proxy*.

In *Local server* mode app acts as local web server. This mode is recommended for cases where your phone and PC are in the same local network, so data can be transferred directly between these two devices.

In *Proxy mode*, connection between phone and PC is done via Proxy server. This mode is especially recommended for cases where your phone and PC are not the same network and data can't be transferred directly. Typical scenario is when your phone is connected to Internet via cellular network. Required *Proxy server* is a single file PHP script that have to be deployed on private WWW server.

## Proxy script

In order to use *Send to Phone* in the *Proxy mode*, single file PHP 5 script have to be deployed on WWW server. This script will act as Proxy server.

Script can be downloaded from [here](https://github.com/mkiol/SendToJolla/blob/master/proxy/).

## API description (deprecated)

*Send To Phone* implements HTTP server that expose an API.
API is public and can be used by anyone who want to create client application.

API description is located [here](https://github.com/mkiol/SendToJolla/blob/master/API.md).

---------------

This software is distributed under
[Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/).
