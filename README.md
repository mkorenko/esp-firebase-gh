## ESP32/ESP8266 firebase-google-home companion library

The library allows to easily integrate with the [firebase-google-home framework](https://github.com/mkorenko/firebase-google-home) by providing simple "on command" and "report state" callbacks.
Depends and based on: [Firebase-ESP-Client](https://github.com/mobizt/Firebase-ESP-Client).

### Library API / usage example
Consider the following Arduino IDE project example:
```
// path to this library
// note: Arduino IDE  accepts custom libs in "src" dir only
#include "src/firebase-esp-gh/firebase-esp-gh.h"

FirebaseEspGh firebase_esp_gh;

// firebase credentials
const char* firebase_api_key = "firebase_api_key";
const char* firebase_user_email = "firebase_user_email";
const char* firebase_user_password = "firebase_user_password";
const char* firebase_db_url = "https://firebase_db_url.firebaseio.com/";
const char* firebase_device_id = "firebase_device_id";

// "on command" callback
void gh_on_command(
  FirebaseJson *result,
  std::string &command,
  FirebaseJson &params
) {
  // this callback is called on each GH command
  // (when new "cmd" object is set to "device_id -> cmd" DB path
  // see https://github.com/mkorenko/firebase-google-home#database-structure-receiving-gh-commands)

  if (command == "action.devices.commands.OnOff") {
    // code to turn your device on or off, based on the "on" param:
    // https://developers.home.google.com/cloud-to-cloud/traits/onoff#parameters
    FirebaseJsonData _on;
    params.get(_on, "on");

    // double check that the "on" param was successfully parsed
    if (!_on.success) {
      // to respond with an error - return the only "error_code" field:
      // https://github.com/mkorenko/firebase-google-home#device_id---cmd---result
      result->add("error_code", "hardError");
      return;
    }

    bool on = _on.to<bool>();
    if ((on && !{bool_your_device_is_on}) ||
        (!on && {bool_your_device_is_on})) {
      if (on) {
         // code to turn your device on
      } else {
         // code to turn you device off
      }

      // success - return new partial GH state
      // https://github.com/mkorenko/firebase-google-home#device_id---cmd---result
      result->add("on", on);
      return;
    }

    // soft error, already {on_param_value}:
    result->add("error_code", on ? "alreadyOn" : "alreadyOff");
      return;
    }

  // hard error, this command is unsupported:
  result->add("error_code", "hardError");
}

// "report state" callback
void gh_on_device_state_request(
    FirebaseJson *state,
    FirebaseJson *gh_state,
    FirebaseJson *gh_notifications
) {
  // custom state
  // https://github.com/mkorenko/firebase-google-home#device_id---state
  state->add("internal_device_status", {internal_device_status_value});
  state->set("timestamp/.sv", "timestamp");

  // gh state
  // https://github.com/mkorenko/firebase-google-home#device_id---state---gh_state
  gh_state->add("on", {bool_your_device_is_on});

  FirebaseJson current_run_cycle_en;
  current_run_cycle_en.add("currentCycle", {string_your_device_run_cycle});
  current_run_cycle_en.add("lang", "en");
  FirebaseJsonArray current_run_cycle;
  current_run_cycle.add(current_run_cycle_en);
  gh_state->add("currentRunCycle", current_run_cycle);

  // gh notifications
  // https://github.com/mkorenko/firebase-google-home#device_id---state---gh_state---gh_notifications
  if ({bool_notification_example_error_no_water}) {
    FirebaseJson notification;
    notification.add("priority", 0);
    notification.add("status", "FAILURE");
    notification.add("errorCode", "needsWater");
    gh_notifications->add("RunCycle", notification);
  }
}

void setup() {
  firebase_esp_gh.begin(
      // pass firebase credentials and RTDB url
      firebase_api_key,
      firebase_user_email,
      firebase_user_password,
      firebase_db_url,

      // pass device id
      firebase_device_id,

      // pass the "on command" callback
      gh_on_command,
      // and the "report state" callback
      gh_on_device_state_request
  );
}

void loop() {
  // required:
  firebase_esp_gh.loop();
}

// call "report_device_state()" to notify about your device state change:
// this will schedule state report - the "report state" callback will be
// called shortly:
firebase_esp_gh.report_device_state();

```
