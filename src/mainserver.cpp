#include "mainserver.h"
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>

bool led1_state = false;
bool led2_state = false;
bool isAPMode = true;

// Access control config
const char *kAuthorizedId = "2210000"; // ID da dang ky truoc
const char *kSessionCookie = "session=1";
bool isAuthenticated = false;

// Door/relay config
const uint8_t DOOR_RELAY_PIN = 17;
const uint8_t DOOR_LOCK_LEVEL = LOW; // Doi thanh HIGH neu relay active-low nguoc lai
const uint8_t DOOR_UNLOCK_LEVEL = HIGH;
bool door_locked = true;

// Last known sensor values for UI fallback
float last_temperature = 0;
float last_humidity = 0;

WebServer server(80);

String ssid = "MY-ESP32-NETWORK";
String password = "12345678";
String wifi_ssid = "";
String wifi_password = "";

unsigned long connect_start_ms = 0;
bool connecting = false;

// ========== Helpers ==========
bool hasValidSession()
{
  if (isAuthenticated)
    return true;

  if (server.hasHeader("Cookie"))
  {
    String cookie = server.header("Cookie");
    if (cookie.indexOf(kSessionCookie) >= 0)
    {
      isAuthenticated = true;
      return true;
    }
  }

  return false;
}

bool requireAuth()
{
  if (hasValidSession())
    return true;

  server.sendHeader("Location", "/login");
  server.send(302, "text/plain", "Redirecting to login");
  return false;
}

void applyDoorState()
{
  digitalWrite(DOOR_RELAY_PIN, door_locked ? DOOR_LOCK_LEVEL : DOOR_UNLOCK_LEVEL);
}

String doorStateLabel()
{
  return door_locked ? "LOCKED" : "UNLOCKED";
}

bool initFS()
{
  if (SPIFFS.begin(false))
    return true;
  return SPIFFS.begin(true);
}

