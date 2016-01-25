$LOAD_PATH.unshift File.expand_path('.', __FILE__)
require 'spec_helper'

describe XFireDB do
  it 'can process a string' do
    data = "+String result"
    res = XFireDB::Result.new(data)
    res.process

    expect(res.success?).to be true
    expect(res.data).to eq "String result"
  end

  it 'can process a boolean' do
    t = "&true"
    f = "&false"

    r1 = XFireDB::Result.new(t)
    r2 = XFireDB::Result.new(f)
    r1.process
    r2.process

    expect(r1.success?).to be true
    expect(r2.success?).to be true

    expect(r1.data).to be true
    expect(r2.data).to be false
  end

  it 'can process a numeral' do
    n = "%256"
    res = XFireDB::Result.new(n)
    res.process

    expect(res.success?).to be true
    expect(res.data).to be 256
  end

  it 'can process an acknowledgement' do
    v1 = "OK"
    v2 = "-OK"

    a1 = XFireDB::Result.new(v1)
    a2 = XFireDB::Result.new(v2)
    a1.process
    a2.process

    expect(a1.success?).to be true
    expect(a2.success?).to be true
  end

  it 'can process a nil return' do
    v1 = "nil"
    v2 = "-nil"

    a1 = XFireDB::Result.new(v1)
    a2 = XFireDB::Result.new(v2)
    a1.process
    a2.process

    expect(a1.null?).to be true
    expect(a2.null?).to be true
  end
end

