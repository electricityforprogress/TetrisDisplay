
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>



//*********Tetris Game*********
#include "GamePiece.h"
#define NEOPIN 4

#define LED_ROWS 22 //16
#define LED_COLUMNS 10 //7
const int LED_COUNT = LED_ROWS * LED_COLUMNS;

const int GAME_COLUMNS = LED_COLUMNS;
const int GAME_ROWS = LED_ROWS;

int DLY = 10; // 210; //delay used in tetris left/right/drop/rotate moves

#define RED       0xFF0000
#define BLUE      0x0000FF
#define GREEN     0x00FF00
#define WHITE     0xFFFFFF
#define BLACK     0x000000
#define YELLOW    0xFFFF00
#define SALMON    0x0000AA
#define MAGENTA   0xAA0000
#define INDIGO    0x00AA00

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <WiFi.h>
#include <ESP8266WebServer.h>

#define esp8266_rest_pin 13

//define server and access port
ESP8266WebServer server(80);

//WiFiServer server(80);

//define wifi network and password
const char *ssid = "tetris";
const char *password = "password";


byte gameField[LED_COUNT];
// MVMT SETUP
int prevBtn = 1;
int prevX = 0;
int prevY = 0;
int bri = 32;

int j = 0;
int delayTime = 5;

unsigned long loopStartTime = 0;

byte p1[4] = {1, 1, 1, 1};  
byte p2[6] = {2, 2, 2, 0, 2, 0};
byte p3[6] = {3, 0, 3, 0, 3, 3};
byte p4[6] = {0, 4, 0, 4, 4, 4};
byte p5[6] = {5, 5, 0, 0, 5, 5};
byte p6[6] = {0, 6, 6, 6, 6, 0};
byte p7[4] = {7, 7, 7, 7 };

GamePiece  _gamePieces[7] = 
{
  GamePiece(2, 2, p1 ),
  GamePiece(3, 2, p2 ),
  GamePiece(3, 2, p3 ),
  GamePiece(2, 3, p4 ),
  GamePiece(2, 3, p5 ),
  GamePiece(2, 3, p6 ),
  GamePiece(4, 1, p7 )
};

unsigned long colors[8] =
{
  RED,
  BLUE,
  GREEN,
  YELLOW,
  MAGENTA,
  SALMON,
  INDIGO,
  WHITE  
};

GamePiece * fallingPiece = NULL;
GamePiece * rotated = NULL;
GamePiece * nextPiece = NULL;

byte gameLevel = 1;
byte currentRow = 0;
byte currentColumn = 0;
byte gameLines = 0;
boolean gameOver = false;
boolean displayMode = false;
int color = 0;
int wheelRate = 10;
boolean white = true; //use to toggle all white LEDs
int gameRate = 400; //base rate for pieces falling

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(LED_ROWS, LED_COLUMNS, NEOPIN,
                            NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
                            NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
                            NEO_GRB            + NEO_KHZ800);

int pixColor = 255;

int x = 0;
int y = 0;

long currMillis = 0;
long prevMillis = 0;

int rate = 100;




void updateDisplay() {
  matrix.fillScreen(0);
  if (x > matrix.width()) {
    x = 0;
    y++;
  }
  if (y > matrix.height()) y = 0;

  matrix.drawPixel(x, y, pixColor);
  x++;
  if (x == matrix.width() && y == matrix.height()) matrix.fillScreen(1000);
  //matrix.show();
}


const String HtmlHtml = "<html><head>"
                        "<meta name='viewport' content='width=device-width, initial-scale=1' /></head>";
const String HtmlTitle = "<h1><a href=''>Tetris Display Controller</a></h1><br/>\n";
const String HtmlTextbox =
  "<form action='msg'>Textbox: <br><input type='text' name='textbox'><br>"
  "<input type='submit' value='Submit'></form>";
const String HtmlHtmlClose = "</html>";

String buttonArray(byte buttons) {

  String buttonHtml = "<a href='/left'><button style='class:btn;height:120px;width:120px;color:white;background-color:grey'>Left</button></a> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; "
                      "<a href='/right'><button style='class:btn;height:120px;width:120px;color:white;background-color:grey'>Right</button></a> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                      "<a href='/drop'><button style='class:btn;height:120px;width:120px;color:white;background-color:orange'>Drop</button></a> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                      "<a href='/rotate'><button style='class:btn;height:120px;width:120px;color:white;background-color:blue'>Rotate</button></a> ";
                      
  return buttonHtml;
}