// ========== Pages ==========
String mainPage()
{
  if (glob_temperature != -1)
    last_temperature = glob_temperature;
  if (glob_humidity != -1)
    last_humidity = glob_humidity;

  float temperature = last_temperature;
  float humidity = last_humidity;
  String led1 = led1_state ? "ON" : "OFF";
  String led2 = led2_state ? "ON" : "OFF";
  String door = doorStateLabel();

  return R"rawliteral(
    <!DOCTYPE html><html><head>
      <meta charset='UTF-8'>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <title>Access Dashboard</title>
      <style>
        :root {
          --bg:#eef1f7;
          --card:#ffffff;
          --accent:#2d6cdf;
          --accent-2:#0f9d58;
          --danger:#e84a5f;
          --text:#1f2a3d;
          --muted:#6b7380;
        }
        * { box-sizing: border-box; }
        body { font-family: "Segoe UI", Arial, sans-serif; background:radial-gradient(circle at 20% 20%, #f4f7fb, #e7ecf5); margin:0; color:var(--text); }
        header { display:flex; align-items:center; gap:12px; padding:16px 18px; position:sticky; top:0; background:rgba(255,255,255,0.95); backdrop-filter:blur(8px); box-shadow:0 2px 10px rgba(0,0,0,0.05); z-index:10; }
        .logo { display:flex; align-items:center; gap:10px; font-weight:700; letter-spacing:0.4px; color:var(--text); }
        .logo .logoMark { color: var(--accent); font-weight:900; font-size:20px; letter-spacing:1px; }
        .shell { max-width:460px; margin:18px auto; padding:0 14px 24px; }
        .card { background:var(--card); border-radius:14px; box-shadow:0 8px 24px rgba(17,24,39,0.08); padding:18px; margin-top:14px; }
        h2 { margin:0; }
        .section { margin:14px 0; }
        .row { display:flex; justify-content:space-between; align-items:center; gap:10px; flex-wrap:wrap; }
        .pill { padding:4px 12px; border-radius:999px; background:var(--accent); color:#fff; font-size:13px; }
        button { padding:10px 14px; font-size:15px; border:none; border-radius:10px; cursor:pointer; transition:transform 120ms ease, box-shadow 120ms ease; }
        button:active { transform:scale(0.98); }
        .primary { background:var(--accent); color:#fff; box-shadow:0 8px 18px rgba(45,108,223,0.3); }
        .secondary { background:#f1f3f7; color:var(--text); }
        .danger { background:var(--danger); color:#fff; box-shadow:0 8px 18px rgba(232,74,95,0.28); }
        .metric { display:flex; justify-content:space-between; margin:6px 0; color:var(--muted); }
        .metric span:first-child { color:var(--text); font-weight:600; }
        .grid { display:grid; grid-template-columns:1fr 1fr; gap:10px; }
      </style>
    </head>
    <body>
      <header>
        <div class='logo'>
          <span class='logoMark'>HCMUT</span>
          <span>Room management system</span>
        </div>
        <div style='margin-left:auto'>
          <button class='secondary' onclick="window.location='/logout'">Logout</button>
        </div>
      </header>

      <div class='shell'>
        <div class='card'>
          <div class='section'>
            <h2>Dashboard</h2>
            <div class='metric'><span>Temperature</span><span id='temp'>)rawliteral" +
         String(temperature) + R"rawliteral( °C</span></div>
            <div class='metric'><span>Humidity</span><span id='hum'>)rawliteral" +
         String(humidity) + R"rawliteral( %</span></div>
          </div>

          <div class='section'>
            <h3>Controls</h3>
            <div class='grid'>
              <button class='primary' onclick='toggleLED(1)'>LED1: <span id="l1">)rawliteral" +
         led1 + R"rawliteral(</span></button>
              <button class='primary' onclick='toggleLED(2)'>LED2: <span id="l2">)rawliteral" +
         led2 + R"rawliteral(</span></button>
            </div>
          </div>

          <div class='section'>
            <h3>Door</h3>
            <div class='row'>
              <span class='pill' id='doorState'>)rawliteral" +
         door + R"rawliteral(</span>
              <button class='danger' style='flex:1' onclick='toggleDoor()'>Lock / Unlock Door</button>
            </div>
          </div>

          <div class='section'>
            <button class='secondary' style='width:100%' onclick="window.location='/settings'">Wi-Fi Settings</button>
          </div>
        </div>
      </div>

      <script>
        function toggleLED(id) {
          fetch('/toggle?led='+id)
          .then(response=>response.json())
          .then(json=>{
            document.getElementById('l1').innerText=json.led1;
            document.getElementById('l2').innerText=json.led2;
          });
        }

        function toggleDoor() {
          fetch('/door?action=toggle')
          .then(res=>res.json())
          .then(d=>{
            document.getElementById('doorState').innerText=d.state;
          });
        }

        setInterval(()=>{
          fetch('/sensors')
           .then(res=>res.json())
           .then(d=>{
             document.getElementById('temp').innerText=d.temp + ' °C';
             document.getElementById('hum').innerText=d.hum + ' %';
             if (d.door) document.getElementById('doorState').innerText=d.door;
           });
        },3000);
      </script>
    </body></html>
  )rawliteral";
}

