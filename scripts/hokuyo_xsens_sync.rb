#! /usr/bin/env ruby

#
# this test will take a log_dir and extract the logs relevant 
# to synchronising xsens and hokuyo data.
#

require 'orocos'
include Orocos
require 'simlog'
include Pocosim
require 'gnuplot'
require 'gdal/osr'

# setup coordinate transform
from = Gdal::Osr::SpatialReference.new
from.set_well_known_geog_cs('WGS84')
to   = Gdal::Osr::SpatialReference.new
to.set_well_known_geog_cs('WGS84')
to.set_utm(32, 1) # UTM band 32, north
transform = Gdal::Osr::CoordinateTransformation.new(from, to)

if !ARGV[0] then 
    puts "usage: test.rb log_dir"
    exit
end

## Helper functions
# convert quaternion to direct cosine matrix
def q_to_dcm( q_0, q_1, q_2, q_3 )
    # here q_0 is meant to be the real part
    Matrix[[2*q_0**2 + 2*q_1**2 - 1, 2*q_1*q_2 - 2*q_0*q_3, 2*q_1*q_3 + 2*q_0*q_2],
	[ 2*q_1*q_2 + 2*q_0*q_3, 2*q_0**2 + 2*q_2**2 - 1, 2*q_2*q_3 - 2*q_0*q_1],
	[ 2*q_1*q_3 - 2*q_0*q_2, 2*q_2*q_3 + 2*q_0*q_1, 2*q_0**2 + 2*q_3**2 - 1]]
end

#gets the yaw angle in rad -pi/2 .. pi/2 
def q_to_yaw( q_0, q_1, q_2, q_3 )
  Math.atan(( 2*q_1*q_2+2*q_0*q_3)/(2*q_0**2+2*q_1**2-1))
end
#gets the roll angle in rad -pi/2 .. pi/2 
def q_to_roll( q_0, q_1, q_2, q_3 )
  Math.atan(( 2*q_2*q_3+2*q_0*q_1)/(2*q_0**2+2*q_3**2-1))
end
#gets the pitch angle in rad -pi .. pi
def q_to_pitch( q_0, q_1, q_2, q_3 )
  Math.asin(2*q_1*q_3-2*q_0*q_2)
end

Orocos.initialize

# Get the streams we are interested in
imu_log = Logfiles.new File.open("#{ARGV[0]}/xsens_imu.0.log"), Orocos.registry
gps_log = Logfiles.new File.open("#{ARGV[0]}/gps.0.log"), Orocos.registry
hokuyo_log = Logfiles.new File.open("#{ARGV[0]}/hokuyo.0.log"), Orocos.registry

streams = Array.new
streams[0]  = imu_log.stream("xsens_imu.orientation_samples")
streams[1]  = gps_log.stream("gps.solution")
streams[2]  = hokuyo_log.stream("hokuyo.scans")
joint = JointStream.new(true, *streams)

hokuyo_f = File.open("hokuyo.dat", 'w')
imu_f = File.open("imu.dat", 'w')

first = 1
while step_info = joint.step
    stream_idx, time, data = step_info
    
    time_offset ||= data.time.seconds

    if stream_idx == 0
	rot = data.orientation
	time = data.time.seconds + data.time.microseconds*1e-6 - time_offset
	angle = q_to_roll( rot.re, rot.im[0], rot.im[1], rot.im[2] )
	angle2 = q_to_pitch( rot.re, rot.im[0], rot.im[1], rot.im[2] )
	imu_f.puts "#{time} #{angle} #{angle2}"	
    end
    if stream_idx == 2
	time = data.time.seconds + data.time.microseconds*1e-6 - time_offset

	r = data.ranges.to_a.slice(250,500).inject(0) { |sum,c| sum+c }
	hokuyo_f.puts "#{time} #{r}"
    end
end

hokuyo_f.close
imu_f.close
