// =====================================================
// 04_Serial_Debug.ino
// Lenh test qua Serial de tach loi PS2 va loi drive
// =====================================================

void handleSerialDebugCommand()
{
  while (Serial.available() > 0)
  {
    char c = Serial.read();

    if (c == '\r' || c == '\n' || c == ' ') continue;

    switch (c)
    {
      case 'w':
      case 'W':
        setSerialTarget(DRIVE_PWM_LIMIT, DRIVE_PWM_LIMIT);
        Serial.println(F("[SERIAL] w: tien"));
        break;

      case 's':
      case 'S':
        setSerialTarget(-DRIVE_PWM_LIMIT, -DRIVE_PWM_LIMIT);
        Serial.println(F("[SERIAL] s: lui"));
        break;

      case 'a':
      case 'A':
        setSerialTarget(-DRIVE_PWM_LIMIT, DRIVE_PWM_LIMIT);
        Serial.println(F("[SERIAL] a: quay trai"));
        break;

      case 'd':
      case 'D':
        setSerialTarget(DRIVE_PWM_LIMIT, -DRIVE_PWM_LIMIT);
        Serial.println(F("[SERIAL] d: quay phai"));
        break;

      case '1':
        setSerialTarget(DRIVE_PWM_LIMIT, 0);
        Serial.println(F("[SERIAL] 1: chi P1 tien"));
        break;

      case '2':
        setSerialTarget(-DRIVE_PWM_LIMIT, 0);
        Serial.println(F("[SERIAL] 2: chi P1 lui"));
        break;

      case '3':
        setSerialTarget(0, DRIVE_PWM_LIMIT);
        Serial.println(F("[SERIAL] 3: chi P2 tien"));
        break;

      case '4':
        setSerialTarget(0, -DRIVE_PWM_LIMIT);
        Serial.println(F("[SERIAL] 4: chi P2 lui"));
        break;

      case 'x':
      case 'X':
        serialOverrideActive = false;
        serialTarget.left = 0;
        serialTarget.right = 0;
        stopDrive();
        Serial.println(F("[SERIAL] x: dung, tra ve dieu khien joystick"));
        break;

      case 't':
      case 'T':
        Serial.println(F("[SERIAL] t: chay self-test motor"));
        runDriveSelfTest();
        break;

      case 'r':
      case 'R':
        toggleRelayState();
        Serial.println(F("[SERIAL] r: toggle relay"));
        break;

      case 'p':
      case 'P':
        setRelayState(true);
        Serial.println(F("[SERIAL] p: relay ON"));
        break;

      case 'o':
      case 'O':
        setRelayState(false);
        Serial.println(F("[SERIAL] o: relay OFF"));
        break;

      case 'h':
      case 'H':
      case '?':
        Serial.println(F("[HELP] Lenh Serial:"));
        Serial.println(F("  w: tien | s: lui | a: trai | d: phai | x: stop/thoat override"));
        Serial.println(F("  1: P1 tien | 2: P1 lui | 3: P2 tien | 4: P2 lui | t: self-test"));
        Serial.println(F("  r: toggle relay | p: relay ON | o: relay OFF"));
        break;

      default:
        Serial.print(F("[SERIAL] Lenh khong biet: "));
        Serial.println(c);
        break;
    }
  }
}

void setSerialTarget(int left, int right)
{
  serialOverrideActive = true;
  serialTarget.left = clampInt(left, -DRIVE_PWM_LIMIT, DRIVE_PWM_LIMIT);
  serialTarget.right = clampInt(right, -DRIVE_PWM_LIMIT, DRIVE_PWM_LIMIT);
  serialTarget.left = applyMinEffectiveDuty(serialTarget.left);
  serialTarget.right = applyMinEffectiveDuty(serialTarget.right);
  serialTarget.lxRaw = 128;
  serialTarget.lyRaw = 128;
  serialTarget.x = 0;
  serialTarget.y = 0;
  serialTarget.cmd = CMD_SERIAL_TEST;
}
