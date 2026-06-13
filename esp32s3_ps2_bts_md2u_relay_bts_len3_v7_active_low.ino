#include <Arduino.h>

#if __has_include(<esp_arduino_version.h>)
  #include <esp_arduino_version.h>
#endif
#ifndef ESP_ARDUINO_VERSION_MAJOR
  #define ESP_ARDUINO_VERSION_MAJOR 2
#endif

// =====================================================
// ESP32-S3 + PS2 Receiver + 2 BTS7960 + MD2U-MD20 Stepper + Relay
// V7: relay active LOW noi vao net BTS_L_EN3, dieu khien qua GPIO14/opto
// Dieu khien tien/lui/trai/phai bang joystick ben trai
// KHONG dung PS2X_lib, tranh loi avr/io.h tren ESP32-S3
// =====================================================

// =========================
// CHAN PS2 THEO SO DO
// =========================
#define PS2_DAT 19
#define PS2_CMD 20
#define PS2_CS  21
#define PS2_CLK 47

// =========================
// LED BAO CODE DANG CHAY
// Ban da chuyen LED_STATUS sang GPIO35
// =========================
#define HEARTBEAT_LED_ENABLE 1
#define HEARTBEAT_LED_GPIO   35
#define HEARTBEAT_PERIOD_MS  500UL

// =========================
// P1 - BTS7960 so 1: motor trai
// =========================
#define BTS1_RPWM 4
#define BTS1_REN  5
#define BTS1_LPWM 6
#define BTS1_LEN  7

// =========================
// P2 - BTS7960 so 2: motor phai
// GPIO46 da doi sang GPIO18
// =========================
#define BTS2_RPWM 8
#define BTS2_REN  18
#define BTS2_LPWM 9
#define BTS2_LEN  10

// =========================
// P3 chua dung, dua ve safe OFF
// =========================
#define BTS3_RPWM 11
#define BTS3_REN  12
#define BTS3_LPWM 13
#define BTS3_LEN  14

// =========================
// MD2U-MD20 STEPPER DRIVE
// Theo ket noi ban mo ta:
//   CW       cua MD2U -> MOTOR_DIR/MOTOR_DTR -> ESP_DIR  GPIO17
//   CCW      cua MD2U -> MOTOR_EN            -> ESP_EN   GPIO16
//   HOLD OFF cua MD2U -> MOTOR_STEP          -> ESP_STEP GPIO15
// =========================
#define MD2U_CW_PIN        17
#define MD2U_CCW_PIN       16
#define MD2U_HOLDOFF_PIN   15

// Cong tac hanh trinh theo so do: HT1 -> GPIO1, HT2 -> GPIO2.
// Trong so do co tro keo xuong 10k ve GND, khi tac dong se len 3V3.
#define HT1_PIN            1     // gioi han hanh trinh quay thuan/CW
#define HT2_PIN            2     // gioi han hanh trinh quay nguoc/CCW
#define HT_ACTIVE_LEVEL    HIGH  // neu cam bien hanh trinh tich cuc muc LOW thi doi thanh LOW

// Tan so xung step. Neu motor bi mat buoc, giam xuong 200..500Hz.
// Neu muon quay nhanh hon, tang len 1000..2000Hz sau khi test on dinh.
#define MD2U_STEP_FREQ_HZ      800
#define MD2U_PWM_BITS          8
#define MD2U_PULSE_DUTY        128

// Do mach opto cua ban la kieu keo len 5V nen mac dinh logic phia drive bi dao.
#define MD2U_OPTO_INVERTED     BTS_OPTO_INVERTED

// Muc logic phia drive khi khong phat xung CW/CCW.
// Thuong de LOW de input step khong bi treo len cao.
#define MD2U_PULSE_IDLE_LEVEL  LOW

// HOLD OFF: da so drive, HOLD OFF active se nha motor/tat holding torque.
// Mac dinh de LOW de drive duoc phep chay/giu luc.
// Neu thuc te motor khong co luc giu, doi MD2U_HOLDOFF_RUN_LEVEL thanh HIGH.
#define MD2U_HOLDOFF_RUN_LEVEL LOW

// Neu muon nhan SELECT dung khan cap ca step, de 1.
#define MD2U_SELECT_EMERGENCY_STOP 1


// =====================================================
// CAU HINH QUAN TRONG
// =====================================================

// Mach opto cua ban co dien tro keo len 5V o dau ra.
// Thuong: ESP GPIO HIGH -> opto dan -> tin hieu phia BTS bi keo LOW.
// Do do mac dinh de 1.
// Neu ban do truc tiep tai chan BTS ma thay ESP HIGH -> BTS HIGH thi doi thanh 0.
#define BTS_OPTO_INVERTED 1

// Neu day joystick len ma xe chay lui, doi ca 2 dong nay thanh 1.
// Neu chi mot ben motor nguoc chieu, chi doi rieng ben do.
#define INVERT_LEFT_MOTOR   0
#define INVERT_RIGHT_MOTOR  0

