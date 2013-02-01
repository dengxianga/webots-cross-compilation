/*
 * File:          DeviceManager.hpp
 * Date:          June 2012
 * Description:   Class allowing to create or retrieve devices
 * Author:        fabien.rohrer@cyberbotics.com
 * Modifications: January 2013 adapted to DARwIn-OP robot by david.mansolino@epfl.ch
 */

#ifndef DEVICE_MANAGER_HPP
#define DEVICE_MANAGER_HPP

#include <webots/types.h>

#include <vector>

class Device;
class CameraR;
class Led;
class ServoR;
class SingleValueSensor;
class TripleValuesSensor;

class DeviceManager {
public:
  static DeviceManager *instance();
  static void cleanup();

  const std::vector<Device *> &devices() const { return mDevices; }
  Device *findDeviceFromTag(WbDeviceTag tag) const;

  CameraR *camera() const { return mCamera; }
  Led *led(int at) const { return mLeds[at]; }
  ServoR *servo(int at) const { return mServos[at]; }
  SingleValueSensor *servoForceFeedback(int at) const { return mServosForceFeedback[at]; }
  TripleValuesSensor *accelerometer() const { return mAccelerometer; }
  TripleValuesSensor *gyro() const { return mGyro; }

  void apply(int simulationTime);

private:
  static DeviceManager *cInstance;

  DeviceManager();
  virtual ~DeviceManager();

  void clear();

  std::vector<Device *> mDevices;

  CameraR *mCamera;
  Led *mLeds[5];
  ServoR *mServos[20];
  SingleValueSensor *mServosForceFeedback[20];
  TripleValuesSensor *mAccelerometer;
  TripleValuesSensor *mGyro;
};

#endif
