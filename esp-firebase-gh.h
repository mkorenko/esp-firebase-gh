#ifndef Firebase_GH
#define Firebase_GH

#include <string>
#include "Arduino.h"
#include <Firebase_ESP_Client.h>

#ifndef DEVICE_ONLINE_REFRESH_INTERVAL_MS
#define DEVICE_ONLINE_REFRESH_INTERVAL_MS 60000
#endif

#ifndef FIREBASE_QUERY_DEBOUNCE_MS
#define FIREBASE_QUERY_DEBOUNCE_MS 100
#endif

class FirebaseEspGh {
  public:
    void begin(
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
    );
    void loop();
    void report_device_state();
  private:
    const char *_db_url;
    const char *_api_key;
    const char *_user_email;
    const char *_user_password;
    std::string _device_root;
    FirebaseConfig _config;
    FirebaseAuth _auth;
    FirebaseData _cmd_change_fbdo;
    FirebaseData _fbdo;
    std::function<void (
        FirebaseJson *result,
        std::string &command,
        FirebaseJson &params
    )> _on_gh_command;
    std::function<void (
        FirebaseJson *state,
        FirebaseJson *gh_state,
        FirebaseJson *gh_notifications
    )> _on_device_state_request;

    void _on_cmd_data_change();

    unsigned long _firebase_last_query_at = 0;

    bool _cmd_result_scheduled = false;
    FirebaseJson _cmd_result;
    bool _cmd_result_loop();

    bool _power_at_reported = false;
    unsigned long _report_online_last_at = 0;
    bool _report_online_status_loop();

    bool _report_state_scheduled = false;
    bool _report_device_state_loop();
};

#endif
