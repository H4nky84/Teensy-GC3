//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//
//	Pete Brownlow	27/6/11	Heartbeat is now half second, counts adjusted so slot timeout unchanged

//#include "project.h"

//#pragma udata Q_QUEUE
// Linker script puts this in GPR1
// 32 entry Q queue for regularly refreshed loco packets
refresh_queue_t q_queue[MAX_HANDLES];

//dispatch queue
dispatch_queue_t d_queue[MAX_HANDLES];

//#pragma udata S_QUEUE
// Linker script puts this in GPR2
// 16 entry S queue for one off packets
dcc_queue_t s_queue[16];

// indices for the queues
unsigned char q_idx;
unsigned char q_state;
unsigned char s_head;
unsigned char s_tail;
unsigned char idle_next;



void send_s(void);
void send_q(void);
void send_idle(void);

//
// Scale 128 step format speed to 28 step format
//
unsigned char step28(unsigned char spd) {
    unsigned char ret = 0b01000000;         // reverse
    if (spd > 0x7f) {
        ret = ret | 0b00100000;             // forwards
    }
    if ((spd & 0x7f) == 1) {
        return ret | 0x01;                  // Em stop
    }
    ret = ret | ((spd & 0x7f)>>3);          // shift 4 MSBs
    if (spd & 0b00000100) {
        ret = ret | 0x10;                   // Put LSB in bit 4
    }
    return ret;
}

//
// Scale 128 step format speed to 14 step format
//
unsigned char step14(unsigned char spd) {
    unsigned char ret = 0b01000000;         // reverse
    if (spd & 128) {
        ret = ret | 0b00100000;             // forwards
    }
    if ((spd & 0x7f) == 1) {
        return ret | 0x01;                  // Em stop
    }
    return ret | ((spd & 0x7f)>>3);         // shift 4 MSBs
}

//
// em_stop()
//
void em_stop(void) {
    unsigned char i;
    // Set bit for broadcast for next packet
    dcc_flags.dcc_em_stop = 1;
    // Set all sessions to Emergency stop, preserving direction bit.
	for (i = 0; i < MAX_HANDLES; i++) {
        q_queue[i].speed = (q_queue[i].speed & 0x80) | 1;
    }      
}

//
// broadcast_stop(void)
//
// Send 6 broadcast emergency stop packets
//
void broadcast_stop(void) {
    dcc_buff_m[0] = 0;
    dcc_buff_m[1] = 0b01110001;
    dcc_buff_m[2] = 0b01110001;
    dcc_buff_m[6] = 6;            // repeat 6 times
    dcc_bytes_m = 3;
	// hand off buffer	
    dcc_flags.dcc_rdy_m = 0;
}

//
// packet_gen()
//
// Called from the input wait loop to handle main track packet
// generation.
//
// 14/28 speed step is a 3 or 4 byte packet <address> <01DUSSSS> <error>
// ought to be able to set U to FL control
// 128 speed step is a 4 or 5 byte packet <Address> <00111111> <speed>
// <error>
//
// <Address> can be 7 bit in a single byte <0aaaaaaa> or 14 bit in two 
// bytes <11aaaaaa> <aaaaaaaa>. The first byte of a 14 bit address has
// a valid range of 0xc0 to 0xe7.
//
// Packets in the S queue have priority to ensure we respond to throttle
// speed changes as soon as possible.
//
// Loco speed/dir is refreshed every time round.
//
void packet_gen(void) {
  
    if (dcc_flags.dcc_em_stop == 1) {
        if (analogOperationActive == 1){
          analogWrite(DCC_NEG, 0);
          analogWrite(DCC_POS, 0);   
        }  else {
        // Broadcast emergency stop
        broadcast_stop();
        dcc_flags.dcc_em_stop = 0;
        }
    } else if (idle_next == 0) {
        if (s_queue[s_tail].status.valid == 1) {
            // Send an immediate update
            send_s();
        } else {
            // Send something from the refresh stack
            send_q();
        }
    } else {
        // Send an idle
        send_idle();
    }
}

