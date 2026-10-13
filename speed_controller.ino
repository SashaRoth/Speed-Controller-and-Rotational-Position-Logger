/*
Created by Sasha Roth at the University of Pittsburgh, December 23rd 2025
Please credit if you use my code :) that would be pretty cool
Email me at aroth050@gmail.com with any questions

This code runs a motor speed controller which I developed for the University of Pittsburgh's Materials Science department
to run a large rotary magnet used for testing magnetic memory shape alloys. Its functions include: opening and scrolling through a menu,
setting the rotation limit and initial direction of the motor, executing a run, and logging data from each run.

Components used:
(1) Arduino Nano Every
(1) L298N Motor Driver
(1) HW-125 Micro SD Card Reader Module
(1) 12V DC motor: https://www.amazon.com/dp/B0FFMPDN5K?ref=ppx_yo2ov_dt_b_fed_asin_title
(1) LM2596 Buck Converter
(1) 12V Barrel Jack to Screw Terminal Adapter
(1) 20x4 I2C LCD Screen Module
(1) Large Potentiometer
(4) Two-Terminal Pushbuttons
(1) SPST Rocker Switch
Plus lots of protoboard
*/

/*
Layout of /SYSDATA.txt
Line 0: Number of current entries
Line 1: Percent storage remaining
*/

//libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SdFat.h>
#include <Encoder.h>
//#include <avr/wdt.h>
//#include <avr/io.h>

//Global variables
bool direction = true;
bool hasRotLimit = false;

int rotLimit = 100;
int prevRotLimit = 100;
byte speed = 0;
int potInput = 0;
byte arrowPos = 1;
byte select = 1;
byte numOptions = 0;

//Pin name declarations
const byte speedControl = 6;  //Motors
const byte dir1 = 7;
const byte dir2 = 8;
const byte scrollUp = A2; //Buttons
const byte scrollDown = A1;
const byte enter = 4;
const byte back = 5;
const byte CS = 10;

//Create an LCD object (used an I2C detection script to find address)
LiquidCrystal_I2C lcd(0x27, 20, 4);

//Create SD card objects
#define SD_FAT_TYPE 3
SdFat32 sd;

//Create encoder object 
Encoder Enc(2, 3);
const uint16_t CPR = 1920;

/*
//Prints the cause of board reset (for debugging)
void printResetCause() {
  uint8_t cause = MCUSR;
  MCUSR = 0; // clear flags so future resets are fresh

  Serial.print(F("Reset cause: "));
  if (cause & (1<<PORF)) Serial.print(F("[Power-on] "));
  if (cause & (1<<EXTRF)) Serial.print(F("[External reset pin] "));
  if (cause & (1<<BORF)) Serial.print(F("[Brown-out] "));
  if (cause & (1<<WDRF)) Serial.print(F("[Watchdog] "));
  Serial.println();
}
*/

void setup() {
  //Serial monitor + reset error check
  Serial.begin(9600);
  delay(200);
  //printResetCause();

  //Pin I/O initializations
  pinMode(speedControl, OUTPUT);  //Motors
  pinMode(dir1, OUTPUT);
  pinMode(dir2, OUTPUT);
  pinMode(A0, INPUT);               //Potentiometer
  pinMode(scrollUp, INPUT_PULLUP);  //Buttons
  pinMode(scrollDown, INPUT_PULLUP);
  pinMode(enter, INPUT_PULLUP);
  pinMode(back, INPUT_PULLUP);

  //Pin status initializations
  analogWrite(speedControl, LOW);
  digitalWrite(dir1, LOW);
  digitalWrite(dir2, LOW);

  //Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  //Initialize SD pins and start SPI communication
  pinMode(CS, OUTPUT);  // keep SPI in master mode on Nano/UNO
  digitalWrite(CS, HIGH);
  SPI.begin();

  //Initialize SD card
  Serial.println(F("Initializing SD card..."));
  delay(50);
  bool ok = sd.begin(CS, SD_SCK_MHZ(4));
  if (!ok) {
    Serial.println(F("SD init FAILED. Check CS pin, wiring, power, card format."));
    SDError(0);
  }
  Serial.println(F("SD init OK."));

  //Error check system data file, create if not already existing
  while (!checkSystemData()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Checking system data");
  }

  //Opening LCD sequence to play once
  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print("Sasha's Creation");
  delay(2000);
  lcd.clear();
}

