int lightPin = D0;
int motionSensorPin = D1;

void lightOn()
{

}

void lightOff()
{

}

void setup() 
{
  Serial.begin(9600);
  pinMode(lightPin,  OUTPUT);
  pinMode(motionSensorPin, INPUT);
}

void loop() 
{
  int LightValue = analogRead(A0);
  Serial.println(LightValue);
  if (digitalRead(motionSensorPin) == 1)
  {
    digitalWrite(lightPin, HIGH);
  }
  else 
  {
    digitalWrite(lightPin, LOW);
  }
  
}
