#include <Servo.h>

Servo myservo;
const int pingPin = 7;
const int pingPin2 = 9;
long duration;
long duration1;
void setup ()
{myservo.attach(5);
}
void loop ()
{
pinMode (pingPin, OUTPUT);
http://www.instructables.com/id/Wex-the-One-Eyed-Watcher/
digitalWrite (pingPin, LOW);
delayMicroseconds (2);
digitalWrite (pingPin, HIGH);
delayMicroseconds (5);
digitalWrite (pingPin, LOW);
pinMode (pingPin, INPUT);
duration = pulseIn (pingPin, HIGH);
if (duration <5000)
{
myservo.write(125);
delay(500);
}
// else
{
pinMode (pingPin2, OUTPUT);
digitalWrite (pingPin2, LOW);
delayMicroseconds (2);
digitalWrite (pingPin2, HIGH);
delayMicroseconds (5);
digitalWrite (pingPin2, LOW);
pinMode (pingPin2, INPUT);
duration1 = pulseIn (pingPin2, HIGH);
if (duration1 <5000)
{
myservo.write(5);
delay(500);
}
if (duration >5000 and duration1 >5000)
{myservo.write(65);
delay(500);
}}}