/////////////////////////////////////////////////////////////////////////INITIAL MAIN MENU///////////////////////////////////

void loop() {
  //Create variables for button statuses
  int menu = 0;
  bool pressUp = Button(scrollUp);
  bool pressDown = Button(scrollDown);
  bool pressEnter = Button(enter);
  bool pressBack = Button(back);

  //Initial menu
  lcd.setCursor(0, 0);
  lcd.print("MAIN MENU");
  lcd.setCursor(1, 1);
  lcd.print(" Configure Run    ");
  lcd.setCursor(1, 2);
  lcd.print(" Data Logs         ");
  lcd.setCursor(2, 3);
  lcd.print("            ");
  lcd.setCursor(0, arrowPos);
  lcd.print(">");
  numOptions = 2;

  //detect menu scroll
  if (pressUp) {
    scrollUpFunc(select, arrowPos, numOptions);
  } else if (pressDown) {
    scrollDownFunc(select, arrowPos, numOptions);
  } else {
  }

  //detect invalid menu position
  if (arrowPos > numOptions) {
    lcd.setCursor(0, arrowPos);
    lcd.print(" ");
    arrowPos = 1;
    select = 1;
  }

  //detect menu select
  if (pressEnter) {
    menu = select;
    delay(100);
  } else {
  }

  //Make choices based on input
  switch (menu) {
    case 0:
      break;

      ////////////////////////////////////////////////////////////////////CONFIGURE NEW RUN SUB-MENU///////////////////////////////////////

    case 1:
      arrowPos = 1;
      select = 1;
      direction = true;
      numOptions = 3;

      lcd.clear();
      lcd.setCursor(13, 2);
      lcd.print("CW ");
      lcd.setCursor(0, 2);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(" ");
      delay(200);

      while (menu == 1) {
        //initialize submenu values
        byte menu_sub1 = 0;
        numOptions = 3;

        //Create variables for "Configure Run" button statuses
        bool pressUp_sub1 = Button(scrollUp);
        bool pressDown_sub1 = Button(scrollDown);
        bool pressEnter_sub1 = Button(enter);
        bool pressBack_sub1 = Button(back);

        //Initial menu
        lcd.setCursor(0, 0);
        lcd.print("NEW RUN             ");
        lcd.setCursor(2, 1);
        lcd.print("Rotations: ");
        if (!hasRotLimit) {
          lcd.print("UL");
        } else {
          lcd.print(rotLimit);  //Rotation limit preview
        }

        lcd.setCursor(2, 2);
        lcd.print("Direction: ");
        if (direction) {
          lcd.print("CW   ");
        } else {
          lcd.print("CCW  ");  //Rotation direction preview
        }

        lcd.setCursor(2, 3);
        lcd.print("START");
        lcd.setCursor(0, arrowPos);
        lcd.print(">");

        //detect menu scroll
        if (pressUp_sub1) {
          scrollUpFunc(select, arrowPos, numOptions);
          //Serial.println("scrolling!");
          delay(100);
        } else if (pressDown_sub1) {
          scrollDownFunc(select, arrowPos, numOptions);
          //Serial.println(select);
          delay(100);
        } else {
        }

        //detect menu select
        if (pressEnter_sub1) {
          menu_sub1 = select;
          delay(200);
        } else {
        }

        //Make choices based on input
        switch (menu_sub1) {
          delay(100);
          case 0:
            break;
            //////////////////////////////////////////CHANGE # OF TURNS//////////
          case 1:
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("ROTATIONS");
            if (hasRotLimit) {
              lcd.setCursor(8, 2);
              lcd.print(rotLimit);
            } else {
              lcd.setCursor(6, 2);
              lcd.print("UNLIMITED");
            }

            while (menu_sub1 == 1) {
              arrowPos = 1;
              select = 1;
              numOptions = 2;

              //Create variables for Rotation Menu button statuses
              bool pressUp_rotMenu = Button(scrollUp);
              bool pressDown_rotMenu = Button(scrollDown);
              bool pressEnter_rotMenu = Button(enter);
              bool pressBack_rotMenu = Button(back);

              //detect unlimited/limited toggle
              //if there is a limit, change it to unlimited turns
              if (pressEnter_rotMenu) {
                hasRotLimit = !hasRotLimit;
                if (!hasRotLimit) {
                  prevRotLimit = rotLimit;
                  lcd.setCursor(6, 2);
                  lcd.print("         ");
                  lcd.setCursor(6, 2);
                  lcd.print("UNLIMITED");
                  delay(100);
                }
                //if there is no limit, change it to the last known rotation limit
                else {
                  rotLimit = prevRotLimit;
                  lcd.setCursor(6, 2);
                  lcd.print("         ");
                  lcd.setCursor(8, 2);
                  lcd.print(rotLimit);
                  delay(100);
                }
                delay(100);
              }
              //if there is a rotation limit, increase or decrease with appropriate input
              if (hasRotLimit) {
                if (pressUp_rotMenu) {
                  rotLimit += 10;
                  lcd.setCursor(6, 2);
                  lcd.print("         ");
                  lcd.setCursor(8, 2);
                  lcd.print(rotLimit);
                  delay(200);
                } else if (pressDown_rotMenu) {
                  rotLimit -= 10;
                  lcd.setCursor(6, 2);
                  lcd.print("         ");
                  lcd.setCursor(8, 2);
                  lcd.print(rotLimit);
                  delay(200);
                }
              } else {
              }
              //back button exit sequence
              if (pressBack_rotMenu) {
                menu_sub1 = 0;
                arrowPos = 1;
                select = 1;
                lcd.setCursor(0, 2);
                lcd.print(" ");
                lcd.setCursor(0, 3);
                lcd.print(" ");
                delay(200);
              }
              //delay(100);
            }
            break;
            //////////////////////////////////////////CHANGE DIRECTION///////////
          case 2:
            changeDirection();
            break;
            //////////////////////////////////////////START RUN/////////////////
          case 3:
            executeRun();
            break;

          default:
            break;
        }
        //back button exit sequence
        if (pressBack_sub1) {
          menu = 0;
          arrowPos = 1;
          select = 1;
          lcd.setCursor(0, 2);
          lcd.print(" ");
          lcd.setCursor(0, 3);
          lcd.print(" ");
          delay(200);
        }
      }
      break;

      ////////////////////////////////////////////////////////////////////DATA LOGGING SUB-MENU///////////////////////////////////////

    case 2:
      lcd.clear();
      while(menu == 2){
        bool pressBack_sub2 = Button(back);

        //initialize data logs lcd screen
        lcd.setCursor(0, 0);
          lcd.print("DATA LOGS");
        lcd.setCursor(0, 1); 
          float cardmem = readLine(2) * 0.1;
          lcd.print(cardmem); lcd.print("% memory left");
        lcd.setCursor(0, 2);
          lcd.print("Runs logged: "); lcd.print(readLine(1));

        if (pressBack_sub2) {
          delay(200);
          menu = 0;
          arrowPos = 1;
          select = 1;
          lcd.setCursor(0, 2);
          lcd.print(" ");
          lcd.setCursor(0, 3);
          lcd.print(" ");
        }
      }

      break;

    default:
      break;
  }
  delay(100);
}


