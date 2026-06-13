// =====================================================
// 06_MD2U_Stepper.ino
// Dieu khien drive step MD2U-MD20 bang nut PS2
//
// Ket noi:
//   Tam giac  -> quay thuan/CW, phat xung tren MOTOR_DIR/MOTOR_DTR -> GPIO17
//   X/Cross   -> quay nguoc/CCW, phat xung tren MOTOR_EN           -> GPIO16
//   HOLD OFF  -> MOTOR_STEP -> GPIO15
//   HT1       -> gioi han chieu thuan, mac dinh GPIO1 active HIGH
//   HT2       -> gioi han chieu nguoc, mac dinh GPIO2 active HIGH
// =====================================================

#define MD2U_CW_CHANNEL   6
#define MD2U_CCW_CHANNEL  7

void initMd2uStepper()
{
  pinMode(HT1_PIN, INPUT_PULLDOWN);
  pinMode(HT2_PIN, INPUT_PULLDOWN);

  pinMode(MD2U_HOLDOFF_PIN, OUTPUT);
  setMd2uHoldOffRun(true);

  md2uAttachPulsePin(MD2U_CW_PIN, MD2U_CW_CHANNEL);
  md2uAttachPulsePin(MD2U_CCW_PIN, MD2U_CCW_CHANNEL);

  stopMd2uStepper();

  Serial.println(F("[MD2U] Da khoi tao drive step MD2U-MD20."));
  Serial.print(F("[MD2U] CW -> GPIO")); Serial.print(MD2U_CW_PIN);
  Serial.print(F(", CCW -> GPIO")); Serial.print(MD2U_CCW_PIN);
  Serial.print(F(", HOLD_OFF -> GPIO")); Serial.println(MD2U_HOLDOFF_PIN);
  Serial.print(F("[MD2U] HT1 -> GPIO")); Serial.print(HT1_PIN);
  Serial.print(F(", HT2 -> GPIO")); Serial.print(HT2_PIN);
  Serial.print(F(", HT_ACTIVE_LEVEL=")); Serial.println(HT_ACTIVE_LEVEL == HIGH ? F("HIGH") : F("LOW"));
  Serial.print(F("[MD2U] STEP_FREQ=")); Serial.print(MD2U_STEP_FREQ_HZ);
  Serial.println(F(" Hz"));
}

void updateMd2uStepperControl()
{
  md2uHt1 = isHt1Active();
  md2uHt2 = isHt2Active();

#if MD2U_SELECT_EMERGENCY_STOP
  if (ps2ButtonPressed(PSB_SELECT))
  {
    stopMd2uStepper();
    return;
  }
#endif

  if (!ps2State.connected)
  {
    stopMd2uStepper();
    return;
  }

  bool trianglePressed = ps2ButtonPressed(PSB_TRIANGLE);
  bool crossPressed    = ps2ButtonPressed(PSB_CROSS);

  // Neu nhan dong thoi 2 nut thi dung de tranh phat 2 chieu cung luc.
  if (trianglePressed && crossPressed)
  {
    stopMd2uStepper();
    return;
  }

  // Tam giac: quay thuan/CW, bi chan boi HT1.
  if (trianglePressed)
  {
    if (md2uHt1)
    {
      stopMd2uStepper();
    }
    else
    {
      startMd2uStepper(MD2U_CW);
    }
    return;
  }

  // X/Cross: quay nguoc/CCW, bi chan boi HT2.
  if (crossPressed)
  {
    if (md2uHt2)
    {
      stopMd2uStepper();
    }
    else
    {
      startMd2uStepper(MD2U_CCW);
    }
    return;
  }

  stopMd2uStepper();
}

void startMd2uStepper(Md2uDir dir)
{
  if (dir == MD2U_STOP)
  {
    stopMd2uStepper();
    return;
  }

  // Bao ve lap lai: neu dang chay chieu do roi thi khong ghi lai lien tuc.
  if (md2uCurrentDir == dir)
  {
    return;
  }

  // Tat ca xung truoc khi doi chieu.
  md2uWritePulsePin(MD2U_CW_PIN, MD2U_CW_CHANNEL, false);
  md2uWritePulsePin(MD2U_CCW_PIN, MD2U_CCW_CHANNEL, false);
  delayMicroseconds(50);

  // Dam bao HOLD OFF dang o trang thai cho phep drive chay.
  setMd2uHoldOffRun(true);

  if (dir == MD2U_CW)
  {
    md2uWritePulsePin(MD2U_CW_PIN, MD2U_CW_CHANNEL, true);
    md2uWritePulsePin(MD2U_CCW_PIN, MD2U_CCW_CHANNEL, false);
  }
  else if (dir == MD2U_CCW)
  {
    md2uWritePulsePin(MD2U_CW_PIN, MD2U_CW_CHANNEL, false);
    md2uWritePulsePin(MD2U_CCW_PIN, MD2U_CCW_CHANNEL, true);
  }

  md2uCurrentDir = dir;
  md2uLastChangeMs = millis();

  Serial.print(F("[MD2U] Start "));
  Serial.println(md2uDirToText(dir));
}

