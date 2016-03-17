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
    expect(client.set("test-key", "Test key data")).to be true
    expect(client.get("test-key")).to eq "Test key data"
    expect(client.delete("test-key")).to be true
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

    expect(client.map_delete('test-map', 'hkey1', 'hkey2')).to eq 2
    client.close
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
    client.close
  end

  it 'is able to handle lists' do
    data = ["List data 1", "List data 2", "List data 3"]
    client = XFireDB.connect('localhost', 7000,
                        XFireDB::SSL | XFireDB::AUTH | XFireDB::STREAM,
                          'root', 'root')
    data.each do |entry|
      expect(client.list_push('test-list', entry)).to be true
    end

    #expect(client.list_size('test-list')).to be 3
    expect(client.list_ref('test-list', '0..-1')).to match_array(data)
    expect(client.list_set('test-list', 1, 'List data 3')).to be true
    expect(client.list_pop('test-list', '0..1')).to match_array(["List data 1", "List data 3"])
    expect(client.list_clear('test-list')).to be true
    client.close
  end
end

