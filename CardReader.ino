#include "cmd.h"
#include "Cardio.h"

#define SerialDevice SerialUSB //32u4,samd21
//#define SerialDevice Serial

Cardio_ Cardio;

void SerialCheck() {
  switch (packet_read()) {
    case SG_NFC_CMD_RESET:
      sg_nfc_cmd_reset();
      break;
    case SG_NFC_CMD_GET_FW_VERSION:
      sg_nfc_cmd_get_fw_version();
      break;
    case SG_NFC_CMD_GET_HW_VERSION:
      sg_nfc_cmd_get_hw_version();
      break;
    case SG_NFC_CMD_POLL:
      sg_nfc_cmd_poll();
      break;
    case SG_NFC_CMD_MIFARE_READ_BLOCK:
      sg_nfc_cmd_mifare_read_block();
      break;
    case SG_NFC_CMD_FELICA_ENCAP:
      sg_nfc_cmd_felica_encap();
      break;
    case SG_NFC_CMD_MIFARE_AUTHENTICATE:
      sg_res_init();
      break;
    case SG_NFC_CMD_MIFARE_SELECT_TAG:
      sg_nfc_cmd_mifare_select_tag();
      break;
    case SG_NFC_CMD_MIFARE_SET_KEY_AIME:
      sg_nfc_cmd_mifare_set_key();
      break;
    case SG_NFC_CMD_MIFARE_SET_KEY_BANA:
      sg_nfc_cmd_mifare_set_key();
      break;
    case SG_NFC_CMD_RADIO_ON:
      sg_nfc_cmd_radio_on();
      break;
    case SG_NFC_CMD_RADIO_OFF:
      sg_nfc_cmd_radio_off();
      break;
    case SG_RGB_CMD_RESET:
      sg_led_cmd_reset();
      break;
    case SG_RGB_CMD_GET_INFO:
      sg_led_cmd_get_info();
      break;
    case SG_RGB_CMD_SET_COLOR:
      sg_led_cmd_set_color();
      break;
    case 0:
      break;
    default:
      sg_res_init();
  }
}

void setup() {
  SerialDevice.begin(38400);
  //  SerialUSB.begin(119200);//high_baudrate=true
  SerialDevice.setTimeout(0);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    while (1) {
      fill_solid(leds, NUM_LEDS, 0xFF0000);
      FastLED[0].show(leds, NUM_LEDS, BRI);
      delay(500);
      fill_solid(leds, NUM_LEDS, 0x000000);
      FastLED[0].show(leds, NUM_LEDS, BRI);
      delay(500);
    }
  }
  nfc.setPassiveActivationRetries(0x10);//设定等待次数
  nfc.SAMConfig();
  memset(&req, 0, sizeof(req.bytes));
  memset(&res, 0, sizeof(res.bytes));
  fill_solid(leds, NUM_LEDS, 0x0000FF);
  FastLED[0].show(leds, NUM_LEDS, BRI);
  Cardio.begin(false);
}

void loop() {
  polling_result = nfc.felica_Polling(systemCode, requestCode, felica.IDm, felica.PMm, &systemCodeResponse, 200);
  if(polling_result){
    Cardio.setUID(2, felica.IDm);
    Cardio.sendState();
  }
  SerialCheck();
  packet_write();
}

static uint8_t len, r, checksum;
static bool escape = false;

static uint8_t packet_read() {

  while (SerialDevice.available()) {
    r = SerialDevice.read();
    if (r == 0xE0) {
      req.frame_len = 0xFF;
      len = 0;
      continue;
    }
    if (req.frame_len == 0xFF) {
      req.frame_len = r;
      len = 1;
      checksum = r;
      continue;
    }
    if (r == 0xD0) {
      escape = true;
      continue;
    }
    if (escape) {
      r++;
      escape = false;
    }
    if (len == req.frame_len && checksum == r) {
      return req.cmd;
    }
    req.bytes[len++] = r;
    checksum += r;
  }
  return 0;
}

static void packet_write() {
  uint8_t checksum = 0, len = 0;
  if (res.cmd == 0) {
    return;
  }
  SerialDevice.write(0xE0);
  while (len <= res.frame_len) {
    uint8_t w;
    if (len == res.frame_len) {
      w = checksum;
    } else {
      w = res.bytes[len];
      checksum += w;
    }
    if (w == 0xE0 || w == 0xD0) {
      if (SerialDevice.availableForWrite() < 2)
        return;
      SerialDevice.write(0xD0);
      SerialDevice.write(--w);
    } else {
      if (SerialDevice.availableForWrite() < 1)
        return;
      SerialDevice.write(w);
    }
    len++;
  }
  res.cmd = 0;
}