//End MAIN
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---Executive functions---//

void executeRun() {
  //run sequence for when there is a limit
    int limit = (hasRotLimit ? rotLimit : 9999);

    //set initial cycle count conditions
    uint8_t RPM = 0;
    uint16_t cycle = 0, temp = 0;
    uint32_t initMillis = millis(), duration = 0, duration_mil = 0; //timing variables
    bool confirm_menu = false;
    float angle = 0.00, prevAngle = 0.00; //angular position of the motor

    //log initial conditions of run
    long entry = initWrite();
    if (entry == -1) {
      Serial.println(F("Unable to open initial run file"));
      lcd.clear();
      delay(50);
      return;
    }

    //create custom name for runtime data
    char filename[13];  //8.3 format + null terminator
    snprintf(filename, sizeof(filename), "RUN%d.TXT", entry);
    Serial.print(F("File to be opened: "));
    Serial.println(filename);

    //create and open runtime file, print header
    File32 runtimeFile;

    if (!runtimeFile.open(filename, O_WRONLY | O_CREAT)) {
      lcd.print("Unable to open runtime file");
      lcd.clear();
      return;
    }
    Serial.println(F("Runtime file open success"));
    runtimeFile.println("Time (Milliseconds),Speed (RPM),Angle (Degree),Cycle (Num)");

    //initialize LCD screen
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RUN IN PROGRESS...");
    lcd.setCursor(0, 1);
    lcd.print("Cycle: ");
    lcd.setCursor(12, 1);
    lcd.print("RPM: ");
    lcd.setCursor(0, 2);
    lcd.print("Time elapsed: ");
    lcd.setCursor(0, 3);
    lcd.print("Press BACK to abort");

    //set direction of motor
    if (direction) {
      digitalWrite(dir1, HIGH);
      digitalWrite(dir2, LOW);
    } else {
      digitalWrite(dir1, LOW);
      digitalWrite(dir2, HIGH);
    }

    //zero motor at start position
    Enc.write(0);

    //cycling
    while (cycle < limit) {  //TODO: handle errors states such as SD write hangs, motor stalls, or other... add at least one escape path
      //Create and monitor variables for Run Execution button/potentiometer statuses
      bool pressEnter_run = Button(enter);
      bool pressBack_run = Button(back);

      //control motor speed
      speed = map(analogRead(A0), 1023, 0, 150, 255);
      analogWrite(speedControl, speed);
      delay(10);

      //update time elapsed count
      duration_mil = (millis() - initMillis);
      duration = duration_mil / 1000;

      angle = getPos(cycle);
      Serial.print(F("Angle: ")); Serial.println(angle);
      Serial.print(F("Cycle: ")); Serial.println(cycle);

      RPM = getRPM(angle);
      Serial.print(F("RPM: ")); Serial.println(RPM);

      //write current TIME (mil), SPEED, POSITION, and CYCLE to runtime file
      runtimeFile.print(duration_mil);
      runtimeFile.print(",");
      runtimeFile.print(RPM);
      runtimeFile.print(",");
      runtimeFile.print(angle);
      runtimeFile.print(",");
      runtimeFile.println(cycle);
      delay(10);

      //escape loop sequence -- user presses BACK to abort and ENTER to confirm abort
      if (pressBack_run) {
        confirm_menu = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Press:"));
        lcd.setCursor(0, 1);
        lcd.print(F("ENTER to ABORT"));
        lcd.setCursor(0, 2);
        lcd.print(F("BACK to CONTINUE RUN"));
      }
      if (!confirm_menu){
        //debounce the speed displayed to the LCD screen
        //byte speed_output = speedDebounce(speed);
        //print changes to LCD screen
        lcd.setCursor(7, 1);
        lcd.print(cycle);  //TODO: make sure this doesn't print stale digits
        lcd.setCursor(17, 1);
        lcd.print(RPM);
        if(RPM < 100){
          lcd.print(" ");
        }
        lcd.setCursor(15, 2);
        lcd.print(duration);
      }
      else if (confirm_menu) {
          bool pressEnter_confirm = Button(enter);
          bool pressBack_confirm = Button(back);
          if (pressEnter_confirm) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Aborting run..."));
            analogWrite(speedControl, 0);
            lcd.clear();
            return;
          } 
          else if (pressBack_confirm) {
            confirm_menu = false;
            lcd.clear();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("RUN IN PROGRESS...");
            lcd.setCursor(0, 1);
            lcd.print("Cycle: ");
            lcd.setCursor(10, 1);
            lcd.print("Speed: ");
            lcd.setCursor(0, 2);
            lcd.print("Time elapsed: ");
            lcd.setCursor(0, 3);
            lcd.print("Press BACK to abort");
            delay(50);
          }
        }
      }
    

    //update remaining memory in SD card in SYSDATA.TXT after run is complete
    Serial.print(F("Remaining percent after run: "));
    float freeP = SDfreePercent_x10() * 0.1f;
    Serial.println(freeP);
    overwriteLine(2, SDfreePercent_x10());

    //stop the motor and return to the "Configure New Run" screen
    runtimeFile.close();
    analogWrite(speedControl, 0);
    lcd.clear();
  return;
}


