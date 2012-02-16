require 'vizkit'
require 'orocos'
include Orocos

if !ARGV[0]
    STDERR.puts "usage: test.rb <device name>"
    exit 1
end

ENV['PKG_CONFIG_PATH'] = "#{File.expand_path("..", File.dirname(__FILE__))}/build:#{ENV['PKG_CONFIG_PATH']}"

Orocos.initialize

Orocos::Process.run 'test' do |p|
    #Orocos.log_all_ports

    # initialise hokuyo
    puts "starting hokuyo ..."
    hokuyo = p.task 'driver'

    hokuyo.port = ARGV[0]
    #hokuyo.remission_values = 1

    hokuyo.configure
    hokuyo.start
    puts "done."

    Vizkit::UiLoader.register_default_widget_for "RangeView", "/base/samples/LaserScan"
    Vizkit.display hokuyo.scans
    Vizkit.exec
    
    hokuyo.stop
end

