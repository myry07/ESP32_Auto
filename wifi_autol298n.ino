#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define UP_LEFT 5
#define UP_RIGHT 6
#define DOWN_LEFT 7
#define DOWN_RIGHT 8
#define TURN_LEFT 9
#define TURN_RIGHT 10
#define LED_ON 11
#define LED_OFF 12
#define STOP 0

#define FRONT_RIGHT_MOTOR 0
#define BACK_RIGHT_MOTOR 1
#define FRONT_LEFT_MOTOR 2
#define BACK_LEFT_MOTOR 3
#define LED_PIN 4

#define FORWARD 1
#define BACKWARD -1

// 定义结构体用于存储电机引脚信息
struct MOTOR_PINS {
  int pinIN1;
  int pinIN2;
  int pinLED;  // 新增的LED控制引脚
};

// 定义电机引脚信息
std::vector<MOTOR_PINS> motorPins = {
  { 16, 17, -1 },  //前右电机
  { 22, 23, -1 },  //后右电机
  { 27, 26, -1 },  //前左电机
  { 18, 19, -1 },  //后左电机
  { 4, -1, 4 },    //LED控制
};

const char *ssid = "ESP32_Auto";
const char *password = "wjq110422";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char *htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
      .arrows {
        font-size:70px;
        color:red;
      }
      .circularArrows {
        font-size:80px;
        color:blue;
      }
      td {
        background-color:black;
        border-radius:25%;
        box-shadow:5px 5px #888888;
      }

      td:active {
        transform:
        translate(5px, 5px);
        box-shadow: none;
      }

      .noselect {
        -webkit-touch-callout: none; /* 10S Safari */
        -webkit-user-select: none;  /* Safari */
        -khtml-user-select: none; /* Konquerer HIML */
        -moz-user-select: none;  /* Firefox */
        -ms-user-select: none;  /* Internet Explorer/Edge */
        user-select: none;      /* Non-prefixed version, currently supported by Chrome and Opera */
      }
    </style>
  </head>
  <body class="noselect" align="center" style="background-color:white">

    <h1 style="color: teal; text-align:center;">MYRY</h1>
    <h2 style="color: teal; text-align:center;">ESP32_AUTO</h2>

    <table id="mainTable" style="width: 400px; margin:auto; table-layout:fixed" CELLSPACING="10">
      <tr>
        <td ontouchstart='onTouchStartAndEnd("5")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows">&#11017;</span></td>
        <td ontouchstart='onTouchStartAndEnd("1")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows">&#8679;</span></td>
        <td ontouchstart='onTouchStartAndEnd("6")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows">&#11016;</span></td>
      </tr>
      <tr>
        <td ontouchstart='onTouchStartAndEnd("3")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows">&#8678;</span></td>
        <td></td>
        <td ontouchstart='onTouchStartAndEnd("4")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows">&#8680;</span></td>
      </tr>
      <tr>
        <td ontouchstart='onTouchStartAndEnd("7")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows">&#11019;</span></td>
        <td ontouchstart='onTouchStartAndEnd("2")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows">&#8681;</span></td>
        <td ontouchstart='onTouchStartAndEnd("g")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows">&#11018;</span></td>
      </tr>
      <tr>
        <td ontouchstart='onTouchStartAndEnd("9")' ontouchend='onTouchStartAndEnd("0")'><span class="circularArrows">&#8634;</span></td>
        <td ontouchstart='onTouchStartAndEnd("11")' ontouchend='onTouchStartAndEnd("0")'><span style="color: yellow; font-size: 50px;">LED</span></td>
        <td ontouchstart='onTouchStartAndEnd("10")' ontouchend='onTouchStartAndEnd("0")'><span class="circularArrows">&#8635;</span></td>
      </tr>
    </table>

    <script>
      var webSocketUrl = "ws:\/\/" + window.location.hostname + "/ws";
      var websocket;

      function initWebSocket() {
        websocket = new WebSocket(webSocketUrl);
        websocket.onopen = function(event) {};
        websocket.onclose = function(event) {
          setTimeout(initWebSocket, 2000);
        };
        websocket.onmessage = function(event) {};
      }

      function onTouchStartAndEnd(value) {
        websocket.send(value);
      }

      window.onload = initWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event) { event.preventDefault() });
    </script>

  </body>
