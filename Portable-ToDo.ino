#include <Arduino.h>
#include <SD.h>

int taskIsFinished = 0;
int difficulty = 0;

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
  Serial.println("===================================");
}

void loop() {
  // put your main code here, to run repeatedly:
  downButton.update();
  upButton.update();
  difficultyButton.update();
  finishedButton.update();

  if (difficultyButton.isPressed()) {
    difficultyPressed();
  }
}

void difficultyPressed() {
  difficulty = (difficulty + 1) % 3;
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
