#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// ===== KHAI BÁO SERVO =====
// Khởi tạo 4 đối tượng servo cho các chức năng khác nhau
Servo servoGap;    // GPIO24 - Servo điều khiển gắp (mở/đóng)
Servo servoTay;    // GPIO25 - Servo điều khiển tay (co/duỗi)
Servo servoXoay;   // GPIO26 - Servo xoay (trái/phải)
Servo servoKhac;   // GPIO27 - Servo phụ (lên/xuống)

// ===== VỊ TRÍ HIỆN TẠI CỦA CÁC SERVO =====
// Lưu trữ vị trí hiện tại để theo dõi và di chuyển mượt mà
int posGap = 90;     // Vị trí hiện tại servo gắp (0-180 độ)
int posTay = 90;     // Vị trí hiện tại servo tay
int posXoay = 90;    // Vị trí hiện tại servo xoay
int posKhac = 90;    // Vị trí hiện tại servo phụ

// ===== KHAI BÁO CHÂN ĐIỀU KHIỂN MOTOR =====
#define ENA 15       // PWM cho motor trái (Enable A)
#define ENB 5        // PWM cho motor phải (Enable B)
#define IN1 2        // Điều khiển chiều motor trái - chân 1
#define IN2 4        // Điều khiển chiều motor trái - chân 2
#define IN3 16       // Điều khiển chiều motor phải - chân 1
#define IN4 17       // Điều khiển chiều motor phải - chân 2

// ===== CẤU HÌNH PWM CHO MOTOR =====
#define PWM_CH1 0        // Kênh PWM cho motor trái
#define PWM_CH2 1        // Kênh PWM cho motor phải
#define PWM_FREQ 1000    // Tần số PWM 1kHz
#define PWM_RES 8        // Độ phân giải 8-bit (0-255)

// ===== BIẾN ĐIỀU KHIỂN AN TOÀN SERVO =====
bool servoMoving = false;                    // Cờ kiểm tra servo có đang di chuyển
unsigned long lastServoCommand = 0;          // Thời điểm lệnh servo cuối cùng
const unsigned long SERVO_DELAY = 100;      // Delay 100ms giữa các lệnh servo

// ===== BIẾN TỐC ĐỘ MOTOR =====
int tocdoxetrai = 0;    // Tốc độ motor trái (0-255)
int tocdoxephai = 0;    // Tốc độ motor phải (0-255)

// ===== CẤU HÌNH WIFI ACCESS POINT =====
const char* ssid = "Robocar WebServer ESP32";  // Tên WiFi hotspot
const char* pass = "Robocar@esp32";            // Mật khẩu WiFi
WebServer server(80);                          // Web server trên port 80

// ===== HÀM DI CHUYỂN SERVO AN TOÀN =====
/**
 * Di chuyển servo từ từ để tránh giật và xung đột
 * @param servo: Tham chiếu đến đối tượng servo
 * @param currentPos: Vị trí hiện tại (sẽ được cập nhật)
 * @param targetPos: Vị trí đích (0-180)
 * @param servoName: Tên servo để debug
 * @return: true nếu thành công, false nếu servo đang bận
 */
bool moveServo(Servo &servo, int &currentPos, int targetPos, String servoName) {
  // Kiểm tra nếu servo đang di chuyển hoặc chưa đủ thời gian delay
  if (servoMoving || (millis() - lastServoCommand < SERVO_DELAY)) {
    Serial.println("Servo busy, skipping command");
    return false;
  }

  // Đánh dấu servo đang di chuyển và ghi nhận thời gian
  servoMoving = true;
  lastServoCommand = millis();

  Serial.println("Moving " + servoName + " from " + String(currentPos) + " to " + String(targetPos));

  // Tính toán bước di chuyển (5 độ mỗi lần)
  int step = (targetPos > currentPos) ? 5 : -5;
  int pos = currentPos;

  // Di chuyển từ từ đến vị trí đích
  while (abs(pos - targetPos) > 5) {
    pos += step;
    servo.write(pos);
    delay(20);  // Delay nhỏ giữa mỗi bước để servo kịp di chuyển
    yield();    // Cho phép ESP32 xử lý các tác vụ WiFi khác
  }

  // Di chuyển đến vị trí cuối cùng
  servo.write(targetPos);
  currentPos = targetPos;

  delay(50);           // Delay để servo ổn định
  servoMoving = false; // Đánh dấu servo đã xong

  Serial.println(servoName + " moved to " + String(targetPos));
  return true;
}