//
// queue_add()
//
// Attempt to add an entry to the Q queue in response to OPC_RLOC
//
// The queue may only have one packet for a given decoder address, which
// must be greater than 0. If the address already exists in the queue, or 
// the queue is full, then an error is returned. Otherwise the queue 
// entry is created and a handle is returned to the cab.
// 
// The queue index is the session handle.
//
void queue_add() {
	unsigned char i, free_handle, err;
	unsigned int addr = ((unsigned int)rx_ptr.buf[1]<<8) | rx_ptr.buf[2];
  // Ignore attempts to use address 0 (short addressing)
  if (addr == 0) {
    return;
  }
  //If it is long address 0 and the analog mode is enabled, then activate the analog mode and block the queue
  if (addr == 0xC000) {
    if ((mode_word.analog_en) && (noOfSessions == 0) && (SWAP_OP == 0)) {
      //turn off railcom operaiton and enable analog operation
      railcomEnabled = 0;
      analogOperationActive = 1;
      // free_handle is the handle of available entry
      // Initialise the entry
      q_queue[0].address.addr_int = addr;
      // Ensure correct DCC address format
      q_queue[0].address.addr_hi.long0 = q_queue[0].address.addr_hi.long1;
      q_queue[0].status.valid = 1;
      // Report to cab
      Tx1.buf[0] = OPC_PLOC;
      Tx1.buf[1] = 0;
      Tx1.buf[2] = rx_ptr.buf[1];
      Tx1.buf[3] = rx_ptr.buf[2];
      Tx1.buf[4] = q_queue[0].speed;
      Tx1.buf[5] = q_queue[0].fn1;
      Tx1.buf[6] = q_queue[0].fn2;
      Tx1.buf[7] = q_queue[0].fn2a;
      can_tx(8);
      noOfSessions ++;
      railComIcon();
    }
    //not valid for any other situation
    return;
  }
  
  // Find free entry or match address
  free_handle = 255;
  i = 0;
  err = ERR_LOCO_STACK_FULL;
  while (i < MAX_HANDLES) {
    if ((q_queue[i].status.valid == 0) && (free_handle == 255)) {
        // Found first free entry, save it for later
        free_handle = i;
        err = 0;
    } else if (q_queue[i].address.addr_int == addr) {
        // Found same address in valid slot - error
        err = ERR_LOCO_ADDR_TAKEN;
        //check to see fi this loco is in the dispatch queue
        if (d_queue[i].status.valid) {
          err = 0xFF; //set the error code to 255 (undefined in standard)
          purge_dispatch(i);  //remove from the dispatch queue as it has been taken over by a real cab
          //return the slot index for use by that cab
        }
        break;
    }
    i++;
  }

  if (analogOperationActive == 1) {
    err = ERR_LOCO_STACK_FULL;
  }
  
	if (err == 0) {
		// free_handle is the handle of available entry
    // Initialise the entry
    q_queue[free_handle].address.addr_int = addr;
    // Ensure correct DCC address format
    q_queue[free_handle].address.addr_hi.long0 = q_queue[free_handle].address.addr_hi.long1;
	  q_queue[free_handle].status.valid = 1;
    // Report to cab
    Tx1.buf[0] = OPC_PLOC;
    Tx1.buf[1] = free_handle;
    Tx1.buf[2] = rx_ptr.buf[1];
    Tx1.buf[3] = rx_ptr.buf[2];
    Tx1.buf[4] = q_queue[free_handle].speed;
    Tx1.buf[5] = q_queue[free_handle].fn1;
    Tx1.buf[6] = q_queue[free_handle].fn2;
    Tx1.buf[7] = q_queue[free_handle].fn2a;
    can_tx(8);
    noOfSessions ++;

    /*
    tft.fillRect(188, 60+50, 85, 40, ILI9341_WHITE);
    tft.setCursor(30, 60 + 55);
    tft.setTextColor(ILI9341_BLACK);
    tft.setFont(Arial_16);
    tft.print("Session = ");
    tft.print(free_handle);
    */
        
	} else if (err == 0xFF) {
    // this entry exists but is in the dispatch queue so can be taken over
    // Report to cab
    Tx1.buf[0] = OPC_PLOC;
    Tx1.buf[1] = i;
    Tx1.buf[2] = highByte(q_queue[i].address.addr_int);
    Tx1.buf[3] = lowByte(q_queue[i].address.addr_int);
    Tx1.buf[4] = q_queue[i].speed;
    Tx1.buf[5] = q_queue[i].fn1;
    Tx1.buf[6] = q_queue[i].fn2;
    Tx1.buf[7] = q_queue[i].fn2a;
    can_tx(8);
    //noOfSessions ++;

    /*
    tft.fillRect(188, 60+50, 85, 40, ILI9341_WHITE);
    tft.setCursor(30, 60 + 55);
    tft.setTextColor(ILI9341_BLACK);
    tft.setFont(Arial_16);
    tft.print("Session = ");
    tft.print(free_handle);
    */
        
  } else {
    // Report error code
    Tx1.buf[0] = OPC_ERR;
    Tx1.buf[1] = rx_ptr.buf[1];
    Tx1.buf[2] = rx_ptr.buf[2];
    Tx1.buf[3] = err;
    can_tx(4);
  }
}

