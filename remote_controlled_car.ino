#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// ===== KHAI BÁO SERVO =====
Servo servoGap;    // GPIO24
Servo servoTay;    // GPIO25  
Servo servoXoay;   // GPIO26
Servo servoKhac;   // GPIO27

// ===== VỊ TRÍ HIỆN TẠI CỦA CÁC SERVO =====
int posGap = 90;     
int posTay = 90;     
int posXoay = 90;    
int posKhac = 90;    

// ===== KHAI BÁO CHÂN ĐIỀU KHIỂN MOTOR =====
#define ENA 15       
#define ENB 5        
#define IN1 2        
#define IN2 4        
#define IN3 16       
#define IN4 17       

// ===== CẤU HÌNH PWM CHO MOTOR - TƯƠNG THÍCH PHIÊN BẢN MỚI =====
#define MOTOR_PWM_FREQ 1000    
#define MOTOR_PWM_RES 8        

// ===== BIẾN ĐIỀU KHIỂN AN TOÀN SERVO =====
volatile bool servoMoving = false;           
volatile bool stopServo = false;             
unsigned long lastServoCommand = 0;          
const unsigned long SERVO_DELAY = 50;        
TaskHandle_t servoTaskHandle = NULL;         

// ===== BIẾN QUẢN LÝ SERVO DETACH =====
unsigned long servoDetachTime[4] = {0, 0, 0, 0};  
const unsigned long DETACH_DELAY = 2000;          
bool servoAttached[4] = {false, false, false, false};  

// ===== BIẾN TỐC ĐỘ MOTOR =====
int tocdoxetrai = 0;    
int tocdoxephai = 0;    

// ===== CẤU HÌNH WIFI ACCESS POINT =====
const char* ssid = "Robocar WebServer ESP32";  
const char* pass = "Robocar@esp32";            
WebServer server(80);                          

// ===== STRUCT CHỨA THÔNG TIN SERVO =====
struct ServoCommand {
  Servo* servo;
  int* currentPos;
  int targetPos;
  String name;
  int servoIndex;
};

// ===== HÀM DETACH SERVO SAU KHI HOÀN THÀNH =====
void detachServoAfterDelay(int servoIndex) {
  servoDetachTime[servoIndex] = millis() + DETACH_DELAY;
}

// ===== HÀM ATTACH SERVO KHI CẦN SỬ DỤNG =====
void attachServoIfNeeded(Servo* servo, int pin, int servoIndex) {
  if (!servoAttached[servoIndex]) {
    servo->attach(pin, 500, 2500);
    servoAttached[servoIndex] = true;
    delay(100);  
    Serial.println("Servo " + String(servoIndex) + " attached to pin " + String(pin));
  }
}

// ===== TASK DI CHUYỂN SERVO CHẠY TRÊN CORE RIÊNG =====
void servoTask(void* parameter) {
  ServoCommand* cmd = (ServoCommand*)parameter;
  
  Serial.println("Moving " + cmd->name + " from " + String(*cmd->currentPos) + " to " + String(cmd->targetPos));
  
  // Đảm bảo servo được attach
  int pin = 24 + cmd->servoIndex;
  attachServoIfNeeded(cmd->servo, pin, cmd->servoIndex);
  
  int step = (cmd->targetPos > *cmd->currentPos) ? 5 : -5;  
  int pos = *cmd->currentPos;
  
  // Di chuyển từ từ đến vị trí đích
  while (abs(pos - cmd->targetPos) > 5 && !stopServo) {
    pos += step;
    pos = constrain(pos, 0, 180);
    cmd->servo->write(pos);
    vTaskDelay(pdMS_TO_TICKS(15));  
  }
  
  // Di chuyển đến vị trí cuối cùng nếu chưa bị dừng
  if (!stopServo) {
    cmd->servo->write(cmd->targetPos);
    *cmd->currentPos = cmd->targetPos;
    vTaskDelay(pdMS_TO_TICKS(500));  
    
    Serial.println(cmd->name + " moved to " + String(cmd->targetPos));
    detachServoAfterDelay(cmd->servoIndex);
    
  } else {
    *cmd->currentPos = pos;
    Serial.println(cmd->name + " stopped at " + String(pos));
    detachServoAfterDelay(cmd->servoIndex);
  }
  
  // Reset các cờ
  servoMoving = false;
  stopServo = false;
  
  // Giải phóng bộ nhớ và xóa task
  delete cmd;
  servoTaskHandle = NULL;
  vTaskDelete(NULL);
}