//---Motor control functions---//

//function to smoothly start the motor (used in executeRun sequence)
/*
void startMotor() {
  //set direction
  if (direction) {
    digitalWrite(dir1, HIGH);
    digitalWrite(dir2, LOW);
  } else {
    digitalWrite(dir1, LOW);
    digitalWrite(dir2, HIGH);
  }
  //start motor from 0 (startup takes maximum 2.55 seconds)
  speed = map(analogRead(A0), 1023, 0, 50, 255);
  for (byte i = 0; i <= speed; i += 5) {
    analogWrite(speedControl, i);
    delay(50);
  }
  analogWrite(speedControl, speed);
}
*/

//function to smoothly stop the motor (used in executeRun sequence)
/*
void stopMotor() {
  //stop motor from current speed (takes maximum 2.55 seconds)
  speed = map(analogRead(A0), 1023, 0, 50, 255);
  for (int i = speed; i >= 0; i -= 5) {
    analogWrite(speedControl, i);
    delay(50);
  }
  analogWrite(speedControl, 0);

  //reset direction outputs to low
  digitalWrite(dir1, LOW);
  digitalWrite(dir2, LOW);
  return;
}
*/

//function to change the pre-set direction of the motor (used in submenu 1)
void changeDirection() {
  direction = !direction;
  lcd.setCursor(13, 2);
  if (direction) {
    lcd.print("CW ");
  } else {
    lcd.print("CCW");
  }
}

