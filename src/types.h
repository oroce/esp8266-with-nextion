struct State {
  bool isWifiConnected;
  bool isMQTTConnected;
  float bedroomTemperature;
  float bedroomHumidity;
  float livingroomTemperature;
  float livingroomHumidity;
  float loggiaTemperature;
  float loggiaHumidity;
  const char* frontdoorStatus;
  unsigned long updatedAt;
};