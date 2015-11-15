<?php
/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

// Version: 1.0

// Consts

define('MAX_WAIT_IN', 600);
define('MAX_WAIT_OUT', 10);
define('WAIT_QUANTUM', 1);
define('WAIT_OFFSET', 2);

// Functions

function updateSessionState($app, $sid, $state) {
  $fp = fopen($app.'_'.$sid.'.session', 'w');
  fwrite($fp,json_encode(array('state' => $state, 'timestamp' => time())));
  fclose($fp);
}

function getSessionState($app, $sid, &$state, &$timestamp) {
  if (!file_exists($app.'_'.$sid.'.session')) {
    return false;
  }
  
  $json = json_decode(file_get_contents($app.'_'.$sid.'.session'));

  if (json_last_error() != JSON_ERROR_NONE) {
    return false;
  }

  $state = $json->{'state'};
  $timestamp = $json->{'timestamp'};
  return true;
}

function sessionExists($app, $sid) {
  // TODO check timestamp
  return file_exists($app.'_'.$sid.'.session');
}

function deleteSession($app, $sid) {
  $socket_list = glob($app.'_'.$sid.'*.*');
  if (count($socket_list) > 0) {
    array_map('unlink', $socket_list);
  }
}

function deleteSessions($app) {
  $socket_list = glob($app.'_*.*');
  if (count($socket_list) > 0) {
    array_map('unlink', $socket_list);
  }
}

function writeInSocket($app, $sid, $tid, $data) {
  $fp = fopen($app.'_'.$sid.'_'.$tid.'.socket_in', 'w');
  fwrite($fp, $data);
  fclose($fp);
}

function writeOutSocket($app, $sid, $tid, $data) {
  $fp = fopen($app.'_'.$sid.'_'.$tid.'.socket_out', 'w');
  fwrite($fp, $data);
  fclose($fp);
}

function readInSocket($app, $sid) {
  $socket_list = glob($app.'_'.$sid.'_*.socket_in');
  $data = '';
  foreach ($socket_list as $filename) {
    $di = file_get_contents($filename);
    if ($di == 'disconnect') {
      return 'disconnect';
    }
    $data .= ($di.';');
  }
  return substr($data, 0, strlen($data)-1);
}

function clearInSocket($app, $sid) {
  $socket_list = glob($app.'_'.$sid.'_*.socket_in');
  array_map('unlink', $socket_list);
}

function readOutSocket($app, $sid, $tid) {
  $filename = $app.'_'.$sid.'_'.$tid.'.socket_out';
  $data = '';
  if (file_exists($filename))
    $data = file_get_contents($filename);
  return $data;
}

function clearOutSocket($app, $sid, $tid) {
  $filename = $app.'_'.$sid.'_'.$tid.'.socket_out';
  if (file_exists($filename))
    unlink($filename);
}

function printPage($message) {
  echo $message;
}

// ---------

$int = $_GET['int'];

