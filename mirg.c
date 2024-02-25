
#include <bcm2835.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/un.h>

// pins for various clock dividers
#define PIN1 RPI_BPLUS_GPIO_J8_31
#define PIN2 RPI_BPLUS_GPIO_J8_29
#define PIN4 RPI_BPLUS_GPIO_J8_15
#define PIN8 RPI_BPLUS_GPIO_J8_13
#define PIN16 RPI_BPLUS_GPIO_J8_11

// there are 12 clock ticks per 8th-note
#define CLOCK_DIV 12

// we want a 15 ms pulse but 30 seems a bit more stable
#define PULSE_LEN 30

// where should that come from?
#define UNIX_PATH_MAX    108

typedef enum play_state
{
	play_state_stopped,
	play_state_playing,
}
play_state;

int main(int argc, char **argv)
{

    openlog("slog", LOG_PID|LOG_CONS, LOG_USER);
    syslog(LOG_EMERG, "MIRG: setting up");    

	if(argc != 2)
	{
		syslog(LOG_EMERG, "MIRG: must define hardware midi port, e.g. \"sync hw:1,0,0\"\n");
		return 1;
	}
	
	int err;
	snd_rawmidi_t* midiin;
	const char* portname = argv[1];
	
	err = snd_rawmidi_open(&midiin, NULL, portname, SND_RAWMIDI_SYNC);
	if(err)
	{
	    syslog(LOG_EMERG, "MIRG: error opening midi port\n");
		return 1;
	}
	err = snd_rawmidi_read(midiin, NULL, 0);
	if(err)
	{
		syslog(LOG_EMERG, "MIRG: error initializing midi read\n");
		return 1;
	}
	
	if (!bcm2835_init())
	{
		syslog(LOG_EMERG, "MIRG: failed to init the GPIO library\n");
		return 1;
	}
		
    // SET UP SOCKET
    struct sockaddr_un server_address;
    int server_socket;
    socklen_t address_length;
    int n;

    // create socket
    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket < 0){
        syslog(LOG_EMERG, "MIRG: socket() failed\n");
        return 1;
    } 
    // set server socket address 
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, "/tmp/mirg_display_server.s");
    
    // connect to server 
    if (connect(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        syslog(LOG_EMERG, "MIRG: connect() failed\n");
    }
    
    
    // set up GPIO
	bcm2835_gpio_fsel(PIN1, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(PIN2, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(PIN4, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(PIN8, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(PIN16, BCM2835_GPIO_FSEL_OUTP);
	
	// Run main loop
	play_state state = play_state_stopped;
	uint8_t ticks = 0;
	uint8_t multiplier_cnt = 0;
    syslog(LOG_EMERG, "MIRG: entering main loop");
	while (1)
	{
		uint8_t status;
		int ret;
		if ((ret = snd_rawmidi_read(midiin, &status, 1)) < 0){
            syslog(LOG_EMERG, "MIRG: read error: %d - %s\n", (int)ret, snd_strerror(ret));
            syslog(LOG_EMERG, "MIRG: exiting after read error");
            return 1;
		}
		
		if(status & 0xF0) 
		{
			switch(status)
			{
				case 0xFA: // start
				{
				    printf("start\n");
					ticks = 0; // reset clock
					multiplier_cnt = 0;
					state = play_state_playing;
					break;
				}
				case 0xFB: // continue
				{
			        printf("continue\n");  
					ticks = 0; // reset clock
					multiplier_cnt = 0;
					state = play_state_playing;
					break;
				}
				case 0xFC: // stop
				{
			        printf("stop\n");  
					state = play_state_stopped;
					break;
				}
				case 0xF8: // clock tick
				{
			        //printf("tick\n");  
					if(ticks % CLOCK_DIV == 0) // 
					{
						ticks = 0; // reset clock
						if(state == play_state_playing)
						{
	    			        uint32_t mask = 1 << PIN1;
	    			        if (multiplier_cnt % 2 == 0) mask |= 1 << PIN2; 
	    			        if (multiplier_cnt % 4 == 0) mask |= 1 << PIN4; 
	    			        if (multiplier_cnt % 8 == 0) mask |= 1 << PIN8;
	    			        if (multiplier_cnt % 16 == 0) mask |= 1 << PIN16; 
	    			        
	    			        bcm2835_gpio_set_multi(mask);
							bcm2835_delay(PULSE_LEN);
	    			        bcm2835_gpio_clr_multi(mask);
	    			        printf("Sending to sock: %c\n", multiplier_cnt);
	    			        //n = write(server_socket, (char*)&multiplier_cnt, 1);
                            //if (n < 0) {
                            //    printf("errno is %i\n", n);
                                //syslog(LOG_EMERG, "MIRG: ERROR writing to socket\n");
                            //}           
                            break;      
	    			        
						}
						multiplier_cnt++;
					}
					
					if(state == play_state_playing)
						ticks++;
					
					break;
				}
				default:
					break;
			}
		}
	}
	bcm2835_close();
	return 0;
}
