// Module for interfacing with GPIOs

// Usage:
// komm.digest(request)

#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "module.h"
#include "user_interface.h"
#include "c_types.h"
#include "c_string.h"

#define PULLUP PLATFORM_GPIO_PULLUP
#define FLOAT PLATFORM_GPIO_FLOAT
#define OUTPUT PLATFORM_GPIO_OUTPUT
#define OPENDRAIN PLATFORM_GPIO_OPENDRAIN
#define INPUT PLATFORM_GPIO_INPUT
#define INTERRUPT PLATFORM_GPIO_INT
#define HIGH PLATFORM_GPIO_HIGH
#define LOW PLATFORM_GPIO_LOW

/*** KOMM core library ***/

//useful constants
#define KOMM_PVERSION 2

#define KOMM_PERROR -1
#define KOMM_PECHO -10
#define komm_checkprotocol(pver) (((pver)==0)||((pver)==2)) //Supports KOMM V2
#define komm_checklen(ptr,size) (*((char*)(ptr)+2) == ((size) - 4))
#define komm_checkcrc(ptr,size) 1 //dummy

#define KOMM_IO_NONE 0x00
#define KOMM_IO_AIN_S 0x01
#define KOMM_IO_AIN_NS 0x02
#define KOMM_IO_DIN 0x03
#define KOMM_IO_OC 0x04
#define KOMM_IO_RLY_5A_NONC 0x05
#define KOMM_IO_RLY_10A_NONC 0x06
#define KOMM_IO_RLY_16A_NONC 0x07
#define KOMM_IO_RLY_20A_NONC 0x08
#define KOMM_IO_RLY_5A_NO 0x09
#define KOMM_IO_RLY_10A_NO 0x0A
#define KOMM_IO_RLY_16A_NO 0x0B
#define KOMM_IO_RLY_20A_NO 0x0C

//CRC8 generation
char crc8_gen(const char *ptr, char size){
  return 0;
}
//Get Device Config
int komm_get_device_config(char *reply){
  *reply = 0x00;
  *(reply+1) = 0x01;
  *(reply+2) = 0x07;
  *(reply+3) = (char)KOMM_PVERSION;
  *(reply+4) = 0x00;
  *(reply+5) = 0x00;
  *(reply+6) = 0x00;
  *(reply+7) = 0x00;
  *(reply+8) = 0x00;
  *(reply+9) = 0x03;
  *(reply+10) = crc8_gen(reply,10);
  return 11;
}

#define KOMM_IOCONFNUM 13
#define KOMM_IONUM 10
char io_status[KOMM_IONUM] = {0,0,0,0,0,0,0,0,0,0};
//analog pins name scheme: analog pin number & 0x80
const char io_map[KOMM_IONUM] = {0x80,0,5,6,7,1,2,8,9,10};
#define KOMM_IOMAP_MASK 0x3F
//Set Device Config
//komm_set_device_config(){return KOMM_PECHO;}
//Get IO Configuration
int komm_get_io_config(char *reply){
  *reply = 0x02;
  *(reply+1) = 0x03;
  *(reply+2) = 0x05;
  *(reply+3) = (io_status[0]<<4) | (io_status[1]&0x0f);
  *(reply+4) = (io_status[2]<<4) | (io_status[3]&0x0f);
  *(reply+5) = (io_status[4]<<4) | (io_status[5]&0x0f);
  *(reply+6) = (io_status[6]<<4) | (io_status[7]&0x0f);
  *(reply+7) = (io_status[8]<<4) | (io_status[9]&0x0f);
  *(reply+8) = crc8_gen(reply,8);
  return 9;
}

