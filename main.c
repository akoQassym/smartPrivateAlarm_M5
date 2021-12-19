// including the appropriate libraries
#include <M5Core2.h>
#include "time.h"
#include "math.h"
#include "symbols.h"
RTC_TimeTypeDef TimeStruct;

int count = 0;
int intervalMinSize = 15; // 15 minutes interval
int sleepPhaseAmount = 0;
int movementArr[30];
int *phasesArr;

int tempHour = 0;
int tempMinutes = 0;
int alarmH = 0;
int alarmM = 0;
int bottomH = 0;
int bottomM = 0;
int upH = 0;
int upM = 0;
int trackModeOnFlag = 0;

// START SOUND CONFIG
#include <driver/i2s.h>  
extern const unsigned char previewR[120264];  // Referring to external data (Dingdong audio files are stored inside).
#define CONFIG_I2S_BCK_PIN 12
#define CONFIG_I2S_LRCK_PIN 0
#define CONFIG_I2S_DATA_PIN 2
#define CONFIG_I2S_DATA_IN_PIN 34
#define Speak_I2S_NUMBER I2S_NUM_0  
#define MODE_MIC 0
#define MODE_SPK 1
#define DATA_SIZE 1024


bool InitI2SSpeakOrMic(int mode){  //Init I2S.  
    esp_err_t err = ESP_OK;
    i2s_driver_uninstall(Speak_I2S_NUMBER); // Uninstall the I2S driver.
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),  // Set the I2S operating mode.
        .sample_rate = 44100, // Set the I2S sampling rate.  
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // Fixed 12-bit stereo MSB.  
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, // Set the channel format.  
        .communication_format = I2S_COMM_FORMAT_I2S,  // Set the format of the communication.  
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // Set the interrupt flag.  
        .dma_buf_count = 2, //DMA buffer count.  
        .dma_buf_len = 128, //DMA buffer length. 
    };
    if (mode == MODE_MIC){
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    }else{
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;  //I2S clock setup.  
        i2s_config.tx_desc_auto_clear = true; // Enables auto-cleanup descriptors for understreams.  
    }
    // Install and drive I2S.  
    err += i2s_driver_install(Speak_I2S_NUMBER, &i2s_config, 0, NULL);
    i2s_pin_config_t tx_pin_config;
    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;  // Link the BCK to the CONFIG_I2S_BCK_PIN pin.
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;   
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
    err += i2s_set_pin(Speak_I2S_NUMBER, &tx_pin_config); // Set the I2S pin number. 
    err += i2s_set_clk(Speak_I2S_NUMBER, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO); // Set the clock and bitwidth used by I2S Rx and Tx.
    return true;
}

uint8_t data_0[DATA_SIZE * 100];
// END SOUND CONFIG

void setAlarmInterface() {
  M5.Lcd.setFreeFont(&unicode_24px);  //Set the GFX font to use.
  M5.update();
  if (M5.BtnA.wasReleased()) {    //If the key is pressed. 
    alarmH++;
    if (alarmH>=24) {
      alarmH = 0;
    }
    M5.update();
    M5.Lcd.clear();
  }
  if (M5.BtnB.wasReleased()) {    //If the key is pressed. 
    alarmM++;
    if (alarmM>=60) {
      alarmM = 0;
    }
    M5.update();
    M5.Lcd.clear();
  }
  if (M5.BtnC.wasReleased()) {    //If the key is pressed. 
    M5.update();
    M5.Lcd.clear();
    trackModeOnFlag = 1;
    bottomH = alarmH;
    bottomM = alarmM-1;
    upH = alarmH;
    upM = alarmM+1;
    M5.Lcd.setCursor(0, 150);
    M5.Lcd.printf("%d:%d - %d-%d", bottomH, bottomM, upH, upM);
  }
  
  if (getHour() != tempHour || getMinutes() != tempMinutes) {
    M5.Lcd.clear();  
    tempHour = getHour();
    tempMinutes = getMinutes();
  }
  M5.Lcd.setCursor(80, 35);
  if (getHour() >= 10 && getMinutes() >= 10) {
    M5.Lcd.printf("NOW: %d : %d", getHour(), getMinutes()); 
  } else if (getHour() >= 10 && getMinutes() < 10) {
    M5.Lcd.printf("NOW: %d : 0%d", getHour(), getMinutes()); 
  } else if (getHour() < 10 && getMinutes() >= 10) {
    M5.Lcd.printf("NOW: 0%d : %d", getHour(), getMinutes()); 
  } else if (getHour() < 10 && getMinutes() < 10) {
    M5.Lcd.printf("NOW: 0%d : 0%d", getHour(), getMinutes()); 
  }

  M5.Lcd.drawLine(0, 40, 400, 40, WHITE);

  M5.Lcd.setCursor(85, 80);
  M5.Lcd.printf("SET ALARM:");
  M5.Lcd.setCursor(115, 110);
  if (alarmH >= 10 && alarmM >= 10) {
    M5.Lcd.printf("%d : %d", alarmH, alarmM); 
  } else if (alarmH >= 10 && alarmM < 10) {
    M5.Lcd.printf("%d : 0%d", alarmH, alarmM); 
  } else if (alarmH < 10 && alarmM >= 10) {
    M5.Lcd.printf("0%d : %d", alarmH, alarmM); 
  } else if (alarmH < 10 && alarmM < 10) {
    M5.Lcd.printf("0%d : 0%d", alarmH, alarmM); 
  }

  M5.Lcd.setCursor(20, 220);
  M5.Lcd.printf("hours");
  M5.Lcd.setCursor(120, 220);
  M5.Lcd.printf("minutes");
  M5.Lcd.setCursor(250, 220);
  M5.Lcd.printf("set");

  
}

