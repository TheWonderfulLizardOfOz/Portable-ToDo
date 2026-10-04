#include <Arduino.h>
#include <SD.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

struct task {
  String taskName;
  int taskDifficulty;
  int taskIsFinished;
};

task visibleTasks[5];


int screenStartIndex = 0;
int screenEndIndex = 4;
int taskTotal = 0;
int highlight = 0;

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
  u8g2.begin();

  taskTotal = countAvailableTasks();
  
  if (taskTotal > 0) {
    setVisibleTasks(screenStartIndex, screenEndIndex);
    setDifficultyLights();
    setIsFinishedLight();
  }

  draw();
  Serial.println("Setup complete!");
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

  if (difficultyButton.isPressed()) {
    difficultyPressed();
  }

  if (finishedButton.isPressed()) {
    finishedPressed();
  }

  if (upButton.isPressed()) {
    moveSelection(-1);
    updateLights();
  } else if (downButton.isPressed()) {
    moveSelection(1);
    updateLights();
  }

  if (finishedButton.isPressed() || difficultyButton.isPressed()) {
    saveTaskChanges(screenStartIndex + highlight);
  }

  draw();
}

void draw() {
  u8g2.clearBuffer();
  int screenWidth = 128;
  int screenHeight = 64;
  int yIndex;
  int textIndex = 12;
  int sectionHeight = 12;
  int sectionPadding = 4;
  int difficultyWidth = 32;
  int iconWidth = 8;
  int textWidth = 76;
  int difficultyIndex = 92;

  u8g2.setFont(u8g2_font_ncenB08_tr);
  // Draw Tasks
  for (int i = 0; i < 5; i++) {
    // case of less than 5 tasks
    if (screenStartIndex + i >= taskTotal) {
      break;
    }
    u8g2.setDrawColor(1);
    yIndex = (i + 1) * sectionHeight;
    
    if (i == highlight) {
      u8g2.drawBox(textIndex - (sectionPadding /2), yIndex - sectionHeight + (sectionPadding / 2), textWidth + sectionPadding, sectionHeight);
      u8g2.setDrawColor(0);
    }
    u8g2.drawStr(textIndex, yIndex, visibleTasks[i].taskName.c_str());

  }

  // Cover up overflowing text
  u8g2.setDrawColor(0);
  u8g2.drawBox(difficultyIndex - (sectionPadding / 2), 0, screenWidth - difficultyIndex + (sectionPadding / 2), screenHeight);
  u8g2.setDrawColor(1);

  for (int i = 0; i < 5; i++) {
    // case of less than 5 tasks
    if (screenStartIndex + i >= taskTotal) {
      break;
    }
    yIndex = (i + 1) * sectionHeight;
    switch (visibleTasks[i].taskDifficulty) {
      case 0:
        u8g2.drawStr(difficultyIndex, yIndex, "EASY");
        break;
      case 1:
        u8g2.drawStr(difficultyIndex, yIndex, "MED");
        break;
      case 2:
        u8g2.drawStr(difficultyIndex, yIndex, "HARD");
        break;
      default:
        u8g2.drawStr(difficultyIndex, yIndex, "ERR!");
    }
  }

  // Draw Statuses
  u8g2.setFont(u8g2_font_open_iconic_check_1x_t);
  u8g2.setDrawColor(1);
  for (int i = 0; i < 5; i++) {
    // case of less than 5 tasks
    if (screenStartIndex + i >= taskTotal) {
      break;
    }
    yIndex = (i + 1) * sectionHeight;
    if (visibleTasks[i].taskIsFinished) {
      u8g2.drawGlyph(0, yIndex, 64);
    } else {
      u8g2.drawGlyph(0, yIndex, 68);
    }
  }

  u8g2.sendBuffer();
  delay(16);
}

void difficultyPressed() {
  visibleTasks[highlight].taskDifficulty = (visibleTasks[highlight].taskDifficulty + 1) % 3;
  setDifficultyLights();
}

void finishedPressed() {
  visibleTasks[highlight].taskIsFinished = !visibleTasks[highlight].taskIsFinished;
  setIsFinishedLight();
}

void setDifficultyLights() {
  digitalWrite(EASY, LOW);
  digitalWrite(MEDIUM, LOW);
  digitalWrite(HARD, LOW);

  if (taskTotal <= 0) {
    return;
  }

  switch (visibleTasks[highlight].taskDifficulty) {
    case 0:
      digitalWrite(EASY, HIGH);
      break;
    case 1:
      digitalWrite(MEDIUM, HIGH);
      break;
    case 2:
      digitalWrite(HARD, HIGH);
      break;
    default:
      break;
  }
}

