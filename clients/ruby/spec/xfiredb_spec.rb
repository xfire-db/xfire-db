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
end