int getSeconds() { //Output current time seconds
  M5.update();
  M5.Rtc.GetTime(&TimeStruct);
  return TimeStruct.Seconds;
}

int getMinutes() { //Output current minute
  M5.update();
  M5.Rtc.GetTime(&TimeStruct);
  return TimeStruct.Minutes;
}

int getHour() {
  M5.update();
  M5.Rtc.GetTime(&TimeStruct);
  return TimeStruct.Hours;
}

void setThreshold() {
  if (alarmM <= 15) {
    if (alarmH == 0) {
      bottomH = 23;
    } else {
      bottomH = alarmH-1;
    }
  } else {
    bottomH = alarmH;
  }

  if (alarmM < 15) {
    bottomM = 45 + alarmM;
  } else {
    bottomM = alarmM - 15;
  }

  if (alarmM >= 45) {
    if (alarmH == 23) {
      upH = 0;
    } else {
      upH = alarmH+1;
    }
  }

  if (alarmM >= 45) {
    upM = 15 - 60 + alarmM;
  } else {
    upM = alarmM + 15;
  }

  M5.Lcd.setCursor(0, 150);
  M5.Lcd.printf("%d:%d - %d-%d", bottomH, bottomM, upH, upM);
}

void SpeakInit(void){ 
  M5.Axp.SetSpkEnable(true);  
  InitI2SSpeakOrMic(MODE_SPK);
  
}
void DingDong(void){
  size_t bytes_written = 0;
  i2s_write(Speak_I2S_NUMBER, previewR, 120264, &bytes_written, portMAX_DELAY);
}

void wakeUp(int phase, int startSec) {
  while(phase == 2 || phase == 3){
    M5.Lcd.setCursor(125, 200);
    M5.Lcd.printf("Wake up!");

    M5.update();
    if((getSeconds() - startSec) % 2 == 0){
      M5.update();
      // stopping the vibration when button B is pressed
    if(M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200)){
      M5.update();
      M5.Axp.SetLDOEnable(3, false);
      trackModeOnFlag = 0;
      break;
    }
      M5.Axp.SetLDOVoltage(3, 3000);
      M5.Axp.SetLDOEnable(3, true);
    }

    M5.update();
    if((getSeconds() - startSec) % 2 != 0){
      M5.update();    
      // stopping the vibration when button B is pressed
    if(M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200)){
      M5.update();
      M5.Axp.SetLDOEnable(3, false);
      trackModeOnFlag = 0;
      break;
    }
      M5.Axp.SetLDOEnable(3, false);
    }

    M5.update();
    

    M5.update();
    if ((getHour()*60+getMinutes()) > (upH*60+upM)) {
      phase = 0;
    }
  }
  
  while (phase == 0 || phase == 1) {
    M5.Lcd.setCursor(125, 200);
    M5.Lcd.printf("URGENT!");
    M5.update();
    if((getSeconds() - startSec) % 2 == 0){
      M5.update();
           // stopping the vibration when button B is pressed
    if(M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200)){
      M5.update();
      M5.Axp.SetLDOEnable(3, false);
      trackModeOnFlag = 0;
      break;
    }
      M5.Axp.SetLDOVoltage(3, 3000);
      M5.Axp.SetLDOEnable(3, true);
      DingDong();
      if(M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200)){
        M5.Axp.SetLDOEnable(3, false);
         trackModeOnFlag = 0;
        break;
      }
    }
  

    M5.update();
    if((getSeconds() - startSec) % 2 != 0){
      M5.update();    
           // stopping the vibration when button B is pressed
    if(M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200)){
      M5.update();
      M5.Axp.SetLDOEnable(3, false);
      trackModeOnFlag = 0;
      break;
    }
      M5.Axp.SetLDOEnable(3, false);

        DingDong();
         if(M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200)){
          M5.Axp.SetLDOEnable(3, false);
           trackModeOnFlag = 0;
          break;
          }

    }
    
    M5.update();
    // stopping the vibration when button B is pressed
    if(M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200)){
      M5.update();
      M5.Axp.SetLDOEnable(3, false);
      trackModeOnFlag = 0;
      break;
    }
  }
}

