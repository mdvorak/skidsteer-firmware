#include <Arduino.h>
#include <Bluepad32.h>
#include <ESP32Servo.h>  // by Kevin Harrington
#include <ESP32PWM.h>
#include <vector>

// defines
#define bucketServoPin 17
#define auxServoPin 5
#define lightPin1 18
//#define lightPin2 5
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define ARMUP 5
#define ARMDOWN 6
#define STOP 0

#define RIGHT_MOTOR 1
#define LEFT_MOTOR 0
#define ARM_MOTOR 2

#define FORWARD 1
#define BACKWARD -1

const int FREQUENCY = 100;
const double MIN_POWER = 0.3;

// global variables

Servo bucketServo;
Servo auxServo;

bool removeArmMomentum = false;
bool light = false;
int64_t lastActivity = 0;

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// This struct is never deleted when allocated
struct MOTOR_PINS {
    ESP32PWM *const pinIN1;
    ESP32PWM *const pinIN2;
    double coeficient;

    MOTOR_PINS(int pin1, int pin2, double coeficient = 1)
            : pinIN1(new ESP32PWM()),
              pinIN2(new ESP32PWM()),
              coeficient(coeficient) {
        pinIN1->attachPin(pin1, FREQUENCY);
        pinIN2->attachPin(pin2, FREQUENCY);
    }

    void move(double power) {
        if (power > 0) {
            this->pinIN2->writeScaled(0);
            this->pinIN1->writeScaled(power * coeficient);
        } else if (power < 0) {
            this->pinIN1->writeScaled(0);
            this->pinIN2->writeScaled(-power * coeficient);
        } else {
            this->pinIN1->writeScaled(0);
            this->pinIN2->writeScaled(0);
            this->pinIN1->writeScaled(1);
            this->pinIN2->writeScaled(1);
        }
    }
};

std::vector<MOTOR_PINS> motorPins = {
        MOTOR_PINS(2, 4, 1.0),
        MOTOR_PINS(22, 12, 1.0),
        MOTOR_PINS(13, 21, 0.8),  //ARM_MOTOR pins
};

void reportActivity() {
    lastActivity = esp_timer_get_time();
}


void rotateMotor(int motorNumber, double motorDirection) {
    reportActivity();

    if (abs(motorDirection) > MIN_POWER) {
        motorPins[motorNumber].move(motorDirection);
    } else {
        if (removeArmMomentum) {
            motorPins[ARM_MOTOR].move(1);
            delay(10);
            motorPins[motorNumber].move(0);
            delay(5);
            motorPins[ARM_MOTOR].move(1);
            delay(10);
            removeArmMomentum = false;
        }
        motorPins[motorNumber].move(0);
    }
}

void bucketTilt(int bucketServoValue) {
    reportActivity();
    Serial.printf("bucket=%d\n", bucketServoValue);
    bucketServo.write(bucketServoValue);
}

void auxControl(int auxServoValue) {
    reportActivity();
    Serial.printf("aux=%d\n", auxServoValue);
    auxServo.write(auxServoValue);
}

void lightControl() {
    reportActivity();

    if (!light) {
        digitalWrite(lightPin1, HIGH);
        //digitalWrite(lightPin2, LOW);
        light = true;
        Serial.println("Made it to lights");
    } else {
        digitalWrite(lightPin1, LOW);
        //digitalWrite(lightPin2, LOW);
        light = false;
    }
}

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
            // Additionally, you can get certain gamepad properties like:
            // Model, VID, PID, BTAddr, flags, etc.
            ControllerProperties properties = ctl->getProperties();
            Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(),
                          properties.vendor_id,
                          properties.product_id);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            break;
        }
    }
    if (!foundEmptySlot) {
        Serial.println("CALLBACK: Controller connected, but could not found empty slot");
    }

    // Only one controller allowed
    BP32.enableNewBluetoothConnections(false);
}

void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }

    if (!foundController) {
        Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
    }

    BP32.enableNewBluetoothConnections(true);
}

static int bucketMovement = 0;
static int clawMovement = 0;
const int BUCKET_STEP = 1;
const int64_t BUCKET_INTERVAL = 5;

#define WII_UP 1
#define WII_DOWN 2
#define WII_RIGHT 4
#define WII_LEFT 8

const int AXIS_MAX = 360;

