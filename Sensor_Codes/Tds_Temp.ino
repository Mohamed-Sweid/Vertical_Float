#include <micro_ros_arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <std_msgs/msg/float32.h>
#include <stdio.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <Wire.h>
#include <rcl/rcl.h>

// DS18B20 Configuration
#define ONE_WIRE_BUS 13 // GPIO pin connected to the data pin of the DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// TDS Sensor Configuration
#define TDS_SENSOR_PIN 34 // GPIO pin for TDS sensor analog output (ESP32 ADC pin)
#define VREF 3.3          // Voltage reference of the microcontroller (3.3V for ESP32)
#define ADC_RESOLUTION 4096 // 12-bit ADC resolution on ESP32
#define TDS_CONVERSION_FACTOR 0.5 // Adjust based on the sensor datasheet or calibration

// Micro-ROS setup
rcl_publisher_t temperature_publisher;
rcl_publisher_t tds_publisher;
std_msgs__msg__Float32 temperature_msg;
std_msgs__msg__Float32 tds_msg;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rclc_executor_t executor;

void setup() {
  // Initialize DS18B20
  sensors.begin();

  // Initialize Serial for debugging
  Serial.begin(115200);
  while (!Serial) {}

  // Initialize Micro-ROS
  set_microros_transports(); // Serial transport is used by default

  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);

  rclc_node_init_default(&node, "sensor_node", "", &support);

  // Initialize Temperature Publisher
  rclc_publisher_init_default(
      &temperature_publisher,
      &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
      "temperature");

  // Initialize TDS Publisher
  rclc_publisher_init_default(
      &tds_publisher,
      &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
      "tds");
}

void loop() {
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));

  // Read temperature
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0); // Get the temperature in Celsius

  if (temperature != DEVICE_DISCONNECTED_C) {
    // Publish the temperature
    temperature_msg.data = temperature;
    rcl_publish(&temperature_publisher, &temperature_msg, NULL);

    // Debug output
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");
  } else {
    Serial.println("Error: Could not read temperature data");
  }

  // Read TDS Sensor
   int analogValue = analogRead(TDS_SENSOR_PIN); // Read analog value
  float voltage = (analogValue / (float)ADC_RESOLUTION) * VREF; // Convert to voltage
  float tdsValue = voltage * TDS_CONVERSION_FACTOR * 1000; // Convert to ppm (adjust factor as needed)

  // Publish TDS value
  tds_msg.data = tdsValue;
  rcl_publish(&tds_publisher, &tds_msg, NULL);

  // Debug output
  Serial.print("TDS Value: ");
  Serial.print(tdsValue);
  Serial.println(" ppm");

  delay(1000); // Publish every second
  
}