// ===== HÀM DI CHUYỂN SERVO AN TOÀN =====
bool moveServo(Servo &servo, int &currentPos, int targetPos, String servoName, int servoIndex) {
  if (servoMoving) {
    Serial.println("Servo busy, skipping command for " + servoName);
    return false;
  }
  
  if (abs(currentPos - targetPos) <= 5) {
    Serial.println(servoName + " already at target position " + String(targetPos));
    return true;
  }
  
  if (millis() - lastServoCommand < SERVO_DELAY) {
    Serial.println("Command too fast for " + servoName + ", wait a bit");
    return false;
  }

  servoMoving = true;
  lastServoCommand = millis();
  
  ServoCommand* cmd = new ServoCommand;
  cmd->servo = &servo;
  cmd->currentPos = &currentPos;
  cmd->targetPos = targetPos;
  cmd->name = servoName;
  cmd->servoIndex = servoIndex;
  
  xTaskCreatePinnedToCore(
    servoTask,           
    "ServoTask",         
    4096,               
    cmd,                
    2,                  
    &servoTaskHandle,   
    0                   
  );
  
  return true;
}

// ===== HÀM DỪNG SERVO =====
bool stopServoMovement() {
  if (servoMoving && servoTaskHandle != NULL) {
    stopServo = true;
    Serial.println("Stopping servo movement...");
    
    int timeout = 200;  
    while (servoMoving && timeout > 0) {
      vTaskDelay(pdMS_TO_TICKS(10));
      timeout--;
    }
    
    if (timeout == 0) {
      servoMoving = false;
      stopServo = false;
      Serial.println("Force stopped servo");
    }
    
    return true;
  }
  return false;
}

// ===== HÀM DI CHUYỂN SERVO NHANH =====
void moveServoFast(Servo &servo, int &currentPos, int targetPos, String servoName, int servoIndex) {
  stopServoMovement();
  delay(100);
  
  int pin = 24 + servoIndex;
  attachServoIfNeeded(&servo, pin, servoIndex);
  
  Serial.println("Fast moving " + servoName + " to " + String(targetPos));
  servo.write(targetPos);
  currentPos = targetPos;
  delay(800);  
  
  detachServoAfterDelay(servoIndex);
}

// ===== HÀM KIỂM TRA VÀ DETACH SERVO =====
void checkAndDetachServos() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < 4; i++) {
    if (servoAttached[i] && servoDetachTime[i] > 0 && currentTime >= servoDetachTime[i]) {
      switch(i) {
        case 0: servoGap.detach(); break;
        case 1: servoTay.detach(); break;
        case 2: servoXoay.detach(); break;
        case 3: servoKhac.detach(); break;
      }
      servoAttached[i] = false;
      servoDetachTime[i] = 0;
      Serial.println("Servo " + String(i) + " detached to prevent jitter");
    }
  }
}

