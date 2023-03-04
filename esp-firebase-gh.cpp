#include <string>
#include "Arduino.h"
#include "esp-firebase-gh.h"
#include <Firebase_ESP_Client.h>

void FirebaseEspGh::_on_cmd_data_change() {
  FirebaseJson *json = _cmd_change_fbdo.to<FirebaseJson *>();

  if (_cmd_change_fbdo.dataPath() != "/" ||
      _cmd_change_fbdo.dataType() != "json" ||
      _cmd_change_fbdo.eventType() != "put") {
    json->clear();
    return;
  }

  _cmd_result_scheduled = true;
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
  _config.api_key = firebase_api_key;
  _auth.user.email = firebase_user_email;
  _auth.user.password = firebase_user_password;
  _config.database_url = firebase_db_url;
  _device_root = "/" + std::string(firebase_device_id);

  _on_gh_command = on_gh_command;
  _on_device_state_request = on_device_state_request;

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