//
// throttle_mode()
//
// Set the throttle speed step mode in response to OPC_STMOD. 128, 28 and 14
// speed steps are supported. 28 with interleaved steps will send 28 steps.
//
void throttle_mode(void) {
    if (q_queue[rx_ptr.buf[1]].status.valid == 1) {
        // Handle is valid so update it's mode
        q_queue[rx_ptr.buf[1]].status.throttle_mode = rx_ptr.buf[2] & TMOD_SPD_MASK;
    }
}

//
// queue_update()
//
// Update speed/dir or function state in response to OPC_DSPD or OPC_DFUNx
// for handle and add an immediate speed/dir or function update to the S 
// queue
//
void queue_update(void) {
    unsigned char i = 0;
    unsigned char speed = 0;
    if (analogOperationActive) {
      if (q_queue[rx_ptr.buf[1]].status.valid == 1) {
          // Reset slot timeout 
        q_queue[rx_ptr.buf[1]].timeout = activeTimeout;
        if (rx_ptr.buf[0] == OPC_DSPD) {
          speed = rx_ptr.buf[2] & 0x7F;
          if (speed < 2) speed = 0;
          else {
            speed = map(speed, 0, 127, 30, 255);
          }

          if (op_flags.op_pwr_m) {
            digitalWriteFast(DCC_EN, 1);
            if (bitRead(rx_ptr.buf[2], 7)) {
                analogWrite(DCC_NEG, 0);
                analogWrite(DCC_POS, speed);
            } else {
                analogWrite(DCC_POS, 0);
                analogWrite(DCC_NEG, speed);
            }
          } else {
            digitalWriteFast(DCC_EN, 0);
          }
        }
      }
    } else {
    
    
    // get address of next s queue entry
    dcc_queue_t * s_ptr = &(s_queue[s_head]);

    // Only update for valid slots
    if (q_queue[rx_ptr.buf[1]].status.valid == 1) {
        // Reset slot timeout 
        q_queue[rx_ptr.buf[1]].timeout = activeTimeout;
        if (rx_ptr.buf[0] == OPC_DSPD) {
            // Put speed/dir update in refresh queue
            speed = rx_ptr.buf[2];
            q_queue[rx_ptr.buf[1]].speed = speed;
        } else {
            // DFUN function packet - update refresh queue
            switch (rx_ptr.buf[2]) {
                // Fn0 - 12 go in refresh queue
                case 1:
                    q_queue[rx_ptr.buf[1]].fn1 = rx_ptr.buf[3];
                    break;
    
                case 2:
                    q_queue[rx_ptr.buf[1]].fn2 = rx_ptr.buf[3];
                    break;
    
                case 3:
                    q_queue[rx_ptr.buf[1]].fn2a = rx_ptr.buf[3];
                    break;
    
                default:
                    break;
            }
        }
        // Add immediate packet to S with new speed if possible
        if (s_ptr->status.valid == 0) {
            s_ptr->d[5] = 0;                            // clear error byte
            if (q_queue[rx_ptr.buf[1]].address.addr_hi.long1 == 1) {
                // Put long address in
                s_ptr->d[i] = q_queue[rx_ptr.buf[1]].address.addr_hi.byte;
                s_ptr->d[5] = s_ptr->d[i++];
                s_ptr->d[i] = q_queue[rx_ptr.buf[1]].address.addr_lo;
                s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
           } else {
                // Put short address in
                s_ptr->d[i] = q_queue[rx_ptr.buf[1]].address.addr_lo;
                s_ptr->d[5] = s_ptr->d[i++];
            }
            if (rx_ptr.buf[0] == OPC_DSPD) {
                switch (q_queue[rx_ptr.buf[1]].status.throttle_mode) {
                    case TMOD_SPD_128:
                        s_ptr->d[i] = 0x3F;
                        s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
                        // Speed byte
                        s_ptr->d[i] = speed;
                        break;
    
                    case TMOD_SPD_28:
                    case TMOD_SPD_28I:
                        // 28 speed steps
                        speed = step28(speed);
                        s_ptr->d[i] = speed;
                        s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
                        break;
    
                    case TMOD_SPD_14:
                        // 14 speed steps
                        speed = step14(speed);
                        // *** Put Fn0 into speed byte
                        s_ptr->d[i] = speed;
                        s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
                        break;
    
                }
                s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
                s_ptr->repeat = 1;              // Send speed once
            } else {
                // DFUN function group packet
                switch (rx_ptr.buf[2]) {
                    case 1:
                        // Fn0 - 4 
                        s_ptr->d[i] = 0x80 | (rx_ptr.buf[3] & 0x1F); 
                        break;
        
                    case 2:
                        // Fn5 - 8 
                        s_ptr->d[i] = 0xB0 | (rx_ptr.buf[3] & 0x0F); 
                        break;
        
                    case 3:
                        // Fn9 - 12 
                        s_ptr->d[i] = 0xA0 | (rx_ptr.buf[3] & 0x0F); 
                        break;
        
                    case 4:
                        // Fn13 - 20 
                        s_ptr->d[i] = 0xDE; 
                        s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
                        s_ptr->d[i] = rx_ptr.buf[3]; 
                        break;
        
                    case 5:
                        // Fn13 - 20 
                        s_ptr->d[i] = 0xDF; 
                        s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
                        s_ptr->d[i] = rx_ptr.buf[3]; 
                        break;
    
                    default:
                        break;
                }
                s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
                s_ptr->repeat = 2;              // Send Fn control twice
            }
            // Error byte
            s_ptr->d[i++] = s_ptr->d[5];
            s_ptr->status.byte_count = i;
            // Mark as valid and point to next entry
            s_ptr->status.valid = 1;
            s_head = (s_head + 1) & 0x0f;
        } else {
            // No rooom yet
            mode_word.s_full = 1;
        }
    } else {
        // Slot was invalid
        ;
    }
   }
}