//function to stabilize displayed "speed" value (I ripped this straight from the button debounce function lol)
int speedDebounce(byte speed) {
  uint32_t currentSpeedMillis = millis();
  static uint32_t lastSpeedMillis;
  const uint32_t SPEEDBOUNCETIMEOUT = 300;
  static byte lastSpeed;

  if (lastSpeed != speed) {
    if (currentSpeedMillis - lastSpeedMillis >= SPEEDBOUNCETIMEOUT) {
      lastSpeed = speed;
      return speed;
    }
    return lastSpeed;
  } else {
    lastSpeedMillis = currentSpeedMillis;
    return lastSpeed;
  }
}

//---Encoder functions---//

//reads encoder and returns current motor position in degrees
float getPos(uint16_t &cycle){
  float position;
  long raw;

  raw = Enc.read();

  float motorAngle = abs((raw/(float)CPR)*360);
  float wheelAngle = motorAngle/4;

  cycle = (uint16_t)(wheelAngle/360.0);
  position = wheelAngle - (cycle*360);

  return position;
}

uint8_t getRPM(float angle){
  static float prevAngle;
  static float prevMillis;
  float deltaT;
  float deltaAngle;
  uint8_t RPM;
  static uint8_t prevRPM;

  deltaT = millis() - prevMillis;

  if(angle < prevAngle){
    deltaAngle = (360.0 - prevAngle) + angle;
  }
  else{
    deltaAngle = angle - prevAngle;
  }

  RPM = (uint8_t)((deltaAngle/360.0)/(deltaT/(1000.0*60.0)));

  prevMillis = millis();
  prevAngle = angle;

  if(abs(RPM - prevRPM) > 2){
    prevRPM = RPM;
  }
  else{
    RPM = prevRPM;
  }
  
  return RPM;
}

//---Data logging functions---//

// checks if system data file exists and is openable
bool checkSystemData() {
  Serial.println(F("Now in checkSystemData sequence"));
  delay(50);

  //Serial.println(F("Before sd.exists(\"/SYSDATA.TXT\")"));
  bool exists = sd.exists("/SYSDATA.TXT");
  //Serial.print("exists = ");
  //Serial.println(exists ? "true" : "false");

  if (exists) {
    Serial.println(F("Before systemFile.open(\"/SYSDATA.TXT\") for read"));
    File32 systemFile;
    systemFile.open("/SYSDATA.TXT", O_RDONLY);
    if (systemFile) {
      Serial.println(F("Successfully opened existing /SYSDATA.TXT file. Now closing."));
      systemFile.close();
      return true;
    } else {
      Serial.println(F("Exists but open for read FAILED. Listing root for clues:"));
      listRoot();
      return false;
    }
  } else {
    Serial.println(F("File /SYSDATA.TXT does not exist. Now running constructSystemData sequence."));
    return constructSystemData();
  }
}