String displayControl(){
  
   String displayHtml = "<br><br><br>";
    displayHtml = displayHtml + "<br><h3>electricityforprogress.com</h3><br><br><br>";
   if(displayMode) displayHtml ="<br><br><a href='/display'><button style='class:btn;height:20px;width:120px;color:white;background-color:green'>Display</button></a> ";
   else displayHtml = "<br><br><a href='/game'><button style='class:btn;height:20px;width:120px;color:white;background-color:red'>Game</button></a> ";

   //slider - doesn't work right now sadly, must be misuing the get with javascript
   displayHtml = displayHtml +  "<td align='center'><input type='range' id='slider' min='0' max='255' value='0' step='1' "
    "onchange='showValue(value)'>"
    "<script type='text/javascript'> function showValue(newValue){document.querySelector('#level').value=newValue; " 
                 "server = 'slider/'+ newValue; request = new XMLHttpRequest(); "
                 "request.open('GET', server, true); request.send(null);"
                 "}</script></td>";
  
  if(white) displayHtml = displayHtml + "&nbsp; <a href='/white'><button style='class:btn;height:20px;width:80px;color:black;background-color:white'>White</button></a> ";
  else displayHtml = displayHtml + "&nbsp; <a href='/white'><button style='class:btn;height:20px;width:80px;color:white;background-color:orange'>Colors</button></a> ";
  return displayHtml;
}

void handleRoot() { //on connection to IP root
 // Serial.println("Root page");
  String htmlRes = HtmlHtml + HtmlTitle + buttonArray(4) + displayControl() + HtmlHtmlClose;
//  String htmlRes = HtmlHtml + HtmlTitle + buttonArray(4) + HtmlHtmlClose;
  server.send(200, "text/html", htmlRes);
}


void handle_left(){
 // Serial.println("Left");
  handleRoot();
  //moveRight();
  moveLeft();
}

void handle_right(){
 // Serial.println("Right");  
  handleRoot();
  //moveLeft();
  moveRight();

}

void handle_drop(){
  //  Serial.println("Drop");
  handleRoot();
  drop();
}

void handle_rotate(){
  //  Serial.println("Rotate");
    handleRoot();
    rotateRight();
}

void handle_game(){
  Serial.println("GameMode");
  gameOver = true;
  exec_gameOver();
  //game restart?
  displayMode = true;
  handleRoot();
}

void handle_display(){
  Serial.println("DisplayMode");
  gameOver = true;
  exec_gameOver();
  displayMode = false; //toggle displaymode
  handleRoot();
}

void handle_white(){

  white = !white; //toggle
  handleRoot();
}

void handle_slider() { // sliding values
  String path = server.uri();
  String inValue = getValue(path,'/',2);

  int value = inValue.toInt();
 wheelRate = value; // value of 1 to 255
 handleRoot();
 Serial.print("Slider: "); Serial.println(value);
 
}

void handle_notFound(){ //passing some other variables
  String path = server.uri();
  String inValue = getValue(path,'/',2);

  int value = inValue.toInt();
 // Serial.println("notFound Value: " + value);

}

void wifiConfig(){
  
   WiFi.softAP(ssid, password);
  IPAddress apip = WiFi.softAPIP();
  Serial.println();
  Serial.print("Welcome to Tetris, locate the Tetris network and connect to: ");
  Serial.println(apip);
  server.on("/", handleRoot);
  server.on("/left", handle_left);
  server.on("/right", handle_right);
  server.on("/drop", handle_drop);
  server.on("/rotate", handle_rotate);
  server.on("/display", handle_display);
  server.on("/game", handle_game);
  server.on("/white", handle_white);
  server.on("/slider", handle_slider);
  server.onNotFound(handle_notFound);
  server.begin();

}



String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}







void setup() {


  randomSeed(analogRead(A0));

  Serial.begin(57600);

//configure wifi connection
  wifiConfig();

//***Initialize Display
  matrix.begin();
  matrix.setBrightness(40);
  matrix.fillScreen(0);
  matrix.show();
  delay(100); //
  startGame();
  
}


void loop() {
  
  currMillis = millis();

  server.handleClient();  //set mode and variables

  if(displayMode){
   //play game
    if( millis() - loopStartTime > (gameRate / (gameLevel * 0.40)) )  { //this is how fast the game plays/drops
    if( !gameOver )
    {        
      moveDown();
      gameOver = !isValidLocation(*fallingPiece, currentColumn, currentRow);       
      if(gameOver) exec_gameOver();
    }
    loopStartTime = millis();
    render();
    
    }
  }
  else{
  //show lightshow display
    if(millis() - prevMillis > wheelRate){
      for(byte i=0;i<LED_ROWS;i++){
        for(byte j=0;j<LED_COLUMNS;j++){
          setColor(i,j,Wheel(color));
        }
      }
      if(color>255) color=0; else color++;
      matrix.show();
      prevMillis = millis();
      //Display Mode updates and reads from the slider?
    }
  }

}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return matrix.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}