void setIsFinishedLight() {
  if (taskTotal <= 0) {
    return;
  }

  if (visibleTasks[highlight].taskIsFinished) {
    digitalWrite(FINISHED, HIGH);
  } else {
    digitalWrite(FINISHED, LOW);
  }
}

void updateLights() {
  setDifficultyLights();
  setIsFinishedLight();
}

void addTask(String task) {
  char s[64];
  int d;
  int check = sscanf(task.c_str(), "%63[^,], %d", s, &d);

  if (check == 2 && d >= 0 && d <= 2) {
    // Open the file for writing
      dataFile = SD.open(fileName, FILE_WRITE);
      
      if (dataFile) {
        // Write timestamp and movement data to CSV file
        dataFile.print(task);
        dataFile.print(", 0\n");
        
        // Close the file
        dataFile.close();
        taskTotal += 1;
        setVisibleTasks(screenStartIndex, screenEndIndex);
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

int countAvailableTasks() {
  dataFile = SD.open(fileName, FILE_READ);
  // Skip header so -1
  int count = -1;
  if (dataFile) {
    while (dataFile.available()) {
      String line = dataFile.readStringUntil('\n');
      count += 1;
    }
    dataFile.close();
  } else {
    Serial.println("Error opening file for reading");
    return 0;
  }
  return count;
}

void setVisibleTasks(int start, int end) {
  dataFile = SD.open(fileName, FILE_READ);

  // Skip header so -1
  int count = -1;
  int arrayIndex;
  if (dataFile) {
    while (dataFile.available()) {
      String line = dataFile.readStringUntil('\n');
      if (count >= start && count <= end) {
        arrayIndex = count - start;
        line.trim();              // strips the trailing '\r' and any spaces

        if (line.length() == 0) continue;   // skip blank lines

        visibleTasks[arrayIndex].taskName = getSubString(line, 0);
        visibleTasks[arrayIndex].taskDifficulty = getSubString(line, 1).toInt();
        visibleTasks[arrayIndex].taskIsFinished = getSubString(line, 2).toInt();
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

void saveTaskChanges(int taskIndex) {
  if (taskTotal <= 0) {
    return;
  }
  task curTask = visibleTasks[taskIndex];
  String taskFull = curTask.taskName + ", " + String(curTask.taskDifficulty) + ", " + String(curTask.taskIsFinished);

  // skip header so -1
  int count = -1;
  uint32_t lineStart;

  dataFile = SD.open(fileName, O_RDWR);
  if (dataFile) {
    while (dataFile.available()) {
      lineStart = dataFile.position();
      String line = dataFile.readStringUntil('\n');
      if (count == taskIndex) {
        line.trim();   
        if (line.length() != taskFull.length()) {
          Serial.println("Length Mismatch!");
          dataFile.close();
          return;
        }

        dataFile.seek(lineStart);
        dataFile.print(taskFull);
      } else if (count > taskIndex) {
        break;
      }
      count += 1;
    }
    dataFile.close();
  } else {
    Serial.println("Error opening file");
  }
}

void moveSelection(int moveDirection) {
  int curTaskIndex = screenStartIndex + highlight;
  int newTaskIndex = curTaskIndex + moveDirection;
  int newHighlight = highlight + moveDirection;

  if (taskTotal == 0) {
    return;
  }

  // sets to last task
  if (newTaskIndex < 0) {
    newTaskIndex = taskTotal - 1;
    if (taskTotal <= 5) {
      highlight = newTaskIndex;
      screenStartIndex = 0;
      screenEndIndex = 4;
    } else{
      highlight = 4;
      screenStartIndex = newTaskIndex - 4;
      screenEndIndex = newTaskIndex;
    }
  } 
  // sets to first task
  else if (newTaskIndex >= taskTotal) {
    newTaskIndex = 0;
    screenStartIndex = 0;
    highlight = 0;
    if (taskTotal <= 5) {
      screenEndIndex = taskTotal - 1;
    } else {
      screenEndIndex = 4;
    }
  }
  // move up screen
  else if (newHighlight < 0) {
    highlight = 0;
    screenStartIndex -= 1;
    screenEndIndex -= 1;
  }

  // move down screen
  else if (newHighlight > 4) {
    highlight = 4;
    screenStartIndex += 1;
    screenEndIndex += 1;
  }

  // simply move selection
  else {
    highlight = newHighlight;
  }

  setVisibleTasks(screenStartIndex, screenEndIndex);
}