//
// query_session
//
// Query loco by session number
//
void query_session(void) {
    if (q_queue[rx_ptr.buf[1]].status.valid == 1) {
        // Valid session so respond with PLOC
        Tx1.buf[0] = OPC_PLOC;
        Tx1.buf[1] = rx_ptr.buf[1];
        Tx1.buf[2] = q_queue[rx_ptr.buf[1]].address.addr_hi.byte;
        Tx1.buf[3] = q_queue[rx_ptr.buf[1]].address.addr_lo;
        Tx1.buf[4] = q_queue[rx_ptr.buf[1]].speed;
        Tx1.buf[5] = q_queue[rx_ptr.buf[1]].fn1;
        Tx1.buf[6] = q_queue[rx_ptr.buf[1]].fn2;
        Tx1.buf[7] = q_queue[rx_ptr.buf[1]].fn2a;
        can_tx(8);
    } else {
        // Invalid session so respond with error session not found
        // Can't give an address so send requesting handle in byte 1
        Tx1.buf[0] = OPC_ERR;
        Tx1.buf[1] = rx_ptr.buf[1];
        Tx1.buf[2] = 0;
        Tx1.buf[3] = ERR_SESSION_NOT_PRESENT;
        can_tx(4);
    }
}

//
// purge_session
//
// Purge loco session from the queue
//
void purge_session(void) {
    if (q_queue[rx_ptr.buf[1]].status.valid == 1) noOfSessions --;
    rx_ptr.buf[1] &= 0x3f;
    q_queue[rx_ptr.buf[1]].status.valid = 0;
    q_queue[rx_ptr.buf[1]].address.addr_int = 0;
    q_queue[rx_ptr.buf[1]].speed = 0x80;
    q_queue[rx_ptr.buf[1]].fn1 = 0;
    q_queue[rx_ptr.buf[1]].fn2 = 0;
    q_queue[rx_ptr.buf[1]].fn2a = 0;
    q_queue[rx_ptr.buf[1]].timeout = 0;
    if (analogOperationActive) {
      pinMode(DCC_NEG, OUTPUT);
      pinMode(DCC_POS, OUTPUT);
      analogOperationActive = 0;
      railcomEnabled = mode_word.railcom;
      //analogIcon();
      railComIcon();
    }
    noOfSessions = constrain(noOfSessions, 0, 0x3F);
}

