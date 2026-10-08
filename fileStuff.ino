#include <SPI.h>
#include <SD.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>

void setup()
{
  pinMode(10, OUTPUT);
  Serial.begin(9600);
  delay(1000); 
  bool began = SD.begin(BUILTIN_SDCARD);
  Serial.println(began ? "true" : "false");

  for(int i = 0; i < 5; i++) {
    char fileName[20];
    sprintf(fileName, "file%d.txt", i);
    
    Serial.print("Working with file:");
    Serial.println(fileName);

    SD.remove(fileName);
    File myFile = SD.open(fileName, FILE_WRITE);
    if (myFile) {
      Serial.println("Storing data!");
      myFile.print("hello i am a file that contains data. my name is ");
      myFile.println(fileName);
      myFile.println(floor(rand() * 99999));

      myFile.close();
      Serial.println("Created file: true");
    }
    else {
      Serial.println("Created file: false");
    }

    bool exist = SD.exists(fileName);
    Serial.print("File exists: ");
    Serial.println(exist ? "true" : "false");

    Serial.println("Reading data!");
    myFile = SD.open(fileName);
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();

//    bool removed = SD.remove(fileName);
//    Serial.print("File removed: ");
//    Serial.println(removed ? "true" : "false");

    Serial.println("");
    delay(2000);
  }

  File root = SD.open("/");
  Serial.println("Files on SD card: ");
  printDirectory(root, 0);

}

void loop()
{
  
}

void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}