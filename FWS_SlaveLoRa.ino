/**
 * Author:  Hoang Van Binh (gmail:binhhv.23.1.99@gmail.com)
 *          Che Nam Hoang (gmail: )
 *          
 * Description:
 *  Nhận và phản hồi lệnh AT từ STM32 (Master)
 *  Nhận và phản hồi gói tin LoRa qua RFM95W
 */

#include <string.h>
#include <stdint.h>
#include <SoftwareSerial.h>
#include <LoRa.h>
#include <SPI.h>

/* LoRa parameters*/
uint32_t frequencyBand = 0; //9214E5;
uint32_t bandwidth = 0; //125E6
int spreadingFactor = 0;

/* Pins for LoRa module */
const int nssPin = 10;
const int resetPin = 8;
const int irqPin = 2;

/* Serial dùng để debugPort */
SoftwareSerial debugPort(3, 4);
String stm32Command; // Lưu lệnh AT từ STM32
String atmegaResponse; // Phản hồi lệnh AT 

// Hàm xử lý nhận lệnh AT
void SerialProcess();

// Hàm xử lý nhận gói tin LoRa 
void LoRaProcess();

void setup() {
  // put your setup code here, to run once:
  debugPort.begin(115200);
  Serial.begin(9600);
  Serial.println("\r\nLORA: Ready\r\n");

  SPI.begin();
  LoRa.setPins(nssPin, resetPin, irqPin);
  debugPort.println("SPI config sucessful");
  if (!LoRa.begin(frequencyBand)) {
    debugPort.println("LoRa init failed!!");
  }
  LoRa.setSpreadingFactor(spreadingFactor);
  LoRa.setSignalBandwidth(bandwidth);
  debugPort.println("LoRa init successful");
  debugPort.println("LoRa's looking for packet");
}

void loop() {
  SerialProcess();
  LoRaProcess();
}

/********************** UART ********************/
void SerialProcess()
{
  if (Serial.available()) {
    stm32Command = Serial.readStringUntil('\r');
    Serial.readString();

    debugPort.print(stm32Command); //in lenh ra man hinh debug

    const int numberOfCommand = 5; //Sửa số command ở đây
    const String commandHeader[numberOfCommand] = {"ATI", "AT+LORACFG?", "AT+LORACFG=", "AT+LORASEND=", "AT"};

    void (*reponse[numberOfCommand])() = {ATI, r_LORACFG, w_LORACFG, LORASEND, AT }; // không đưa lệnh AT lên đầu tiên, vì nó sẽ luôn tìm thấy AT trong các lệnh AT

    for (int i = 0; i < numberOfCommand; i++)
    {
      int index = stm32Command.indexOf(commandHeader[i]);
      if (index >= 0)
      {
        reponse[i]();
        break;
      }
      else
      {
        atmegaResponse = "\r\nERROR\r\n";
      }
    }
    //    Serial.print(stm32Command); //in lenh command
    Serial.print(atmegaResponse); //in phan hoi
  }
}

void AT()
{
  atmegaResponse = "\r\nOK\r\n";
}

void ATI()
{
  atmegaResponse = "\r\nManufacture: THTECH\nModel: LoRa RFM95W\nRevision: 1.0.0\r\nOK\r\n";
}

void w_LORACFG()
{
  int numArgs = sscanf(stm32Command.c_str(), "AT+LORACFG=%lu,%lu,%d",
                       &frequencyBand, &bandwidth, &spreadingFactor);
  if (LoRa.begin(frequencyBand * 1000000)) {
    debugPort.println("LoRa init done!!");
    LoRa.setSpreadingFactor(spreadingFactor * 1000);
    LoRa.setSignalBandwidth(bandwidth);
    debugPort.println("LoRa init successful");
    debugPort.println("LoRa's looking for packet");
    atmegaResponse = "\r\nOK\r\n";
  }
  else {
    atmegaResponse = "\r\nERROR\r\n";
  }
}

void r_LORACFG()
{
  atmegaResponse = "\r\n+LORACFG:" + String(frequencyBand) + "," +
                   String(bandwidth) + "," + String(spreadingFactor) + "\r\nOK\r\n";
}

void LORASEND()
{
  char loraTxBuffer[5] = {0};
  int numArgs = sscanf(stm32Command.c_str(), "AT+LORASEND=%c%c%c%c%c",
                       &loraTxBuffer[0], &loraTxBuffer[1], &loraTxBuffer[2],
                       &loraTxBuffer[3], &loraTxBuffer[4]);
  LoRa.beginPacket();
  LoRa.write(loraTxBuffer, 5);
  LoRa.endPacket(true);

  atmegaResponse = "\r\nOK\r\n";

  debugPort.print("Sending packet: ");
  debugPort.println(loraTxBuffer);
}

/********************** LORA ********************/
void LoRaProcess()
{
  int packetSize = LoRa.parsePacket();

  if (packetSize)
  {
    /* Variables definition */
    char loraRxBuffer[4];

    debugPort.print("Receive packet: ");

    for (int i = 0; i < 4; i++)
    {
      loraRxBuffer[i] = (char) LoRa.read();
    }
    while (LoRa.available())
    {
      (char) LoRa.read();
    }

    int Rssi = LoRa.packetRssi();
    String atmegaURC = "+LRURC:" + String(loraRxBuffer[0]) + String(loraRxBuffer[1])
                       + String(loraRxBuffer[2]) + String(loraRxBuffer[3]) + String(Rssi) + "\r\n";

    Serial.print(atmegaURC); //Gửi gói tin nhận được sang STM32

    debugPort.print(atmegaURC); //in ra màn hình log
  }
}