</html>
)HTMLHOMEPAGE";

// 旋转电机
void rotateMotor(int motorNumber, int motorDirection) {
  Serial.print("旋转电机 ");
  Serial.print(motorNumber);
  Serial.print(" 方向 ");
  Serial.println(motorDirection);

  // 添加用于检查引脚状态的调试语句
  Serial.print("IN1 状态: ");
  Serial.println(digitalRead(motorPins[motorNumber].pinIN1));
  Serial.print("IN2 状态: ");
  Serial.println(digitalRead(motorPins[motorNumber].pinIN2));

  if (motorDirection == FORWARD) {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  } else if (motorDirection == BACKWARD) {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);
  } else {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  }
}

// 处理小车运动
void processCarMovement(String inputValue) {
  Serial.printf("接收到的值为 %s %d\n", inputValue.c_str(), inputValue.toInt());
  switch (inputValue.toInt()) {
    case UP:
      rotateMotor(FRONT_RIGHT_MOTOR, FORWARD);
      rotateMotor(BACK_RIGHT_MOTOR, FORWARD);
      rotateMotor(FRONT_LEFT_MOTOR, FORWARD);
      rotateMotor(BACK_LEFT_MOTOR, FORWARD);
      break;

    case DOWN:
      rotateMotor(FRONT_RIGHT_MOTOR, BACKWARD);
      rotateMotor(BACK_RIGHT_MOTOR, BACKWARD);
      rotateMotor(FRONT_LEFT_MOTOR, BACKWARD);
      rotateMotor(BACK_LEFT_MOTOR, BACKWARD);
      break;

    case LEFT:
      rotateMotor(FRONT_RIGHT_MOTOR, FORWARD);
      rotateMotor(BACK_RIGHT_MOTOR, BACKWARD);
      rotateMotor(FRONT_LEFT_MOTOR, BACKWARD);
      rotateMotor(BACK_LEFT_MOTOR, FORWARD);
      break;

    case RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR, BACKWARD);
      rotateMotor(BACK_RIGHT_MOTOR, FORWARD);
      rotateMotor(FRONT_LEFT_MOTOR, FORWARD);
      rotateMotor(BACK_LEFT_MOTOR, BACKWARD);
      break;

    case LED_ON:
      Serial.println("LED 打开");
      digitalWrite(motorPins[0].pinLED, HIGH);  // 使用正确的GPIO引脚编号
      break;

    case LED_OFF:
      Serial.println("LED 关闭");
      digitalWrite(motorPins[0].pinLED, LOW);  // 使用正确的GPIO引脚编号
      break;

    default:
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(BACK_RIGHT_MOTOR, STOP);
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR, STOP);
      break;
  }
}

// 处理根路径请求
void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlHomePage);
}

// 处理未找到页面的请求
void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "File Not Found");
}

// WebSocket事件处理函数
void onWebSocketEvent(AsyncWebSocket *server,
                      AsyncWebSocketClient *client,
                      AwsEventType type,
                      void *arg,
                      uint8_t *data,
                      size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket 客户端 #%u 连接来自 %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket 客户端 #%u 断开连接\n", client->id());
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo *)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        std::string myData = "";
        myData.assign((char *)data, len);
        Serial.printf("接收到的消息: %s\n", myData.c_str());
        processCarMovement(myData.c_str());
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;
  }
}

// 设置引脚模式
void setUpPinModes() {
  for (int i = 0; i < motorPins.size(); i++) {
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);
    pinMode(motorPins[i].pinLED, OUTPUT);
    rotateMotor(i, STOP);
    digitalWrite(4, LOW);
  }
}

// 初始化函数
void setup(void) {
  setUpPinModes();
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP 地址: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.begin();
  Serial.println("HTTP 服务器已启动");
}

// 循环函数
void loop() {
  ws.cleanupClients();
  delay(500);  // 添加延迟