String loginPage()
{
  return R"rawliteral(
    <!DOCTYPE html><html><head>
      <meta charset='UTF-8'>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <title>Access Login</title>
      <style>
        :root { --accent:#2d6cdf; --text:#1f2a3d; }
        * { box-sizing:border-box; }
        body { font-family:"Segoe UI", Arial, sans-serif; margin:0; min-height:100vh; display:flex; align-items:center; justify-content:center; background:linear-gradient(145deg, #e8eef8, #f8fbff); color:var(--text); }
        .card { width:340px; background:#fff; border-radius:14px; box-shadow:0 14px 36px rgba(17,24,39,0.12); padding:26px 22px; text-align:center; }
        .logo { display:flex; align-items:center; justify-content:center; gap:10px; margin-bottom:14px; color:var(--text); }
        .logo .logoMark { color: var(--accent); font-weight:900; font-size:24px; letter-spacing:1px; }
        h2 { margin:6px 0 4px 0; }
        p { margin:0 0 14px 0; color:#647086; }
        input { width:100%; padding:12px; margin:8px 0 12px 0; font-size:16px; border:1px solid #d7deea; border-radius:10px; outline:none; }
        input:focus { border-color:var(--accent); box-shadow:0 0 0 3px rgba(45,108,223,0.15); }
        button { width:100%; padding:12px; font-size:16px; background:var(--accent); color:white; border:none; border-radius:10px; cursor:pointer; box-shadow:0 10px 20px rgba(45,108,223,0.28); transition:transform 120ms ease; }
        button:active { transform:scale(0.985); }
      </style>
    </head>
    <body>
      <div class='card'>
        <div class='logo'>
          <span class='logoMark'>HCMUT</span>
          <span style='font-weight:700;'>Room management system</span>
        </div>
        <h2>Đăng nhập</h2>
        <p>Nhập ID đã được cấp để vào hệ thống.</p>
        <form method='POST' action='/login'>
          <input name='id' placeholder='Nhập ID (vd: 2210000)' required>
          <button type='submit'>Đăng nhập</button>
        </form>
      </div>
    </body></html>
  )rawliteral";
}

String settingsPage()
{
  return R"rawliteral(
    <!DOCTYPE html><html><head>
      <meta charset='UTF-8'>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <title>Settings</title>
      <style>
        :root { --accent:#2d6cdf; --text:#1f2a3d; --muted:#6b7380; }
        * { box-sizing: border-box; }
        body {
          font-family: "Segoe UI", Arial, sans-serif;
          margin: 0;
          min-height: 100vh;
          display: flex;
          align-items: center;
          justify-content: center;
          padding: 18px;
          background: linear-gradient(145deg, #e8eef8, #f8fbff);
          color: var(--text);
        }
        .card {
          width: 360px;
          background: #fff;
          border-radius: 14px;
          box-shadow: 0 14px 36px rgba(17,24,39,0.12);
          padding: 22px;
          text-align: left;
        }
        .logo { display:flex; align-items:center; gap:10px; margin-bottom:10px; }
        .logo .logoMark { color: var(--accent); font-weight: 900; font-size: 22px; letter-spacing: 1px; }
        h2 { margin: 8px 0 4px 0; font-size: 20px; }
        p { margin: 0 0 14px 0; color: var(--muted); font-size: 14px; line-height: 1.4; }
        label { display:block; text-align:left; font-size: 13px; color: var(--muted); margin: 10px 0 6px 2px; }
        input[type=text], input[type=password] {
          width: 100%;
          padding: 12px;
          font-size: 16px;
          border: 1px solid #d7deea;
          border-radius: 10px;
          outline: none;
        }
        input:focus { border-color: var(--accent); box-shadow: 0 0 0 3px rgba(45,108,223,0.15); }
        .actions { display:flex; gap:10px; margin-top: 14px; }
        button {
          flex: 1;
          padding: 12px;
          font-size: 16px;
          border: none;
          border-radius: 10px;
          cursor: pointer;
          transition: transform 120ms ease;
        }
        button:active { transform: scale(0.985); }
        .primary { background: var(--accent); color: white; box-shadow: 0 10px 20px rgba(45,108,223,0.28); }
        .secondary { background: #f1f3f7; color: var(--text); }
        #msg { margin-top: 12px; font-size: 14px; color: var(--muted); min-height: 18px; }
      </style>
    </head>
    <body>
      <div class='card'>
        <div class='logo'>
          <span class='logoMark'>HCMUT</span>
          <span style='font-weight:700;'>Room management system</span>
        </div>
        <h2>Wi-Fi Settings</h2>
        <p>Nhập SSID và mật khẩu để chuyển sang chế độ STA.</p>
        <form id="wifiForm">
          <label for='ssid'>SSID</label>
          <input name="ssid" id="ssid" placeholder="SSID" required>
          <label for='pass'>Password</label>
          <input name="password" id="pass" type="password" placeholder="Password" required>
          <div class='actions'>
            <button class='primary' type="submit">Connect</button>
            <button class='secondary' type="button" onclick="window.location='/'">Back</button>
          </div>
        </form>
        <div id="msg"></div>
      </div>
      <script>
        document.getElementById('wifiForm').onsubmit = function(e){
          e.preventDefault();
          let ssid = document.getElementById('ssid').value;
          let pass = document.getElementById('pass').value;
          fetch('/connect?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass))
            .then(r=>r.text())
            .then(msg=>{
              document.getElementById('msg').innerText=msg;
            });
        };
      </script>
    </body></html>
  )rawliteral";
}

// ========== Handlers ==========
void handleLoginPage()
{
  if (hasValidSession())
  {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
    return;
  }

  server.send(200, "text/html", loginPage());
}

void handleLoginSubmit()
{
  String id = server.arg("id");
  if (id == kAuthorizedId)
  {
    isAuthenticated = true;
    server.sendHeader("Set-Cookie", String(kSessionCookie) + "; Path=/");
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "Logged in");
  }
  else
  {
    server.send(401, "text/plain", "Sai ID, vui long thu lai");
  }
}

void handleLogout()
{
  isAuthenticated = false;
  server.sendHeader("Set-Cookie", "session=; Max-Age=0; Path=/");
  server.sendHeader("Location", "/login");
  server.send(302, "text/plain", "Logged out");
}

void handleRoot()
{
  if (!hasValidSession())
  {
    // Hiển thị trang đăng nhập trực tiếp để tránh bị chặn/caching redirect
    server.send(200, "text/html", loginPage());
    return;
  }

  server.send(200, "text/html", mainPage());
}

void handleToggle()
{
  if (!requireAuth())
    return;
  int led = server.arg("led").toInt();
  if (led == 1)
  {
    led1_state = !led1_state;
    Serial.println("YOUR CODE TO CONTROL LED1");
  }
  else if (led == 2)
  {
    led2_state = !led2_state;
    Serial.println("YOUR CODE TO CONTROL LED2");
  }
  server.send(200, "application/json",
              "{\"led1\":\"" + String(led1_state ? "ON" : "OFF") +
                  "\",\"led2\":\"" + String(led2_state ? "ON" : "OFF") + "\"}");
}

void handleSensors()
{
  if (!requireAuth())
    return;
  float t = glob_temperature;
  float h = glob_humidity;
  String json = "{\"temp\":" + String(t) + ",\"hum\":" + String(h) + ",\"door\":\"" + doorStateLabel() + "\"}";
  server.send(200, "application/json", json);
}

void handleSettings()
{
  if (!requireAuth())
    return;
  server.send(200, "text/html; charset=utf-8", settingsPage());
}

void handleDoor()
{
  if (!requireAuth())
    return;

  String action = server.arg("action");
  if (action == "unlock")
    door_locked = false;
  else if (action == "lock")
    door_locked = true;
  else if (action == "toggle")
    door_locked = !door_locked;

  applyDoorState();

  String json = "{\"state\":\"" + doorStateLabel() + "\"}";
  server.send(200, "application/json", json);
}


void handleConnect()
{
  if (!requireAuth())
    return;
  wifi_ssid = server.arg("ssid");
  wifi_password = server.arg("pass");
  server.send(200, "text/plain", "Connecting....");
  isAPMode = false;
  connecting = true;
  connect_start_ms = millis();
  connectToWiFi();
}

// ========== WiFi ==========
void setupServer()
{
  server.on("/login", HTTP_GET, handleLoginPage);
  server.on("/login", HTTP_POST, handleLoginSubmit);
  server.on("/logout", HTTP_GET, handleLogout);
  server.onNotFound([]()
                    { server.sendHeader("Location", "/login"); server.send(302, "text/plain", ""); });

  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/sensors", HTTP_GET, handleSensors);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/connect", HTTP_GET, handleConnect);
  server.on("/door", HTTP_GET, handleDoor);
 
  const char *headerKeys[] = {"Cookie"};
  server.collectHeaders(headerKeys, 1);
  server.begin();
}

void startAP()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str(), password.c_str());
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  isAPMode = true;
  connecting = false;
  isAuthenticated = false;
}

void connectToWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  Serial.print("Connecting to ");
  Serial.print(wifi_ssid.c_str());
  Serial.print(wifi_password.c_str());
  Serial.println(wifi_ssid);
}

// ========== Main task ==========
void main_server_task(void *pvParameters)
{
  pinMode(BOOT_PIN, INPUT_PULLUP);
  pinMode(DOOR_RELAY_PIN, OUTPUT);
  applyDoorState();

  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS mount failed");
  }

  startAP();
  setupServer();

  while (1)
  {
    server.handleClient();

    // BOOT Button to switch to AP Mode
    if (digitalRead(BOOT_PIN) == LOW)
    {
      vTaskDelay(100);
      if (digitalRead(BOOT_PIN) == LOW)
      {
        if (!isAPMode)
        {
          startAP();
          setupServer();
        }
      }
    }

    // STA Mode
    if (connecting)
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.print("STA IP address: ");
        Serial.println(WiFi.localIP());
        isAPMode = false;
        connecting = false;
      }
      else if (millis() - connect_start_ms > 10000)
      { // timeout 10s
        Serial.println("WiFi connect failed! Back to AP.");
        startAP();
        setupServer();
        connecting = false;
      }
    }

    // Update state of D3 and NEO
    led_D3 = led1_state;
    led_NEO = led2_state;
    vTaskDelay(20); // avoid watchdog reset
  }
}
