/* snapshot.c: snapshot handling routines
   Copyright (c) 1999-2012 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <libspectrum.h>

#include "fuse.h"
#include "machine.h"
#include "memory.h"
#include "module.h"
#include "settings.h"
#include "snapshot.h"
#include "ui/ui.h"
#include "utils.h"

int snapshot_read( const char *filename )
{
  utils_file file;
  libspectrum_snap *snap = libspectrum_snap_alloc();
  int error;

  error = utils_read_file( filename, &file );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_read( snap, file.buffer, file.length,
				 LIBSPECTRUM_ID_UNKNOWN, filename );
  if( error ) {
    utils_close_file( &file ); libspectrum_snap_free( snap );
    return error;
  }

  utils_close_file( &file );

  error = snapshot_copy_from( snap );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  libspectrum_snap_free( snap );

  return 0;
}

int
snapshot_read_buffer( const unsigned char *buffer, size_t length,
		      libspectrum_id_t type )
{
  libspectrum_snap *snap = libspectrum_snap_alloc();
  int error;

  error = libspectrum_snap_read( snap, buffer, length, type, NULL );
  if( error ) { libspectrum_snap_free( snap ); return error; }
    
  error = snapshot_copy_from( snap );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  libspectrum_snap_free( snap );

  return 0;
}

int
snapshot_copy_from( libspectrum_snap *snap )
{
  int error;
  libspectrum_machine machine;

  module_snapshot_enabled( snap );

  machine = libspectrum_snap_machine( snap );

  settings_current.late_timings = libspectrum_snap_late_timings( snap );

  if( machine != machine_current->machine ) {
    error = machine_select( machine );
    if( error ) {
      ui_error( UI_ERROR_ERROR,
		"Loading a %s snapshot, but that's not available",
		libspectrum_machine_name( machine ) );
    }
  } else {
    machine_reset( 0 );
  }

  module_snapshot_from( snap );

  /* Need to reset memory_map_[read|write] after all modules have had a turn
     initialising from the snapshot */
  machine_current->memory_map();

  return 0;
}

int snapshot_write( const char *filename )
{
  libspectrum_id_t type;
  libspectrum_class_t class;
  libspectrum_snap *snap;
  libspectrum_buffer *buffer;
  int flags;

  int error;

  /* Work out what sort of file we want from the filename; default to
     .szx if we couldn't guess */
  error = libspectrum_identify_file_with_class( &type, &class, filename, NULL,
						0 );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_SNAPSHOT || type == LIBSPECTRUM_ID_UNKNOWN )
    type = LIBSPECTRUM_ID_SNAPSHOT_SZX;

  snap = libspectrum_snap_alloc();

  snapshot_copy_to( snap );

  flags = 0;
  buffer = libspectrum_buffer_alloc();
  error = libspectrum_snap_write( buffer, &flags, snap, type, fuse_creator, 0 );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS ) {
    ui_error(
      UI_ERROR_WARNING,
      "A large amount of information has been lost in conversion; the snapshot probably won't work"
    );
  } else if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS ) {
    ui_error(
      UI_ERROR_WARNING,
      "Some information has been lost in conversion; the snapshot may not work"
    );
  }

  libspectrum_snap_free( snap );

  error = utils_write_file( filename, libspectrum_buffer_get_data( buffer ),
                            libspectrum_buffer_get_data_size( buffer ) );
  if( error ) { libspectrum_buffer_free( buffer ); return error; }

  libspectrum_buffer_free( buffer );

  return 0;
}

void
snapshot_copy_to( libspectrum_snap *snap )
{
  libspectrum_snap_set_machine( snap, machine_current->machine );
  libspectrum_snap_set_late_timings( snap, settings_current.late_timings );

  module_snapshot_to( snap );
}
