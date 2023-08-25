#include <M5Stack.h>
#include <WiFi.h>
#include <esp_wps.h>
#include <HTTPClient.h>

//#define WPS
const char* ssid = "elecom-8aee23";
const char* password = "5tjjmkfj5y38";

class HttpData{
    public:
        HttpData(int size){
            this->data_size = size;
            this->data = (uint8_t*)malloc(sizeof(uint8_t)*size);
        }
        int write_buffer(uint8_t* buffer, int buffer_size){
            memcpy(data + read_size, buffer, buffer_size);
            read_size += buffer_size;

            if(bf_size_end < read_size && bf_size == 0){
                set_bf_size();
            }
            return 0;
        }

        int get_bmp_length(){
            return bmp_length;
        }

        bool display_bmp_data(){
            if(bf_size < 1){
                return false;
            }
            bmp_length = read_size - bf_size;
            M5.Lcd.printf("bmp_length = %d\n", bmp_length);
            delay(3000);
            M5.Lcd.clear();
            for(int i = 0; i < bmp_length; i++){
                data[i] = data[bf_size+i];
            }
            realloc(data, sizeof(uint8_t)*bmp_length);
            M5.Lcd.drawBitmap(0, 0, 320, 240, data);
            delay(10000);
            M5.Lcd.clear();
            M5.Lcd.println("complete");
            delay(3000);
            return true;
        }

        int end(){
            free(data);
            return 0;
        }

    private:
        int read_size = 0;
        int data_size;
        uint8_t* data;

        const int bf_size_begin = 0x0A;
        const int bf_size_end = 0x0D;
        int32_t bf_size = 0;

        int bmp_length = 0;

        void set_bf_size(void){
            for(int addr = bf_size_end; bf_size_begin<=addr; addr--){
                bf_size = bf_size << 8;
                bf_size |= data[addr];
            }
        }
};

static esp_wps_config_t wps_config = {
    WPS_TYPE_PBC,
    {
        "ESPRESSIF",
        "ESP32",
        "ESPRESSIF IOT",
        "ESP STATION"
    }
};

boolean start_wps(){
    if(esp_wifi_wps_enable(&wps_config)){
        return true;
    }
    else if(esp_wifi_wps_start(0)){
        return false;
    }
}

boolean stop_wps(){
    return esp_wifi_wps_disable();
}

String wpspin2string(uint8_t a[]){
  char wps_pin[9];
  for(int i=0;i<8;i++){
    wps_pin[i] = a[i];
  }
  wps_pin[8] = '\0';
  return (String)wps_pin;
}

void wifi_event(WiFiEvent_t event, arduino_event_info_t info){
    switch(event){
        case ARDUINO_EVENT_WIFI_STA_START:
            //M5.Lcd.println("Station Mode Started");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            M5.Lcd.println("Connected to :" + String(WiFi.SSID()) + ", IP = " + String(WiFi.localIP()));
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            M5.Lcd.println("WiFi Disconnected, Attempting Reconnection");
            WiFi.reconnect();
            break;
        case ARDUINO_EVENT_WPS_ER_SUCCESS:
            M5.Lcd.println("WPS Successfull, Stopping WPS And Connecting To: " + String(WiFi.SSID()));
            stop_wps();
            delay(10);
            WiFi.begin();
            break;
        case ARDUINO_EVENT_WPS_ER_FAILED:
            //M5.Lcd.println("WPS Failed, Retrying");
            stop_wps();
            start_wps();
            break;
        case ARDUINO_EVENT_WPS_ER_TIMEOUT:
            //M5.Lcd.println("WPS Timedout, Retrying");
            stop_wps();
            start_wps();
            break;
        case ARDUINO_EVENT_WPS_ER_PIN:
            //M5.Lcd.println("WPS_PIN = " + wpspin2string(info.wps_er_pin.pin_code));
            break;
        default:
            break;
    }
}

bool httpget(const char *url)
{
    uint8_t http_buff[4096];
    HTTPClient http;
    bool result = true;

    M5.Lcd.printf("[HTTP] begin...\n");
    http.begin(url);

    M5.Lcd.printf("[HTTP] GET %s\n", url);
    int httpCode = http.GET();
    M5.Lcd.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK)
    {
        WiFiClient *stream = http.getStreamPtr();
        int data_size = http.getSize();
        int read_size = 0;
        HttpData http_data = HttpData(data_size);

        M5.Lcd.printf("[HTTP] GET... size: %d\n", data_size);
        while (http.connected() && read_size < data_size)
        {
            int sz = stream->available();
            if (sz > 0)
            {
                int rd = stream->readBytes(http_buff, min(sz, sizeof(http_buff)));
                read_size += rd;
                http_data.write_buffer(http_buff, rd);
                //M5.Lcd.printf("[HTTP] read: %d/%d\n", read_size, data_size);
                //M5.Lcd.printf("[HTTP] data: %x\n", http_buff[0]);
            }
            else
            {
                delay(50);
            }
        }
        result = true;
        http_data.display_bmp_data();
        delay(3000);
        M5.Lcd.println("Flag2");
        M5.Lcd.printf("bmp length: %d\n", http_data.get_bmp_length());
        M5.Lcd.println("Flag1");
        delay(3000);
        M5.Lcd.println("Flag3");
        delay(3000);
        http_data.end();
    }
    else
    {
        M5.Lcd.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        result = false;
    }
    http.end();
    return result;
}

void setup(){
    M5.begin();
    M5.Lcd.begin();
    M5.Lcd.setTextSize(1);
    delay(7000);
    
    #ifdef WPS
        M5.Lcd.println();
        WiFi.onEvent(wifi_event);
        WiFi.mode(WIFI_MODE_STA);
        M5.Lcd.println("Starting WPS");
        start_wps();
    #else
        WiFi.begin(ssid, password);
    #endif

    while(!WiFi.isConnected()){
        delay(10);
    }

    #ifdef WPS
    #else
        M5.Lcd.println("Connected to :" + String(WiFi.SSID()) + ", IP = " + String(WiFi.localIP()));
    #endif
}

void loop(){
    bool result = httpget("http://192.168.2.104:8080/imagefiles/wallpaper/test_01.bmp");
    M5.Lcd.printf("Complete");
    delay(10000);
}