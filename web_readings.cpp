/*
  Project Name:   Climatron - Your personal air quality monitoring robot
  Description:    Serve current sensor readings as a web page on the local network,
                  reachable at http://climatron.local (or the device IP address)
*/

#include "Arduino.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Measure.hpp>

#include "config.h"     // hardware and internet configuration parameters
#include "climatron.h"  // Climatron main header

#ifdef WEB_READINGS
  // Shared helper function(s)
  extern void debugMessage(String messageText, uint8_t messageLevel);
  extern float pm25toAQI_US(float pm25);

  extern Measure<kSampleCapacity> totalTemperatureF, totalHumidity, totalCO2, totalVOCIndex, totalPM25, totalNOxIndex;

  WebServer webReadingsServer(webReadingsPort);
  bool webReadingsRunning = false;

  // Single page; JavaScript polls /data.json and fills in the values
  const char webReadingsPage[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Climatron</title>
<style>
:root{--bg:#f4f4f2;--card:#fff;--text:#1d1d1b;--muted:#6b6b66;--line:#ccc}
@media (prefers-color-scheme:dark){:root{--bg:#141414;--card:#1f1f1f;--text:#eee;--muted:#999;--line:#555}}
body{margin:0;padding:16px;background:var(--bg);color:var(--text);font-family:-apple-system,system-ui,sans-serif}
h1{font-size:1.4em;margin:0 0 4px}
#meta{color:var(--muted);font-size:.9em;margin-bottom:16px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(280px,1fr));gap:12px}
.card{background:var(--card);border-radius:10px;padding:12px 14px;border-left:6px solid var(--line)}
.head{display:flex;justify-content:space-between;align-items:baseline}
.name{color:var(--muted);font-size:.9em}
.lbl{font-size:.8em;font-weight:600}
.val{font-size:2em;font-weight:600;margin:2px 0 6px}
.unit{font-size:.5em;font-weight:400;color:var(--muted)}
svg{width:100%;height:auto;display:block;overflow:visible}
svg text{fill:var(--muted);font-size:11px}
#device{margin-top:24px}#device h2{font-size:1.1em;margin:0 0 8px}
#device table{border-collapse:collapse;background:var(--card);border-radius:10px;width:100%;max-width:600px}
#device td{padding:5px 12px;border-bottom:1px solid var(--bg);font-size:.9em;word-break:break-word}#device td:first-child{color:var(--muted);white-space:nowrap}
.b0{border-left-color:#2e9e44}.b1{border-left-color:#e2c11b}.b2{border-left-color:#f08c00}.b3{border-left-color:#d23a2a}
</style></head><body>
<h1 id="title">Climatron</h1><div id="meta">Loading…</div>
<div class="grid" id="grid"></div>
<div id="device"><h2>Device</h2><table id="devtab"></table></div>
<script>
const COL=["#2e9e44","#e2c11b","#f08c00","#d23a2a"],LBL=["Good","Fair","Poor","Bad"];
function esc(x){return String(x).replace(/[&<>"]/g,c=>"&#"+c.charCodeAt(0)+";")}
function dur(s){const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return(d?d+"d ":"")+h+"h "+m+"m"}
const items=[
 ["co2","CO₂","ppm",0],["pm25","PM2.5","µg/m³",1],["aqi","US AQI","",0],
 ["voc","VOC index","",0],["nox","NOx index","",0],["temperatureF","Temperature","°F",1],["humidity","Humidity","%",1]];
// Band index matching the device's co2Range()/pm25Range()/... and comfort zones
function band(k,v,t){const r=t[k];if(!r)return null;
 if(r.length==2)return(v<r[0]||v>r[1])?1:0;
 return v<=r[0]?0:v<=r[1]?1:v<=r[2]?2:3}
// Same scaling as screenHelperGraph(): pad range, round axis labels to multiples of 10
function graph(k,h,cap,t){if(!h||!h.length)return'<svg viewBox="0 0 300 120"><text x="150" y="60" text-anchor="middle">Awaiting samples</text></svg>';
 let mn=Math.min(...h),mx=Math.max(...h),r=mx-mn;if(r<10)r=50;const a=(mx+mn)/2;
 mx=10*Math.ceil((a+r)/10);mn=10*Math.floor((a-r)/10);
 const L=40,R=292,T=6,B=112,dx=(R-L-10)/(cap-1),off=cap-h.length;
 const pts=h.map((v,i)=>[L+10+(i+off)*dx,B-(v-mn)/(mx-mn)*(B-T)]);
 let s='<svg viewBox="0 0 300 120"><text x="'+(L-4)+'" y="'+(T+8)+'" text-anchor="end">'+mx+'</text><text x="'+(L-4)+'" y="'+B+'" text-anchor="end">'+mn+'</text>'+
  '<path d="M'+L+' '+T+'V'+B+'H'+R+'" fill="none" stroke="var(--line)"/>'+
  '<polyline fill="none" stroke="var(--text)" stroke-width="1.5" points="'+pts.map(p=>p[0].toFixed(1)+","+p[1].toFixed(1)).join(" ")+'"/>';
 pts.forEach((p,i)=>{const b=band(k,h[i],t);s+='<circle cx="'+p[0].toFixed(1)+'" cy="'+p[1].toFixed(1)+'" r="4.5" fill="'+(b==null?"var(--text)":COL[b])+'"><title>'+h[i]+'</title></circle>'});
 return s+'</svg>'}
async function load(){try{const d=await(await fetch("/data.json")).json();
 document.getElementById("title").textContent=d.deviceID;
 document.getElementById("meta").textContent=(d.simulated?"SIMULATED data · ":"")+"Updated "+d.ageSeconds+" s ago · last "+d.capacity+" samples · WiFi "+d.rssi+" dBm";
 document.getElementById("grid").innerHTML=items.map(([k,n,u,dp])=>{const v=d[k],b=band(k,v,d.thresholds);
  return'<div class="card '+(b==null?"":"b"+b)+'"><div class="head"><span class="name">'+n+'</span>'+(b==null?"":'<span class="lbl" style="color:'+COL[b]+'">'+LBL[b]+'</span>')+
  '</div><div class="val">'+v.toFixed(dp)+' <span class="unit">'+u+'</span></div>'+graph(k,d.history[k],d.capacity,d.thresholds)+'</div>'}).join("")
 const v=d.device;if(v)document.getElementById("devtab").innerHTML=[
  ["MAC address",v.mac],["Firmware build",v.build],["Arduino core / IDF",v.core+" / "+v.idf],["Chip",v.chip],
  ["IP address",v.ip],["WiFi network",v.ssid],["WiFi signal",d.rssi+" dBm"],["Uptime",dur(v.uptime)],
  ["Free heap",(v.freeHeap/1024).toFixed(1)+" KB (min "+(v.minHeap/1024).toFixed(1)+" KB)"],["Last reset",v.resetReason],
  ["Site / location / room",[v.site,v.location,v.room].filter(x=>x).join(" / ")||"–"],
  ["Latitude, longitude",v.latitude+", "+v.longitude],["Altitude",v.altitude+" m"],
  ["Sample / report interval",v.sampleS+" s / "+v.reportS+" s"],["Features",v.features],
  ["Settings portal (when open)","http://"+v.ip+":"+v.portalPort]
 ].map(([k,x])=>"<tr><td>"+k+"</td><td>"+esc(x)+"</td></tr>").join("")}
 catch(e){document.getElementById("meta").textContent="Cannot reach Climatron"}}
load();setInterval(load,15000);
</script></body></html>)rawliteral";

  extern uint32_t webReadingsLastSampleMS;

  String webReadingsHistory(Measure<kSampleCapacity>& measure)
  // Retained samples, oldest first, as a JSON array
  {
    String json = "[";
    uint16_t stored = measure.getStored();
    for (uint16_t i = kSampleCapacity - stored; i < kSampleCapacity; i++) {
      if (i != kSampleCapacity - stored) json += ",";
      json += String(measure.getMember(i), 1);
    }
    return json + "]";
  }

  String webReadingsAQIHistory()
  // US AQI for each retained PM2.5 sample, oldest first
  {
    String json = "[";
    uint16_t stored = totalPM25.getStored();
    for (uint16_t i = kSampleCapacity - stored; i < kSampleCapacity; i++) {
      if (i != kSampleCapacity - stored) json += ",";
      json += String(pm25toAQI_US(totalPM25.getMember(i)), 0);
    }
    return json + "]";
  }

  String webReadingsJSONString(const String& text)
  // Quote a string for JSON
  {
    String out = "\"";
    for (uint16_t i = 0; i < text.length(); i++) {
      char c = text[i];
      if (c == '"' || c == '\\') out += '\\';
      if ((uint8_t)c >= 0x20) out += c;
    }
    return out + "\"";
  }

  const char* webReadingsResetReason()
  {
    switch (esp_reset_reason()) {
      case ESP_RST_POWERON:   return "Power on";
      case ESP_RST_SW:        return "Software restart";
      case ESP_RST_PANIC:     return "Crash (panic)";
      case ESP_RST_INT_WDT:   return "Interrupt watchdog";
      case ESP_RST_TASK_WDT:  return "Task watchdog";
      case ESP_RST_WDT:       return "Other watchdog";
      case ESP_RST_DEEPSLEEP: return "Wake from deep sleep";
      case ESP_RST_BROWNOUT:  return "Brownout (low power)";
      case ESP_RST_EXT:       return "External reset";
      default:                return "Unknown";
    }
  }

  String webReadingsDevice()
  // Device status and non-secret configuration; never include passwords or API keys
  {
    String features = "Readings web page";
    #ifdef INFLUX
      features += ", InfluxDB";
    #endif
    #ifdef THINGSPEAK
      features += ", ThingSpeak";
    #endif
    #ifdef MQTT
      features += ", MQTT";
    #endif
    #ifdef HASSIO_MQTT
      features += ", Home Assistant";
    #endif
    #ifdef HARDWARE_SIMULATE
      features += ", Simulation mode";
    #endif
    #ifdef DEBUG
      features += ", Debug level " + String(DEBUG);
    #endif

    String json = "{";
    json += "\"mac\":" + webReadingsJSONString(WiFi.macAddress()) + ",";
    json += "\"build\":" + webReadingsJSONString(String(__DATE__) + " " + __TIME__) + ",";
    json += "\"core\":" + webReadingsJSONString(String(ESP_ARDUINO_VERSION_MAJOR) + "." + ESP_ARDUINO_VERSION_MINOR + "." + ESP_ARDUINO_VERSION_PATCH) + ",";
    json += "\"idf\":" + webReadingsJSONString(esp_get_idf_version()) + ",";
    json += "\"chip\":" + webReadingsJSONString(String(ESP.getChipModel()) + " rev " + ESP.getChipRevision() + ", " + ESP.getCpuFreqMHz() + " MHz") + ",";
    json += "\"ip\":" + webReadingsJSONString(WiFi.localIP().toString()) + ",";
    json += "\"ssid\":" + webReadingsJSONString(WiFi.SSID()) + ",";
    json += "\"uptime\":" + String((uint32_t)(esp_timer_get_time() / 1000000)) + ",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"minHeap\":" + String(ESP.getMinFreeHeap()) + ",";
    json += "\"resetReason\":" + webReadingsJSONString(webReadingsResetReason()) + ",";
    json += "\"site\":" + webReadingsJSONString(endpointPath.site) + ",";
    json += "\"location\":" + webReadingsJSONString(endpointPath.location) + ",";
    json += "\"room\":" + webReadingsJSONString(endpointPath.room) + ",";
    json += "\"latitude\":" + String(hardwareData.latitude, 4) + ",";
    json += "\"longitude\":" + String(hardwareData.longitude, 4) + ",";
    json += "\"altitude\":" + String(hardwareData.altitude) + ",";
    json += "\"sampleS\":" + String(timeSensorSampleMS / 1000) + ",";
    json += "\"reportS\":" + String(timeReportMS / 1000) + ",";
    json += "\"features\":" + webReadingsJSONString(features) + ",";
    json += "\"portalPort\":" + String(webPortalPort);
    return json + "}";
  }

  void webReadingsHandleData()
  {
    float pm25 = totalPM25.getCurrent();
    String json = "{";
    json += "\"deviceID\":\"" + endpointPath.deviceID + "\",";
    #ifdef HARDWARE_SIMULATE
      json += "\"simulated\":true,";
    #else
      json += "\"simulated\":false,";
    #endif
    json += "\"ageSeconds\":" + String((millis() - webReadingsLastSampleMS) / 1000) + ",";
    json += "\"rssi\":-" + String(hardwareData.rssi) + ",";
    json += "\"co2\":" + String(totalCO2.getCurrent(), 0) + ",";
    json += "\"pm25\":" + String(pm25, 1) + ",";
    json += "\"aqi\":" + String(pm25toAQI_US(pm25), 0) + ",";
    json += "\"voc\":" + String(totalVOCIndex.getCurrent(), 0) + ",";
    json += "\"nox\":" + String(totalNOxIndex.getCurrent(), 0) + ",";
    json += "\"temperatureF\":" + String(totalTemperatureF.getCurrent(), 1) + ",";
    json += "\"humidity\":" + String(totalHumidity.getCurrent(), 1) + ",";
    json += "\"device\":" + webReadingsDevice() + ",";
    json += "\"capacity\":" + String(kSampleCapacity) + ",";
    // warning band limits from config.h so the page colors match the device
    json += "\"thresholds\":{";
    json += "\"co2\":[" + String(sensorCO2Fair) + "," + sensorCO2Poor + "," + sensorCO2Bad + "],";
    json += "\"pm25\":[" + String(sensorPMFair) + "," + sensorPMPoor + "," + sensorPMBad + "],";
    json += "\"voc\":[" + String(sensorVOCFair) + "," + sensorVOCPoor + "," + sensorVOCBad + "],";
    json += "\"nox\":[" + String(sensorNOxFair) + "," + sensorNOxPoor + "," + sensorNOxBad + "],";
    json += "\"aqi\":[50,100,150],";
    json += "\"temperatureF\":[" + String(sensorTempFComfortMin) + "," + sensorTempFComfortMax + "],";
    json += "\"humidity\":[" + String(sensorHumidityComfortMin) + "," + sensorHumidityComfortMax + "]},";
    json += "\"history\":{";
    json += "\"co2\":" + webReadingsHistory(totalCO2) + ",";
    json += "\"pm25\":" + webReadingsHistory(totalPM25) + ",";
    json += "\"aqi\":" + webReadingsAQIHistory() + ",";
    json += "\"voc\":" + webReadingsHistory(totalVOCIndex) + ",";
    json += "\"nox\":" + webReadingsHistory(totalNOxIndex) + ",";
    json += "\"temperatureF\":" + webReadingsHistory(totalTemperatureF) + ",";
    json += "\"humidity\":" + webReadingsHistory(totalHumidity);
    json += "}}";
    webReadingsServer.sendHeader("Cache-Control", "no-store");
    webReadingsServer.send(200, "application/json", json);
  }

  void webReadingsBegin()
  // Start the readings web server; call once WiFi is connected
  {
    if (webReadingsRunning || WiFi.status() != WL_CONNECTED) return;

    if (MDNS.begin(webReadingsHostname)) {
      MDNS.addService("http", "tcp", webReadingsPort);
    }
    else {
      debugMessage("webReadingsBegin(): mDNS responder failed to start", 1);
    }

    webReadingsServer.on("/", []() {
      webReadingsServer.sendHeader("Cache-Control", "no-cache");  // pick up page changes after a reflash
      webReadingsServer.send_P(200, "text/html", webReadingsPage);
    });
    webReadingsServer.on("/data.json", webReadingsHandleData);
    webReadingsServer.onNotFound([]() { webReadingsServer.send(404, "text/plain", "Not found"); });
    webReadingsServer.begin();
    webReadingsRunning = true;
    debugMessage(String("Readings web page at http://") + webReadingsHostname + ".local or http://" + WiFi.localIP().toString(), 1);
  }

  void webReadingsHandle()
  // Feed cycles to the readings web server; call from loop()
  {
    if (webReadingsRunning) {
      webReadingsServer.handleClient();
    }
    else {
      // WiFi may connect after setup(), e.g. following a reconnect
      webReadingsBegin();
    }
  }
#endif