//Set IO Configuration
int komm_set_io_config(char *reply,const char *config,char size){
  int i;
  int cend = (KOMM_IONUM > (size<<1))? size<<1 : KOMM_IONUM;
  for(i=0;i<cend;i++)
  {
    char nmode = (i%2)?(*(config+(i>>1))&0x0f):(*(config+(i>>1))>>4);
    //checking valid mode
    if(nmode!=io_status[i] && nmode<KOMM_IOCONFNUM && !(nmode && ((unsigned char)io_map[i]>>7)^(nmode<3))) //it does NOT support mixed io
    {
      //here made physical change to pin
      //very imcompatible with INTERRUPT mode on Lua!!
      switch(nmode)
      {
        case KOMM_IO_NONE:
          if(!(io_map[i]&0x80))
            platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],INPUT,FLOAT);
          //here write equivalent state for analog
          break;
        case KOMM_IO_AIN_S:
        case KOMM_IO_AIN_NS: break;
        case KOMM_IO_DIN:
          platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],INPUT,FLOAT); break;
        case KOMM_IO_OC:
          platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],OPENDRAIN,FLOAT); break;
        case KOMM_IO_RLY_5A_NONC:
        case KOMM_IO_RLY_10A_NONC:
        case KOMM_IO_RLY_16A_NONC:
        case KOMM_IO_RLY_20A_NONC:
        case KOMM_IO_RLY_5A_NO:
        case KOMM_IO_RLY_10A_NO:
        case KOMM_IO_RLY_16A_NO:
        case KOMM_IO_RLY_20A_NO:
          platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],OUTPUT,FLOAT); break;
        default: break;
      }
      //update io_status with new mode
      io_status[i] = nmode;
    }
  }
  reply[0]=0x02;
  reply[1]=0x04;
  reply[2]=size;
  for(i=0;i<size;i++) reply[3+i] = config[i];
  reply[3+i] = crc8_gen(reply,3+i);
  return 3+1+i;
}
//Get Analog Input Values
int komm_get_ain_values(char *reply){return KOMM_PECHO;}
//Get Analog Inputs Status
int komm_get_ain_status(char *reply){return KOMM_PECHO;}
//Set Analog Inputs Thresholds Common
int komm_set_ain_thresholds_common(char *reply,const char *data,char size){return KOMM_PECHO;}
//Get Analog Inputs Thresholds Common
int komm_get_ain_thresholds_common(char *reply){return KOMM_PECHO;}
//Set Analog Input Thresholds
int komm_set_ain_thresholds(char *reply,char addr,const char *data, char size){return KOMM_PECHO;}
//Get Analog Input Thresholds
int komm_get_ain_thresholds(char *reply,char addr){return KOMM_PECHO;}
//Set Digital Output Values
int komm_set_dout(char *reply,const char *data){return KOMM_PECHO;} //data size fixed to 4bytes
//Set Digital Output Value
int komm_set_dout_s(char *reply, char addr,char value){
  reply[0] = 0x02;
  reply[1] = 0x0C;
  reply[2] = 0x02;
  reply[3] = addr;
  if(addr-1 < KOMM_IONUM && io_status[addr-1]>=KOMM_IO_OC && io_status[addr-1]<=KOMM_IO_RLY_20A_NO)
    platform_gpio_write(KOMM_IOMAP_MASK & io_map[addr-1], value);
  reply[4] = (char) platform_gpio_read(KOMM_IOMAP_MASK & io_map[addr-1]);
  reply[5] = crc8_gen(reply,5);
  return 6;
}
//Get Digital Output Values
int komm_get_dout(char *reply){return KOMM_PECHO;}
//Get Outputs Clousures
int komm_get_clousure(char *reply){return KOMM_PECHO;}
//Get Output Clousures
int komm_get_clousure_s(char *reply, char addr){return KOMM_PECHO;}
//Get Digital Inputs
int komm_get_din(char *reply){return KOMM_PECHO;}
//Get Outputs Fail
//int komm_get_fail(){return KOMM_PECHO;}
//Reset
int komm_reset(char *reply){
  reply[0] = 0x02;
  reply[1] = 0x12;
  reply[2] = 0x01;
  reply[3] = 0x00; //add status check maybe??
  reply[4] = crc8_gen(reply,4);
  system_restart();
  return 5;
}
int komm_echo_function(char *reply,const char *request){
  return KOMM_PECHO; //return ECHO return value
}

int komm_zero_function(char *reply,char protocol,char cmd,char zero_cnt){
  int i;
  *reply = protocol;
  *(reply+1) = cmd;
  *(reply+2) = zero_cnt;
  for(i = 3;i < (zero_cnt + 3);i++)
  {
    *(reply + i) = 0;
  }
  *(reply+i) = crc8_gen(reply,zero_cnt+3); //assuming crc8 of the whole frame, header included(TBD)
  return i;
}

