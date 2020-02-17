#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Zmienne służace do ustanowienia połączenia z WiFi, Bluetooth oraz serwerem MQTT.
SoftwareSerial BTserial(2, 0); // RX, TX
const long baudRate = 9600;
char lancuch[30];
char ssidWIFI[30];  //Tablica przechowująca nazwę SSID sieci WiFi.
char passWIFI[30];  //Tablica przechowująca hasło sieci WiFi.
int indeks;
char sigCh;
int alarm = 0;     //Sygnalizacja czy alarm jest włączony 1-jest 0-nie jest.

const char* mqttServer = "farmer.cloudmqtt.com";   //nazwa serwera MQTT.
const int mqttPort =  10069;                       //port serwera MQTT.
const char* mqttUser = "";                 //nazwe urzytkownika serwera MQTT.
const char* mqttPassword = "";         //hasło urzytkownika serwera MQTT.

WiFiClient espClient;
PubSubClient client(espClient);

int flag = 1;                                     //Flaga sterują switchem mówiąca na jakim etapie działania jest program
String siec = "";                                 //Nazwy sieci WiFi pobierane przy skanowaniu środowiska i wyświetlane w terminalu Bluetooth

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Czujnik zmienne
float czujnik_volt;                               //Wartośc napięcia na porcie analogowym, obliczona na podstawie danych z niego.
float RSRO;                                       // Pobiera stosunek Rezystancji Gazu do Rezystancji czystego powietrza (RS_GAS/RS_air).
int czujnik_Wartosc = 0;                          //Wartość czujnika odczytana na porcie analogowym.
int ppmCO;                                        //Zmienna przechowująca aktualne stęzenei CO w ppm
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Start programu
void setup() {
  Serial.begin(9600);     
  BTserial.begin(baudRate);
  pinMode(15, OUTPUT);   //złącze zyfrowe do które jest podpięty buzzer

}

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Wiadomo: "); 
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
  Serial.println("-----------------------");

}

void loop() {
  switch (flag) {
    case 1: {

        WiFi.mode(WIFI_STA);   //Włączenie modułu WiFi.
        WiFi.disconnect();     //Jeśli jest połączone z jakąś siecią to ma zostać rozłączone dla bezpieczeństwa.
        delay(100);
        BTserial.print("Skanowanie start ... ");
        int n = WiFi.scanNetworks();    //Ile jest dostepnych sieci.
        BTserial.print(n);              //Wyświetlenie liczby dostępnych sieci
        BTserial.println(" Znalezione sieci:");
        for (int i = 0; i < n; i++)
        { siec = (String)WiFi.SSID(i);  //Wypisanie dostepnych sieci po koleji
          BTserial.println(siec);
        }
        BTserial.println("---------------------------------");
        BTserial.println("");
        BTserial.println("Wprowadz SSID WiFi ");;

        flag = 2;       //Przechodzimy do części wyboru sieci
        break;
      }
    case 2: {    //Podajemy SSID do połaczenia się z WiFi
        indeks = 0;
        if (BTserial.available() > 0) {  //Sprawdzenie czy terminal BT jest aktywny

          delay(100);
          while (true) {                 //Zczytanie nazwy SSID podajnej w terminalu BT 
            sigCh = (char)BTserial.read();
            if (sigCh != '\n') {
              lancuch[indeks] = sigCh;
              indeks++;
            }

            if (sigCh == '\n')
              break;
          }

          if (lancuch[0] != '/0') {
            flag = 3;         //Przejście do podania hasłą do sieci WiFi
            strncpy(ssidWIFI, lancuch, indeks - 1); //Zapisujemy nazwę sieci WiFi
            BTserial.println("Wprowadz haslo do WiFi ");
          }
        }
        break;
      }
    case 3: { //Podajemy PASS do połaczenia się z WiFi
        indeks = 0;
        if (BTserial.available() > 0) {

          delay(100);
          while (true) {
            sigCh = (char)BTserial.read();
            if (sigCh != '\n') {
              lancuch[indeks] = sigCh;
              indeks++;
            }

            if (sigCh == '\n')
              break;
          }

          if (lancuch[0] != '/0') {
            flag = 4;
            strncpy(passWIFI, lancuch, indeks - 1); 
           
          }
        }
        break;

        break;
      }
    case 4: { //Połaczenie z WIFI
        WiFi.begin(ssidWIFI, passWIFI);

        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          BTserial.println("Laczenie z WiFi");
        }
        BTserial.println("Polaczenie z WiFI nawiazane");

        flag = 5;
        break;
      }
    case 5 : { //MQTT
        //BTserial.println("Case 5");
        client.setServer(mqttServer, mqttPort);
        client.setCallback(callback);

        while (!client.connected()) {
          BTserial.println("Laczenie z MQTT");

          if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {

            BTserial.println("polaczone");

          } else {

            BTserial.println("failed with state ");
            BTserial.println(client.state());
            delay(2000);

          }
        }

        client.publish("KK/inż", "Witaj"); //Nazwa tematu
        client.subscribe("KK/inż");

        flag = 6;
        break;
      }
    case 6: {

        client.loop();                      
        while (true) {
          delay(1000);
          czujnik_Wartosc = 0;
          for (int i = 0; i < 1000; i++) {     ///odczytanie wartości z czujnika dla 1000 próbek
            czujnik_Wartosc += analogRead(A0);
          }
          czujnik_Wartosc = czujnik_Wartosc / 1000;               //Policzenie średniej wartości odczytanej z czujnika
          czujnik_volt = (float)czujnik_Wartosc*(5.0/1024);       //konwersja na napięcie
          RSRO = (5.0 - czujnik_volt) / czujnik_volt;             //Policzenie RS/RO                          
            alarm=0;                                               //Stan alarmu - wyłączony
            digitalWrite(15, LOW );                                //Wyłączenie alarmu
            ppmCO=96.65730337850716*pow(RSRO,-1.54022636537346);   //Wyliczenie stęzenia CO w ppm na podstawie stosunku RS/RO

           if(ppmCO>=0){                                            //Wartość w ppm musi byćwiększa lub równa zero 
            if(ppmCO>=200){
                alarm =1;                                           //Ustawienie alarmu w stan 1, iformujące na serwerze że alarm został włączony
                digitalWrite(15, HIGH );                            //Włączenie alarmu jeśli stężenei CO przekroczy 200 ppm
              }
           
            String wys = "CO "+(String)ppmCO+" ppm, stan alarmu " +alarm;  //Stworzenie Stringa, ktory bedzie zawierał wszystkie informacje na temat stężenia CO i stnu alarmu.
            char *z = &wys[0];                                             //Konwersja adresu Stringa do wskaźnika typu char w zelu przesłaniu go przez serwer MQTT.
            client.publish("KK/inż",z);                                    //Opublikawanie na serwerze MQTT danych dotyczących stężenia CO oraz stanu alarmu.
           }
        
        }
        break;
      }
  }
}
