#
#   XFireDB key shards
#   Copyright (C) 2015  Michel Megens <dev@michelmegens.net>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

module XFireDB
  # A key shard contains a number of the total keys, called slots.
  # All key shards of all cluster nodes combined contain every possible key.
  class KeyShard
    attr_reader :slots

    @slots = nil
    @keys = nil

    # Create a new key shard
    #
    # @param [Array] Range of this key shard.
    def initialize(range = nil)
      db = XFireDB.db
      @slots = ::Set.new
      @keys = ::Set.new

      db.each do |k, v|
        next if XFireDB.illegal_key? k
        @keys.add? k
      end

      range.nil? and return

      start = range[0]
      _end = range[1]
      start.to_i if start.class == String
      _end.to_i if _end.class == String

      while start <= _end
        @slots.add(start)
        start += 1
      end

    end

    # Add a number of slots to the key shard.
    #
    # @param [Set] slots Number of slots to be added.
    def add_slots(slots)
      @slots = @slots + slots
    end

    # Add a single slot to the key shard.
    #
    # @param [String] Slot to be added.
    def add_slot(slot)
      return nil unless slot.class == String
      @slots.add? slot
    end

    # Get the slot number of a key.
    #
    # #param [String] key 
    def KeyShard.key_to_slot(key)
      slot = XFireDB::Digest.crc16(key) % 16384
      slot.to_s
    end

    # Check if a key is included in the keyshard.
    #
    # @param [String] key Key to check.
    # @return [Boolean] true if the key is included, false otherwise.
    def include?(key)
      key = key.to_s if key.class == Fixnum
      hash = XFireDB::Digest.crc16(key) % 16384
      hash = hash.to_s
      @slots.include?(hash)
    end

    # Add a key to the keyshard.
    #
    # @param [String] Key to be added.
    def add_key(key)
      @keys.add(key) if self.include?(key)
    end

    # Delete a key from the keyshard.
    #
    # @param [String] Key to be deleted.
    def del_key(key)
      @keys.delete(key)
    end

    # Reshard a specific number of keys.
    #
    # @param [Fixnum] num Number of slots to be resharded.
    # @return [Array] An array of the removed slots and keys.
    def reshard(num)
      rm = ::Set.new
      rmkeys = ::Set.new

      @slots.delete_if { |slot|
        num -= 1
        rm.add? slot if num >= 0
        num >= 0
      }

      @keys.each do |key|
        slot = XFireDB::KeyShard.key_to_slot(key)
        rmkeys.add? key if rm.include? slot
      end

      @keys = @keys - rmkeys
      [rm, rmkeys]
    end

    # Get the number of slots in this keyshard.
    #
    # @return [Fixnum] The number of slots in this keyshard.
    def size
      @slots.size
    end
  end
end

