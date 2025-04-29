#include <Arduino.h>

// micro-ROS & rclc:
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rclc/publisher.h>

// message types:
#include <std_msgs/msg/float32.h>
#include <std_msgs/msg/string.h>
#include <rosidl_runtime_c/string_functions.h>

// math helpers:
#include <math.h>

// ─── 1) State machine types & prototype ───
enum State {
  DESCEND,
  COLLECT,
  ASCEND,
  SURFACE_TRANSMIT
};
State currentState;
void transitionTo(State newState);

// ─── 2) Pin assignments ───
const int PRESSURE_PIN   = 4;   // ADC1_6
const int MOTOR_PIN      =  12;   // motor driver IN1 (IN2 = GND)
const int VALVE_PINS[4]  = {5, 2, 15, 13};  // [0,1] = descent; [2,3] = ascend

// ─── 3) Sensor & timing constants ───
const float sensorMaxVoltage            = 5.0f;    // ADC full-scale (V)
const float pressureMax                 = 1.0f;   // sensor full-scale (bar)
const float targetDepthPressureBar      = 1.0f;    // 1 bar ~10 m
const float surfacePressureThresholdBar = 0.1f;    // ≈ 0 bar gauge
const unsigned long COLLECT_DURATION         = 10UL * 1UL * 1000UL; // 10 min
const unsigned long SURFACE_TRANSMIT_DURATION = 5UL * 1UL * 1000UL; // 5 min

// ─── 4) micro-ROS objects ───
rclc_support_t support;
rcl_node_t     node;
rcl_publisher_t pressurePub;
rcl_publisher_t statePub;
rclc_executor_t executor;

// ─── 5) State timing ───
unsigned long stateStartTime;

void setup() {
  // —— Serial & pins ——
  Serial.begin(115200);
  pinMode(PRESSURE_PIN, INPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);  // start motor off

  for (int i = 0; i < 4; i++) {
    pinMode(VALVE_PINS[i], OUTPUT);
    digitalWrite(VALVE_PINS[i], LOW);
  }

  // —— micro-ROS init ——
  set_microros_transports();  // your transport init, e.g. Serial1…
  rcl_allocator_t allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);

  // create node
  rclc_node_init_default(&node, "float_node", "", &support);

  // create publishers
  rclc_publisher_init_default(
    &pressurePub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
    "pressure_bar"
  );
  rclc_publisher_init_default(
    &statePub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
    "float_state"
  );

  // executor (no subscriptions yet)
  rclc_executor_init(&executor, &support.context, 0, &allocator);

  // start descending immediately
  transitionTo(DESCEND);
}

void loop() {
  // — read sensor & convert —
  float raw = analogRead(PRESSURE_PIN);
  float v   = raw * (sensorMaxVoltage / 4095.0f);
  float bar = voltageToBar(v);

  // — publish pressure —
  std_msgs__msg__Float32 pressure_msg;
  pressure_msg.data = bar;
  rcl_publish(&pressurePub, &pressure_msg, NULL);

  // — publish state string —
  const char *stateNames[] = {
    "DESCEND", "COLLECT", "ASCEND", "SURFACE_TRANSMIT"
  };
  std_msgs__msg__String state_msg;
  rosidl_runtime_c__String__init(&state_msg.data);
  rosidl_runtime_c__String__assign(&state_msg.data, stateNames[currentState]);
  rcl_publish(&statePub, &state_msg, NULL);
  rosidl_runtime_c__String__fini(&state_msg.data);

  // — state machine logic —
  unsigned long elapsed = millis() - stateStartTime;
  switch (currentState) {
    case DESCEND:
      if (bar < targetDepthPressureBar) {
        transitionTo(COLLECT);
      } else if (bar >= pressureMax) {
        transitionTo(ASCEND);
      }
      break;

    case COLLECT:
      if (elapsed >= COLLECT_DURATION) {
        transitionTo(ASCEND);
      }
      break;

    case ASCEND:
      if (bar <= surfacePressureThresholdBar) {
        transitionTo(SURFACE_TRANSMIT);
      }
      break;

    case SURFACE_TRANSMIT:
      if (elapsed >= SURFACE_TRANSMIT_DURATION) {
        transitionTo(DESCEND);
      }
      break;
  }

  // — spin micro-ROS —
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
}

void transitionTo(State newState) {
  currentState   = newState;
  stateStartTime = millis();

  // turn everything off by default
  digitalWrite(MOTOR_PIN, LOW);
  for (int i = 0; i < 4; i++) {
    digitalWrite(VALVE_PINS[i], LOW);
  }

  // activate pins per state
  switch (newState) {
    case DESCEND:
      digitalWrite(VALVE_PINS[0], HIGH);
      digitalWrite(VALVE_PINS[1], HIGH);
      digitalWrite(MOTOR_PIN,     HIGH);
      break;
    case COLLECT:
      // all closed, pump off
      break;
    case ASCEND:
      digitalWrite(VALVE_PINS[2], HIGH);
      digitalWrite(VALVE_PINS[3], HIGH);
      digitalWrite(MOTOR_PIN,     HIGH);
      break;
    case SURFACE_TRANSMIT:
      // all off
      break;
  }
}

float voltageToBar(float measuredVoltage) {
  float actualV = measuredVoltage * (5.0f / sensorMaxVoltage);  // map 0–3.3 V → 0–5 V
  float p       = (actualV / 5.0f) * pressureMax;                // 0–5 V → 0–10 bar
  return fmaxf(0.0f, fminf(p, pressureMax));                     // clamp
}
