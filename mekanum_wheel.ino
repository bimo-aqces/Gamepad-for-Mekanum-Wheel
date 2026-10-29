#include <Bluepad32.h>

class MotorSystem {
  public:
    MotorSystem(uint8_t pwm, uint8_t in1, uint8_t in2): pwm(pwm), in1(in1), in2(in2) {
      pinMode(pwm, OUTPUT); // biar entar pas manggil langsung setup
      pinMode(in1, OUTPUT);
      pinMode(in2, OUTPUT);
      set_direction(0);
    }

    void set_direction(int16_t dir) { // clockwise atw counterclockwise tergantung pwm (dir)
      int speed = (abs(dir)); // geser 2 bit, 512 to 128
      analogWrite(pwm, speed >> 3);
      if (dir > 0) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
      } 
      else if (dir < 0) {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
      } 
      else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
      }
    }

  private:
    uint8_t pwm, in1, in2;
};

MotorSystem bl(23, 18, 19); // belakang kanan
MotorSystem br(32, 5, 33);  // belakang kiri
MotorSystem fl(13, 27, 14); // depan kanan
MotorSystem fr(17, 25, 26); // depan kiri // gw coba ganti barusan left rightnya // yak benar kebalik

ControllerPtr gamepad = nullptr;

void is_connected(ControllerPtr controller) {
  if (gamepad == nullptr) gamepad = controller; 
}

void is_disconnected(ControllerPtr controller) {
  if (gamepad == controller) gamepad = nullptr; 
}

const int DEADZONE = 50;

// Deadzone + return skala -1 sampai 1 supaya mulus "ceunah"
float applyDeadzone(int v) {
  if (abs(v) < DEADZONE){
    return 0;
  }
  float sign;
  if(v<=0){
    sign = -1.0f;
  }
  else{
    sign = 1.0f;
  }
  return sign * (abs(v) - DEADZONE) / (512.0f - DEADZONE);  
}

void stopAll() {
  fl.set_direction(0);
  fr.set_direction(0);
  bl.set_direction(0);
  br.set_direction(0);
}

void setup() {
  Serial.begin(115200);
  BP32.setup(&is_connected, &is_disconnected);
}

void loop() {
  BP32.update();

  if (gamepad && gamepad->isConnected()) {
    Serial.printf("Kiri X:%d Y:%d | Kanan X:%d Y:%d\n",
      gamepad->axisX(),  gamepad->axisY(), gamepad->axisRX(), gamepad->axisRY()); //bwt cek gamepad pas nyolok ajh

    float vy  = applyDeadzone(-gamepad->axisY()); // stick kiri: maju / mundur, y wajib - btw
    float vx  = applyDeadzone(gamepad->axisX()); // stick kiri: kanan / kiri
    float rot = applyDeadzone(gamepad->axisRX()); // stick kanan: putar searah jarum jam

    // Eq inverse kinematics mecanum (konfigurasi X)
    float pFL = vy + vx + rot;
    float pFR = vy - vx - rot;
    float pBL = vy - vx + rot;
    float pBR = vy + vx - rot;

    // menghindari val > 0 dengan cara bagi max val dari keempat anu
    float maxVal = max(max(fabs(pFL), fabs(pFR)), max(fabs(pBL), fabs(pBR)));
    if (maxVal > 1.0f) {
      pFL /= maxVal; 
      pFR /= maxVal; 
      pBL /= maxVal; 
      pBR /= maxVal;
    }

    // return value jadi pwm normal, kek ngaliin jadi persentase, btw isinya dir jadi udh >> 2 // gw ganti >> 2 nya disini karna masi keliatan brutal
    fl.set_direction((int16_t) (pFL * 512));
    fr.set_direction((int16_t) (pFR * 512));
    bl.set_direction((int16_t) (pBL * 512));
    br.set_direction((int16_t) (pBR * 512));

  } 
  else {
    stopAll();  
  }

  delay(10);
}
