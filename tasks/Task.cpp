#include "Task.hpp"

#include <rtt/FileDescriptorActivity.hpp>
#include <hokuyo.hh>
#include <memory>
#include <iostream>

using namespace hokuyo;
using namespace std;
using Orocos::Logger;


RTT::FileDescriptorActivity* Task::getFileDescriptorActivity()
{ return dynamic_cast< RTT::FileDescriptorActivity* >(getActivity().get()); }


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
    if (_rate.value() && !handle_error(*driver, "setBaudRate", driver->setBaudrate(_rate.value())))
        return false;
    if (!handle_error(*driver, "open", driver->open(_port.value())))
        return false;

    Logger::log(Logger::Info) << driver->getInfo() << Logger::endl;
    m_driver = driver.release();

    getFileDescriptorActivity()->watch(m_driver->getFileDescriptor());
    return true;
}

bool Task::startHook()
{
    if (!handle_error(*m_driver, "start",
                m_driver->startAcquisition(0, _start_step.value(), 
                    _end_step.value(), _scan_skip.value(), _merge_count.value(), _remission_values.value())
            ))
        return false;

    // The timestamper may have already sent data and filled the buffer. Clear
    // it so that the next updateHook() gets a valid timestamp.
    _timestamps.clear();
    return true;
}

void Task::updateHook()
{
    base::samples::LaserScan reading;
    if (!m_driver->readRanges(reading))
	return;

    if (_timestamps.connected())
    {
        // Now, we need to synchronize the scans that we will be receiving
        // with the timestamps. What we assume here is that the timestamp
        // computed on the driver side is always in-between two device
        // ticks. Meaning that the "right" tick is between the current
        // driver timestamp and the previous driver timestamp
        //
        // We are unable to do that for the first sample, so drop the first
        // sample
        if (m_last_device.seconds == 0)
        {
            m_last_device = reading.time;
            return;
        }

        base::Time ts;
        bool found = false;
        while(_timestamps.read(ts))
        {
            if (ts > m_last_device && ts < reading.time)
            {
                found = true;
                break;
            }
        }

        if (!found) // did not find any matching timestamp
            return error();

        // Update latency statistics and then make reading.time the external
        // timestamp
        int last_s   = ts.seconds;
        int last_us  = ts.microseconds;
        int s        = reading.time.seconds;
        int us       = reading.time.microseconds;

        int dt = ((s - last_s) * 1000 + (us - last_us) / 1000);
        _latency.write(m_latency_stats.update(dt));

        m_last_device = reading.time;
        reading.time = ts;
    }
    
    if (m_last_stamp.seconds != 0)
    {
        int last_s   = m_last_stamp.seconds;
        int last_us  = m_last_stamp.microseconds;
        int s        = reading.time.seconds;
        int us       = reading.time.microseconds;

        int dt = ((s - last_s) * 1000 + (us - last_us) / 1000);
        _period.write(m_period_stats.update(dt));
    }

    m_last_stamp = reading.time;
    _scans.write(reading);
}

void Task::errorHook()
{
    base::samples::LaserScan reading;
    if (handle_error(*m_driver, "error", m_driver->fullReset()))
        if (handle_error(*m_driver, "reading", m_driver->readRanges(reading)))
            return recovered();

    fatal();
}
void Task::stopHook()
{
    m_driver->stopAcquisition();
}
void Task::cleanupHook()
{
    getFileDescriptorActivity()->unwatch(m_driver->getFileDescriptor());
    delete m_driver;
    m_driver = 0;
}

bool Task::handle_error(URG& driver, string const& phase, bool retval)
{
    if (!retval)
    {
        cerr << "Error during " << phase << ": " << driver.errorString() << endl;
        return false;
    }
    return true;
}

