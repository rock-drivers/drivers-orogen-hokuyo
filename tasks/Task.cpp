#include "Task.hpp"

#include <rtt/extras/FileDescriptorActivity.hpp>
#include <hokuyo.hh>
#include <memory>
#include <iostream>
#include <TimestampSynchronizer.hpp>

using namespace hokuyo;
using namespace std;
using RTT::Logger;

Task::Task(std::string const& name)
    : TaskBase(name)
    , m_driver(0)
    , timestamp_synchronizer(0)
{
}

Task::~Task()
{
    delete m_driver;
    delete timestamp_synchronizer;
}

bool Task::configureHook()
{
   timestamp_synchronizer =
       new aggregator::TimestampSynchronizer<base::samples::LaserScan>
       (base::Time::fromSeconds(0.10),
	//the time needed to capture a full scan is subtracted when pushing the
	//item into the synchronizer
	base::Time::fromSeconds(0),
	base::Time::fromSeconds(0.025),
	base::Time::fromSeconds(20),
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
    base::samples::LaserScan reading;
    base::Time now = base::Time::now();
    if( m_driver->readRanges(reading) ) {
	if (!m_last_device.isNull()) {
	    //assume about 25ms period
	    int lost = (int)((reading.time - m_last_device).toSeconds() /
			     0.025 - 0.5) - 1;
	    if (lost > 0)
		timestamp_synchronizer->lostItems(lost);
	}
	m_last_device = reading.time;
	//this accounts for the time needed to take the sample, as
	//we only get the timestamp after that
	timestamp_synchronizer->pushItem
	    (reading, now - base::Time::fromSeconds(0.025/8*7));
    }

    base::Time ts;

    while (_timestamps.read(ts) == RTT::NewData)
	timestamp_synchronizer->pushReference(ts);


    while(timestamp_synchronizer->fetchItem(reading,ts,now)) {
	reading.time = ts;

	if (!m_last_stamp.isNull())
	    {
		base::Time dt = reading.time - m_last_stamp;
		_period.write(m_period_stats.update(dt.toMilliseconds()));
	    }

	m_last_stamp = reading.time;
	_scans.write(reading);
    }
}

//void Task::errorHook()
//{
//}
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
    delete timestamp_synchronizer;
    timestamp_synchronizer = 0;
}

