#include "Task.hpp"

#include <rtt/extras/FileDescriptorActivity.hpp>
#include <hokuyo.hh>
#include <memory>
#include <iostream>

using namespace hokuyo;
using namespace std;
using RTT::Logger;

Task::Task(std::string const& name)
    : TaskBase(name)
    , m_driver(0)
{
}

Task::~Task()
{
    delete m_driver;
}

bool Task::configureHook()
{
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

void Task::readData(bool use_external_timestamps)
{
    base::samples::LaserScan reading;
    if (!m_driver->readRanges(reading))
	return;

    if (use_external_timestamps && _timestamps.connected())
    {
        // Now, we need to synchronize the scans that we will be receiving
        // with the timestamps. What we assume here is that the timestamp
        // computed on the driver side is always in-between two device
        // ticks. Meaning that the "right" tick is between the current
        // driver timestamp and the previous driver timestamp
        //
        // We are unable to do that for the first sample, so drop the first
        // sample
        if (m_last_device.isNull() == 0)
        {
            m_last_device = reading.time;
            return;
        }

        base::Time ts;
        bool found = false;
        while(_timestamps.read(ts) == RTT::NewData)
        {
            if (ts > m_last_device && ts < reading.time)
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            // Update latency statistics and then make reading.time the external
            // timestamp
            base::Time dt = reading.time - ts;
            _latency.write(m_latency_stats.update(dt.toMilliseconds()));

            m_last_device = reading.time;
            reading.time = ts;
        }
        else
        {
            // did not find any matching timestamp. Go into degraded mode.
            error(TIMESTAMP_MISMATCH);
        }
    }
    
    if (!m_last_stamp.isNull())
    {
        base::Time dt = reading.time - m_last_stamp;
        _period.write(m_period_stats.update(dt.toMilliseconds()));
    }

    m_last_stamp = reading.time;
    _scans.write(reading);
}

void Task::updateHook()
{
    readData(true);
}

void Task::errorHook()
{
    readData(false);
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
}

