#include "esp_vfs.h"
#include <ArduinoJson.h>
#include <HardwareSerial.h>

// Robot ve Referans Nokta Bilgileri
const int IDNumber = 1;
const char *city = "TR-32";
const char *token_num = "7Y30V-N70MF-Z4IFB-C7P7L-OWN3Y";
const char *hedefMAC = "A8:42:E3:57:4E:A4";

// haberleşme ayarları
const char *ssid = "ITECH_Cihazlar";
const char *password = ".12345678.";
const char *soft_ap_ssid = "I-BOLD";
const char *author = "ali_temir";
const char *socketUrl = "94.154.34.33"; // Websocket URL'sini doğrudan bir char dizisi olarak tanımlayın
const uint16_t socketPort = 4005;
const char *SMS = "SMS";
const char *TEL = "TEL";
const char *ALL = "ALL";

// local wifi ayarı
const char *ap_pwd = "12345678";
const char *hotspot_ssid = "mustafa_ali_can";
const char *mdns_host = "iyap1";
const char *mdns_host_uppercase = "I-yap1";
const char *base_path = "/data";
const char *TAG = "MAIN";

int lokaltim;
int lokalfark;

int saved_lokaltim;
int saved_lokalfark;

enum op_errors
{
  OP_OK = 0,
  OP_ERR = 1,
  OP_OUT_OF_LIMIT = 2,
  OP_WRITE_FAIL = 3

};
uint16_t readHoldingRegisters(uint16_t u16ReadAddress, uint16_t u16ReadQty);
uint8_t writeSingleRegister(uint16_t u16WriteAddress, uint16_t u16WriteValue);
uint8_t writeSingleCoil(uint16_t u16WriteAddress, uint8_t u8State);
uint8_t readCoils(uint16_t u16WriteAddress, uint8_t u8State);

op_errors parse_ektur(const JsonObject &doc);

// op_errors parse_status();

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Renk Teması
#define TEMA "#d900ff"
// #define TEMA "#4dce11"
// #define TEMA "#1418e6"
// #define TEMA "#0db5e9"

// Kullanilan pinler
#define LED_PIN 2
#define SERIAL1_RX 16
#define SERIAL1_TX 17

// thermocouple pinleri
#define thermoSO 19
#define thermoCS 21
#define thermoCLK 18

#define ENCODER_EN 0x11CC

// Uzaktan kontrol PLC adresleri
#define FORWARD_ADDRESS 0x8AB
#define BACKWARD_ADDRESS 0x8AA
#define RIGHT_ADDRESS 0x8AC
#define LEFT_ADDRESS 0x8AD
#define STOP_ADDRESS 0x8AE

// Akim kontrol adresleri

#define AKIM_ILERI 0x11C2
#define AKIM_GERI 0x11C4

// saat ve dakikaların adresi
#define ALARM_ADDR_BEGIN 0x1214

// Akım ve gerilim okuma adresleri
#define CURRENT 0x1012
#define VOLTAGE 0x1011

// Ek tur adresleri
#define EKTUR_ILERI 0x80A
#define EKTUR_GERI 0x80B

// Zamanlayıcı adresleri - Tek seferde 8 adres okunur
#define ALARM_SETTINGS_ADDR_BEGIN 0x119A // 0x119A - 0x11A1 arası

// Robotun Durum adresi
#define DURUM 0x103A

// Mesajlarin Hata Durumu
#define state_code 0x103C

// Kış Modu Ayarları
#define ILERI_SURE 0x11A4
#define GECIKME 0x11A6
#define GERI_SURE 0x11A8
#define KIS_MODU_AKTIF 0x11AE
#define BIRINCI_MOD 0x11B0
#define IKINCI_MOD 0x11B2

#define TEMPERATURE 0x1028

// Saat Verileri Okuma Adresleri
#define CLOCKS_ENABLED 0x112C
#define SANIYE_READ 0x1521
#define DAKIKA_READ 0x1522
#define SAAT_READ 0x1523

// Saat Verileri Yazma adresler
#define DAKIKA_WRITE 0x1065
#define SAAT_WRITE 0x1064

// Sarj ve gerilim limitleri
#define SARJ_LIMIT 0x11BA
#define GERILIM_LIMIT 0x11B8

// Sarj ve gerilim kalb
#define SARJ_kalb 0x11D2
#define GERILIM_kalb 0x11D0

// sure kalb
#define ilerisure_kalb 0x11F4
#define gerisure_kalb 0x11F6

// tur baslamasi
#define TOUR_DIST 0x11DA

// payıya
#define PATIYA 0x802

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)
#define SCRATCH_BUFSIZE 4096
#define IS_FILE_EXT(filename, ext) \
  (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

struct file_server_data
{
  char base_path[ESP_VFS_PATH_MAX + 1];
  char scratch[SCRATCH_BUFSIZE];
};