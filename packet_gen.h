#ifndef __PACKET_GEN_H

//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//

//
// The Refresh queue ( or Q queue) holds entries for loco
// control data that is continually sent to the track.
//
// 8 byte entries:
//				; byte 1: status
//				; byte 2: addr high
//              ; byte 3: addr low
//              ; byte 4: speed/dir in dcc 128 step format
//				; byte 5: Fn group 1 (F0 to F4)
//				; byte 6: Fn group 2 (F5 to F8)
//				; byte 7: Fn group 2a (F9 to F12)
//				; byte 8: slot timeout (if used)
//

// status byte
typedef union {
	struct {
		unsigned byte_count:3;
		unsigned :2;
		unsigned throttle_mode:2;
		unsigned valid:1;
	} ;
	unsigned char byte;
} slot_status;

// DCC address high byte
typedef union {
    // As six address bits plus two bits that should be high for
    // a long address
    struct {
        unsigned hi:6;
        unsigned long0:1;
        unsigned long1:1;
    } ;
    // as a single byte
    unsigned char byte;
} dcc_address_high;

// DCC address
typedef union {
    // the two bytes
    struct {
        unsigned char addr_lo;
        dcc_address_high addr_hi;
    } ;
    // the whole thing as an int
    unsigned int addr_int;
} dcc_address;

// A single queue entry
typedef struct refresh_queue {
	slot_status status;
	dcc_address address;
	unsigned char speed;
    unsigned char fn1;
    unsigned char fn2;
    unsigned char fn2a;
    unsigned char timeout;
} refresh_queue_t;

// The array of 32 entries for the loco queue
#define MAX_HANDLES 32
extern refresh_queue_t q_queue[MAX_HANDLES];
extern unsigned char q_idx;
extern unsigned char q_state;

// The send queue (or S queue) holds DCC formatted packets to
// be sent "immediately" to the track.
//
// Immediate packets include speed and funtion updates and service
// mode packets. In future they could include accessory control
// packets.
//
// 8 byte entries:
//				; byte 1: bit 8: status
//				; byte 1: bits 2:0: byte count
//				; byte 2:7: data
//				; byte 8: bits 2:0: repeat count
//

// A single queue entry
typedef struct dcc_queue {
	slot_status status;
    unsigned char d[6];
    unsigned char repeat;
} dcc_queue_t;

// The array of 16 entries for the temporary queue
//extern struct dcc_queue s_queue[16];
extern dcc_queue_t s_queue[16];
extern unsigned char s_head;
extern unsigned char s_tail;
extern unsigned char s_cnt;

void packet_gen(void);
void queue_add(void);
void throttle_mode(void);
void queue_update(void);
void query_session(void);
void purge_session(void);
void keep_alive(void);
void dcc_packet(void);
void em_stop(void);
void consist_add(void);
void consist_remove(void);
void ops_write(void);

#define __PACKET_GEN_H
#endif