void stopMd2uStepper()
{
  if (md2uCurrentDir != MD2U_STOP)
  {
    Serial.println(F("[MD2U] Stop"));
  }

  md2uWritePulsePin(MD2U_CW_PIN, MD2U_CW_CHANNEL, false);
  md2uWritePulsePin(MD2U_CCW_PIN, MD2U_CCW_CHANNEL, false);

  md2uCurrentDir = MD2U_STOP;
  md2uLastChangeMs = millis();
}

void setMd2uHoldOffRun(bool runEnable)
{
  // runEnable=true: ghi muc logic phia drive theo MD2U_HOLDOFF_RUN_LEVEL.
  // runEnable=false: ghi muc nguoc lai, tuong duong HOLD OFF active.
  bool driveLevel = runEnable ? MD2U_HOLDOFF_RUN_LEVEL : !MD2U_HOLDOFF_RUN_LEVEL;
  md2uWriteDriveLogic(MD2U_HOLDOFF_PIN, driveLevel);
}

void md2uWriteDriveLogic(int pin, bool driveLevelHigh)
{
#if MD2U_OPTO_INVERTED
  bool espLevel = !driveLevelHigh;
#else
  bool espLevel = driveLevelHigh;
#endif
  digitalWrite(pin, espLevel ? HIGH : LOW);
}

void md2uAttachPulsePin(int pin, int channel)
{
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, MD2U_STEP_FREQ_HZ, MD2U_PWM_BITS);
#else
  ledcSetup(channel, MD2U_STEP_FREQ_HZ, MD2U_PWM_BITS);
  ledcAttachPin(pin, channel);
#endif
}

void md2uWritePulsePin(int pin, int channel, bool pulseEnable)
{
  int duty;

  if (pulseEnable)
  {
    duty = MD2U_PULSE_DUTY;
  }
  else
  {
    // Giu muc idle phia drive khi khong phat xung.
#if MD2U_OPTO_INVERTED
    duty = (MD2U_PULSE_IDLE_LEVEL == HIGH) ? 0 : 255;
#else
    duty = (MD2U_PULSE_IDLE_LEVEL == HIGH) ? 255 : 0;
#endif
  }

  md2uWritePwmCompat(pin, channel, duty);
}

void md2uWritePwmCompat(int pin, int channel, int duty)
{
  duty = clampInt(duty, 0, 255);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);
#else
  ledcWrite(channel, duty);
#endif
}

bool isHt1Active()
{
  return digitalRead(HT1_PIN) == HT_ACTIVE_LEVEL;
}

bool isHt2Active()
{
  return digitalRead(HT2_PIN) == HT_ACTIVE_LEVEL;
}

const char *md2uDirToText(Md2uDir dir)
{
  switch (dir)
  {
    case MD2U_CW:   return "CW/THUAN";
    case MD2U_CCW:  return "CCW/NGUOC";
    default:        return "STOP";
  }
}

void logMd2uStatusIfChanged()
{
  static Md2uDir lastDir = MD2U_STOP;
  static bool lastHt1 = false;
  static bool lastHt2 = false;
  static unsigned long lastLogMs = 0;

  bool changed = (lastDir != md2uCurrentDir) || (lastHt1 != md2uHt1) || (lastHt2 != md2uHt2);
  bool periodic = (millis() - lastLogMs >= 1000UL);

  if (!changed && !periodic) return;

  lastLogMs = millis();
  lastDir = md2uCurrentDir;
  lastHt1 = md2uHt1;
  lastHt2 = md2uHt2;

  Serial.print(F("[MD2U] dir="));
  Serial.print(md2uDirToText(md2uCurrentDir));
  Serial.print(F(" | HT1="));
  Serial.print(md2uHt1 ? 1 : 0);
  Serial.print(F(" | HT2="));
  Serial.print(md2uHt2 ? 1 : 0);
  Serial.print(F(" | TRI="));
  Serial.print(ps2ButtonPressed(PSB_TRIANGLE) ? 1 : 0);
  Serial.print(F(" | X="));
  Serial.println(ps2ButtonPressed(PSB_CROSS) ? 1 : 0);
}
