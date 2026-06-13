// =====================================================
// 03_BTS7960_Drive.ino
// Dieu khien 2 BTS7960 qua opto cach ly
// P1 = motor trai, P2 = motor phai
// =====================================================

void initBtsDrivers()
{
  initOneBtsDrive(driveLeft);
  initOneBtsDrive(driveRight);
  initUnusedBts3Safe();

  enableBtsDrive(driveLeft, true);
  enableBtsDrive(driveRight, true);

  Serial.println(F("[BTS] Da khoi tao 2 drive BTS7960 P1/P2."));
}

void initOneBtsDrive(BtsDrive &d)
{
  pinMode(d.renPin, OUTPUT);
  pinMode(d.lenPin, OUTPUT);

  writeBtsDigitalInput(d.renPin, false);
  writeBtsDigitalInput(d.lenPin, false);

  pwmAttachCompat(d.rpwmPin, d.rpwmChannel);
  pwmAttachCompat(d.lpwmPin, d.lpwmChannel);

  writeBtsPwmInput(d.rpwmPin, d.rpwmChannel, 0);
  writeBtsPwmInput(d.lpwmPin, d.lpwmChannel, 0);

  Serial.print(F("[BTS] "));
  Serial.print(d.name);
  Serial.print(F(" RPWM=GPIO")); Serial.print(d.rpwmPin);
  Serial.print(F(", REN=GPIO")); Serial.print(d.renPin);
  Serial.print(F(", LPWM=GPIO")); Serial.print(d.lpwmPin);
  Serial.print(F(", LEN=GPIO")); Serial.println(d.lenPin);
}

void initUnusedBts3Safe()
{
  // P3 khong dung de chay motor, nhung BTS3_LEN/GPIO14 da duoc dung lam relay.
  // Khong duoc writeBtsDigitalInput(BTS3_LEN, false) nua,
  // vi net BTS_L_EN3 dang noi vao relay active LOW, ghi sai co the lam relay ON.

  pinMode(BTS3_REN, OUTPUT);
  writeBtsDigitalInput(BTS3_REN, false);

  pwmAttachCompat(BTS3_RPWM, 4);
  pwmAttachCompat(BTS3_LPWM, 5);
  writeBtsPwmInput(BTS3_RPWM, 4, 0);
  writeBtsPwmInput(BTS3_LPWM, 5, 0);

  Serial.println(F("[BTS] P3 chua dung; BTS3_LEN/GPIO14 duoc tach ra lam relay."));
}

void enableBtsDrive(BtsDrive &d, bool enable)
{
  writeBtsDigitalInput(d.renPin, enable);
  writeBtsDigitalInput(d.lenPin, enable);
}

void writeBtsDigitalInput(int pin, bool btsLogicHigh)
{
#if BTS_OPTO_INVERTED
  bool espLevel = !btsLogicHigh;
#else
  bool espLevel = btsLogicHigh;
#endif
  digitalWrite(pin, espLevel ? HIGH : LOW);
}

int convertBtsDutyToEspDuty(int btsDuty)
{
  btsDuty = clampInt(btsDuty, 0, DRIVE_PWM_MAX);

#if BTS_OPTO_INVERTED
  return DRIVE_PWM_MAX - btsDuty;
#else
  return btsDuty;
#endif
}

void writeBtsPwmInput(int pin, int channel, int btsDuty)
{
  btsDuty = clampInt(btsDuty, 0, DRIVE_PWM_MAX);
  int espDuty = convertBtsDutyToEspDuty(btsDuty);
  pwmWriteCompat(pin, channel, espDuty);
}

void pwmAttachCompat(int pin, int channel)
{
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, DRIVE_PWM_FREQ_HZ, DRIVE_PWM_BITS);
#else
  ledcSetup(channel, DRIVE_PWM_FREQ_HZ, DRIVE_PWM_BITS);
  ledcAttachPin(pin, channel);
#endif
}

void pwmWriteCompat(int pin, int channel, int duty)
{
  duty = clampInt(duty, 0, DRIVE_PWM_MAX);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);
#else
  ledcWrite(channel, duty);
#endif
}

void setOneMotor(BtsDrive &d, int speedSigned)
{
  speedSigned = clampInt(speedSigned, -DRIVE_PWM_MAX, DRIVE_PWM_MAX);

  if (d.invertedMotor)
  {
    speedSigned = -speedSigned;
  }

  int duty = absInt(speedSigned);

  if (speedSigned > 0)
  {
    writeBtsPwmInput(d.rpwmPin, d.rpwmChannel, duty);
    writeBtsPwmInput(d.lpwmPin, d.lpwmChannel, 0);
  }
  else if (speedSigned < 0)
  {
    writeBtsPwmInput(d.rpwmPin, d.rpwmChannel, 0);
    writeBtsPwmInput(d.lpwmPin, d.lpwmChannel, duty);
  }
  else
  {
    writeBtsPwmInput(d.rpwmPin, d.rpwmChannel, 0);
    writeBtsPwmInput(d.lpwmPin, d.lpwmChannel, 0);
  }
}

void applyDriveTarget(const DriveTarget &target)
{
  currentLeftPwm  = rampToward(currentLeftPwm,  target.left,  DRIVE_RAMP_STEP);
  currentRightPwm = rampToward(currentRightPwm, target.right, DRIVE_RAMP_STEP);

  setOneMotor(driveLeft, currentLeftPwm);
  setOneMotor(driveRight, currentRightPwm);
}

void stopDrive()
{
  currentLeftPwm = 0;
  currentRightPwm = 0;
  setOneMotor(driveLeft, 0);
  setOneMotor(driveRight, 0);
}

void runDriveSelfTest()
{
  Serial.println(F("[TEST] Bat dau self-test motor. Hay nang banh xe len!"));

  setOneMotor(driveLeft, 160);
  setOneMotor(driveRight, 0);
  delay(700);
  stopDrive();
  delay(300);

  setOneMotor(driveLeft, -160);
  setOneMotor(driveRight, 0);
  delay(700);
  stopDrive();
  delay(300);

  setOneMotor(driveLeft, 0);
  setOneMotor(driveRight, 160);
  delay(700);
  stopDrive();
  delay(300);

  setOneMotor(driveLeft, 0);
  setOneMotor(driveRight, -160);
  delay(700);
  stopDrive();
  delay(300);

  Serial.println(F("[TEST] Ket thuc self-test motor."));
}

int rampToward(int current, int target, int step)
{
  if (current < target)
  {
    current += step;
    if (current > target) current = target;
  }
  else if (current > target)
  {
    current -= step;
    if (current < target) current = target;
  }
  return current;
}

int clampInt(int v, int vmin, int vmax)
{
  if (v < vmin) return vmin;
  if (v > vmax) return vmax;
  return v;
}

int absInt(int v)
{
  return (v < 0) ? -v : v;
}
