#include <EEPROM.h>

//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//

#include "project.h"


//
// cmd_rmode()
//
// Read mode word and current limit from EEPROM
//
void cmd_rmode(void) {
  mode_word.byte = EEPROM.read(EE_MW);
  imax = ee_read_short(EE_IMAX);
  inactiveTimeout = ee_read_short(EE_INACTIVE_TIMEOUT);
  activeTimeout = ee_read_short(EE_ACTIVE_TIMEOUT);
}

//
// cmd_wmode()
//
// Write mode word and current limit to EEPROM
//
void cmd_wmode(void) {
  ee_write_short(EE_IMAX, imax);
  ee_write_short(EE_INACTIVE_TIMEOUT, inactiveTimeout);
  ee_write_short(EE_ACTIVE_TIMEOUT, activeTimeout);
  EEPROM.update(EE_MW, mode_word.byte);
  EEPROM.update(EE_MAGIC, MAGIC);
}