// ===== HÀM CẬP NHẬT TỐC ĐỘ MOTOR - TƯƠNG THÍCH PHIÊN BẢN MỚI =====
void updateMotorSpeed() {
  analogWrite(ENA, tocdoxetrai);
  analogWrite(ENB, tocdoxephai);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== KHỞI TẠO HỆ THỐNG ===");

  WiFi.mode(WIFI_OFF);
  delay(500);

  // ===== KHỞI TẠO MOTOR TRƯỚC - SỬ DỤNG ANALOGWRITE =====
  Serial.println("Khởi tạo motor...");
  
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Khởi tạo tất cả chân ở trạng thái LOW
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  
  // Test motor ngay sau khi khởi tạo
  Serial.println("Testing motor...");
  analogWrite(ENA, 150);
  analogWrite(ENB, 150);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(1000);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  Serial.println("Motor test completed");

  // ===== KHỞI TẠO CÁC SERVO SAU =====
  Serial.println("Khởi tạo servo...");
  
  // Cấp phát timer cho servo TÁCH RIÊNG với motor
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servoGap.setPeriodHertz(50);
  servoTay.setPeriodHertz(50);
  servoXoay.setPeriodHertz(50);
  servoKhac.setPeriodHertz(50);

  // Khởi tạo từng servo một cách cẩn thận
  Serial.println("Khởi tạo Servo Gap (GPIO24)...");
  servoGap.attach(24, 500, 2500);
  servoAttached[0] = true;
  servoGap.write(90);
  delay(1000);
  
  Serial.println("Khởi tạo Servo Tay (GPIO25)...");
  servoTay.attach(25, 500, 2500);
  servoAttached[1] = true;
  servoTay.write(90);
  delay(1000);
  
  Serial.println("Khởi tạo Servo Xoay (GPIO26)...");
  servoXoay.attach(26, 500, 2500);
  servoAttached[2] = true;
  servoXoay.write(90);
  delay(1000);
  
  Serial.println("Khởi tạo Servo Khac (GPIO27)...");
  servoKhac.attach(27, 500, 2500);
  servoAttached[3] = true;
  servoKhac.write(90);
  delay(1000);

  Serial.println("Tất cả servo đã được khởi tạo ở vị trí 90 độ");
  
  // Detach tất cả servo sau khi khởi tạo
  Serial.println("Detaching servos to prevent jitter...");
  servoGap.detach();
  servoTay.detach();
  servoXoay.detach();
  servoKhac.detach();
  
  for (int i = 0; i < 4; i++) {
    servoAttached[i] = false;
    servoDetachTime[i] = 0;
  }
  
  Serial.println("Servo sẵn sàng!");

  // ===== KHỞI TẠO WIFI =====
  Serial.println("Khởi tạo WiFi...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  delay(1000);

  // ===== CẤU HÌNH ROUTES =====
  server.on("/", handleRoot);

  // ===== SERVO ROUTES =====
  server.on("/mogap", []() { 
    Serial.println("Command: Mở gắp");
    if (moveServo(servoGap, posGap, 180, "Gap", 0)) {
      server.send(200, "text/plain", "Mở gắp OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  server.on("/donggap", []() { 
    Serial.println("Command: Đóng gắp");
    if (moveServo(servoGap, posGap, 0, "Gap", 0)) {
      server.send(200, "text/plain", "Đóng gắp OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  server.on("/co", []() { 
    Serial.println("Command: Co tay");
    if (moveServo(servoTay, posTay, 0, "Tay", 1)) {
      server.send(200, "text/plain", "Co tay OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  server.on("/duoi", []() { 
    Serial.println("Command: Duỗi tay");
    if (moveServo(servoTay, posTay, 180, "Tay", 1)) {
      server.send(200, "text/plain", "Duỗi tay OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  server.on("/xoaytrai", []() { 
    Serial.println("Command: Xoay trái");
    if (moveServo(servoXoay, posXoay, 0, "Xoay", 2)) {
      server.send(200, "text/plain", "Xoay trái OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  server.on("/xoayphai", []() { 
    Serial.println("Command: Xoay phải");
    if (moveServo(servoXoay, posXoay, 180, "Xoay", 2)) {
      server.send(200, "text/plain", "Xoay phải OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  server.on("/servo4len", []() { 
    Serial.println("Command: Servo 4 lên");
    if (moveServo(servoKhac, posKhac, 180, "Servo4", 3)) {
      server.send(200, "text/plain", "Servo 4 lên OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  server.on("/servo4xuong", []() { 
    Serial.println("Command: Servo 4 xuống");
    if (moveServo(servoKhac, posKhac, 0, "Servo4", 3)) {
      server.send(200, "text/plain", "Servo 4 xuống OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  // ===== NÚT DỪNG SERVO =====
  server.on("/stopservo", []() { 
    Serial.println("Command: Stop servo");
    if (stopServoMovement()) {
      server.send(200, "text/plain", "Servo đã dừng"); 
    } else {
      server.send(200, "text/plain", "Không có servo nào đang chạy");
    }
  });

  server.on("/reset", []() { 
    Serial.println("Command: Reset all servos");
    stopServoMovement();
    delay(200);
    moveServoFast(servoGap, posGap, 90, "Gap", 0);
    delay(100);
    moveServoFast(servoTay, posTay, 90, "Tay", 1);
    delay(100);
    moveServoFast(servoXoay, posXoay, 90, "Xoay", 2);
    delay(100);
    moveServoFast(servoKhac, posKhac, 90, "Servo4", 3);
    server.send(200, "text/plain", "Reset OK"); 
  });

  server.on("/status", []() {
    String status = "Gap:" + String(posGap) + 
                   ",Tay:" + String(posTay) + 
                   ",Xoay:" + String(posXoay) + 
                   ",Servo4:" + String(posKhac) +
                   ",Moving:" + String(servoMoving) +
                   ",Speed:" + String(tocdoxetrai);
    server.send(200, "text/plain", status);
  });

  // ===== MOTOR ROUTES - SỬ DỤNG ANALOGWRITE =====
  server.on("/tien", []() { 
    digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
    updateMotorSpeed();
    Serial.println("Đang tiến");
    server.send(200, "text/plain", "Tiến OK"); 
  });
  
  server.on("/lui", []() { 
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    updateMotorSpeed();
    Serial.println("Đang lùi");
    server.send(200, "text/plain", "Lùi OK"); 
  });
  
  server.on("/trai", []() { 
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
    updateMotorSpeed();
    Serial.println("Đang rẽ trái");
    server.send(200, "text/plain", "Trái OK"); 
  });
  
  server.on("/phai", []() { 
    digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    updateMotorSpeed();
    Serial.println("Đang rẽ phải");
    server.send(200, "text/plain", "Phải OK"); 
  });
  
  server.on("/stop", []() { 
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    Serial.println("Đang dừng");
    server.send(200, "text/plain", "Dừng OK"); 
  });

  server.on("/speed", []() {
    if (server.hasArg("value")) {
      int newspeed = server.arg("value").toInt();
      newspeed = constrain(newspeed, 0, 255);
      tocdoxetrai = newspeed;
      tocdoxephai = newspeed;
      Serial.println("Tốc độ: " + String(newspeed));
      server.send(200, "text/plain", "Speed OK");
    } else {
      server.send(400, "text/plain", "Thiếu tham số tốc độ");
    }
  });

  server.begin();
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("=== HỆ THỐNG SẴN SÀNG ===");
}

void handleRoot() {
  String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no, maximum-scale=1, minimum-scale=1, viewport-fit=cover'>";
  page += "<title>Robocar & Servo Controller</title>";
  page += "<style>";
  
  page += "html, body { margin: 0; padding: 0; width: 100%; user-select: none; -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; touch-action: manipulation; }";
  page += "body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #1a2a6c, #b21f1f, #fdbb2d); color: white; display: flex; flex-direction: column; align-items: center; text-align: center; min-height: 100vh; padding-bottom: 20px; }";
  page += "h1 { margin: 15px 0; text-shadow: 2px 2px 4px rgba(0,0,0,0.5); }";
  page += ".control-panel { width: 90%; max-width: 800px; background: rgba(0,0,0,0.4); border-radius: 20px; padding: 20px; box-shadow: 0 10px 25px rgba(0,0,0,0.3); margin: 10px 0; }";
  page += ".status-bar { position: sticky; top: 0; background: rgba(0,0,0,0.8); padding: 10px; border-radius: 10px; margin-bottom: 15px; width: 90%; max-width: 800px; z-index: 100; box-shadow: 0 4px 10px rgba(0,0,0,0.3); }";
  page += ".speed-control { margin: 15px 0; padding: 15px; background: rgba(0,0,0,0.3); border-radius: 15px; }";
  page += ".speed-slider { width: 90%; margin: 10px 0; -webkit-appearance: none; height: 15px; border-radius: 10px; background: rgba(255,255,255,0.2); outline: none; }";
  page += ".speed-slider::-webkit-slider-thumb { -webkit-appearance: none; width: 25px; height: 25px; border-radius: 50%; background: #2193b0; cursor: pointer; }";
  page += ".speed-value { font-size: 24px; font-weight: bold; margin: 5px 0; }";
  page += ".movement-controls { display: flex; flex-direction: column; align-items: center; margin: 15px 0; }";
  page += ".direction-controls { display: flex; justify-content: space-between; width: 100%; margin: 10px 0; }";
  page += ".vertical-controls, .horizontal-controls { display: flex; flex-direction: column; align-items: center; }";
  page += ".horizontal-controls { flex-direction: row; justify-content: center; gap: 20px; }";
  page += ".btn { padding: 15px 0; margin: 5px; font-size: 18px; border: none; border-radius: 50px; cursor: pointer; background: #2193b0; color: white; box-shadow: 0 4px 15px rgba(0,0,0,0.2); transition: all 0.3s ease; width: 100%; max-width: 150px; }";
  page += ".btn:active, .btn.pressed { transform: scale(0.95); background: #1c7a94; box-shadow: 0 2px 10px rgba(0,0,0,0.3); }";
  page += ".btn:disabled { background: #cccccc; cursor: not-allowed; }";
  page += ".direction-btn { width: 60px; height: 60px; border-radius: 50%; display: flex; justify-content: center; align-items: center; font-size: 24px; padding: 0; }";
  page += ".servo-controls { margin: 15px 0; }";
  page += ".servo-section { margin: 15px 0; padding: 10px; background: rgba(0,0,0,0.3); border-radius: 15px; }";
  page += ".section-title { margin: 5px 0 10px 0; font-size: 18px; font-weight: bold; }";
  page += ".servo-buttons { display: flex; justify-content: center; gap: 10px; flex-wrap: wrap; }";
  page += ".stop-btn { background: #ff4757 !important; }";
  
  page += "@media (max-width: 768px) {";
  page += "  .btn { padding: 12px 0; font-size: 16px; }";
  page += "  .direction-btn { width: 50px; height: 50px; }";
  page += "  h1 { font-size: 24px; }";
  page += "  .direction-controls { flex-direction: column; }";
  page += "  .vertical-controls { margin-bottom: 20px; }";
  page += "}";
  
  page += "@media (orientation: landscape) {";
  page += "  .direction-controls { flex-direction: row; }";
  page += "  .vertical-controls, .horizontal-controls { margin: 0 20px; }";
  page += "  .servo-buttons { flex-direction: row; }";
  page += "}";
  
  page += "</style></head><body>";

  page += "<div class='status-bar'>";
  page += "<h1>Robocar & Servo Controller</h1>";
  page += "<div id='status'>Sẵn sàng</div>";
  page += "</div>";
  
  page += "<div class='control-panel'>";
  page += "<div class='speed-control'>";
  page += "<div>Tốc độ: <span class='speed-value' id='speedValue'>0</span></div>";
  page += "<input type='range' class='speed-slider' id='speedRange' min='0' max='255' value='0' oninput='updateSpeed(this.value)'>";
  page += "</div>";
  
  page += "<div class='movement-controls'>";
  page += "<h2>Điều khiển di chuyển</h2>";
  page += "<div class='direction-controls'>";
  
  page += "<div class='vertical-controls'>";
  page += "<button class='btn direction-btn' id='tien' onmousedown=\"sendCmd('tien', true)\" onmouseup=\"sendCmd('stop', true)\" ontouchstart=\"sendCmd('tien', true)\" ontouchend=\"sendCmd('stop', true)\">▲</button>";
  page += "<div style='margin: 10px 0;'>Tiến/Lùi</div>";
  page += "<button class='btn direction-btn' id='lui' onmousedown=\"sendCmd('lui', true)\" onmouseup=\"sendCmd('stop', true)\" ontouchstart=\"sendCmd('lui', true)\" ontouchend=\"sendCmd('stop', true)\">▼</button>";
  page += "</div>";
  
  page += "<div class='vertical-controls'>";
  page += "<div class='horizontal-controls'>";
  page += "<button class='btn direction-btn' id='trai' onmousedown=\"sendCmd('trai', true)\" onmouseup=\"sendCmd('stop', true)\" ontouchstart=\"sendCmd('trai', true)\" ontouchend=\"sendCmd('stop', true)\">◀</button>";
  page += "<button class='btn direction-btn' id='phai' onmousedown=\"sendCmd('phai', true)\" onmouseup=\"sendCmd('stop', true)\" ontouchstart=\"sendCmd('phai', true)\" ontouchend=\"sendCmd('stop', true)\">▶</button>";
  page += "</div>";
  page += "<div style='margin: 10px 0;'>Trái/Phải</div>";
  page += "</div>";
  
  page += "</div>";
  page += "</div>";
  page += "</div>";
  
  page += "<div class='control-panel servo-controls'>";
  page += "<h2>Điều khiển Servo</h2>";
  
  page += "<div class='servo-section'>";
  page += "<div class='section-title'>Gắp</div>";
  page += "<div class='servo-buttons'>";
  page += "<button class='btn' onclick=\"sendCmd('mogap')\">Mở gắp</button>";
  page += "<button class='btn' onclick=\"sendCmd('donggap')\">Đóng gắp</button>";
    page += "</div>";
  
  page += "<div class='servo-section'>";
  page += "<div class='section-title'>Tay</div>";
  page += "<div class='servo-buttons'>";
  page += "<button class='btn' onclick=\"sendCmd('co')\">Co tay</button>";
  page += "<button class='btn' onclick=\"sendCmd('duoi')\">Duỗi tay</button>";
  page += "</div>";
  page += "</div>";
  
  page += "<div class='servo-section'>";
  page += "<div class='section-title'>Xoay</div>";
  page += "<div class='servo-buttons'>";
  page += "<button class='btn' onclick=\"sendCmd('xoaytrai')\">Xoay trái</button>";
  page += "<button class='btn' onclick=\"sendCmd('xoayphai')\">Xoay phải</button>";
  page += "</div>";
  page += "</div>";
  
  page += "<div class='servo-section'>";
  page += "<div class='section-title'>Servo 4</div>";
  page += "<div class='servo-buttons'>";
  page += "<button class='btn' onclick=\"sendCmd('servo4len')\">Lên</button>";
  page += "<button class='btn' onclick=\"sendCmd('servo4xuong')\">Xuống</button>";
  page += "</div>";
  page += "</div>";
  
  page += "<div class='servo-section'>";
  page += "<div class='section-title'>Điều khiển</div>";
  page += "<div class='servo-buttons'>";
  page += "<button class='btn stop-btn' onclick=\"sendCmd('stopservo')\">Dừng Servo</button>";
  page += "<button class='btn' onclick=\"sendCmd('reset')\">Reset All</button>";
  page += "</div>";
  page += "</div>";
  
  page += "</div>";

  page += "<script>";
  
  page += "let isPressed = {};";
  page += "let speedValue = 0;";
  page += "let statusUpdateInterval;";
  
  page += "function updateSpeed(value) {";
  page += "  speedValue = value;";
  page += "  document.getElementById('speedValue').innerText = value;";
  page += "  fetch('/speed?value=' + value)";
  page += "    .then(response => response.text())";
  page += "    .then(data => console.log('Speed updated:', data))";
  page += "    .catch(error => console.error('Speed error:', error));";
  page += "}";
  
  page += "function sendCmd(cmd, isMovement = false) {";
  page += "  if (isMovement && isPressed[cmd]) return;";
  page += "  if (isMovement) isPressed[cmd] = true;";
  page += "  ";
  page += "  let url = '/' + cmd;";
  page += "  if (isMovement && speedValue > 0) {";
  page += "    fetch('/speed?value=' + speedValue).then(() => {";
  page += "      return fetch(url);";
  page += "    })";
  page += "    .then(response => response.text())";
  page += "    .then(data => {";
  page += "      document.getElementById('status').innerText = data;";
  page += "      if (cmd === 'stop') {";
  page += "        Object.keys(isPressed).forEach(key => isPressed[key] = false);";
  page += "      }";
  page += "    })";
  page += "    .catch(error => {";
  page += "      console.error('Error:', error);";
  page += "      document.getElementById('status').innerText = 'Lỗi kết nối';";
  page += "      if (cmd === 'stop') {";
  page += "        Object.keys(isPressed).forEach(key => isPressed[key] = false);";
  page += "      }";
  page += "    });";
  page += "  } else {";
  page += "    fetch(url)";
  page += "    .then(response => response.text())";
  page += "    .then(data => {";
  page += "      document.getElementById('status').innerText = data;";
  page += "      if (cmd === 'stop') {";
  page += "        Object.keys(isPressed).forEach(key => isPressed[key] = false);";
  page += "      }";
  page += "    })";
  page += "    .catch(error => {";
  page += "      console.error('Error:', error);";
  page += "      document.getElementById('status').innerText = 'Lỗi kết nối';";
  page += "      if (cmd === 'stop') {";
  page += "        Object.keys(isPressed).forEach(key => isPressed[key] = false);";
  page += "      }";
  page += "    });";
  page += "  }";
  page += "}";
  
  page += "function updateStatus() {";
  page += "  fetch('/status')";
  page += "    .then(response => response.text())";
  page += "    .then(data => {";
  page += "      if (!data.includes('Error')) {";
  page += "        let parts = data.split(',');";
  page += "        let statusText = 'Servo: ';";
  page += "        parts.forEach(part => {";
  page += "          if (part.includes(':')) {";
  page += "            statusText += part + ' ';";
  page += "          }";
  page += "        });";
  page += "        document.getElementById('status').innerText = statusText;";
  page += "      }";
  page += "    })";
  page += "    .catch(error => console.log('Status update failed'));";
  page += "}";
  
  page += "document.addEventListener('DOMContentLoaded', function() {";
  page += "  document.getElementById('speedRange').value = 0;";
  page += "  document.getElementById('speedValue').innerText = '0';";
  page += "  ";
  page += "  statusUpdateInterval = setInterval(updateStatus, 2000);";
  page += "  ";
  page += "  document.addEventListener('touchstart', function(e) {";
  page += "    e.preventDefault();";
  page += "  }, {passive: false});";
  page += "  ";
  page += "  document.addEventListener('touchmove', function(e) {";
  page += "    e.preventDefault();";
  page += "  }, {passive: false});";
  page += "});";
  
  page += "window.addEventListener('beforeunload', function() {";
  page += "  clearInterval(statusUpdateInterval);";
  page += "  sendCmd('stop');";
  page += "});";
  
  page += "</script>";
  page += "</body></html>";
  
  server.send(200, "text/html", page);
}

void loop() {
  server.handleClient();
  
  // Kiểm tra và detach servo định kỳ
  checkAndDetachServos();
  
  delay(10);
}