//
// purge_session
//
// Purge loco session from the queue
//
void purge_session(unsigned char idx) {
    if (q_queue[idx].status.valid == 1) noOfSessions --;
    idx &= 0x3f;
    q_queue[idx].status.valid = 0;
    q_queue[idx].address.addr_int = 0;
    q_queue[idx].speed = 0x80;
    q_queue[idx].fn1 = 0;
    q_queue[idx].fn2 = 0;
    q_queue[idx].fn2a = 0;
    q_queue[idx].timeout = 0;
    if (analogOperationActive) {
      pinMode(DCC_NEG, OUTPUT);
      pinMode(DCC_POS, OUTPUT);
      analogOperationActive = 0;
      railcomEnabled = mode_word.railcom;
      //analogIcon();
      railComIcon();
    }
    noOfSessions = constrain(noOfSessions, 0, 0x3F);
    //noOfSessions --;
}

//
// keep_alive()
//
// bump session timeout
//
void keep_alive(void) {
    q_queue[rx_ptr.buf[1]].timeout = 80;
}

//
// dcc_packet()
//
// Send a DCC packet
//
void dcc_packet(void) {
    // get address of next s queue entry
    dcc_queue_t * s_ptr = &(s_queue[s_head]);
    // Add to next S queue entry if possible
    if (s_ptr->status.valid == 0) {
        switch (rx_ptr.buf[0]) {
            case OPC_RDCC6:
                s_ptr->d[5] = rx_ptr.buf[7];
                // fall through
            case OPC_RDCC5:
                s_ptr->d[4] = rx_ptr.buf[6];
                // fall through
            case OPC_RDCC4:
                s_ptr->d[3] = rx_ptr.buf[5];
                // fall through
            case OPC_RDCC3:
                s_ptr->d[2] = rx_ptr.buf[4];
                s_ptr->d[1] = rx_ptr.buf[3];
                s_ptr->d[0] = rx_ptr.buf[2];
                s_ptr->repeat = rx_ptr.buf[1] & 0x7;
                break;

            default:
                break;
        }
        // Byte count is in OPC
        s_ptr->status.byte_count = ((rx_ptr.buf[0] & 0x70)>>5) - 1;
        // Mark as valid and point to next entry
        s_ptr->status.valid = 1;
        s_head = (s_head + 1) & 0x0f;
    } else {
        // No room yet
        mode_word.s_full = 1;
    }
}

//
// consist_add()
//
// Add ops mode programming commands to S queue to put loco in a 
// consist by writing to CV19. Consist address of zero will remove
// the loco from the consist
//
void consist_add(void) {
    // CV address for Consist Address
    cv = 19 - 1;
    // Consist address
    cv_data = rx_ptr.buf[2];
    ops_write();
}