//constructs a default system data file
bool constructSystemData() {
  Serial.println(F("Now in constructSystemData sequence"));
  delay(50);

  // If a stale file/alias exists, remove it first
  //Serial.println(F("Before sd.remove(\"/SYSDATA.TXT\")"));
  if (sd.exists("/SYSDATA.TXT")) {
    if (sd.remove("/SYSDATA.TXT")) {
      Serial.println(F("Removed old /SYSDATA.TXT"));
    } else {
      Serial.println(F("Remove FAILED (might be a DIR or write-protected). Listing root:"));
      listRoot();
      return false;
    }
  } else {
    Serial.println(F("No existing /SYSDATA.TXT to remove."));
  }

  //Serial.println(F("Before systemFile.open(\"/SYSDATA.TXT\", FILE_WRITE)"));
  File32 newSystemFile;
  newSystemFile.open("/SYSDATA.TXT", O_WRONLY | O_CREAT);
  if (!newSystemFile) {
    Serial.println(F("Open for write FAILED. Check LOCK tab, FAT32, CS, power. Listing root:"));
    listRoot();
    return false;
  }

  Serial.println(F("Writing new system data contents..."));
  bool ok = true;
  ok &= newSystemFile.println("0");
  Serial.println(F("Runs logged: 0"));

  uint32_t freeSpaceP_x10 = SDfreePercent_x10();
  float freeSpaceP = (float)(freeSpaceP_x10 * 0.1f);
  ok &= newSystemFile.println(freeSpaceP);
  //Serial.print(F("Memory remaining: ")); Serial.print(freeSpaceP); Serial.println(F("%"));
  newSystemFile.println("\n");

  newSystemFile.close();

  Serial.println(ok ? F("Write OK, file closed.") : F("Write FAILED, file closed."));

  //Serial.println(F("Before SD.exists(\"/SYSDATA.TXT\") post-write"));
  bool nowExists = sd.exists("/SYSDATA.TXT");
  Serial.print(F("nowExists = "));
  Serial.println(nowExists ? F("true") : F("false"));
  return ok && nowExists;
}

//writes the rotation limit, direction, and duration of a run to its own file
long initWrite() {
  Serial.println(F("initWrite: begin"));

  //read previous entry from SYSDATA.TXT to calculate the current entry #
  long currentEntry = readLine(1) + 1;
  if (currentEntry == 0) {  //if readLine returned -1, indicating fail, currentEntry would equal 0
    Serial.println(F("incrementEntry: readLine fail"));
    return -1;
  } else if (currentEntry < 0) {  //if currentEntry overflowed, it would return a large negative number (unlikely given 'long' datatype)
    Serial.println(F("incrementEntry: currentEntry overflow"));
    return -1;
  }

  //update the first line of SYSDATA.TXT to reflect new entry
  Serial.println(F("Before overWrite line"));
  if (!overwriteLine(1, currentEntry)) {
    Serial.println(F("incrementEntry: overwrite failed"));
    return -1;
  }

  //print current entry (just error-checking)
  Serial.print(F("incrementEntry: Current run # = "));
  Serial.println(currentEntry);

  //open or create initial condition datafile (holds run #, rotation limit, and direction)
  File32 initdata;

  if (!initdata.open("/INITDATA.TXT", O_WRONLY | O_CREAT | O_APPEND)) {
    Serial.println(F("initWrite: open INITDATA failed"));
    return -1;
  }

  //if the INITDATA.TXT file was just created, it will have no data, so create the header
  if (initdata.fileSize() == 0) {
    initdata.println(F("Run,Rotations,Direction"));
  }

  //print run #
  initdata.print(currentEntry);
  initdata.print(',');

  //print rotation limit
  if (hasRotLimit) {
    initdata.print(rotLimit);
  } else {
    initdata.print(F("Unlimited"));
  }

  //print direction
  initdata.print(',');
  initdata.println(direction ? F("Clockwise") : F("Counter-clockwise"));

  initdata.close();
  Serial.println(F("initWrite: success"));
  return currentEntry;
}

