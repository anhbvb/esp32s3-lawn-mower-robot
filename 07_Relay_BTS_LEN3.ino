// =====================================================
// 07_Relay_BTS_LEN3.ino
// V7: Relay active LOW, chan kich relay noi vao net BTS_L_EN3.
//
// Ket noi thuc te:
//   ESP_L_EN_3 / GPIO14 -> opto -> BTS_L_EN3 -> chan kich relay
//
// Do dau ra opto dang co tro keo len 5V:
//   GPIO14 LOW  -> opto tat -> BTS_L_EN3 HIGH -> relay OFF
//   GPIO14 HIGH -> opto dan -> BTS_L_EN3 LOW  -> relay ON
//
// Nhan nut Vuong/Square tren tay PS2 de toggle bat/tat relay.
// Lenh Serial test:
//   r: toggle relay
//   o: relay OFF
//   p: relay ON
// =====================================================

void initRelayOutput()
{
  // Quan trong: dua GPIO14 ve LOW de opto tat, BTS_L_EN3 duoc keo len HIGH,
  // relay active LOW se OFF.
  digitalWrite(RELAY_PIN, RELAY_ESP_OFF_LEVEL);
  pinMode(RELAY_PIN, OUTPUT);
  setRelayState(false);

  lastSquarePressed = ps2ButtonPressed(PSB_SQUARE);

  Serial.print(F("[RELAY] Khoi tao relay qua BTS_L_EN3, ESP pin GPIO"));
  Serial.print(RELAY_PIN);
  Serial.println(F("."));
  Serial.println(F("[RELAY] Relay active LOW o phia BTS_L_EN3, nhung ESP GPIO14 HIGH moi lam relay ON do qua opto."));
  Serial.println(F("[RELAY] Mac dinh OFF khi code bat dau chay."));
  Serial.println(F("[RELAY] Nhan nut Vuong/Square tren PS2 de bat/tat relay."));
  Serial.println(F("[RELAY] Lenh Serial test: r=toggle, p=ON, o=OFF."));
}

void updateRelayFromPs2()
{
  if (!ps2State.connected)
  {
    lastSquarePressed = false;
    return;
  }

  bool squarePressed = ps2ButtonPressed(PSB_SQUARE);
  unsigned long now = millis();

  // Toggle theo canh nhan: giu nut vuong khong bi bat/tat lien tuc.
  if (squarePressed && !lastSquarePressed)
  {
    if (now - lastRelayToggleMs >= RELAY_TOGGLE_DEBOUNCE_MS)
    {
      lastRelayToggleMs = now;
      toggleRelayState();
      Serial.println(F("[RELAY] Toggle by PS2 SQUARE."));
    }
  }

  lastSquarePressed = squarePressed;
}

void setRelayState(bool on)
{
  relayState = on;

  // relayState la trang thai mong muon cua relay.
  // Do relay active LOW nhung qua opto bi dao, GPIO14 HIGH se lam relay ON.
  digitalWrite(RELAY_PIN, relayState ? RELAY_ESP_ON_LEVEL : RELAY_ESP_OFF_LEVEL);
}

void toggleRelayState()
{
  setRelayState(!relayState);

  Serial.print(F("[RELAY] Toggle -> "));
  Serial.println(relayState ? F("ON") : F("OFF"));
}

void logRelayStatusIfChanged()
{
  static bool lastRelayState = false;
  static bool lastSquare = false;
  static bool first = true;
  static unsigned long lastPeriodicMs = 0;

  bool squareNow = ps2ButtonPressed(PSB_SQUARE);
  bool changed = first || (lastRelayState != relayState) || (lastSquare != squareNow);
  bool periodic = millis() - lastPeriodicMs >= 1000UL;

  if (!changed && !periodic) return;

  first = false;
  lastPeriodicMs = millis();
  lastRelayState = relayState;
  lastSquare = squareNow;

  Serial.print(F("[RELAY] BTS_L_EN3/GPIO"));
  Serial.print(RELAY_PIN);
  Serial.print(F(" = "));
  Serial.print(relayState ? F("ON") : F("OFF"));
  Serial.print(F(" | ESP_LEVEL="));
  Serial.print(digitalRead(RELAY_PIN) ? F("HIGH") : F("LOW"));
  Serial.print(F(" | SQUARE="));
  Serial.print(squareNow ? 1 : 0);
  Serial.print(F(" | buttonsRaw=0x"));
  Serial.println(ps2State.buttonsRaw, HEX);
}
