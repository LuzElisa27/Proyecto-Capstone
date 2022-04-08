//Bibliotecas
#include <WiFi.h>  // Biblioteca para el control de WiFi
#include <PubSubClient.h> //Biblioteca para conexion MQTT
#include <Wire.h> //Bilbioteca de comunicacion I2C
#include "HX711.h"

//Datos de WiFi
const char* ssid = "Losguerreros";  // Aquí debes poner el nombre de tu red
const char* password = "BDA5415CqKz37NCN";  // Aquí debes poner la contraseña de tu red

//Datos del broker MQTT
const char* mqtt_server = "192.168.100.238"; // Si estas en una red local, coloca la IP asignada, en caso contrario, coloca la IP publica
IPAddress server(192,168,100,238);

// Objetos
WiFiClient espClient; // Este objeto maneja los datos de conexion WiFi
PubSubClient client(espClient); // Este objeto maneja los datos de conexion al brokerr

// Variables
const int Pin_DT1 = 4;
const int Pin_SCK1 = 2; 
const int Pin_DT2 = 14;
const int Pin_SCK2 = 15;
const int boton = 13;

int actual1,actual2,siguiente1,siguiente2,inicio;

int UnidadesINT3P;
int UnidadesINT1P;
int restar=0;
int tomado;

HX711 balanza1;
HX711 balanza2;

const float interruptor1P15A = 11.8; //Peso por unidad de interruptores tipo 1P-15A.
const float interruptor3P70A = 31.5; //Peso por unidad de interruptores tipo 3P70A.

long timeNow, timeLast; // Variables de control de tiempo no bloqueante
int data = 0; // Contador
int wait = 5000;  // Indica la espera cada 5 segundos para envío de mensajes MQTT

