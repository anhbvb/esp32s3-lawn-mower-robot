// =====================================================
// 05_Log_UART.ino
// Log trang thai joystick va motor
// =====================================================

void logStartupInfo()
{
  Serial.println();
  Serial.println(F("================================================"));
  Serial.println(F("ESP32-S3 BTS7960 PS2 LEFT JOYSTICK - V3 LOW PWM DEBUG"));
  Serial.println(F("================================================"));
  Serial.println(F("Sua chinh V3: PWM giam xuong 500Hz de qua opto on dinh hon."));
  Serial.print(F("PS2_DAT=GPIO")); Serial.println(PS2_DAT);
  Serial.print(F("PS2_CMD=GPIO")); Serial.println(PS2_CMD);
  Serial.print(F("PS2_CS =GPIO")); Serial.println(PS2_CS);
  Serial.print(F("PS2_CLK=GPIO")); Serial.println(PS2_CLK);
  Serial.println();
  Serial.println(F("P1 = motor trai: RPWM GPIO4, REN GPIO5, LPWM GPIO6, LEN GPIO7"));
  Serial.println(F("P2 = motor phai: RPWM GPIO8, REN GPIO18, LPWM GPIO9, LEN GPIO10"));
  Serial.println();
  Serial.print(F("BTS_OPTO_INVERTED       = ")); Serial.println(BTS_OPTO_INVERTED);
  Serial.print(F("DRIVE_PWM_FREQ_HZ       = ")); Serial.println(DRIVE_PWM_FREQ_HZ);
  Serial.print(F("DRIVE_PWM_LIMIT         = ")); Serial.println(DRIVE_PWM_LIMIT);
  Serial.print(F("DRIVE_PWM_MIN_EFFECTIVE = ")); Serial.println(DRIVE_PWM_MIN_EFFECTIVE);
  Serial.println();
  Serial.println(F("Joystick trai: len=tien, xuong=lui, trai=quay trai, phai=quay phai."));
  Serial.println(F("Neu joystick khong co analog, D-PAD se dieu khien fallback."));
  Serial.println(F("Lenh Serial debug: w/s/a/d/x, 1/2/3/4, t, h"));
  Serial.println(F("================================================"));
  Serial.println();
}

void logDriveStatus(const DriveTarget &target)
{
  static unsigned long lastLogMs = 0;
  unsigned long now = millis();
  if (now - lastLogMs < DRIVE_LOG_PERIOD_MS) return;
  lastLogMs = now;

  Serial.print(F("[RUN] PS2="));
  Serial.print(ps2State.connected ? F("OK") : F("LOST"));
  Serial.print(F(" mode=0x"));
  Serial.print(ps2State.modeByte, HEX);
  Serial.print(F(" analog="));
  Serial.print(ps2State.analogMode ? 1 : 0);

  Serial.print(F(" | LX=")); Serial.print(target.lxRaw);
  Serial.print(F(" LY=")); Serial.print(target.lyRaw);
  Serial.print(F(" X=")); Serial.print(target.x);
  Serial.print(F(" Y=")); Serial.print(target.y);

  Serial.print(F(" | target L=")); Serial.print(target.left);
  Serial.print(F(" R=")); Serial.print(target.right);

  Serial.print(F(" | pwm L=")); Serial.print(currentLeftPwm);
  Serial.print(F(" R=")); Serial.print(currentRightPwm);

  Serial.print(F(" | ESPduty P1_R=")); Serial.print(convertBtsDutyToEspDuty(currentLeftPwm > 0 ? absInt(currentLeftPwm) : 0));
  Serial.print(F(" P1_L=")); Serial.print(convertBtsDutyToEspDuty(currentLeftPwm < 0 ? absInt(currentLeftPwm) : 0));
  Serial.print(F(" P2_R=")); Serial.print(convertBtsDutyToEspDuty(currentRightPwm > 0 ? absInt(currentRightPwm) : 0));
  Serial.print(F(" P2_L=")); Serial.print(convertBtsDutyToEspDuty(currentRightPwm < 0 ? absInt(currentRightPwm) : 0));

  Serial.print(F(" | CMD=")); Serial.print(moveCmdToText(target.cmd));
  Serial.print(F(" | serial=")); Serial.print(serialOverrideActive ? 1 : 0);
  Serial.print(F(" | fail=")); Serial.print(ps2State.failCount);
  Serial.print(' ');
  ps2PrintRawFrame();
  Serial.println();
}

const char *moveCmdToText(MoveCmd cmd)
{
  switch (cmd)
  {
    case CMD_STOP:        return "STOP";
    case CMD_FORWARD:     return "FORWARD";
    case CMD_BACKWARD:    return "BACKWARD";
    case CMD_LEFT:        return "LEFT";
    case CMD_RIGHT:       return "RIGHT";
    case CMD_MIXED:       return "MIXED";
    case CMD_SERIAL_TEST: return "SERIAL_TEST";
    default:              return "UNKNOWN";
  }
}

void updateHeartbeatLed()
{
#if HEARTBEAT_LED_ENABLE
  static unsigned long lastMs = 0;
  static bool state = false;
  unsigned long now = millis();

  if (now - lastMs >= HEARTBEAT_PERIOD_MS)
  {
    lastMs = now;
    state = !state;
    digitalWrite(HEARTBEAT_LED_GPIO, state ? HIGH : LOW);
  }
#endif
}
