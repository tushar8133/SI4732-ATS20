void func1(int aa, int bb) {
  Serial.print("func1 "); Serial.print(aa); Serial.println(bb);
}
void func2(int aa, int bb) {
  Serial.print("func2 "); Serial.print(aa); Serial.println(bb);
}
void func3(int aa, int bb) {
  Serial.print("func3 "); Serial.print(aa); Serial.println(bb);
}
void func4(int aa, int bb) {
  Serial.print("func4 "); Serial.print(aa); Serial.println(bb);
}

typedef struct
{
  const int pin;
  const char * const title;
  const bool complex;
  long tstamp;
  void (*myfunc)(int, int);
} Btns;

Btns btns[] = {
  {12, "SETTINGS", true, 0, func1 },
  {11, "VOL", false, 0, func2 },
  {4, "STEP", false, 0, func3 }
};

void setup() {
  Serial.begin(9600);

  // METHOD 01
  // switch (2) {
  //   case 0 : func1(11, 22) ; break;
  //   case 1 : func2(11, 22) ; break;
  //   case 2 : func3(11, 22) ; break;
  // }

  // METHOD 02
  // void (*hello)(int, int) = &func1;
  // hello(11, 22);

  // METHOD 03
  // void (*hello[])(int, int) = {&func1, &func2, &func3, &func4};
  // hello[0](11, 22);
  // hello[1](11, 22);

  // METHOD 04
  // void (*hello[])(int, int) { func1, func2, func3, func4 };
  // hello[1](11, 22);
  // hello[0](11, 22);

  // METHOD 05
  // void (*hello[])(int, int) = { func1, func2, func3, func4 };
  // hello[0](11, 22);
  // hello[1](11, 22);

  // METHOD 06
  // void (*hello)();
  // hello = func1;
  // func1(11, 22);

  // METHOD 07
  // typedef void (*hello)();
  // hello myfunc[] = { func1, func2, func3, func4 };
  // btns[2].myfunc(11, 22);

}

void loop() {
  
}
