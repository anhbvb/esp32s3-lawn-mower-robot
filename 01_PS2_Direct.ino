// =====================================================
// 01_PS2_Direct.ino
// Doc tay cam PS2 bang bit-bang truc tiep
// =====================================================

void configPS2Controller()
{
  pinMode(PS2_DAT, INPUT_PULLUP);
  pinMode(PS2_CMD, OUTPUT);
  pinMode(PS2_CLK, OUTPUT);
  pinMode(PS2_CS,  OUTPUT);

  digitalWrite(PS2_CMD, HIGH);
  digitalWrite(PS2_CLK, HIGH);
  digitalWrite(PS2_CS,  HIGH);

  Serial.println(F("[PS2] Khoi tao giao tiep PS2 direct, khong dung PS2X_lib."));

  for (int i = 0; i < 5; i++)
  {
    ps2EnterConfig();
    delay(10);
    ps2SetAnalogMode();
    delay(10);
    ps2ExitConfig();
    delay(50);

    updatePS2();
    if (ps2State.connected && ps2State.analogMode)
    {
      Serial.print(F("[PS2] Ket noi thanh cong. Mode=0x"));
      Serial.println(ps2State.modeByte, HEX);
      return;
    }
  }

  if (ps2State.connected)
  {
    Serial.print(F("[PS2] Co phan hoi nhung chua vao analog mode. Mode=0x"));
    Serial.println(ps2State.modeByte, HEX);
    Serial.println(F("[PS2] Se dung nut D-PAD de fallback neu joystick analog khong co du lieu."));
  }
  else
  {
    Serial.println(F("[PS2] CANH BAO: chua ket noi duoc tay cam/receiver PS2."));
    Serial.println(F("[PS2] Kiem tra DAT=GPIO19, CMD=GPIO20, CS=GPIO21, CLK=GPIO47, nguon 3V3/GND."));
  }
}

void updatePS2()
{
  uint8_t tx[9] = {0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t rx[9] = {0};

  ps2Command(tx, rx, 9);

  for (int i = 0; i < 9; i++) ps2State.raw[i] = rx[i];

  ps2State.modeByte = rx[1];

  bool validFrame = (rx[2] == 0x5A) && (rx[1] == 0x41 || rx[1] == 0x73 || rx[1] == 0x79);

  if (validFrame)
  {
    ps2State.connected = true;
    ps2State.analogMode = (rx[1] == 0x73 || rx[1] == 0x79);
    ps2State.buttonsRaw = ((uint16_t)rx[4] << 8) | rx[3];
    ps2State.lastOkMs = millis();

    if (ps2State.analogMode)
    {
      ps2State.rx = rx[5];
      ps2State.ry = rx[6];
      ps2State.lx = rx[7];
      ps2State.ly = rx[8];
    }
    else
    {
      ps2State.rx = 128;
      ps2State.ry = 128;
      ps2State.lx = 128;
      ps2State.ly = 128;
    }
  }
  else
  {
    ps2State.failCount++;

    if (millis() - ps2State.lastOkMs > 300)
    {
      ps2State.connected = false;
      ps2State.analogMode = false;
      ps2State.buttonsRaw = 0xFFFF;
      ps2State.lx = 128;
      ps2State.ly = 128;
      ps2State.rx = 128;
      ps2State.ry = 128;
    }
  }
}

bool ps2ButtonPressed(uint16_t mask)
{
  if (!ps2State.connected) return false;
  return (ps2State.buttonsRaw & mask) == 0;
}

void ps2Command(const uint8_t *tx, uint8_t *rx, int len)
{
  digitalWrite(PS2_CS, LOW);
  delayMicroseconds(PS2_CMD_DELAY_US);

  for (int i = 0; i < len; i++)
  {
    rx[i] = ps2TransferByte(tx[i]);
    delayMicroseconds(PS2_BYTE_DELAY_US);
  }

  digitalWrite(PS2_CMD, HIGH);
  digitalWrite(PS2_CLK, HIGH);
  digitalWrite(PS2_CS, HIGH);
  delayMicroseconds(PS2_CMD_DELAY_US);
}

uint8_t ps2TransferByte(uint8_t out)
{
  uint8_t in = 0;

  for (int bit = 0; bit < 8; bit++)
  {
    digitalWrite(PS2_CMD, (out & (1 << bit)) ? HIGH : LOW);
    delayMicroseconds(PS2_CLK_DELAY_US);

    digitalWrite(PS2_CLK, LOW);
    delayMicroseconds(PS2_CLK_DELAY_US);

    if (digitalRead(PS2_DAT))
    {
      in |= (1 << bit);
    }

    digitalWrite(PS2_CLK, HIGH);
    delayMicroseconds(PS2_CLK_DELAY_US);
  }

  digitalWrite(PS2_CMD, HIGH);
  return in;
}

void ps2EnterConfig()
{
  uint8_t tx[5] = {0x01, 0x43, 0x00, 0x01, 0x00};
  uint8_t rx[5] = {0};
  ps2Command(tx, rx, 5);
}

void ps2SetAnalogMode()
{
  uint8_t tx[9] = {0x01, 0x44, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00};
  uint8_t rx[9] = {0};
  ps2Command(tx, rx, 9);
}

void ps2ExitConfig()
{
  uint8_t tx[9] = {0x01, 0x43, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};
  uint8_t rx[9] = {0};
  ps2Command(tx, rx, 9);
}

void ps2PrintRawFrame()
{
  Serial.print(F("RAW:"));
  for (int i = 0; i < 9; i++)
  {
    Serial.print(' ');
    if (ps2State.raw[i] < 16) Serial.print('0');
    Serial.print(ps2State.raw[i], HEX);
  }
}