//writes the speed and position of the motor per unit time to its own file
bool runtimeWrite(File32 &runtimeFile, byte speed, uint32_t duration_mil, int pos) {
  ;
}

//reads and returns a specific line of sysdata.txt
long readLine(byte targetLine) {
  if(targetLine != 1 && targetLine != 2){
    return -1;
  }

  File32 to_read;
  if(!to_read.open("/SYSDATA.TXT", O_RDONLY)){
    return -1;
  }

  long value = 0;
  byte currentLine = 1;
  
  while(true){
    int b = to_read.read();
    if(b < 0){
      break;
    }

    char c = (char)b;

    if (c >= '0' && c <= '9') {
      if (value <= LONG_MAX / 10)
        value = value * 10 + (c - '0');
    }
    else if (c == '\n' || c == '\r') {
      if(to_read.read() != '\n'){
        to_read.seekCur(-1);
      }
      if (currentLine == targetLine) {
        to_read.close();
        return value;
      }
      else {
        value = 0;
        currentLine = 2;  // only two lines, no counting needed
      }
    }
  }
  to_read.close();
  return (currentLine == targetLine) ? value : -1;
}

//overwrites a specific line of sysdata.txt
bool overwriteLine(byte targetLine, long newValue) {
  if(targetLine != 1 && targetLine != 2){
    return false;
  }

  if(!sd.exists("/SYSDATA.TXT")){
    Serial.println(F("overwriteLine: No system data found."));
    return false;
  }

  File32 sysfile;

  if (!sysfile.open("/SYSDATA.TXT", O_RDONLY)) {
    Serial.println(F("overwriteLine: open sysfile for read failed"));
    return false;
  }

  long line1 = readLine(1);
  long line2 = readLine(2);
  bool replaced = false;

  sysfile.close();
  if (!sysfile.open("/SYSDATA.TXT", O_WRONLY | O_TRUNC)) {
    Serial.println(F("overwriteLine: open sysfile for truncate failed"));
    return false;
  }

  if(targetLine == 1){
    sysfile.println(newValue);
    sysfile.println(line2);
    replaced = true;
  }
  else if(targetLine == 2){
    sysfile.println(line1);
    sysfile.println(newValue);
    replaced = true;
  }
  
  if (!replaced) {
    Serial.println(F("overwriteLine: target line missing"));
    sysfile.close();
    return false;
  }

  sysfile.close();
  return true;
}

//lists the roots of the SD card, for debugging
void listRoot() {
  Serial.println(F("Listing root:"));
  File32 SDroot;
  SDroot.open("/");
  if (!SDroot) {
    Serial.println(F("Root open FAILED"));
    return;
  }
  SDroot.rewind();
  while (true) {
    File32 entry;
    entry.openNext(&SDroot, O_RDONLY);
    if (!entry) break;
    Serial.print(F("  "));
    char fname[64];
    entry.getName(fname, sizeof(fname));
    Serial.print(fname);
    if (entry.isDir()) Serial.print(F(" <DIR>"));
    Serial.println();
    entry.close();
  }
  SDroot.close();
}

/*Unused functions
//calculates the remaining memory of the SD card (copied + modded from SdFat example: examplesV1/VolumeFreeSpace)
uint32_t SDfreeSpace() {
  sd.vol()->cacheClear();

  uint32_t volFree = sd.vol()->freeClusterCount();

  uint32_t freeSpace = (volFree*sd.vol()->sectorsPerCluster())/2;

  Serial.print(F("freeSpace: ")); Serial.print(freeSpace); Serial.println(F(" KiB (KiB = 1024 bytes)"));
  return freeSpace;
}

//calculates the used memory of the SD card
uint32_t SDusedSpace(){
  sd.vol()->cacheClear();

  uint32_t volTotal = sd.vol()->clusterCount();
  uint32_t volFree = sd.vol()->freeClusterCount();
  uint32_t volUsed = volTotal - volFree;

  uint32_t usedSpace = (volUsed*sd.vol()->sectorsPerCluster())/2;

  Serial.print(F("usedSpace: ")); Serial.print(usedSpace); Serial.println(F(" KiB (KiB = 1024 bytes)"));
  return usedSpace;
}*/