if ($int == 'in') {

  //
  // IN interface: app<->proxy
  //
  
  $json = json_decode($HTTP_RAW_POST_DATA);

  header('Content-type: application/json');

  // Checking if JSON is valid
  if (json_last_error() != JSON_ERROR_NONE) {
    http_response_code('400');
    echo json_encode(array('id' => 'error', 'type' => 1, 'msg' => 'parse error'));
    exit(1);
  }

  if ($json->{'app'} == '') {
    http_response_code('400');
    echo json_encode(array('id' => 'error', 'type' => 3, 'msg' => 'unknown app'));
    exit(1);
  }
  $app = $json->{'app'};

  // Checking SID
  $sid = 0;
  if ($json->{'sid'} != 0) {
    $sid = $json->{'sid'};
  }

  // Connect request handling
  if ($json->{'id'} == 'connect') {

    $wait_timer = MAX_WAIT_IN;
    if ($json->{'wait'} != 0) {
      // Wait timer setting
      if ($json->{'wait'} > MAX_WAIT_IN)
	$wait_timer = MAX_WAIT_IN;
      else
	$wait_timer = $json->{'wait'};
    }

    if ($sid != 0) {
      // Existing connection handler

      if (!sessionExists($app, $sid)) {
	http_response_code('400');
	echo json_encode(array('id' => 'error', 'type' => 66, 'msg' => 'unknown session'));
	exit(1);
      }

      // Main loop
      $end_time = time() + $wait_timer;
      while(time() < $end_time) {
      
      	if (!sessionExists($app, $sid)) {
	  http_response_code('500');
	  echo json_encode(array('id' => 'error', 'type' => 66, 'msg' => 'unknown session'));
	  deleteSession($app, $sid);
	  exit(1);
	}
	
	$data = readInSocket($app, $sid);
	
	if ($data == "") {
	
	  // No data
	  updateSessionState($app, $sid, 'waiting_for_data');
	  sleep(WAIT_QUANTUM);
	  
	} else if ($data == 'disconnect') {
	
	  // Disconnect request
	  http_response_code('200');
	  echo json_encode(array('id' => 'connect_ack', 'wait' => 0, 'sid' => $json->{'sid'}));
	  deleteSession($app, $sid);
	  exit(1);
	  
	} else {
	
	  http_response_code('200');
	  echo json_encode(array('id' => 'data', 'sid' => $sid, 'data' => $data));
	  clearInSocket($app, $sid);
	  updateSessionState($app, $sid, 'waiting_for_connect');
	  exit(1);
	  
	}
      }

      http_response_code('200');
      echo json_encode(array('id' => 'connect_ack', 'wait' => $wait_timer, 'sid' => $json->{'sid'}));
      updateSessionState($app, $sid, 'waiting_for_connect');
      exit(1);

    } else {
      // New connection handler

      // Deleting all active sessions for app
      deleteSessions($app);

      // New SID
      $sid = mt_rand(1,9999);

      http_response_code('200');
      echo json_encode(array('id' => 'connect_ack', 'wait' => $wait_timer, 'sid' => $sid));
      // Creating new settion
      updateSessionState($app, $sid, 'waiting_for_connect');
      exit(1);
    }

  }

  // Data request handling
  if ($json->{'id'} == 'data') {

    if (!sessionExists($app, $sid)) {
      http_response_code('400');
      echo json_encode(array('id' => 'error', 'type' => 6, 'msg' => 'unknown session'));
      exit(1);
    }

    if ($json->{'data'} == '') {
      deleteSession($app, $sid);
      http_response_code('400');
      echo json_encode(array('id' => 'error', 'type' => 7, 'msg' => 'no data'));
      exit(1);
    }
    
    if ($json->{'tid'} == '') {
      deleteSession($app, $sid);
      http_response_code('400');
      echo json_encode(array('id' => 'error', 'type' => 8, 'msg' => 'no tid'));
      exit(1);
    }

    writeOutSocket($app, $sid, $json->{'tid'}, $json->{'data'});

    echo json_encode(array('id' => 'data_ack', 'sid' => $sid));
    exit(1);

  }

  // Disconnect request handling
  if ($json->{'id'} == 'disconnect') {

    if (!sessionExists($app, $sid)) {
      http_response_code('400');
      echo json_encode(array('id' => 'error', 'type' => 6, 'msg' => 'unknown session'));
      exit(1);
    }
    
    writeInSocket($app, $sid, 0, 'disconnect');
    
    //TODO disconnect on out int
    
    http_response_code('200');
    echo json_encode(array('id' => 'disconnect_ack', 'sid' => $sid));
    exit(1);

  }

  deleteSession($app, $sid);
  http_response_code('400');
  echo json_encode(array('id' => 'error', 'type' => 2, 'msg' => 'unknown request'));
  exit(1);
  
} else if ($int == 'out') {

  //
  // OUT interface: proxy<->API client (e.g. web browser)
  //

  $app = $_GET['app'];
  if ($app == '') {
    http_response_code('400');
    printPage('App param is not defined!');
    exit(1);
  }

  // Check if any connection is death
  $list = glob('*_*.session');
  if (count($list) == 0) {
    // No connections
    http_response_code('404');
    printPage("$app is not connected!");
    exit(1);
  }

  $sid = 0;
  foreach ($list as $filename) {
    $fromsid = explode('_', explode('.', $filename)[0]);
    $from_ = $fromsid[0]; $sid_ = $fromsid[1];
    
    $state = ''; $timestamp = 0;
    if (!getSessionState($from_, $sid_, $state, $timestamp)) {
      http_response_code('500');
      printPage('Internal error. Can not get session state!');
      exit(1);
    }
    
    //echo "from: $from_, sid: $sid_, state: $state, timestamp: $timestamp, time: ".time();
    if ($state == 'waiting_for_connect' && (time() - $timestamp) > WAIT_OFFSET) {
      // Death session detected!
      deleteSession($from_, $sid_);
    } else {
      // Check if destination is connected i.e. app == from
      if ($from_ == $app)
	$sid = $sid_;
    }
  }

  if ($sid == 0) {
    http_response_code('404');
    printPage("$app is not connected!");
    exit(1);
  }

  // Generate ID for request
  $uid = uniqid();
  $body = $_SERVER['REQUEST_METHOD'] === 'POST' ? base64_encode($HTTP_RAW_POST_DATA) : '';

  $url = 'http'.(isset($_SERVER['HTTPS']) ? 's' : '').'://'."{$_SERVER['HTTP_HOST']}/{$_SERVER['REQUEST_URI']}";
  
  writeInSocket($app, $sid, $uid, base64_encode(json_encode(array('type' => 'http_request', 
							  'uid' => $uid, 
							  'method' => $_SERVER['REQUEST_METHOD'], 
							  'source' => $_SERVER['REMOTE_ADDR'], 
							  'url' => $url,
							  'body' => $body))));
  // Waiting loop for reply
  $end_time = time() + MAX_WAIT_OUT;
  while(time() < $end_time) {
    $data = readOutSocket($app, $sid, $uid);
    if ($data == "") {
    
      // No data
      sleep(WAIT_QUANTUM);
      
    } else {

      // Got data
      $json = json_decode(base64_decode($data));
      if (json_last_error() == JSON_ERROR_NONE) {
	if ($json->{'type'} == 'http_response' && $json->{'uid'} == $uid) {
	
	  clearOutSocket($app, $sid, $uid);
	  
	  $status = $json->{'status'};
	  if ($status > 99 && $status < 700)
	    http_response_code($status);
	  else
	    http_response_code(500);
	  
	  $content_type = $json->{'content_type'};
	  if ($content_type != "")
	    header('Content-type: '.$content_type);
	  
	  $body = base64_decode($json->{'body'});
	  if ($body != "")
	    echo $body;
	  exit(1);
	}
      }
      
      // Found data but not for this http session -> waiting
      sleep(WAIT_QUANTUM);
      
    }
  }

  http_response_code('408');
  printPage('Request Timeout');
  exit(1);

}

http_response_code('404');
exit(1);
?>