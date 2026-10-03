#include <Arduino.h>
#include <SD.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

int taskIsFinished = 0;
int difficulty = -1;
int hilighted = 0;
String visibleTasks[5] = {""};
int statuses[5][2] = {
  { 0, 0 },
  { 0, 0 },
  { 0, 0 },
  { 0, 0 },
  { 0, 0 }
};
int taskIndex = 0;
int screenStartIndex = 0;
int screenEndIndex = 4;

class Button {
  public:
    int pin;
    int state;
    int pressed = 0;

    Button(int pin, int defaultState){
      this->pin = pin;
      this->state = defaultState;
      pinMode(pin, INPUT);
    }
  
    void update() {
      int newState = digitalRead(pin);
      
      this->pressed = state == LOW && newState == HIGH;
      this->state = newState;
    }
    int isPressed(){
      return this->pressed;
    }
};

enum Lights {
  FINISHED = 8,
  HARD = 7,
  MEDIUM = 6,
  EASY = 5
};

Button downButton = Button(9, 0);
Button upButton = Button(10, 0);
Button difficultyButton = Button(2, 0);
Button finishedButton = Button(3, 0);

File dataFile;

#define SD_CS    4  
String fileName = "/ToDo.csv";  // File name on SD card

void setup() {
  Serial.begin(9600);
  delay(3000);  // Give serial monitor time to connect
  
  Serial.println("===================================");
  Serial.println("Unoriginal To Do List Project");
  Serial.println("===================================");
  pinMode(FINISHED, OUTPUT);
  pinMode(HARD, OUTPUT);
  pinMode(MEDIUM, OUTPUT);
  pinMode(EASY, OUTPUT);

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    Serial.println("Check connections and continue anyway.");
  } else {
    Serial.println("SD card initialized successfully.");
  }
  
  // Create CSV file with headers if it doesn't exist
  if (!SD.exists(fileName)) {
    dataFile = SD.open(fileName, FILE_WRITE);
    if (dataFile) {
      dataFile.println("Task,Difficulty,Status");
      dataFile.close();
      Serial.println("Created new CSV file with headers");
    } else {
      Serial.println("Error creating file!");
    }
  } else {
    Serial.println("File exists, will append to it");
  }
  Serial.println("Setup complete!");
  u8g2.begin();
  setVisibleTasks(screenStartIndex, screenEndIndex);
  Serial.println("===================================");
}

void loop() {
   if (Serial.available() > 0) {
    String incoming = Serial.readStringUntil('\n'); // Read incoming string
    Serial.println("Received: " + incoming);         // Print to computer
    addTask(incoming);
  }
  // put your main code here, to run repeatedly:
  downButton.update();
  upButton.update();
  difficultyButton.update();
  finishedButton.update();
  Serial.println(difficultyButton.state);

  if (difficultyButton.isPressed()) {
    difficultyPressed();
  }
  draw();
}

void draw() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 12, visibleTasks[0].c_str());
  u8g2.drawStr(0, 24, visibleTasks[1].c_str());
  u8g2.drawStr(0, 36, visibleTasks[2].c_str());
  u8g2.drawStr(0, 48, visibleTasks[3].c_str());
  u8g2.drawStr(0, 60, visibleTasks[4].c_str());
  u8g2.sendBuffer();
  delay(16);
}

void difficultyPressed() {
  difficulty = (difficulty + 1) % 3;
  setDifficultyLights();
}

void setDifficultyLights() {
  digitalWrite(EASY, LOW);
  digitalWrite(MEDIUM, LOW);
  digitalWrite(HARD, LOW);
  Serial.println(difficulty);

  switch (difficulty) {
    case 0:
      digitalWrite(EASY, HIGH);
      
      Serial.println("easy");
      break;
    case 1:
      digitalWrite(MEDIUM, HIGH);
      Serial.println("med");
      break;
    case 2:
      digitalWrite(HARD, HIGH);
      Serial.println("hard");
      break;
    default:
      break;
  }
}

void addTask(String task) {
  char s[64];
  int d;
  int check = sscanf(task.c_str(), "%63[^,], %d", s, &d);
  Serial.println(s);
  Serial.println(d);
  Serial.println(check);

  if (check == 2 && d >= 0 && d <= 2) {
    // Open the file for writing
      dataFile = SD.open(fileName, FILE_WRITE);
      
      if (dataFile) {
        // Write timestamp and movement data to CSV file
        dataFile.print(task);
        dataFile.print(", 0\n");
        
        // Close the file
        dataFile.close();
      } else {
        Serial.println("Error opening file for writing");
      }
  } else {
    Serial.println("Error with task format, must be '<task name>, <number 0-2>'");
  }
  readWholeFile();
}

void readWholeFile() {
  dataFile = SD.open(fileName);
  while (dataFile.available())
  {
    Serial.write(dataFile.read());
  }

  dataFile.close();
}

String getSubString(const String& line, int index, char sep = ',') {
  int start = 0;
  for (int i = 0; i < index; i++) {
    start = line.indexOf(sep, start);
    if (start == -1) return "";
    start++;                       // move past the separator
  }
  int end = line.indexOf(sep, start);
  if (end == -1) end = line.length();

  String field = line.substring(start, end);
  field.trim();
  return field;
}

void setVisibleTasks(int start, int end) {
  dataFile = SD.open(fileName, FILE_READ);

  // Skip header so -1
  int count = -1;
  int arrayIndex;
  Serial.println(start);
  Serial.println(end);
  if (dataFile) {
    while (dataFile.available()) {
      String line = dataFile.readStringUntil('\n');
      Serial.println(line);
      Serial.println(count);
      if (count >= start && count <= end) {
        arrayIndex = count - start;
        line.trim();              // strips the trailing '\r' and any spaces

        if (line.length() == 0) continue;   // skip blank lines

        visibleTasks[arrayIndex] = getSubString(line, 0);
        statuses[arrayIndex][0] = getSubString(line, 1).toInt();
        statuses[arrayIndex][1] = getSubString(line, 2).toInt();
      } else if (count > end) {
        break;
      }
      count += 1;
    }
    dataFile.close();
  } else {
    Serial.println("Error opening file for reading");
  }
}
