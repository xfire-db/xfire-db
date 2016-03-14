/*
 *  XFireDB interface
 *  Copyright (C) 2015   Michel Megens <dev@michelmegens.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <ruby.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/crc.h>

#include "se.h"

/*
 * Document-method: crc16
 *
 * Hash a string into a Fixnum using the CRC16 hashing algorithm. The
 * resulting number is a 16-bit number.
 */
VALUE rb_digest_crc16(VALUE klass, VALUE str)
{
	const char *tmp = StringValueCStr(str);
	u16 crc;

	crc = xfiredb_crc16(tmp);
	return INT2NUM(crc);
}

VALUE c_digest;
extern VALUE c_xfiredb_mod;

/*
 * Document-class: Digest
 *
 * Crypto class to bring crc16 support to ruby.
 */
void init_digest(void)
{
	c_digest = rb_define_class_under(c_xfiredb_mod, "Digest",
					rb_cObject);

	rb_define_singleton_method(c_digest, "crc16", rb_digest_crc16, 1); 
}

