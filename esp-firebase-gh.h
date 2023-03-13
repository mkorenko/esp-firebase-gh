#ifndef Firebase_GH
#define Firebase_GH

#include <string>
#include "Arduino.h"
#include <Firebase_ESP_Client.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

// LOCAL_FULFILLMENT
#ifndef DISABLE_LOCAL_FULFILLMENT
#include <WiFiUdp.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif

#ifndef LOCAL_SDK_HTTP_PORT
#define LOCAL_SDK_HTTP_PORT 3310
#endif

#ifndef LOCAL_SDK_UDP_MAX_PACKET_SIZE
#define LOCAL_SDK_UDP_MAX_PACKET_SIZE 32
#endif

#ifndef LOCAL_SDK_UDP_DISCOVERY_STRING
#define LOCAL_SDK_UDP_DISCOVERY_STRING "firebase_gh_d"
#endif

#ifndef LOCAL_SDK_UDP_BROADCAST_PORT
#define LOCAL_SDK_UDP_BROADCAST_PORT 3311
#endif

#ifndef LOCAL_SDK_UDP_LISTEN_PORT
#define LOCAL_SDK_UDP_LISTEN_PORT 3312
#endif
#endif
// END - LOCAL_FULFILLMENT

#ifndef DEVICE_ONLINE_REFRESH_INTERVAL_MS
#define DEVICE_ONLINE_REFRESH_INTERVAL_MS 60000
#endif

#ifndef FIREBASE_QUERY_DEBOUNCE_MS
#define FIREBASE_QUERY_DEBOUNCE_MS 100
#endif

class FirebaseEspGh {
  public:
    FirebaseEspGh(void);

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
          FirebaseJson *gh_state,
          FirebaseJson *gh_notifications,
          FirebaseJson *custom_state
      )> on_device_state_request
    );
    void loop();
    void report_device_state();
  private:
    // LOCAL_FULFILLMENT
    #ifndef DISABLE_LOCAL_FULFILLMENT
    #if defined(ESP8266)
    WiFiUDP _udp;
    ESP8266WebServer _http_server;
    #else
    WiFiUDP _udp;
    WebServer _http_server;
    #endif
    bool _http_request_handled = false;
    bool _handle_upd_discovery();
    void _handle_http_cmd();
    void _handle_http_device_state_query();
    #endif
    // END - LOCAL_FULFILLMENT

    const char *_db_url;
    const char *_api_key;
    const char *_user_email;
    const char *_user_password;
    const char *_device_id;
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
        FirebaseJson *gh_state,
        FirebaseJson *gh_notifications,
        FirebaseJson *custom_state
    )> _on_device_state_request;

    void _on_cmd_data_change();

    unsigned long _firebase_last_query_at = 0;

    bool _cmd_result_scheduled = false;
    FirebaseJson _cmd_result;
    std::string _cmd_result_request_id;
    bool _cmd_result_loop();

    bool _power_at_reported = false;
    unsigned long _report_online_last_at = 0;
    bool _report_online_status_loop();

    bool _report_state_scheduled = false;
    bool _report_device_state_loop();
};

#endif
