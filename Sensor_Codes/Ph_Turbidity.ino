#include <micro_ros_arduino.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/float32.h>

// --- Sensor Pins ---
const int phPin = 35;
const int turbidityPin = 34;

// --- Micro-ROS Variables ---
rcl_node_t node;
rcl_publisher_t ph_publisher;
rcl_publisher_t turbidity_publisher;
rcl_timer_t timer;
rclc_executor_t executor;
rcl_allocator_t allocator;
rclc_support_t support;

// --- ROS Messages ---
std_msgs__msg__Float32 ph_msg;
std_msgs__msg__Float32 turbidity_msg;

// --- Timer Callback ---
void timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  RCLC_UNUSED(last_call_time);
  if (timer != NULL) {
    // Read pH raw and convert to voltage
    int ph_raw = analogRead(phPin);
    float ph_voltage = ph_raw * (3.3 / 4095.0);
    float ph_value = 3.5 * ph_voltage;  // Adjust based on your calibration

    // Read turbidity raw and convert to NTU
    int turbidity_raw = analogRead(turbidityPin);
    float turbidity_voltage = turbidity_raw * (3.3 / 4095.0);
    float turbidity_ntu = turbidity_voltage * 100.0 - 175;  // Adjust based on your calibration

    // Publish pH
    ph_msg.data = ph_value;
    rcl_publish(&ph_publisher, &ph_msg, NULL);

    // Publish turbidity
    turbidity_msg.data = turbidity_ntu;
    rcl_publish(&turbidity_publisher, &turbidity_msg, NULL);
  }
}

void error_loop() {
  while (1) {
    delay(100);
  }
}

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();} }
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){} }

void setup() {
  Serial.begin(115200);
  set_microros_transports();
  delay(2000);

  pinMode(phPin, INPUT);
  pinMode(turbidityPin, INPUT);

 // analogSetAtten(ADC_11db);  // For full 0–3.3V range on ESP32

  allocator = rcl_get_default_allocator();
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_node_init_default(&node, "esp32_water_quality_node", "", &support));

  // pH Publisher
  RCCHECK(rclc_publisher_init_default(
    &ph_publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
    "/ph"));

  // Turbidity Publisher
  RCCHECK(rclc_publisher_init_default(
    &turbidity_publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
    "/turbidity"));

  // Timer (1Hz)
  RCCHECK(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(1000),
    timer_callback));

  // Executor
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));
}

void loop() {
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  delay(100);
}
