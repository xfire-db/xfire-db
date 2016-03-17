$LOAD_PATH.unshift File.expand_path('.', __FILE__)
require 'spec_helper'

describe XFireDB do
  it 'has a version number' do
    expect(XFireDB::VERSION).not_to be nil
  end

  it 'is not connected' do
    x = XFireDB.new
    expect(x.connected?).to be false
  end

  it 'is able to connect and disconnect' do
    x = XFireDB.connect('localhost', 7000,
                        XFireDB::SSL | XFireDB::AUTH | XFireDB::STREAM,
                          'root', 'root')
    expect(x.class).to be XFireDB::Client
    expect(x.close).to be nil
  end

  it 'is able to sent a query' do
    client = XFireDB.connect('localhost', 7000,
                        XFireDB::SSL | XFireDB::AUTH | XFireDB::STREAM,
                          'root', 'root')
    client.query("GET key1") {|r| expect(r.data).to eq "key1 data"}
    client.close
  end

  it 'is able to handle hashmaps' do
    client = XFireDB.connect('localhost', 7000,
                        XFireDB::SSL | XFireDB::AUTH | XFireDB::STREAM,
                          'root', 'root')
    expect(client.map_add('test-map', 'hkey1', 'Test data 1')).to be true
    expect(client.map_add('test-map', 'hkey2', 'Test data 2')).to be true
    expect(client.map_size('test-map')).to eq 2

    hash = client.map_ref('test-map', 'hkey1', 'hkey2')
    expect(hash['hkey1']).to eq 'Test data 1'
    expect(hash['hkey2']).to eq 'Test data 2'

    expect(client.map_delete('test-map', 'hkey1', 'hkey2')). to eq 2
  end

  it 'is able to handle sets' do
    client = XFireDB.connect('localhost', 7000,
                        XFireDB::SSL | XFireDB::AUTH | XFireDB::STREAM,
                          'root', 'root')
    expect(client.set_add('test-set', 'set key 1', 'set key 2')).to eq 2
    expect(client.set_add('test-set', 'set key 1')).to eq 0
    expect(client.set_include?('test-set', 'set key 1')).to be true
    expect(client.set_delete('test-set', 'set key 2')).to eq 1
    expect(client.set_clear('test-set')).to be true
  end
end

