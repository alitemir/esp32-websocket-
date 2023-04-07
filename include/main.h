#include "esp_vfs.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Renk Teması
#define TEMA "#d900ff"
// #define TEMA "#4dce11"
// #define TEMA "#1418e6"
// #define TEMA "#0db5e9"

// Kullanilan pinler
#define LED_PIN 2
#define MAX485_DE 22
#define MAX485_RE_NEG 23
#define SERIAL1_RX 5
#define SERIAL1_TX 17

// thermocouple pinleri
#define thermoSO 19
#define thermoCS 21
#define thermoCLK 18

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

// Robotun sarj ve tur adresleri
#define CHARGING_COIL 0x800
#define ON_TOUR_COIL 0x801

// Robotun Durum adresi
#define DURUM 0x100C

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

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)
#define SCRATCH_BUFSIZE 4096
#define IS_FILE_EXT(filename, ext) \
  (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

struct file_server_data
{
  char base_path[ESP_VFS_PATH_MAX + 1];
  char scratch[SCRATCH_BUFSIZE];
};