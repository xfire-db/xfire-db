# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'xfiredb/version'

Gem::Specification.new do |spec|
  spec.name          = "xfiredb"
  spec.version       = XFireDB::VERSION
  spec.authors       = ["Michel Megens"]
  spec.email         = ["dev@michelmegens.net"]

  spec.summary       = %q{XFireDB connector}
  spec.description   = %q{XFireDB ruby client library}
  spec.homepage      = "xfiredb.bietje.net"

  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler", "~> 1.10"
  spec.add_development_dependency "rake", "~> 10.0"
  spec.add_development_dependency "rspec"
end
