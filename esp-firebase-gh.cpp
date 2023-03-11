#include <string>
#include "Arduino.h"
#include "esp-firebase-gh.h"
#include <Firebase_ESP_Client.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#ifndef DISABLE_LOCAL_FULFILLMENT
#include <WiFiUdp.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#endif

#ifndef DISABLE_LOCAL_FULFILLMENT
FirebaseEspGh::FirebaseEspGh() :
  _http_server(LOCAL_SDK_HTTP_PORT) {}
#else
FirebaseEspGh::FirebaseEspGh() {}
#endif

void FirebaseEspGh::_on_cmd_data_change() {
  FirebaseJson *json = _cmd_change_fbdo.to<FirebaseJson *>();

  if (_cmd_change_fbdo.dataPath() != "/" ||
      _cmd_change_fbdo.dataType() != "json" ||
      _cmd_change_fbdo.eventType() != "put") {
    json->clear();
    return;
  }

  _cmd_result_scheduled = true;

  FirebaseJsonData _request_id;
  json->get(_request_id, "request_id");
  if (_request_id.success) {
    std::string request_id = _request_id.to<std::string>();
    if (_cmd_result_request_id == request_id) {
      json->clear();
      return;
    }
    _cmd_result_request_id = request_id;
  }

  _cmd_result.clear();

  FirebaseJsonData _command;
  json->get(_command, "command");
  if (!_command.success) {
    _cmd_result.add("error_code", "hardError");
    json->clear();
    return;
  }
  std::string command = _command.to<std::string>();

  FirebaseJsonData _params;
  json->get(_params, "params");
  if (!_params.success) {
    _cmd_result.add("error_code", "hardError");
    json->clear();
    return;
  }
  FirebaseJson params;
  _params.get<FirebaseJson>(params);

  json->clear();

  _on_gh_command(&_cmd_result, command, params);
}

#ifndef DISABLE_LOCAL_FULFILLMENT

void FirebaseEspGh::_handle_upd_discovery() {
  int packet_size = _udp.parsePacket();
  if (!packet_size || packet_size > LOCAL_SDK_UDP_MAX_PACKET_SIZE) { return; }

  char incoming_packet[packet_size + 1];
  _udp.read(incoming_packet, packet_size);
  incoming_packet[packet_size] = 0;

  if (strcmp(incoming_packet, LOCAL_SDK_UDP_DISCOVERY_STRING) != 0) {
    return;
  }

  _udp.beginPacket(_udp.remoteIP(), LOCAL_SDK_UDP_LISTEN_PORT);
  _udp.printf(_device_id);
  _udp.endPacket();
}

void FirebaseEspGh::_handle_http_cmd() {
  _cmd_result.clear();
  std::string http_response;

  if (_http_server.hasArg("plain")== false){
    _cmd_result.add("error_code", "hardError");
    _cmd_result.toString(http_response);
    _http_server.send(
        400,
        "application/json",
        http_response.c_str()
    );
    return;
  }

  FirebaseJson json;
  json.setJsonData(_http_server.arg("plain"));

  FirebaseJsonData _request_id;
  json.get(_request_id, "request_id");
  if (_request_id.success) {
    std::string request_id = _request_id.to<std::string>();
    if (_cmd_result_request_id == request_id) {
      _cmd_result.toString(http_response);
      _http_server.send(
          200,
          "application/json",
          http_response.c_str()
      );

      return;
    }

    _cmd_result_request_id = request_id;
  }

  FirebaseJsonData _command;
  json.get(_command, "command");
  if (!_command.success) {
    _cmd_result.add("error_code", "hardError");
    _cmd_result.toString(http_response);
    _http_server.send(
        400,
        "application/json",
        http_response.c_str()
    );
    return;
  }
  std::string command = _command.to<std::string>();

  FirebaseJsonData _params;
  json.get(_params, "params");
  if (!_params.success) {
    _cmd_result.add("error_code", "hardError");
    _cmd_result.toString(http_response);
    _http_server.send(
        400,
        "application/json",
        http_response.c_str()
    );
    return;
  }
  FirebaseJson params;
  _params.get<FirebaseJson>(params);

  _on_gh_command(&_cmd_result, command, params);

  _cmd_result.toString(http_response);
  _http_server.send(
      200,
      "application/json",
      http_response.c_str()
  );
}

