require 'orocos'
include Orocos

if !ARGV[0]
    STDERR.puts "usage: max_remission.rb <device name>"
    exit 1
end

ENV['PKG_CONFIG_PATH'] = "#{File.expand_path("..", File.dirname(__FILE__))}/build:#{ENV['PKG_CONFIG_PATH']}"

Orocos.initialize

Orocos::Process.spawn 'test_imu' do |p|
    #Orocos.log_all_ports

    # initialise hokuyo
    puts "starting hokuyo ..."
    hokuyo = p.task 'hokuyo'

    hokuyo.port = ARGV[0]
    hokuyo.remission_values = 1

    hokuyo.configure
    hokuyo.start
    puts "done."

    scan_reader = hokuyo.scans.reader
    loop do
	if scan_line = reader.read
	    ranges = scan_line.ranges.to_a
	    remission = scan_line.remission.to_a
	    max_rem, idx = remission.each_with_index.max

	    print % "rem: %f\trange: %f\r" % [ max_rem, ranges[idx] ]
	end
	sleep 0.01
    end
end

