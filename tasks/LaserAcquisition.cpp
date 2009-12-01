#include "LaserAcquisition.hpp"

#include <rtt/FileDescriptorActivity.hpp>
#include <hokuyo.hh>
#include <memory>
#include <iostream>

using namespace hokuyo;
using namespace std;
using Orocos::Logger;

RTT::FileDescriptorActivity* LaserAcquisition::getFileDescriptorActivity()
{ return dynamic_cast< RTT::FileDescriptorActivity* >(getActivity().get()); }

LaserAcquisition::LaserAcquisition(std::string const& name)
    : LaserAcquisitionBase(name)
    , m_driver(0)
{
}

LaserAcquisition::~LaserAcquisition()
{
    delete m_driver;
}

bool LaserAcquisition::configureHook()
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

bool LaserAcquisition::startHook()
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

void LaserAcquisition::updateHook()
{
    DFKI::LaserReadings reading;
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
            m_last_device = reading.stamp;
            return;
        }

        DFKI::Time ts;
        bool found = false;
        while(_timestamps.read(ts))
        {
            if (ts > m_last_device && ts < reading.stamp)
            {
                found = true;
                break;
            }
        }

        if (!found) // did not find any matching timestamp
            return error();

        // Update latency statistics and then make reading.stamp the external
        // timestamp
        int last_s   = ts.seconds;
        int last_us  = ts.microseconds;
        int s        = reading.stamp.seconds;
        int us       = reading.stamp.microseconds;

        int dt = ((s - last_s) * 1000 + (us - last_us) / 1000);
        _latency.write(m_latency_stats.update(dt));

        m_last_device = reading.stamp;
        reading.stamp = ts;
    }
    
    if (m_last_stamp.seconds != 0)
    {
        int last_s   = m_last_stamp.seconds;
        int last_us  = m_last_stamp.microseconds;
        int s        = reading.stamp.seconds;
        int us       = reading.stamp.microseconds;

        int dt = ((s - last_s) * 1000 + (us - last_us) / 1000);
        _period.write(m_period_stats.update(dt));
    }

    m_last_stamp = reading.stamp;
    _scans.write(reading);
}
void LaserAcquisition::errorHook()
{
    DFKI::LaserReadings reading;
    if (handle_error(*m_driver, "error", m_driver->fullReset()))
        if (handle_error(*m_driver, "reading", m_driver->readRanges(reading)))
            return recovered();

    fatal();
}

void LaserAcquisition::stopHook()
{ m_driver->stopAcquisition(); }

bool LaserAcquisition::handle_error(URG& driver, string const& phase, bool retval)
{
    if (!retval)
    {
        cerr << "Error during " << phase << ": " << driver.errorString() << endl;
        return false;
    }
    return true;
}

