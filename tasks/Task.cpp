#include "Task.hpp"

#include <rtt/extras/FileDescriptorActivity.hpp>
#include <hokuyo.hh>
#include <memory>
#include <iostream>
#include <aggregator/TimestampEstimator.hpp>

using namespace hokuyo;
using namespace std;
using RTT::Logger;

Task::Task(std::string const& name)
    : TaskBase(name)
    , m_driver(0)
    , timestampEstimator(0)
{
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
    , m_driver(0)
    , timestampEstimator(0)
{
}

Task::~Task()
{
    delete m_driver;
    delete timestampEstimator;
}

bool Task::configureHook()
{
    //BUG the inital period is dependand on the device
    timestampEstimator =
       new aggregator::TimestampEstimator
       (base::Time::fromSeconds(20),
	base::Time::fromSeconds(0.025));

    auto_ptr<URG> driver(new URG());
    if (_rate.value() && !driver->setBaudrate(_rate.value()))
    {
        std::cerr << "failed to set the baud rate to " << _rate.get() << std::endl;
        return false;
    }

    if (!driver->open(_port.value()))
    {
        std::cerr << "failed to open the device on " << _port.get() << std::endl;
        return false;
    }

    Logger::log(Logger::Info) << driver->getInfo() << Logger::endl;
    m_driver = driver.release();
    return true;
}

bool Task::startHook()
{
    RTT::extras::FileDescriptorActivity* fd_activity =
        getActivity<RTT::extras::FileDescriptorActivity>();
    if (fd_activity)
        fd_activity->watch(m_driver->getFileDescriptor());

    if (!m_driver->startAcquisition(0, _start_step.value(), _end_step.value(), _scan_skip.value(), _merge_count.value(), _remission_values.value()))
    {
        std::cerr << "failed to start acquisition" << std::endl;
        return false;
    }

    // The timestamper may have already sent data and filled the buffer. Clear
    // it so that the next updateHook() gets a valid timestamp.
    _timestamps.clear();
    return true;
}

void Task::updateHook()
{
    base::Time ts;

    while (_timestamps.read(ts) == RTT::NewData) {
	timestampEstimator->updateReference(ts);
    }

    //read sample from hardware
    base::samples::LaserScan reading;
    if (!m_driver->readRanges(reading))
	return;

    //System time when the sample was read
    base::Time readSampleTime = base::Time::now();

    //do not use the sample time here (reading.time), as it
    //is from the hokuyo internal clock which is known to drift
    reading.time = timestampEstimator->update(readSampleTime, m_driver->getPacketCounter());

    _scans.write(reading);
    _timestamp_estimator_status.write(timestampEstimator->getStatus());
}

void Task::stopHook()
{
    RTT::extras::FileDescriptorActivity* fd_activity =
        getActivity<RTT::extras::FileDescriptorActivity>();
    if (fd_activity)
        fd_activity->clearAllWatches();

    m_driver->stopAcquisition();
}
void Task::cleanupHook()
{
    delete m_driver;
    m_driver = 0;
    delete timestampEstimator;
    timestampEstimator = 0;
}

