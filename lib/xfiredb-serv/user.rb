#
#   XFireDB user
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
  class User
    attr_reader :user, :password, :authenticated

    @user = nil
    @password = nil
    @authenticated = false

    def initialize(user, pw)
      @user = user
      @password = pw
    end

    def auth
      db = XFireDB.db
      map = db['xfiredb']
      local_pw = map["user::#{@user}"]
      local_pw = BCrypt::Password.new(local_pw)
      @authenticated = local_pw == @password ? true : false
    end
  end
end