// Neu can test drive doc lap, nang banh xe len roi doi thanh 1.
// Khi khoi dong, code se quay P1/P2 tien/lui ngan de xac nhan BTS va motor co hoat dong.
// Mac dinh de 0 cho an toan.
#define DRIVE_SELF_TEST_ON_BOOT 0

// SELECT tren tay PS2 de dung khan cap.
// Neu tay PS2 clone bi loi nut SELECT luon bi nhan, doi thanh 0.
#define ENABLE_SELECT_EMERGENCY_STOP 1

// =====================================================
// THAM SO DIEU KHIEN
// =====================================================

// =========================
// RELAY NGOAI - V7
// Relay thuc te tich cuc muc LOW.
// Ban da noi chan kich relay vao net BTS_L_EN3.
// Net BTS_L_EN3 duoc dieu khien boi ESP_L_EN_3 = GPIO14 thong qua opto.
//
// Do co opto keo len 5V o dau ra:
//   GPIO14 HIGH -> opto dan  -> BTS_L_EN3 LOW  -> relay ON
//   GPIO14 LOW  -> opto tat  -> BTS_L_EN3 HIGH -> relay OFF
//
// Vi vay trong code, RELAY_ON phia ESP = HIGH, RELAY_OFF phia ESP = LOW.
// =========================
#define RELAY_PIN             BTS3_LEN   // GPIO14, ESP_L_EN_3 -> opto -> BTS_L_EN3
#define RELAY_ESP_ON_LEVEL    HIGH
#define RELAY_ESP_OFF_LEVEL   LOW
#define RELAY_TOGGLE_DEBOUNCE_MS 180UL

const long SERIAL_BAUD = 115200;

const int JOY_CENTER   = 128;
const int JOY_DEADZONE = 18;

// Quan trong: opto PC817/EL817 + pull-up 10k thuong khong hop voi PWM 18kHz.
// De 500Hz de dau ra opto len/xuong ro hon khi test BTS7960.
const int DRIVE_PWM_FREQ_HZ = 500;
const int DRIVE_PWM_BITS    = 8;
const int DRIVE_PWM_MAX     = 255;

// Gioi han toc do lan dau test.
// Neu motor yeu/khong quay, co the tang 220..255.
const int DRIVE_PWM_LIMIT   = 220;

// Duty nho qua motor co the khong thang ma sat.
// Khi co lenh chay, neu duty nho hon muc nay se nang len muc nay.
const int DRIVE_PWM_MIN_EFFECTIVE = 75;

// Ramp PWM de tranh giat co khi doi huong/tang toc.
const int DRIVE_RAMP_STEP   = 10;
const int LOOP_DELAY_MS     = 20;

const unsigned long DRIVE_LOG_PERIOD_MS = 250UL;

// =========================
// THAM SO GIAO TIEP PS2 DIRECT
// =========================
const int PS2_CLK_DELAY_US  = 8;
const int PS2_BYTE_DELAY_US = 16;
const int PS2_CMD_DELAY_US  = 50;

// =========================
// KIEU DU LIEU
// =========================
enum MoveCmd
{
  CMD_STOP,
  CMD_FORWARD,
  CMD_BACKWARD,
  CMD_LEFT,
  CMD_RIGHT,
  CMD_MIXED,
  CMD_SERIAL_TEST
};

enum Md2uDir
{
  MD2U_STOP = 0,
  MD2U_CW,
  MD2U_CCW
};

struct DriveTarget
{
  int left;       // -255..255
  int right;      // -255..255
  int lxRaw;      // 0..255
  int lyRaw;      // 0..255
  int x;          // -127..127 sau deadzone
  int y;          // -127..127 sau deadzone, tien la duong
  MoveCmd cmd;
};

struct BtsDrive
{
  const char *name;
  int rpwmPin;
  int renPin;
  int lpwmPin;
  int lenPin;
  int rpwmChannel;
  int lpwmChannel;
  bool invertedMotor;
};

struct Ps2State
{
  bool connected;
  bool analogMode;
  uint8_t modeByte;
  uint8_t raw[9];
  uint16_t buttonsRaw;   // bit = 0 la dang nhan, bit = 1 la nha
  int lx;
  int ly;
  int rx;
  int ry;
  unsigned long lastOkMs;
  unsigned long failCount;
};

// Button mask giong PS2X_lib
#define PSB_SELECT    0x0001
#define PSB_L3        0x0002
#define PSB_R3        0x0004
#define PSB_START     0x0008
#define PSB_PAD_UP    0x0010
#define PSB_PAD_RIGHT 0x0020
#define PSB_PAD_DOWN  0x0040
#define PSB_PAD_LEFT  0x0080
#define PSB_L2        0x0100
#define PSB_R2        0x0200
#define PSB_L1        0x0400
#define PSB_R1        0x0800
#define PSB_TRIANGLE  0x1000
#define PSB_CIRCLE    0x2000
#define PSB_CROSS     0x4000
#define PSB_SQUARE    0x8000

BtsDrive driveLeft  = {"P1_LEFT",  BTS1_RPWM, BTS1_REN, BTS1_LPWM, BTS1_LEN, 0, 1, INVERT_LEFT_MOTOR};
BtsDrive driveRight = {"P2_RIGHT", BTS2_RPWM, BTS2_REN, BTS2_LPWM, BTS2_LEN, 2, 3, INVERT_RIGHT_MOTOR};

