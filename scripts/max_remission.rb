require 'orocos'
include Orocos

if !ARGV[0]
    STDERR.puts "usage: max_remission.rb <device name>"
    exit 1
end

ENV['PKG_CONFIG_PATH'] = "#{File.expand_path("..", File.dirname(__FILE__))}/build:#{ENV['PKG_CONFIG_PATH']}"

Orocos.initialize

Orocos::Process.spawn 'test' do |p|
    #Orocos.log_all_ports

    # initialise hokuyo
    puts "starting hokuyo ..."
    hokuyo = p.task 'driver'

    hokuyo.port = ARGV[0]
    hokuyo.remission_values = 1

    hokuyo.configure
    hokuyo.start
    puts "done."

    scan_reader = hokuyo.scans.reader
    loop do
	if scan_line = scan_reader.read
	    ranges = scan_line.ranges.to_a
	    remission = scan_line.remission.to_a
	    max_rem, idx = remission.each_with_index.max
            angle = (scan_line.start_angle + scan_line.angular_resolution * idx)/Math::PI*180.0

	    print "idx: %f\tangle: %f\trem: %f\trange: %f\r" % [ idx, angle, max_rem, ranges[idx] ]
	end
	sleep 0.01
    end
end

