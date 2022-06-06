struct Point1{
    int x;
    int y;
    float z;
};
struct Point1 p1 = {1, 2, 3};

struct Point2 {
    int x;
    int y;
    float z;
};
struct Point2 p2;

struct Point3 {
    int x;
    int y;
    float z;
} p3 = {1, 2, 3};


struct Point4 {
    int x;
    int y;
    float z;
} p4;

void setup() {
  Serial.begin(9600);
  Serial.println("START");

  p1 = {11, 22, 33};
  p1.x = 111;
  
  p2 = {11, 22, 33};
  p2.x = 111;
  
  p3 = {11, 22, 33};
  p3.x = 111;
  
  p4 = {11, 22, 33};
  p4.x = 111;
  
  Serial.println(p4.y);

}

void loop() {
  // put your main code here, to run repeatedly:

}
