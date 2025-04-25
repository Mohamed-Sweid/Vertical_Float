#include <Arduino.h>
#include <micro_ros_arduino.h>

#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/float32.h>

const int pHPin = 34;        // pH sensor analog input pin
float neutralVoltage = 2.06; // Calibration value for pH 7.0 (adjust during calibration)
float acidVoltage = 2.54;    // Calibration value for pH 4.0 (adjust during calibration)
float slope = 0.0;           // Calculated during calibration
float intercept = 0.0;       // Calculated during calibration

// Error_Handling
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}
void error_loop(){
  while(1){
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(100);
  }
}

// CallBack From Timer Work By Spin Function
void timer_callback(rcl_timer_t * timer, int64_t last_call_time){
  RCLC_UNUSED(last_call_time);
  if (timer != NULL) {
    // Read and average 10 samples for stability
    int sensorValue = 0;
    for (int i = 0; i < 10; i++) {
    sensorValue += analogRead(pHPin);
    delay(10);
    }
    float voltage = (sensorValue / 10.0) * (3.3 / 4095.0);

    // Calculate pH value using calibration parameters
    float pHValue = slope * voltage + intercept;
    
    // Publish the pH value
    msg.data = pHValue;
    RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
  }
}


void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // Set ADC to 12-bit resolution (0-4095)
  analogSetAtten(ADC_11db);  // Set input range to 0-3.3V
  
  // Calculate calibration parameters
  // Use two-point calibration (pH 4.0 and pH 7.0)
  slope = (7.0 - 4.0) / (neutralVoltage - acidVoltage);
  intercept = 7.0 - slope * neutralVoltage;

  set_microros_transports();
  
  allocator = rcl_get_default_allocator();

  // Create init_options
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // Create node
  RCCHECK(rclc_node_init_default(&node, "ph_sensor_node", "", &support));

  // Create publisher
  RCCHECK(rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
    "pHReading"));

  // Create timer
  rcl_timer_t timer;
  const unsigned int timer_timeout = 1000; // 1000ms
  RCCHECK(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback));//timer_callback==Function Called Every 1Second

  // Create executor
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));
}
// Working Loop
void loop() {
  delay(100);
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
}

// Testing loop

// void loop() {
//   // Read and average 10 samples for stability
//   int sensorValue = 0;
//   for (int i = 0; i < 10; i++) {
//     sensorValue += analogRead(pHPin);
//     delay(10);
//   }
//   float voltage = (sensorValue / 10.0) * (3.3 / 4095.0);

//   // Calculate pH value using calibration parameters
//   float pHValue = slope * voltage + intercept;

//   Serial.print("Voltage: ");
//   Serial.print(voltage, 3);
//   Serial.print("V  |  pH: ");
//   Serial.println(pHValue, 2);
  
//   delay(1000); // Wait between readings
// }