//
// ops_write()
//
// Add an ops mode direct bit or byte write to the S queue if possible.
// The session number is used to look up the address in the Q queue.
//
// The packet is repeated twice
//
void ops_write(void) {
    unsigned char i = 0;

    // get address of next s queue entry
    dcc_queue_t * s_ptr = &(s_queue[s_head]);

    // Add to next S queue entry if possible
    if (s_ptr->status.valid == 0) {
        s_ptr->d[5] = 0;                                // clear error byte
        if (q_queue[rx_ptr.buf[1]].address.addr_hi.long1 == 1) {
            // Put long address in
            s_ptr->d[i] = q_queue[rx_ptr.buf[1]].address.addr_hi.byte;
            s_ptr->d[5] = s_ptr->d[i++];
            s_ptr->d[i] = q_queue[rx_ptr.buf[1]].address.addr_lo;
            s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
       } else {
            // Put short address in
            s_ptr->d[i] = q_queue[rx_ptr.buf[1]].address.addr_lo;
            s_ptr->d[5] = s_ptr->d[i++];
        }
        if ((rx_ptr.buf[0] == OPC_WCVO) || (rx_ptr.buf[0] == OPC_PCON)) {
            // Instruction byte for CV access long form for direct byte
            s_ptr->d[i] = 0b11101100 | ((cv>>8) & 0x3);
        } else {
            // Instruction byte for CV access long form for direct bit
            s_ptr->d[i] = 0b11101000 | ((cv>>8) & 0x3);
        }
        s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
        // CV LSBs
        s_ptr->d[i] = cv & 0xFF;
        s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
        // Data will be data byte (WCVO or PCON) or bit manipulation instruction (WCVB)
        s_ptr->d[i] = cv_data;
        s_ptr->d[5] = s_ptr->d[5] ^ s_ptr->d[i++];
        // Error byte
        s_ptr->d[i++] = s_ptr->d[5];
        s_ptr->status.byte_count = i;
        // Repeat
        s_ptr->repeat = 2;
        // Mark as valid and point to next entry
        s_ptr->status.valid = 1;
        s_head = (s_head + 1) & 0x0f;
    } else {
        // No room yet
        mode_word.s_full = 1;
    }
}

void send_s(void) {
    unsigned char i;

    // Send a packet from the S queue
    // number of bytes
	dcc_bytes_m = s_queue[s_tail].status.byte_count;
    dcc_buff_m[6] = 1;                  // send once
    // copy the packet
	for (i = 0; i < dcc_bytes_m; i++) {
		dcc_buff_m[i] = s_queue[s_tail].d[i];
	}
    if (s_queue[s_tail].repeat > 1) {
        // count repeats and force idle insertion between repetitions
        s_queue[s_tail].repeat = s_queue[s_tail].repeat - 1;
        idle_next = 1;
    } else {
        // done with this slot
        s_queue[s_tail].status.valid = 0;
        s_tail = (s_tail + 1) & 0x0f;       // wrap at 16
    }
	// hand off buffer	
    dcc_flags.dcc_rdy_m = 0;
}