void FirebaseEspGh::_handle_http_device_state_query() {
  FirebaseJson state;
  FirebaseJson gh_state;
  FirebaseJson gh_notifications;

  _on_device_state_request(&state, &gh_state, &gh_notifications);

  std::string http_response;
  gh_state.toString(http_response);
  _http_server.send(
      200,
      "application/json",
      http_response.c_str()
  );
}

#endif

bool FirebaseEspGh::_cmd_result_loop() {
  if (!_cmd_result_scheduled) { return false; }
  _cmd_result_scheduled = false;

  Firebase.RTDB.set(&_fbdo, _device_root + "/cmd/result", &_cmd_result);

  return true;
}

bool FirebaseEspGh::_report_online_status_loop() {
  if (_report_online_last_at != 0 &&
      millis() - _report_online_last_at < DEVICE_ONLINE_REFRESH_INTERVAL_MS) {
    return false;
  }

  if (!_power_at_reported) {
    _power_at_reported = true;
    Firebase.RTDB.setTimestamp(&_fbdo, _device_root + "/power_at");
    return true;
  }

  _report_online_last_at = millis();
  Firebase.RTDB.setTimestamp(&_fbdo, _device_root + "/online_at");
  return true;
}

bool FirebaseEspGh::_report_device_state_loop() {
  if (!_report_state_scheduled) { return false; }
  _report_state_scheduled = false;

  FirebaseJson state;
  FirebaseJson gh_state;
  FirebaseJson gh_notifications;
  _on_device_state_request(&state, &gh_state, &gh_notifications);

  gh_state.add("gh_notifications", gh_notifications);
  state.set("gh_state", gh_state);

  Firebase.RTDB.set(&_fbdo, _device_root + "/state", &state);

  return true;
}

void FirebaseEspGh::begin(
    const char *firebase_api_key,
    const char *firebase_user_email,
    const char *firebase_user_password,
    const char *firebase_db_url,
    const char *firebase_device_id,
    std::function<void (
      FirebaseJson *result,
      std::string &command,
      FirebaseJson &params
    )> on_gh_command,
    std::function<void (
        FirebaseJson *state,
        FirebaseJson *gh_state,
        FirebaseJson *gh_notifications
    )> on_device_state_request
) {
  // config
  _config.api_key = firebase_api_key;
  _auth.user.email = firebase_user_email;
  _auth.user.password = firebase_user_password;
  _config.database_url = firebase_db_url;
  _device_id = firebase_device_id;
  _device_root = "/" + std::string(firebase_device_id);

  // set callbacks
  _on_gh_command = on_gh_command;
  _on_device_state_request = on_device_state_request;

  #ifndef DISABLE_LOCAL_FULFILLMENT
  // UDP discovery
  _udp.begin(LOCAL_SDK_UDP_BROADCAST_PORT);

  // HTTP server
  _http_server.on(
      "/cmd",
      HTTP_POST,
      std::bind(&FirebaseEspGh::_handle_http_cmd, this)
  );
    _http_server.on(
      "/query",
      HTTP_GET,
      std::bind(&FirebaseEspGh::_handle_http_device_state_query, this)
  );
  _http_server.begin();
  #endif

  // Firebase
  Firebase.begin(&_config, &_auth);
  Firebase.RTDB.beginStream(&_cmd_change_fbdo, _device_root + "/cmd");
}

void FirebaseEspGh::loop() {
  if (!Firebase.ready()) { return; }

  // perf: one firebase action in one loop cycle
  Firebase.RTDB.readStream(&_cmd_change_fbdo);
  if (_cmd_change_fbdo.streamAvailable()) {
    return _on_cmd_data_change();
  }

  #ifndef DISABLE_LOCAL_FULFILLMENT
  _handle_upd_discovery();
  _http_server.handleClient();
  #endif

  if (millis() - _firebase_last_query_at < FIREBASE_QUERY_DEBOUNCE_MS) {
    return;
  }

  if (_cmd_result_loop()) {
    _firebase_last_query_at = millis();
    return;
  }
  if (_report_online_status_loop()) {
    _firebase_last_query_at = millis();
    return;
  }
  if (_report_device_state_loop()) {
    _firebase_last_query_at = millis();
    return;
  }
}

void FirebaseEspGh::report_device_state() {
  _report_state_scheduled = true;
}