// Inicialización del programa
void setup() {
  // Iniciar comunicación serial
  Serial.begin (115200);

  pinMode(boton,INPUT);  
 
Serial.println();
  Serial.println();
  Serial.print("Conectar a ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password); // Esta es la función que realiz la conexión a WiFi
 
  while (WiFi.status() != WL_CONNECTED) { // Este bucle espera a que se realice la conexión
    delay(500); //dado que es de suma importancia esperar a la conexión, debe usarse espera bloqueante
    Serial.print(".");  // Indicador de progreso
  }
  
  // Cuando se haya logrado la conexión, el programa avanzará, por lo tanto, puede informarse lo siguiente
  Serial.println();
  Serial.println("WiFi conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
  
  delay (1000); // Esta espera es solo una formalidad antes de iniciar la comunicación con el broker

  // Conexión con el broker MQTT
  client.setServer(server, 1883); // Conectarse a la IP del broker en el puerto indicado
  /*client.setCallback(callback); // Activar función de CallBack, permite recibir mensajes MQTT y ejecutar funciones a partir de ellos*/
  delay(1500);  // Esta espera es preventiva, espera a la conexión para no perder información

  timeLast = millis (); // Inicia el control de tiempo
  
  balanza1.begin(Pin_DT1, Pin_SCK1);
  balanza2.begin(Pin_DT2, Pin_SCK2);
  Serial.print("Lectura del valor del ADC: Kg");
  Serial.println(balanza1.read());
  Serial.println(balanza2.read());
  Serial.println("No ponga ningún objeto sobre la balanza");
  Serial.println("Destarando...");
  balanza1.set_scale(216583.1652);
  balanza2.set_scale(218664.1038); 
  balanza1.tare(20);
  balanza2.tare(20);
  Serial.println("Coloque sobre la balanza un peso conocido:");
}// fin del void setup ()

// Función para reconectarse
void reconnect() {
  // Bucle hasta lograr conexión
  while (!client.connected()) { // Pregunta si hay conexión
    Serial.print("Tratando de contectarse...");
    // Intentar reconexión
    if (client.connect("ESP32CAMClient")) { //Pregunta por el resultado del intento de conexión
      Serial.println("Conectado");
      client.subscribe("esp32/balanzas"); // Esta función realiza la suscripción al tema
    }// fin del  if (client.connect("ESP32CAMClient"))
    else {  //en caso de que la conexión no se logre
      Serial.print("Conexion fallida, Error rc=");
      Serial.print(client.state()); // Muestra el codigo de error
      Serial.println(" Volviendo a intentar en 5 segundos");
      // Espera de 5 segundos bloqueante
      delay(5000);
      Serial.println (client.connected ()); // Muestra estatus de conexión
    }// fin del else
  }// fin del bucle while (!client.connected())
}// fin de void reconnect()
int i=0;
void loop() 
{
  //Verificar siempre que haya conexión al broker
  if (!client.connected()) {
    reconnect();  // En caso de que no haya conexión, ejecutar la función de reconexión, definida despues del void setup ()
  }// fin del if (!client.connected())
  client.loop(); // Esta función es muy importante, ejecuta de manera no bloqueante las funciones necesarias para la comunicación con el broker

  int valor=digitalRead(boton);
  if(valor==LOW)
  {
    timeNow = millis(); // Control de tiempo para esperas no bloqueantes
    if (timeNow - timeLast > wait) 
    { // Manda un mensaje por MQTT cada cinco segundos
      timeLast = timeNow; // Actualización de seguimiento de tiempo
  
      if (balanza1.wait_ready_timeout(2000)) 
      {
          float pesouno = balanza1.get_units(20);
          float peso1 = pesouno * 1000;
          UnidadesINT1P = peso1 / interruptor1P15A;
          if(i==0)
          {
            if(UnidadesINT1P!=0)
            {
              restar=restar+UnidadesINT1P;  
            }
            actual1=UnidadesINT1P-restar;
            Serial.print("La bascula 1 se reiniciara en ");
            Serial.print(actual1);
            Serial.println(" Unidades");
          }
          else
          {
            siguiente1=UnidadesINT1P-restar;
            if(siguiente1!=actual1)
            {
              tomado=abs(siguiente1-actual1);
              
              if(siguiente1>actual1)
              {
                actual1=siguiente1;
                Serial.print("En la bascula 1 se agrego: ");
                data=tomado;
                char datastring[8];
                dtostrf(data,1,2,datastring);
                client.publish("esp32/balanzas",datastring);
                Serial.print(tomado);
                Serial.print(" unidades quedando en existencia: ");
                Serial.print(actual1);
                Serial.println(" Unidades");
                  
              }
              else
              {
                actual1=siguiente1;
                Serial.print("En la bascula 1 se tomo: ");
                Serial.print(tomado);
                Serial.print(" unidades quedando en existencia: ");
                Serial.print(actual1);
                Serial.println(" Unidades");
              }
              
            }
            
          }
        } 
        else 
        {
          Serial.println("HX711 no encontrado.");
          delay(2000);
        }
      
        if (balanza2.wait_ready_timeout(1000)) 
        {
          float pesodos = balanza2.get_units(20);
          float peso2 = pesodos * 1000;
          UnidadesINT3P = peso2 / interruptor3P70A;
          if(i==0)
          {
            actual2=UnidadesINT3P;
            Serial.print("La bascula 2 se reinicia en ");
            Serial.print(actual2);
            Serial.println(" Unidades");
          }
          else
          {
            siguiente2=UnidadesINT3P;
            if(siguiente2!=actual2)
            {
              tomado=abs(siguiente2-actual2);

              if(siguiente2>actual2)
              {
                actual2=siguiente2;
                Serial.print("En la bascula 2 se agrego: ");
                Serial.print(tomado);
                Serial.print(" unidades quedando en existencia: ");
                Serial.print(actual2);
                Serial.println(" Unidades");
              }
              else
              {
                actual2=siguiente2;
                Serial.print("En la bascula 2 se tomo: ");
                Serial.print(tomado);
                Serial.print(" unidades quedando en existencia: ");
                Serial.print(actual2);
                Serial.println(" Unidades");
              }
              
            }
          }
          
        } 
        else 
        {
          Serial.println("HX711 no encontrado.");
          delay(2000);
        }
    }
    i=1;
  }
  
}


/*// Esta función permite tomar acciones en caso de que se reciba un mensaje correspondiente a un tema al cual se hará una suscripción
void callback(char* topic, byte* message, unsigned int length) 
{

  // Indicar por serial que llegó un mensaje
  Serial.print("Llegó un mensaje en el tema: ");
  Serial.print(topic);

  // Concatenar los mensajes recibidos para conformarlos como una varialbe String
  String messageTemp; // Se declara la variable en la cual se generará el mensaje completo  
  for (int i = 0; i < length; i++) 
  {  // Se imprime y concatena el mensaje
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  // Se comprueba que el mensaje se haya concatenado correctamente
  Serial.println();
  Serial.print ("Mensaje concatenado en una sola variable: ");
  Serial.println (messageTemp);

  // En esta parte puedes agregar las funciones que requieras para actuar segun lo necesites al recibir un mensaje MQTT

  // Ejemplo, en caso de recibir el mensaje true - false, se cambiará el estado del led soldado en la placa.
  // El ESP323CAM está suscrito al tema esp/output
  if (String(topic) == "esp32/output") {  // En caso de recibirse mensaje en el tema esp32/output
    if(messageTemp == "true")
    {
      Serial.println("Led encendido");
      digitalWrite(flashLedPin, HIGH);
    }// fin del if (String(topic) == "esp32/output")
    else if(messageTemp == "false")
    {
      Serial.println("Led apagado");
      digitalWrite(flashLedPin, LOW);
    }// fin del else if(messageTemp == "false")
  }// fin del if (String(topic) == "esp32/output")
}// fin del void callback*/