void send_q(void) {
    unsigned char i;
    unsigned char speed;

	// send a packet from the Q queue
	if (q_queue[q_idx].status.valid == 1) {
        // Assemble the packet
        i = 0;
        if (q_queue[q_idx].address.addr_hi.long1 == 1) {
            // long address
            dcc_buff_m[i] = q_queue[q_idx].address.addr_hi.byte;
            dcc_buff_m[5] = dcc_buff_m[i++];
            dcc_buff_m[i] = q_queue[q_idx].address.addr_lo;
            dcc_buff_m[5] = dcc_buff_m[5] ^ dcc_buff_m[i++];
        } else {
            // short address
            dcc_buff_m[i] = q_queue[q_idx].address.addr_lo;
            dcc_buff_m[5] = dcc_buff_m[i++];
        }
        switch (q_state) {
            case (0):
                // Function group 1
                dcc_buff_m[i] = 0x80 | q_queue[q_idx].fn1; 
                break;

            case (1):
                // Function group 2
                dcc_buff_m[i] = 0xB0 | q_queue[q_idx].fn2; 
                break;

            case (2):
                // Function group 2a
                dcc_buff_m[i] = 0xA0 | q_queue[q_idx].fn2a; 
                break;

            default:
                // Send a speed update
                speed = q_queue[q_idx].speed;
                switch (q_queue[q_idx].status.throttle_mode) {
                    case TMOD_SPD_128:
                        // 128 speed steps
                        dcc_buff_m[i] = 0x3F;
                        dcc_buff_m[5] = dcc_buff_m[5] ^ dcc_buff_m[i++];
                        // Speed byte
                        dcc_buff_m[i] = speed;
                        break;
    
                    case TMOD_SPD_28:
                    case TMOD_SPD_28I:
                        // 28 speed steps
                        speed = step28(speed);
                        dcc_buff_m[i] = speed;
                        break;
    
                    case TMOD_SPD_14:
                        // 14 speed steps
                        speed = step14(speed);
                        // *** Put Fn0 into speed byte
                        dcc_buff_m[i] = speed;
                        break;
    
                }
                break;

        }
        dcc_buff_m[5] = dcc_buff_m[5] ^ dcc_buff_m[i++];
        dcc_buff_m[i++] = dcc_buff_m[5];        // final error byte
        dcc_bytes_m = i;
        dcc_buff_m[6] = 1;                      // send once
		// hand off buffer
	    dcc_flags.dcc_rdy_m = 0;
    } else {
        // Not valid so send an idle immediately
        send_idle();
    }

    // Find next valid slot
    i = MAX_HANDLES;
    while (i > 0) {
        q_idx++;
        if (q_idx >= MAX_HANDLES) {
            // Wrap around, send an idle next time to prevent
            // back to back packets if only one loco in stack
            q_idx = 0;
            idle_next = 1;
            q_state++;
            if (q_state > 6) {
                // Cycles through 7 states
                // 4 speed refresh, 3 function refresh
                q_state = 0;
            }
        }
        i--;
        if (q_queue[q_idx].status.valid == 1) {
            // Found one
            i = 0;
        }
    }

    // Find next valid slot
//	q_idx++;
//    i = MAX_HANDLES;
//    while ((i > 0) && (q_queue[q_idx].status.valid == 0)) {
//        if (q_idx >= MAX_HANDLES) {
//            // Wrap around, send an idle next time to prevent
//            // back to back packets if only one loco in stack
//            q_idx = 0;
//            idle_next = 1;
//            q_state++;
//            if (q_state > 6) {
//                // Cycles through 7 states
//                // 4 speed refresh, 3 function refresh
//                q_state = 0;
//            }
//        }
//        i--;
//    }
//    // Point to next slot
//    q_idx++;
//    if (q_idx >= MAX_HANDLES) {
//        // Wrap around
//        q_idx = 0;
//        q_state++;
//        if (q_state > 6) {
//            // Cycles through 7 states
//            // 4 speed refresh, 3 function refresh
//            q_state = 0;
//        }
//    }
}

void send_idle(void) {
  // send an idle packet
  dcc_buff_m[0] = 0xff;
  dcc_buff_m[1] = 0;
  dcc_buff_m[2] = 0xff;
  dcc_bytes_m = 3;
  dcc_buff_m[6] = 1;                  // send once
  // hand off buffer
  dcc_flags.dcc_rdy_m = 0;
  idle_next = 0;
}

//
// disp_queue_add()
//
// Attempt to add an entry to the D queue
//
//This stores a loco session once a loco has been dispatched for retrieval later on if enabled
//uses the same session position as the main stack to make processing easier
//
void d_queue_add(unsigned char idx) {
    // Initialise the entry
    d_queue[idx].slot_idx = idx;
    // Ensure correct DCC address format
    //d_queue[free_handle].address.addr_hi.long0 = q_queue[free_handle].address.addr_hi.long1;
    d_queue[idx].status.valid = 1;
    d_queue[idx].timeout = dispatchTimeout;
}

//
// purge_dispatch
//
// Purge loco session from the dispatch queue
//
void purge_dispatch(unsigned char idx) {
    d_queue[idx].slot_idx = 0;
    d_queue[idx].status.valid = 0;
    d_queue[idx].timeout = 0;
}

void purge_allSessions(){
  //Purge all active sessions
  for (i = 0; i < MAX_HANDLES; i++) {
    if (q_queue[i].status.valid == 1) {
      rx_ptr.buf[1] = i;
      purge_session(i);
      purge_dispatch(i);  //remove session from dispatch
  
      Tx1.buf[0] = OPC_ERR;
      Tx1.buf[1] = i;
      Tx1.buf[2] = 0;
      Tx1.buf[3] = ERR_SESSION_CANCELLED;
      can_tx(4); 
    }
  }
  q_idx = 0;
  q_state = 0;
  
  noOfSessions = 0;
  
  if (analogOperationActive) {
    pinMode(DCC_NEG, OUTPUT);
    pinMode(DCC_POS, OUTPUT);
    analogOperationActive = 0;
    railcomEnabled = mode_word.railcom;
    //analogIcon();
    railComIcon();
  }
}

