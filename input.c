/* input.c: generalised input events layer for Fuse
   Copyright (c) 2004 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 49 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include "fuse.h"
#include "input.h"
#include "keyboard.h"
#include "settings.h"
#include "snapshot.h"
#include "tape.h"
#include "ui/ui.h"
#include "utils.h"
#ifdef USE_WIDGET
#include "widget/widget.h"
#endif				/* #ifdef USE_WIDGET */

static int keypress( const input_event_key_t *event );
static int keyrelease( const input_event_key_t *event );

int
input_event( const input_event_t *event )
{

  switch( event->type ) {

  case INPUT_EVENT_KEYPRESS: return keypress( &( event->types.key ) );
  case INPUT_EVENT_KEYRELEASE: return keyrelease( &( event->types.key ) );

  }

  ui_error( UI_ERROR_ERROR, "unknown input event type %d", event->type );
  return 1;

}

static int
keypress( const input_event_key_t *event )
{
  input_key key;
  const keysyms_key_info *ptr;

  key = event->key;

#ifdef USE_WIDGET
  if( widget_level >= 0 ) {
    widget_keyhandler( key );
    return 0;
  }
#endif				/* #ifdef USE_WIDGET */

  ptr = keysyms_get_data( key );

  if( ptr ) {
    keyboard_press( ptr->key1 );
    keyboard_press( ptr->key2 );
  }

#ifdef USE_WIDGET
  switch( key ) {
  case INPUT_KEY_F1:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_MENU, &widget_menu_main );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F2:
    fuse_emulation_pause();
    snapshot_write( "snapshot.z80" );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F3:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_FILESELECTOR, NULL );
    if( widget_filesel_name ) {
      utils_open_file( widget_filesel_name, settings_current.auto_load, NULL );
      free( widget_filesel_name );
      display_refresh_all();
    }
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F4:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_GENERAL, NULL );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F5:
    machine_reset();
    break;
  case INPUT_KEY_F6:
    fuse_emulation_pause();
    tape_write( "tape.tzx" );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F7:
    fuse_emulation_pause();
    widget_apply_to_file( tape_open_default_autoload );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F8:
    tape_toggle_play();
    break;
  case INPUT_KEY_F9:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_SELECT, NULL );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F10:
    fuse_exiting = 1;
    break;

  default: break;		/* Remove gcc warning */

  }
#endif				/* #ifdef USE_WIDGET */

  return 0;
}

static int
keyrelease( const input_event_key_t *event )
{
  const keysyms_key_info *ptr;

  ptr = keysyms_get_data( event->key );

  if( ptr ) {
    keyboard_release( ptr->key1 );
    keyboard_release( ptr->key2 );
  }

  return 0;
}