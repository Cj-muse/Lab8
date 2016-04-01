/**********************************************************************
                         kbd.c file
***********************************************************************/
#include "header.h"
#define KEYBD	         0x60	/* I/O port for keyboard data */
#define PORT_B           0x61   /* port_B of 8255 */
#define KBIT	         0x80	/* bit used to ack characters to keyboard */

#define NSCAN             64	/* Number of scan codes */
#define KBLEN             64    // input buffer size

/*************************** keymap ***********************************/

 /* Scan codes to ASCII for unshifted keys; unused keys are left out */
char unshift[NSCAN] = { 
 0,033,'1','2','3','4','5','6',        '7','8','9','0','-','=','\b','\t',
 'q','w','e','r','t','y','u','i',      'o','p','[',']', '\r', 0,'a','s',
 'd','f','g','h','j','k','l',';',       0, 0, 0, 0,'z','x','c','v',
 'b','n','m',',','.','/', 0,'*',        0, ' '       
};

/* Scan codes to ASCII for shifted keys; unused keys are left out */
char shift[NSCAN] = {
 0,033,'!','@','#','$','%','^',        '&','*','(',')','_','+','\b','\t',
 'Q','W','E','R','T','Y','U','I',      'O','P','{','}', '\r', 0,'A','S',
 'D','F','G','H','J','K','L',':',       0,'~', 0,'|','Z','X','C','V',
 'B','N','M','<','>','?',0,'*',         0, ' '  
};


typedef struct kbd{           // data struct representing the keyboard
    char buf[KBLEN];          // input buffer
    int  head, tail;          // CIRCULAR buffer
    SEMAPHORE data;   // initialize data.value=0; data.queue=0;
}KBD;

KBD kbd;

int kbd_init()
{
	printf("kbinit()\n");
	kbd.head = kbd.tail = 0;
  	kbd.data.value = 0;
  	kbd.data.queue = 0;
  	printf("kbinit done\n"); 
}

/******************************************************************************
 kbhandler() is the kbd interrupt handler. The KBD generates 2 interrupts
 for each key typed; one when the key is pressed and another one when the
 key is released. Each key generates a scan code. The scan code of key
 release is 0x80 + the scan code of key pressed. When the kbd interrupts,the
 scan code is in the data port (0x60) of the KBD interface. Read the scan code
 from the data port. Then ack the key input by strobing its PORT_B at 0x61.
*****************************************************************************/

// need to update this function
int kbhandler()
{
  int scode, value, c;
  KBD *kp = &kbd;
  
  /* Fetch scan code from the keyboard hardware and acknowledge it. */
  scode = in_byte(KEYBD);	 /* get the scan code of the key */
  value = in_byte(PORT_B);	 /* strobe the keyboard to ack the char */
  out_byte(PORT_B, value | KBIT);/* first, strobe the bit high */
  out_byte(PORT_B, value);	 /* then strobe it low */

  printf("scode=%x    ", scode);  

  if (scode & 0x80)         // ignore key releases
     goto out;

  // translate scan code to ASCII; //using shift[ ] table if shifted;
  c = unshift[scode];

	//(2). Store the character c in kbd.buf[] for process to get
  	/* store the character in buf[ ] for process to get */
  	if (kbd.data >= KBLEN){   // buf[ ] already FULL
  		printf("%c\n", 0x07);
   	goto out;          
  	}

  	kbd.buf[kbd.head++] = c;
  	kbd.head %= KBLEN;

  	kbd.data++;           // inc data in buf[] by 1
  	//(3). V(&kbd.data);
  	kwakeup(&kbd.data);   // wakeup process in upper half

out:
  out_byte(0x20, 0x20); // issue EOI to 8259 Interrupt controller
}

/********************** upper half routine ***********************/ 
int getc()
{
 int c;
  P(&kbd.data);

  lock();
    c = kbd.buf[kbd.tail++] & 0x7F;
    kbd.tail %= KBLEN;
  unlock();

  return c;
}