Ps2State ps2State = {false, false, 0x00, {0}, 0xFFFF, 128, 128, 128, 128, 0, 0};

int currentLeftPwm  = 0;
int currentRightPwm = 0;
DriveTarget lastTarget = {0, 0, 128, 128, 0, 0, CMD_STOP};

Md2uDir md2uCurrentDir = MD2U_STOP;
unsigned long md2uLastChangeMs = 0;
bool md2uHt1 = false;
bool md2uHt2 = false;

bool relayState = false;
bool lastSquarePressed = false;
unsigned long lastRelayToggleMs = 0;

// Serial override de test motor khong can joystick
bool serialOverrideActive = false;
DriveTarget serialTarget = {0, 0, 128, 128, 0, 0, CMD_SERIAL_TEST};

// =========================
// KHAI BAO TRUOC HAM
// =========================
void configPS2Controller();
void updatePS2();
bool ps2ButtonPressed(uint16_t mask);
void ps2Command(const uint8_t *tx, uint8_t *rx, int len);
uint8_t ps2TransferByte(uint8_t out);
void ps2EnterConfig();
void ps2SetAnalogMode();
void ps2ExitConfig();
void ps2PrintRawFrame();

int applyJoystickDeadzone(int value);
DriveTarget readDriveFromLeftJoystick();
DriveTarget readDriveFromDpadFallback();
MoveCmd classifyMoveCommand(int x, int y, int left, int right);
int applyMinEffectiveDuty(int signedDuty);

void initBtsDrivers();
void initOneBtsDrive(BtsDrive &d);
void initUnusedBts3Safe();
void enableBtsDrive(BtsDrive &d, bool enable);
void writeBtsDigitalInput(int pin, bool btsLogicHigh);
void writeBtsPwmInput(int pin, int channel, int btsDuty);
void pwmAttachCompat(int pin, int channel);
void pwmWriteCompat(int pin, int channel, int duty);
int convertBtsDutyToEspDuty(int btsDuty);
void setOneMotor(BtsDrive &d, int speedSigned);
void applyDriveTarget(const DriveTarget &target);
void stopDrive();
void runDriveSelfTest();
int rampToward(int current, int target, int step);
int clampInt(int v, int vmin, int vmax);
int absInt(int v);

void initRelayOutput();
void updateRelayFromPs2();
void setRelayState(bool on);
void toggleRelayState();
void logRelayStatusIfChanged();

void initMd2uStepper();
void updateMd2uStepperControl();
void startMd2uStepper(Md2uDir dir);
void stopMd2uStepper();
void setMd2uHoldOffRun(bool runEnable);
void md2uWriteDriveLogic(int pin, bool driveLevelHigh);
void md2uAttachPulsePin(int pin, int channel);
void md2uWritePulsePin(int pin, int channel, bool pulseEnable);
void md2uWritePwmCompat(int pin, int channel, int duty);
bool isHt1Active();
bool isHt2Active();
const char *md2uDirToText(Md2uDir dir);
void logMd2uStatusIfChanged();

void handleSerialDebugCommand();
void setSerialTarget(int left, int right);
void logStartupInfo();
void logDriveStatus(const DriveTarget &target);
const char *moveCmdToText(MoveCmd cmd);
void updateHeartbeatLed();

void setup()
{
  // Dua relay ve OFF som nhat co the ngay khi ESP32-S3 bat dau chay code.
  // Neu muon relay chac chan OFF ca trong thoi gian reset/bootloader,
  // nen gan them dien tro keo xuong 10k tu GPIO39 ve GND tren phan cung.
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_ESP_OFF_LEVEL);
  relayState = false;

  Serial.begin(SERIAL_BAUD);
  delay(800);

#if HEARTBEAT_LED_ENABLE
  pinMode(HEARTBEAT_LED_GPIO, OUTPUT);
  digitalWrite(HEARTBEAT_LED_GPIO, LOW);
#endif

  initBtsDrivers();
  stopDrive();

  initMd2uStepper();
  initRelayOutput();

#if DRIVE_SELF_TEST_ON_BOOT
  runDriveSelfTest();
#endif

  configPS2Controller();
  logStartupInfo();
}

void loop()
{
  updatePS2();
  updateRelayFromPs2();
  updateMd2uStepperControl();
  handleSerialDebugCommand();

  DriveTarget target;

  if (serialOverrideActive)
  {
    target = serialTarget;
  }
  else
  {
    target = readDriveFromLeftJoystick();

#if ENABLE_SELECT_EMERGENCY_STOP
    if (ps2ButtonPressed(PSB_SELECT))
    {
      target.left = 0;
      target.right = 0;
      target.cmd = CMD_STOP;
    }
#endif
  }

  applyDriveTarget(target);
  logDriveStatus(target);
  logMd2uStatusIfChanged();
  logRelayStatusIfChanged();
  updateHeartbeatLed();

  delay(LOOP_DELAY_MS);
}
