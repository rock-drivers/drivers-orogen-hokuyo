require 'vizkit'
require 'orocos'
include Orocos

if !ARGV[0]
    STDERR.puts "usage: test.rb <device name>"
    exit 1
end

Orocos.initialize

Orocos.run 'hokuyo::Task' => "driver" do |p|
    #Orocos.log_all_ports

    # initialise hokuyo
    puts "starting hokuyo ..."
    hokuyo = p.task 'driver'

    hokuyo.io_port = ARGV[0]
    #hokuyo.remission_values = 1

    hokuyo.configure
    hokuyo.start
    puts "done."

    widget = Vizkit.default_loader.RangeView
    widget.show
    hokuyo.scans.connect_to widget
    Vizkit.exec
    
    hokuyo.stop
end

