#include "Wrapper.hpp"

#include "Camera.hpp"
#include "Communication.hpp"
#include "Device.hpp"
#include "DeviceManager.hpp"
#include "DarwinOutputPacket.hpp"
#include "DarwinInputPacket.hpp"
#include "Led.hpp"
#include "Servo.hpp"
#include "Sensor.hpp"
#include "Time.hpp"

#include <QtCore/QtCore>

#include <webots/robot.h>

#include <vector>
#include <iostream>

#include <cstdlib>
#include <cstdio>

#define PORT 5023

using namespace std;

Communication *Wrapper::cCommunication = NULL;
Time *Wrapper::cTime = NULL;
bool Wrapper::cSuccess = true;

void Wrapper::init() {
  DeviceManager::instance();

  cCommunication = new Communication;
}

void Wrapper::cleanup() {
  delete cCommunication;

  DeviceManager::cleanup();
}

bool Wrapper::start(void *arg) {
  if (!arg) return false;
  cCommunication->initialize(QString((const char*)arg), PORT);
  cTime = new Time();
  cSuccess = cCommunication->isInitialized();
  return cSuccess;
}

void Wrapper::stop() {
  if (!hasFailed())
    stopActuators();

  cCommunication->cleanup();

  if (cTime) {
    delete cTime;
    cTime = NULL;
  }
}

int Wrapper::robotStep(int step) {
  // get simulation time at the beginning of this step
  int beginStepTime = cTime->currentSimulationTime();

  // apply to sensors
  DeviceManager::instance()->apply(beginStepTime);

  // setup the output packet
  DarwinOutputPacket outputPacket;
  outputPacket.apply(beginStepTime);

  // 3 trials before giving up
  for(int i=0; i<3; i++) {

    // send the output packet
    cSuccess = cCommunication->sendPacket(&outputPacket);
    if (!cSuccess) {
      cerr << "Failed to send packet to DARwIn-OP. Retry (" << (i+1) << ")..." << endl;
      continue;
    }

    // setup and receive the input packet
    int answerSize = outputPacket.answerSize();
    DarwinInputPacket inputPacket(answerSize);
    cSuccess = cCommunication->receivePacket(&inputPacket);
    if (!cSuccess) {
      cerr << "Failed to receive packet from DARwIn-OP. Retry (" << (i+1) << ")..." << endl;
      continue;
    }
    inputPacket.decode(beginStepTime, outputPacket);

    if (cSuccess) break;
  }
  if (!cSuccess) return 0;

  // Time management -> in order to be always as close as possible to 1.0x
  int newTime = cTime->currentSimulationTime();
  int static oldTime;
  double static timeStep=0;

  // calculate difference between this time step and the previous one
  int difference = (newTime-oldTime);
  if(difference > 10*step) // if time difference is to big (simulation has been stopped for example)
    difference = 10*step;  // set the difference to 10 * TimeStep
    
  // Recalculate time actual time step
  // The time step is not calculate only on one step,
  // but it take also in count the previous time step
  // with a bigger importance to the most recent
  timeStep = timeStep * 9 + difference;
  timeStep = timeStep/10;

  if((int)timeStep < step) { // the packet is sent at time
    Time::wait((step - timeStep) + 0.5);
    oldTime = cTime->currentSimulationTime();
    return 0;
  }
  else { // the delay asked is not fulfilled
    oldTime = newTime;
    return timeStep - step;
  }
}

void Wrapper::stopActuators() {
  // reset all the requests

  for (int i=0; i<5; i++) {
    Led *led = DeviceManager::instance()->led(i);
    led->resetLedRequested();
  }

  vector<Device *>::const_iterator it;
  const vector<Device *> &devices = DeviceManager::instance()->devices();
  for (it=devices.begin() ; it < devices.end(); it++) {
    Sensor *s = dynamic_cast<Sensor *>(*it);
    if (s)
      s->resetSensorRequested();
  }

  // reset actuators
  for(int i=0; i<5; i++)  // cause problem when running simulation step-by-step!
    ledSet(DeviceManager::instance()->led(i)->tag(), 0);

  // send the packet
  robotStep(0);
}

void Wrapper::setRefreshRate(WbDeviceTag tag, int rate) {
  Device *device = DeviceManager::instance()->findDeviceFromTag(tag);
  Sensor *sensor = dynamic_cast<Sensor *>(device);
  if (sensor) {
    sensor->setLastRefreshTime(0);
    sensor->setRate(rate);
  } else
    cerr << "Wrapper::setRefreshRate: unknown device" << endl;
}

void Wrapper::setMotorForceRefreshRate(WbDeviceTag tag, int rate) {
  Device *device = DeviceManager::instance()->findDeviceFromTag(tag);
  SingleValueSensor *servoForceFeedback = DeviceManager::instance()->servoForceFeedback(device->index());
  if (servoForceFeedback) {
    servoForceFeedback->setLastRefreshTime(0);
    servoForceFeedback->setRate(rate);
  } else
    cerr << "Wrapper::setRefreshRate: unknown device" << endl;
}

void Wrapper::ledSet(WbDeviceTag tag, int state) {
  Device *device = DeviceManager::instance()->findDeviceFromTag(tag);
  Led *led = dynamic_cast<Led *>(device);
  if (led) {
    led->setLedRequested();
    led->setState(state);
  }
}

void Wrapper::servoSetPosition(WbDeviceTag tag, double position) {
  Device *device = DeviceManager::instance()->findDeviceFromTag(tag);
  ServoR *servo = dynamic_cast<ServoR *>(device);
  if (servo) {
    servo->setServoRequested();
    servo->setPositionRequested();
    servo->setPosition(position);
  }
}


void Wrapper::servoSetVelocity(WbDeviceTag tag, double velocity) {
  Device *device = DeviceManager::instance()->findDeviceFromTag(tag);
  ServoR *servo = dynamic_cast<ServoR *>(device);
  if (servo) {
    servo->setServoRequested();
    servo->setVelocityRequested();
    servo->setVelocity(velocity);
  }
}

void Wrapper::servoSetAcceleration(WbDeviceTag tag, double acceleration) {
  Device *device = DeviceManager::instance()->findDeviceFromTag(tag);
  ServoR *servo = dynamic_cast<ServoR *>(device);
  if (servo) {
    servo->setServoRequested();
    servo->setAccelerationRequested();
    servo->setAcceleration(acceleration);
  }
}

void Wrapper::servoSetMotorForce(WbDeviceTag tag, double force) {
  Device *device = DeviceManager::instance()->findDeviceFromTag(tag);
  ServoR *servo = dynamic_cast<ServoR *>(device);
  if (servo) {
    servo->setServoRequested();
    servo->setMotorForceRequested();
    servo->setMotorForce(force);
  }
}

void Wrapper::servoSetForce(WbDeviceTag tag, double force) {
  Device *device = DeviceManager::instance()->findDeviceFromTag(tag);
  ServoR *servo = dynamic_cast<ServoR *>(device);
  if (servo) {
    servo->setServoRequested();
    servo->setForceRequested();
    servo->setForce(force);
  }
}
void Wrapper::servoSetControlP(WbDeviceTag tag, double p) {
  Device *device = DeviceManager::instance()->findDeviceFromTag(tag);
  ServoR *servo = dynamic_cast<ServoR *>(device);
  if (servo) {
    servo->setServoRequested();
    servo->setControlPRequested();
    servo->setControlP(p);
  }
}