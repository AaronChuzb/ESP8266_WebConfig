#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h> //闪存文件系统
#include <ArduinoJson.h>//json数据处理库（第三方）

ESP8266WebServer server(80); //创建Web服务端口为80

void setup() {
  pinMode(0 , INPUT_PULLUP);//板子上的FLASH按键，我的nodemcu是0
  Serial.begin(115200);
  Serial.println("");
  if (SPIFFS.begin()) { // 打开闪存文件系统，记得在你连接板子下载的过程中选Flash Size的时候不要选no SPIFFS,你可以选1M、2M、3M都无所谓，因为两个文件都很小  
      Serial.println("闪存文件系统打开成功");
      SPIFFS.format();
  } else {
    Serial.println("闪存文件系统打开失败");  
  }
  if(SPIFFS.exists("/config.json")){ // 判断有没有config.json这个文件
    Serial.println("存在配置信息，正在自动连接");
    const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 60; //分配一个内存空间
    DynamicJsonDocument doc(capacity);// 声明json处理对象
    File configJson = SPIFFS.open("/config.json", "r");
    deserializeJson(doc, configJson); // json数据序列化
    const char* ssid = doc["ssid"];
    const char* password = doc["password"];
    WiFi.mode(WIFI_STA); // 更换wifi模式
    WiFi.begin(ssid, password); // 连接wifi
    while(WiFi.status()!= WL_CONNECTED){
      delay(500);
      Serial.println(".");  
    }
    configJson.close();
    Serial.println(WiFi.localIP());
  } else {
     Serial.println("不存在配置信息，正在打开web配网模式"); 
     IPAddress softLocal(192,168,1,1);
     IPAddress softGateway(192,168,1,1);
     IPAddress softSubnet(255,255,255,0);
     WiFi.softAPConfig(softLocal, softGateway, softSubnet);
     WiFi.softAP("esp8266", "12345678"); //这里是配网模式下热点的名字和密码
     Serial.println(WiFi.softAPIP());
  }
  server.on("/", handleRoot);//web首页监听
  server.on("/set", handleConnect); // 配置ssid密码监听，感觉跟express的路由好像
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  if(digitalRead(0)==0){ //按键扫描
      removeConfig();
  }
}


void handleRoot() { //展示网页的关键代码
  if(SPIFFS.exists("/index.html")){
      File index = SPIFFS.open("/index.html", "r");
      server.streamFile(index, "text/html");
      index.close();
  }  
}

void handleConnect() { //处理配置信息的函数
  String ssid = server.arg("ssid");   //arg是获取请求参数，视频中最后面展示了请求的完整链接
  String password = server.arg("password"); 
  server.send(200, "text/plain", "OK");
  WiFi.mode(WIFI_STA); //改变wifi模式
  WiFi.begin(ssid.c_str(), password.c_str());//String类型直接用会报错，不要问为什么，转成char *就行了。
  while(WiFi.status()!= WL_CONNECTED){
     delay(500);
     Serial.println(".");  
  }
  Serial.println(WiFi.localIP());
  removeConfig(); // 不管有没有配置先删除一次再说。
  String payload; // 拼接构造一段字符串形式的json数据长{"ssid":"xxxxx","password":"xxxxxxxxxxx"}
  payload += "{\"ssid\":\"";
  payload += ssid;
  payload +="\",\"password\":\"";
  payload += password;
  payload += "\"}";
  File wifiConfig = SPIFFS.open("/config.json", "w");
  wifiConfig.println(payload);//将数据写入config.json文件中
  wifiConfig.close();
}

void removeConfig(){
    if(SPIFFS.exists("/config.json")){ // 判断有没有config.json这个文件
      if (SPIFFS.remove("/config.json")){
         Serial.println("删除旧配置");
      } else {
        Serial.println("删除旧配置失败");
      } 
    }
}
