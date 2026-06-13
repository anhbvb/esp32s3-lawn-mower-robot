// =====================================================
// 02_Joystick_Mixer.ino
// Chuyen joystick trai/D-PAD thanh toc do 2 motor
// =====================================================

int applyJoystickDeadzone(int value)
{
  int a = absInt(value);

  if (a <= JOY_DEADZONE)
  {
    return 0;
  }

  int sign = (value >= 0) ? 1 : -1;
  int scaled = (a - JOY_DEADZONE) * 127 / (127 - JOY_DEADZONE);
  return sign * clampInt(scaled, 0, 127);
}

DriveTarget readDriveFromLeftJoystick()
{
  DriveTarget t;

  if (!ps2State.connected)
  {
    t.left = 0;
    t.right = 0;
    t.lxRaw = 128;
    t.lyRaw = 128;
    t.x = 0;
    t.y = 0;
    t.cmd = CMD_STOP;
    lastTarget = t;
    return t;
  }

  if (!ps2State.analogMode)
  {
    t = readDriveFromDpadFallback();
    lastTarget = t;
    return t;
  }

  t.lxRaw = ps2State.lx;
  t.lyRaw = ps2State.ly;

  // PS2: LX < 128 la trai, LX > 128 la phai.
  // PS2: LY < 128 la day len. Quy uoc y duong = tien.
  int xRaw = t.lxRaw - JOY_CENTER;
  int yRaw = JOY_CENTER - t.lyRaw;

  t.x = applyJoystickDeadzone(xRaw);
  t.y = applyJoystickDeadzone(yRaw);

  // Neu joystick dang o giua, van cho phep D-PAD dieu khien de debug.
  if (t.x == 0 && t.y == 0)
  {
    DriveTarget dpad = readDriveFromDpadFallback();
    if (dpad.left != 0 || dpad.right != 0)
    {
      lastTarget = dpad;
      return dpad;
    }
  }

  int leftRaw  = t.y + t.x;
  int rightRaw = t.y - t.x;

  int maxAbs = absInt(leftRaw);
  if (absInt(rightRaw) > maxAbs) maxAbs = absInt(rightRaw);

  if (maxAbs > 127)
  {
    leftRaw  = leftRaw  * 127 / maxAbs;
    rightRaw = rightRaw * 127 / maxAbs;
  }

  t.left  = leftRaw  * DRIVE_PWM_LIMIT / 127;
  t.right = rightRaw * DRIVE_PWM_LIMIT / 127;

  t.left  = clampInt(t.left,  -DRIVE_PWM_LIMIT, DRIVE_PWM_LIMIT);
  t.right = clampInt(t.right, -DRIVE_PWM_LIMIT, DRIVE_PWM_LIMIT);

  t.left  = applyMinEffectiveDuty(t.left);
  t.right = applyMinEffectiveDuty(t.right);

  t.cmd = classifyMoveCommand(t.x, t.y, t.left, t.right);
  lastTarget = t;
  return t;
}

DriveTarget readDriveFromDpadFallback()
{
  DriveTarget t;
  t.lxRaw = 128;
  t.lyRaw = 128;
  t.x = 0;
  t.y = 0;
  t.left = 0;
  t.right = 0;
  t.cmd = CMD_STOP;

  if (!ps2State.connected) return t;

  if (ps2ButtonPressed(PSB_PAD_UP))
  {
    t.left = DRIVE_PWM_LIMIT;
    t.right = DRIVE_PWM_LIMIT;
    t.y = 127;
    t.cmd = CMD_FORWARD;
  }
  else if (ps2ButtonPressed(PSB_PAD_DOWN))
  {
    t.left = -DRIVE_PWM_LIMIT;
    t.right = -DRIVE_PWM_LIMIT;
    t.y = -127;
    t.cmd = CMD_BACKWARD;
  }
  else if (ps2ButtonPressed(PSB_PAD_LEFT))
  {
    t.left = -DRIVE_PWM_LIMIT;
    t.right = DRIVE_PWM_LIMIT;
    t.x = -127;
    t.cmd = CMD_LEFT;
  }
  else if (ps2ButtonPressed(PSB_PAD_RIGHT))
  {
    t.left = DRIVE_PWM_LIMIT;
    t.right = -DRIVE_PWM_LIMIT;
    t.x = 127;
    t.cmd = CMD_RIGHT;
  }

  return t;
}

MoveCmd classifyMoveCommand(int x, int y, int left, int right)
{
  if (left == 0 && right == 0) return CMD_STOP;

  if (absInt(y) >= absInt(x))
  {
    if (y > 0) return CMD_FORWARD;
    if (y < 0) return CMD_BACKWARD;
  }
  else
  {
    if (x < 0) return CMD_LEFT;
    if (x > 0) return CMD_RIGHT;
  }

  return CMD_MIXED;
}

int applyMinEffectiveDuty(int signedDuty)
{
  if (signedDuty == 0) return 0;

  int sign = (signedDuty > 0) ? 1 : -1;
  int a = absInt(signedDuty);

  if (a < DRIVE_PWM_MIN_EFFECTIVE)
  {
    a = DRIVE_PWM_MIN_EFFECTIVE;
  }

  return sign * a;
}