void processGamepad(ControllerPtr ctl) {
    int axisX = constrain(ctl->axisX(), -AXIS_MAX, AXIS_MAX);
    int axisY = constrain(ctl->axisY(), -AXIS_MAX, AXIS_MAX);

    double leftMotor = (double)constrain(-axisY + axisX, -AXIS_MAX, AXIS_MAX) / (double)AXIS_MAX;
    double rightMotor = (double)constrain(-axisY - axisX, -AXIS_MAX, AXIS_MAX) / (double)AXIS_MAX;

    //Serial.printf("%d/%d -> %.3f/%.3f %d\n", axisX, axisY, leftMotor, rightMotor, ctl->dpad());

    if (ctl->dpad() & WII_DOWN) {
        rotateMotor(RIGHT_MOTOR, BACKWARD);
        rotateMotor(LEFT_MOTOR, BACKWARD);
    } else if (ctl->dpad() & WII_UP) {
        rotateMotor(RIGHT_MOTOR, FORWARD);
        rotateMotor(LEFT_MOTOR, FORWARD);
    } else if (ctl->dpad() & WII_LEFT) {
        rotateMotor(RIGHT_MOTOR, FORWARD);
        rotateMotor(LEFT_MOTOR, BACKWARD);
    } else if (ctl->dpad() & WII_RIGHT) {
        rotateMotor(RIGHT_MOTOR, BACKWARD);
        rotateMotor(LEFT_MOTOR, FORWARD);
    } else {
        rotateMotor(LEFT_MOTOR, leftMotor);
        rotateMotor(RIGHT_MOTOR, rightMotor);
    }

    if (ctl->x()) {
        Serial.println("arm backward");
        rotateMotor(ARM_MOTOR, BACKWARD);
        removeArmMomentum = true;
    } else if (ctl->y()) {
        Serial.println("arm forward");
        rotateMotor(ARM_MOTOR, FORWARD);
    } else {
        //Serial.println("arm stop");
        rotateMotor(ARM_MOTOR, STOP);
    }

    if (ctl->a()) {
        bucketMovement = BUCKET_STEP;
    } else if (ctl->b()) {
        bucketMovement = -BUCKET_STEP;
    } else {
        bucketMovement = 0;
    }

    if (ctl->miscButtons() & 2) {
        clawMovement = -BUCKET_STEP;
    } else if (ctl->miscButtons() & 4) {
        clawMovement = BUCKET_STEP;
    } else {
        clawMovement = 0;
    }
}

void processControllers() {
    for (auto ctrl : myControllers) {
        if (ctrl && ctrl->isConnected() && ctrl->hasData()) {
            processGamepad(ctrl);
        }
    }
}

void setUpPinModes() {
    bucketServo.attach(bucketServoPin);
    auxServo.attach(auxServoPin);
    auxControl(150);
    bucketTilt(140);

    pinMode(lightPin1, OUTPUT);
    //pinMode(lightPin2, OUTPUT);

    pinMode(25, INPUT_PULLUP);
}

void turnOff() {
    Serial.println("TURNING OFF");
    delay(100);
    pinMode(23, INPUT);
    esp_deep_sleep_start();
}

// Arduino setup function. Runs in CPU 1
void setup() {
    pinMode(23, OUTPUT);
    digitalWrite(23, HIGH);

    setUpPinModes();
    Serial.begin(115200);

    // Controller
    Serial.begin(115200);
    Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t *addr = BP32.localBdAddress();
    Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    // Setup the Bluepad32 callbacks
    BP32.setup(&onConnectedController, &onDisconnectedController);

    pinMode(0, OUTPUT);
    digitalWrite(0, LOW);

    // "forgetBluetoothKeys()" should be called when the user performs
    // a "device factory reset", or similar.
    // Calling "forgetBluetoothKeys" in setup() just as an example.
    // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
    // But it might also fix some connection / re-connection issues.
    //BP32.forgetBluetoothKeys();

    // Enables mouse / touchpad support for gamepads that support them.
    // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
    // - First one: the gamepad
    // - Second one, which is a "virtual device", is a mouse.
    // By default, it is disabled.
    BP32.enableVirtualDevice(false);
    BP32.enableNewBluetoothConnections(true);
}

// Arduino loop function. Runs in CPU 1.
void loop() {
    static bool btn = true;
    static int64_t btnPress = 0;
    if (!digitalRead(25)) {
        if (!btn) {
            btn = true;
            btnPress = esp_timer_get_time();
            Serial.println("BTN PRESS");
        }
    } else if (btn) {
        btn = false;
        Serial.println("BTN RELEASE");

        // 3 sec
        if (esp_timer_get_time() - btnPress > 3000000ll) {
            turnOff();
        }
    }

    static int64_t lastBucketUpdate = 0;
    static int lastBucketTilt = 90;
    static int lastClawTilt = 90;

    if (esp_timer_get_time() - lastBucketUpdate > BUCKET_INTERVAL * 1000ll) {
        lastBucketUpdate = esp_timer_get_time();

        if (bucketMovement != 0) {
            lastBucketTilt = max(10, min(170, lastBucketTilt + bucketMovement));
            bucketTilt(lastBucketTilt);
        }

        if (clawMovement != 0) {
            lastClawTilt = max(10, min(170, lastClawTilt + clawMovement));
            auxControl(lastClawTilt);
        }
    }

    // This call fetches all the controllers' data.
    // Call this function in your main loop.
    bool dataUpdated = BP32.update();
    if (dataUpdated)
        processControllers();

    int64_t secondsElapsed = (esp_timer_get_time() - lastActivity) / 1000000;
    if (secondsElapsed > 300)  // 5 min
    {
        // Turn itself off
        turnOff();
    }
}