//parse function
int komm_digest(char *reply,const char *ptr,char size){
  //check header
  char rsize;
  if(!komm_checkprotocol(*ptr)) return KOMM_PERROR; //checks for correct protocol version
  if(!komm_checklen(ptr,size)) return KOMM_PERROR; //checks for correct len field.
  if(!komm_checkcrc(ptr,size)) return KOMM_PERROR; //checks CRC correctness
  char cmd = *(ptr+1);
  char len = *(ptr+2);
  const char *data_start = ptr + 3;
  switch(*ptr)
  {
    case 0x00:
      switch(cmd)
      {
        case 0x01:
          rsize = komm_get_device_config(reply); break;
        case 0x02: //set_device_config
          rsize = komm_echo_function(reply,ptr); break;
         default:
          rsize = komm_echo_function(reply,ptr);
      }
      break;
    case 0x02:
      switch(cmd)
      {
        case 0x03:
          rsize = komm_get_io_config(reply); break;
        case 0x04:
          rsize = komm_set_io_config(reply,data_start,len); break;
        case 0x05:
          rsize = komm_get_ain_values(reply); break;
        case 0x06:
          rsize = komm_get_ain_status(reply); break;
        case 0x07:
          rsize = komm_set_ain_thresholds_common(reply,data_start,len); break;
        case 0x08:
          rsize = komm_get_ain_thresholds_common(reply); break;
        case 0x09:
          rsize = komm_set_ain_thresholds(reply,*(data_start),data_start+1,len-1); break;
        case 0x0A:
          rsize = komm_get_ain_thresholds(reply,*data_start); break;
        case 0x0B:
          rsize = komm_set_dout(reply,data_start); break;
        case 0x0C:
          rsize = komm_set_dout_s(reply,*data_start,*(data_start+1)); break;
        case 0x0D:
          rsize = komm_get_dout(reply); break;
        case 0x0E:
          rsize = komm_get_clousure(reply);
        case 0x0F:
          rsize = komm_get_clousure_s(reply,*data_start); break;
        case 0x10:
          rsize = komm_get_din(reply); break;
        case 0x11: //get_fail
          rsize = komm_zero_function(reply,2,0x11,4); break;
        case 0x12:
          rsize = komm_reset(reply); break;
        default: //unknown function. return echo.
          rsize = komm_echo_function(reply,ptr);
      }
      break;
    default: rsize = komm_echo_function(reply,ptr);
  }
  //handle return signal
  return rsize;
}

/*** KOMM core library END ***/

// Lua: write( id, string1, [string2], ..., [stringn] )
static int Lkomm_digest( lua_State* L )
{
  char reply[255];
  const char* ptr;
  size_t len,rlen;
  luaL_checktype( L, 1, LUA_TSTRING );
  ptr = lua_tolstring( L, 1, &len );
  rlen = komm_digest(reply,ptr,len);
  if(rlen > 0)
  {
    lua_pushlstring(L, reply,rlen);
  }
  else if(rlen == KOMM_PECHO)
  {
    //c_memcpy(reply,ptr,len);
    int i;
    for(i=0;i<len;i++) *(reply+i)=*(ptr+i);
    lua_pushlstring(L, reply,len);
  }
  else lua_pushlstring(L,reply,0); //empty string(maybe)
  return 1;
}

static int Lkomm_info( lua_State* L)
{
    char buf[255];
    c_sprintf(buf, "KOMM Server Version %d", KOMM_PVERSION);
    lua_pushfstring(L, buf);
    return 1;
}

static unsigned i=0;
static int Lkomm_test( lua_State* L)
{
    i=!i;
    platform_gpio_write(2, i);
    lua_pushinteger(L, i);
    return 1;
}

// Module function map, this is how we tell Lua what API our module has
const LUA_REG_TYPE komm_map[] =
{
  { LSTRKEY( "info" ), LFUNCVAL( Lkomm_info ) },
  { LSTRKEY( "digest" ), LFUNCVAL( Lkomm_digest ) },
  { LSTRKEY( "test" ), LFUNCVAL( Lkomm_test ) },
  { LNILKEY, LNILVAL } // This map must always end like this
};

NODEMCU_MODULE(KOMM, "komm", komm_map, NULL);
