#include <ESP8266WiFi.h>
#include <PubSubClient.h>
/*
char ssid[] = "ATTEGJxhGs";  //  your network SSID (name)
char pass[] = "pvrm%r=8h#?g";       // your network password
*/
char ssid[] = "Verizon-MiFi6620L-BDDE";  //  your network SSID (name)
char pass[] = "29de3e0e";       // your network password
char* mqtt_server = "ec2-54-201-112-223.us-west-2.compute.amazonaws.com";
char message_buff[10];
//IPAddress server(50, 201, 112, 223);

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // put your setup code here, to run once:
  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883); 
  client.setCallback(callback); 

}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  int i = 0;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (i = 0; i < length; i++) {
     message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  String msgString = String(message_buff);
  Serial.println();
  Serial.println(msgString);

  if (msgString == "SUNDAY") {
    digitalWrite(BUILTIN_LED, HIGH);
  } else {
    digitalWrite(BUILTIN_LED, LOW);  
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print(client.state());
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("espClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
       client.publish("outTopic", "hello worldz");
      // ... and resubscribe
      client.subscribe("presence");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
