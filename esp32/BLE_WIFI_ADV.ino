/************Define Section *************/
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEUUID.h>
#include <String.h>
#include <WiFi.h>
#include <PubSubClient.h>
/******************************************/

// Change host name for each ESP
#define HOSTNAME "ESP3"
#define MQTTCLIENTNAME "ESP32_003client"
#define IBEACON "ibeacon"

/************Other Global Variables (Project Global) *************/
//Wifi SSAD
const char* ssid = "vaibhav";

// WIFI password
const char* password =  "vaibhav@123";

//const char* mqttServer = "m10.cloudmqtt.com";
//const int mqttPort = 16428;
//const char* mqttUser = "rvthfgpv";
//const char* mqttPassword = "9OaXE2qQNdWt";

// MQTT Server
const char* mqttServer = "raspberrypi";

// MQTT Port
const int mqttPort = 1883;
/********************************************************************/

/************Global variables & Define (Project local) **************/
int scanTime = 3; //In seconds
char beacon_uuid [33]; // UUID
char beacon_major [5]; // MAJOR ID
char beacon_minor [5]; // MINOR ID
char beacon_mac_addr[19]; // MAC ADDRESS
char tx_power [3]; // TXPOWER
char iBeacon_id[5]; // IBEACON ID
int rssi_int; // RSSI int value
char beacon_rssi[4]; // RSSI value
char beacon_type[8]; // IBEACON TYPE (ibeacon)
char host_name[7] = HOSTNAME ; // HOSTname
char json_ble_send[1000]; //json ble buffer
char topic [130]; // MQTT topic
static boolean ble_data_collect = false;
/********************************************************************/
// TBD
WiFiClient espClient;
//TBD
PubSubClient client(espClient);


static uint8_t BLEStoregeCount = 0;
typedef struct {
   char BLE_host_name[7];
   char BLE_beacon_type[8];
   char BLE_mac_addr[19];
   char BLE_beacon_rssi[4];
   char BLE_beacon_uuid [33];
   char BLE_beacon_major [5];
   char BLE_beacon_minor [5];
   char BLE_tx_power [3];
} BLEStorageData;

BLEStorageData bledata[100];

//*****************//


/* Call back function for BLE Advertisement, it will validate for ibeacon data and extract to Beacon buffers */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
      void onResult(BLEAdvertisedDevice advertisedDevice) {
         // collect data from advertisement into ble_buffer
         char *ble_buffer = BLEUtils::buildHexData(nullptr, (uint8_t*)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());
         // Extracting Data from ble_buffer to ibeacon_buffer (if other than ibeacon discard packet)
         memset(iBeacon_id, 0, 5);
         memcpy(iBeacon_id, &ble_buffer[0], 4);
         iBeacon_id[4] = '\0';
         if (strcmp(iBeacon_id, "4c00")  == 0) {
            memcpy(bledata[BLEStoregeCount].BLE_host_name, HOSTNAME, 6);
            bledata[BLEStoregeCount].BLE_host_name[6] = '\0';

            memcpy(bledata[BLEStoregeCount].BLE_beacon_type, IBEACON, 7);
            bledata[BLEStoregeCount].BLE_beacon_type[7] = '\0';

            memcpy(bledata[BLEStoregeCount].BLE_mac_addr, advertisedDevice.getAddress().toString().c_str(), 18);
            bledata[BLEStoregeCount].BLE_mac_addr[18] = '\0';

            rssi_int = advertisedDevice.getRSSI();
            itoa(rssi_int, bledata[BLEStoregeCount].BLE_beacon_rssi, 10);
            bledata[BLEStoregeCount].BLE_beacon_rssi[3] = '\0';

            memcpy(bledata[BLEStoregeCount].BLE_beacon_uuid, &ble_buffer[8], 32);
            bledata[BLEStoregeCount].BLE_beacon_uuid[31] = '\0';

            memcpy(bledata[BLEStoregeCount].BLE_beacon_major, &ble_buffer[40], 4);
            bledata[BLEStoregeCount].BLE_beacon_major[4] = '\0';

            memcpy(bledata[BLEStoregeCount].BLE_beacon_minor, &ble_buffer[44], 4);
            bledata[BLEStoregeCount].BLE_beacon_minor[4] = '\0';

            memcpy(bledata[BLEStoregeCount].BLE_tx_power, &ble_buffer[48], 2);
            bledata[BLEStoregeCount].BLE_tx_power[2] = '\0';
            BLEStoregeCount++;
         }
      }




      //Serial.printf("Advertised UUID: %s \n", advertisedDevice.getServiceUUID().toString());
};

void setup() {
   //Serial setup
   Serial.begin(115200);
}
void(* resetFunc) (void) = 0;//declare reset function at address 0

int WifiRboot =0;
int MqttRboot=0;
void loop() {
   BLEDevice::init("");
   WiFi.begin(ssid, password);
   while (1) {
      while (WiFi.status() != WL_CONNECTED) {
         WifiRboot++;
         if(WifiRboot >=10)
         {
          resetFunc(); //call reset
         }
         delay(2000);
         Serial.println("Connecting to WiFi..");
      }
      Serial.println("Connected to the WiFi network");
      WifiRboot=0;
      client.setServer(mqttServer, mqttPort);

      while (!client.connected()) {
         Serial.println("Connecting to MQTT...");
          //if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
         if (client.connect(MQTTCLIENTNAME)) {

            Serial.println("connected");

         } else {
             MqttRboot++;
             if(MqttRboot >=10)
             {
              resetFunc(); //call reset
             }
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
            

         }
      }
      MqttRboot=0;
      client.loop();
      Serial.println("Setting up BLE...");
      BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
      pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(),true);
      pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
      BLEScanResults foundDevices = pBLEScan->start(scanTime);
      int TempIndex = 0;
      bool PublishStatus = false;
      printf("No of BLE Entries =%d\n",BLEStoregeCount);
      for (TempIndex = 0; TempIndex < BLEStoregeCount; TempIndex++)
      {
         memset(json_ble_send, 0, 1000);
         memset(topic, 0, 130);
         sprintf(topic, "happy-bubbles/ble/%s/ibeacon/%s", bledata[TempIndex].BLE_host_name, bledata[TempIndex].BLE_beacon_uuid);
         sprintf(json_ble_send, "{\"hostname\": \"%s\",\r\n\"beacon_type\": \"ibeacon\",\r\n\"mac\": \"%s\",\r\n\"rssi\": %s,\r\n\"uuid\": \"%s\",\r\n\"major\": \"%s\",\r\n\"minor\": \"%s\",\r\n\"tx_power\": \"%s\"}", bledata[TempIndex].BLE_host_name, bledata[TempIndex].BLE_mac_addr, bledata[TempIndex].BLE_beacon_rssi, bledata[TempIndex].BLE_beacon_uuid, bledata[TempIndex].BLE_beacon_major, bledata[TempIndex].BLE_beacon_minor, bledata[TempIndex].BLE_tx_power);
         Serial.printf("topic =%s\n", topic);
         Serial.printf("json_ble_send =%s\n", json_ble_send);
         PublishStatus = client.publish(topic, json_ble_send);
         Serial.printf("Publish Status =%d\n", PublishStatus );
      }
      BLEStoregeCount = 0;


      Serial.print("Devices found: ");
      Serial.println(foundDevices.getCount());
      Serial.println("Scan done!");
      delay(2000);
      //sleep(100);
   }

}