int analyze30(int startSec) {
  float accX = 0.0F;
  float accY = 0.0F;
  float accZ = 0.0F;
  int detectedAmount = 0;

  while((getSeconds() - startSec <= 1 && getSeconds() - startSec >= 0) || (60 - startSec - getSeconds() <= 1 && 60 - startSec - getSeconds() >= 0)) {
    M5.update();
    M5.IMU.getAccelData(&accX, &accY, &accZ); //Stores the triaxial accelerometer.
    float magnitude = sqrt(accX * accX + accY * accY + accZ * accZ);
    //accelerometr has some error, that is why we put a range
    if (magnitude > 1.15 || magnitude < 0.85) {
      detectedAmount++;
    }
    delay(10); //delay for 10ms
  }
  if (detectedAmount >= 10) {
    return 1;
  } else {
    return 0;
  }
}

float calculateRelativeFrequency() {
  int moveAmount = 0;
  for (int i = 0; i < intervalMinSize * 2; i++) {
    if (movementArr[i] == 1) {
      moveAmount++;
    }
  }
  float relFreqVal = (float)moveAmount / (float)(intervalMinSize * 2);
  return relFreqVal;
}

int analyzePhase() {
  int phase;
  float relFreq = calculateRelativeFrequency();

  if (sleepPhaseAmount == 1) {
    phasesArr = (int*) malloc(sleepPhaseAmount * sizeof(int));
  } else if (sleepPhaseAmount > 1) {
    phasesArr = (int*) realloc(phasesArr, sleepPhaseAmount * sizeof(int));
  }

  if (relFreq <= 0.07) {
    phase = 0;
  } else if (relFreq <= 0.13) {
    phase = 1;
  } else if (relFreq <= 0.28) {
    phase = 2; // REM
  } else if (relFreq <= 0.59 || relFreq > 0.59) {
    phase = 3;
  }

  phasesArr[sleepPhaseAmount-1] = phase;
  return phase;
}

void setup() {
  // run M5 Stack
  M5.begin(9600);
  Serial.begin(9600);
  M5.IMU.Init();
  SpeakInit();
}

// function executed after the M5 is set up
void loop() {
  if (trackModeOnFlag == 0) {
    setAlarmInterface();
  } else {
    M5.update();
    M5.Lcd.clear();
    count = 0;
    while (count < (intervalMinSize * 2)) {
      int sec = getSeconds();
      int isMovement = analyze30(sec);
      movementArr[count] = isMovement;
      count++;
    }
    sleepPhaseAmount++;
    int phase = analyzePhase();
    M5.Lcd.setCursor(125, 100);
    M5.Lcd.printf("phase: %d!", phase);
    int nowH = getHour();
    int nowM = getMinutes();
    if ((nowH*60+nowM) >= (bottomH*60+bottomM) && (nowH*60+nowM) <= (upH*60+upM)) {
      wakeUp(phase, getSeconds());
    } else if ((nowH*60+nowM) > (upH*60+upM)) {
      wakeUp(phase, getSeconds());
    }
  }  
}