// ===== HÀM DI CHUYỂN SERVO NHANH (KHẨN CẤP) =====
/**
 * Di chuyển servo ngay lập tức không cần kiểm tra
 * Dùng cho reset hoặc khẩn cấp
 */
void moveServoFast(Servo &servo, int &currentPos, int targetPos, String servoName) {
  servo.write(targetPos);
  currentPos = targetPos;
  Serial.println(servoName + " fast move to " + String(targetPos));
  delay(100);
}

void setup() {
  // ===== KHỞI TẠO SERIAL MONITOR =====
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== KHỞI TẠO HỆ THỐNG ===");

  // ===== TẮT WIFI TẠM THỜI =====
  // Tránh xung đột khi khởi tạo servo
  WiFi.mode(WIFI_OFF);
  delay(500);

  // ===== KHỞI TẠO CÁC SERVO =====
  Serial.println("Khởi tạo servo...");

  // Phân bổ timer PWM riêng biệt cho servo (tránh xung đột với motor)
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Cấu hình tần số PWM 50Hz cho servo (chuẩn)
  servoGap.setPeriodHertz(50);
  servoTay.setPeriodHertz(50);
  servoXoay.setPeriodHertz(50);
  servoKhac.setPeriodHertz(50);

  // Gắn servo vào các GPIO với pulse width 500-2500μs
  servoGap.attach(24, 500, 2500);
  delay(200);  // Delay giữa mỗi servo để tránh xung đột
  servoTay.attach(25, 500, 2500);
  delay(200);
  servoXoay.attach(26, 500, 2500);
  delay(200);
  servoKhac.attach(27, 500, 2500);
  delay(200);

  // ===== ĐẶT VỊ TRÍ BAN ĐẦU CHO TẤT CẢ SERVO =====
  Serial.println("Đặt vị trí ban đầu...");
  servoGap.write(90);   // Vị trí giữa
  delay(500);
  servoTay.write(90);
  delay(500);
  servoXoay.write(90);
  delay(500);
  servoKhac.write(90);
  delay(500);
  Serial.println("Servo sẵn sàng!");

  // ===== KHỞI TẠO CÁC CHÂN ĐIỀU KHIỂN MOTOR =====
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // ===== CẤU HÌNH PWM CHO MOTOR =====
  // Kênh PWM 0 cho motor trái (ENA)
  ledcSetup(PWM_CH1, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENA, PWM_CH1);
  // Kênh PWM 1 cho motor phải (ENB)
  ledcSetup(PWM_CH2, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENB, PWM_CH2);

  // ===== ĐẶT TRẠNG THÁI DỪNG BAN ĐẦU =====
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  // ===== KHỞI TẠO WIFI ACCESS POINT =====
  Serial.println("Khởi tạo WiFi...");
  WiFi.mode(WIFI_AP);                    // Chế độ Access Point
  WiFi.softAP(ssid, pass);               // Tạo hotspot
  delay(1000);

  // ===== CẤU HÌNH CÁC ROUTE CHO WEB SERVER =====
  
  // Route chính - hiển thị giao diện web
  server.on("/", handleRoot);

  // ===== CÁC ROUTE ĐIỀU KHIỂN SERVO =====
  
  // Servo gắp - mở (180 độ)
  server.on("/mogap", []() { 
    if (moveServo(servoGap, posGap, 180, "Gap")) {
      server.send(200, "text/plain", "Mở gắp OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");  // 503 = Service Unavailable
    }
  });

  // Servo gắp - đóng (0 độ)
  server.on("/donggap", []() { 
    if (moveServo(servoGap, posGap, 0, "Gap")) {
      server.send(200, "text/plain", "Đóng gắp OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  // Servo tay - co (0 độ)
  server.on("/co", []() { 
    if (moveServo(servoTay, posTay, 0, "Tay")) {
      server.send(200, "text/plain", "Co tay OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  // Servo tay - duỗi (180 độ)
  server.on("/duoi", []() { 
    if (moveServo(servoTay, posTay, 180, "Tay")) {
      server.send(200, "text/plain", "Duỗi tay OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  // Servo xoay - trái (0 độ)
  server.on("/xoaytrai", []() { 
    if (moveServo(servoXoay, posXoay, 0, "Xoay")) {
      server.send(200, "text/plain", "Xoay trái OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  // Servo xoay - phải (180 độ)
  server.on("/xoayphai", []() { 
    if (moveServo(servoXoay, posXoay, 180, "Xoay")) {
      server.send(200, "text/plain", "Xoay phải OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  // Servo phụ - lên (180 độ)
  server.on("/servo4len", []() { 
    if (moveServo(servoKhac, posKhac, 180, "Servo4")) {
      server.send(200, "text/plain", "Servo 4 lên OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  // Servo phụ - xuống (0 độ)
  server.on("/servo4xuong", []() { 
    if (moveServo(servoKhac, posKhac, 0, "Servo4")) {
      server.send(200, "text/plain", "Servo 4 xuống OK"); 
    } else {
      server.send(503, "text/plain", "Servo busy");
    }
  });

  // Reset tất cả servo về vị trí giữa (90 độ)
  server.on("/reset", []() { 
    Serial.println("Reset all servos");
    moveServoFast(servoGap, posGap, 90, "Gap");
    moveServoFast(servoTay, posTay, 90, "Tay");
    moveServoFast(servoXoay, posXoay, 90, "Xoay");
    moveServoFast(servoKhac, posKhac, 90, "Servo4");
    server.send(200, "text/plain", "Reset OK"); 
  });

  // Lấy trạng thái hiện tại của hệ thống
  server.on("/status", []() {
    String status = "Gap:" + String(posGap) + 
                   ",Tay:" + String(posTay) + 
                   ",Xoay:" + String(posXoay) + 
                   ",Servo4:" + String(posKhac) +
                   ",Moving:" + String(servoMoving) +
                   ",Speed:" + String(tocdoxetrai);
    server.send(200, "text/plain", status);
  });

  // ===== CÁC ROUTE ĐIỀU KHIỂN MOTOR =====
  
  // Di chuyển tiến (cả 2 motor quay cùng chiều)
  server.on("/tien", []() { 
    digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);  // Motor trái tiến
    digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);  // Motor phải tiến
    Serial.println("Đang tiến");
    server.send(200, "text/plain", "Tiến OK"); 
  });
  
  // Di chuyển lùi (cả 2 motor đảo chiều)
  server.on("/lui", []() { 
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);   // Motor trái lùi
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);   // Motor phải lùi
    Serial.println("Đang lùi");
    server.send(200, "text/plain", "Lùi OK"); 
  });
  
  // Rẽ trái (motor trái lùi, motor phải tiến)
  server.on("/trai", []() { 
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);   // Motor trái lùi
    digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);  // Motor phải tiến
    Serial.println("Đang rẽ trái");
    server.send(200, "text/plain", "Trái OK"); 
  });
  
  // Rẽ phải (motor trái tiến, motor phải lùi)
  server.on("/phai", []() { 
    digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);  // Motor trái tiến
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);   // Motor phải lùi
    Serial.println("Đang rẽ phải");
    server.send(200, "text/plain", "Phải OK"); 
  });
  
  // Dừng tất cả motor
  server.on("/stop", []() { 
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);    // Dừng motor trái
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);    // Dừng motor phải
    Serial.println("Đang dừng");
    server.send(200, "text/plain", "Dừng OK"); 
  });

  // ===== ROUTE ĐIỀU KHIỂN TỐC ĐỘ =====
  server.on("/speed", []() {
    if (server.hasArg("value")) {                      // Kiểm tra có tham số value
      int newspeed = server.arg("value").toInt();      // Chuyển đổi sang số nguyên
      newspeed = constrain(newspeed, 0, 255);          // Giới hạn 0-255
      tocdoxetrai = newspeed;                          // Cập nhật tốc độ
      tocdoxephai = newspeed;
      Serial.println("Tốc độ: " + String(newspeed));
      server.send(200, "text/plain", "Speed OK");
    } else {
      server.send(400, "text/plain", "Thiếu tham số tốc độ");  // 400 = Bad Request
    }
  });

  // ===== KHỞI ĐỘNG WEB SERVER =====
  server.begin();
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());  // In địa chỉ IP của Access Point
  Serial.println("=== HỆ THỐNG SẴN SÀNG ===");
}

// ===== HÀM TẠO GIAO DIỆN WEB =====
/**
 * Tạo trang HTML với giao diện điều khiển responsive
 * Bao gồm: điều khiển di chuyển, servo, tốc độ, trạng thái
 */
void handleRoot() {
  String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  
  // ===== META TAGS CHO MOBILE =====
  page += "<meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no, maximum-scale=1, minimum-scale=1, viewport-fit=cover'>";
  page += "<title>Robocar & Servo Controller</title>";
  page += "<style>";
  
  // ===== CSS STYLING =====
  
  // Reset và cơ bản
  page += "html, body { margin: 0; padding: 0; width: 100%; user-select: none; -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; touch-action: manipulation; }";
  
  // Background gradient và layout chính
  page += "body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #1a2a6c, #b21f1f, #fdbb2d); color: white; display: flex; flex-direction: column; align-items: center; text-align: center; min-height: 100vh; padding-bottom: 20px; }";
  
  // Tiêu đề
  page += "h1 { margin: 15px 0; text-shadow: 2px 2px 4px rgba(0,0,0,0.5); }";
  
  // Panel điều khiển chính
  page += ".control-panel { width: 90%; max-width: 800px; background: rgba(0,0,0,0.4); border-radius: 20px; padding: 20px; box-shadow: 0 10px 25px rgba(0,0,0,0.3); margin: 10px 0; }";
  
  // Thanh trạng thái sticky
  page += ".status-bar { position: sticky; top: 0; background: rgba(0,0,0,0.8); padding: 10px; border-radius: 10px; margin-bottom: 15px; width: 90%; max-width: 800px; z-index: 100; box-shadow: 0 4px 10px rgba(0,0,0,0.3); }";
  
  // Điều khiển tốc độ
  page += ".speed-control { margin: 15px 0; padding: 15px; background: rgba(0,0,0,0.3); border-radius: 15px; }";
  page += ".speed-slider { width: 90%; margin: 10px 0; -webkit-appearance: none; height: 15px; border-radius: 10px; background: rgba(255,255,255,0.2); outline: none; }";
  page += ".speed-slider::-webkit-slider-thumb { -webkit-appearance: none; width: 25px; height: 25px; border-radius: 50%; background: #2193b0; cursor: pointer; }";
  page += ".speed-value { font-size: 24px; font-weight: bold; margin: 5px 0; }";
  
  // Layout điều khiển di chuyển
  page += ".movement-controls { display: flex; flex-direction: column; align-items: center; margin: 15px 0; }";
  page += ".direction-controls { display: flex; justify-content: space-between; width: 100%; margin: 10px 0; }";
  page += ".vertical-controls, .horizontal-controls { display: flex; flex-direction: column; align-items: center; }";
  page += ".horizontal-controls { flex-direction: row; justify-content: center; gap: 20px; }";
  
  // Styling nút bấm
  page += ".btn { padding: 15px 0; margin: 5px; font-size: 18px; border: none; border-radius: 50px; cursor: pointer; background: #2193b0; color: white; box-shadow: 0 4px 15px rgba(0,0,0,0.2); transition: all 0.3s ease; width: 100%; max-width: 150px; }";
  page += ".btn:active, .btn.pressed { transform: scale(0.95); background: #1c7a94; box-shadow: 0 2px 10px rgba(0,0,0,0.3); }";
  page += ".btn:disabled { background: #cccccc; cursor: not-allowed; }";
  page += ".direction-btn { width: 60px; height: 60px; border-radius: 50%; display: flex; justify-content: center; align-items: center; font-size: 24px; padding: 0; }";
  
  // Điều khiển servo
  page += ".servo-controls { margin: 15px 0; }";
  page += ".servo-section { margin: 15px 0; padding: 10px; background: rgba(0,0,0,0.3); border-radius: 15px; }";
  page += ".section-title { margin: 5px 0 10px 0; font-size: 18px; font-weight: bold; }";
  page += ".servo-buttons { display: flex; justify-content: center; gap: 10px; flex-wrap: wrap; }";
  
  // ===== RESPONSIVE DESIGN =====
  
  // Mobile portrait
  page += "@media (max-width: 768px) {";
  page += "  .btn { padding: 12px 0; font-size: 16px; }";
  page += "  .direction-btn { width: 50px; height: 50px; }";
  page += "  h1 { font-size: 24px; }";
  page += "  .direction-controls { flex-direction: column; }";
  page += "  .vertical-controls { margin-bottom: 20px; }";
  page += "}";
  
  // Landscape mode
  page += "@media (orientation: landscape) {";
  page += "  .direction-controls { flex-direction: row; }";
  page += "  .vertical-controls, .horizontal-controls { margin: 0 20px; }";
  page += "  .servo-buttons { flex-direction: row; }";
  page += "}";
  
  page += "</style></head><body>";

  // ===== THANH TRẠNG THÁI =====
  page += "<div class='status-bar'>";
  page += "<h1>Robocar & Servo Controller</h1>";
  page += "<div id='status'>Sẵn sàng</div>";
  page += "</div>";
  
  // ===== PANEL ĐIỀU KHIỂN DI CHUYỂN =====
  page += "<div class='control-panel'>";
  
  // Điều khiển tốc độ
  page += "<div class='speed-control'>";
  page += "<div>Tốc độ: <span class='speed-value' id='speedValue'>0</span></div>";
  page += "<input type='range' class='speed-slider' id='speedRange' min='0' max='255' value='0' oninput='updateSpeed(this.value)'>";
  page += "</div>";
  
  // Điều khiển di chuyển
  page += "<div class='movement-controls'>";
  page += "<h2>Điều khiển di chuyển</h2>";
  page += "<div class='direction-controls'>";
  
  // Nút tiến/lùi (dọc)
  page += "<div class='vertical-controls'>";
  page += "<button class='btn direction-btn' id='tien' onmousedown=\"sendCmd('tien', true)\" onmouseup=\"sendCmd('stop', true)\" ontouchstart=\"sendCmd('tien', true)\" ontouchend=\"sendCmd('stop', true)\">▲</button>";
  page += "<div style='margin: 10px 0;'>Tiến/Lùi</div>";
  page += "<button class='btn direction-btn' id='lui' onmousedown=\"sendCmd('lui', true)\" onmouseup=\"sendCmd('stop', true)\" ontouchstart=\"sendCmd('lui', true)\" ontouchend=\"sendCmd('stop', true)\">▼</button>";
  page += "</div>";
  
  // Nút trái/phải (ngang)
  page += "<div class='vertical-controls'>";
  page += "<div class='horizontal-controls'>";
  page += "<button class='btn direction-btn' id='trai' onmousedown=\"sendCmd('trai', true)\" onmouseup=\"sendCmd('stop', true)\" ontouchstart=\"sendCmd('trai', true)\" ontouchend=\"sendCmd('stop', true)\">◀</button>";
  page += "<button class='btn direction-btn' id='phai' onmousedown=\"sendCmd('phai', true)\" onmouseup=\"sendCmd('stop', true)\" ontouchstart=\"sendCmd('phai', true)\" ontouchend=\"sendCmd('stop', true)\">▶</button>";
  page += "</div>";
  page += "<div style='margin: 10px 0;'>Trái/Phải</div>";
  page += "</div>";
  
  page += "</div>"; // End direction-controls
  page += "</div>"; // End movement-controls
  page += "</div>"; // End control-panel
  
  // ===== PANEL ĐIỀU KHIỂN SERVO =====
  page += "<div class='control-panel servo-controls'>";
  page += "<h2>Điều khiển Servo</h2>";
  
  // Servo gắp
  page += "<div class='servo-section'>";
  page += "<div class='section-title'>Servo Gắp</div>";
  page += "<div class='servo-buttons'>";
  page += "<button class='btn' id='mogap' onclick=\"sendServoCmd('mogap')\">Mở Gắp</button>";
  page += "<button class='btn' id='donggap' onclick=\"sendServoCmd('donggap')\">Đóng Gắp</button>";
  page += "</div>";
  page += "</div>";
  
  // Servo tay
  page += "<div class='servo-section'>";
  page += "<div class='section-title'>Servo Tay</div>";
  page += "<div class='servo-buttons'>";
  page += "<button class='btn' id='co' onclick=\"sendServoCmd('co')\">Co Tay</button>";
  page += "<button class='btn' id='duoi' onclick=\"sendServoCmd('duoi')\">Duỗi Tay</button>";
  page += "</div>";
  page += "</div>";
  
    // Servo xoay
  page += "<div class='servo-section'>";
  page += "<div class='section-title'>Servo Xoay</div>";
  page += "<div class='servo-buttons'>";
  page += "<button class='btn' id='xoaytrai' onclick=\"sendServoCmd('xoaytrai')\">Xoay Trái</button>";
  page += "<button class='btn' id='xoayphai' onclick=\"sendServoCmd('xoayphai')\">Xoay Phải</button>";
  page += "</div>";
  page += "</div>";
  
  // ===== SERVO 4 (SERVO PHỤ) =====
  page += "<div class='servo-section'>";
  page += "<div class='section-title'>Servo 4</div>";
  page += "<div class='servo-buttons'>";
  page += "<button class='btn' id='servo4len' onclick=\"sendServoCmd('servo4len')\">Lên</button>";
  page += "<button class='btn' id='servo4xuong' onclick=\"sendServoCmd('servo4xuong')\">Xuống</button>";
  page += "</div>";
  page += "</div>";
  
  // ===== CÁC NÚT ĐIỀU KHIỂN CHUNG =====
  page += "<div class='servo-section'>";
  // Nút reset tất cả servo về vị trí giữa (màu đỏ để cảnh báo)
  page += "<button class='btn' onclick=\"sendCmd('reset')\" style='background: #ff6b6b;'>Reset All Servos</button>";
  // Nút kiểm tra trạng thái hệ thống (màu xanh)
  page += "<button class='btn' onclick=\"getStatus()\" style='background: #4ecdc4;'>Trạng thái</button>";
  page += "</div>";
  
  page += "</div>"; // End servo-controls panel

  // ===== JAVASCRIPT CODE =====
  page += "<script>";
  
  // ===== BIẾN TOÀN CỤC =====
  page += "let activeButton = null;";  // Theo dõi nút đang được nhấn
  // Danh sách tất cả các nút servo để quản lý trạng thái
  page += "let servoButtons = ['mogap', 'donggap', 'co', 'duoi', 'xoaytrai', 'xoayphai', 'servo4len', 'servo4xuong'];";

  // ===== HÀM VÔ HIỆU HÓA TẤT CẢ NÚT SERVO =====
  // Dùng khi servo đang di chuyển để tránh xung đột
  page += "function disableServoButtons() {";
  page += "  servoButtons.forEach(id => {";
  page += "    let btn = document.getElementById(id);";
  page += "    if (btn) btn.disabled = true;";  // Vô hiệu hóa nút
  page += "  });";
  page += "}";

  // ===== HÀM KÍCH HOẠT LẠI TẤT CẢ NÚT SERVO =====
  page += "function enableServoButtons() {";
  page += "  servoButtons.forEach(id => {";
  page += "    let btn = document.getElementById(id);";
  page += "    if (btn) btn.disabled = false;";  // Kích hoạt lại nút
  page += "  });";
  page += "}";
  
  // ===== HÀM CẬP NHẬT TỐC ĐỘ =====
  // Được gọi khi người dùng kéo thanh trượt tốc độ
  page += "function updateSpeed(val) {";
  page += "  document.getElementById('speedValue').innerText = val;";  // Hiển thị giá trị
  page += "  fetch('/speed?value=' + val);";  // Gửi request cập nhật tốc độ
  page += "}";
  
  // ===== HÀM GỬI LỆNH ĐIỀU KHIỂN DI CHUYỂN =====
  page += "function sendCmd(cmd, updateStatus = false) {";
  // Xử lý hiệu ứng visual cho các nút di chuyển
  page += "  if (cmd !== 'stop' && updateStatus) {";
  page += "    if (activeButton) activeButton.classList.remove('pressed');";  // Bỏ hiệu ứng nút cũ
  page += "    activeButton = document.getElementById(cmd);";
  page += "    if (activeButton) activeButton.classList.add('pressed');";     // Thêm hiệu ứng nút mới
  page += "    document.getElementById('status').innerText = 'Đang ' + getStatusText(cmd);";
  page += "  } else if (cmd === 'stop') {";
  // Xử lý khi dừng
  page += "    if (activeButton) activeButton.classList.remove('pressed');";
  page += "    activeButton = null;";
  page += "    document.getElementById('status').innerText = 'Đang dừng';";
  page += "  }";
  // Gửi HTTP request đến ESP32
  page += "  fetch('/' + cmd)";
  page += "    .then(response => response.text())";
  page += "    .catch(error => console.error('Error:', error));";
  page += "}";

  // ===== HÀM GỬI LỆNH ĐIỀU KHIỂN SERVO =====
  page += "function sendServoCmd(cmd) {";
  page += "  disableServoButtons();";  // Vô hiệu hóa tất cả nút servo
  page += "  document.getElementById('status').innerText = 'Servo đang di chuyển...';";
  page += "  fetch('/' + cmd)";  // Gửi lệnh servo
  page += "    .then(response => response.text())";
  page += "    .then(data => {";
  page += "      document.getElementById('status').innerText = data;";  // Hiển thị kết quả
  page += "      setTimeout(enableServoButtons, 500);";  // Kích hoạt lại sau 500ms
  page += "    })";
  page += "    .catch(error => {";
  page += "      document.getElementById('status').innerText = 'Lỗi: ' + error;";
  page += "      enableServoButtons();";  // Kích hoạt lại ngay nếu có lỗi
  page += "    });";
  page += "}";
  
  // ===== HÀM CHUYỂN ĐỔI LỆNH THÀNH TEXT HIỂN THỊ =====
  page += "function getStatusText(cmd) {";
  page += "  const statusMap = {";
  page += "    'tien': 'tiến tới',";
  page += "    'lui': 'lùi lại',";
  page += "    'trai': 'rẽ trái',";
  page += "    'phai': 'rẽ phải'";
  page += "  };";
  page += "  return statusMap[cmd] || cmd;";  // Trả về text tương ứng hoặc cmd gốc
  page += "}";

  // ===== HÀM LẤY TRẠNG THÁI HỆ THỐNG =====
  page += "function getStatus() {";
  page += "  fetch('/status')";  // Gửi request lấy trạng thái
  page += "    .then(response => response.text())";
  page += "    .then(data => {";
  page += "      document.getElementById('status').innerText = 'Trạng thái: ' + data;";
  page += "    });";
  page += "}";
  
  // ===== NGĂN CÁC HÀNH VI KHÔNG MONG MUỐN =====
  // Vô hiệu hóa menu chuột phải trên mobile
  page += "document.addEventListener('contextmenu', function(e) { e.preventDefault(); });";
  
  // ===== TỰ ĐỘNG CẬP NHẬT TRẠNG THÁI =====
  // Cập nhật trạng thái mỗi 5 giây
  page += "setInterval(getStatus, 5000);";
  
  page += "</script>";
  page += "</body></html>";

  // Gửi trang HTML hoàn chỉnh về client
  server.send(200, "text/html", page);
}

// ===== VÒNG LẶP CHÍNH =====
void loop() {
  // ===== CẬP NHẬT TỐC ĐỘ MOTOR =====
  // Áp dụng tốc độ hiện tại cho cả 2 motor qua PWM
  ledcWrite(PWM_CH1, tocdoxetrai);   // Motor trái
  ledcWrite(PWM_CH2, tocdoxephai);   // Motor phải
  
  // ===== XỬ LÝ CÁC REQUEST HTTP =====
  server.handleClient();  // Xử lý các request từ web client
  yield();               // Cho phép ESP32 xử lý WiFi và các tác vụ khác

  // ===== KIỂM TRA VÀ RESET SERVO NẾU CẦN =====
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 10000) {  // Kiểm tra mỗi 10 giây
    // Nếu servo bị kẹt quá 5 giây thì reset
    if (servoMoving && (millis() - lastServoCommand > 5000)) {
      Serial.println("Servo timeout, resetting...");
      servoMoving = false;  // Reset cờ servo
    }
    lastCheck = millis();
  }
}