//calculates percentage of SD memory remaining
uint16_t SDfreePercent_x10() {
  sd.vol()->cacheClear();

  uint32_t volTotal = sd.vol()->clusterCount();
  if (volTotal == 0) {
    return 0;
  }
  uint32_t volFree = sd.vol()->freeClusterCount();
  uint16_t freePercent_x10 = (volFree * 1000UL) / volTotal;

  Serial.print(F("Percent memory remaining: "));
  Serial.print(freePercent_x10 / 10);
  Serial.print(F("."));
  Serial.print(freePercent_x10 % 10);
  Serial.println(F("%"));
  return freePercent_x10;
}

//displays SD card error messages on the LCD screen
void SDError(int scenario) {
  switch (scenario) {
    case 0:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("SD init fail"));
      lcd.setCursor(0, 1);
      lcd.print(F("Check card, format"));
      lcd.setCursor(0, 2);
      lcd.print(F("asr195@pitt.edu"));
      while (!sd.begin(CS, SD_SCK_MHZ(4))) {
        Serial.println(F("Waiting for initialization success..."));
        delay(100);
      }
      lcd.clear();
      break;
    case 1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Unable to create"));
      lcd.setCursor(0, 1);
      lcd.print(F("new system file"));
      lcd.setCursor(0, 2);
      lcd.print(F("asr195@pitt.edu"));
      lcd.clear();
      delay(50);
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Unable to access"));
      lcd.setCursor(0, 1);
      lcd.print(F("root directory"));
      lcd.setCursor(0, 2);
      lcd.print(F("asr195@pitt.edu"));
      lcd.clear();
      delay(50);
      break;
    case 3:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Unable to create"));
      lcd.setCursor(0, 1);
      lcd.print(F("new run file"));
      lcd.setCursor(0, 2);
      lcd.print(F("asr195@pitt.edu"));
      lcd.clear();
      delay(50);
      break;
    case 4:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("File access fail"));
      lcd.setCursor(0, 1);
      lcd.print(F("Check card, format"));
      lcd.setCursor(0, 2);
      lcd.print(F("asr195@pitt.edu"));
      lcd.clear();
      delay(50);
      break;
    case 5:

      break;
  }
}

//---Menu and button functions---//

//function to move menu arrow up
void scrollUpFunc(byte &select, byte &arrowPos, byte numOptions) {
  select++;
  if (select > numOptions) {
    select = 1;
  }
  int prevPos = arrowPos;
  arrowPos++;
  if (arrowPos > numOptions) {
    arrowPos = 1;
  }

  lcd.setCursor(0, prevPos);
  lcd.print(" ");
  lcd.setCursor(0, arrowPos);
  lcd.print(">");
}

//function to move menu arrow down
void scrollDownFunc(byte &select, byte &arrowPos, byte numOptions) {
  select--;
  if (select < 1) {
    select = numOptions;
  }
  int prevPos = arrowPos;
  arrowPos--;
  if (arrowPos < 1) {
    arrowPos = numOptions;
  }

  lcd.setCursor(0, prevPos);
  lcd.print(" ");
  lcd.setCursor(0, arrowPos);
  lcd.print(">");
}

//function to debounce button input (not mine, I ripped this straight from ChatGPT)
bool Button(byte pin) {
  static uint32_t lastTime[10];  // adjust size to number of buttons
  static bool lastState[10];
  static bool latched[10];

  byte idx = pin;  // ok if pins are small numbers

  bool reading = digitalRead(pin);

  if (reading != lastState[idx]) {
    lastTime[idx] = millis();
    lastState[idx] = reading;
  }

  if ((millis() - lastTime[idx]) > 5) {  // 30 ms debounce
    if (reading == LOW && !latched[idx]) {
      latched[idx] = true;
      return true;  // one clean press
    }
    if (reading == HIGH) {
      latched[idx] = false;
    }
  }

  return false;
}